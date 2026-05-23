import Head from "next/head";
import SectionTitle from "@/components/landingPage/sectionTitle";
import Footer from "@/components/landingPage/footer";
import PopupWidget from "@/components/landingPage/popupWidget";

const Legal = () => {
  return (
    <>
      <Head>
        <title>EPS - Legal notice</title>
        <meta
          name="description"
          content="Legal notice for Epstein Paper System."
        />
        <link rel="icon" href="/favicon.ico" />
      </Head>
      <SectionTitle pretitle="Legal" title="Legal notice" align="center">
        <p>
          Epstein Paper System provides tools for indexing, searching, and viewing public court documents and public government releases.
        </p>
        <p>
          EPS is an independent research project and is not affiliated with any court, government agency, law enforcement body, party, witness, or media organization.
        </p>
        <p>
          The project may rely on OCR, automated parsing, and metadata extraction. Those processes can introduce errors, omissions, or formatting issues.
        </p>
        <p>
          Content made available through EPS should be treated as a research aid, not as an official record. Original source documents remain the authority for citation or legal use.
        </p>
        <p>
          References to people, organizations, events, or allegations reflect the contents of public records and should not be read as independent findings by EPS.
        </p>
      </SectionTitle>
      <Footer />
      <PopupWidget />
    </>
  );
};

export default Legal;
