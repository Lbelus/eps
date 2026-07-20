import Head from "next/head";
import SectionTitle from "@/components/landingPage/sectionTitle";
import Footer from "@/components/landingPage/footer";
import PopupWidget from "@/components/landingPage/popupWidget";

const Privacy = () => {
  return (
    <>
      <Head>
        <title>EPS - Privacy policy</title>
        <meta
          name="description"
          content="Privacy policy for Epstein Paper System."
        />
        <link rel="icon" href="/favicon.ico" />
      </Head>
      <SectionTitle pretitle="Legal" title="Privacy policy" align="center">
        <p>
          EPS is designed as a document research tool. The searchable corpus is built from public records and public releases.
        </p>
        <p>
          When you use the site, standard technical information such as request metadata, browser information, and basic usage signals may be processed to operate, secure, and improve the service.
        </p>
        <p>
          Search terms and interactions may be handled by the application and REST API to return results, diagnose issues, and monitor reliability.
        </p>
        <p>
          EPS does not use analytics cookies. The frontend may use local browser storage for basic preferences such as theme and notice dismissal, while the hosting provider may keep standard operational and security logs.
        </p>
        <p>
          EPS does not sell user data. Access to operational data should be limited to maintainers or authorized service providers who need it to run the project.
        </p>
        <p>
          Public documents may contain names, allegations, and sensitive details already present in source records. Users should handle that material responsibly and verify it against the original record.
        </p>
      </SectionTitle>
      <Footer />
      <PopupWidget />
    </>
  );
};

export default Privacy;
