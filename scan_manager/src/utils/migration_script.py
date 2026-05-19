"""Copy the SQLite FTS5 corpus (scan_manager/data/epstein.db) into the MySQL
court-documents schema used by rest_api/.

Configuration via environment variables (with safe local-dev defaults):

    SQLITE_DB         path to SQLite source             (default: data/epstein.db)
    MYSQL_HOST        MySQL host                        (default: 127.0.0.1)
    MYSQL_PORT        MySQL port                        (default: 3306)
    MYSQL_USER        MySQL user                        (default: dev_admin)
    MYSQL_PASSWORD    MySQL password                    (default: dev_admin)
    MYSQL_DATABASE    target database                   (default: test_rest_DB)
    BATCH_SIZE        rows per executemany batch        (default: 200)

Apply the schema first:
    mysql -h 127.0.0.1 -P 3306 -u dev_admin -p test_rest_DB \\
        < rest_api/db/schema.sql

Then run:
    cd scan_manager
    .venv/bin/python -m src.utils.migration_script
"""

import os
import sqlite3
import sys
import time
from pathlib import Path

import mysql.connector

SCAN_MANAGER_DIR = Path(__file__).resolve().parents[2]

SQLITE_DB = os.environ.get("SQLITE_DB", str(SCAN_MANAGER_DIR / "data" / "epstein.db"))

MYSQL_CONFIG = {
    "host":     os.environ.get("MYSQL_HOST",     "127.0.0.1"),
    "port":     int(os.environ.get("MYSQL_PORT", "3306")),
    "user":     os.environ.get("MYSQL_USER",     "dev_admin"),
    "password": os.environ.get("MYSQL_PASSWORD", "dev_admin"),
    "database": os.environ.get("MYSQL_DATABASE", "test_rest_DB"),
}

BATCH_SIZE = int(os.environ.get("BATCH_SIZE", "200"))


def chunked(seq, size):
    for i in range(0, len(seq), size):
        yield seq[i:i + size]


def main() -> int:
    print(f"[source] {SQLITE_DB}")
    print(f"[target] mysql://{MYSQL_CONFIG['user']}@{MYSQL_CONFIG['host']}:{MYSQL_CONFIG['port']}/{MYSQL_CONFIG['database']}")

    if not Path(SQLITE_DB).exists():
        print(f"error: SQLite source not found: {SQLITE_DB}", file=sys.stderr)
        return 1

    sqlite_conn = sqlite3.connect(SQLITE_DB)
    sqlite_conn.row_factory = sqlite3.Row
    sqlite_cur = sqlite_conn.cursor()

    mysql_conn = mysql.connector.connect(**MYSQL_CONFIG)
    mysql_cur = mysql_conn.cursor()

    # Wipe target so the migration is idempotent.
    mysql_cur.execute("SET FOREIGN_KEY_CHECKS=0")
    mysql_cur.execute("DELETE FROM pages")
    mysql_cur.execute("DELETE FROM documents")
    mysql_conn.commit()

    # ---- documents ----
    sqlite_cur.execute("SELECT COUNT(*) FROM documents")
    doc_total = sqlite_cur.fetchone()[0]
    print(f"[documents] copying {doc_total:,} rows...")

    sqlite_cur.execute("""
        SELECT document_id, filename, source, page_count, full_text, created_at
        FROM documents
        ORDER BY document_id
    """)
    insert_doc = """
        INSERT INTO documents
            (document_id, filename, source, page_count, full_text, created_at)
        VALUES (%s, %s, %s, %s, %s, %s)
    """
    started = time.time()
    inserted = 0
    while True:
        rows = sqlite_cur.fetchmany(BATCH_SIZE)
        if not rows:
            break
        mysql_cur.executemany(insert_doc, [
            (r["document_id"], r["filename"], r["source"],
             r["page_count"], r["full_text"], r["created_at"])
            for r in rows
        ])
        mysql_conn.commit()
        inserted += len(rows)
        print(f"  documents: {inserted:,}/{doc_total:,}")
    print(f"[documents] done in {time.time() - started:.1f}s")

    # ---- pages ----
    sqlite_cur.execute("SELECT COUNT(*) FROM pages")
    page_total = sqlite_cur.fetchone()[0]
    print(f"[pages] copying {page_total:,} rows...")

    sqlite_cur.execute("""
        SELECT page_id, document_id, page_number, page_text
        FROM pages
        ORDER BY page_id
    """)
    insert_page = """
        INSERT INTO pages
            (page_id, document_id, page_number, page_text)
        VALUES (%s, %s, %s, %s)
    """
    started = time.time()
    inserted = 0
    while True:
        rows = sqlite_cur.fetchmany(BATCH_SIZE)
        if not rows:
            break
        mysql_cur.executemany(insert_page, [
            (r["page_id"], r["document_id"], r["page_number"], r["page_text"])
            for r in rows
        ])
        mysql_conn.commit()
        inserted += len(rows)
        if inserted % (BATCH_SIZE * 10) == 0 or inserted == page_total:
            print(f"  pages: {inserted:,}/{page_total:,}")
    print(f"[pages] done in {time.time() - started:.1f}s")

    # Restore FK checks and bump the AUTO_INCREMENT counters so new inserts
    # do not collide with the IDs we just imported.
    mysql_cur.execute("SET FOREIGN_KEY_CHECKS=1")
    mysql_cur.execute("SELECT COALESCE(MAX(document_id), 0) + 1 FROM documents")
    next_doc_id = mysql_cur.fetchone()[0]
    mysql_cur.execute(f"ALTER TABLE documents AUTO_INCREMENT = {next_doc_id}")
    mysql_cur.execute("SELECT COALESCE(MAX(page_id), 0) + 1 FROM pages")
    next_page_id = mysql_cur.fetchone()[0]
    mysql_cur.execute(f"ALTER TABLE pages AUTO_INCREMENT = {next_page_id}")
    mysql_conn.commit()

    mysql_cur.close()
    mysql_conn.close()
    sqlite_cur.close()
    sqlite_conn.close()

    print(f"[done] documents={doc_total:,}  pages={page_total:,}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
