"""
Epstein/Maxwell document crawler.

Searches for documents related to Jeffrey Epstein, Ghislaine Maxwell, and
associated individuals across multiple public sources:

  1. DOJ Epstein Disclosures  — walks justice.gov/epstein/doj-disclosures tree
  2. CourtListener             — RECAP archive of federal court filings
  3. DocumentCloud             — public document repository (FOIA releases, etc.)

Search subjects are defined in config/search_subjects.yaml.

Usage:
    python -m src.web.crawler --prime                 # one-time: open headed browser,
                                                       # solve any age gate / queue,
                                                       # save session for reuse.
    python -m src.web.crawler                         # full run (all sources)
    python -m src.web.crawler --source doj            # DOJ tree only
    python -m src.web.crawler --source courtlistener  # CourtListener only
    python -m src.web.crawler --source documentcloud  # DocumentCloud only
    python -m src.web.crawler --headless false        # show browser window
    python -m src.web.crawler --reset                 # clear progress and restart
"""

import argparse
import hashlib
import json
import re
import sys
import time
from pathlib import Path
from urllib.parse import urljoin, urlparse, quote_plus

import requests
import yaml
from playwright.sync_api import sync_playwright

# playwright-stealth: optional but recommended for the DOJ source.
# Supports both the legacy module-level stealth_sync() and the newer Stealth() class.
_STEALTH_FN = None
try:
    from playwright_stealth import stealth_sync as _STEALTH_FN  # legacy API
except ImportError:
    try:
        from playwright_stealth import Stealth
        _STEALTH_FN = Stealth().apply_stealth_sync  # newer API
    except ImportError:
        _STEALTH_FN = None

sys.stdout.reconfigure(line_buffering=True)
sys.stderr.reconfigure(line_buffering=True)

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

CONFIG_DIR = Path(__file__).resolve().parents[2] / "config"
DATA_DIR   = Path(__file__).resolve().parents[2] / "data"
OUTPUT_DIR = DATA_DIR / "input"
PDF_LOG    = DATA_DIR / "crawl_pdfs.txt"
STATE_FILE = DATA_DIR / "doj_session_state.json"

# ---------------------------------------------------------------------------
# Search subjects
# ---------------------------------------------------------------------------

def load_subjects() -> dict:
    path = CONFIG_DIR / "search_subjects.yaml"
    with open(path, encoding="utf-8") as f:
        return yaml.safe_load(f)


def build_search_terms(subjects: dict) -> list[str]:
    """Flatten all name lists into unique search terms, plus the explicit queries."""
    terms = []
    for key in ("primary", "politicians", "legal", "business",
                "entertainment", "inner_circle", "victims_public"):
        terms.extend(subjects.get(key, []))
    terms.extend(subjects.get("queries", []))
    return list(dict.fromkeys(terms))  # dedupe, preserve order

# ---------------------------------------------------------------------------
# HTTP / browser config
# ---------------------------------------------------------------------------

BASE_URL        = "https://www.justice.gov"
DISCLOSURE_ROOT = "/epstein/doj-disclosures"
PRIME_URL       = BASE_URL + "/epstein"

UA = (
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
    "AppleWebKit/537.36 (KHTML, like Gecko) "
    "Chrome/122.0.0.0 Safari/537.36"
)
HEADERS = {"User-Agent": UA, "Accept": "text/html,application/xhtml+xml,*/*;q=0.8"}

# Markers that suggest the page Akamai/Queue-It returned is a challenge, not content.
CHALLENGE_MARKERS = (
    "Just a moment",
    "Verifying you are human",
    "queue-it",
    "queue.justice.gov",
    "Access Denied",
    "Pardon Our Interruption",
)

# Init script: pad the bot-detection surface beyond just webdriver.
# Run before any page script. Defensible posture for public-records research,
# not a challenge solver.
STEALTH_INIT_SCRIPT = """
Object.defineProperty(navigator, 'webdriver', { get: () => undefined });
Object.defineProperty(navigator, 'languages', { get: () => ['en-US', 'en'] });
Object.defineProperty(navigator, 'plugins',   { get: () => [1, 2, 3, 4, 5] });
Object.defineProperty(navigator, 'platform',  { get: () => 'MacIntel' });
window.chrome = window.chrome || { runtime: {} };
const originalQuery = window.navigator.permissions && window.navigator.permissions.query;
if (originalQuery) {
    window.navigator.permissions.query = (parameters) => (
        parameters.name === 'notifications'
            ? Promise.resolve({ state: Notification.permission })
            : originalQuery(parameters)
    );
}
"""

