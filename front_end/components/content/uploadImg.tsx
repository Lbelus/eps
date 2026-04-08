"use client";
import React, { useState, useRef, useEffect } from "react";
import { useSession } from "next-auth/react";
import { useRouter } from "next/router";
import Link from "next/link";
const UploadImg: React.FC = () => {
  const router = useRouter();
  const { data: session, status } = useSession();

  const [file, setFile] = useState<File | null>(null);
  const inputFile = useRef<HTMLInputElement | null>(null);

  if (status === "loading") {
    return <p>Loading...</p>;
  }
  if (!session) {
    typeof window !== 'undefined' && router.replace('/api/auth/signin');
    return <p>Redirecting...</p>;
  }

  const handleFileChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    const fileList = event.target.files;
    if (fileList && fileList[0]) {
      setFile(fileList[0]);
    }
  };

  const handleSubmit = async (event: any) => {
    event.preventDefault();
    if (!file) {
      alert("Please select a file to upload.");
      return;
    }

    const formData = new FormData();
    formData.append('file', file);

    try {
      const response = await fetch('/api/createImg', {
        method: 'POST',
        body: formData,
      });
      if (!response.ok) {
        throw new Error('Failed to upload file');
      }
      const fileData = await response.json();
      const imageUrl = fileData.url;
      console.log("Image Uploaded, URL:", imageUrl);
      alert(`Image Uploaded, URL: ${imageUrl}`)
    } catch (error) {
      console.error("Error during the file upload:", error);
      alert("Failed to upload file");
    }
  };

  return (
    <>
    <div className="max-w-2xl mx-auto mt-10 p-6 rounded-lg shadow-md">
      <h2 className="text-2xl font-bold mb-6 text-center">Upload Image</h2>
      <form onSubmit={handleSubmit} className="space-y-6">
        <div>
          <label htmlFor="file" className="block text-sm font-medium text-white-700">
            Select Image
          </label>
          <input
            type="file"
            id="file"
            name="file"
            ref={inputFile}
            onChange={handleFileChange}
            className="mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm"
            required
          />
        </div>
        <button
          type="submit"
          className="flex justify-center rounded border border-stroke px-6 py-2 font-medium hover:shadow-1"
        >
          Submit Data
        </button>
      </form>
    </div>
    <section className="mb-8">
        <Link href="/private">
          <div className="flex items-center space-x-2 text-2xl font-medium text-indigo-500 dark:text-gray-100 hover:underline">
            Go to private page
          </div>
        </Link>
    </section>
    </>
  );
};

export default UploadImg;
