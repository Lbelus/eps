"""
Conversational RAG over the Epstein/Maxwell document database.

The model drives retrieval agentically: it searches the FTS5 index, locates
pages inside matching documents, reads the page text, and answers with
filename + page citations. Conversation history is kept for follow-ups.

Two backends, selected with RAG_PROVIDER:
  - "anthropic" (default): Claude via the Anthropic API
  - "openai": any OpenAI-compatible server — Ollama, vLLM, LM Studio, etc.

Environment variables:
  RAG_PROVIDER   anthropic | openai
  RAG_MODEL      model id (defaults: claude-opus-4-8 / qwen3:8b)
  RAG_BASE_URL   openai provider only (default http://localhost:11434/v1)
  RAG_API_KEY    openai provider only (default "ollama"; local servers ignore it)
"""

import json
import os
import sqlite3

import anthropic
import openai

from core.database import init_db

PROVIDER = os.environ.get("RAG_PROVIDER", "anthropic")
MODEL = os.environ.get("RAG_MODEL") or ("claude-opus-4-8" if PROVIDER == "anthropic" else "qwen3:8b")
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
search_documents to find candidate documents, find_pages to locate the relevant \
pages inside a document, read_pages to read them before making claims. \
Try multiple search phrasings (names, aliases, OCR-tolerant variants) before \
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
        if name == "find_pages":
            return _find_pages(conn, tool_input["filename"], tool_input["term"]), False
        if name == "read_pages":
            return _read_pages(conn, tool_input["filename"], tool_input["pages"]), False
        return f"Unknown tool: {name}", True
    except sqlite3.OperationalError as e:
        # Bad FTS5 syntax etc. — hand the error back so the model can rephrase.
        return f"Query error: {e}", True


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


class _ThinkFilter:
    """Suppress inline <think>...</think> reasoning some open models stream."""

    def __init__(self):
        self._buffer = ""
        self._thinking = False

    def feed(self, delta: str) -> str:
        self._buffer += delta
        out = []
        while self._buffer:
            if self._thinking:
                end = self._buffer.find("</think>")
                if end == -1:
                    self._buffer = self._buffer[-8:]  # keep a partial-tag tail
                    break
                self._buffer = self._buffer[end + len("</think>"):]
                self._thinking = False
            else:
                start = self._buffer.find("<think>")
                if start == -1:
                    # Hold back a potential partial "<think" prefix at the end.
                    safe = len(self._buffer)
                    for k in range(1, min(len("<think>"), safe) + 1):
                        if "<think>".startswith(self._buffer[safe - k:]):
                            safe -= k
                            break
                    out.append(self._buffer[:safe])
                    self._buffer = self._buffer[safe:]
                    break
                out.append(self._buffer[:start])
                self._buffer = self._buffer[start + len("<think>"):]
                self._thinking = True
        return "".join(out)


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
