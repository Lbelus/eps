// import { ref, uploadBytes, getDownloadURL } from "firebase/storage";
// import { storage } from '@/components/firebaseObj/firebase';
// import multer from 'multer';
// import { v4 as uuidv4 } from 'uuid';

// const upload = multer({ storage: multer.memoryStorage() });

// export const config = {
//   api: {
//     bodyParser: false,
//   },
// };

// export default async function handler(req: any, res: any) {
//   if (req.method === 'POST') {
//     // ------------------ api route authentication
//     const url = `${req.headers["x-forwarded-proto"]}://${req.headers.host}/api/auth/session`;
//     const sessionRes = await fetch(url, {
//     method: 'GET',
//     headers: {
//         'Content-Type': 'application/json',
//         'Cookie': req.headers.cookie || ''
//     }
//     });
//     if (!sessionRes.ok) {
//     console.error('Failed to fetch session:', sessionRes.status, sessionRes.statusText);
//     return res.status(401).json({ message: 'Not authenticated' });
//     }
//     const session = await sessionRes.json();
//     if (!session || !session.user) {
//     return res.status(401).json({ message: 'Not authenticated' });
//     }
//     // ------------------ end of api route authentication
//     upload.single('file')(req, res, async (error:any) => {
//       if (error) {
//         return res.status(500).json({ error: error.message });
//       }

//       if (!req.file) {
//         return res.status(400).send('No file uploaded.');
//       }

//       const file = req.file;
//       const randomFileName = `${uuidv4()}-${file.originalname}`;
//       const storageRef = ref(storage, `images/${randomFileName}`);

//       try {
//         const snapshot = await uploadBytes(storageRef, file.buffer);
//         const downloadURL = await getDownloadURL(snapshot.ref);
//         res.status(200).json({ url: downloadURL });
//       } catch (error:any) {
//         console.error('Failed to upload file:', error);
//         res.status(500).json({ error: 'Failed to upload file', details: error.message });
//       }
//     });
//   } else {
//     res.setHeader('Allow', ['POST']);
//     res.status(405).end(`Method ${req.method} Not Allowed`);
//   }
// }
