"""Semantic index build and cosine search, with the embedding server faked."""

import numpy as np
import pytest

import core.embeddings as embeddings
from core.database import init_db
from core.embeddings import MIN_PAGE_CHARS, SemanticIndex, build_index


def fake_embed(texts, prefix):
    """Deterministic unit vectors: dimension i set by text hash, no network."""
    vectors = np.zeros((len(texts), 8), dtype=np.float32)
    for i, text in enumerate(texts):
        vectors[i, hash(text) % 8] = 1.0
    return vectors


@pytest.fixture()
def built(db_path, monkeypatch):
    monkeypatch.setattr(embeddings, "embed_texts", fake_embed)
    build_index(db_path)
    conn = init_db(db_path)
    yield conn
    conn.close()


def test_build_embeds_substantial_pages(built):
    (n_embedded,) = built.execute("SELECT count(*) FROM page_embeddings").fetchone()
    (n_eligible,) = built.execute(
        "SELECT count(*) FROM pages WHERE length(page_text) >= ?", (MIN_PAGE_CHARS,)
    ).fetchone()
    assert n_embedded == n_eligible > 0


def test_build_is_resumable_and_idempotent(built, db_path, monkeypatch, capsys):
    before = built.execute("SELECT count(*) FROM page_embeddings").fetchone()
    monkeypatch.setattr(embeddings, "embed_texts", fake_embed)
    build_index(db_path)  # second run
    assert "up to date" in capsys.readouterr().out
    assert built.execute("SELECT count(*) FROM page_embeddings").fetchone() == before


def test_search_ranks_matching_vector_first(built, monkeypatch):
    index = SemanticIndex(built)
    target_id, blob = built.execute("SELECT page_id, embedding FROM page_embeddings").fetchone()
    target_vec = np.frombuffer(blob, dtype=np.float32)
    monkeypatch.setattr(embeddings, "embed_texts", lambda texts, prefix: target_vec[None, :])
    results = index.search("anything", k=3)
    assert results[0][0] == target_id
    assert results[0][1] == pytest.approx(1.0)
    assert results[0][1] >= results[1][1]  # descending scores


def test_empty_index_returns_no_results(db):
    index = SemanticIndex(db)  # page_embeddings table doesn't exist here
    assert len(index) == 0
    assert index.search("anything") == []


def test_embed_texts_normalizes(monkeypatch):
    class FakeResponse:
        def raise_for_status(self):
            pass

        def json(self):
            return {"embeddings": [[3.0, 4.0], [0.0, 0.0]]}

    monkeypatch.setattr(embeddings.requests, "post", lambda *a, **k: FakeResponse())
    vectors = embeddings.embed_texts(["a", "b"], "search_query: ")
    assert np.linalg.norm(vectors[0]) == pytest.approx(1.0)
    assert not np.isnan(vectors).any()  # zero vector must not divide to NaN
