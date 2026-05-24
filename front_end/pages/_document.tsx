import Document, { Head, Html, Main, NextScript } from "next/document";

class MyDocument extends Document {
  render() {
    return (
      <Html lang="en" suppressHydrationWarning>
        <Head>
          <meta charSet="utf-8" />
          <meta name="application-name" content="EPS" />
          <meta name="apple-mobile-web-app-title" content="EPS" />
          <meta name="theme-color" content="#0f172a" />
          <meta name="color-scheme" content="light dark" />
          <meta name="robots" content="index, follow" />
          <link rel="icon" href="/eps-favicon.svg" type="image/svg+xml" />
          <link rel="manifest" href="/site.webmanifest" />
        </Head>
        <body>
          <Main />
          <NextScript />
        </body>
      </Html>
    );
  }
}

export default MyDocument;
