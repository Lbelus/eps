"use client";
import React, { useState, useRef, useEffect } from "react";
import { useSession } from "next-auth/react";
import OrderTable from "@/components/UI/tables/orderTable";
import LoadingBar from "@/components/UI/elements/LoadingBar"; // Import the LoadingBar component
import Papa from "papaparse";
import { STUDENT } from "@/types/students";
import { useRouter } from "next/router";

const emptyStudent = (): STUDENT => ({
  firstname: "",
  lastname: "",
  studentId: "",
  document: "",
  file: undefined,
  link: "",
  cid: "",
  transactionId: "",
  publicDocRef: function (publicDocRef: any): void {
    throw new Error("Function not implemented.");
  }
});

const PurchaseOrder: React.FC = () => {
  const router = useRouter();
  const { data: session, status } = useSession();

  const [file, setFile] = useState("");
  const [parsedData, setParsedData] = useState<STUDENT[]>([]); // State to hold the parsed data
  const inputFile = useRef<HTMLInputElement | null>(null);
  const [collectionId, setCollectionId] = useState("");
  const [loadingProgress, setLoadingProgress] = useState(0); // Loading progress state
  const [isLoading, setIsLoading] = useState(false); // Loading state

  if (status === "loading") {
    return <p>Loading...</p>;
  }
  if (!session) {
    typeof window !== 'undefined' && router.replace('/api/auth/signin');
    return <p>Redirecting...</p>;
  }
  const handleFileChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const selectedFile = e.target.files?.[0];
    if (selectedFile) {
      setFile(selectedFile.name);
      parseCsvFile(selectedFile);
    }
  };

  const parseCsvFile = (file: File) => {
    Papa.parse(file, {
      header: true,
      skipEmptyLines: true,
      complete: (result) => {
        const data = result.data as STUDENT[];
        setParsedData(data);
      },
      error: (error) => {
        console.error("Error parsing CSV file:", error);
      },
    });
  };

  const handleUpdateStudent = (index: number, field: keyof STUDENT, value: string) => {
    const updatedData = [...parsedData];
    updatedData[index] = { ...updatedData[index], [field]: value };
    setParsedData(updatedData);
  };

  const handleAddRow = () => {
    setParsedData((prevData) => [...prevData, emptyStudent()]);
  };

  const handleDeleteRow = (index: number) => {
    setParsedData((prevData) => prevData.filter((_, i) => i !== index));
  };

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

  const handleSubmit = async (event:any) => {
    event.preventDefault();
    setIsLoading(true); // Start loading
    setLoadingProgress(0); // Reset loading progress
    
    // Upload each file and collect their URLs
    const fileUploadPromises = parsedData.map(async (student, index) => {
        if (student.file) {
            const formData = new FormData();
            formData.append('file', student.file);

            try {
                const response = await fetch('/api/createImg', {
                    method: 'POST',
                    body: formData,
                });

                if (!response.ok) {
                    throw new Error('Failed to upload file');
                }

                const fileData = await response.json();
                const imageUrl = fileData.url;

                console.log("Image Uploaded, URL:", imageUrl);
                // Return modified student object with the URL
                setLoadingProgress((prevProgress) => prevProgress + (100 / parsedData.length));
                return { ...student, imageUrl: imageUrl };
            } catch (error) {
                console.error("Error during the file upload for student:", student.firstname, error);
                alert(`Failed to upload file for ${student.firstname}`);
                return student; // Return unmodified student if the upload fails
            }
        } else {
            setLoadingProgress((prevProgress) => prevProgress + (100 / parsedData.length));
            return student; // Return unmodified student if no file to upload
        }
    });

    let updatedStudents;
    try {
        // Wait for all file uploads to complete
        updatedStudents = await Promise.all(fileUploadPromises);
    } catch (error) {
        console.error("Error during file uploads:", error);
        alert("Error during file uploads");
        setIsLoading(false); // Stop loading in case of error
        return; // Stop further execution in case of error
    }

    // Prepare the data to upload, excluding the file objects and including any new image URLs
    const dataToUpload = updatedStudents.map(student => ({
        firstname: student.firstname,
        lastname: student.lastname,
        studentId: student.studentId,
        DocumentDesignation: student.document,
        imageUrl: student.imageUrl || null, // Use the new image URL if available
    }));

    try {
        const response = await fetch("/api/createOrder", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({
                docRef: collectionId,
                data: dataToUpload,
            }),
        });

        if (response.ok) {
            const data = await response.json();
            alert("Data updated successfully");
            console.log(data);
        } else {
            throw new Error("Failed to update order data");
        }
    } catch (error) {
        console.error("Failed to update order data:", error);
        alert("Failed to update order data");
    } finally {
        setIsLoading(false); // Stop loading
    }
};
  return (
    <>
      <div className="col-span-5 xl:col-span-2">
        <div className="rounded-sm border border-stroke bg-white shadow-default dark:border-strokedark dark:bg-boxdark">
          <div className="border-b border-stroke px-7 py-4 dark:border-strokedark">
            <h3 className="font-medium text-black dark:text-white">Upload Your CSV File</h3>
          </div>
          <div className="p-7">
            <form>
              <div
                id="FileUpload"
                className="relative mb-5.5 block w-full cursor-pointer appearance-none rounded border border-dashed border-primary bg-gray px-4 py-4 dark:bg-meta-4 sm:py-7.5"
              >
                <input
                  type="file"
                  ref={inputFile}
                  onChange={handleFileChange}
                  accept=".csv"
                  className="absolute inset-0 z-50 m-0 h-full w-full cursor-pointer p-0 opacity-0 outline-none"
                />
                <div className="flex flex-col items-center justify-center space-y-3">
                  <span className="flex h-10 w-10 items-center justify-center rounded-full border border-stroke bg-white dark:border-strokedark dark:bg-boxdark">
                    <svg
                      width="16"
                      height="16"
                      viewBox="0 0 16 16"
                      fill="none"
                      xmlns="http://www.w3.org/2000/svg"
                    >
                      <path
                        fillRule="evenodd"
                        clipRule="evenodd"
                        d="M1.99967 9.33337C2.36786 9.33337 2.66634 9.63185 2.66634 10V12.6667C2.66634 12.8435 2.73658 13.0131 2.8616 13.1381C2.98663 13.2631 3.1562 13.3334 3.33301 13.3334H12.6663C12.8431 13.3334 13.0127 13.2631 13.1377 13.1381C13.2628 13.0131 13.333 12.8435 13.333 12.6667V10C13.333 9.63185 13.6315 9.33337 13.9997 9.33337C14.3679 9.33337 14.6663 9.63185 14.6663 10V12.6667C14.6663 13.1971 14.4556 13.7058 14.0806 14.0809C13.7055 14.456 13.1968 14.6667 12.6663 14.6667H3.33301C2.80257 14.6667 2.29387 14.456 1.91879 14.0809C1.54372 13.7058 1.33301 13.1971 1.33301 12.6667V10C1.33301 9.63185 1.63148 9.33337 1.99967 9.33337Z"
                        fill="#3C50E0"
                      />
                      <path
                        fillRule="evenodd"
                        clipRule="evenodd"
                        d="M7.5286 1.52864C7.78894 1.26829 8.21106 1.26829 8.4714 1.52864L11.8047 4.86197C12.0651 5.12232 12.0651 5.54443 11.8047 5.80478C11.5444 6.06513 11.1223 6.06513 10.8619 5.80478L8 2.94285L5.13807 5.80478C4.87772 6.06513 4.45561 6.06513 4.19526 5.80478C3.93491 5.54443 3.93491 5.12232 4.19526 4.86197L7.5286 1.52864Z"
                        fill="#3C50E0"
                      />
                      <path
                        fillRule="evenodd"
                        clipRule="evenodd"
                        d="M7.99967 1.33337C8.36786 1.33337 8.66634 1.63185 8.66634 2.00004V10C8.66634 10.3682 8.36786 10.6667 7.99967 10.6667C7.63148 10.6667 7.33301 10.3682 7.33301 10V2.00004C7.33301 1.63185 7.63148 1.33337 7.99967 1.33337Z"
                        fill="#3C50E0"
                      />
                    </svg>
                  </span>
                  <p>
                    <span className="text-primary">{file ? file : "Click to upload"}</span>
                    {file ? "" : " or drag and drop"}
                  </p>
                  <p className="mt-1.5">CSV format</p>
                </div>
              </div>

              <div className="flex justify-end gap-4.5">
                <button
                  className="flex justify-center rounded border border-stroke px-6 py-2 font-medium text-black hover:shadow-1 dark:border-strokedark dark:text-white"
                  type="button"
                  onClick={() => {
                    if (inputFile.current) inputFile.current.value = "";
                    setFile("");
                    setParsedData([]);
                  }}
                >
                  Clear
                </button>
              </div>
            </form>
          </div>
        </div>
      </div>
      <OrderTable
        studentData={parsedData}
        onUpdateStudent={handleUpdateStudent}
        onAddRow={handleAddRow}
        onDeleteRow={handleDeleteRow}
      />
      {isLoading && <LoadingBar progress={loadingProgress} />}
      <div className="mt-6">
        <button
          className="flex justify-center rounded bg-primary px-6 py-2 font-medium text-white hover:bg-opacity-90"
          onClick={handleSubmit}
        >
          Submit Data
        </button>
      </div>
    </>
  );
};

export default PurchaseOrder;
