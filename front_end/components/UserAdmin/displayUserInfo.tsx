import { useRouter } from 'next/router';
import React, { useState, useEffect } from 'react';
import { useSession } from "next-auth/react"
import Link from "next/link";

const DisplayUserInfo = ({}) => {
  const router = useRouter();
  const { data: session, status } = useSession();
  const [userData, setUserData] = useState({});
  const [documentId, setDocumentId] = useState('');
  const [companyName, setCompanyName] = useState('');
  const [phoneNumber, setPhoneNumber] = useState('');
  const [emailAddress, setEmailAddress] = useState('');
  const [address, setAddress] = useState('');
  const [city, setCity] = useState('');
  const [state, setState] = useState('');
  const [postalCode, setPostalCode] = useState('');
  const [country, setCountry] = useState('');
  const [username, setUsername] = useState('');
  const [bio, setBio] = useState('');

  if (status === "loading") {
    return <p>Loading...</p>;
  }
  if (!session) {
    typeof window !== 'undefined' && router.replace('/api/auth/signin');
    return <p>Redirecting...</p>;
  }
  useEffect(() => {
    if (session?.user?.email) {
      fetch(`/api/readUserDocData?email=${encodeURIComponent(session.user.email)}`)
        .then(res => res.json())
        .then(data => {
          setUserData(data);
          setDocumentId(data.id)  // Store the fetched data in state
          setCompanyName(data.companyName || '');
          setPhoneNumber(data.phoneNumber || '');
          setEmailAddress(data.email || '');
          setAddress(data.address || '');
          setCity(data.city || '');
          setState(data.state || '');
          setPostalCode(data.postalCode || '');
          setCountry(data.country || '');
          setUsername(data.name || '');
          setBio(data.bio || '');
        })
        .catch(error => {
          console.error('Error fetching user data:', error);
        });
    }
  }, [session?.user?.email]);

  return (
    <>
    <div className="border-b border-stroke px-7 py-4 dark:border-strokedark">
      <h2 className="text-2xl font-bold mb-6 text-center">
        Personal Information
      </h2>
      document Id: {documentId} <br></br>
      username: {username} <br></br>
      Email : {emailAddress} <br></br>
      Company Name: {companyName} <br></br>
      Phone Number: {phoneNumber} <br></br>
      Address : {address} <br></br>
      City: {city} <br></br>
      State: {state} <br></br>
      PostalCode: {postalCode} <br></br>
      Country: {country} <br></br>
      Bio: {bio} <br></br>
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

export default DisplayUserInfo;