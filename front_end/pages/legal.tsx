import Head from "next/head";
import SectionTitle from "@/components/landingPage/sectionTitle";
import Footer from "@/components/landingPage/footer";
import PopupWidget from "@/components/landingPage/popupWidget";

const Legal = () => {
  return (
    <>
      <Head>
        <title>S|C - Mentions légales</title>
        <meta
          name="description"
          content="Mentions légales de S|C - Support Centre."
        />
        <link rel="icon" href="/favicon.ico" />
      </Head>
      <SectionTitle
        pretitle="Légal"
        title="Mentions légales" align="center">
            <p>S|C (Support Centre) est une plateforme interne destinée à relier l'expérience utilisateur et les services backend.</p>
            <p>Les contenus présentés ont pour objectif de faciliter la prise en charge et le suivi des demandes.</p>
            <p>Nous nous efforçons de maintenir les informations à jour, sans garantir l'absence d'erreurs ou d'omissions.</p>
            <p>L'utilisation des informations et services disponibles sur ce site se fait sous la responsabilité de l'utilisateur.</p>
            <p>Ces mentions peuvent être modifiées pour refléter l'évolution des services proposés.</p>
      </SectionTitle>
      <Footer />
      <PopupWidget />
    </>
  );
}

export default Legal;
