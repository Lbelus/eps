import React, { useEffect, useMemo, useState } from "react";

type Status =
  | { type: "idle"; message: string }
  | { type: "loading"; message: string }
  | { type: "success"; message: string }
  | { type: "error"; message: string };

type CourtDocument = {
  document_id: number;
  filename: string;
  source: string;
  page_count: number;
  full_text?: string;
  created_at: string;
};

type SearchHit = Omit<CourtDocument, "full_text"> & {
  score: number;
  snippet: string;
};

type CourtPage = {
  page_id: number;
  document_id: number;
  page_number: number;
  page_text: string;
};

type ResultItem = CourtDocument & {
  score?: number;
  snippet?: string;
};

type ResultMode = "all" | "search" | "filename";
type PaginationDirection = "next" | "previous";
type SourceFilter = "doj" | "cl" | "dc";
type DetailTab = "pages" | "fullText" | "metadata";

type RuntimeConfig = {
  restApiUrl?: string;
};

const DEFAULT_SEARCH_ENGINE_API_URL = process.env.NEXT_PUBLIC_REST_API_URL?.replace(/[/]$/, "") || "";
const MAX_DOCUMENT_LIMIT = 100;

const SOURCE_LABELS: Record<string, string> = {
  cl: "CourtListener",
  dc: "DocumentCloud",
  doj: "DOJ",
};

const SOURCE_FILTER_OPTIONS: Array<{ value: SourceFilter; label: string }> = [
  { value: "doj", label: "DOJ" },
  { value: "cl", label: "CourtListener" },
  { value: "dc", label: "DocumentCloud" },
];

const escapeRegExp = (value: string) => value.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");

const buildQueryString = (params: Record<string, string | number>) => {
  const searchParams = new URLSearchParams();

  Object.entries(params).forEach(([key, value]) => {
    const normalized = String(value).trim();
    if (normalized !== "") {
      searchParams.append(key, normalized);
    }
  });

  const query = searchParams.toString();
  return query ? `?${query}` : "";
};

const normalizeEndpoint = (endpoint: string) => endpoint.trim().replace(/\/$/, "");

const getBrowserReachableApiUrl = (endpoint: string) => {
  const normalized = normalizeEndpoint(endpoint);

  if (!normalized || typeof window === "undefined") {
    return normalized;
  }

  try {
    const url = new URL(normalized);
    const pageHostname = window.location.hostname;
    const isLocalApiHost = url.hostname === "localhost" || url.hostname === "127.0.0.1" || url.hostname === "::1";
    const isLocalPageHost = pageHostname === "localhost" || pageHostname === "127.0.0.1" || pageHostname === "::1";

    if (isLocalApiHost && !isLocalPageHost) {
      url.hostname = pageHostname;
      return normalizeEndpoint(url.toString());
    }
  } catch (_error) {
    return normalized;
  }

  return normalized;
};

const parsePositiveInt = (value: string, fallback: number, max?: number) => {
  const parsed = Number.parseInt(value, 10);
  const result = Number.isFinite(parsed) && parsed > 0 ? parsed : fallback;
  return typeof max === "number" ? Math.min(result, max) : result;
};

const parseNonNegativeInt = (value: string, fallback: number) => {
  const parsed = Number.parseInt(value, 10);
  return Number.isFinite(parsed) && parsed >= 0 ? parsed : fallback;
};

const getQueryTerms = (query: string) => {
  const terms = query
    .toLowerCase()
    .split(/[^a-z0-9]+/i)
    .map((term) => term.trim())
    .filter((term) => term.length >= 2);

  return Array.from(new Set(terms));
};

const formatDate = (value: string) => {
  const date = new Date(value);
  if (Number.isNaN(date.getTime())) {
    return value || "Unknown";
  }
  return date.toLocaleDateString(undefined, {
    year: "numeric",
    month: "short",
    day: "numeric",
  });
};

const sourceLabel = (source: string) => SOURCE_LABELS[source] || source.toUpperCase();
const csvField = (value: string | number | null | undefined) =>
  "\"" + String(value ?? "").replace(/"/g, "\"\"") + "\"";

