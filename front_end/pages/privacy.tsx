import Head from "next/head";
import SectionTitle from "@/components/landingPage/sectionTitle";
import Footer from "@/components/landingPage/footer";
import PopupWidget from "@/components/landingPage/popupWidget";

const Privacy = () => {
  return (
    <>
      <Head>
        <title>S|C - Politique de confidentialité</title>
        <meta
          name="description"
          content="Politique de confidentialité de S|C - Support Centre."
        />
        <link rel="icon" href="/favicon.ico" />
      </Head>
      <SectionTitle
        pretitle="Légal"
        title="Politique de confidentialité" align="center">
            <p>S|C respecte la confidentialité des utilisateurs et des équipes qui utilisent la plateforme.</p>
            <p>Les informations saisies dans S|C sont utilisées pour traiter les demandes, assurer leur suivi et améliorer l'expérience de support.</p>
            <p>Nous ne collectons que les données nécessaires au fonctionnement du service et à la coordination des activités commerciales et administratives.</p>
            <p>Les données sont conservées le temps requis pour répondre aux besoins opérationnels et respecter les obligations légales.</p>
            <p>Aucune donnée n'est vendue à des tiers ; les accès sont limités aux personnes autorisées.</p>
      </SectionTitle>
      <Footer />
      <PopupWidget />
    </>
  );
}

export default Privacy;
