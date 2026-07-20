"""
FastAPI wrapper exposing the RAG chat as a Server-Sent Events streaming endpoint.

Reads data/epstein.db directly (no MySQL / rest_api needed). Conversation
history — including tool_use/tool_result blocks — is held server-side in
memory, keyed by session_id, so follow-up questions keep their retrieval
context. Sessions are lost on restart; this is a local dev service.

Run from scan_manager/:
    .venv/bin/uvicorn --app-dir src api.rag_server:app --port 8000
"""

import json
import secrets
import threading

import anthropic
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import StreamingResponse
from pydantic import BaseModel

from core.database import init_db
from core.rag import stream_turn

DB_PATH = "./data/epstein.db"

app = FastAPI(title="EPS RAG chat")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:3000", "http://127.0.0.1:3000"],
    allow_methods=["*"],
    allow_headers=["*"],
)

client = anthropic.Anthropic()

_sessions: dict[str, list] = {}
_busy: set[str] = set()
_lock = threading.Lock()


class ChatRequest(BaseModel):
    message: str
    session_id: str | None = None


def _sse(payload: dict) -> str:
    return f"data: {json.dumps(payload)}\n\n"


@app.get("/rag/health")
def health():
    conn = init_db(DB_PATH)
    try:
        n_docs, n_pages = conn.execute(
            "SELECT (SELECT count(*) FROM documents), (SELECT count(*) FROM pages)"
        ).fetchone()
    finally:
        conn.close()
    return {"status": "ok", "documents": n_docs, "pages": n_pages}


@app.delete("/rag/session/{session_id}")
def reset_session(session_id: str):
    with _lock:
        _sessions.pop(session_id, None)
    return {"status": "ok"}


@app.post("/rag/chat")
def chat(req: ChatRequest):
    message = req.message.strip()
    if not message:
        raise HTTPException(status_code=400, detail="message is empty")

    with _lock:
        session_id = req.session_id or secrets.token_hex(8)
        if session_id in _busy:
            raise HTTPException(status_code=409, detail="session is already answering a question")
        _busy.add(session_id)
        messages = _sessions.setdefault(session_id, [])

    def event_stream():
        conn = init_db(DB_PATH)
        turn_start = len(messages)
        messages.append({"role": "user", "content": message})
        try:
            yield _sse({"type": "session", "session_id": session_id})
            for event in stream_turn(client, conn, messages):
                yield _sse(event)
            yield _sse({"type": "done"})
        except anthropic.APIError as e:
            # Roll back the failed turn so the session history stays valid.
            del messages[turn_start:]
            yield _sse({"type": "error", "message": str(e)})
        finally:
            conn.close()
            with _lock:
                _busy.discard(session_id)

    return StreamingResponse(
        event_stream(),
        media_type="text/event-stream",
        headers={"Cache-Control": "no-store", "X-Accel-Buffering": "no"},
    )
