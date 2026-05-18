#!/usr/bin/env bash
# Wrapper for the EPS scan_manager crawler + ingest pipeline.
# Activates the local .venv, then dispatches to a subcommand.
#
# Usage:
#   ./crawl.sh prime             # one-time interactive session prime (headed)
#   ./crawl.sh all               # crawl DOJ + CourtListener + DocumentCloud
#   ./crawl.sh doj               # DOJ disclosure tree only
#   ./crawl.sh cl                # CourtListener RECAP archive only
#   ./crawl.sh dc                # DocumentCloud only
#   ./crawl.sh reset             # clear progress and session state
#   ./crawl.sh ingest [workers]  # OCR data/input/ into data/epstein.db
#   ./crawl.sh search "phrase"   # full-text search
#   ./crawl.sh help              # this message
#
# Extra flags to the crawler can be passed after the subcommand, e.g.
#   ./crawl.sh doj --headless false

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
cd "$SCRIPT_DIR"

VENV_PYTHON="$SCRIPT_DIR/.venv/bin/python"
if [[ ! -x "$VENV_PYTHON" ]]; then
    echo "error: $VENV_PYTHON not found." >&2
    echo "create it first:" >&2
    echo "    python3 -m venv .venv && .venv/bin/pip install -r requirements.txt" >&2
    echo "    .venv/bin/playwright install chromium" >&2
    exit 1
fi

usage() {
    sed -n '2,18p' "$0" | sed 's/^# \{0,1\}//'
}

cmd="${1:-help}"
shift || true

case "$cmd" in
    prime)
        exec "$VENV_PYTHON" -m src.web.crawler --prime "$@"
        ;;
    all)
        exec "$VENV_PYTHON" -m src.web.crawler "$@"
        ;;
    doj)
        exec "$VENV_PYTHON" -m src.web.crawler --source doj "$@"
        ;;
    cl|courtlistener)
        exec "$VENV_PYTHON" -m src.web.crawler --source courtlistener "$@"
        ;;
    dc|documentcloud)
        exec "$VENV_PYTHON" -m src.web.crawler --source documentcloud "$@"
        ;;
    reset)
        exec "$VENV_PYTHON" -m src.web.crawler --reset "$@"
        ;;
    ingest)
        workers="${1:-}"
        if [[ -n "$workers" ]]; then
            shift
            exec "$VENV_PYTHON" src/main.py --mode ingest --workers "$workers" "$@"
        else
            exec "$VENV_PYTHON" src/main.py --mode ingest "$@"
        fi
        ;;
    search)
        if [[ $# -eq 0 ]]; then
            echo "error: search requires a query string" >&2
            echo "       ./crawl.sh search \"grand jury\"" >&2
            exit 2
        fi
        query="$1"
        shift
        exec "$VENV_PYTHON" src/main.py --mode search --query "$query" "$@"
        ;;
    help|-h|--help)
        usage
        ;;
    *)
        echo "error: unknown subcommand: $cmd" >&2
        usage >&2
        exit 2
        ;;
esac
