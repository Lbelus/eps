"""Schema, ingest bookkeeping, and FTS5 search behavior."""

import sqlite3

import pytest

from core.database import detect_source, get_ingested_filenames, insert_document, search_documents


def test_detect_source_prefixes():
    assert detect_source("cl_gov.uscourts.pdf") == "cl"
    assert detect_source("dc_Some-Doc.pdf") == "dc"
    assert detect_source("EFTA000001.pdf") == "doj"


def test_ingested_filenames(db):
    names = get_ingested_filenames(db)
    assert "dc_Maxwell-Indictment.pdf" in names
    assert len(names) == 3


def test_duplicate_filename_rejected(db):
    with pytest.raises(sqlite3.IntegrityError):
        insert_document(db, "cl_flight.pdf", "cl", 1, "text", [{"number": 1, "text": "text"}])


def test_fts_search_finds_document(db):
    results = search_documents(db, "indictment")
    assert [r["filename"] for r in results] == ["dc_Maxwell-Indictment.pdf"]
    assert ">>>" in results[0]["snippet"]  # highlight markers


def test_fts_search_implicit_and(db):
    # Both terms appear only in the flight-logs document.
    results = search_documents(db, "flight Teterboro")
    assert [r["filename"] for r in results] == ["cl_flight.pdf"]


def test_fts_search_no_results(db):
    assert search_documents(db, "nonexistentterm") == []


def test_fts_stays_synced_on_delete(db):
    db.execute("DELETE FROM documents WHERE filename = 'cl_flight.pdf'")
    db.commit()
    assert search_documents(db, "Teterboro") == []
