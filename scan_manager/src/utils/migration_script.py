import sqlite3
import mysql.connector

SQLITE_DB = "./data/epstein.db"

MYSQL_CONFIG = {
    "host": "172.18.0.2",
    "port": 3306,
    "user": "dev_admin",
    "password": "dev_admin",
    "database": "test_rest_DB",
}

def main():
    sqlite_conn = sqlite3.connect(SQLITE_DB)
    sqlite_conn.row_factory = sqlite3.Row
    sqlite_cur = sqlite_conn.cursor()

    mysql_conn = mysql.connector.connect(**MYSQL_CONFIG)
    mysql_cur = mysql_conn.cursor()

    mysql_cur.execute("SET FOREIGN_KEY_CHECKS=0")

    mysql_cur.execute("DELETE FROM pages")
    mysql_cur.execute("DELETE FROM documents")

    sqlite_cur.execute("""
        SELECT document_id, filename, source, page_count, full_text, created_at
        FROM documents
        ORDER BY document_id
    """)
    documents = sqlite_cur.fetchall()

    mysql_cur.executemany("""
        INSERT INTO documents
        (document_id, filename, source, page_count, full_text, created_at)
        VALUES (%s, %s, %s, %s, %s, %s)
    """, [
        (
            row["document_id"],
            row["filename"],
            row["source"],
            row["page_count"],
            row["full_text"],
            row["created_at"],
        )
        for row in documents
    ])

    sqlite_cur.execute("""
        SELECT page_id, document_id, page_number, page_text
        FROM pages
        ORDER BY page_id
    """)
    pages = sqlite_cur.fetchall()

    mysql_cur.executemany("""
        INSERT INTO pages
        (page_id, document_id, page_number, page_text)
        VALUES (%s, %s, %s, %s)
    """, [
        (
            row["page_id"],
            row["document_id"],
            row["page_number"],
            row["page_text"],
        )
        for row in pages
    ])

    mysql_cur.execute("SET FOREIGN_KEY_CHECKS=1")
    mysql_conn.commit()

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

    print("Migration complete.")

if __name__ == "__main__":
    main()