PAGE_DELAY     = 0.8
DOWNLOAD_DELAY = 1.0
SEARCH_DELAY   = 1.5

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
# DOJ session helpers
# ---------------------------------------------------------------------------

def apply_stealth(page) -> None:
    """Apply playwright-stealth fingerprint patches to a page, if installed."""
    if _STEALTH_FN is None:
        return
    try:
        _STEALTH_FN(page)
    except Exception as e:
        print(f"    [stealth warning] {e}")


def page_looks_like_challenge(page) -> bool:
    """Return True if the current page appears to be a bot/queue challenge."""
    try:
        title = (page.title() or "").strip()
        body  = page.content()[:4000]
    except Exception:
        return False
    if any(m.lower() in title.lower() for m in CHALLENGE_MARKERS):
        return True
    if any(m in body for m in CHALLENGE_MARKERS):
        return True
    return False


def build_doj_context(browser, accept_downloads: bool = True):
    """Create a browser context, loading saved DOJ storage state if available."""
    kwargs = dict(user_agent=UA, accept_downloads=accept_downloads)
    if STATE_FILE.exists():
        kwargs["storage_state"] = str(STATE_FILE)
        print(f"[session] loaded {STATE_FILE.name}")
    else:
        print(f"[session] no saved state at {STATE_FILE.name} — run --prime first if Akamai blocks")
    context = browser.new_context(**kwargs)
    context.add_init_script(STEALTH_INIT_SCRIPT)
    return context


def warmup_doj(page, max_attempts: int = 3) -> bool:
    """Navigate to PRIME_URL with retry/back-off. Returns True if the page is content (not a challenge)."""
    for attempt in range(1, max_attempts + 1):
        try:
            print(f"[warmup] {PRIME_URL} (attempt {attempt}/{max_attempts})")
            page.goto(PRIME_URL, wait_until="networkidle", timeout=45_000)
            page.wait_for_timeout(3_000)
        except Exception as e:
            print(f"    [warmup error] {e}")
            time.sleep(2 * attempt)
            continue
        if page_looks_like_challenge(page):
            print(f"    [warmup] challenge page detected, backing off...")
            time.sleep(4 * attempt)
            continue
        return True
    return False


def prime_session(headless: bool = False) -> int:
    """One-time interactive prime: open a real browser, let the user solve any gate, save state."""
    print(f"\n{'='*60}")
    print(f"[prime] opening {PRIME_URL} in a {'headless' if headless else 'headed'} browser")
    print(f"[prime] solve any age gate / queue, then return here and press Enter")
    print(f"{'='*60}\n")

    with sync_playwright() as p:
        browser = p.chromium.launch(
            headless=headless,
            args=["--disable-blink-features=AutomationControlled"],
        )
        context = browser.new_context(user_agent=UA, accept_downloads=True)
        context.add_init_script(STEALTH_INIT_SCRIPT)
        page = context.new_page()
        apply_stealth(page)

        try:
            page.goto(PRIME_URL, wait_until="domcontentloaded", timeout=60_000)
        except Exception as e:
            print(f"[prime] navigation error: {e}")

        try:
            input("Press Enter once the disclosure page is fully loaded and content is visible... ")
        except EOFError:
            pass

        if page_looks_like_challenge(page):
            print("[prime] page still looks like a challenge — state will NOT be saved")
            browser.close()
            return 1

        STATE_FILE.parent.mkdir(parents=True, exist_ok=True)
        context.storage_state(path=str(STATE_FILE))
        print(f"[prime] saved session state to {STATE_FILE}")
        browser.close()
    return 0

# ---------------------------------------------------------------------------
# Link extraction (DOJ)
# ---------------------------------------------------------------------------

