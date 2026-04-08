// import { db } from '@/components/firebaseObj/firebase';
// import { doc, collection, getDocs } from "firebase/firestore";
// import { NextApiRequest, NextApiResponse } from 'next';

// export default async function handler(req: NextApiRequest, res: NextApiResponse) {
//   if (req.method === 'GET') {
//     try {
//       // ------------------ api route authentication
//       const url = `${req.headers["x-forwarded-proto"]}://${req.headers.host}/api/auth/session`;
//       const sessionRes = await fetch(url, {
//         method: 'GET',
//         headers: {
//           'Content-Type': 'application/json',
//           'Cookie': req.headers.cookie || ''
//         }
//       });
//       if (!sessionRes.ok) {
//         console.error('Failed to fetch session:', sessionRes.status, sessionRes.statusText);
//         return res.status(401).json({ message: 'Not authenticated' });
//       }
//       const session = await sessionRes.json();
//       if (!session || !session.user) {
//         return res.status(401).json({ message: 'Not authenticated' });
//       }
//       // ------------------ end of api route authentication

//       const { docRef } = req.query;
//       if (!docRef) {
//         return res.status(400).json({ error: "docRef query parameter is required" });
//       }

//       const userDocRef = doc(db, "users", docRef.toString());
//       const ordersCollectionRef = collection(userDocRef, "orders");
//       const ordersSnapshot = await getDocs(ordersCollectionRef);

//       let orders: any[] = [];
//       ordersSnapshot.forEach((orderDoc) => {
//         const data = orderDoc.data();
//         const metadata = data.metadata || {};
//         orders.push({
//           id: orderDoc.id,
//           status: metadata.status || 'N/A',
//           date: metadata.timestamp ? new Date(metadata.timestamp).toLocaleDateString() : 'N/A',
//           orderPayload: data.orderPayload || [],
//         });
//       });

//       res.status(200).json({ orders });
//     } catch (error) {
//       console.error('Failed to fetch orders:', error);
//       res.status(500).json({ error: "Failed to fetch orders", details: error });
//     }
//   } else {
//     res.setHeader('Allow', ['GET']);
//     res.status(405).end(`Method ${req.method} Not Allowed`);
//   }
// }
