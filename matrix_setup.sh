#!/usr/bin/env bash
set -euo pipefail

###############################################################################
# private Matrix stack helper
#
# Usage:
#   ./setup_matrix_stack.sh up
#   ./setup_matrix_stack.sh synapse-gen
#   ./setup_matrix_stack.sh create-users
#   ./setup_matrix_stack.sh status
#   ./setup_matrix_stack.sh nginx-setup
#   ./setup_matrix_stack.sh tailscale-serve
###############################################################################

PROJECT_DIR="${PWD}"
COMPOSE_FILE="${PROJECT_DIR}/docker-compose.yml"
SYNAPSE_DATA_DIR="${PROJECT_DIR}/synapse-data"
ELEMENT_DIR="${PROJECT_DIR}/element-web"

ACTION="${1:-up}"

DOCKER="docker"

usage() {
  cat <<EOF
Usage: $0 <command>

Commands:
  up              Generate missing config, write compose/config, install nginx, and start stack
  synapse-gen     Generate Synapse homeserver config into ./synapse-data
  create-users    Create Synapse admin user
  status          Show status of synapse / element-web containers
  nginx-setup     Install nginx and write local reverse-proxy config
  tailscale-serve Expose nginx privately through Tailscale Serve

Environment:
  SYNAPSE_SERVER_NAME   Optional server name, default: matrix.local
  ELEMENT_BASE_URL      Optional Element homeserver URL, default: https://matrix
  TAILSCALE_PORT        Optional Tailscale HTTPS port, default: 443

Notes:
  - Run this script as your normal user.
  - It uses 'docker' if available, otherwise falls back to 'sudo docker'.
  - Tailscale must already be installed and authenticated for tailscale-serve.
EOF
}

ensure_prereqs() {
  if ! command -v docker >/dev/null 2>&1; then
    echo "ERROR: docker is not installed or not in PATH." >&2
    exit 1
  fi

  if docker info >/dev/null 2>&1; then
    DOCKER="docker"
  else
    if command -v sudo >/dev/null 2>&1; then
      echo ">> 'docker info' failed, trying 'sudo docker info'..."
      if sudo docker info >/dev/null 2>&1; then
        DOCKER="sudo docker"
      else
        echo "ERROR: Cannot talk to docker daemon, even with sudo." >&2
        exit 1
      fi
    else
      echo "ERROR: docker requires root, but 'sudo' is not available." >&2
      exit 1
    fi
  fi

  if ! ${DOCKER} compose version >/dev/null 2>&1; then
    echo "ERROR: '${DOCKER} compose' (v2) is not available." >&2
    echo "       On Ubuntu: sudo apt install docker-compose-plugin" >&2
    exit 1
  fi
}

ensure_tailscale() {
  if ! command -v tailscale >/dev/null 2>&1; then
    echo "ERROR: tailscale is not installed." >&2
    echo "Install it first, then run:"
    echo "  sudo tailscale up"
    exit 1
  fi

  if ! tailscale status >/dev/null 2>&1; then
    echo "ERROR: tailscale is installed but not connected." >&2
    echo "Run:"
    echo "  sudo tailscale up"
    exit 1
  fi
}

synapse_gen() {
  ensure_prereqs

  local server_name="${SYNAPSE_SERVER_NAME:-matrix.local}"

  echo ">> Generating Synapse config..."
  echo "   Server name: ${server_name}"
  echo "   Output dir : ${SYNAPSE_DATA_DIR}"

  mkdir -p "${SYNAPSE_DATA_DIR}"

  ${DOCKER} run -it --rm \
    -v "${SYNAPSE_DATA_DIR}:/data" \
    -e SYNAPSE_SERVER_NAME="${server_name}" \
    -e SYNAPSE_REPORT_STATS=no \
    matrixdotorg/synapse:latest generate

  echo
  echo ">> Synapse config generated under ${SYNAPSE_DATA_DIR}."
  echo "   homeserver.yaml should now exist there."
}

