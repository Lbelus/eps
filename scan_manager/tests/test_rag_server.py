"""FastAPI SSE server: endpoints, session handling, and error rollback.

stream_turn is stubbed — no model calls are made.
"""

import json
import os

import openai
import pytest
from fastapi.testclient import TestClient

os.environ.setdefault("ANTHROPIC_API_KEY", "test-key")  # make_client() runs at import

import api.rag_server as srv


def sse_events(response):
    return [
        json.loads(line[len("data: "):])
        for line in response.text.split("\n\n")
        if line.startswith("data: ")
    ]


@pytest.fixture()
def app_client(db_path, monkeypatch):
    monkeypatch.setattr(srv, "DB_PATH", db_path)
    monkeypatch.setattr(srv, "_sessions", {})
    monkeypatch.setattr(srv, "_busy", set())

    def fake_stream_turn(client, conn, messages):
        yield {"type": "tool", "name": "search_documents", "input": {"query": "x"}}
        yield {"type": "text", "text": "answer"}
        messages.append({"role": "assistant", "content": "answer"})

    monkeypatch.setattr(srv, "stream_turn", fake_stream_turn)
    return TestClient(srv.app)


def test_health_reports_corpus_and_model(app_client):
    body = app_client.get("/rag/health").json()
    assert body["status"] == "ok"
    assert body["documents"] == 3
    assert body["pages"] == 6
    assert "provider" in body and "model" in body


def test_chat_streams_events_in_order(app_client):
    response = app_client.post("/rag/chat", json={"message": "hi"})
    assert response.status_code == 200
    assert response.headers["content-type"].startswith("text/event-stream")
    events = sse_events(response)
    assert events[0]["type"] == "session"
    assert [e["type"] for e in events[1:]] == ["tool", "text", "done"]


def test_empty_message_rejected(app_client):
    assert app_client.post("/rag/chat", json={"message": "   "}).status_code == 400


def test_session_continuity(app_client):
    first = sse_events(app_client.post("/rag/chat", json={"message": "one"}))
    session_id = first[0]["session_id"]
    app_client.post("/rag/chat", json={"message": "two", "session_id": session_id})
    history = srv._sessions[session_id]
    # Two turns: (user, assistant) x 2, in order.
    assert [m["role"] for m in history] == ["user", "assistant", "user", "assistant"]
    assert history[0]["content"] == "one" and history[2]["content"] == "two"


def test_busy_session_conflicts(app_client):
    srv._busy.add("sess1")
    response = app_client.post("/rag/chat", json={"message": "hi", "session_id": "sess1"})
    assert response.status_code == 409


def test_provider_error_rolls_back_turn(app_client, monkeypatch):
    def failing_stream_turn(client, conn, messages):
        yield {"type": "text", "text": "partial"}
        raise openai.OpenAIError("model exploded")

    monkeypatch.setattr(srv, "stream_turn", failing_stream_turn)
    response = app_client.post("/rag/chat", json={"message": "hi", "session_id": "sess2"})
    events = sse_events(response)
    assert events[-1]["type"] == "error"
    assert srv._sessions["sess2"] == []       # failed turn removed from history
    assert "sess2" not in srv._busy           # lock released


def test_reset_session(app_client):
    first = sse_events(app_client.post("/rag/chat", json={"message": "one"}))
    session_id = first[0]["session_id"]
    assert app_client.delete(f"/rag/session/{session_id}").status_code == 200
    assert session_id not in srv._sessions
