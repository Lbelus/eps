"""
Conversational RAG over the Epstein/Maxwell document database.

The model drives retrieval agentically: it searches the FTS5 index, locates
pages inside matching documents, reads the page text, and answers with
filename + page citations. Conversation history is kept for follow-ups.

Two backends, selected with RAG_PROVIDER:
  - "openai" (default): any OpenAI-compatible server — Ollama, vLLM, LM Studio,
    etc. This project runs on free, open-source models via Ollama by default.
  - "anthropic": Claude via the paid Anthropic API (opt in with RAG_PROVIDER=anthropic)

Environment variables:
  RAG_PROVIDER   openai | anthropic
  RAG_MODEL      model id (defaults: qwen3:4b / claude-opus-4-8)
  RAG_BASE_URL   openai provider only (default http://localhost:11434/v1)
  RAG_API_KEY    openai provider only (default "ollama"; local servers ignore it)
"""

import json
import os
import sqlite3

import anthropic
import openai
import requests

from core import embeddings
from core.database import init_db

PROVIDER = os.environ.get("RAG_PROVIDER", "openai")
MODEL = os.environ.get("RAG_MODEL") or ("claude-opus-4-8" if PROVIDER == "anthropic" else "qwen3:4b")
BASE_URL = os.environ.get("RAG_BASE_URL", "http://localhost:11434/v1")
API_KEY = os.environ.get("RAG_API_KEY", "ollama")
MAX_TOKENS = 16000

ProviderError = (anthropic.APIError, openai.OpenAIError)


def make_client():
    if PROVIDER == "anthropic":
        return anthropic.Anthropic()
    if PROVIDER == "openai":
        return openai.OpenAI(base_url=BASE_URL, api_key=API_KEY)
    raise ValueError(f"Unknown RAG_PROVIDER: {PROVIDER}")
# Local models pay heavily for prompt processing — keep retrieval payloads
# smaller there so each tool round-trip stays fast.
if PROVIDER == "openai":
    MAX_PAGES_PER_READ = 3
    MAX_CHARS_PER_PAGE = 4000
else:
    MAX_PAGES_PER_READ = 5
    MAX_CHARS_PER_PAGE = 8000

SYSTEM_PROMPT = """You are a research assistant for a corpus of public court filings and \
government releases related to the Jeffrey Epstein and Ghislaine Maxwell cases \
(DOJ disclosures, CourtListener RECAP filings, DocumentCloud documents). \
The corpus is OCR'd, so text may contain recognition errors.

Answer questions using ONLY what you retrieve with your tools. Workflow: \
search_documents (exact keywords) or semantic_search (concepts/paraphrase) to \
find candidates, find_pages to locate the relevant pages inside a document, \
read_pages to read them before making claims. Use search_documents for names, \
case numbers, and exact phrases; use semantic_search when the wording is \
uncertain or conceptual. Try multiple phrasings and both search tools before \
concluding something is absent from the corpus.

Cite every claim as (filename, p. N). If retrieval turns up nothing relevant, \
say so plainly rather than answering from general knowledge — and clearly label \
any background context that does not come from the corpus.

The search tool uses SQLite FTS5 MATCH syntax: bare terms are ANDed; use OR, \
NEAR(a b, N), quoted "exact phrases", and prefix* matching as needed."""

# Small local models need blunter guardrails than Claude: without them they
# skip retrieval and fabricate citations.
if PROVIDER == "openai":
    SYSTEM_PROMPT += (
        "\n\nIMPORTANT: You MUST call search_documents before answering any question "
        "about the cases or documents — never answer from memory. Only cite filenames "
        "and page numbers that appeared in a tool result this conversation; never "
        "invent or placeholder a citation."
    )

# Reasoning models (qwen3, gpt-oss, ...) generate lengthy hidden reasoning
# before every answer, which dominates local latency — a one-sentence answer
# can cost thousands of invisible tokens. Default it off; set RAG_REASONING
# to low/medium/high to trade latency for answer quality, or "" to omit the
# parameter for servers that reject it.
REASONING_EFFORT = os.environ.get("RAG_REASONING", "none")

# With reasoning off, small models sometimes skip retrieval and answer from
# memory with invented citations. Forcing a tool call on the first round of
# each turn prevents that; set RAG_FORCE_SEARCH=0 to disable.
FORCE_TOOL_FIRST = os.environ.get("RAG_FORCE_SEARCH", "1") == "1"

