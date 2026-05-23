import Head from "next/head";
import SectionTitle from "@/components/landingPage/sectionTitle";
import Footer from "@/components/landingPage/footer";
import PopupWidget from "@/components/landingPage/popupWidget";

const Terms = () => {
  return (
    <>
      <Head>
        <title>EPS - Terms of use</title>
        <meta
          name="description"
          content="Terms of use for Epstein Paper System."
        />
        <link rel="icon" href="/favicon.ico" />
      </Head>
      <SectionTitle pretitle="Legal" title="Terms of use" align="center">
        <p>
          Epstein Paper System is a research interface for searching and reviewing public court records and related public documents.
        </p>
        <p>
          By using EPS, you agree to use the service for lawful research, archival, journalistic, educational, or public-interest purposes.
        </p>
        <p>
          The documents and search results are provided for reference only. EPS does not provide legal advice and does not guarantee that every record is complete, current, or free from OCR or metadata errors.
        </p>
        <p>
          You are responsible for verifying important information against the original source records before relying on it.
        </p>
        <p>
          Do not misuse the service, attempt to disrupt its infrastructure, or use the content to harass, threaten, or unlawfully target any person.
        </p>
      </SectionTitle>
      <Footer />
      <PopupWidget />
    </>
  );
};

export default Terms;