const buildCsv = (rows: ResultItem[]) => {
  const headers = ["document_id", "filename", "source", "source_label", "page_count", "created_at", "score"];
  const body = rows.map((item) =>
    [
      item.document_id,
      item.filename,
      item.source,
      sourceLabel(item.source),
      item.page_count,
      item.created_at,
      typeof item.score === "number" ? item.score : "",
    ]
      .map(csvField)
      .join(",")
  );

  return [headers.map(csvField).join(","), ...body].join("\n");
};

const timestampForFilename = (date: Date) =>
  [
    date.getFullYear(),
    String(date.getMonth() + 1).padStart(2, "0"),
    String(date.getDate()).padStart(2, "0"),
    String(date.getHours()).padStart(2, "0"),
    String(date.getMinutes()).padStart(2, "0"),
    String(date.getSeconds()).padStart(2, "0"),
  ].join("-");


const renderApiSnippet = (snippet: string) => {
  if (!snippet) {
    return "No snippet available.";
  }

  const parts = snippet.split(/(>>>|<<<)/g);
  let highlighted = false;

  return parts.map((part, index) => {
    if (part === ">>>") {
      highlighted = true;
      return null;
    }
    if (part === "<<<") {
      highlighted = false;
      return null;
    }
    if (!part) {
      return null;
    }

    return highlighted ? (
      <mark key={index} className="rounded bg-amber-200 px-0.5 text-slate-950">
        {part}
      </mark>
    ) : (
      <React.Fragment key={index}>{part}</React.Fragment>
    );
  });
};

const renderHighlightedText = (text: string, terms: string[]) => {
  if (!text) {
    return "No text available.";
  }
  if (terms.length === 0) {
    return text;
  }

  const pattern = new RegExp(`(${terms.map(escapeRegExp).join("|")})`, "gi");
  return text.split(pattern).map((part, index) => {
    const isMatch = terms.some((term) => part.toLowerCase() === term.toLowerCase());
    return isMatch ? (
      <mark key={index} className="rounded bg-amber-200 px-0.5 text-slate-950">
        {part}
      </mark>
    ) : (
      <React.Fragment key={index}>{part}</React.Fragment>
    );
  });
};

