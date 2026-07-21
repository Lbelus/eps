"""
Shared fixtures for the scan_manager test suite.

Tests run fully offline: no Ollama, no Anthropic API. The RAG database
tools are exercised against a small fixture corpus; model loops use fake
clients; the FastAPI server is tested with the turn generator stubbed.

Run from scan_manager/:  .venv/bin/pytest
"""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))

from core.database import init_db, insert_document  # noqa: E402

FIXTURE_DOCS = [
    {
        "filename": "dc_Maxwell-Indictment.pdf",
        "source": "dc",
        "pages": [
            "UNITED STATES DISTRICT COURT SOUTHERN DISTRICT OF NEW YORK. "
            "Indictment 20 Cr. 330. GHISLAINE MAXWELL, defendant. Conspiracy "
            "to entice minors to travel to engage in illegal sex acts.",
            "Count Two: enticement of a minor. Between 1994 and 1997 the "
            "defendant assisted Jeffrey Epstein.",
        ],
    },
    {
        "filename": "cl_flight.pdf",
        "source": "cl",
        "pages": [
            "Flight logs for the private aircraft. Departure Palm Beach, "
            "arrival Teterboro. Passenger manifest attached.",
        ],
    },
    {
        "filename": "EFTA000001.pdf",
        "source": "doj",
        "pages": [
            "Grand jury subpoena issued to the records custodian.",
            "Deposition transcript of the pilot regarding the flight logs.",
            "x" * 9000,  # oversized page to exercise read_pages truncation
        ],
    },
]


@pytest.fixture()
def db_path(tmp_path):
    """Path to a small ingested corpus in a temp SQLite database."""
    path = str(tmp_path / "test.db")
    conn = init_db(path)
    for doc in FIXTURE_DOCS:
        insert_document(
            conn,
            filename=doc["filename"],
            source=doc["source"],
            page_count=len(doc["pages"]),
            full_text="\n".join(doc["pages"]),
            pages=[{"number": i + 1, "text": t} for i, t in enumerate(doc["pages"])],
        )
    conn.close()
    return path


@pytest.fixture()
def db(db_path):
    """Open connection to the fixture corpus."""
    conn = init_db(db_path)
    yield conn
    conn.close()