def extract_links(pw_page, base_url: str) -> tuple[list[str], list[str]]:
    """Return (section_links, doc_links) from current page."""
    html = pw_page.content()
    all_hrefs = re.findall(r'href=["\']([^"\']+)["\']', html)

    section_links = []
    doc_links = []

    for href in all_hrefs:
        if href.startswith("#") or not href.strip():
            continue

        if href.startswith("/"):
            full = BASE_URL + href
        elif href.startswith("http"):
            full = href
        else:
            full = urljoin(base_url, href)

        full = re.sub(r"[?&]bm-verify=[^&#]*", "", full).rstrip("?&")
        full = full.split("#")[0]

        parsed = urlparse(full)

        if parsed.query == "page=0":
            full = full.replace("?page=0", "")
            parsed = urlparse(full)

        if DOC_EXTENSIONS.search(parsed.path):
            doc_links.append(full)
            continue

        if "justice.gov" in parsed.netloc and parsed.path.startswith("/epstein/"):
            if parsed.path not in (DISCLOSURE_ROOT, "/epstein", "/epstein/"):
                section_links.append(full)

    return list(dict.fromkeys(section_links)), list(dict.fromkeys(doc_links))

# ---------------------------------------------------------------------------
# Filename
# ---------------------------------------------------------------------------

def filename_from_url(url: str, prefix: str = "") -> str:
    path = urlparse(url).path
    name = path.rstrip("/").split("/")[-1]
    name = re.sub(r"[^\w.\-]", "_", name)
    if not name or name == "_":
        name = hashlib.md5(url.encode()).hexdigest()[:16]
    if "." not in name:
        name += ".pdf"
    if prefix:
        name = f"{prefix}_{name}"
    return name

# ---------------------------------------------------------------------------
# Download
# ---------------------------------------------------------------------------

def is_html_gate(path: Path) -> bool:
    try:
        with open(path, "rb") as f:
            header = f.read(512)
        return b"<!DOCTYPE html" in header or b"<html" in header
    except Exception:
        return False


def download_file(url: str, dest: Path, session_cookies: dict = None) -> bool:
    try:
        resp = requests.get(
            url, headers=HEADERS, cookies=session_cookies or {},
            timeout=120, stream=True, allow_redirects=True,
        )
        resp.raise_for_status()
        with open(dest, "wb") as f:
            for chunk in resp.iter_content(chunk_size=32_768):
                f.write(chunk)
        if dest.stat().st_size == 0 or is_html_gate(dest):
            dest.unlink()
            return False
        return True
    except Exception as e:
        print(f"    [download error] {e}")
        return False


# ---------------------------------------------------------------------------
# Source 1: DOJ disclosure tree walk
# ---------------------------------------------------------------------------

def crawl_doj(pw_page, context, seen_docs: set) -> set:
    """Walk justice.gov/epstein/doj-disclosures and return discovered doc URLs.

    Visited-section tracking is process-local: every run retraverses the full
    tree so newly-published sections are picked up. Download dedup is still
    handled by the persistent PDF_LOG via seen_docs.
    """
    queue = [BASE_URL + DISCLOSURE_ROOT]
    visited: set[str] = set()
    doc_urls: set[str] = set()

    print(f"\n{'='*60}")
    print(f"[DOJ] walking disclosure tree...")
    print(f"{'='*60}")

    while queue:
        page_url = queue.pop(0)
        if page_url in visited:
            continue

        print(f"  SECTION: {page_url.replace(BASE_URL, '')}")
        try:
            pw_page.goto(page_url, wait_until="domcontentloaded", timeout=25_000)
            pw_page.wait_for_timeout(1_500)
        except Exception as e:
            print(f"    [nav error] {e}")
            visited.add(page_url)
            continue

        sections, docs = extract_links(pw_page, page_url)
        new_sections = [s for s in sections if s not in visited and s not in queue]
        new_docs     = [d for d in docs     if d not in seen_docs]

        queue.extend(new_sections)
        doc_urls.update(new_docs)

        if new_docs:
            print(f"    -> {len(new_docs)} new docs  |  queue: {len(queue)}  |  total: {len(doc_urls)}")

        visited.add(page_url)
        time.sleep(PAGE_DELAY)

    print(f"[DOJ] {len(doc_urls)} documents found  |  {len(visited)} sections visited")
    return doc_urls


# ---------------------------------------------------------------------------
# Source 2: CourtListener RECAP archive
# ---------------------------------------------------------------------------

COURTLISTENER_API = "https://www.courtlistener.com/api/rest/v4"

