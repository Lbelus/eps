import Head from "next/head";
import Link from "next/link";
import React from "react";
import SearchEngineQueryClient from "@/components/services/SearchEngineQueryClient";

export default function SearchEngineServicePage() {
  return (
    <>
      <Head>
        <title>Search engine · REST requests</title>
        <meta
          name="description"
          content="Query the Search Engine courtdocuments routes with filters, pagination, and response preview."
        />
      </Head>

      <main className="min-h-screen bg-slate-50 px-4 py-10 dark:bg-slate-950">
        <div className="mx-auto flex max-w-6xl flex-col gap-6">
          <Link
            href="/"
            className="inline-flex items-center gap-2 text-sm font-medium text-slate-600 hover:text-slate-900 dark:text-slate-300 dark:hover:text-white"
          >
            ← Back to home
          </Link>
          <SearchEngineQueryClient />
        </div>
      </main>
    </>
  );
}
