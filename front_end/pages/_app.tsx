import '@/styles/globals.css';
import type { AppProps } from 'next/app';
import { ThemeProvider } from 'next-themes';
import SupabaseSessionProvider from '@/components/session/SupabaseSessionProvider';

export default function App({ Component, pageProps }: AppProps) {
  return (
    <ThemeProvider attribute="class" defaultTheme="light">
      <SupabaseSessionProvider>
        <Component {...pageProps} />
      </SupabaseSessionProvider>
    </ThemeProvider>
  );
}
