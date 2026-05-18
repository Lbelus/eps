import React, { useMemo, useState } from "react";

type RouteKey = "list" | "byId" | "pages" | "search";

type Status =
  | { type: "idle"; message: string }
  | { type: "loading"; message: string }
  | { type: "success"; message: string }
  | { type: "error"; message: string };

const ROUTE_OPTIONS: { key: RouteKey; label: string; description: string }[] = [
  {
    key: "list",
    label: "List documents",
    description: "GET /courtdocuments?limit=&offset=",
  },
  {
    key: "byId",
    label: "Read a document",
    description: "GET /courtdocuments/<id>",
  },
  {
    key: "pages",
    label: "Read pages",
    description: "GET /courtdocuments/<id>/pages",
  },
  {
    key: "search",
    label: "Search",
    description: "GET /courtdocuments/search?q=...&limit=&offset=",
  },
];

const toQueryString = (params: Record<string, string>) => {
  const searchParams = new URLSearchParams();
  Object.entries(params).forEach(([key, value]) => {
    if (value.trim() !== "") {
      searchParams.append(key, value.trim());
    }
  });
  const query = searchParams.toString();
  return query ? `?${query}` : "";
};

const buildRequestPath = (
  route: RouteKey,
  values: { documentId: string; limit: string; offset: string; query: string }
) => {
  if (route === "list") {
    return `/courtdocuments${toQueryString({ limit: values.limit, offset: values.offset })}`;
  }

  if (route === "byId") {
    return `/courtdocuments/${values.documentId.trim()}`;
  }

  if (route === "pages") {
    return `/courtdocuments/${values.documentId.trim()}/pages`;
  }

  return `/courtdocuments/search${toQueryString({
    q: values.query,
    limit: values.limit,
    offset: values.offset,
  })}`;
};

const isFormValid = (
  route: RouteKey,
  values: { documentId: string; query: string }
) => {
  if (route === "byId" || route === "pages") {
    return values.documentId.trim() !== "";
  }

  if (route === "search") {
    return values.query.trim() !== "";
  }

  return true;
};

const DEFAULT_REST_API_URL =
  process.env.NEXT_PUBLIC_REST_API_URL?.replace(/\/$/, "") || "http://127.0.0.1:3004";

