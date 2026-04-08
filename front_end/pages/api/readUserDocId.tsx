// import getFirebaseAdmin from '@/components/firebaseObj/firebaseAdmin';
// import { NextApiRequest, NextApiResponse } from 'next';

// export default async function handler(req: NextApiRequest, res: NextApiResponse) {
//   try {
// // ------------------ api route authentication
//     const url = `${req.headers["x-forwarded-proto"]}://${req.headers.host}/api/auth/session`;
//     const sessionRes = await fetch(url, {
//       method: 'GET',
//       headers: {
//         'Content-Type': 'application/json',
//         'Cookie': req.headers.cookie || ''
//       }
//     });
//     if (!sessionRes.ok) {
//       console.error('Failed to fetch session:', sessionRes.status, sessionRes.statusText);
//       return res.status(401).json({ message: 'Not authenticated' });
//     }
//     const session = await sessionRes.json();
//     if (!session || !session.user) {
//       return res.status(401).json({ message: 'Not authenticated' });
//     }
// // ------------------ end of api route authentication
//     const admin = getFirebaseAdmin();
//     const db = admin.firestore();

//     // Ensure that the request has an email query parameter
//     if (!req.query.email) {
//       return res.status(400).json({ error: 'Email query parameter is required' });
//     }

//     const usersRef = db.collection('users');
//     const snapshot = await usersRef.where('email', '==', req.query.email as string).get();

//     if (snapshot.empty) {
//       console.log('No matching documents.');
//       return res.status(404).json({ error: 'No user found with that email' });
//     }

//     const userDoc = snapshot.docs[0];

//     res.status(200).json({ id: userDoc.id });
//   } catch (error: any) {
//     console.error('Error accessing Firestore:', error);
//     res.status(500).json({ error: error.message });
//   }
// }
