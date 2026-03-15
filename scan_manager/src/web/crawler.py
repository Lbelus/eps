"""
DOJ Epstein document crawler.

Crawls the structured Epstein Library on justice.gov/epstein/doj-disclosures,
which contains all released documents organised into data sets, court records,
FOIA releases, and declassified files.

The /multimedia-search endpoint (used by the page's search box) is blocked at
Akamai's WAF layer for automated clients. Instead we walk the disclosure index
directly — guaranteed to reach every document without search.

Flow:
  1. Start at /epstein/doj-disclosures and discover all sub-section URLs
  2. Visit each sub-section with Playwright (passes Akamai JS challenge)
  3. Collect all document download links (PDF, ZIP, etc.)
  4. Download each file to data/input/

Usage:
    python -m src.web.crawler                  # full run
    python -m src.web.crawler --headless false  # show browser window
    python -m src.web.crawler --reset           # clear progress and restart
"""

import argparse
import hashlib
import re
import sys
import time
from pathlib import Path
from urllib.parse import urljoin, urlparse

import requests
from playwright.sync_api import sync_playwright

sys.stdout.reconfigure(line_buffering=True)
sys.stderr.reconfigure(line_buffering=True)

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

BASE_URL      = "https://www.justice.gov"
DISCLOSURE_ROOT = "/epstein/doj-disclosures"

QUEUE_IT_COOKIE_NAME  = "QueueITAccepted-SDFrts345E-V3_usdojsearch"
QUEUE_IT_COOKIE_VALUE = (
    "EventId%3Dusdojsearch%26RedirectType%3Dsafetynet"
    "%26IssueTime%3D1773594786"
    "%26Hash%3Dcda37fddf24c0964e9f6a5b41af6ef8389e78e6f0c505158790393444a0c8221"
)

DATA_DIR   = Path(__file__).resolve().parents[2] / "data"
OUTPUT_DIR = DATA_DIR / "input"
SEEN_FILE  = DATA_DIR / "crawl_seen.txt"
PDF_LOG    = DATA_DIR / "crawl_pdfs.txt"

UA = (
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
    "AppleWebKit/537.36 (KHTML, like Gecko) "
    "Chrome/122.0.0.0 Safari/537.36"
)
HEADERS = {"User-Agent": UA, "Accept": "text/html,application/xhtml+xml,*/*;q=0.8"}

PAGE_DELAY     = 0.8
DOWNLOAD_DELAY = 1.0

# File extensions we care about
DOC_EXTENSIONS = re.compile(r'\.(pdf|zip|doc|docx|xls|xlsx|mp4|mov|avi)(\?[^"]*)?$', re.I)


# ---------------------------------------------------------------------------
# Progress helpers
# ---------------------------------------------------------------------------

def load_set(path: Path) -> set:
    if path.exists():
        return set(path.read_text(encoding="utf-8").splitlines())
    return set()

def append_line(path: Path, value: str):
    with open(path, "a", encoding="utf-8") as f:
        f.write(value + "\n")


# ---------------------------------------------------------------------------
# Link extraction
# ---------------------------------------------------------------------------

def extract_links(pw_page, base_url: str) -> tuple[list[str], list[str]]:
    """
    Return (section_links, doc_links) found on the current page.
    section_links: /epstein/... sub-pages to recurse into
    doc_links:     direct document download URLs
    """
    html = pw_page.content()

    all_hrefs = re.findall(r'href=["\']([^"\']+)["\']', html)

    section_links = []
    doc_links = []

    for href in all_hrefs:
        # Resolve relative URLs
        if href.startswith("/"):
            full = BASE_URL + href
        elif href.startswith("http"):
            full = href
        else:
            full = urljoin(base_url, href)

        # Strip Akamai bm-verify noise
        full = re.sub(r"[?&]bm-verify=[^&#]*", "", full).rstrip("?&")

        parsed = urlparse(full)

        # Document files
        if DOC_EXTENSIONS.search(parsed.path):
            doc_links.append(full)
            continue

        # Sub-sections of the Epstein disclosure tree on justice.gov
        if "justice.gov" in parsed.netloc and parsed.path.startswith("/epstein/"):
            if parsed.path not in (DISCLOSURE_ROOT, "/epstein", "/epstein/"):
                section_links.append(full)

    return list(dict.fromkeys(section_links)), list(dict.fromkeys(doc_links))


# ---------------------------------------------------------------------------
# Filename
# ---------------------------------------------------------------------------

def filename_from_url(url: str) -> str:
    path = urlparse(url).path
    name = path.rstrip("/").split("/")[-1]
    name = re.sub(r"[^\w.\-]", "_", name)
    if not name or name == "_":
        name = hashlib.md5(url.encode()).hexdigest()[:16]
    # Ensure it has an extension
    if "." not in name:
        name += ".pdf"
    return name


# ---------------------------------------------------------------------------
# Download
# ---------------------------------------------------------------------------

