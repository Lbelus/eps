import Head from "next/head";
import SectionTitle from "@/components/landingPage/sectionTitle";
import Footer from "@/components/landingPage/footer";
import PopupWidget from "@/components/landingPage/popupWidget";

const Terms = () => {
  return (
    <>
      <Head>
        <title>S|C - Conditions d'utilisation</title>
        <meta
          name="description"
          content="Conditions d'utilisation de S|C - Support Centre."
        />
        <link rel="icon" href="/favicon.ico" />
      </Head>
      <SectionTitle
        pretitle="Légal"
        title="Conditions d'utilisation" align="center">
            <p>S|C (Support Centre) est une interface centralisée pour accéder à nos services backend.</p>
            <p>En utilisant ce site, vous acceptez de respecter les lois applicables ainsi que ces conditions.</p>
            <p>Vous vous engagez à utiliser la plateforme de manière responsable, sans perturber les services ni porter atteinte aux droits d'autrui.</p>
            <p>Les fonctionnalités peuvent évoluer afin d'améliorer la prise en charge et le suivi des demandes.</p>
            <p>Si vous n'acceptez pas ces conditions, merci de ne pas utiliser S|C.</p>
      </SectionTitle>
      <Footer />
      <PopupWidget />
    </>
  );
}

export default Terms;