const SearchEngineQueryClient: React.FC = () => {
  const [endpoint, setEndpoint] = useState(DEFAULT_REST_API_URL);
  const [route, setRoute] = useState<RouteKey>("list");
  const [documentId, setDocumentId] = useState("");
  const [query, setQuery] = useState("");
  const [limit, setLimit] = useState("");
  const [offset, setOffset] = useState("");
  const [status, setStatus] = useState<Status>({ type: "idle", message: "" });
  const [responseCode, setResponseCode] = useState<number | null>(null);
  const [responseBody, setResponseBody] = useState("");

  const requestPath = useMemo(
    () => buildRequestPath(route, { documentId, limit, offset, query }),
    [route, documentId, limit, offset, query]
  );

  const commandPreview = useMemo(() => {
    const cleanedEndpoint = endpoint.replace(/\/$/, "");
    return `curl -X GET "${cleanedEndpoint}${requestPath}"`;
  }, [endpoint, requestPath]);

  const handleSend = async () => {
    if (!endpoint.trim()) {
      setStatus({ type: "error", message: "Please enter an API URL." });
      return;
    }

    if (!isFormValid(route, { documentId, query })) {
      setStatus({
        type: "error",
        message:
          route === "search"
            ? "Please enter a search term."
            : "Please enter a document identifier.",
      });
      return;
    }

    const cleanedEndpoint = endpoint.replace(/\/$/, "");
    const url = `${cleanedEndpoint}${requestPath}`;

    setStatus({ type: "loading", message: "Request in progress..." });
    setResponseCode(null);
    setResponseBody("");

    try {
      const response = await fetch(url, { method: "GET" });
      const text = await response.text();

      setResponseCode(response.status);

      const safeBody = text.trim() ? text : "(empty response)";
      try {
        const parsed = JSON.parse(text);
        setResponseBody(JSON.stringify(parsed, null, 2));
      } catch {
        setResponseBody(safeBody);
      }

      if (!response.ok) {
        setStatus({
          type: "error",
          message: `The service responded with status ${response.status}.`,
        });
        return;
      }

      setStatus({ type: "success", message: "Request completed successfully." });
    } catch (error) {
      const message = error instanceof Error ? error.message : "Unknown network error";
      setStatus({ type: "error", message: `Request failed: ${message}` });
    }
  };

  return (
    <section className="rounded-lg border border-slate-200 bg-white p-6 shadow-sm dark:border-slate-700 dark:bg-slate-900">
      <header className="mb-6">
        <h1 className="text-2xl font-semibold text-slate-900 dark:text-white">Search engine · REST requests</h1>
        <p className="mt-2 text-sm text-slate-600 dark:text-slate-300">
          Run supported GET calls against the court document search service.
        </p>
      </header>

      <div className="grid gap-6 lg:grid-cols-[2fr,1fr]">
        <div className="space-y-4">
          <label className="block text-sm font-medium text-slate-700 dark:text-slate-200">
            Service URL
            <input
              type="text"
              value={endpoint}
              onChange={(event) => setEndpoint(event.target.value)}
              placeholder={DEFAULT_REST_API_URL}
              className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm text-slate-800 shadow-sm focus:border-primary focus:outline-none dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100"
            />
          </label>

          <div className="grid gap-2 sm:grid-cols-2">
            {ROUTE_OPTIONS.map((option) => (
              <button
                key={option.key}
                type="button"
                onClick={() => setRoute(option.key)}
                className={`rounded-md border px-3 py-2 text-left text-sm transition ${
                  route === option.key
                    ? "border-indigo-500 bg-indigo-50 text-indigo-700 dark:border-indigo-400 dark:bg-indigo-950/60 dark:text-indigo-200"
                    : "border-slate-200 bg-white text-slate-700 hover:border-slate-300 dark:border-slate-700 dark:bg-slate-900 dark:text-slate-300"
                }`}
              >
                <p className="font-medium">{option.label}</p>
                <p className="mt-1 text-xs opacity-80">{option.description}</p>
              </button>
            ))}
          </div>

          <div className="grid gap-4 sm:grid-cols-2">
            {(route === "byId" || route === "pages") && (
              <label className="block text-sm font-medium text-slate-700 dark:text-slate-200 sm:col-span-2">
                Document ID
                <input
                  type="text"
                  value={documentId}
                  onChange={(event) => setDocumentId(event.target.value)}
                  placeholder="123"
                  className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm text-slate-800 shadow-sm focus:border-primary focus:outline-none dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100"
                />
              </label>
            )}

            {route === "search" && (
              <label className="block text-sm font-medium text-slate-700 dark:text-slate-200 sm:col-span-2">
                Query q
                <input
                  type="text"
                  value={query}
                  onChange={(event) => setQuery(event.target.value)}
                  placeholder="keyword"
                  className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm text-slate-800 shadow-sm focus:border-primary focus:outline-none dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100"
                />
              </label>
            )}

            {(route === "list" || route === "search") && (
              <>
                <label className="block text-sm font-medium text-slate-700 dark:text-slate-200">
                  limit
                  <input
                    type="number"
                    min={0}
                    value={limit}
                    onChange={(event) => setLimit(event.target.value)}
                    placeholder="50"
                    className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm text-slate-800 shadow-sm focus:border-primary focus:outline-none dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100"
                  />
                </label>
                <label className="block text-sm font-medium text-slate-700 dark:text-slate-200">
                  offset
                  <input
                    type="number"
                    min={0}
                    value={offset}
                    onChange={(event) => setOffset(event.target.value)}
                    placeholder="0"
                    className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm text-slate-800 shadow-sm focus:border-primary focus:outline-none dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100"
                  />
                </label>
              </>
            )}
          </div>

          <button
            type="button"
            onClick={handleSend}
            className="rounded-md bg-indigo-600 px-4 py-2 text-sm font-semibold text-white shadow-sm transition hover:bg-indigo-500"
          >
            Run request
          </button>

          <div className="rounded-md border border-slate-200 bg-slate-50 p-3 dark:border-slate-700 dark:bg-slate-950">
            <p className="text-xs font-medium text-slate-500">cURL preview</p>
            <code className="mt-1 block overflow-auto text-xs text-slate-700 dark:text-slate-300">{commandPreview}</code>
          </div>
        </div>

        <aside className="space-y-4 rounded-md border border-slate-200 bg-slate-50 p-4 dark:border-slate-700 dark:bg-slate-950">
          <h2 className="text-sm font-semibold text-slate-800 dark:text-slate-100">Result</h2>
          <p
            className={`text-sm ${
              status.type === "error"
                ? "text-red-600"
                : status.type === "success"
                  ? "text-emerald-600"
                  : "text-slate-600 dark:text-slate-300"
            }`}
          >
            {status.message || "No request has been sent yet."}
          </p>

          <div className="rounded-md border border-slate-200 bg-white p-3 text-xs text-slate-700 dark:border-slate-700 dark:bg-slate-900 dark:text-slate-200">
            <p className="font-medium">HTTP status</p>
            <p className="mt-1">{responseCode ?? "-"}</p>
          </div>

          <div className="rounded-md border border-slate-200 bg-white p-3 dark:border-slate-700 dark:bg-slate-900">
            <p className="text-xs font-medium text-slate-500">Response body</p>
            <pre className="mt-2 max-h-[380px] overflow-auto whitespace-pre-wrap text-xs text-slate-700 dark:text-slate-200">
              {responseBody || "(empty)"}
            </pre>
          </div>
        </aside>
      </div>
    </section>
  );
};

export default SearchEngineQueryClient;