def download_file(url: str, dest: Path, session_cookies: dict) -> bool:
    try:
        resp = requests.get(
            url, headers=HEADERS, cookies=session_cookies,
            timeout=120, stream=True, allow_redirects=True,
        )
        resp.raise_for_status()
        with open(dest, "wb") as f:
            for chunk in resp.iter_content(chunk_size=32_768):
                f.write(chunk)
        return dest.stat().st_size > 0
    except Exception as e:
        print(f"    [download error] {e}")
        return False


# ---------------------------------------------------------------------------
# Main crawl
# ---------------------------------------------------------------------------

def crawl(headless: bool = True):
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    seen_pages = load_set(SEEN_FILE)
    seen_docs  = load_set(PDF_LOG)

    with sync_playwright() as p:
        browser = p.chromium.launch(
            headless=headless,
            args=["--disable-blink-features=AutomationControlled"],
        )
        context = browser.new_context(
            user_agent=UA,
            accept_downloads=True,
        )
        context.add_init_script(
            'Object.defineProperty(navigator, "webdriver", {get: () => undefined})'
        )
        context.add_cookies([{
            "name":   QUEUE_IT_COOKIE_NAME,
            "value":  QUEUE_IT_COOKIE_VALUE,
            "domain": ".justice.gov",
            "path":   "/",
        }])

        pw_page = context.new_page()

        # Warm up: let Akamai issue a valid session cookie
        print(f"[warmup] {BASE_URL}/epstein")
        pw_page.goto(f"{BASE_URL}/epstein", wait_until="networkidle", timeout=45_000)
        pw_page.wait_for_timeout(4_000)

        # ---- Phase 1: walk the disclosure tree and collect all doc links ----
        queue   = [BASE_URL + DISCLOSURE_ROOT]
        doc_urls: set[str] = set()

        print(f"\n[phase 1] walking disclosure tree…")

        while queue:
            page_url = queue.pop(0)

            if page_url in seen_pages:
                continue

            print(f"  SECTION: {page_url.replace(BASE_URL, '')}")
            try:
                pw_page.goto(page_url, wait_until="domcontentloaded", timeout=25_000)
                pw_page.wait_for_timeout(1_500)
            except Exception as e:
                print(f"    [nav error] {e}")
                seen_pages.add(page_url)
                append_line(SEEN_FILE, page_url)
                continue

            sections, docs = extract_links(pw_page, page_url)

            new_sections = [s for s in sections if s not in seen_pages and s not in queue]
            new_docs     = [d for d in docs     if d not in seen_docs]

            queue.extend(new_sections)
            doc_urls.update(new_docs)

            if new_docs:
                print(f"    → {len(new_docs)} new docs  |  queue: {len(queue)}  |  total docs: {len(doc_urls)}")

            seen_pages.add(page_url)
            append_line(SEEN_FILE, page_url)
            time.sleep(PAGE_DELAY)

        print(f"\n[phase 1 done] {len(doc_urls)} documents to download")

        # ---- Phase 2: download ----
        session_cookies = {c["name"]: c["value"] for c in context.cookies()}

        print(f"\n[phase 2] downloading…")
        downloaded = 0

        for doc_url in sorted(doc_urls):
            if doc_url in seen_docs:
                continue

            filename = filename_from_url(doc_url)
            dest = OUTPUT_DIR / filename

            if dest.exists():
                append_line(PDF_LOG, doc_url)
                seen_docs.add(doc_url)
                continue

            print(f"  {filename}")
            ok = download_file(doc_url, dest, session_cookies)

            if ok:
                size_kb = dest.stat().st_size / 1024
                print(f"    saved {size_kb:,.0f} KB")
                append_line(PDF_LOG, doc_url)
                seen_docs.add(doc_url)
                downloaded += 1
            else:
                # Retry via Playwright (handles redirects behind Akamai)
                print(f"    retrying via Playwright…")
                try:
                    with context.expect_download(timeout=30_000) as dl_info:
                        pw_page.goto(doc_url, timeout=30_000)
                    dl = dl_info.value
                    dl.save_as(dest)
                    size_kb = dest.stat().st_size / 1024
                    print(f"    saved {size_kb:,.0f} KB (via Playwright)")
                    append_line(PDF_LOG, doc_url)
                    seen_docs.add(doc_url)
                    downloaded += 1
                except Exception as e:
                    print(f"    [failed] {e}")
                    if dest.exists():
                        dest.unlink()

            time.sleep(DOWNLOAD_DELAY)

        browser.close()

    print(f"\n[done] {downloaded} files downloaded to {OUTPUT_DIR}")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="DOJ Epstein document crawler")
    parser.add_argument("--headless", default="true", help="true/false")
    parser.add_argument("--reset", action="store_true", help="clear progress and restart")
    args = parser.parse_args()

    if args.reset:
        for f in (SEEN_FILE, PDF_LOG):
            if f.exists():
                f.unlink()
        print("[reset] progress cleared")

    crawl(headless=args.headless.lower() != "false")
