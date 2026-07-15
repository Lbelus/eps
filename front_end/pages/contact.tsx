import Head from "next/head";
import SectionTitle from "@/components/landingPage/sectionTitle";
import Footer from "@/components/landingPage/footer";
import PopupWidget from "@/components/landingPage/popupWidget";

const CONTACT_EMAIL = "papersystem@proton.me";

const Contact = () => {
  return (
    <>
      <Head>
        <title>EPS - Contact</title>
        <meta
          name="description"
          content="Contact Epstein Paper System for requests and inquiries."
        />
        <link rel="icon" href="/favicon.ico" />
      </Head>
      <SectionTitle pretitle="Contact" title="Contact EPS" align="center">
        <p>
          Please contact us if you have any request or inquiry at{" "}
          <a
            href={`mailto:${CONTACT_EMAIL}`}
            className="font-semibold text-slate-900 underline underline-offset-4 dark:text-slate-100"
          >
            {CONTACT_EMAIL}
          </a>
          .
        </p>
      </SectionTitle>
      <Footer />
      <PopupWidget />
    </>
  );
};

export default Contact;
