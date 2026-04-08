'use client';
import { Session } from '@supabase/supabase-js';
import { createContext, useContext, useEffect, useState } from 'react';
import { supabase } from '@/lib/supabaseClient';

type SessionContextValue = {
  session: Session | null;
};

const SessionContext = createContext<SessionContextValue>({ session: null });

export default function SupabaseSessionProvider({ children }: { children: React.ReactNode }) {
  const [session, setSession] = useState<Session | null>(null);

  useEffect(() => {
    const getSession = async () => {
      const { data } = await supabase.auth.getSession();
      setSession(data.session);
    };
    getSession();

    const { data: listener } = supabase.auth.onAuthStateChange((_event, session) => {
      setSession(session);
    });

    return () => {
      listener.subscription.unsubscribe();
    };
  }, []);

  return <SessionContext.Provider value={{ session }}>{children}</SessionContext.Provider>;
}

export const useSupabaseSession = () => useContext(SessionContext);
