#!/bin/bash

# Prompt for user input
read -p "Enter the server's IP address: " SERVER_IP
read -p "Enter the full path for certificate storage (e.g., /home/user/rest_api/ssl): " CERT_PATH

# Create necessary directories
mkdir -p "$CERT_PATH"
cd "$CERT_PATH"

# Create the server private key and certificate
openssl genrsa -out ca.key 2048
openssl req -x509 -new -nodes -key ca.key -sha256 -days 365 -out ca.crt \
    -subj "/C=US/ST=State/L=City/O=Organization/CN=MyCA"

openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr -config <(cat <<-EOF
[req]
distinguished_name = req_distinguished_name
[req_distinguished_name]
CN = $SERVER_IP
[req_ext]
subjectAltName = @alt_names
[alt_names]
IP.1 = $SERVER_IP
EOF
)
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
    -out server.crt -days 365 -sha256 -extfile <(cat <<-EOF
subjectAltName = IP:$SERVER_IP
EOF
)

# Create the client certificate
openssl genrsa -out client.key 2048
openssl req -new -key client.key -out client.csr \
    -subj "/C=US/ST=State/L=City/O=Organization/CN=Client"
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
    -out client.crt -days 365 -sha256

# Create the .p12 file for the client
openssl pkcs12 -export -out client.p12 -inkey client.key -in client.crt

# Set up UFW rules
# sudo ufw default deny incoming
# sudo ufw default allow outgoing
# sudo ufw allow 443/tcp
# sudo ufw allow 80/tcp
# sudo ufw allow 22/tcp
# sudo ufw reload
# sudo ufw status verbose

# Create the Nginx configuration file
NGINX_CONF="/etc/nginx/sites-available/sql_workbench"
if [ ! -f "$NGINX_CONF" ]; then
    sudo tee "$NGINX_CONF" > /dev/null <<EOF
server {
    listen 443 ssl;
    server_name $SERVER_IP;

    ssl_certificate $CERT_PATH/server.crt;
    ssl_certificate_key $CERT_PATH/server.key;
    ssl_client_certificate $CERT_PATH/ca.crt;
    ssl_verify_client on;

    location / {
        proxy_pass http://127.0.0.1:3000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade \$http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host \$host;
        proxy_set_header X-Real-IP \$remote_addr;
        proxy_set_header X-Forwarded-For \$proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto \$scheme;
        proxy_read_timeout 86400s;
        proxy_send_timeout 86400s;
    }

    location /vnc/ {
        proxy_pass http://127.0.0.1:3000/vnc/;
        proxy_http_version 1.1;
        proxy_set_header Upgrade \$http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host \$host;
        proxy_read_timeout 86400s;
        proxy_send_timeout 86400s;
    }
}
EOF

    # Enable the Nginx configuration
    sudo ln -s /etc/nginx/sites-available/sql_workbench /etc/nginx/sites-enabled/
    sudo rm -f /etc/nginx/sites-enabled/default
fi

# Adjust permissions for certificate files
sudo chmod 644 $CERT_PATH/*.crt $CERT_PATH/*.key

# Test and reload Nginx
sudo nginx -t && sudo nginx -s reload

# Start the mysql_workbench container
docker run -d \
  --name=mysql-workbench \
  --cap-add=IPC_LOCK \
  -e PUID=1000 \
  -e PGID=1000 \
  -e TZ=Etc/UTC \
  -p 3000:3000 \
  -p 3001:3001 \
  -v /path/to/config:/config \
  --shm-size="1gb" \
  --restart unless-stopped \
  --network sqlRest \
  lscr.io/linuxserver/mysql-workbench:latest

# Output completion message
echo "Setup complete! Access your browser at: https://$SERVER_IP"
echo "Download the client certificate (client.p12) for your device from: $CERT_PATH"
