import Head from "next/head";
import Link from "next/link";
import React from "react";

export default function MatrixChatServicePage() {
  return (
    <>
      <Head>
        <title>Matrix chat · Coming soon</title>
        <meta
          name="description"
          content="Placeholder service for future Matrix chat server integration."
        />
      </Head>

      <main className="min-h-screen bg-slate-50 px-4 py-10 dark:bg-slate-950">
        <div className="mx-auto flex max-w-3xl flex-col gap-6 rounded-lg border border-slate-200 bg-white p-6 shadow-sm dark:border-slate-700 dark:bg-slate-900">
          <Link
            href="/"
            className="inline-flex items-center gap-2 text-sm font-medium text-slate-600 hover:text-slate-900 dark:text-slate-300 dark:hover:text-white"
          >
            ← Back to home
          </Link>

          <h1 className="text-2xl font-semibold text-slate-900 dark:text-slate-100">Matrix chat</h1>
          <p className="text-slate-600 dark:text-slate-300">
            This service is a placeholder. It will point to a Matrix server for team messaging.
          </p>
          <p className="text-sm text-slate-500 dark:text-slate-400">
            Integration coming soon: redirect to the Matrix homeserver and room configuration.
          </p>
        </div>
      </main>
    </>
  );
}
