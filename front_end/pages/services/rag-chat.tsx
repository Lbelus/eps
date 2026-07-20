import Head from "next/head";
import Link from "next/link";
import React, { useEffect, useRef, useState } from "react";

type ToolCall = {
  name: string;
  input: Record<string, unknown>;
};

type ChatMessage = {
  role: "user" | "assistant";
  text: string;
  tools: ToolCall[];
};

type StreamEvent =
  | { type: "session"; session_id: string }
  | { type: "text"; text: string }
  | { type: "tool"; name: string; input: Record<string, unknown> }
  | { type: "done" }
  | { type: "error"; message: string };

const RAG_API_URL = (process.env.NEXT_PUBLIC_RAG_API_URL || "http://localhost:8000").replace(/\/$/, "");

const TOOL_LABELS: Record<string, string> = {
  search_documents: "Searching documents",
  find_pages: "Locating pages",
  read_pages: "Reading pages",
};

const describeTool = (tool: ToolCall) => {
  const label = TOOL_LABELS[tool.name] || tool.name;
  const detail =
    typeof tool.input.query === "string"
      ? tool.input.query
      : typeof tool.input.filename === "string"
        ? `${tool.input.filename}${Array.isArray(tool.input.pages) ? ` p. ${tool.input.pages.join(", ")}` : ""}${
            typeof tool.input.term === "string" ? ` · "${tool.input.term}"` : ""
          }`
        : "";
  return detail ? `${label}: ${detail}` : label;
};

