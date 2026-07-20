#!/usr/bin/env node

import { createInterface } from "node:readline";

const knownBotPatterns = [
  /bot/i,
  /crawler/i,
  /spider/i,
  /googlebot/i,
  /bingbot/i,
  /gptbot/i,
  /claudebot/i,
  /perplexitybot/i,
  /ahrefsbot/i,
  /semrushbot/i,
  /duckduckbot/i,
  /yandexbot/i,
  /baiduspider/i,
  /facebookexternalhit/i,
  /slurp/i,
];

const browserPatterns = [
  /chrome/i,
  /firefox/i,
  /safari/i,
  /edg\//i,
  /opr\//i,
  /mobile/i,
];

const totals = {
  human_browser: 0,
  known_bot: 0,
  likely_bot: 0,
  unknown: 0,
};

const byPath = new Map();

const pick = (value, keys) => {
  for (const key of keys) {
    if (value && typeof value === "object" && value[key] !== undefined && value[key] !== null) {
      return value[key];
    }
  }
  return "";
};

const getNested = (value, path) =>
  path.reduce((current, key) => (current && typeof current === "object" ? current[key] : undefined), value);

const getRequestPath = (entry) => {
  const nestedPath =
    getNested(entry, ["request", "path"]) ||
    getNested(entry, ["request", "url"]) ||
    getNested(entry, ["proxy", "path"]);
  const path = nestedPath || pick(entry, ["requestPath", "path", "url", "pathname"]);

  if (!path || typeof path !== "string") {
    return "unknown";
  }

  try {
    return new URL(path).pathname || "unknown";
  } catch (_error) {
    return path.split("?")[0] || "unknown";
  }
};

const getStatus = (entry) => {
  const status = pick(entry, ["status", "statusCode"]);
  return Number(status || getNested(entry, ["response", "statusCode"]) || getNested(entry, ["proxy", "statusCode"]));
};

const getUserAgent = (entry) => {
  const direct = pick(entry, ["userAgent", "requestUserAgent", "ua"]);
  if (direct) {
    return String(direct);
  }

  const headers = entry?.headers || entry?.request?.headers || entry?.proxy?.headers || {};
  return String(headers["user-agent"] || headers["User-Agent"] || "");
};

const classify = (entry) => {
  const userAgent = getUserAgent(entry);
  const path = getRequestPath(entry);
  const status = getStatus(entry);

  if (knownBotPatterns.some((pattern) => pattern.test(userAgent))) {
    return "known_bot";
  }

  if (!userAgent || status === 404 || path.includes("wp-admin") || path.includes(".env")) {
    return "likely_bot";
  }

  if (browserPatterns.some((pattern) => pattern.test(userAgent))) {
    return "human_browser";
  }

  return "unknown";
};

const rememberPath = (bucket, path) => {
  if (!byPath.has(path)) {
    byPath.set(path, {
      human_browser: 0,
      known_bot: 0,
      likely_bot: 0,
      unknown: 0,
    });
  }

  byPath.get(path)[bucket] += 1;
};

let parsed = 0;
let skipped = 0;

const input = createInterface({
  input: process.stdin,
  crlfDelay: Infinity,
});

input.on("line", (line) => {
  const trimmed = line.trim();
  if (!trimmed) {
    return;
  }

  try {
    const entry = JSON.parse(trimmed);
    const bucket = classify(entry);
    const path = getRequestPath(entry);
    totals[bucket] += 1;
    rememberPath(bucket, path);
    parsed += 1;
  } catch (_error) {
    skipped += 1;
  }
});

input.on("close", () => {
  const date = new Date().toISOString().slice(0, 10);
  const topPaths = Array.from(byPath.entries())
    .map(([path, counts]) => ({
      path,
      total: counts.human_browser + counts.known_bot + counts.likely_bot + counts.unknown,
      ...counts,
    }))
    .sort((a, b) => b.total - a.total)
    .slice(0, 20);

  console.log(JSON.stringify({ date, parsed, skipped, totals, topPaths }, null, 2));
});
