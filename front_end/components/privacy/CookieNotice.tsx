import Link from "next/link";
import { useEffect, useState } from "react";

const STORAGE_KEY = "eps_cookie_notice_ack";

const CookieNotice = () => {
  const [visible, setVisible] = useState(false);

  useEffect(() => {
    try {
      setVisible(window.localStorage.getItem(STORAGE_KEY) !== "true");
    } catch (_error) {
      setVisible(true);
    }
  }, []);

  const dismiss = () => {
    setVisible(false);
    try {
      window.localStorage.setItem(STORAGE_KEY, "true");
    } catch (_error) {
      // Keep the notice dismissed for this session if storage is unavailable.
    }
  };

  if (!visible) {
    return null;
  }

  return (
    <div className="fixed inset-x-0 bottom-0 z-50 px-4 pb-4 sm:px-6">
      <div className="mx-auto flex max-w-4xl flex-col gap-3 rounded-lg border border-slate-200 bg-white p-4 shadow-lg dark:border-slate-700 dark:bg-slate-900 sm:flex-row sm:items-center sm:justify-between">
        <p className="text-sm leading-6 text-slate-700 dark:text-slate-200">
          EPS does not use analytics cookies. We may use local browser storage for basic preferences, and our host may keep standard security and operational logs. See our{" "}
          <Link href="/privacy" className="font-semibold underline underline-offset-2 hover:text-slate-950 dark:hover:text-white">
            Privacy Policy
          </Link>{" "}
          for details.
        </p>
        <button
          type="button"
          onClick={dismiss}
          className="shrink-0 rounded-md bg-slate-900 px-4 py-2 text-sm font-semibold text-white transition hover:bg-slate-700 dark:bg-slate-100 dark:text-slate-950 dark:hover:bg-white"
        >
          Got it
        </button>
      </div>
    </div>
  );
};

export default CookieNotice;
