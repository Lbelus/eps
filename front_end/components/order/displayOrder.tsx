"use client";
import React, { useState, useEffect } from "react";
import { useSession } from "next-auth/react";
import { useRouter } from "next/router";
import Link from "next/link";
import { ORDER } from "@/types/order_t";

const DisplayOrder: React.FC = () => {
  const router = useRouter();
  const { data: session, status } = useSession();

  const [collectionId, setCollectionId] = useState("");
  const [orders, setOrders] = useState<ORDER[]>([]);

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

  // Fetch orders data when collectionId changes
  useEffect(() => {
    if (collectionId) {
      fetch(`/api/readOrders?docRef=${encodeURIComponent(collectionId)}`)
        .then((res) => res.json())
        .then((data) => {
          if (data.orders) {
            setOrders(data.orders);
          } else {
            console.error('No orders found for user:', data);
          }
        })
        .catch((error) => {
          console.error('Error fetching orders:', error);
        });
    }
  }, [collectionId]);

  return (
    <>
    <div className="max-w-4xl mx-auto mt-10 p-6 rounded-lg shadow-md">
      <h2 className="text-2xl font-bold mb-6 text-center">Your Orders</h2>
      {orders.length > 0 ? (
        <div className="space-y-4">
          {orders.map((order) => (
            <div key={order.id} className="p-4 border rounded-lg shadow-sm">
              <h3 className="text-lg font-semibold">Order ID: {order.id}</h3>
              <p><strong>Status:</strong> {order.status}</p>
              <p><strong>Date:</strong> {order.date}</p>
              <h4 className="font-semibold mt-2">Order Details:</h4>
              <ul className="list-disc list-inside">
                {order.orderPayload.map((item, index) => (
                  <li key={index}>{JSON.stringify(item)}</li>
                ))}
              </ul>
            </div>
          ))}
        </div>
      ) : (
        <p className="text-center text-gray-500">No orders found.</p>
      )}
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

export default DisplayOrder;
