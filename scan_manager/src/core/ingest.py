"""
Parallel PDF ingestion pipeline: OCR → SQLite with FTS5.
"""

import os
import sys
import multiprocessing
from pathlib import Path

from core.database import init_db, get_ingested_filenames, insert_document, detect_source

sys.stdout.reconfigure(line_buffering=True)
sys.stderr.reconfigure(line_buffering=True)


# Render PDFs at a lower DPI and only a few pages at a time so peak memory is
# bounded regardless of document size (a 263-page PDF no longer loads at once).
OCR_DPI = 150
PAGE_WINDOW = 4  # pages rendered into RAM per worker at any moment


def ocr_single_pdf(filepath: str) -> dict:
    """OCR a single PDF page-by-page (bounded memory). Runs in a worker process."""
    from pdf2image import convert_from_path, pdfinfo_from_path
    from PIL import ImageOps, ImageEnhance
    import pytesseract

    filepath = Path(filepath)
    try:
        total = int(pdfinfo_from_path(filepath)["Pages"])
    except Exception as e:
        return {"error": f"pdfinfo: {e}", "filename": filepath.name}

    pages = []
    try:
        for start in range(1, total + 1, PAGE_WINDOW):
            end = min(start + PAGE_WINDOW - 1, total)
            window = convert_from_path(
                filepath, dpi=OCR_DPI, first_page=start, last_page=end
            )
            for offset, img in enumerate(window):
                # Preprocess: grayscale → invert → contrast boost (matches doc_serializer)
                gray = img.convert("L")
                inverted = ImageOps.invert(gray)
                enhanced = ImageEnhance.Contrast(inverted).enhance(2.0)
                text = pytesseract.image_to_string(enhanced)
                pages.append({"number": start + offset, "text": text})
                # Free the page bitmaps before moving on so they don't accumulate.
                for im in (img, gray, inverted, enhanced):
                    im.close()
            del window
    except Exception as e:
        return {"error": str(e), "filename": filepath.name}

    return {
        "filename": filepath.name,
        "page_count": len(pages),
        "pages": pages,
        "full_text": "\n\n".join(p["text"] for p in pages),
    }


def ingest_all(input_dir: str, db_path: str, workers: int = None):
    """OCR all PDFs in input_dir and store in SQLite."""
    conn = init_db(db_path)
    already_done = get_ingested_filenames(conn)

    all_pdfs = sorted(Path(input_dir).glob("*.pdf"))
    pending = [str(p) for p in all_pdfs if p.name not in already_done]

    print(f"[ingest] {len(pending)} to process, {len(already_done)} already done, {len(all_pdfs)} total")

    if not pending:
        print("[ingest] nothing to do")
        return

    if workers is None:
        # Cap at 2 by default: peak memory ≈ workers × PAGE_WINDOW pages.
        workers = min(os.cpu_count() - 1, 2) if os.cpu_count() and os.cpu_count() > 1 else 1

    print(f"[ingest] using {workers} workers")

    done = 0
    errors = 0

    with multiprocessing.Pool(workers) as pool:
        for result in pool.imap_unordered(ocr_single_pdf, pending):
            done += 1
            filename = result.get("filename", "???")

            if "error" in result:
                errors += 1
                print(f"  [{done}/{len(pending)}] FAIL {filename}: {result['error']}")
                continue

            source = detect_source(filename)
            try:
                insert_document(
                    conn, filename, source,
                    result["page_count"], result["full_text"], result["pages"],
                )
                print(f"  [{done}/{len(pending)}] {filename} ({result['page_count']} pages, {source})")
            except Exception as e:
                errors += 1
                print(f"  [{done}/{len(pending)}] DB ERROR {filename}: {e}")

    conn.close()
    print(f"\n[ingest] done: {done - errors} ingested, {errors} errors")
