import Head from "next/head";
import Link from "next/link";
import Footer from "@/components/landingPage/footer";
import PopupWidget from "@/components/landingPage/popupWidget";

type ChangelogEntry = {
  date: string;
  title: string;
  description: string;
};

const entries: ChangelogEntry[] = [
  {
    date: "2026-07-28",
    title: "Corpus snapshot expanded to dataset 9",
    description:
      "The compressed SQLite corpus snapshot was refreshed to 345,494 documents and 613,880 pages, adding the latest torrent dataset to the searchable archive.",
  },
  {
    date: "2026-07-25",
    title: "Corpus snapshot expanded with datasets 6, 7, and 8",
    description:
      "The compressed SQLite corpus snapshot was refreshed to 15,274 documents and 153,381 pages after ingesting the next public data batches.",
  },
  {
    date: "2026-07-20",
    title: "Current-page CSV export",
    description:
      "The document search page now lets visitors download the currently loaded result page as a metadata-only CSV, making it easier to keep a local shortlist without exporting OCR text.",
  },
  {
    date: "2026-07-20",
    title: "Cookie and privacy notice",
    description:
      "A site-wide notice now explains that EPS does not use analytics cookies, while the privacy page documents local preference storage and standard hosting logs.",
  },
  {
    date: "2026-07-19",
    title: "Document size filtering",
    description:
      "The document search UI and REST API now support page-count range filters, letting visitors narrow results from zero pages up to any maximum document size.",
  },
  {
    date: "2026-07-19",
    title: "Multi-source document filtering",
    description:
      "Search results can now be filtered across one or more document sources, including DOJ, CourtListener, and DocumentCloud records, without leaving the document viewer.",
  },
  {
    date: "2026-07-17",
    title: "Detailed project changelog",
    description:
      "The public changelog was expanded into a curated technical timeline so visitors can understand how EPS has evolved without exposing internal development metadata.",
  },
  {
    date: "2026-07-17",
    title: "Frontend request limits",
    description:
      "The document search UI now caps result requests at 100 documents, matching the REST API limit and preventing oversized browser requests.",
  },
  {
    date: "2026-07-16",
    title: "SQLite to MySQL migration hardening",
    description:
      "The migration script now inserts OCR-heavy text through prepared row-level statements, preventing arbitrary document text from breaking generated SQL batches.",
  },
  {
    date: "2026-07-16",
    title: "Filename search",
    description:
      "A filename search mode was added to the document viewer, backed by a metadata-only REST endpoint for quickly finding records by file name.",
  },
  {
    date: "2026-07-15",
    title: "Largest corpus refresh so far",
    description:
      "The compressed SQLite corpus was refreshed to 4,437 documents and 112,000 pages, incorporating newly collected CourtListener and DocumentCloud material.",
  },
  {
    date: "2026-07-15",
    title: "Contact access and inquiry form",
    description:
      "A dedicated contact page and floating contact widget were added so visitors can reach the project maintainers from the site.",
  },
  {
    date: "2026-07-12",
    title: "REST API startup orchestration groundwork",
    description:
      "Internal route registration and connection-pool management were reorganized to make the REST API easier to start from configuration rather than scattered hardcoded setup.",
  },
  {
    date: "2026-07-09",
    title: "Expanded crawler subjects",
    description:
      "Crawler search subjects grew from 44 to 71 terms, adding estate, banking, staff, and witness-related topics while improving multi-word query behavior.",
  },
  {
    date: "2026-07-09",
    title: "Corpus refresh after keyword expansion",
    description:
      "The corpus was refreshed to 4,397 documents and 108,372 pages after broadening search coverage and pruning false positives.",
  },
  {
    date: "2026-07-05",
    title: "Read-only API response caching",
    description:
      "The nginx layer now caches read-only corpus responses, reducing repeated load on the C++ API and database for common list, document, and search requests.",
  },
  {
    date: "2026-07-05",
    title: "Compressed JSON API responses",
    description:
      "JSON responses are gzip-compressed through the reverse proxy, which is especially useful for text-heavy document and search payloads.",
  },
  {
    date: "2026-07-05",
    title: "Safer pagination parameters",
    description:
      "REST API list and search parameters are now validated and clamped, preventing oversized result sets and malformed query values from disrupting requests.",
  },
  {
    date: "2026-07-05",
    title: "Bounded search snippets",
    description:
      "Search results now fetch a bounded text window for snippets instead of transferring full document text for every hit.",
  },
  {
    date: "2026-07-05",
    title: "Metadata-only document listing",
    description:
      "The document list endpoint now returns metadata without full OCR text, making list views lighter while keeping full text available through detail endpoints.",
  },
  {
    date: "2026-07-05",
    title: "UTF-8 safe snippets",
    description:
      "Search snippet generation was adjusted to avoid cutting through multibyte characters, preventing invalid JSON from strict parsers.",
  },
  {
    date: "2026-07-01",
    title: "Corpus cleanup refresh",
    description:
      "The corpus was refreshed to about 3,815 documents and 67,113 pages after removing use-of-force false positives from the collected set.",
  },
  {
    date: "2026-06-29",
    title: "Connection pool manager groundwork",
    description:
      "REST API infrastructure work began on managing multiple connection pools, laying groundwork for broader database support.",
  },
  {
    date: "2026-06-24",
    title: "OCR memory usage fix",
    description:
      "PDF ingestion now processes pages in smaller windows at a lower default DPI and worker count, reducing peak memory use while keeping OCR practical for very large documents.",
  },
  {
    date: "2026-06-24",
    title: "Corpus snapshot restored",
    description:
      "The compressed corpus snapshot was regenerated from the current database, producing 3,787 documents and 66,042 pages after a crawl and ingest pass.",
  },
  {
    date: "2026-06-15",
    title: "REST API package linkage fix",
    description:
      "Build and helper scripts were adjusted so the REST API package links correctly against MySQL client headers and libraries.",
  },
  {
    date: "2026-06-13",
    title: "Reusable REST API package layout",
    description:
      "The REST API core was reorganized toward a shared-library package with public headers, CMake package metadata, and pkg-config support.",
  },
  {
    date: "2026-06-05",
    title: "Documentation refresh",
    description:
      "The root, frontend, and environment documentation were rewritten to better explain the project, local setup, and container command gateway.",
  },
  {
    date: "2026-05-24",
    title: "Site metadata and crawler guidance",
    description:
      "Site metadata, robots output, sitemap configuration, manifest data, and LLM-oriented API guidance were added for clearer discovery and automated access.",
  },
  {
    date: "2026-05-24",
    title: "Runtime REST API configuration",
    description:
      "The frontend gained a runtime configuration endpoint so deployments can supply the REST API origin without relying only on a baked browser bundle value.",
  },
  {
    date: "2026-05-24",
    title: "Document visualizer deployment",
    description:
      "The search page moved from raw JSON output toward a usable OCR document viewer with result lists, metadata, snippets, and page navigation.",
  },
  {
    date: "2026-05-24",
    title: "Footer and legal content cleanup",
    description:
      "Dead frontend links were removed, and the terms, privacy policy, and legal notice were rewritten around the current EPS document-search purpose.",
  },
  {
    date: "2026-05-22",
    title: "Container command gateway",
    description:
      "A container-oriented command bridge and whitelist were added so an LLM assistant can perform approved project operations through controlled host-side commands.",
  },
  {
    date: "2026-05-20",
    title: "REST API beta deployment setup",
    description:
      "The REST API received beta deployment configuration, environment examples, nginx reverse-proxy configuration, install script updates, and CORS-related startup options.",
  },
  {
    date: "2026-05-18",
    title: "MySQL credential handling",
    description:
      "Database credentials moved out of helper scripts and into environment variables, keeping local defaults documented while making production configuration safer.",
  },
  {
    date: "2026-05-18",
    title: "Database configuration naming cleanup",
    description:
      "REST API database configuration fields were renamed and rewired so host, database, port, and password settings match their actual runtime meaning.",
  },
  {
    date: "2026-05-18",
    title: "SQLite corpus connected to the REST API",
    description:
      "A MySQL schema and migration path were added so the scan manager corpus can be copied from SQLite into the database served by the C++ REST API.",
  },
  {
    date: "2026-05-18",
    title: "Crawler pipeline commands",
    description:
      "The scan manager wrapper gained migrate and pipeline commands, making crawl, ingest, and database refresh flows easier to run in order.",
  },
  {
    date: "2026-05-18",
    title: "DOJ crawler hardening",
    description:
      "The crawler gained stronger session handling, browser-state persistence, stealth support, challenge detection, and faster cookie-backed downloads.",
  },
  {
    date: "2026-05-18",
    title: "Frontend build cleanup",
    description:
      "Obsolete frontend pages, unused auth pieces, and orphaned components were removed so production builds could complete reliably.",
  },
  {
    date: "2026-05-16",
    title: "Frontend deployment cleanup",
    description:
      "The frontend was trimmed for deployment by removing unused API routes, stale user/order/image components, and legacy generated output.",
  },
  {
    date: "2026-05-10",
    title: "Beta build controls",
    description:
      "CMake flags, helper commands, and install-script updates were added to support beta builds of the REST API.",
  },
  {
    date: "2026-05-01",
    title: "CORS and configuration loader",
    description:
      "The REST API gained CORS handling plus a YAML-like configuration loader for server and MySQL settings.",
  },
  {
    date: "2026-04-08",
    title: "Next.js frontend foundation",
    description:
      "The initial TypeScript frontend was added with shared layout components, service pages, theming support, and a first search page shell.",
  },
  {
    date: "2026-04-08",
    title: "Court document REST routes",
    description:
      "The REST API gained the first court-document repository and route implementation for document search and retrieval.",
  },
  {
    date: "2026-03-30",
    title: "Multi-source corpus pipeline",
    description:
      "The scan manager expanded into a multi-source crawler and OCR ingest pipeline with SQLite FTS5 search, covering DOJ, CourtListener, and DocumentCloud sources.",
  },
  {
    date: "2026-03-30",
    title: "Initial compressed corpus snapshot",
    description:
      "A compressed SQLite database snapshot was added so the document corpus could be shared and restored without publishing the raw database file.",
  },
  {
    date: "2026-03-15",
    title: "DOJ disclosure crawler",
    description:
      "The crawler was rewritten to walk the structured DOJ Epstein disclosure index directly, collecting document links from disclosure sections instead of relying on blocked search pages.",
  },
  {
    date: "2026-03-15",
    title: "Crawler query and token configuration",
    description:
      "Crawler query generation, YAML token templates, and scan-manager guidance were expanded to support OCR extraction and document parsing experiments.",
  },
  {
    date: "2026-03-08",
    title: "Matrix setup script groundwork",
    description:
      "Early infrastructure work added a Matrix setup script and nginx reverse-proxy setup notes that informed later deployment tooling.",
  },
  {
    date: "2026-02-15",
    title: "Initial EPS application foundation",
    description:
      "The first substantial project structure landed: REST API scaffolding, scan manager utilities, database repository examples, helper scripts, and early documentation.",
  },
];

