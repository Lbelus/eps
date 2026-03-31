"""
CLI search over the Epstein/Maxwell document database.
"""

from core.database import init_db, search_documents


def search_cli(db_path: str, query: str, limit: int = 20):
    conn = init_db(db_path)
    results = search_documents(conn, query, limit)
    conn.close()

    if not results:
        print(f"No results for: {query}")
        return

    print(f"\n{len(results)} results for: {query}\n")
    for i, r in enumerate(results, 1):
        print(f"[{i}] {r['filename']} ({r['source']}, {r['page_count']} pages)")
        snippet = r["snippet"].replace("\n", " ").strip()
        print(f"    {snippet}")
        print()
