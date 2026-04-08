/** @type {import('next-sitemap').IConfig} */
module.exports = {
  siteUrl: 'http://localhost:3000',
  generateRobotsTxt: true,
  robotsTxtOptions: {
    policies: [
      {
        userAgent: '*',
        disallow: [
          '/apiTest',
        ],
      },
      {
        userAgent: '*',
        allow: '/',
      },
    ],
  },
};