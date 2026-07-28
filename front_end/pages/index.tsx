import React from "react";
import Head from "next/head";
import Link from "next/link";
import HeroLogIn from "../components/session/hero-login-btn";
import Footer from "@/components/landingPage/footer";
import PopupWidget from "../components/landingPage/popupWidget";
import ThemeChanger from "../components/landingPage/DarkSwitch";

export default function Home() {
  return (
    <>
      <Head>
        <title>EPS — Epstein Paper System</title>
        <meta
          name="description"
          content="EPS is a court document search engine and viewer focused on the Jeffrey Epstein court case."
        />
        <link rel="icon" href="/eps-favicon.svg" type="image/svg+xml" />
      </Head>

      <header className="bg-[#f8f5f0] border-b border-slate-200 p-4 dark:border-slate-700 dark:bg-slate-950">
        <nav className="container mx-auto flex justify-between items-center">
          <Link href="/" className="text-xl font-semibold text-slate-900">
            EPS
          </Link>
          <div className="flex items-center gap-5 text-slate-700">
            <Link
              href="/changelog"
              className="text-sm font-medium text-slate-700 transition hover:text-slate-950 dark:text-slate-200 dark:hover:text-white"
            >
              Changelog
            </Link>
            <Link
              href="/contact"
              className="text-sm font-medium text-slate-700 transition hover:text-slate-950 dark:text-slate-200 dark:hover:text-white"
            >
              Contact
            </Link>
            <div className="hidden lg:block">
              <ThemeChanger />
            </div>
          </div>
        </nav>
      </header>

      <main className="flex flex-col flex-1">
        <section className="hero">
          <h1 className="text-6xl font-extrabold mb-4">EPS</h1>
          <p className="text-xl text-slate-700 max-w-3xl mx-auto mb-4">
            Epstein Paper System is a dedicated court document search engine and viewer for records related
            to the Jeffrey Epstein court case.
          </p>
          <p className="text-lg text-slate-600 max-w-3xl mx-auto mb-8">
            The platform provides a single interface to search filings, review documents, and navigate case
            materials with speed and clarity.
          </p>
          <div className="mx-auto mb-8 grid max-w-2xl grid-cols-2 gap-3 text-left sm:gap-4">
            <div className="rounded-lg border border-slate-200 bg-white/80 px-4 py-3 shadow-sm dark:border-slate-700 dark:bg-slate-900">
              <p className="text-sm font-medium uppercase tracking-wide text-slate-500 dark:text-slate-400">Documents</p>
              <p className="mt-1 text-2xl font-bold text-slate-950 dark:text-white">345,494</p>
            </div>
            <div className="rounded-lg border border-slate-200 bg-white/80 px-4 py-3 shadow-sm dark:border-slate-700 dark:bg-slate-900">
              <p className="text-sm font-medium uppercase tracking-wide text-slate-500 dark:text-slate-400">Pages</p>
              <p className="mt-1 text-2xl font-bold text-slate-950 dark:text-white">613,880</p>
            </div>
          </div>
          <HeroLogIn />
          <div className="mt-10" id="services">
            <h2 className="text-2xl font-semibold mb-6 text-slate-900">Available services</h2>
            <div className="mx-auto grid max-w-3xl gap-4 sm:grid-cols-2 text-left">
              <Link
                href="/services/search-engine"
                className="block rounded-lg border border-slate-200 bg-white/80 px-4 py-3 text-slate-700 shadow-sm transition hover:shadow-md dark:border-slate-700 dark:bg-slate-900"
              >
                <h3 className="text-lg font-semibold text-slate-900">Court document search</h3>
                <p className="text-sm text-slate-600">Search and paginate documents from the /courtdocuments API.</p>
              </Link>
              <Link
                href="/services/matrix-chat"
                className="block rounded-lg border border-slate-200 bg-white/80 px-4 py-3 text-slate-700 shadow-sm transition hover:shadow-md dark:border-slate-700 dark:bg-slate-900"
              >
                <h3 className="text-lg font-semibold text-slate-900">Matrix chat (placeholder)</h3>
                <p className="text-sm text-slate-600">Temporary entry point for the future Matrix messaging service.</p>
              </Link>
              <Link
                href="/changelog"
                className="block rounded-lg border border-slate-200 bg-white/80 px-4 py-3 text-slate-700 shadow-sm transition hover:shadow-md dark:border-slate-700 dark:bg-slate-900 sm:col-span-2"
              >
                <h3 className="text-lg font-semibold text-slate-900">Project changelog</h3>
                <p className="text-sm text-slate-600">Track recent EPS improvements and public-facing project updates.</p>
              </Link>
            </div>
          </div>
        </section>

        <section className="py-16 bg-slate-50">
          <div className="container mx-auto grid gap-6 sm:grid-cols-2 lg:grid-cols-3 px-4">
            <div className="feature-card">
              <h2 className="text-lg font-semibold mb-2">Unified access</h2>
              <p>A single entry point for exploring and reviewing case documents.</p>
            </div>
            <div className="feature-card">
              <h2 className="text-lg font-semibold mb-2">Fast retrieval</h2>
              <p>Quick lookup across indexed filings to find relevant records efficiently.</p>
            </div>
            <div className="feature-card">
              <h2 className="text-lg font-semibold mb-2">Document traceability</h2>
              <p>Structured document navigation to improve clarity, consistency, and review flow.</p>
            </div>
          </div>
        </section>

        <section className="py-16 container mx-auto px-4 text-center">
          <h2 className="text-3xl font-bold mb-4">Why EPS?</h2>
          <p className="text-slate-600 max-w-2xl mx-auto mb-8">
            EPS is built to make court case document research straightforward. Teams can quickly locate
            filings, open records, and move through case materials from one streamlined interface.
          </p>
          <div className="grid gap-6 sm:grid-cols-2 lg:grid-cols-3">
            <div className="feature-card">Focused court-case document discovery</div>
            <div className="feature-card">Consistent and readable viewing workflow</div>
            <div className="feature-card">Faster legal document navigation</div>
          </div>
        </section>
      </main>
      <Footer />
      <PopupWidget />
    </>
  );
}
