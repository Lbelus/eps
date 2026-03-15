# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Project Is

**EPS** is a two-component document processing system:

1. **`scan_manager/`** — Python pipeline: raw PDFs/HTML → OCR → JSON → CSV
2. **`rest_api/`** — C++17 REST API server backed by MySQL, containerized with Docker

---

## Scan Manager (Python)

### Setup
```bash
cd scan_manager
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

Requires **Tesseract OCR** installed as a system dependency.

### Run
```bash
python src/main.py --mode scan          # Full pipeline: OCR → tokenize → CSV
python src/main.py --mode scan_exclude  # OCR only files NOT already in CSV
python src/main.py --mode scan_include  # OCR only files already in CSV
python src/main.py --mode round_trip    # Re-import CSV corrections into JSON
```

### Data flow
`data/input/` (PDF/HTML) → `data/output/` (raw JSON) → `data/json/` (enriched JSON) → `data/csv/merged.csv`

### Architecture
- `src/core/doc_serializer.py` — OCR engine: PDF via pdf2image+Tesseract, HTML via BeautifulSoup
- `src/core/token_finder.py` — Field extraction using KMP or regex, driven by `config/doc_template.yaml`
- `src/core/round_trip.py` — Bidirectional CSV↔JSON conversion
- `src/plugin/scan_manager.py` — High-level orchestration
- `config/doc_template.yaml` — Token pattern definitions (edit here to add new field extractions, no code changes needed)

---

## REST API (C++)

### Prerequisites
Docker and Docker Compose. All build/run commands use the helper script.

### Setup & Build
```bash
cd rest_api
source bash_scripts/helper_script.sh

rest_api_build_dev   # Build Docker image
rest_api_run_dev     # Run container
```

### Build modes (run inside container)
```bash
re          # Light build: Wall, Wextra, Werror, address sanitizer
re_full     # Full debug build: g3, verbose warnings, address sanitizer
go_tests    # Build with Google Test
```

### Testing
```bash
# Unit tests (GTest) — run inside container after go_tests
./run_tests

# Integration tests (bash/curl) — run from host
bash tests/external_ExampleUser_route_tests.sh

# Manual test client
python mock_rest_client.py
```

### API (default port 3004)
```
GET    /exampleusers?limit=10&offset=0
GET    /exampleusers/:id
POST   /exampleusers   {"name":"...","email":"..."}
PUT    /exampleusers/:id
DELETE /exampleusers/:id
```

### Architecture
- **`src/core/rest_api.hpp`** — Crow HTTP framework routing and middleware
- **`src/core/mysql_conn_pool.hpp`** — Thread-safe MySQL connection pool (acquire/release with idle timeout and auto-reconnect)
- **`src/db_repository/example_repository.hpp`** — Repository pattern template; use this as the model for new tables
  - `IExampleUsersRepository` — virtual interface
  - `MySQLExampleUsersRepository` — production implementation using MySQL++ SSQLS
  - `FakeExampleUsersRepository` — in-memory fake for unit tests (enabled via `REPO_FAKE_ONLY` macro)
- **`src/main.cpp`** — Entry point; initializes MySQL credentials and connection pool
- **`bash_scripts/helper_script.sh`** — 20+ helper functions for Docker, build, and test operations
- **`bash_scripts/generate_mysqlpp_table.sh`** — Generates SSQLS struct definitions from SQL DDL

### Adding a new table/entity
1. Write SQL DDL, run `generate_mysqlpp_table.sh` to get the SSQLS struct
2. Copy `example_repository.hpp` pattern, implement the interface for the new struct
3. Add routes in `rest_api.hpp`
4. Add GTest tests mirroring `tests/test_example_repo.cc`
5. Add bash integration tests mirroring `tests/external_ExampleUser_route_tests.sh`
