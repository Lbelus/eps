This is a [Next.js](https://nextjs.org/) project bootstrapped with [`create-next-app`](https://github.com/vercel/next.js/tree/canary/packages/create-next-app).

## Getting Started

First, run the development server:

```bash
npm run dev
# or
yarn dev
# or
pnpm dev
# or
bun dev
```

Open [http://localhost:3000](http://localhost:3000) with your browser to see the result.

You can start editing the page by modifying `pages/index.tsx`. The page auto-updates as you edit the file.

This project uses [`next/font`](https://nextjs.org/docs/basic-features/font-optimization) to automatically optimize and load Inter, a custom Google Font.

## Learn More

To learn more about Next.js, take a look at the following resources:

- [Next.js Documentation](https://nextjs.org/docs) - learn about Next.js features and API.
- [Learn Next.js](https://nextjs.org/learn) - an interactive Next.js tutorial.

You can check out [the Next.js GitHub repository](https://github.com/vercel/next.js/) - your feedback and contributions are welcome!



## Create manage your Docker container

Generate the image;
```bash
sudo docker build -t img_sc .
```
Create the container
```bash
sudo docker run -it -v /PathToMyProject/:/workspace --name cont_surface img_sc
```
The container is created but currently stopped
```bash
sudo docker start -ai cont_sc
```
The container is created but currently running
```bash
sudo docker exec -it cont_sc /bin/bash
```

## Environment Variables

To work that project will require api keys and various variable. 
Create a .env.local file with those variables :
```txt
GOOGLE_ID=
GOOGLE_SECRET=
NEXTAUTH_URL=
SECRET=
FIREBASE_API_KEY=
FIREBASE_AUTH_DOMAIN=
FIREBASE_PROJECT_ID=
FIREBASE_STORAGE_BUCKET=
FIREBASE_MESSAGING_SENDER_ID=
FIREBASE_APP_ID=
FIREBASE_MEASUREMENT_ID=
GOOGLE_APPLICATION_CREDENTIALS=./service-account.json
AUTH_FIREBASE_PRIVATE_KEY=
AUTH_FIREBASE_CLIENT_EMAIL=
AUTH_FIREBASE_PROJECT_ID=
```

## AUTH

Obtain given https address from tunneling and add it to .env file and your google console developer authorized url
```
NEXTAUTH_URL=
```

How to Generate a Strong Secret:
You can generate a strong secret using Node.js's crypto module. Run the following command in your terminal:
bash
```bash
node -e "console.log(require('crypto').randomBytes(32).toString('hex'))"
```

## FIREBASE
go to firebase web console and add your google project.

https://console.firebase.google.com;


follow these steps : https://firebase.google.com/codelabs/firebase-nextjs#2

- Add a web app to your Firebase project.
- Navigate to your Project overview in your Firebase project, and then click ``</>`` Web.
- In the App nickname text box, enter a memorable app nickname, such as My Next.js app.
- Click Register app > Next > Next > Continue to console.
- Store firebase config in .env.local.

### Set up Authentication
In the Firebase console, navigate to Authentication.
- Click Get started.
- In the Additional providers column, click Google > Enable.
- In the Public-facing name for project text box, enter a memorable name, such as My Next.js app.
- From the Support email for project drop-down, select your email address.
- Click Save.
- update .env variables to match firebaseconfig variables with the client id and the client secret coming from the google cloud console.

### Set up Cloud Firestore
In the Firebase console, navigate to cloud Firestore.
- Click Create database > Start in test mode > Next.
- Use the default location or select a location of your choice.
- Later in the project, you'll add Security Rules to secure your data. Do not distribute or expose an app publicly without adding Security Rules for your database.
- For a real app, you want to choose a location that's close to your users. Note that this location cannot be changed later, and it will also automatically be the location of your default Cloud Storage bucket (next step).
- Click Done.

### Set up Cloud Storage for Firebase
In the Firebase console, navigate to Storage.
- Click Get started > Start in test mode > Next.
- Later in this project, you'll add Security Rules to secure your data. Do not distribute or expose an app publicly without adding Security Rules for your Storage bucket.
- The location of your bucket should already be selected (due to setting up Firestore in the previous step).
- Click Done.

### Service account

- Go to project settings navigate to Service Accounts
- Click on generate new private Key
- Download the service-account.json file and save it in root under the name service-account.json


### server identification
Google cloud console does not recognize localhost as a valid https address.
To address ``:)`` that situation, you will use a tunneling service to obtain a valid https address. 

For google cloud services to work you will need to add the web client address in the credentials. 

- Go to Google Cloud Console, under credentials
- Select +create credentials then OAuth client ID. 
- select the type of application, give a name to the credentials and a valid https address
- Authorized redirect URIs add the https://myValidAddress/api/auth/callback/google with the /api/auth/callback/google extension.
- Add the newly create credentials to the .env.local file


## Tunneling services

They are various tunneling services we can rely on : 

### Ngrok:

- Install ngrok
```bash
curl -s https://ngrok-agent.s3.amazonaws.com/ngrok.asc \
	| sudo tee /etc/apt/trusted.gpg.d/ngrok.asc >/dev/null \
	&& echo "deb https://ngrok-agent.s3.amazonaws.com buster main" \
	| sudo tee /etc/apt/sources.list.d/ngrok.list \
	&& sudo apt update \
	&& sudo apt install ngrok
```
- Go to website : ``https://ngrok.com``
- Log in
- Get token and add it to the config
- run ngrok 
```bash
ngrok http 3000
```

### Serveo:

```bash
ssh -R mycustomsite:80:localhost:3000 serveo.net
```

### local tunnel

- Go to website : ``https://localtunnel.github.io/www/``
- Install local tunnel
```bash
npm install -g localtunnel
```
- Run lt
```bash
lt --port 3000
```

## Deploy on Vercel

The easiest way to deploy your Next.js app is to use the [Vercel Platform](https://vercel.com/new?utm_medium=default-template&filter=next.js&utm_source=create-next-app&utm_campaign=create-next-app-readme) from the creators of Next.js.

Check out our [Next.js deployment documentation](https://nextjs.org/docs/deployment) for more details.