def search_courtlistener(search_terms: list[str], seen_docs: set) -> set:
    """
    Search CourtListener's RECAP archive for Epstein/Maxwell-related filings.
    Uses the free public API (no auth required for search).
    """
    doc_urls: set[str] = set()

    print(f"\n{'='*60}")
    print(f"[CourtListener] searching RECAP archive...")
    print(f"{'='*60}")

    cl_headers = {**HEADERS, "Accept": "application/json"}

    searched = set()
    for query in search_terms:
        if query in searched:
            continue
        searched.add(query)

        print(f"  QUERY: {query}")
        try:
            resp = requests.get(
                f"{COURTLISTENER_API}/search/",
                params={
                    "q": query,
                    "type": "r",  # RECAP documents
                    "order_by": "score desc",
                    "page_size": 20,
                },
                headers=cl_headers,
                timeout=30,
            )
            if resp.status_code == 429:
                print(f"    [rate limited] waiting 30s...")
                time.sleep(30)
                continue
            resp.raise_for_status()
            data = resp.json()
        except Exception as e:
            print(f"    [error] {e}")
            time.sleep(SEARCH_DELAY)
            continue

        results = data.get("results", [])
        new_count = 0

        for result in results:
            # Extract PDF URLs from recap_documents nested in each result
            recap_docs = result.get("recap_documents", [])
            for rdoc in recap_docs:
                filepath = rdoc.get("filepath_local", "")
                if filepath:
                    url = f"https://storage.courtlistener.com/{filepath}"
                    if url not in seen_docs and url not in doc_urls:
                        doc_urls.add(url)
                        new_count += 1

            # Also check top-level filepath_local
            filepath_local = result.get("filepath_local", "")
            if filepath_local:
                url = f"https://storage.courtlistener.com/{filepath_local}"
                if url not in seen_docs and url not in doc_urls:
                    doc_urls.add(url)
                    new_count += 1

        if new_count:
            print(f"    -> {new_count} new docs  |  total: {len(doc_urls)}")

        time.sleep(SEARCH_DELAY)

    print(f"[CourtListener] {len(doc_urls)} documents found")
    return doc_urls


# ---------------------------------------------------------------------------
# Source 3: DocumentCloud
# ---------------------------------------------------------------------------

DOCUMENTCLOUD_API = "https://api.www.documentcloud.org/api"

def search_documentcloud(search_terms: list[str], seen_docs: set) -> set:
    """
    Search DocumentCloud for Epstein/Maxwell-related public documents.
    """
    doc_urls: set[str] = set()

    print(f"\n{'='*60}")
    print(f"[DocumentCloud] searching public documents...")
    print(f"{'='*60}")

    # Use focused queries — primary subjects and key phrases
    dc_queries = [t for t in search_terms if any(kw in t.lower() for kw in
                  ("epstein", "maxwell", "giuffre", "flight log", "black book",
                   "little st james", "zorro ranch", "trafficking"))]
    # Also add a few combined queries
    dc_queries.append("Epstein Maxwell")
    dc_queries = list(dict.fromkeys(dc_queries))

    searched = set()
    for query in dc_queries:
        if query in searched:
            continue
        searched.add(query)

        print(f"  QUERY: {query}")
        try:
            resp = requests.get(
                f"{DOCUMENTCLOUD_API}/documents/search/",
                params={
                    "q": query,
                    "per_page": 25,
                    "hl": "false",
                },
                headers={**HEADERS, "Accept": "application/json"},
                timeout=30,
            )
            if resp.status_code == 429:
                print(f"    [rate limited] waiting 30s...")
                time.sleep(30)
                continue
            resp.raise_for_status()
            data = resp.json()
        except Exception as e:
            print(f"    [error] {e}")
            time.sleep(SEARCH_DELAY)
            continue

        results = data.get("results", [])
        new_count = 0

        for doc in results:
            # Construct PDF URL from id + slug
            # asset_url is just the S3 base ("https://s3.documentcloud.org/")
            # Full path is: {asset_url}documents/{id}/{slug}.pdf
            doc_id = doc.get("id")
            slug = doc.get("slug", "")
            asset_base = doc.get("asset_url", "https://s3.documentcloud.org/").rstrip("/")

            pdf_url = None
            if doc_id and slug:
                pdf_url = f"{asset_base}/documents/{doc_id}/{slug}.pdf"
            elif doc_id:
                pdf_url = f"{asset_base}/documents/{doc_id}/document.pdf"

            if pdf_url and pdf_url not in seen_docs:
                doc_urls.add(pdf_url)
                new_count += 1

        if new_count:
            print(f"    -> {new_count} new docs  |  total: {len(doc_urls)}")

        time.sleep(SEARCH_DELAY)

    print(f"[DocumentCloud] {len(doc_urls)} documents found")
    return doc_urls


