import { useRouter } from 'next/router';
import React, { useState, useEffect } from 'react';
import { useSession } from "next-auth/react"
import Link from "next/link";

const UpdateUserInfo = () => {
  const router = useRouter();
  const { data: session, status } = useSession();
  const [collectionId, setcollectionId] = useState('');
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
          setcollectionId(data.id)  // Store the fetched data in state
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

  const handleSubmit = async (event:any) => {
    event.preventDefault();
    const userData = {
      collectionId,
      companyName,
      phoneNumber,
      emailAddress,
      address,
      city,
      state,
      postalCode,
      country,
      username,
      bio
    };

    fetch('/api/updateUserDocData', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        docRef: userData.collectionId,
        data: userData
      })
    })
    .then(response => response.json())
    .then(data => {
      alert("Data updated successfully");
      console.log(data);
    })
    .catch(error => {
      console.error("Failed to update data:", error);
      alert("Failed to update data");
    });
  };
  return (

        <div className="grid grid-cols-5 gap-8">
          <div className="col-span-5 xl:col-span-3">
            <div className="rounded-sm border border-stroke  ">
              <div className="border-b border-stroke px-7 py-4 ">
                <h2 className="text-2xl font-bold mb-6 text-center">
                  Update Personal Information
                </h2>
              </div>
              <div className="p-7">
                <form onSubmit={handleSubmit}>
                  <div className="mb-5.5 flex flex-col gap-5.5 sm:flex-row">
                    <div className="w-full sm:w-1/2">
                      <label
                        className="mb-3 block text-sm font-medium"
                        htmlFor="fullName"
                      >
                        Company Name
                      </label>
                      <div className="relative">
                          <input
                          type="text"
                          name="companyName"
                          id="companyName"
                          value={companyName}
                          onChange={e => setCompanyName(e.target.value)}
                          className="gray w-full rounded border border-stroke "
                          placeholder="My company name"
                        />
                      </div>
                    </div>

                    <div className="w-full sm:w-1/2">
                      <label
                        className="mb-3 block text-sm font-medium"
                        htmlFor="phoneNumber"
                      >
                        Phone Number
                      </label>
                      <input
                        type="text"
                        name="phoneNumber"
                        id="phoneNumber"
                        value={phoneNumber}
                        onChange={e => setPhoneNumber(e.target.value)}
                        className="gray w-full rounded border border-stroke "
                        placeholder="+333 333 333"
                      />
                    </div>
                  </div>

                  <div className="mb-5.5">
                    <label
                      className="mb-3 block text-sm font-medium"
                      htmlFor="emailAddress"
                    >
                      Email Address
                    </label>
                    <div className="relative">
                      <input
                        type="text"
                        name="emailAddress"
                        id="emailAddress"
                        value={emailAddress}
                        onChange={e => setEmailAddress(e.target.value)}
                        className="gray w-full rounded border border-stroke "
                        placeholder=""
                      />
                    </div>
                  </div>

                  <div className="mb-5.5">
                    <label
                      className="mb-3 block text-sm font-medium"
                      htmlFor="Username"
                    >
                      Username
                    </label>
                    <input
                      type="text"
                      name="username"
                      id="username"
                      value={username}
                      onChange={e => setUsername(e.target.value)}
                      className="gray w-full rounded border border-stroke "
                      placeholder="My username"
                    />
                  </div>

                  <div className="mb-5.5">
                    <label
                      className="mb-3 block text-sm font-medium"
                      htmlFor="Address"
                    >
                      Address
                    </label>
                    <input
                      type="text"
                      name="address"
                      id="address"
                      value={address}
                      onChange={e => setAddress(e.target.value)}
                      className="gray w-full rounded border border-stroke "
                      placeholder="My Address"
                    />
                  </div>

                  <div className="mb-5.5">
                    <label
                      className="mb-3 block text-sm font-medium"
                      htmlFor="City"
                    >
                      City
                    </label>
                    <input
                      type="text"
                      name="city"
                      id="city"
                      value={city}
                      onChange={e => setCity(e.target.value)}
                      className="gray w-full rounded border border-stroke "
                      placeholder="My Address"
                    />
                  </div>

                  <div className="mb-5.5">
                    <label
                      className="mb-3 block text-sm font-medium"
                      htmlFor="PostalCode"
                    >
                      Postal code
                    </label>
                    <input
                      type="text"
                      name="postalCode"
                      id="postalCode"
                      value={postalCode}
                      onChange={e => setPostalCode(e.target.value)}
                      className="gray w-full rounded border border-stroke "
                      placeholder="My postalCode"
                    />
                  </div>

                  <div className="mb-5.5">
                    <label
                      className="mb-3 block text-sm font-medium"
                      htmlFor="State"
                    >
                      State
                    </label>
                    <input
                      type="text"
                      name="state"
                      id="state"
                      value={state}
                      onChange={e => setState(e.target.value)}
                      className="gray w-full rounded border border-stroke "
                      placeholder="My state"
                    />
                  </div>

                  <div className="mb-5.5">
                    <label
                      className="mb-3 block text-sm font-medium"
                      htmlFor="Country"
                    >
                      Country
                    </label>
                    <input
                      type="text"
                      name="country"
                      id="country"
                      value={country}
                      onChange={e => setCountry(e.target.value)}
                      className="gray w-full rounded border border-stroke "
                      placeholder="My country"
                    />
                  </div>

                  <div className="mb-5.5">
                    <label
                      className="mb-3 block text-sm font-medium"
                      htmlFor="Username"
                    >
                      BIO
                    </label>
                    <div className="relative">
                      <textarea
                        className="gray w-full rounded border border-stroke "
                        name="bio"
                        id="bio"
                        rows={6}
                        value={bio}
                        onChange={e => setBio(e.target.value)}
                        placeholder="Write your bio here"
                      ></textarea>
                    </div>
                  </div>

                  <div className="flex justify-end gap-4.5">
                      <button type="submit" className="flex justify-center rounded border border-stroke px-6 py-2 font-medium hover:shadow-1">
                        Save
                      </button>
                  </div>
                </form>
              </div>
            </div>
          </div>
          <section className="mb-8">
          <Link href="/private">
            <div className="flex items-center space-x-2 text-2xl font-medium text-indigo-500 dark:text-gray-100 hover:underline">
              Go to private page
            </div>
          </Link>
        </section>
        </div>
        
  );
};

export default UpdateUserInfo;