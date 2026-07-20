"""
Conversational RAG over the Epstein/Maxwell document database.

Claude drives retrieval agentically: it searches the FTS5 index, locates
pages inside matching documents, reads the page text, and answers with
filename + page citations. Conversation history is kept for follow-ups.
"""

import sqlite3

import anthropic

from core.database import init_db

MODEL = "claude-opus-4-8"
MAX_TOKENS = 16000
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


def stream_turn(client: anthropic.Anthropic, conn: sqlite3.Connection, messages: list):
    """One user turn as an event generator, executing tool calls until the model is done.

    Yields {"type": "text", "text": ...} for answer deltas and
    {"type": "tool", "name": ..., "input": ...} before each tool call.
    Appends assistant/tool-result turns to `messages` as it goes.
    """
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


def run_turn(client: anthropic.Anthropic, conn: sqlite3.Connection, messages: list) -> None:
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
    client = anthropic.Anthropic()

    print(f"RAG chat over {n_docs:,} documents / {n_pages:,} pages. 'exit' or Ctrl-D to quit.\n")
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
        except anthropic.APIError as e:
            # Drop the failed turn so history stays valid for the next question.
            print(f"\n[API error: {e}]")
            del messages[turn_start:]
        print()
    conn.close()
