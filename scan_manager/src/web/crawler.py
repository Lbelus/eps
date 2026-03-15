"""
DOJ Epstein document crawler.

Search flow:
  1. Iterate queries from queries.py
  2. Hit search.justice.gov (no bot protection) to collect result page URLs
  3. Use Playwright to visit each justice.gov page and extract PDF links
     (Playwright executes the Akamai JS challenge automatically)
  4. Download PDFs to data/input/ using the session cookies from Playwright

Usage:
    python crawler.py                     # run all generated queries
    python crawler.py --limit 50          # stop after 50 queries
    python crawler.py --headless false    # show browser window
    python crawler.py --reset             # clear progress and restart
"""

import argparse
import hashlib
import html as htmllib
import json
import re
import sys
import time
from pathlib import Path
from urllib.parse import urlparse

import requests
from playwright.sync_api import sync_playwright

# Force line-buffered output so logs appear immediately when redirected to a file
sys.stdout.reconfigure(line_buffering=True)
sys.stderr.reconfigure(line_buffering=True)

try:
    from src.web.queries import generate_queries
except ModuleNotFoundError:
    from queries import generate_queries  # when run directly from src/web/

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

SEARCH_BASE   = "https://search.justice.gov/search"
AFFILIATE     = "justice"
START_URL     = "https://www.justice.gov/epstein"

QUEUE_IT_COOKIE_NAME  = "QueueITAccepted-SDFrts345E-V3_usdojsearch"
QUEUE_IT_COOKIE_VALUE = (
    "EventId%3Dusdojsearch%26RedirectType%3Dsafetynet"
    "%26IssueTime%3D1773594786"
    "%26Hash%3Dcda37fddf24c0964e9f6a5b41af6ef8389e78e6f0c505158790393444a0c8221"
)

DATA_DIR    = Path(__file__).resolve().parents[2] / "data"
OUTPUT_DIR  = DATA_DIR / "input"
SEEN_FILE   = DATA_DIR / "crawl_seen.txt"
PDF_LOG     = DATA_DIR / "crawl_pdfs.txt"

HEADERS = {
    "User-Agent": (
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/122.0.0.0 Safari/537.36"
    ),
    "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
}

# Seconds to wait between requests (be polite to DOJ servers)
SEARCH_DELAY  = 0.4
PAGE_DELAY    = 0.8
DOWNLOAD_DELAY = 1.2


# ---------------------------------------------------------------------------
# Progress tracking
# ---------------------------------------------------------------------------

def load_set(path: Path) -> set:
    if path.exists():
        return set(path.read_text(encoding="utf-8").splitlines())
    return set()


def append_to_file(path: Path, value: str):
    with open(path, "a", encoding="utf-8") as f:
        f.write(value + "\n")


# ---------------------------------------------------------------------------
# Search
# ---------------------------------------------------------------------------

def _parse_results_data(html: str) -> dict:
    m = re.search(r'data-react-props="({.*?})"', html, re.S)
    if not m:
        return {}
    raw = htmllib.unescape(m.group(1))
    return json.loads(raw).get("resultsData", {})


def search_page(query: str, page: int) -> tuple[list[str], int]:
    """Return (url_list, total_pages) for one search results page."""
    resp = requests.get(
        SEARCH_BASE,
        params={"query": query, "affiliate": AFFILIATE, "page": page},
        headers=HEADERS,
        timeout=30,
    )
    resp.raise_for_status()
    rd = _parse_results_data(resp.text)
    urls = [r["url"] for r in rd.get("results", []) if r.get("url")]
    total_pages = rd.get("totalPages", 1)
    return urls, total_pages


MAX_PAGES_PER_QUERY = 5  # 5 pages × 20 results = 100 results per query


def collect_result_urls(query: str) -> list[str]:
    """Fetch up to MAX_PAGES_PER_QUERY result pages and return deduplicated URLs."""
    urls, total_pages = search_page(query, 1)
    pages_to_fetch = min(total_pages, MAX_PAGES_PER_QUERY)
    for page in range(2, pages_to_fetch + 1):
        time.sleep(SEARCH_DELAY)
        more, _ = search_page(query, page)
        urls.extend(more)
    # Strip Akamai bm-verify params that get injected into some result URLs
    cleaned = []
    for u in urls:
        u = re.sub(r"[?&]bm-verify=[^&#]*", "", u).rstrip("?&")
        cleaned.append(u)
    return list(dict.fromkeys(cleaned))  # preserve order, deduplicate


# ---------------------------------------------------------------------------
# PDF link extraction (Playwright)
# ---------------------------------------------------------------------------

PDF_SELECTORS = [
    'a[href$=".pdf"]',
    'a[href*=".pdf?"]',
    'a[href*="/sites/default/files/"]',
    'a[href*="documents/"]',
]


def extract_pdf_links(pw_page, page_url: str) -> list[str]:
    """Navigate to page_url and return all PDF hrefs found."""
    try:
        pw_page.goto(page_url, wait_until="domcontentloaded", timeout=25_000)
        pw_page.wait_for_timeout(1_500)  # let any JS rendering finish
    except Exception as e:
        print(f"    [page error] {e}")
        return []

    links = pw_page.eval_on_selector_all(
        ", ".join(PDF_SELECTORS),
        "els => els.map(el => el.href)",
    )
    # Also catch direct .pdf links anywhere in the HTML
    extra = re.findall(
        r'https?://[^\s"\'<>]+\.pdf(?:[?#][^\s"\'<>]*)?',
        pw_page.content(),
        re.I,
    )
    return list(dict.fromkeys(links + extra))


# ---------------------------------------------------------------------------
# Download
# ---------------------------------------------------------------------------

