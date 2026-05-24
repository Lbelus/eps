import type { NextApiRequest, NextApiResponse } from "next";

type RuntimeConfig = {
  restApiUrl: string;
};

export default function handler(_request: NextApiRequest, response: NextApiResponse<RuntimeConfig>) {
  response.setHeader("Cache-Control", "no-store");
  response.status(200).json({
    restApiUrl: process.env.NEXT_PUBLIC_REST_API_URL?.replace(/[/]$/, "") || "",
  });
}
