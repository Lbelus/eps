import Document, { Html, Head, Main, NextScript, DocumentContext } from 'next/document';

class MyDocument extends Document {
  static async getInitialProps(ctx: DocumentContext) {
    const initialProps = await Document.getInitialProps(ctx);
    return { ...initialProps };
  }

  render() {
    return (
      <Html lang="en">
        <Head>
          <meta charSet="UTF-8" />
          <meta name="viewport" content="width=device-width, initial-scale=1.0" />
          <title>Your Website Title</title>
          <meta name="description" content="A brief description of your webpage" />
          <meta name="keywords" content="keyword1, keyword2, keyword3" />
          <meta name="author" content="Your Name or Company" />
          <meta property="og:title" content="Your Website Title" />
          <meta property="og:description" content="A brief description of your webpage" />
          <meta property="og:image" content="URL to image for social media preview" />
          <meta property="og:url" content="URL of your page" />
          <meta property="og:type" content="website" />
          <meta name="twitter:card" content="summary_large_image" />
          <meta name="twitter:site" content="@your_twitter_handle" />
          <meta name="twitter:title" content="Your Website Title" />
          <meta name="twitter:description" content="A brief description of your webpage" />
          <meta name="twitter:image" content="URL to image for Twitter card" />
          <link rel="icon" href="/favicon.ico" />
          <link rel="apple-touch-icon" sizes="180x180" href="/apple-touch-icon.png" />
          <link rel="icon" type="image/png" sizes="32x32" href="/favicon-32x32.png" />
          <link rel="icon" type="image/png" sizes="16x16" href="/favicon-16x16.png" />
          <link rel="manifest" href="/site.webmanifest" />
          <link rel="canonical" href="https://yourwebsite.com/current-page-url" />
          <link rel="stylesheet" href="https://path/to/stylesheet.css" />
          <link rel="preconnect" href="https://fonts.googleapis.com" />
          {/* <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin /> */}
          <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@400;700&display=swap" rel="stylesheet" />
          <link href="https://fonts.googleapis.com/css2?family=Press+Start+2P&display=swap" rel="stylesheet" />
          <meta name="theme-color" content="#ffffff" />
          <meta name="robots" content="index, follow" />
          <meta name="google-site-verification" content="verification_token" />
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