export default function RagChatServicePage() {
  const [messages, setMessages] = useState<ChatMessage[]>([]);
  const [input, setInput] = useState("");
  const [sessionId, setSessionId] = useState<string | null>(null);
  const [streaming, setStreaming] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const scrollRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    scrollRef.current?.scrollTo({ top: scrollRef.current.scrollHeight });
  }, [messages]);

  const applyEvent = (event: StreamEvent) => {
    if (event.type === "session") {
      setSessionId(event.session_id);
      return;
    }
    if (event.type === "error") {
      setError(event.message);
      return;
    }
    if (event.type === "text" || event.type === "tool") {
      setMessages((previous) => {
        const next = [...previous];
        const last = { ...next[next.length - 1] };
        if (event.type === "text") {
          last.text += event.text;
        } else {
          last.tools = [...last.tools, { name: event.name, input: event.input }];
        }
        next[next.length - 1] = last;
        return next;
      });
    }
  };

  const sendMessage = async () => {
    const question = input.trim();
    if (!question || streaming) {
      return;
    }

    setError(null);
    setInput("");
    setStreaming(true);
    setMessages((previous) => [
      ...previous,
      { role: "user", text: question, tools: [] },
      { role: "assistant", text: "", tools: [] },
    ]);

    try {
      const response = await fetch(`${RAG_API_URL}/rag/chat`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ message: question, session_id: sessionId }),
      });

      if (!response.ok || !response.body) {
        throw new Error((await response.text()) || `Request failed with status ${response.status}.`);
      }

      const reader = response.body.getReader();
      const decoder = new TextDecoder();
      let buffer = "";

      while (true) {
        const { done, value } = await reader.read();
        if (done) {
          break;
        }
        buffer += decoder.decode(value, { stream: true });

        const chunks = buffer.split("\n\n");
        buffer = chunks.pop() || "";
        for (const chunk of chunks) {
          const line = chunk.trim();
          if (!line.startsWith("data: ")) {
            continue;
          }
          applyEvent(JSON.parse(line.slice(6)) as StreamEvent);
        }
      }
    } catch (fetchError) {
      const message =
        fetchError instanceof Error
          ? fetchError.message
          : "Unable to reach the RAG API. Is the scan_manager server running on port 8000?";
      setError(message);
      // Drop the empty assistant bubble if nothing streamed back.
      setMessages((previous) => {
        const last = previous[previous.length - 1];
        return last?.role === "assistant" && !last.text && last.tools.length === 0
          ? previous.slice(0, -1)
          : previous;
      });
    } finally {
      setStreaming(false);
    }
  };

  const resetConversation = async () => {
    if (sessionId) {
      void fetch(`${RAG_API_URL}/rag/session/${sessionId}`, { method: "DELETE" }).catch(() => undefined);
    }
    setSessionId(null);
    setMessages([]);
    setError(null);
  };

  return (
    <>
      <Head>
        <title>RAG chat · Court document research</title>
        <meta
          name="description"
          content="Conversational research over the court document corpus with cited answers."
        />
      </Head>

      <main className="min-h-screen bg-slate-50 px-4 py-10 dark:bg-slate-950">
        <div className="mx-auto flex max-w-4xl flex-col gap-5">
          <div className="flex items-center justify-between">
            <Link
              href="/"
              className="inline-flex items-center gap-2 text-sm font-medium text-slate-600 hover:text-slate-900 dark:text-slate-300 dark:hover:text-white"
            >
              ← Back to home
            </Link>
            <button
              type="button"
              onClick={() => void resetConversation()}
              disabled={streaming || messages.length === 0}
              className="rounded-md border border-slate-300 px-3 py-1.5 text-sm font-semibold text-slate-700 disabled:cursor-not-allowed disabled:opacity-40 dark:border-slate-700 dark:text-slate-200"
            >
              New conversation
            </button>
          </div>

          <header className="border-b border-slate-200 pb-4 dark:border-slate-700">
            <h1 className="text-2xl font-semibold text-slate-900 dark:text-white">RAG chat</h1>
            <p className="mt-2 max-w-3xl text-sm text-slate-600 dark:text-slate-300">
              Ask questions about the court document corpus. Claude searches the full-text index, reads the
              relevant pages, and answers with (filename, page) citations.
            </p>
          </header>

          <div className="flex min-h-[560px] flex-col rounded-lg border border-slate-200 bg-white shadow-sm dark:border-slate-700 dark:bg-slate-900">
            <div ref={scrollRef} className="min-h-0 flex-1 space-y-4 overflow-auto p-5">
              {messages.length === 0 ? (
                <div className="flex h-full min-h-[420px] items-center justify-center text-center text-sm text-slate-500 dark:text-slate-300">
                  Try: &ldquo;What does the Maxwell indictment charge?&rdquo; or &ldquo;Which documents mention
                  flight logs?&rdquo;
                </div>
              ) : (
                messages.map((message, index) => (
                  <div key={index} className={message.role === "user" ? "flex justify-end" : "flex justify-start"}>
                    <div
                      className={`max-w-[85%] rounded-lg px-4 py-3 text-sm leading-6 ${
                        message.role === "user"
                          ? "bg-slate-900 text-white dark:bg-slate-100 dark:text-slate-950"
                          : "border border-slate-200 bg-slate-50 text-slate-800 dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100"
                      }`}
                    >
                      {message.tools.length > 0 && (
                        <ul className="mb-2 space-y-1">
                          {message.tools.map((tool, toolIndex) => (
                            <li
                              key={toolIndex}
                              className="rounded bg-slate-200 px-2 py-1 text-xs text-slate-600 dark:bg-slate-800 dark:text-slate-300"
                            >
                              {describeTool(tool)}
                            </li>
                          ))}
                        </ul>
                      )}
                      <p className="whitespace-pre-wrap">
                        {message.text ||
                          (message.role === "assistant" && streaming && index === messages.length - 1
                            ? "Researching..."
                            : message.text)}
                      </p>
                    </div>
                  </div>
                ))
              )}
            </div>

            {error && (
              <p className="border-t border-slate-200 px-5 py-3 text-sm text-red-600 dark:border-slate-700">
                {error}
              </p>
            )}

            <div className="flex gap-3 border-t border-slate-200 p-4 dark:border-slate-700">
              <input
                type="text"
                value={input}
                onChange={(event) => setInput(event.target.value)}
                onKeyDown={(event) => {
                  if (event.key === "Enter" && !event.shiftKey) {
                    event.preventDefault();
                    void sendMessage();
                  }
                }}
                placeholder="Ask about the documents..."
                disabled={streaming}
                className="w-full rounded-md border border-slate-300 px-3 py-2 text-sm text-slate-800 shadow-sm focus:border-slate-500 focus:outline-none disabled:opacity-60 dark:border-slate-700 dark:bg-slate-950 dark:text-slate-100"
              />
              <button
                type="button"
                onClick={() => void sendMessage()}
                disabled={streaming || !input.trim()}
                className="rounded-md bg-slate-900 px-4 py-2 text-sm font-semibold text-white transition hover:bg-slate-700 disabled:cursor-not-allowed disabled:opacity-40 dark:bg-slate-100 dark:text-slate-950 dark:hover:bg-white"
              >
                {streaming ? "Streaming..." : "Send"}
              </button>
            </div>
          </div>
        </div>
      </main>
    </>
  );
}
