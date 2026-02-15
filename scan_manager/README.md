# Scan_manager

```mermaid
graph TD
    A[/"Clients"/] --> B("(Auth)")
    B --> C["Monolith (Python App)"]
    C <--> D["REST API (C++)"]
    D <--> E[("MySQL Database")]
    D <--> F[("Redis (Optional)")]

    subgraph Optional
        G{"Load Balancer: Clients - reverse proxy / proxy  <-> Monolith"}
    end

    B --> G
    C --> A
    G <--> C
    G --> A

    subgraph "Monolith Services"
        C1[["Doc Serializer (XLS, PDF, HTML -> JSON) <br>(PDF) Input: 689 KB Output: 8 KB Time: 2.5641 s <br>(HTM) Input: 44 KB Output: 20 KB Time: 0.0300 s"]]
        C2[["Document Parser <br>Input: 20 KB <br>Output: 25 KB <br>Time: 0.0005 s"]]
    end

    C --> C1
    C --> C2

    %% Metrics as comments and annotations
    C1 --> M1("Serialize a pdf in 2.50s ")
    M1 --> M2("Stores processed JSON only (no raw files)")

    C2 --> M3("No metric but load is of no consequences")
    M3 --> M4("Return serialized e-mails to the client in a json format")   


    %% Assign classes
    class A client;
    class B auth;
    class C monolith;
    class D api;
    class E,F db;
    class G proxy;
    class C1,C2,C3,C4 service;
    class M1,M2,M3,M4,M5,M6,M7,M8 metric;

    %% Define styles
    classDef client fill:#e3f2fd,stroke:#2196f3,stroke-width:2px,color:#000;
    classDef auth fill:#fff3e0,stroke:#fb8c00,stroke-width:2px,color:#000;
    classDef monolith fill:#ede7f6,stroke:#673ab7,stroke-width:2px,color:#000;
    classDef api fill:#e0f7fa,stroke:#00acc1,stroke-width:2px,color:#000;
    classDef db fill:#f1f8e9,stroke:#558b2f,stroke-width:2px,color:#000;
    classDef proxy fill:#fce4ec,stroke:#ec407a,stroke-width:2px,color:#000;
    classDef service fill:#f3e5f5,stroke:#9c27b0,stroke-width:1.5px,color:#000;
    classDef metric fill:#f5f5f5,stroke:#9e9e9e,stroke-dasharray: 5 5,color:#000;
```

### Data Highlights

| Component            | Input  | Output | Time     |
| -------------------- | ------ | ------ | -------- |
| Doc Serializer (PDF) | 689 KB | 8 KB   | 2.5641 s |
| Doc Serializer (HTM) | 44 KB  | 20 KB  | 0.0300 s |
| Document Parser      | 20 KB  | 25 KB  | 0.0005 s |


### Color Roles

| Role          | Background     | Border      |
| ------------- | -------------- | ----------- |
| Clients       | Light Blue     | Blue        |
| Auth          | Light Orange   | Deep Orange |
| Monolith      | Light Purple   | Indigo      |
| REST API      | Aqua           | Cyan        |
| Databases     | Light Green    | Green       |
| Proxy/LB      | Pink           | Rose        |
| Internal Svcs | Light Lavender | Purple      |
| Metrics       | Light Gray     | Dashed Gray |


### Custom Shapes Legend

| Shape           | Component Type             |
| --------------- | -------------------------- |
| `rect`          | General Services / App     |
| `cylinder`      | Databases (MySQL, Redis)   |
| `parallelogram` | Client Input/Output        |
| `hexagon`       | Load Balancer / Proxy      |
| `roundrect`     | Auth / Identity            |
| `subroutine`    | Internal Monolith Services |

* **`/"..."`** → Parallelogram (Client)
* **`[...]`** → Rectangle (Monolith/API)
* **`[(...)]`** → Cylinder (Database)
* **`[[...]]`** → Subroutine (Internal service)
* **`{{...}}`** → Hexagon (Proxy/Load Balancer)
* **`(...)`** → Roundrect (Auth, metrics)


***

# Scan_manager
Small toolkit to turn messy business docs into structured data and back again.
It can OCR PDFs/HTML → JSON, extract fields via KMP/regex + YAML and talk to a REST backend.

## Description

**What it does**

* Convert **PDF/HTML → JSON** with OCR (Tesseract) — one page, one text block.
* Find fields by **tokens** (KMP or regex) driven by a **YAML config**.
* Minimal **REST client** for simple CRUD-ish backends.

## Installation

### Requirements

* **Python 3.10+**
* **System deps**

  * Tesseract OCR (Prerequisites at: https://pypi.org/project/pytesseract/)


### System packages (examples)

* Ubuntu/Debian:

  ```bash
  sudo apt-get update
  sudo apt-get install tesseract-ocr poppler-utils
  ```

### Python packages

```bash
python -m venv .venv && source .venv/bin/activate  # on Windows: .venv\Scripts\activate
pip install -r requirements.txt
```

## Usage

### 1) PDF/HTML → JSON (OCR)

```python
from pathlib import Path
from core.json_converter import json_converter

converter = json_converter(Path("./input"), Path("./output"))
converter.process_docs()  # writes one JSON per input file
```

### 2) Token extraction (YAML-driven KMP/regex)

`config/doc_template.yaml` (example):

```yaml
documents:
  invoice:
    match: ".*invoice.*\\.json$"
    tokens:
      - label: total_eur
        method: regex
        pattern: "Total\\s*:?\\s*([\\d.,]+)\\s*(?:EUR|€)"
        group: 1
      - label: order_number
        method: kmp
        pattern: "Numéro de la commande"
        extract_after: 30
```

Run:

```python
from core.token_finder import load_token_config, token_finder

token_config = load_token_config("./config/doc_template.yaml")
tf = token_finder("./output", "./updated", token_config)
tf.process_docs()  # writes enriched JSON with page["pos"] and page["values"]
```


### 3) Minimal REST client

```python
from core.rest_client import RestApiClient

api = RestApiClient("127.0.0.1:3004")
print(api.read_all("users"))
print(api.create_entity("users", {"name": "Alice"}))
```

## Support

* Open an issue in the repository (preferred).
* Or reach out via email: **[lorris.belus@cs-soprasteria.com](mailto:lorris.belus@cs-soprasteria.com)**.


```

## Authors and acknowledgment

* **Author and Maintainer:** *Lorris BELUS*

* Thanks to open-source projects: Tesseract, Poppler, pdf2image, BeautifulSoup.

