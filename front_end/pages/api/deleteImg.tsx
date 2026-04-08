// import { storage} from '@/components/firebaseObj/firebase';
// import { ref, deleteObject } from "firebase/storage";
// import { NextApiRequest, NextApiResponse } from 'next';

// export default async function handler(req: NextApiRequest, res: NextApiResponse) {
//     if (req.method === 'POST') {
//       try {
//         // ------------------ api route authentication
//         const url = `${req.headers["x-forwarded-proto"]}://${req.headers.host}/api/auth/session`;
//         const sessionRes = await fetch(url, {
//           method: 'GET',
//           headers: {
//             'Content-Type': 'application/json',
//             'Cookie': req.headers.cookie || ''
//           }
//         });
//         if (!sessionRes.ok) {
//           console.error('Failed to fetch session:', sessionRes.status, sessionRes.statusText);
//           return res.status(401).json({ message: 'Not authenticated' });
//         }
//         const session = await sessionRes.json();
//         if (!session || !session.user) {
//           return res.status(401).json({ message: 'Not authenticated' });
//         }
//         // ------------------ end of api route authentication
//   
//         const { url: imageUrl } = req.body;
//         if (!imageUrl) {
//           return res.status(400).json({ error: "Image URL is required" });
//         }
//         const imageRef = ref(storage, imageUrl);
//         await deleteObject(imageRef);
//   
//         res.status(200).json({ message: "Image deleted successfully" });
//       } catch (error: any) {
//         console.error('Failed to delete image:', error);
//         res.status(500).json({ error: "Failed to delete image", details: error.message });
//       }
//     } else {
//       res.setHeader('Allow', ['POST']);
//       res.status(405).end(`Method ${req.method} Not Allowed`);
//     }
// }
