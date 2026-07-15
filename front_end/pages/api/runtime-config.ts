import type { NextApiRequest, NextApiResponse } from "next";

type RuntimeConfig = {
  restApiUrl: string;
};

const normalizeEndpoint = (endpoint: string) => endpoint.trim().replace(/[/]$/, "");

const inferLocalRestApiUrl = (request: NextApiRequest) => {
  const host = request.headers["x-forwarded-host"] || request.headers.host;
  const proto = request.headers["x-forwarded-proto"] || "http";
  const firstHost = Array.isArray(host) ? host[0] : host;
  const firstProto = Array.isArray(proto) ? proto[0] : proto;

  if (!firstHost) {
    return "";
  }

  const hostname = firstHost.split(":")[0];
  return normalizeEndpoint(`${firstProto}://${hostname}:3004`);
};

export default function handler(request: NextApiRequest, response: NextApiResponse<RuntimeConfig>) {
  const configuredRestApiUrl = normalizeEndpoint(process.env.NEXT_PUBLIC_REST_API_URL || "");

  response.setHeader("Cache-Control", "no-store");
  response.status(200).json({
    restApiUrl: configuredRestApiUrl || inferLocalRestApiUrl(request),
  });
}
