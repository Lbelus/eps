/** @type {import('next-sitemap').IConfig} */
const siteUrl = process.env.NEXT_PUBLIC_SITE_URL || 'http://localhost:3000';

module.exports = {
  siteUrl,
  generateRobotsTxt: true,
  robotsTxtOptions: {
    policies: [
      {
        userAgent: '*',
        allow: [
          '/',
          '/llms.txt',
        ],
        disallow: [
          '/api/',
          '/apiTest',
        ],
      },
    ],
  },
};
