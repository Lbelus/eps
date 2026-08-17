#!/bin/sh
# Prepare the SQLite corpus, then exec the container CMD (uvicorn).
#
# The full epstein.db is git-ignored and large, but the repo tracks a
# compressed snapshot (epstein.db.gz). On a fresh host the data volume holds
# only the .gz, so decompress it once; if a real .db is already mounted (e.g. a
# dev host reusing its locally-built index) leave it untouched.
set -e

DATA_DIR="${RAG_DATA_DIR:-/app/data}"
DB="${RAG_DB_PATH:-$DATA_DIR/epstein.db}"
GZ="$DATA_DIR/epstein.db.gz"

if [ ! -f "$DB" ]; then
    if [ -f "$GZ" ]; then
        echo "[entrypoint] $DB not found — decompressing $GZ ..."
        gunzip -c "$GZ" > "$DB"
        echo "[entrypoint] database ready."
    else
        echo "[entrypoint] ERROR: no database at $DB and no snapshot at $GZ." >&2
        echo "[entrypoint] Mount a corpus into $DATA_DIR (epstein.db or epstein.db.gz)." >&2
        exit 1
    fi
fi

# Semantic search needs the page_embeddings table. A decompressed snapshot may
# not include it; opt in to build it here (needs Ollama reachable, ~1h for the
# full corpus). Keyword search works regardless; semantic falls back to FTS.
if [ "${RAG_BUILD_EMBEDDINGS:-0}" = "1" ]; then
    echo "[entrypoint] RAG_BUILD_EMBEDDINGS=1 — building semantic index ..."
    python src/main.py --mode embed || \
        echo "[entrypoint] WARN: embed step failed; semantic_search will fall back to FTS."
fi

exec "$@"
