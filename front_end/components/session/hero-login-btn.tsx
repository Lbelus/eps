import Link from 'next/link';

export default function HeroLogIn() {
  return (
    <Link
      href="/services/search-engine"
      className="game-font inline-block mt-4 px-4 py-2 text-xl bg-slate-700 text-white rounded hover:bg-slate-800"
    >
      Enter EPS
    </Link>
  );
}
