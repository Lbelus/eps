"use client";
import React, { useState, useEffect } from "react";
import { useSession } from "next-auth/react";
import { useRouter } from "next/router";
import Link from "next/link";

const RemoveOrder: React.FC = () => {
  const router = useRouter();
  const { data: session, status } = useSession();

  const [collectionId, setCollectionId] = useState("");
  const [orderId, setOrderID] = useState("");

  // Redirect if not authenticated
  useEffect(() => {
    if (status === "loading") {
      return;
    }
    if (!session) {
      typeof window !== 'undefined' && router.replace('/api/auth/signin');
    }
  }, [session, status, router]);

  // Fetch user data
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

  // Handle order deletion
  const handleDelete = async (event: any) => {
    event.preventDefault();
    try {
      const response = await fetch("/api/deleteOrder", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          docRef: collectionId,
          orderId: orderId,
        }),
      });

      if (response.ok) {
        const data = await response.json();
        alert("Order deleted successfully");
        console.log(data);
      } else {
        throw new Error("Failed to delete order");
      }
    } catch (error) {
      console.error("Failed to delete order:", error);
      alert("Failed to delete order");
    }
  };

  return (
    <>
    <div className="max-w-4xl mx-auto mt-10 p-6 rounded-lg shadow-md">
      <h2 className="text-2xl font-bold mb-6 text-center">Manage Your Orders</h2>
      <form onSubmit={handleDelete} className="space-y-6">
        <div>
          <label htmlFor="orderId" className="block text-sm font-medium text-white-700">
            Order ID to Delete
          </label>
          <input
            type="text"
            id="orderId"
            name="orderId"
            value={orderId}
            onChange={(e) => setOrderID(e.target.value)}
            className="gray mt-1 block w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:outline-none focus:ring-indigo-500 focus:border-indigo-500 sm:text-sm"
            required
          />
        </div>
        <button
          type="submit"
          className="flex justify-center rounded border border-stroke px-6 py-2 font-medium hover:shadow-1"
        >
          Delete Order
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

export default RemoveOrder;