def filename_from_url(url: str) -> str:
    path = urlparse(url).path
    name = path.rstrip("/").split("/")[-1]
    if not name.lower().endswith(".pdf"):
        name = hashlib.md5(url.encode()).hexdigest()[:16] + ".pdf"
    # Sanitise for filesystem
    name = re.sub(r'[^\w.\-]', '_', name)
    return name


def download_pdf(url: str, dest: Path, session_cookies: dict) -> bool:
    try:
        resp = requests.get(
            url,
            headers=HEADERS,
            cookies=session_cookies,
            timeout=90,
            stream=True,
        )
        resp.raise_for_status()
        content_type = resp.headers.get("Content-Type", "")
        if "pdf" not in content_type.lower() and not url.lower().endswith(".pdf"):
            print(f"    [skip] not a PDF ({content_type}): {url}")
            return False
        with open(dest, "wb") as f:
            for chunk in resp.iter_content(chunk_size=16_384):
                f.write(chunk)
        return True
    except Exception as e:
        print(f"    [download error] {e}")
        return False


# ---------------------------------------------------------------------------
# Main crawl loop
# ---------------------------------------------------------------------------

def crawl(query_iter, headless: bool = True):
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    seen_pages = load_set(SEEN_FILE)   # result pages already visited
    seen_pdfs  = load_set(PDF_LOG)     # PDF URLs already downloaded

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=headless)
        context = browser.new_context(
            user_agent=HEADERS["User-Agent"],
            accept_downloads=True,
        )

        # Inject Queue-IT cookie so justice.gov lets us through its waiting room
        context.add_cookies([{
            "name":   QUEUE_IT_COOKIE_NAME,
            "value":  QUEUE_IT_COOKIE_VALUE,
            "domain": ".justice.gov",
            "path":   "/",
        }])

        pw_page = context.new_page()

        # Warm-up: visit the Epstein page once so Akamai issues session cookies
        print(f"[warmup] navigating to {START_URL}")
        try:
            pw_page.goto(START_URL, wait_until="domcontentloaded", timeout=30_000)
            pw_page.wait_for_timeout(4_000)
        except Exception as e:
            print(f"[warmup error] {e} — continuing anyway")

        # Collect session cookies for use in requests-based downloads
        def get_session_cookies() -> dict:
            return {c["name"]: c["value"] for c in context.cookies()}

        # ---- Phase 1: collect all result page URLs via search API ----
        result_urls: set[str] = set()
        print("\n[phase 1] searching…")

        for query in query_iter:
            print(f"  QUERY: {query[:80]}")
            try:
                urls = collect_result_urls(query)
                new = [u for u in urls if u not in seen_pages]
                result_urls.update(new)
                print(f"    → {len(new)} new URLs (total {len(result_urls)})")
            except Exception as e:
                print(f"    [search error] {e}")
            time.sleep(SEARCH_DELAY)

        print(f"\n[phase 1 done] {len(result_urls)} unique result pages to visit")

        # ---- Phase 2: visit each result page and collect PDF links ----
        pdf_urls: set[str] = set()
        print("\n[phase 2] extracting PDF links…")

        for page_url in result_urls:
            if page_url in seen_pages:
                continue
            print(f"  PAGE: {page_url[:100]}")
            links = extract_pdf_links(pw_page, page_url)
            for link in links:
                if link not in seen_pdfs:
                    pdf_urls.add(link)
                    print(f"    PDF: {link[:100]}")
            append_to_file(SEEN_FILE, page_url)
            seen_pages.add(page_url)
            time.sleep(PAGE_DELAY)

        print(f"\n[phase 2 done] {len(pdf_urls)} PDF URLs to download")

        # ---- Phase 3: download PDFs ----
        print("\n[phase 3] downloading…")
        session_cookies = get_session_cookies()

        for pdf_url in pdf_urls:
            if pdf_url in seen_pdfs:
                continue
            filename = filename_from_url(pdf_url)
            dest = OUTPUT_DIR / filename
            if dest.exists():
                append_to_file(PDF_LOG, pdf_url)
                seen_pdfs.add(pdf_url)
                continue

            print(f"  DOWNLOAD: {filename}")
            ok = download_pdf(pdf_url, dest, session_cookies)
            if ok:
                size_kb = dest.stat().st_size / 1024
                print(f"    saved {size_kb:.0f} KB → {dest.name}")
                append_to_file(PDF_LOG, pdf_url)
                seen_pdfs.add(pdf_url)
            else:
                # Retry once through Playwright for files behind Akamai
                try:
                    print(f"    retrying via Playwright…")
                    with context.expect_download() as dl_info:
                        pw_page.goto(pdf_url, timeout=30_000)
                    download = dl_info.value
                    download.save_as(dest)
                    print(f"    saved via Playwright → {dest.name}")
                    append_to_file(PDF_LOG, pdf_url)
                    seen_pdfs.add(pdf_url)
                except Exception as e:
                    print(f"    [retry error] {e}")
                    if dest.exists():
                        dest.unlink()

            time.sleep(DOWNLOAD_DELAY)

        browser.close()

    print(f"\n[done] files saved to {OUTPUT_DIR}")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="DOJ Epstein document crawler")
    parser.add_argument("--limit",    type=int,  default=None,           help="max number of queries to run")
    parser.add_argument("--headless", type=str,  default="true",         help="true/false — show browser window")
    parser.add_argument("--reset",    action="store_true",               help="clear progress files and restart")
    args = parser.parse_args()

    if args.reset:
        for f in (SEEN_FILE, PDF_LOG):
            if f.exists():
                f.unlink()
        print("[reset] progress cleared")

    headless = args.headless.lower() != "false"
    queries  = generate_queries()
    if args.limit:
        from itertools import islice
        queries = islice(queries, args.limit)

    crawl(queries, headless=headless)