# ---------------------------------------------------------------------------
# Main crawl
# ---------------------------------------------------------------------------

def crawl(headless: bool = True, sources: list[str] = None):
    if sources is None:
        sources = ["doj", "courtlistener", "documentcloud"]

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    seen_docs  = load_set(PDF_LOG)

    subjects = load_subjects()
    search_terms = build_search_terms(subjects)
    print(f"[config] loaded {len(search_terms)} search terms from search_subjects.yaml")

    all_doc_urls: set[str] = set()

    # ---- Source 1: DOJ disclosure tree (needs Playwright) ----
    if "doj" in sources:
        with sync_playwright() as p:
            browser = p.chromium.launch(
                headless=headless,
                args=["--disable-blink-features=AutomationControlled"],
            )
            context = build_doj_context(browser)
            pw_page = context.new_page()
            apply_stealth(pw_page)

            if not warmup_doj(pw_page):
                print("[doj] warmup failed — page still looks like a challenge.")
                print("[doj] re-prime the session:  python -m src.web.crawler --prime")
                browser.close()
                if "courtlistener" not in sources and "documentcloud" not in sources:
                    return
            else:
                doj_docs = crawl_doj(pw_page, context, seen_docs)
                all_doc_urls.update(doj_docs)

                # Save session cookies for DOJ downloads
                doj_cookies = {c["name"]: c["value"] for c in context.cookies()}

                # ---- DOJ downloads (inside Playwright context for retry) ----
                doj_new = doj_docs - seen_docs
                if doj_new:
                    print(f"\n[download] {len(doj_new)} DOJ documents...")
                    downloaded = _download_batch(
                        sorted(doj_new), seen_docs, doj_cookies,
                        prefix="", pw_page=pw_page, context=context,
                    )
                    print(f"[download] {downloaded} DOJ files saved")

                # Persist any refreshed cookies/state for the next run.
                try:
                    STATE_FILE.parent.mkdir(parents=True, exist_ok=True)
                    context.storage_state(path=str(STATE_FILE))
                except Exception as e:
                    print(f"[session] could not save state: {e}")

                browser.close()

    # ---- Source 2: CourtListener ----
    if "courtlistener" in sources:
        cl_docs = search_courtlistener(search_terms, seen_docs)
        all_doc_urls.update(cl_docs)

        cl_new = cl_docs - seen_docs
        if cl_new:
            print(f"\n[download] {len(cl_new)} CourtListener documents...")
            downloaded = _download_batch(sorted(cl_new), seen_docs, prefix="cl")
            print(f"[download] {downloaded} CourtListener files saved")

    # ---- Source 3: DocumentCloud ----
    if "documentcloud" in sources:
        dc_docs = search_documentcloud(search_terms, seen_docs)
        all_doc_urls.update(dc_docs)

        dc_new = dc_docs - seen_docs
        if dc_new:
            print(f"\n[download] {len(dc_new)} DocumentCloud documents...")
            downloaded = _download_batch(sorted(dc_new), seen_docs, prefix="dc")
            print(f"[download] {downloaded} DocumentCloud files saved")

    total_new = all_doc_urls - load_set(PDF_LOG)
    print(f"\n{'='*60}")
    print(f"[done] total documents across all sources: {len(all_doc_urls)}")
    print(f"[done] files in {OUTPUT_DIR}")
    print(f"{'='*60}")


