"""
SQLite database with FTS5 full-text search for Epstein/Maxwell documents.
"""

import sqlite3
from pathlib import Path

SCHEMA = """
CREATE TABLE IF NOT EXISTS documents (
    document_id INTEGER PRIMARY KEY AUTOINCREMENT,
    filename    TEXT UNIQUE NOT NULL,
    source      TEXT NOT NULL,
    page_count  INTEGER NOT NULL,
    full_text   TEXT NOT NULL,
    created_at  TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS pages (
    page_id     INTEGER PRIMARY KEY AUTOINCREMENT,
    document_id INTEGER NOT NULL REFERENCES documents(document_id),
    page_number INTEGER NOT NULL,
    page_text   TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_pages_document ON pages(document_id);
CREATE INDEX IF NOT EXISTS idx_documents_source ON documents(source);

CREATE VIRTUAL TABLE IF NOT EXISTS documents_fts USING fts5(
    full_text,
    content='documents',
    content_rowid='document_id'
);

CREATE TRIGGER IF NOT EXISTS documents_ai AFTER INSERT ON documents BEGIN
    INSERT INTO documents_fts(rowid, full_text) VALUES (new.document_id, new.full_text);
END;

CREATE TRIGGER IF NOT EXISTS documents_ad AFTER DELETE ON documents BEGIN
    INSERT INTO documents_fts(documents_fts, rowid, full_text) VALUES('delete', old.document_id, old.full_text);
END;

CREATE TRIGGER IF NOT EXISTS documents_au AFTER UPDATE ON documents BEGIN
    INSERT INTO documents_fts(documents_fts, rowid, full_text) VALUES('delete', old.document_id, old.full_text);
    INSERT INTO documents_fts(rowid, full_text) VALUES (new.document_id, new.full_text);
END;
"""


def detect_source(filename: str) -> str:
    if filename.startswith("cl_"):
        return "cl"
    if filename.startswith("dc_"):
        return "dc"
    return "doj"


def init_db(db_path: str) -> sqlite3.Connection:
    conn = sqlite3.connect(db_path)
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA synchronous=NORMAL")
    conn.executescript(SCHEMA)
    return conn


def get_ingested_filenames(conn: sqlite3.Connection) -> set:
    cursor = conn.execute("SELECT filename FROM documents")
    return {row[0] for row in cursor}


def insert_document(conn: sqlite3.Connection, filename: str, source: str,
                    page_count: int, full_text: str, pages: list[dict]):
    cursor = conn.execute(
        "INSERT INTO documents (filename, source, page_count, full_text) VALUES (?, ?, ?, ?)",
        (filename, source, page_count, full_text),
    )
    doc_id = cursor.lastrowid
    conn.executemany(
        "INSERT INTO pages (document_id, page_number, page_text) VALUES (?, ?, ?)",
        [(doc_id, p["number"], p["text"]) for p in pages],
    )
    conn.commit()


def search_documents(conn: sqlite3.Connection, query: str, limit: int = 20) -> list[dict]:
    conn.row_factory = sqlite3.Row
    cursor = conn.execute(
        """
        SELECT d.document_id, d.filename, d.source, d.page_count,
               snippet(documents_fts, 0, '>>>', '<<<', '...', 32) as snippet
        FROM documents_fts f
        JOIN documents d ON d.document_id = f.rowid
        WHERE documents_fts MATCH ?
        ORDER BY rank
        LIMIT ?
        """,
        (query, limit),
    )
    return [dict(row) for row in cursor]
