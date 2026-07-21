"""The OpenAI-compatible agent loop against a fake streaming client.

Covers: tool-call assembly from streamed deltas, forced search on the first
round, the discard-and-nudge retry when the model answers from memory
(Ollama treats tool_choice="required" as advisory), and think-tag filtering.
"""

from types import SimpleNamespace

from core.rag import _stream_turn_openai


def text_chunk(text):
    return SimpleNamespace(
        choices=[SimpleNamespace(delta=SimpleNamespace(content=text, tool_calls=None))]
    )


def tool_chunk(name, arguments, call_id="call_1", index=0):
    call = SimpleNamespace(
        index=index,
        id=call_id,
        function=SimpleNamespace(name=name, arguments=arguments),
    )
    return SimpleNamespace(
        choices=[SimpleNamespace(delta=SimpleNamespace(content=None, tool_calls=[call]))]
    )


class FakeClient:
    """Returns queued responses; records every request's kwargs."""

    def __init__(self, responses):
        self.calls = []
        completions = SimpleNamespace(create=self._create)
        self.chat = SimpleNamespace(completions=completions)
        self._responses = list(responses)

    def _create(self, **kwargs):
        self.calls.append(kwargs)
        return iter(self._responses.pop(0))


def run(client, db, messages):
    return list(_stream_turn_openai(client, db, messages))


def test_search_then_answer(db):
    client = FakeClient([
        [tool_chunk("search_documents", '{"query": "flight"}')],
        [text_chunk("Found it "), text_chunk("(cl_flight.pdf, p. 1).")],
    ])
    messages = [{"role": "user", "content": "flight logs?"}]
    events = run(client, db, messages)

    assert {"type": "tool", "name": "search_documents", "input": {"query": "flight"}} in events
    assert "".join(e["text"] for e in events if e["type"] == "text") == "Found it (cl_flight.pdf, p. 1)."

    # First round forces retrieval, follow-up rounds don't.
    assert client.calls[0]["tool_choice"] == "required"
    assert client.calls[1]["tool_choice"] == "auto"

    # History: user, assistant+tool_calls, tool result, final assistant.
    roles = [m["role"] for m in messages]
    assert roles == ["user", "assistant", "tool", "assistant"]
    assert "cl_flight.pdf" in messages[2]["content"]  # real FTS result fed back


def test_tool_call_assembled_from_split_deltas(db):
    client = FakeClient([
        [
            tool_chunk("search_", '{"query"', call_id="call_9"),
            tool_chunk("documents", ': "flight"}', call_id=""),
        ],
        [text_chunk("done")],
    ])
    events = run(client, db, [{"role": "user", "content": "q"}])
    tool_events = [e for e in events if e["type"] == "tool"]
    assert tool_events == [{"type": "tool", "name": "search_documents", "input": {"query": "flight"}}]


def test_ungrounded_answer_discarded_and_retried(db):
    client = FakeClient([
        [text_chunk("Maxwell is charged (INVENTED-1.pdf, p. 1).")],  # answered from memory
        [tool_chunk("search_documents", '{"query": "Maxwell"}')],
        [text_chunk("Grounded answer.")],
    ])
    messages = [{"role": "user", "content": "who is charged?"}]
    events = run(client, db, messages)

    streamed = "".join(e["text"] for e in events if e["type"] == "text")
    assert "INVENTED" not in streamed  # fabricated answer never reached the user
    assert streamed == "Grounded answer."
    assert len(client.calls) == 3
    assert client.calls[1]["tool_choice"] == "required"  # still forcing on the retry

    # The corrective nudge was injected into history.
    assert any(
        m["role"] == "system" and "Do not answer from memory" in m["content"] for m in messages
    )


def test_think_tags_stripped_from_stream_and_history(db):
    client = FakeClient([
        [tool_chunk("search_documents", '{"query": "flight"}')],
        [text_chunk("<think>let me reason"), text_chunk(" secretly</think>Answer.")],
    ])
    messages = [{"role": "user", "content": "q"}]
    events = run(client, db, messages)

    streamed = "".join(e["text"] for e in events if e["type"] == "text")
    assert streamed == "Answer."
    assert messages[-1]["content"] == "Answer."


def test_invalid_tool_arguments_reported_not_crashed(db):
    client = FakeClient([
        [tool_chunk("search_documents", "not json at all")],
        [text_chunk("recovered")],
    ])
    messages = [{"role": "user", "content": "q"}]
    events = run(client, db, messages)

    tool_result = next(m for m in messages if m["role"] == "tool")
    assert "Invalid tool arguments" in tool_result["content"]
    assert any(e["type"] == "text" and e["text"] == "recovered" for e in events)
