// import { db } from '@/components/firebaseObj/firebase';
// import { NextApiRequest, NextApiResponse } from 'next';
// import { doc, addDoc, collection } from "firebase/firestore";

// export default async function handler(req: NextApiRequest, res: NextApiResponse) {
//   if (req.method === 'POST') {
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
//       const { docRef, data } = req.body;

//       if (!Array.isArray(data)) {
//         throw new Error("Data should be an array");
//       }

//       const userDocRef = doc(db, "users", docRef);
//       const ordersCollectionRef = collection(userDocRef, "orders");

//       const enrichedDataPromises = data.map(async (item: any) => {
//         return {
//           ...item,
//           status: "Pending",
//           link: "",
//         };
//       });
//       const enrichedData = await Promise.all(enrichedDataPromises);

//       const orderDoc = {
//         metadata: {
//           status: "Pending",
//           timestamp: Date.now(),
//         },
//         orderPayload: enrichedData,
//       };

//       const orderDocRef = await addDoc(ordersCollectionRef, orderDoc);

//       res.status(200).json({ message: "Update successful" });
//     } catch (error:any) {
//       console.error('Update failed:', error);
//       res.status(500).json({ error: "Update failed", details: error.message });
//     }
//   } else {
//     res.setHeader('Allow', ['POST']);
//     res.status(405).end(`Method ${req.method} Not Allowed`);
//   }
// }
