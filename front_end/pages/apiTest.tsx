// import { useRouter } from 'next/router';
// import { useSession } from "next-auth/react"
// import DisplayUserInfo from '@/components/UserAdmin/displayUserInfo';
// import UpdateUserInfo from '@/components/UserAdmin/updateUserInfo';
// import UserOrder from '@/components/order/userOrder'
// import DisplayOrder from '@/components/order/displayOrder'
// import RemoveOrder from '@/components/order/removeOrder'
// import UploadImg from '@/components/content/uploadImg'
// import RemoveImg from '@/components/content/removeImg'

// const Settings = ({}) => {
//   const router = useRouter();
//   const { data: session, status } = useSession();
//   if (status === 'loading') {
//     return <p>Loading...</p>; // Show a loading message while checking the session status
//   }
//   if (status === 'unauthenticated') {
//     if (typeof window !== 'undefined') {
//       router.replace('/api/auth/signin');
//     }
//     return <p>Redirecting...</p>;
//   }
//   return (
//   <>
//     <DisplayUserInfo/>
//     <UpdateUserInfo/>
//     <UserOrder/>
//     <DisplayOrder/>
//     <RemoveOrder/>
//     <UploadImg/>
//     <RemoveImg/>
//   </>
//   );
// };

// export default Settings;

