"""The retrieval tools the model calls, plus tool dispatch and the think filter."""

import core.rag as rag
from core.rag import (
    MAX_CHARS_PER_PAGE,
    TOOLS,
    _execute_tool,
    _find_pages,
    _openai_tools,
    _read_pages,
    _search_documents,
    _ThinkFilter,
)


class TestSearchDocuments:
    def test_match_includes_metadata_and_snippet(self, db):
        out = _search_documents(db, "indictment")
        assert "dc_Maxwell-Indictment.pdf" in out
        assert "source=dc" in out
        assert ">>>" in out

    def test_no_match_suggests_rephrasing(self, db):
        out = _search_documents(db, "zebra")
        assert "No documents matched" in out


class TestFindPages:
    def test_locates_page_numbers(self, db):
        out = _find_pages(db, "EFTA000001.pdf", "deposition")
        assert "p. 2" in out
        assert "p. 1" not in out

    def test_case_insensitive(self, db):
        out = _find_pages(db, "dc_Maxwell-Indictment.pdf", "GHISLAINE")
        assert "p. 1" in out

    def test_missing_document(self, db):
        assert "No document named" in _find_pages(db, "nope.pdf", "term")

    def test_term_absent(self, db):
        assert "not found in any page" in _find_pages(db, "cl_flight.pdf", "subpoena")


class TestReadPages:
    def test_reads_requested_pages(self, db):
        out = _read_pages(db, "dc_Maxwell-Indictment.pdf", [1, 2])
        assert "=== dc_Maxwell-Indictment.pdf p. 1 ===" in out
        assert "Count Two" in out

    def test_caps_pages_per_call(self, db):
        out = _read_pages(db, "EFTA000001.pdf", [1, 2, 3, 1, 2, 3, 1])
        # Only the first MAX_PAGES_PER_READ page numbers are honored.
        assert out.count("===") <= rag.MAX_PAGES_PER_READ * 2

    def test_truncates_oversized_page(self, db):
        out = _read_pages(db, "EFTA000001.pdf", [3])
        assert f"[page truncated at {MAX_CHARS_PER_PAGE} chars]" in out
        assert len(out) < 10000

    def test_unknown_pages(self, db):
        assert "No pages" in _read_pages(db, "cl_flight.pdf", [99])


class TestExecuteTool:
    def test_dispatch(self, db):
        out, is_error = _execute_tool(db, "search_documents", {"query": "flight"})
        assert not is_error
        assert "cl_flight.pdf" in out

    def test_unknown_tool(self, db):
        out, is_error = _execute_tool(db, "bogus", {})
        assert is_error

    def test_bad_fts_syntax_returned_as_error(self, db):
        out, is_error = _execute_tool(db, "search_documents", {"query": 'AND OR ('})
        assert is_error
        assert "Query error" in out

    def test_semantic_search_unavailable_without_index(self, db, monkeypatch):
        monkeypatch.setattr(rag, "_semantic_index", None)
        out, is_error = _execute_tool(db, "semantic_search", {"query": "travel"})
        assert not is_error
        assert "search_documents" in out  # tells the model to fall back


class TestToolSchemas:
    def test_openai_tools_mirror_anthropic(self):
        converted = _openai_tools()
        assert [t["function"]["name"] for t in converted] == [t["name"] for t in TOOLS]
        for original, conv in zip(TOOLS, converted):
            assert conv["function"]["parameters"] == original["input_schema"]

    def test_semantic_search_registered(self):
        assert any(t["name"] == "semantic_search" for t in TOOLS)


class TestThinkFilter:
    def test_passthrough_released_on_flush(self):
        # Leading plain text is withheld mid-stream (it might precede an orphan
        # </think>) and released once the stream ends with no think block seen.
        f = _ThinkFilter()
        assert f.feed("plain text") == ""
        assert f.flush() == "plain text"

    def test_strips_think_block(self):
        f = _ThinkFilter()
        assert f.feed("a<think>hidden</think>b") == "ab"

    def test_strips_across_chunk_boundaries(self):
        f = _ThinkFilter()
        out = "".join(f.feed(c) for c in ["a<thi", "nk>hid", "den</thi", "nk>b"])
        assert out == "ab"

    def test_strips_orphan_closing_tag(self):
        # Template auto-opened the block, so only the closing </think> arrives.
        f = _ThinkFilter()
        assert f.feed("hidden reasoning</think>visible") == "visible"

    def test_streams_incrementally_after_close(self):
        # Once the block closes, subsequent text streams delta-by-delta again.
        f = _ThinkFilter()
        f.feed("reasoning</think>")
        assert f.feed("more ") == "more "
        assert f.feed("text") == "text"

    def test_discards_unterminated_reasoning_on_flush(self):
        f = _ThinkFilter()
        assert f.feed("<think>reasoning that never closes") == ""
        assert f.flush() == ""
