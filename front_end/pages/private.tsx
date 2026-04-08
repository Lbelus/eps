import { useRouter } from "next/router";
import { useSession } from "next-auth/react";
import Link from "next/link";
import React from "react";
import Head from "next/head";

export default function Home() {
  const router = useRouter();
  const { data: session, status } = useSession();

  if (status === 'loading') {
    return <p>Loading...</p>; // Show a loading message while checking the session status
  }

  if (status === 'unauthenticated') {
    if (typeof window !== 'undefined') {
      router.replace('/api/auth/signin');
    }
    return <p>Redirecting...</p>;
  }

  return (
    <>
      <Head>
        <title>This is our sweet lovely app.</title>
        <meta name="description" content="This is an api route template" />
        <link rel="icon" href="/favicon.ico" />
      </Head>
      <span> This is a private page</span>
      <Link href="/">
        <span className="flex items-center space-x-2 text-2xl font-medium text-indigo-500 dark:text-gray-100 hover:underline">
          <span>Go back to public</span>
        </span>
      </Link>
      <Link href="/apiTest">
        <span className="flex items-center space-x-2 text-2xl font-medium text-indigo-500 dark:text-gray-100 hover:underline">
          <span>Go to api and components test page</span>
        </span>
      </Link>
    </>
  );
}
