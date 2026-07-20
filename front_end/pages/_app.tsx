import "@/styles/globals.css";
import Head from "next/head";
import type { AppProps } from "next/app";
import { useRouter } from "next/router";
import { ThemeProvider } from "next-themes";
import CookieNotice from "@/components/privacy/CookieNotice";

const siteName = "EPS - Epstein Paper System";
const siteDescription =
  "EPS is a court document search engine and viewer focused on the Jeffrey Epstein court case.";
const siteKeywords = [
  "EPS",
  "Epstein Paper System",
  "court documents",
  "document search",
  "legal research",
  "public records",
  "Jeffrey Epstein court case",
];
const siteUrl = process.env.NEXT_PUBLIC_SITE_URL?.replace(/[/]$/, "") || "";

export default function App({ Component, pageProps }: AppProps) {
  const router = useRouter();
  const path = router.asPath.split("?")[0] || "/";
  const canonicalUrl = siteUrl ? `${siteUrl}${path === "/" ? "" : path}` : undefined;
  const socialImage = siteUrl ? `${siteUrl}/background.png` : "/background.png";

  return (
    <>
      <Head>
        <title>{siteName}</title>
        <meta name="viewport" content="width=device-width, initial-scale=1" />
        <meta name="description" content={siteDescription} />
        <meta name="keywords" content={siteKeywords.join(", ")} />
        <meta name="author" content="Epstein Paper System" />
        <meta property="og:site_name" content={siteName} />
        <meta property="og:title" content={siteName} />
        <meta property="og:description" content={siteDescription} />
        <meta property="og:image" content={socialImage} />
        {canonicalUrl && <meta property="og:url" content={canonicalUrl} />}
        <meta property="og:type" content="website" />
        <meta name="twitter:card" content="summary_large_image" />
        <meta name="twitter:title" content={siteName} />
        <meta name="twitter:description" content={siteDescription} />
        <meta name="twitter:image" content={socialImage} />
        {canonicalUrl && <link rel="canonical" href={canonicalUrl} />}
      </Head>
      <ThemeProvider attribute="class" defaultTheme="light">
        <Component {...pageProps} />
        <CookieNotice />
      </ThemeProvider>
    </>
  );
}