patch_synapse_config() {
  local hs="${SYNAPSE_DATA_DIR}/homeserver.yaml"

  if [[ ! -f "${hs}" ]]; then
    echo "ERROR: ${hs} not found." >&2
    exit 1
  fi

  python3 - <<PY
from pathlib import Path
import re

path = Path(r"${hs}")
text = path.read_text()

def replace_or_append(pattern, replacement):
    global text
    new_text, n = re.subn(pattern, replacement, text, flags=re.MULTILINE)
    if n == 0:
        if not new_text.endswith("\n"):
            new_text += "\n"
        new_text += replacement + "\n"
    text = new_text

replace_or_append(r'^enable_registration:.*$', 'enable_registration: false')
replace_or_append(r'^report_stats:.*$', 'report_stats: false')
replace_or_append(r'^public_baseurl:.*$', 'public_baseurl: "${ELEMENT_BASE_URL:-https://matrix}/"')

text = re.sub(r'(\bx_forwarded:\s*)false\b', r'\1true', text)

path.write_text(text)
PY
}

write_element_config() {
  local server_name="${SYNAPSE_SERVER_NAME:-matrix.local}"
  local base_url="${ELEMENT_BASE_URL:-https://matrix}"

  mkdir -p "${ELEMENT_DIR}"

  cat > "${ELEMENT_DIR}/config.json" <<EOF
{
  "default_server_config": {
    "m.homeserver": {
      "base_url": "${base_url}",
      "server_name": "${server_name}"
    }
  },
  "disable_custom_urls": true,
  "disable_guests": true,
  "brand": "Private Matrix"
}
EOF
}

write_compose() {
  cat > "${COMPOSE_FILE}" <<"EOF"
services:
  synapse:
    image: matrixdotorg/synapse:latest
    container_name: synapse
    restart: unless-stopped
    environment:
      SYNAPSE_CONFIG_PATH: /data/homeserver.yaml
    volumes:
      - ./synapse-data:/data
    ports:
      - "127.0.0.1:8008:8008"

  element-web:
    image: vectorim/element-web:latest
    container_name: element-web
    restart: unless-stopped
    depends_on:
      - synapse
    volumes:
      - ./element-web/config.json:/app/config.json:ro
    ports:
      - "127.0.0.1:8080:80"
    environment:
      - TZ=Europe/Paris
EOF
}

install_nginx() {
  if command -v nginx >/dev/null 2>&1; then
    echo ">> nginx already installed."
    return
  fi

  echo ">> Installing nginx..."
  sudo apt-get update
  sudo apt-get install -y nginx
}

