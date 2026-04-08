'use client';
import { supabase } from '@/lib/supabaseClient';
import { useSupabaseSession } from './SupabaseSessionProvider';
import Link from 'next/link';

export default function HeroLogIn() {
  const { session } = useSupabaseSession();

  const handleSignIn = async () => {
    await supabase.auth.signInAnonymously();
  };

  if (session) {
    return (
      <Link
        href="/image-selection"
        className="game-font inline-block mt-4 px-4 py-2 text-xl bg-slate-700 text-white rounded hover:bg-slate-800"
      >
        Get started
      </Link>
    );
  }

  return (
    <>
      Not signed in <br />
      <button
        onClick={handleSignIn}
        className="inline-block mt-2 px-4 py-2 bg-slate-700 text-white rounded hover:bg-slate-800"
      >
        Sign in anonymously
      </button>
    </>
  );
}