# Low temperature makes tool-calling far more reliable on small local models
# (at the default ~0.8, qwen3 skips forced tool calls often enough to matter).
TEMPERATURE = float(os.environ.get("RAG_TEMPERATURE", "0.2"))

TOOLS = [
    {
        "name": "search_documents",
        "description": (
            "Full-text search over all documents (FTS5). Returns matching documents "
            "with a short snippet around the first hit. Call this when the answer "
            "depends on document content you have not already read this conversation. "
            "Query syntax: implicit AND between terms, OR, NEAR(a b, N), "
            "\"quoted phrases\", prefix*."
        ),
        "input_schema": {
            "type": "object",
            "properties": {
                "query": {"type": "string", "description": "FTS5 MATCH query"},
                "limit": {"type": "integer", "description": "Max results (default 10)"},
            },
            "required": ["query"],
        },
    },
    {
        "name": "semantic_search",
        "description": (
            "Semantic similarity search over individual pages — finds pages about a "
            "concept even when the wording differs (paraphrases, synonyms, OCR noise). "
            "Best for conceptual questions; for exact names, case numbers, or phrases "
            "prefer search_documents. Returns pages with filenames and page numbers."
        ),
        "input_schema": {
            "type": "object",
            "properties": {
                "query": {"type": "string", "description": "Natural-language description of what to find"},
                "limit": {"type": "integer", "description": "Max pages (default 8)"},
            },
            "required": ["query"],
        },
    },
    {
        "name": "find_pages",
        "description": (
            "Locate pages inside one document whose text contains a term "
            "(case-insensitive substring match). Returns page numbers with a short "
            "excerpt so you know which pages to read."
        ),
        "input_schema": {
            "type": "object",
            "properties": {
                "filename": {"type": "string", "description": "Exact filename from search_documents"},
                "term": {"type": "string", "description": "Substring to locate"},
            },
            "required": ["filename", "term"],
        },
    },
    {
        "name": "read_pages",
        "description": (
            f"Read the full OCR text of specific pages of one document "
            f"(max {MAX_PAGES_PER_READ} pages per call). Use before quoting or "
            "asserting anything specific."
        ),
        "input_schema": {
            "type": "object",
            "properties": {
                "filename": {"type": "string", "description": "Exact filename from search_documents"},
                "pages": {
                    "type": "array",
                    "items": {"type": "integer"},
                    "description": "Page numbers to read",
                },
            },
            "required": ["filename", "pages"],
        },
    },
]


def _search_documents(conn: sqlite3.Connection, query: str, limit: int = 10) -> str:
    cursor = conn.execute(
        """
        SELECT d.filename, d.source, d.page_count,
               snippet(documents_fts, 0, '>>>', '<<<', '...', 32) AS snip
        FROM documents_fts f
        JOIN documents d ON d.document_id = f.rowid
        WHERE documents_fts MATCH ?
        ORDER BY rank
        LIMIT ?
        """,
        (query, min(limit, 25)),
    )
    rows = cursor.fetchall()
    if not rows:
        return "No documents matched. Try different terms, OR-ed variants, or prefix*."
    lines = [f"{len(rows)} documents matched '{query}':"]
    for filename, source, page_count, snip in rows:
        snip = " ".join(snip.split())
        lines.append(f"- {filename} (source={source}, {page_count} pages): {snip}")
    return "\n".join(lines)


_semantic_index: "embeddings.SemanticIndex | None" = None


def _semantic_search(conn: sqlite3.Connection, query: str, limit: int = 8) -> str:
    global _semantic_index
    if _semantic_index is None:
        _semantic_index = embeddings.SemanticIndex(conn)
    if not len(_semantic_index):
        return ("Semantic index not built yet — it is unavailable. "
                "Use search_documents instead. (Operator: run --mode embed.)")
    hits = _semantic_index.search(query, min(limit, 20))
    placeholders = ",".join("?" * len(hits))
    rows = conn.execute(
        f"""
        SELECT p.page_id, d.filename, p.page_number, substr(p.page_text, 1, 240)
        FROM pages p JOIN documents d ON d.document_id = p.document_id
        WHERE p.page_id IN ({placeholders})
        """,
        [page_id for page_id, _ in hits],
    ).fetchall()
    by_id = {row[0]: row for row in rows}
    lines = [f"{len(hits)} semantically similar pages for '{query}':"]
    for page_id, score in hits:
        _, filename, page_number, excerpt = by_id[page_id]
        excerpt = " ".join(excerpt.split())
        lines.append(f"- {filename} p. {page_number} (score {score:.2f}): {excerpt}...")
    return "\n".join(lines)


