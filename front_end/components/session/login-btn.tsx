'use client';
import { supabase } from '@/lib/supabaseClient';
import { useSupabaseSession } from './SupabaseSessionProvider';

export default function LogIn() {
  const { session } = useSupabaseSession();

  const handleSignIn = async () => {
    await supabase.auth.signInAnonymously();
  };

  const handleSignOut = async () => {
    await supabase.auth.signOut();
  };

  if (session) {
    return (
      <>
        Signed in as {session.user?.id}
        <br />
        <button onClick={handleSignOut}>Sign out</button>
      </>
    );
  }

  return (
    <>
      Not signed in <br />
      <button onClick={handleSignIn}>Sign in anonymously</button>
    </>
  );
}