const SearchEngineQueryClient: React.FC = () => {
  const [query, setQuery] = useState("");
  const [limit, setLimit] = useState("20");
  const [offset, setOffset] = useState("0");
  const [mode, setMode] = useState<ResultMode>("all");
  const [activePageLimit, setActivePageLimit] = useState(20);
  const [canNavigatePrevious, setCanNavigatePrevious] = useState(false);
  const [canNavigateNext, setCanNavigateNext] = useState(false);
  const [sourceFilters, setSourceFilters] = useState<SourceFilter[]>([]);
  const [minPages, setMinPages] = useState("0");
  const [maxPages, setMaxPages] = useState("");
  const [results, setResults] = useState<ResultItem[]>([]);
  const [selectedDocument, setSelectedDocument] = useState<CourtDocument | null>(null);
  const [pages, setPages] = useState<CourtPage[]>([]);
  const [activePageNumber, setActivePageNumber] = useState(1);
  const [pageJump, setPageJump] = useState("1");
  const [activeTab, setActiveTab] = useState<DetailTab>("pages");
  const [status, setStatus] = useState<Status>({
    type: "idle",
    message: DEFAULT_SEARCH_ENGINE_API_URL ? "Loading recent documents." : "Loading API configuration.",
  });
  const [detailStatus, setDetailStatus] = useState<Status>({ type: "idle", message: "Select a document to read." });
  const [apiBaseUrl, setApiBaseUrl] = useState(() => getBrowserReachableApiUrl(DEFAULT_SEARCH_ENGINE_API_URL));
  const [configLoaded, setConfigLoaded] = useState(() => Boolean(getBrowserReachableApiUrl(DEFAULT_SEARCH_ENGINE_API_URL)));

  const queryTerms = useMemo(() => getQueryTerms(query), [query]);
  const activePage = useMemo(
    () => pages.find((page) => page.page_number === activePageNumber) || pages[0],
    [activePageNumber, pages]
  );
  const cleanedEndpoint = useMemo(() => normalizeEndpoint(apiBaseUrl), [apiBaseUrl]);
  const resultLimit = useMemo(() => parsePositiveInt(limit, 20, MAX_DOCUMENT_LIMIT), [limit]);
  const resultOffset = useMemo(() => parseNonNegativeInt(offset, 0), [offset]);
  const minPageFilter = useMemo(() => parseNonNegativeInt(minPages, 0), [minPages]);
  const maxPageFilter = useMemo(() => {
    const trimmed = maxPages.trim();
    return trimmed === "" ? "" : parseNonNegativeInt(trimmed, 0);
  }, [maxPages]);
  const sourceParam = useMemo(() => {
    if (sourceFilters.length === 0 || sourceFilters.length === SOURCE_FILTER_OPTIONS.length) {
      return "";
    }

    return SOURCE_FILTER_OPTIONS
      .map((option) => option.value)
      .filter((source) => sourceFilters.includes(source))
      .join(",");
  }, [sourceFilters]);

  const requestJson = async <T,>(path: string): Promise<T> => {
    if (!cleanedEndpoint) {
      throw new Error("Missing NEXT_PUBLIC_REST_API_URL.");
    }

    const response = await fetch(`${cleanedEndpoint}${path}`);
    const body = await response.text();

    if (!response.ok) {
      throw new Error(body || `Request failed with status ${response.status}.`);
    }

    return JSON.parse(body) as T;
  };

  const loadDocument = async (documentId: number) => {
    setDetailStatus({ type: "loading", message: "Loading document." });
    setSelectedDocument(null);
    setPages([]);
    setActiveTab("pages");

    try {
      const [document, documentPages] = await Promise.all([
        requestJson<CourtDocument>(`/courtdocuments/${documentId}`),
        requestJson<CourtPage[]>(`/courtdocuments/${documentId}/pages`),
      ]);

      setSelectedDocument(document);
      setPages(documentPages);
      const firstPage = documentPages[0]?.page_number || 1;
      setActivePageNumber(firstPage);
      setPageJump(String(firstPage));
      setDetailStatus({ type: "success", message: "Document loaded." });
    } catch (error) {
      const message = error instanceof Error ? error.message : "Unable to load document.";
      setDetailStatus({ type: "error", message });
    }
  };

  const resetCursorNavigation = () => {
    setCanNavigatePrevious(false);
    setCanNavigateNext(false);
  };

  const loadResults = async (
    nextMode: ResultMode,
    requestedOffset: number,
    direction?: PaginationDirection,
    cursor?: ResultItem
  ) => {
    const normalizedQuery = query.trim();
    const pageMaxParam = maxPageFilter === "" ? "" : maxPageFilter;
    const offsetCap = nextMode === "all" ? 1000000 : 10000;
    const nextOffset = direction
      ? Math.max(0, requestedOffset)
      : Math.min(Math.max(0, requestedOffset), offsetCap);
    const requestLimit = Math.min(resultLimit, nextMode === "all" ? 100 : 50);

    if (typeof maxPageFilter === "number" && maxPageFilter !== 0 && minPageFilter > maxPageFilter) {
      setStatus({ type: "error", message: "Minimum pages cannot be greater than maximum pages." });
      return;
    }
    if ((nextMode === "search" || nextMode === "filename") && !normalizedQuery) {
      setStatus({
        type: "error",
        message: nextMode === "filename" ? "Enter a filename first." : "Enter a search term first.",
      });
      return;
    }
    if (direction && !cursor) {
      setStatus({ type: "error", message: "Unable to continue from the current result page." });
      return;
    }
    if (direction && nextMode === "search" && typeof cursor?.score !== "number") {
      setStatus({ type: "error", message: "The current search result has no relevance cursor." });
      return;
    }

    const paginationParams: Record<string, string | number> = direction && cursor
      ? { direction, cursor_id: cursor.document_id }
      : { offset: nextOffset };
    if (direction && cursor && nextMode === "search") {
      paginationParams.cursor_score = cursor.score as number;
    } else if (direction && cursor && nextMode === "filename") {
      paginationParams.cursor_filename = cursor.filename;
    }

    setMode(nextMode);
    setStatus({
      type: "loading",
      message: nextMode === "search" ? "Searching document text." : nextMode === "filename" ? "Searching filenames." : "Loading documents.",
    });

    try {
      const commonParams = {
        source: sourceParam,
        page_min: minPageFilter,
        page_max: pageMaxParam,
        limit: requestLimit,
        ...paginationParams,
      };
      const path =
        nextMode === "search"
          ? "/courtdocuments/search" + buildQueryString({ q: normalizedQuery, ...commonParams })
          : nextMode === "filename"
            ? "/courtdocuments/by-filename" + buildQueryString({ q: normalizedQuery, ...commonParams })
            : "/courtdocuments" + buildQueryString(commonParams);

      const data = await requestJson<Array<SearchHit | CourtDocument>>(path);
      const nextResults = data as ResultItem[];

      if (direction && nextResults.length === 0) {
        if (direction === "next") {
          setCanNavigateNext(false);
        } else {
          setCanNavigatePrevious(false);
        }
        setStatus({ type: "success", message: "No more documents in this direction." });
        return;
      }

      setOffset(String(nextOffset));
      setActivePageLimit(requestLimit);
      setCanNavigatePrevious(nextOffset > 0 && nextResults.length > 0);
      setCanNavigateNext(direction === "previous" ? nextResults.length > 0 : nextResults.length === requestLimit);
      setResults(nextResults);
      setStatus({
        type: "success",
        message: nextResults.length > 0 ? `${nextResults.length} document${nextResults.length === 1 ? "" : "s"} found.` : "No documents found.",
      });

      if (nextResults.length > 0) {
        await loadDocument(nextResults[0].document_id);
      } else {
        setSelectedDocument(null);
        setPages([]);
        setDetailStatus({ type: "idle", message: "No document selected." });
      }
    } catch (error) {
      const message = error instanceof Error ? error.message : "Unable to load documents.";
      setStatus({ type: "error", message });
    }
  };

  const handlePreviousResults = () => {
    const firstResult = results[0];
    if (!firstResult) {
      return;
    }
    const previousOffset = Math.max(0, resultOffset - activePageLimit);
    void loadResults(mode, previousOffset, "previous", firstResult);
  };

  const handleNextResults = () => {
    const lastResult = results[results.length - 1];
    if (!lastResult) {
      return;
    }
    void loadResults(mode, resultOffset + results.length, "next", lastResult);
  };
  const handleExportCsv = () => {
    if (results.length === 0 || typeof window === "undefined") {
      return;
    }

    const csv = buildCsv(results);
    const blob = new Blob([csv], { type: "text/csv;charset=utf-8" });
    const url = window.URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = `eps-document-results-${timestampForFilename(new Date())}.csv`;
    document.body.appendChild(link);
    link.click();
    link.remove();
    window.URL.revokeObjectURL(url);
  };


  const handlePageJump = () => {
    const requestedPage = parseNonNegativeInt(pageJump, activePageNumber);
    const exists = pages.some((page) => page.page_number === requestedPage);
    if (exists) {
      setActivePageNumber(requestedPage);
    }
  };

  const toggleSourceFilter = (source: SourceFilter) => {
    resetCursorNavigation();
    setSourceFilters((current) =>
      current.includes(source) ? current.filter((item) => item !== source) : [...current, source]
    );
  };

  useEffect(() => {
    if (DEFAULT_SEARCH_ENGINE_API_URL) {
      return;
    }

    let cancelled = false;

    const loadRuntimeConfig = async () => {
      setStatus({ type: "loading", message: "Loading API configuration." });

      try {
        const response = await fetch("/api/runtime-config", { cache: "no-store" });
        const data = (await response.json()) as RuntimeConfig;
        const nextApiBaseUrl = getBrowserReachableApiUrl(data.restApiUrl || "");

        if (!response.ok || !nextApiBaseUrl) {
          throw new Error("Missing NEXT_PUBLIC_REST_API_URL.");
        }

        if (!cancelled) {
          setApiBaseUrl(nextApiBaseUrl);
          setConfigLoaded(true);
        }
      } catch (error) {
        const message = error instanceof Error ? error.message : "Unable to load API configuration.";
        if (!cancelled) {
          setStatus({ type: "error", message });
        }
      }
    };

    void loadRuntimeConfig();

    return () => {
      cancelled = true;
    };
  }, []);

  useEffect(() => {
    if (configLoaded) {
      void loadResults("all", 0);
    }
    // Run when the API endpoint configuration is available.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [configLoaded]);

  return (
    <section className="space-y-5">
      <header className="border-b border-slate-200 pb-4 dark:border-slate-700">
        <h1 className="text-2xl font-semibold text-slate-900 dark:text-white">Document search</h1>
        <p className="mt-2 max-w-3xl text-sm text-slate-600 dark:text-slate-300">
          Search public court records, inspect matching snippets, and read OCR text by page.
        </p>
      </header>

      <div className="grid min-h-[720px] gap-5 xl:grid-cols-[360px,1fr]">
        <aside className="flex min-h-0 flex-col rounded-lg border border-slate-200 bg-white dark:border-slate-700 dark:bg-slate-900">
          <div className="border-b border-slate-200 p-4 dark:border-slate-700">
            <div className="space-y-3">
              <label className="block text-sm font-medium text-slate-700 dark:text-slate-200">
                Search terms
                <input
                  type="search"
                  value={query}
                  onChange={(event) => {
                    setQuery(event.target.value);
                    resetCursorNavigation();
                  }}
                  onKeyDown={(event) => {
                    if (event.key === "Enter") {
                      void loadResults("search", resultOffset);
                    }
                  }}
                  placeholder="Epstein, flight logs, subpoena..."
                  className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm text-slate-800 shadow-sm focus:border-slate-500 focus:outline-none dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100"
                />
              </label>

              <fieldset className="space-y-2">
                <legend className="text-sm font-medium text-slate-700 dark:text-slate-200">Sources</legend>
                <div className="grid grid-cols-2 gap-2">
                  <button
                    type="button"
                    aria-pressed={sourceFilters.length === 0}
                    onClick={() => {
                      setSourceFilters([]);
                      resetCursorNavigation();
                    }}
                    className={`rounded-md border px-3 py-2 text-left text-sm font-medium transition ${
                      sourceFilters.length === 0
                        ? "border-slate-900 bg-slate-900 text-white dark:border-slate-100 dark:bg-slate-100 dark:text-slate-950"
                        : "border-slate-300 text-slate-700 hover:border-slate-500 dark:border-slate-700 dark:text-slate-200"
                    }`}
                  >
                    All sources
                  </button>
                  {SOURCE_FILTER_OPTIONS.map((option) => {
                    const selected = sourceFilters.includes(option.value);
                    return (
                      <button
                        key={option.value}
                        type="button"
                        aria-pressed={selected}
                        onClick={() => toggleSourceFilter(option.value)}
                        className={`rounded-md border px-3 py-2 text-left text-sm font-medium transition ${
                          selected
                            ? "border-slate-900 bg-slate-900 text-white dark:border-slate-100 dark:bg-slate-100 dark:text-slate-950"
                            : "border-slate-300 text-slate-700 hover:border-slate-500 dark:border-slate-700 dark:text-slate-200"
                        }`}
                      >
                        {option.label}
                      </button>
                    );
                  })}
                </div>
              </fieldset>

              <fieldset className="space-y-2">
                <legend className="text-sm font-medium text-slate-700 dark:text-slate-200">Page count</legend>
                <div className="grid grid-cols-2 gap-3">
                  <label className="block text-sm font-medium text-slate-700 dark:text-slate-200">
                    Min
                    <input
                      type="number"
                      min={0}
                      value={minPages}
                      onChange={(event) => {
                        setMinPages(event.target.value);
                        resetCursorNavigation();
                      }}
                      className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm text-slate-800 shadow-sm focus:border-slate-500 focus:outline-none dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100"
                    />
                  </label>
                  <label className="block text-sm font-medium text-slate-700 dark:text-slate-200">
                    Max
                    <input
                      type="number"
                      min={0}
                      value={maxPages}
                      onChange={(event) => {
                        setMaxPages(event.target.value);
                        resetCursorNavigation();
                      }}
                      placeholder="All"
                      className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm text-slate-800 shadow-sm focus:border-slate-500 focus:outline-none dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100"
                    />
                  </label>
                </div>
              </fieldset>

              <div className="grid grid-cols-2 gap-3">
                <label className="block text-sm font-medium text-slate-700 dark:text-slate-200">
                  Limit
                  <input
                    type="number"
                    min={1}
                    max={MAX_DOCUMENT_LIMIT}
                    value={limit}
                    onChange={(event) => {
                      setLimit(event.target.value);
                      resetCursorNavigation();
                    }}
                    className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm text-slate-800 shadow-sm focus:border-slate-500 focus:outline-none dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100"
                  />
                </label>
                <label className="block text-sm font-medium text-slate-700 dark:text-slate-200">
                  Offset
                  <input
                    type="number"
                    min={0}
                    max={mode === "all" ? 1000000 : 10000}
                    value={offset}
                    onChange={(event) => {
                      setOffset(event.target.value);
                      resetCursorNavigation();
                    }}
                    className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm text-slate-800 shadow-sm focus:border-slate-500 focus:outline-none dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100"
                  />
                </label>
              </div>

              <div className="flex flex-wrap gap-2">
                <button
                  type="button"
                  onClick={() => void loadResults("search", resultOffset)}
                  className="rounded-md bg-slate-900 px-4 py-2 text-sm font-semibold text-white transition hover:bg-slate-700 dark:bg-slate-100 dark:text-slate-950 dark:hover:bg-white"
                >
                  Search
                </button>
                <button
                  type="button"
                  onClick={() => void loadResults("filename", resultOffset)}
                  className="rounded-md border border-slate-300 px-4 py-2 text-sm font-semibold text-slate-700 transition hover:border-slate-500 dark:border-slate-700 dark:text-slate-200"
                >
                  Filename
                </button>
                <button
                  type="button"
                  onClick={() => void loadResults("all", resultOffset)}
                  className="rounded-md border border-slate-300 px-4 py-2 text-sm font-semibold text-slate-700 transition hover:border-slate-500 dark:border-slate-700 dark:text-slate-200"
                >
                  All documents
                </button>
              </div>

              <p
                className={`text-sm ${
                  status.type === "error"
                    ? "text-red-600"
                    : status.type === "success"
                      ? "text-emerald-700 dark:text-emerald-400"
                      : "text-slate-500 dark:text-slate-300"
                }`}
              >
                {status.message}
              </p>
            </div>
          </div>

          <div className="flex flex-wrap items-center justify-between gap-2 border-b border-slate-200 px-4 py-3 text-sm dark:border-slate-700">
            <button
              type="button"
              onClick={handlePreviousResults}
              disabled={!canNavigatePrevious || status.type === "loading"}
              className="rounded-md border border-slate-300 px-3 py-1.5 font-semibold text-slate-700 disabled:cursor-not-allowed disabled:opacity-40 dark:border-slate-700 dark:text-slate-200"
            >
              Previous
            </button>
            <span className="text-slate-500 dark:text-slate-300">Offset {resultOffset}</span>
            <button
              type="button"
              onClick={handleExportCsv}
              disabled={results.length === 0}
              className="rounded-md border border-slate-300 px-3 py-1.5 font-semibold text-slate-700 disabled:cursor-not-allowed disabled:opacity-40 dark:border-slate-700 dark:text-slate-200"
            >
              Export CSV
            </button>
            <button
              type="button"
              onClick={handleNextResults}
              disabled={!canNavigateNext || status.type === "loading"}
              className="rounded-md border border-slate-300 px-3 py-1.5 font-semibold text-slate-700 disabled:cursor-not-allowed disabled:opacity-40 dark:border-slate-700 dark:text-slate-200"
            >
              Next
            </button>
          </div>

          <div className="min-h-0 flex-1 overflow-auto">
            {results.length === 0 ? (
              <div className="p-4 text-sm text-slate-500 dark:text-slate-300">No results to display.</div>
            ) : (
              <div className="divide-y divide-slate-200 dark:divide-slate-700">
                {results.map((item) => {
                  const selected = selectedDocument?.document_id === item.document_id;
                  return (
                    <button
                      key={item.document_id}
                      type="button"
                      onClick={() => void loadDocument(item.document_id)}
                      className={`block w-full px-4 py-4 text-left transition ${
                        selected ? "bg-slate-100 dark:bg-slate-800" : "hover:bg-slate-50 dark:hover:bg-slate-800/60"
                      }`}
                    >
                      <div className="flex items-start justify-between gap-3">
                        <h2 className="min-w-0 text-sm font-semibold leading-5 text-slate-900 dark:text-white">
                          {item.filename}
                        </h2>
                        <span className="shrink-0 rounded bg-slate-200 px-2 py-1 text-[11px] font-semibold uppercase text-slate-700 dark:bg-slate-700 dark:text-slate-100">
                          {item.source}
                        </span>
                      </div>
                      <div className="mt-2 flex flex-wrap gap-x-3 gap-y-1 text-xs text-slate-500 dark:text-slate-300">
                        <span>{sourceLabel(item.source)}</span>
                        <span>{item.page_count} pages</span>
                        <span>{formatDate(item.created_at)}</span>
                        {typeof item.score === "number" && <span>Score {item.score.toFixed(2)}</span>}
                      </div>
                      {item.snippet && (
                        <p className="mt-3 line-clamp-4 whitespace-pre-wrap text-xs leading-5 text-slate-600 dark:text-slate-300">
                          {renderApiSnippet(item.snippet)}
                        </p>
                      )}
                    </button>
                  );
                })}
              </div>
            )}
          </div>
        </aside>

        <article className="min-w-0 rounded-lg border border-slate-200 bg-white dark:border-slate-700 dark:bg-slate-900">
          {!selectedDocument ? (
            <div className="flex min-h-[520px] items-center justify-center p-8 text-center text-sm text-slate-500 dark:text-slate-300">
              {detailStatus.message}
            </div>
          ) : (
            <div className="flex h-full min-h-[720px] flex-col">
              <div className="border-b border-slate-200 p-5 dark:border-slate-700">
                <div className="flex flex-wrap items-start justify-between gap-4">
                  <div className="min-w-0">
                    <p className="text-xs font-semibold uppercase text-slate-500 dark:text-slate-400">
                      Document {selectedDocument.document_id}
                    </p>
                    <h2 className="mt-1 break-words text-xl font-semibold text-slate-900 dark:text-white">
                      {selectedDocument.filename}
                    </h2>
                    <div className="mt-3 flex flex-wrap gap-2 text-xs text-slate-600 dark:text-slate-300">
                      <span className="rounded bg-slate-200 px-2 py-1 font-semibold dark:bg-slate-700 dark:text-slate-100">
                        {sourceLabel(selectedDocument.source)}
                      </span>
                      <span className="rounded border border-slate-200 px-2 py-1 dark:border-slate-700">
                        {selectedDocument.page_count} pages
                      </span>
                      <span className="rounded border border-slate-200 px-2 py-1 dark:border-slate-700">
                        Added {formatDate(selectedDocument.created_at)}
                      </span>
                    </div>
                  </div>
                  <p
                    className={`text-sm ${
                      detailStatus.type === "error"
                        ? "text-red-600"
                        : detailStatus.type === "success"
                          ? "text-emerald-700 dark:text-emerald-400"
                          : "text-slate-500 dark:text-slate-300"
                    }`}
                  >
                    {detailStatus.message}
                  </p>
                </div>
              </div>

              <div className="flex flex-wrap gap-2 border-b border-slate-200 px-5 py-3 dark:border-slate-700">
                {([
                  ["pages", "Pages"],
                  ["fullText", "Full text"],
                  ["metadata", "Metadata"],
                ] as Array<[DetailTab, string]>).map(([tab, label]) => (
                  <button
                    key={tab}
                    type="button"
                    onClick={() => setActiveTab(tab)}
                    className={`rounded-md px-3 py-1.5 text-sm font-semibold ${
                      activeTab === tab
                        ? "bg-slate-900 text-white dark:bg-slate-100 dark:text-slate-950"
                        : "border border-slate-300 text-slate-700 dark:border-slate-700 dark:text-slate-200"
                    }`}
                  >
                    {label}
                  </button>
                ))}
              </div>

              <div className="min-h-0 flex-1 overflow-auto p-5">
                {activeTab === "pages" && (
                  <div className="grid gap-5 lg:grid-cols-[180px,1fr]">
                    <div className="space-y-3">
                      <div className="flex gap-2">
                        <input
                          type="number"
                          min={1}
                          max={selectedDocument.page_count}
                          value={pageJump}
                          onChange={(event) => setPageJump(event.target.value)}
                          className="w-full rounded-md border border-slate-300 px-3 py-2 text-sm text-slate-800 dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100"
                          aria-label="Jump to page"
                        />
                        <button
                          type="button"
                          onClick={handlePageJump}
                          className="rounded-md bg-slate-900 px-3 py-2 text-sm font-semibold text-white dark:bg-slate-100 dark:text-slate-950"
                        >
                          Go
                        </button>
                      </div>
                      <div className="grid max-h-[520px] grid-cols-3 gap-2 overflow-auto pr-1 lg:grid-cols-2">
                        {pages.map((page) => (
                          <button
                            key={page.page_id}
                            type="button"
                            onClick={() => {
                              setActivePageNumber(page.page_number);
                              setPageJump(String(page.page_number));
                            }}
                            className={`rounded-md border px-2 py-1.5 text-sm font-semibold ${
                              activePage?.page_number === page.page_number
                                ? "border-slate-900 bg-slate-900 text-white dark:border-slate-100 dark:bg-slate-100 dark:text-slate-950"
                                : "border-slate-300 text-slate-700 dark:border-slate-700 dark:text-slate-200"
                            }`}
                          >
                            {page.page_number}
                          </button>
                        ))}
                      </div>
                    </div>

                    <div className="min-w-0 rounded-md border border-slate-200 bg-slate-50 dark:border-slate-700 dark:bg-slate-950">
                      <div className="flex items-center justify-between border-b border-slate-200 px-4 py-3 dark:border-slate-700">
                        <h3 className="text-sm font-semibold text-slate-900 dark:text-white">
                          Page {activePage?.page_number || "-"}
                        </h3>
                        <div className="flex gap-2">
                          <button
                            type="button"
                            disabled={!activePage || activePage.page_number <= 1}
                            onClick={() => {
                              const previous = Math.max(1, activePageNumber - 1);
                              setActivePageNumber(previous);
                              setPageJump(String(previous));
                            }}
                            className="rounded-md border border-slate-300 px-3 py-1 text-xs font-semibold text-slate-700 disabled:opacity-40 dark:border-slate-700 dark:text-slate-200"
                          >
                            Previous
                          </button>
                          <button
                            type="button"
                            disabled={!activePage || activePage.page_number >= selectedDocument.page_count}
                            onClick={() => {
                              const next = Math.min(selectedDocument.page_count, activePageNumber + 1);
                              setActivePageNumber(next);
                              setPageJump(String(next));
                            }}
                            className="rounded-md border border-slate-300 px-3 py-1 text-xs font-semibold text-slate-700 disabled:opacity-40 dark:border-slate-700 dark:text-slate-200"
                          >
                            Next
                          </button>
                        </div>
                      </div>
                      <pre className="max-h-[600px] overflow-auto whitespace-pre-wrap p-4 text-sm leading-6 text-slate-800 dark:text-slate-100">
                        {renderHighlightedText(activePage?.page_text || "", queryTerms)}
                      </pre>
                    </div>
                  </div>
                )}

                {activeTab === "fullText" && (
                  <pre className="max-h-[660px] overflow-auto whitespace-pre-wrap rounded-md border border-slate-200 bg-slate-50 p-4 text-sm leading-6 text-slate-800 dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100">
                    {renderHighlightedText(selectedDocument.full_text || "", queryTerms)}
                  </pre>
                )}

                {activeTab === "metadata" && (
                  <dl className="grid gap-4 text-sm sm:grid-cols-2">
                    <div className="rounded-md border border-slate-200 p-4 dark:border-slate-700">
                      <dt className="font-semibold text-slate-500 dark:text-slate-400">Document ID</dt>
                      <dd className="mt-1 text-slate-900 dark:text-white">{selectedDocument.document_id}</dd>
                    </div>
                    <div className="rounded-md border border-slate-200 p-4 dark:border-slate-700">
                      <dt className="font-semibold text-slate-500 dark:text-slate-400">Source</dt>
                      <dd className="mt-1 text-slate-900 dark:text-white">{sourceLabel(selectedDocument.source)}</dd>
                    </div>
                    <div className="rounded-md border border-slate-200 p-4 dark:border-slate-700 sm:col-span-2">
                      <dt className="font-semibold text-slate-500 dark:text-slate-400">Filename</dt>
                      <dd className="mt-1 break-words text-slate-900 dark:text-white">{selectedDocument.filename}</dd>
                    </div>
                    <div className="rounded-md border border-slate-200 p-4 dark:border-slate-700">
                      <dt className="font-semibold text-slate-500 dark:text-slate-400">Pages</dt>
                      <dd className="mt-1 text-slate-900 dark:text-white">{selectedDocument.page_count}</dd>
                    </div>
                    <div className="rounded-md border border-slate-200 p-4 dark:border-slate-700">
                      <dt className="font-semibold text-slate-500 dark:text-slate-400">Created</dt>
                      <dd className="mt-1 text-slate-900 dark:text-white">{formatDate(selectedDocument.created_at)}</dd>
                    </div>
                  </dl>
                )}
              </div>
            </div>
          )}
        </article>
      </div>
    </section>
  );
};

export default SearchEngineQueryClient;