const Changelog = () => {
  return (
    <>
      <Head>
        <title>EPS - Changelog</title>
        <meta
          name="description"
          content="Recent changes and project updates for Epstein Paper System."
        />
        <link rel="icon" href="/eps-favicon.svg" type="image/svg+xml" />
      </Head>

      <main className="min-h-screen bg-[#f8f5f0] px-4 py-12 text-slate-900 dark:bg-slate-950 dark:text-slate-100">
        <div className="mx-auto max-w-4xl">
          <Link
            href="/"
            className="text-sm font-semibold text-slate-600 underline underline-offset-4 transition hover:text-slate-950 dark:text-slate-300 dark:hover:text-white"
          >
            Back to EPS
          </Link>

          <header className="mt-8 border-b border-slate-300 pb-8 dark:border-slate-700">
            <p className="text-sm font-bold uppercase tracking-wider text-slate-600 dark:text-slate-400">
              Project updates
            </p>
            <h1 className="mt-3 text-4xl font-bold tracking-tight text-slate-950 dark:text-white">
              Changelog
            </h1>
            <p className="mt-4 max-w-2xl text-base leading-7 text-slate-600 dark:text-slate-300">
              Recent work on EPS, listed from newest to oldest.
            </p>
          </header>

          <ol className="mt-8 space-y-4">
            {entries.map((entry) => (
              <li
                key={`${entry.date}-${entry.title}`}
                className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm dark:border-slate-800 dark:bg-slate-900"
              >
                <time className="text-sm font-semibold text-slate-500 dark:text-slate-400">
                  {entry.date}
                </time>
                <h2 className="mt-2 text-xl font-semibold text-slate-950 dark:text-white">
                  {entry.title}
                </h2>
                <p className="mt-2 text-sm leading-6 text-slate-600 dark:text-slate-300">
                  {entry.description}
                </p>
              </li>
            ))}
          </ol>
        </div>
      </main>

      <Footer />
      <PopupWidget />
    </>
  );
};

export default Changelog;
