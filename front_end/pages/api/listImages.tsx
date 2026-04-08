// import { NextApiRequest, NextApiResponse } from 'next'
// import { supabase } from '@/lib/supabase/client'

// export default async function handler(req: NextApiRequest, res: NextApiResponse) {
//   if (req.method !== 'GET') {
//     res.setHeader('Allow', ['GET']);
//     return res.status(405).json({ message: 'Method Not Allowed' });
//   }

//   try {
//     const { data, error } = await supabase
//       .from('images')
//       .select('id, name, description, price');

//     if (error) {
//       throw error;
//     }

//     res.status(200).json({ images: data });
//   } catch (error: any) {
//     console.error('Failed to list images:', error);
//     res
//       .status(500)
//       .json({ error: 'Failed to list images', details: error.message });
//   }
// }
