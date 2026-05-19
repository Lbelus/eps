import React, { useMemo, useState } from "react";

type Status =
  | { type: "idle"; message: string }
  | { type: "loading"; message: string }
  | { type: "success"; message: string }
  | { type: "error"; message: string };

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

const DEFAULT_SEARCH_ENGINE_API_URL =
  process.env.NEXT_PUBLIC_SEARCH_ENGINE_API_URL?.replace(/\/$/, "") ||
  process.env.NEXT_PUBLIC_REST_API_URL?.replace(/\/$/, "") ||
  "http://127.0.0.1:3004";

const SearchEngineQueryClient: React.FC = () => {
  const [endpoint, setEndpoint] = useState(DEFAULT_SEARCH_ENGINE_API_URL);
  const [query, setQuery] = useState("");
  const [limit, setLimit] = useState("");
  const [offset, setOffset] = useState("");
  const [status, setStatus] = useState<Status>({ type: "idle", message: "" });
  const [responseCode, setResponseCode] = useState<number | null>(null);
  const [responseBody, setResponseBody] = useState("");

  const requestPath = useMemo(
    () =>
      `/courtdocuments/search${toQueryString({
        q: query,
        limit,
        offset,
      })}`,
    [query, limit, offset]
  );

  const commandPreview = useMemo(() => {
    const cleanedEndpoint = endpoint.replace(/\/$/, "");
    return `curl -X GET "${cleanedEndpoint}${requestPath}"`;
  }, [endpoint, requestPath]);

  const handleSend = async () => {
    if (!endpoint.trim()) {
      setStatus({
        type: "error",
        message:
          "Missing API endpoint. Set NEXT_PUBLIC_SEARCH_ENGINE_API_URL or NEXT_PUBLIC_REST_API_URL in .env.",
      });
      return;
    }

    if (!query.trim()) {
      setStatus({ type: "error", message: "Please enter a search term." });
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
      const message =
        error instanceof Error ? error.message : "Unknown network error";

      setStatus({ type: "error", message: `Request failed: ${message}` });
    }
  };

  return (
    <section className="rounded-lg border border-slate-200 bg-white p-6 shadow-sm dark:border-slate-700 dark:bg-slate-900">
      <header className="mb-6">
        <h1 className="text-2xl font-semibold text-slate-900 dark:text-white">
          Search engine
        </h1>
        <p className="mt-2 text-sm text-slate-600 dark:text-slate-300">
          Search court documents.
        </p>
      </header>

      <div className="grid gap-6 lg:grid-cols-[2fr,1fr]">
        <div className="space-y-4">
          <label className="block text-sm font-medium text-slate-700 dark:text-slate-200 sm:col-span-2">
            API endpoint
            <input
              type="text"
              value={endpoint}
              onChange={(event) => setEndpoint(event.target.value)}
              placeholder={DEFAULT_SEARCH_ENGINE_API_URL}
              className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm text-slate-800 shadow-sm focus:border-primary focus:outline-none dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100"
            />
          </label>

          <label className="block text-sm font-medium text-slate-700 dark:text-slate-200 sm:col-span-2">
            Search term
            <input
              type="text"
              value={query}
              onChange={(event) => setQuery(event.target.value)}
              placeholder="keyword"
              className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm text-slate-800 shadow-sm focus:border-primary focus:outline-none dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100"
            />
          </label>

          <div className="grid gap-4 sm:grid-cols-2">
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
          </div>

          <button
            type="button"
            onClick={handleSend}
            className="rounded-md bg-indigo-600 px-4 py-2 text-sm font-semibold text-white shadow-sm transition hover:bg-indigo-500"
          >
            Search
          </button>

          <div className="rounded-md border border-slate-200 bg-slate-50 p-3 dark:border-slate-700 dark:bg-slate-950">
            <p className="text-xs font-medium text-slate-500">cURL preview</p>
            <code className="mt-1 block overflow-auto text-xs text-slate-700 dark:text-slate-300">
              {commandPreview}
            </code>
          </div>
        </div>

        <aside className="space-y-4 rounded-md border border-slate-200 bg-slate-50 p-4 dark:border-slate-700 dark:bg-slate-950">
          <h2 className="text-sm font-semibold text-slate-800 dark:text-slate-100">
            Result
          </h2>

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
