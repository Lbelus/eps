"use client";
import React, { useState, useEffect } from "react";
import { useSession } from "next-auth/react";
import { useRouter } from "next/router";
import Link from "next/link";

const RemoveImg: React.FC = () => {
  const router = useRouter();
  const { data: session, status } = useSession();

  const [url, setUrl] = useState("");

  if (status === "loading") {
    return <p>Loading...</p>;
  }
  if (!session) {
    typeof window !== 'undefined' && router.replace('/api/auth/signin');
    return <p>Redirecting...</p>;
  }

  const handleSubmit = async (event: any) => {
    event.preventDefault();
    if (!url) {
      alert("Please provide a URL.");
      return;
    }

    try {
      const response = await fetch('/api/deleteImg', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ url }),
      });
      if (!response.ok) {
        throw new Error('Failed to delete image');
      }
      const data = await response.json();
      console.log("Image Deleted:", data.message);
      alert(`Image Deleted: ${data.message}`);
    } catch (error) {
      console.error("Error during the image deletion:", error);
      alert("Failed to delete image");
    }
  };

  return (
    <>
    <div className="max-w-2xl mx-auto mt-10 p-6 rounded-lg shadow-md">
      <h2 className="text-2xl font-bold mb-6 text-center">Remove Image</h2>
      <form onSubmit={handleSubmit} className="space-y-6">
        <div>
          <label htmlFor="url" className="block text-sm font-medium text-white-700">
            Image URL
          </label>
          <input
            type="text"
            id="url"
            name="url"
            value={url}
            onChange={(e) => setUrl(e.target.value)}
            className="gray mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm"
            required
          />
        </div>
        <button
          type="submit"
          className="flex justify-center rounded border border-stroke px-6 py-2 font-medium hover:shadow-1"
        >
          Delete Image
        </button>
      </form>
    </div>
    <section className="mt-8">
        <Link href="/private">
          <div className="flex items-center space-x-2 text-2xl font-medium text-indigo-500 dark:text-gray-100 hover:underline">
            Go to private page
          </div>
        </Link>
    </section>
    </>
  );
};

export default RemoveImg;