"""
Semantic (vector) index over document pages.

Embeddings come from a local model served by Ollama (nomic-embed-text by
default) and are stored as normalized float32 blobs in a page_embeddings
table inside epstein.db. Search is brute-force cosine over a numpy matrix —
at ~112k pages that's tens of milliseconds, no vector-DB dependency needed.

Build (one-time, resumable):   python src/main.py --mode embed
Query:                         SemanticIndex(conn).search("query text")

nomic-embed-text requires task prefixes: "search_document: " when embedding
pages, "search_query: " when embedding queries — retrieval quality drops
noticeably without them.
"""

import os
import sqlite3
import time

import numpy as np
import requests

EMBED_MODEL = os.environ.get("RAG_EMBED_MODEL", "nomic-embed-text")
EMBED_URL = os.environ.get("RAG_EMBED_URL", "http://localhost:11434").rstrip("/")

BATCH_SIZE = 32
MIN_PAGE_CHARS = 50      # skip blank/garbage OCR pages
MAX_EMBED_CHARS = 8000   # nomic-embed context is 8192 tokens; pages rarely exceed this

SCHEMA = """
CREATE TABLE IF NOT EXISTS page_embeddings (
    page_id   INTEGER PRIMARY KEY REFERENCES pages(page_id),
    embedding BLOB NOT NULL
);
"""


def embed_texts(texts: list[str], prefix: str) -> np.ndarray:
    """Embed a batch of texts, L2-normalized so cosine == dot product."""
    response = requests.post(
        f"{EMBED_URL}/api/embed",
        json={"model": EMBED_MODEL, "input": [prefix + t[:MAX_EMBED_CHARS] for t in texts]},
        timeout=600,
    )
    response.raise_for_status()
    vectors = np.asarray(response.json()["embeddings"], dtype=np.float32)
    norms = np.linalg.norm(vectors, axis=1, keepdims=True)
    norms[norms == 0] = 1.0
    return vectors / norms


def build_index(db_path: str):
    """Embed every substantial page not yet in page_embeddings. Resumable."""
    conn = sqlite3.connect(db_path)
    conn.execute("PRAGMA journal_mode=WAL")
    conn.executescript(SCHEMA)

    (total,) = conn.execute(
        """
        SELECT count(*) FROM pages p
        LEFT JOIN page_embeddings e ON e.page_id = p.page_id
        WHERE e.page_id IS NULL AND length(p.page_text) >= ?
        """,
        (MIN_PAGE_CHARS,),
    ).fetchone()
    if total == 0:
        print("Semantic index is up to date.")
        return
    print(f"Embedding {total:,} pages with {EMBED_MODEL} via {EMBED_URL} ...")

    done = 0
    started = time.monotonic()
    while True:
        rows = conn.execute(
            """
            SELECT p.page_id, p.page_text FROM pages p
            LEFT JOIN page_embeddings e ON e.page_id = p.page_id
            WHERE e.page_id IS NULL AND length(p.page_text) >= ?
            ORDER BY p.page_id LIMIT ?
            """,
            (MIN_PAGE_CHARS, BATCH_SIZE),
        ).fetchall()
        if not rows:
            break
        vectors = embed_texts([text for _, text in rows], "search_document: ")
        conn.executemany(
            "INSERT INTO page_embeddings (page_id, embedding) VALUES (?, ?)",
            [(page_id, vec.tobytes()) for (page_id, _), vec in zip(rows, vectors)],
        )
        conn.commit()
        done += len(rows)
        if done % (BATCH_SIZE * 25) == 0 or done >= total:
            rate = done / (time.monotonic() - started)
            remaining = (total - done) / rate if rate else 0
            print(f"  {done:,}/{total:,} pages  ({rate:.0f}/s, ~{remaining / 60:.0f} min left)", flush=True)
    print("Semantic index build complete.")
    conn.close()


class SemanticIndex:
    """In-memory cosine search over the page_embeddings table."""

    def __init__(self, conn: sqlite3.Connection):
        try:
            rows = conn.execute("SELECT page_id, embedding FROM page_embeddings").fetchall()
        except sqlite3.OperationalError:
            rows = []  # index never built — behave as empty so callers fall back to FTS
        self.page_ids = np.array([r[0] for r in rows], dtype=np.int64)
        if rows:
            self.matrix = np.frombuffer(b"".join(r[1] for r in rows), dtype=np.float32).reshape(len(rows), -1)
        else:
            self.matrix = np.zeros((0, 1), dtype=np.float32)

    def __len__(self):
        return len(self.page_ids)

    def search(self, query: str, k: int = 10) -> list[tuple[int, float]]:
        """Return [(page_id, score)] for the k most similar pages."""
        if not len(self):
            return []
        query_vec = embed_texts([query], "search_query: ")[0]
        scores = self.matrix @ query_vec
        k = min(k, len(scores))
        top = np.argpartition(scores, -k)[-k:]
        top = top[np.argsort(scores[top])[::-1]]
        return [(int(self.page_ids[i]), float(scores[i])) for i in top]