def _find_pages(conn: sqlite3.Connection, filename: str, term: str) -> str:
    like = f"%{term}%"
    cursor = conn.execute(
        """
        SELECT p.page_number, substr(p.page_text, max(1, instr(lower(p.page_text), lower(?)) - 120), 300)
        FROM pages p
        JOIN documents d ON d.document_id = p.document_id
        WHERE d.filename = ? AND p.page_text LIKE ?
        ORDER BY p.page_number
        LIMIT 20
        """,
        (term, filename, like),
    )
    rows = cursor.fetchall()
    if not rows:
        exists = conn.execute(
            "SELECT 1 FROM documents WHERE filename = ?", (filename,)
        ).fetchone()
        if not exists:
            return f"No document named '{filename}' in the database."
        return f"'{term}' not found in any page of {filename}."
    lines = [f"'{term}' appears on {len(rows)} page(s) of {filename} (max 20 shown):"]
    for page_number, excerpt in rows:
        excerpt = " ".join(excerpt.split())
        lines.append(f"- p. {page_number}: ...{excerpt}...")
    return "\n".join(lines)


def _read_pages(conn: sqlite3.Connection, filename: str, pages: list[int]) -> str:
    pages = pages[:MAX_PAGES_PER_READ]
    placeholders = ",".join("?" * len(pages))
    cursor = conn.execute(
        f"""
        SELECT p.page_number, p.page_text
        FROM pages p
        JOIN documents d ON d.document_id = p.document_id
        WHERE d.filename = ? AND p.page_number IN ({placeholders})
        ORDER BY p.page_number
        """,
        (filename, *pages),
    )
    rows = cursor.fetchall()
    if not rows:
        return f"No pages {pages} found for '{filename}'. Check the filename and page numbers."
    parts = []
    for page_number, page_text in rows:
        if len(page_text) > MAX_CHARS_PER_PAGE:
            page_text = page_text[:MAX_CHARS_PER_PAGE] + f"\n[page truncated at {MAX_CHARS_PER_PAGE} chars]"
        parts.append(f"=== {filename} p. {page_number} ===\n{page_text}")
    return "\n\n".join(parts)


def _execute_tool(conn: sqlite3.Connection, name: str, tool_input: dict) -> tuple[str, bool]:
    """Run one tool call. Returns (result_text, is_error)."""
    try:
        if name == "search_documents":
            return _search_documents(conn, tool_input["query"], tool_input.get("limit", 10)), False
        if name == "semantic_search":
            return _semantic_search(conn, tool_input["query"], tool_input.get("limit", 8)), False
        if name == "find_pages":
            return _find_pages(conn, tool_input["filename"], tool_input["term"]), False
        if name == "read_pages":
            return _read_pages(conn, tool_input["filename"], tool_input["pages"]), False
        return f"Unknown tool: {name}", True
    except sqlite3.OperationalError as e:
        # Bad FTS5 syntax etc. — hand the error back so the model can rephrase.
        return f"Query error: {e}", True
    except requests.RequestException as e:
        # Embedding server unreachable — tell the model to fall back to FTS.
        return f"semantic_search unavailable (embedding server error: {e}). Use search_documents.", True


def stream_turn(client, conn: sqlite3.Connection, messages: list):
    """One user turn as an event generator, executing tool calls until the model is done.

    Yields {"type": "text", "text": ...} for answer deltas and
    {"type": "tool", "name": ..., "input": ...} before each tool call.
    Appends assistant/tool-result turns to `messages` as it goes
    (message format is provider-specific — don't mix providers in one session).
    """
    if PROVIDER == "openai":
        yield from _stream_turn_openai(client, conn, messages)
    else:
        yield from _stream_turn_anthropic(client, conn, messages)