write_nginx_config() {
  local nginx_conf="/etc/nginx/sites-available/matrix-tailnet"

  echo ">> Writing nginx config to ${nginx_conf}..."

  sudo tee "${nginx_conf}" > /dev/null <<'EOF'
server {
    listen 127.0.0.1:8090;
    server_name _;

    client_max_body_size 50M;

    location /_matrix {
        proxy_pass http://127.0.0.1:8008;
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header X-Forwarded-Proto https;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_read_timeout 600s;
        proxy_send_timeout 600s;
    }

    location /_synapse/client {
        proxy_pass http://127.0.0.1:8008;
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header X-Forwarded-Proto https;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_read_timeout 600s;
        proxy_send_timeout 600s;
    }

    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header X-Forwarded-Proto https;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
EOF

  sudo ln -sfn "${nginx_conf}" /etc/nginx/sites-enabled/matrix-tailnet
  sudo rm -f /etc/nginx/sites-enabled/default

  echo ">> Testing nginx config..."
  sudo nginx -t

  echo ">> Restarting nginx..."
  sudo systemctl enable nginx >/dev/null 2>&1 || true
  sudo systemctl restart nginx

  echo ">> Testing local nginx routes..."
  curl -fsS http://127.0.0.1:8090/ > /dev/null
  curl -fsS http://127.0.0.1:8090/_matrix/client/versions > /dev/null

  echo ">> nginx reverse proxy is ready on 127.0.0.1:8090"
}

nginx_setup() {
  install_nginx
  write_nginx_config
}

do_up() {
  ensure_prereqs

  echo ">> Project dir: ${PROJECT_DIR}"

  mkdir -p "${SYNAPSE_DATA_DIR}" "${ELEMENT_DIR}"

  if [[ ! -f "${SYNAPSE_DATA_DIR}/homeserver.yaml" ]]; then
    echo ">> Synapse config missing, generating it now..."
    synapse_gen
  fi

  echo ">> Patching Synapse config..."
  patch_synapse_config

  echo ">> Writing Element config..."
  write_element_config

  if [[ -f "${COMPOSE_FILE}" ]]; then
    echo ">> Backing up existing docker-compose.yml to docker-compose.yml.bak"
    cp "${COMPOSE_FILE}" "${COMPOSE_FILE}.bak"
  fi

  echo ">> Writing docker-compose.yml..."
  write_compose

  echo ">> docker-compose.yml:"
  sed -n '1,120p' "${COMPOSE_FILE}" || true
  echo

  echo ">> Starting stack: ${DOCKER} compose up -d"
  ${DOCKER} compose up -d

  echo ">> Setting up nginx reverse proxy..."
  nginx_setup

  echo
  do_status

  cat <<'MSG'

===============================================================================
Stack started.

Next steps:

1. Create your admin user:
     ./setup_matrix_stack.sh create-users

2. Expose nginx privately to your tailnet:
     ./setup_matrix_stack.sh tailscale-serve

3. Then open Element through your Tailscale Serve URL.
===============================================================================
MSG
}

create_users() {
  ensure_prereqs

  if [[ ! -f "${SYNAPSE_DATA_DIR}/homeserver.yaml" ]]; then
    echo "ERROR: ${SYNAPSE_DATA_DIR}/homeserver.yaml not found."
    echo "Run: ./setup_matrix_stack.sh synapse-gen"
    exit 1
  fi

  echo ">> Ensuring Synapse is running..."
  ${DOCKER} compose up -d synapse

  echo
  echo ">> Creating Matrix admin user via register_new_matrix_user."
  echo

  read -rp "Admin username [admin]: " ADMIN_USER
  ADMIN_USER="${ADMIN_USER:-admin}"

  read -s -rp "Admin password: " ADMIN_PASS
  echo
  read -s -rp "Confirm admin password: " ADMIN_PASS2
  echo

  if [[ "${ADMIN_PASS}" != "${ADMIN_PASS2}" ]]; then
    echo "ERROR: Admin passwords do not match." >&2
    exit 1
  fi

  echo ">> Creating admin user '${ADMIN_USER}' (if not existing)..."
  ${DOCKER} exec -it synapse register_new_matrix_user \
    -c /data/homeserver.yaml \
    -u "${ADMIN_USER}" \
    -p "${ADMIN_PASS}" \
    -a \
    --exists-ok \
    http://localhost:8008

  cat <<EOF

===============================================================================
Matrix admin created / verified:

  - Admin: ${ADMIN_USER}
  - Full ID: @${ADMIN_USER}:${SYNAPSE_SERVER_NAME:-matrix.local}

Next:
  - Open Element through your Tailscale URL
  - Log in as @${ADMIN_USER}:${SYNAPSE_SERVER_NAME:-matrix.local}
  - Create rooms and invite your selected users
===============================================================================
EOF
}

do_status() {
  ensure_prereqs
  echo ">> Container status:"
  ${DOCKER} ps --format 'table {{.Names}}\t{{.Status}}\t{{.Ports}}' \
    --filter "name=synapse" \
    --filter "name=element-web" || true
}

tailscale_serve() {
  ensure_tailscale

  local port="${TAILSCALE_PORT:-443}"

  echo ">> Resetting previous Tailscale Serve config..."
  sudo tailscale serve reset

  echo ">> Enabling Tailscale Serve on port ${port} -> 127.0.0.1:8090"
  sudo tailscale serve --bg --https="${port}" http://127.0.0.1:8090

  echo
  echo ">> Current Tailscale Serve status:"
  tailscale serve status || true
}

case "${ACTION}" in
  up)
    do_up
    ;;
  synapse-gen|gen-synapse)
    synapse_gen
    ;;
  create-users)
    create_users
    ;;
  status)
    do_status
    ;;
  nginx-setup)
    nginx_setup
    ;;
  tailscale-serve)
    tailscale_serve
    ;;
  *)
    usage
    exit 1
    ;;
esac
