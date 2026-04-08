"use client";
import React, { useState, useEffect } from "react";
import { useSession } from "next-auth/react";
import { useRouter } from "next/router";
import Link from "next/link";
import { EXAMPLE_TYPE } from "@/types/example_t";

const emptyEXAMPLE_TYPE = (): EXAMPLE_TYPE[] => ([
  {
    data01: "value01",
    data02: "value02",
    data03: "value03",
    data04: "value04",
  },
]);

const UserOrder: React.FC = () => {
  const router = useRouter();
  const { data: session, status } = useSession();
  const [collectionId, setCollectionId] = useState("");

  if (status === "loading") {
    return <p>Loading...</p>;
  }
  if (!session) {
    typeof window !== 'undefined' && router.replace('/api/auth/signin');
    return <p>Redirecting...</p>;
  }
  
  useEffect(() => {
    if (session?.user?.email) {
      fetch(`/api/readUserDocId?email=${encodeURIComponent(session.user.email)}`)
        .then((res) => res.json())
        .then((data) => {
          setCollectionId(data.id); // Store the fetched data in state
        })
        .catch((error) => {
          console.error("Error fetching user data:", error);
        });
    }
  }, [session?.user?.email]);

  const handleSubmit = async (event: any) => {
    event.preventDefault();
    try {
      const response = await fetch("/api/createOrder", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          docRef: collectionId,
          data: emptyEXAMPLE_TYPE(),
        }),
      });

      if (response.ok) {
        const data = await response.json();
        alert("Data updated successfully");
        console.log(data);
      } else {
        throw new Error("Failed to update order data");
      }
    } catch (error) {
      console.error("Failed to update order data:", error);
      alert("Failed to update order data");
    }
  };

  return (
    <>
    <div className="max-w-2xl mx-auto mt-10 p-6 rounded-lg shadow-md">
        <h2 className="text-2xl font-bold mb-6 text-center">Make an Order</h2>
        <div className="space-y-6">
        <div className="flex items-center justify-between">
            <span className="text-gray-700">Data 01:</span>
            <span className="gray font-medium text-black">value01</span>
        </div>
        <div className="flex items-center justify-between">
            <span className="text-gray-700">Data 02:</span>
            <span className="gray font-medium text-black">value02</span>
        </div>
        <div className="flex items-center justify-between">
            <span className="text-gray-700">Data 03:</span>
            <span className="gray font-medium text-black">value03</span>
        </div>
        <div className="flex items-center justify-between">
            <span className="text-gray-700">Data 04:</span>
            <span className="gray font-medium text-black">value04</span>
        </div>
        </div>
        <div className="flex justify-center mt-8">
        <button
            type="button"
            className="flex justify-center rounded border border-stroke px-6 py-2 font-medium hover:shadow-1"
            onClick={handleSubmit}
        >
            Set Order
        </button>
        </div>
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
}

export default UserOrder;