def _stream_turn_anthropic(client: anthropic.Anthropic, conn: sqlite3.Connection, messages: list):
    while True:
        with client.messages.stream(
            model=MODEL,
            max_tokens=MAX_TOKENS,
            system=[{
                "type": "text",
                "text": SYSTEM_PROMPT,
                "cache_control": {"type": "ephemeral"},
            }],
            thinking={"type": "adaptive"},
            tools=TOOLS,
            messages=messages,
        ) as stream:
            for text in stream.text_stream:
                yield {"type": "text", "text": text}
            response = stream.get_final_message()

        messages.append({"role": "assistant", "content": response.content})

        if response.stop_reason != "tool_use":
            return

        tool_results = []
        for block in response.content:
            if block.type != "tool_use":
                continue
            yield {"type": "tool", "name": block.name, "input": block.input}
            result, is_error = _execute_tool(conn, block.name, block.input)
            tool_results.append({
                "type": "tool_result",
                "tool_use_id": block.id,
                "content": result,
                "is_error": is_error,
            })
        messages.append({"role": "user", "content": tool_results})


def _openai_tools() -> list[dict]:
    return [
        {
            "type": "function",
            "function": {
                "name": t["name"],
                "description": t["description"],
                "parameters": t["input_schema"],
            },
        }
        for t in TOOLS
    ]


def _partial_tail(s: str, tag: str) -> int:
    """Length of the longest suffix of `s` that is a proper prefix of `tag`.

    Used to hold back a tag that may be split across streaming deltas (e.g. `s`
    ends in "</thin" — don't emit it yet, the "k>" may arrive next).
    """
    for k in range(min(len(tag) - 1, len(s)), 0, -1):
        if tag.startswith(s[-k:]):
            return k
    return 0


class _ThinkFilter:
    """Strip a model's reasoning from the streamed answer text.

    Handles two shapes:
      * an explicit ``<think>...</think>`` block anywhere in the stream, and
      * a block auto-opened by the chat template (Ollama/qwen3 do this), where
        only the closing ``</think>`` reaches us with no visible opener.

    The second shape is why leading text must be withheld: reasoning that starts
    the stream has no opener to mark it, so we hold everything until we either
    see a ``</think>`` (the held text was reasoning — discard it) or the stream
    ends (a model that never reasons — flush it as the answer). Callers MUST call
    ``flush()`` once the stream is exhausted to release any held answer text.
    """

    _OPEN = "<think>"
    _CLOSE = "</think>"

    def __init__(self):
        self._buffer = ""
        self._thinking = False
        self._resolved = False  # settled whether the stream opened mid-reasoning?

    def feed(self, delta: str) -> str:
        self._buffer += delta
        out = []
        while self._buffer:
            if not self._resolved:
                close = self._buffer.find(self._CLOSE)
                open_ = self._buffer.find(self._OPEN)
                if close != -1 and (open_ == -1 or close < open_):
                    # Orphan closer first: the leading text was reasoning. Drop it.
                    self._buffer = self._buffer[close + len(self._CLOSE):]
                    self._resolved = True
                    continue
                if open_ != -1:
                    # Explicit opener present — hand off to the normal path below.
                    self._resolved = True
                    continue
                # No tag yet; the buffer could still be leading reasoning. Hold it
                # all (a split trailing tag stays buffered for the next delta).
                break
            if self._thinking:
                end = self._buffer.find(self._CLOSE)
                if end == -1:
                    keep = _partial_tail(self._buffer, self._CLOSE)
                    self._buffer = self._buffer[len(self._buffer) - keep:] if keep else ""
                    break
                self._buffer = self._buffer[end + len(self._CLOSE):]
                self._thinking = False
            else:
                start = self._buffer.find(self._OPEN)
                if start == -1:
                    safe = len(self._buffer) - _partial_tail(self._buffer, self._OPEN)
                    out.append(self._buffer[:safe])
                    self._buffer = self._buffer[safe:]
                    break
                out.append(self._buffer[:start])
                self._buffer = self._buffer[start + len(self._OPEN):]
                self._thinking = True
        return "".join(out)

    def flush(self) -> str:
        """Release held text at stream end. Discards an unterminated think block."""
        # Unresolved + not thinking → the stream had no think tags at all, so the
        # held buffer was the answer. Otherwise we ended mid-reasoning: discard.
        tail = self._buffer if (not self._resolved and not self._thinking) else ""
        self._buffer = ""
        return tail