def _download_via_playwright(doc_url: str, dest: Path, pw_page) -> bool:
    """Download a DOJ document.

    Most justice.gov disclosure PDFs are served with
    Content-Disposition: attachment, which makes Playwright's page.goto()
    raise "Download is starting" before we can extract anything. So:

      1. Try a plain requests.get() with the cookies the browser already
         holds. This is what works for nearly every direct PDF URL and
         avoids driving Chromium per-file.
      2. If the response was HTML (download_file() deletes those and
         returns False), the URL is probably an age gate / interstitial.
         Drive Chromium, click through #age-button-yes, and capture the
         download triggered by the click.
    """
    cookies = pw_page.context.cookies()
    cookie_dict = {c["name"]: c["value"] for c in cookies}

    if download_file(doc_url, dest, cookie_dict):
        return True

    # Fall back: render the URL, look for an age gate, click it, capture download.
    try:
        pw_page.goto(doc_url, wait_until="domcontentloaded", timeout=30_000)
    except Exception:
        # goto raised (e.g. Download is starting). Re-pull cookies in case the
        # server set new ones in the abortive response, then retry the HTTP path.
        cookies = pw_page.context.cookies()
        cookie_dict = {c["name"]: c["value"] for c in cookies}
        return download_file(doc_url, dest, cookie_dict)

    age_btn = pw_page.query_selector("#age-button-yes")
    if age_btn:
        print(f"    [age gate] clicking Yes...")
        try:
            with pw_page.expect_download(timeout=30_000) as dl_info:
                age_btn.click()
            dl_info.value.save_as(str(dest))
            return dest.exists() and dest.stat().st_size > 0
        except Exception as e:
            print(f"    [age gate download error] {e}")

    # Page rendered something else (or no age gate); try requests one more time
    # with whatever cookies the page picked up.
    cookies = pw_page.context.cookies()
    cookie_dict = {c["name"]: c["value"] for c in cookies}
    return download_file(doc_url, dest, cookie_dict)


def _download_batch(
    doc_urls: list[str],
    seen_docs: set,
    session_cookies: dict = None,
    prefix: str = "",
    pw_page=None,
    context=None,
) -> int:
    """Download a batch of document URLs. Returns count of successful downloads."""
    downloaded = 0

    for doc_url in doc_urls:
        if doc_url in seen_docs:
            continue

        filename = filename_from_url(doc_url, prefix=prefix)
        dest = OUTPUT_DIR / filename

        if dest.exists():
            append_line(PDF_LOG, doc_url)
            seen_docs.add(doc_url)
            continue

        print(f"  {filename}")

        ok = False

        if pw_page:
            # For DOJ: use Playwright as primary (bypasses Akamai)
            ok = _download_via_playwright(doc_url, dest, pw_page)
        else:
            # For CourtListener/DocumentCloud: direct requests works fine
            ok = download_file(doc_url, dest, session_cookies)

        if ok and dest.exists() and dest.stat().st_size > 0 and not is_html_gate(dest):
            size_kb = dest.stat().st_size / 1024
            print(f"    saved {size_kb:,.0f} KB")
            append_line(PDF_LOG, doc_url)
            seen_docs.add(doc_url)
            downloaded += 1
        else:
            if dest.exists():
                dest.unlink()
            print(f"    [failed]")

        time.sleep(DOWNLOAD_DELAY)

    return downloaded


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

VALID_SOURCES = ["doj", "courtlistener", "documentcloud"]

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Epstein/Maxwell document crawler")
    parser.add_argument("--headless", default="true", help="true/false")
    parser.add_argument("--reset", action="store_true", help="clear progress and restart")
    parser.add_argument(
        "--prime", action="store_true",
        help="open a headed browser to solve any age gate / queue, save the session, then exit",
    )
    parser.add_argument(
        "--source", default="all",
        help=f"comma-separated sources: {', '.join(VALID_SOURCES)}, or 'all'",
    )
    args = parser.parse_args()

    if args.reset:
        # crawl_seen.txt is no longer written, but clean it up if a previous run left one.
        for f in (PDF_LOG, STATE_FILE, DATA_DIR / "crawl_seen.txt"):
            if f.exists():
                f.unlink()
        print("[reset] progress cleared")

    if args.prime:
        sys.exit(prime_session(headless=False))

    if args.source == "all":
        sources = VALID_SOURCES
    else:
        sources = [s.strip() for s in args.source.split(",")]
        for s in sources:
            if s not in VALID_SOURCES:
                parser.error(f"unknown source: {s}. Choose from: {', '.join(VALID_SOURCES)}")

    crawl(headless=args.headless.lower() != "false", sources=sources)