def _stream_turn_openai(client: "openai.OpenAI", conn: sqlite3.Connection, messages: list):
    reasoning_kwargs = {"reasoning_effort": REASONING_EFFORT} if REASONING_EFFORT else {}
    force_search = FORCE_TOOL_FIRST
    nudged = False
    while True:
        stream = client.chat.completions.create(
            model=MODEL,
            max_tokens=MAX_TOKENS,
            messages=[{"role": "system", "content": SYSTEM_PROMPT}] + messages,
            tools=_openai_tools(),
            tool_choice="required" if force_search else "auto",
            temperature=TEMPERATURE,
            stream=True,
            **reasoning_kwargs,
        )
        think_filter = _ThinkFilter()
        content_parts = []
        tool_calls: dict[int, dict] = {}
        for chunk in stream:
            if not chunk.choices:
                continue
            delta = chunk.choices[0].delta
            if delta.content:
                visible = think_filter.feed(delta.content)
                if visible:
                    content_parts.append(visible)
                    # While forcing retrieval, hold text back — if the model
                    # answered without searching we discard it and retry.
                    if not force_search:
                        yield {"type": "text", "text": visible}
            for tc in delta.tool_calls or []:
                entry = tool_calls.setdefault(tc.index, {"id": "", "name": "", "arguments": ""})
                if tc.id:
                    entry["id"] = tc.id
                if tc.function:
                    entry["name"] += tc.function.name or ""
                    entry["arguments"] += tc.function.arguments or ""

        tail = think_filter.flush()  # release any answer text held past a think block
        if tail:
            content_parts.append(tail)
            if not force_search:
                yield {"type": "text", "text": tail}

        # Some servers (Ollama) treat tool_choice="required" as advisory —
        # enforce it here: throw away the ungrounded answer and demand
        # retrieval once, then accept whatever comes back.
        if force_search and not tool_calls and not nudged:
            nudged = True
            messages.append({
                "role": "system",
                "content": "Do not answer from memory. Call search_documents for the "
                           "user's question before answering, and cite only filenames "
                           "returned by tools.",
            })
            continue
        if force_search:
            for part in content_parts:  # flush text withheld during the forced round
                yield {"type": "text", "text": part}
        force_search = False

        assistant_msg = {"role": "assistant", "content": "".join(content_parts)}
        if tool_calls:
            assistant_msg["tool_calls"] = [
                {
                    "id": entry["id"] or f"call_{index}",
                    "type": "function",
                    "function": {"name": entry["name"], "arguments": entry["arguments"]},
                }
                for index, entry in sorted(tool_calls.items())
            ]
        messages.append(assistant_msg)

        if not tool_calls:
            return

        for call in assistant_msg["tool_calls"]:
            name = call["function"]["name"]
            try:
                tool_input = json.loads(call["function"]["arguments"] or "{}")
            except json.JSONDecodeError as e:
                result = f"Invalid tool arguments (not JSON): {e}"
            else:
                yield {"type": "tool", "name": name, "input": tool_input}
                result, is_error = _execute_tool(conn, name, tool_input)
                if is_error:
                    result = f"ERROR: {result}"
            messages.append({"role": "tool", "tool_call_id": call["id"], "content": result})


def run_turn(client, conn: sqlite3.Connection, messages: list) -> None:
    """CLI consumer of stream_turn: print deltas and tool-call notices."""
    for event in stream_turn(client, conn, messages):
        if event["type"] == "text":
            print(event["text"], end="", flush=True)
        elif event["type"] == "tool":
            print(f"\n  [{event['name']}: {event['input']}]", flush=True)
    print()


def chat_cli(db_path: str):
    conn = init_db(db_path)
    n_docs, n_pages = conn.execute(
        "SELECT (SELECT count(*) FROM documents), (SELECT count(*) FROM pages)"
    ).fetchone()
    client = make_client()

    print(f"RAG chat over {n_docs:,} documents / {n_pages:,} pages "
          f"({PROVIDER}: {MODEL}). 'exit' or Ctrl-D to quit.\n")
    messages = []
    while True:
        try:
            user_input = input("you> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break
        if not user_input:
            continue
        if user_input.lower() in {"exit", "quit"}:
            break
        turn_start = len(messages)
        messages.append({"role": "user", "content": user_input})
        try:
            run_turn(client, conn, messages)
        except ProviderError as e:
            # Drop the failed turn so history stays valid for the next question.
            print(f"\n[API error: {e}]")
            del messages[turn_start:]
        print()
    conn.close()
