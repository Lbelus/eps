\#!/usr/bin/env bash

set -euo pipefail



\###############################################################################

\# private Matrix stack helper

\#

\# Usage:

\#   ./setup\_matrix\_stack.sh up

\#   ./setup\_matrix\_stack.sh synapse-gen

\#   ./setup\_matrix\_stack.sh create-users

\#   ./setup\_matrix\_stack.sh status

\#   ./setup\_matrix\_stack.sh tailscale-serve

\###############################################################################



PROJECT\_DIR="${PWD}"

COMPOSE\_FILE="${PROJECT\_DIR}/docker-compose.yml"

SYNAPSE\_DATA\_DIR="${PROJECT\_DIR}/synapse-data"

ELEMENT\_DIR="${PROJECT\_DIR}/element-web"



ACTION="${1:-up}"



DOCKER="docker"



usage() {

&nbsp; cat <<EOF

Usage: $0 <command>



Commands:

&nbsp; up              Generate missing config, write compose/config, and start stack

&nbsp; synapse-gen     Generate Synapse homeserver config into ./synapse-data

&nbsp; create-users    Create Synapse admin user

&nbsp; status          Show status of synapse / element-web containers

&nbsp; tailscale-serve Expose Element privately through Tailscale Serve



Environment:

&nbsp; SYNAPSE\_SERVER\_NAME   Optional server name, default: matrix.local

&nbsp; ELEMENT\_BASE\_URL      Optional Element homeserver URL, default: https://matrix

&nbsp; TAILSCALE\_PORT        Optional Tailscale HTTPS port, default: 443



Notes:

&nbsp; - Run this script as your normal user.

&nbsp; - It uses 'docker' if available, otherwise falls back to 'sudo docker'.

&nbsp; - Tailscale must already be installed and authenticated for tailscale-serve.

EOF

}



ensure\_prereqs() {

&nbsp; if ! command -v docker >/dev/null 2>\&1; then

&nbsp;   echo "ERROR: docker is not installed or not in PATH." >\&2

&nbsp;   exit 1

&nbsp; fi



&nbsp; if docker info >/dev/null 2>\&1; then

&nbsp;   DOCKER="docker"

&nbsp; else

&nbsp;   if command -v sudo >/dev/null 2>\&1; then

&nbsp;     echo ">> 'docker info' failed, trying 'sudo docker info'..."

&nbsp;     if sudo docker info >/dev/null 2>\&1; then

&nbsp;       DOCKER="sudo docker"

&nbsp;     else

&nbsp;       echo "ERROR: Cannot talk to docker daemon, even with sudo." >\&2

&nbsp;       exit 1

&nbsp;     fi

&nbsp;   else

&nbsp;     echo "ERROR: docker requires root, but 'sudo' is not available." >\&2

&nbsp;     exit 1

&nbsp;   fi

&nbsp; fi



&nbsp; if ! ${DOCKER} compose version >/dev/null 2>\&1; then

&nbsp;   echo "ERROR: '${DOCKER} compose' (v2) is not available." >\&2

&nbsp;   echo "       On Ubuntu: sudo apt install docker-compose-plugin" >\&2

&nbsp;   exit 1

&nbsp; fi

}



ensure\_tailscale() {

&nbsp; if ! command -v tailscale >/dev/null 2>\&1; then

&nbsp;   echo "ERROR: tailscale is not installed." >\&2

&nbsp;   echo "Install it first, then run:"

&nbsp;   echo "  sudo tailscale up"

&nbsp;   exit 1

&nbsp; fi



&nbsp; if ! tailscale status >/dev/null 2>\&1; then

&nbsp;   echo "ERROR: tailscale is installed but not connected." >\&2

&nbsp;   echo "Run:"

&nbsp;   echo "  sudo tailscale up"

&nbsp;   exit 1

&nbsp; fi

}



synapse\_gen() {

&nbsp; ensure\_prereqs



&nbsp; local server\_name="${SYNAPSE\_SERVER\_NAME:-matrix.local}"



&nbsp; echo ">> Generating Synapse config..."

&nbsp; echo "   Server name: ${server\_name}"

&nbsp; echo "   Output dir : ${SYNAPSE\_DATA\_DIR}"



&nbsp; mkdir -p "${SYNAPSE\_DATA\_DIR}"



&nbsp; ${DOCKER} run -it --rm \\

&nbsp;   -v "${SYNAPSE\_DATA\_DIR}:/data" \\

&nbsp;   -e SYNAPSE\_SERVER\_NAME="${server\_name}" \\

&nbsp;   -e SYNAPSE\_REPORT\_STATS=no \\

&nbsp;   matrixdotorg/synapse:latest generate



&nbsp; echo

&nbsp; echo ">> Synapse config generated under ${SYNAPSE\_DATA\_DIR}."

&nbsp; echo "   homeserver.yaml should now exist there."

}



patch\_synapse\_config() {

&nbsp; local hs="${SYNAPSE\_DATA\_DIR}/homeserver.yaml"



&nbsp; if \[\[ ! -f "${hs}" ]]; then

&nbsp;   echo "ERROR: ${hs} not found." >\&2

&nbsp;   exit 1

&nbsp; fi



&nbsp; python3 - <<PY

from pathlib import Path

import re



path = Path(r"${hs}")

text = path.read\_text()



def replace\_or\_append(pattern, replacement):

&nbsp;   global text

&nbsp;   new\_text, n = re.subn(pattern, replacement, text, flags=re.MULTILINE)

&nbsp;   if n == 0:

&nbsp;       if not new\_text.endswith("\\n"):

&nbsp;           new\_text += "\\n"

&nbsp;       new\_text += replacement + "\\n"

&nbsp;   text = new\_text



replace\_or\_append(r'^enable\_registration:.\*$', 'enable\_registration: false')

replace\_or\_append(r'^report\_stats:.\*$', 'report\_stats: false')

replace\_or\_append(r'^public\_baseurl:.\*$', 'public\_baseurl: "${ELEMENT\_BASE\_URL:-https://matrix}/"')



\# Try to ensure Synapse respects reverse-proxy headers if the key exists.

text = re.sub(r'(\\bx\_forwarded:\\s\*)false\\b', r'\\1true', text)



path.write\_text(text)

PY

}



write\_element\_config() {

&nbsp; local server\_name="${SYNAPSE\_SERVER\_NAME:-matrix.local}"

&nbsp; local base\_url="${ELEMENT\_BASE\_URL:-https://matrix}"



&nbsp; mkdir -p "${ELEMENT\_DIR}"



&nbsp; cat > "${ELEMENT\_DIR}/config.json" <<EOF

{

&nbsp; "default\_server\_config": {

&nbsp;   "m.homeserver": {

&nbsp;     "base\_url": "${base\_url}",

&nbsp;     "server\_name": "${server\_name}"

&nbsp;   }

&nbsp; },

&nbsp; "disable\_custom\_urls": true,

&nbsp; "disable\_guests": true,

&nbsp; "brand": "Private Matrix"

}

EOF

}



write\_compose() {

&nbsp; cat > "${COMPOSE\_FILE}" <<"EOF"

services:

&nbsp; synapse:

&nbsp;   image: matrixdotorg/synapse:latest

&nbsp;   container\_name: synapse

&nbsp;   restart: unless-stopped

&nbsp;   environment:

&nbsp;     SYNAPSE\_CONFIG\_PATH: /data/homeserver.yaml

&nbsp;   volumes:

&nbsp;     - ./synapse-data:/data

&nbsp;   ports:

&nbsp;     - "127.0.0.1:8008:8008"



&nbsp; element-web:

&nbsp;   image: vectorim/element-web:latest

&nbsp;   container\_name: element-web

&nbsp;   restart: unless-stopped

&nbsp;   depends\_on:

&nbsp;     - synapse

&nbsp;   volumes:

&nbsp;     - ./element-web/config.json:/app/config.json:ro

&nbsp;   ports:

&nbsp;     - "127.0.0.1:8080:80"

&nbsp;   environment:

&nbsp;     - TZ=Europe/Paris

EOF

}



do\_up() {

&nbsp; ensure\_prereqs



&nbsp; echo ">> Project dir: ${PROJECT\_DIR}"



&nbsp; mkdir -p "${SYNAPSE\_DATA\_DIR}" "${ELEMENT\_DIR}"



&nbsp; if \[\[ ! -f "${SYNAPSE\_DATA\_DIR}/homeserver.yaml" ]]; then

&nbsp;   echo ">> Synapse config missing, generating it now..."

&nbsp;   synapse\_gen

&nbsp; fi



&nbsp; echo ">> Patching Synapse config..."

&nbsp; patch\_synapse\_config



&nbsp; echo ">> Writing Element config..."

&nbsp; write\_element\_config



&nbsp; if \[\[ -f "${COMPOSE\_FILE}" ]]; then

&nbsp;   echo ">> Backing up existing docker-compose.yml to docker-compose.yml.bak"

&nbsp;   cp "${COMPOSE\_FILE}" "${COMPOSE\_FILE}.bak"

&nbsp; fi



&nbsp; echo ">> Writing docker-compose.yml..."

&nbsp; write\_compose



&nbsp; echo ">> docker-compose.yml:"

&nbsp; sed -n '1,120p' "${COMPOSE\_FILE}" || true

&nbsp; echo



&nbsp; echo ">> Starting stack: ${DOCKER} compose up -d"

&nbsp; ${DOCKER} compose up -d



&nbsp; echo

&nbsp; do\_status



&nbsp; cat <<'MSG'



===============================================================================

Stack started.



Next steps:



1\. Create your admin user:

&nbsp;    ./setup\_matrix\_stack.sh create-users



2\. Expose Element privately to your tailnet:

&nbsp;    ./setup\_matrix\_stack.sh tailscale-serve



3\. Then open Element through your Tailscale Serve URL instead of localhost.

===============================================================================

MSG

}



create\_users() {

&nbsp; ensure\_prereqs



&nbsp; if \[\[ ! -f "${SYNAPSE\_DATA\_DIR}/homeserver.yaml" ]]; then

&nbsp;   echo "ERROR: ${SYNAPSE\_DATA\_DIR}/homeserver.yaml not found."

&nbsp;   echo "Run: ./setup\_matrix\_stack.sh synapse-gen"

&nbsp;   exit 1

&nbsp; fi



&nbsp; echo ">> Ensuring Synapse is running..."

&nbsp; ${DOCKER} compose up -d synapse



&nbsp; echo

&nbsp; echo ">> Creating Matrix admin user via register\_new\_matrix\_user."

&nbsp; echo



&nbsp; read -rp "Admin username \[admin]: " ADMIN\_USER

&nbsp; ADMIN\_USER="${ADMIN\_USER:-admin}"



&nbsp; read -s -rp "Admin password: " ADMIN\_PASS

&nbsp; echo

&nbsp; read -s -rp "Confirm admin password: " ADMIN\_PASS2

&nbsp; echo



&nbsp; if \[\[ "${ADMIN\_PASS}" != "${ADMIN\_PASS2}" ]]; then

&nbsp;   echo "ERROR: Admin passwords do not match." >\&2

&nbsp;   exit 1

&nbsp; fi



&nbsp; echo ">> Creating admin user '${ADMIN\_USER}' (if not existing)..."

&nbsp; ${DOCKER} exec -it synapse register\_new\_matrix\_user \\

&nbsp;   -c /data/homeserver.yaml \\

&nbsp;   -u "${ADMIN\_USER}" \\

&nbsp;   -p "${ADMIN\_PASS}" \\

&nbsp;   -a \\

&nbsp;   --exists-ok \\

&nbsp;   http://localhost:8008



&nbsp; cat <<EOF



===============================================================================

Matrix admin created / verified:



&nbsp; - Admin: ${ADMIN\_USER}

&nbsp; - Full ID: @${ADMIN\_USER}:${SYNAPSE\_SERVER\_NAME:-matrix.local}



Next:

&nbsp; - Open Element through your Tailscale URL

&nbsp; - Log in as @${ADMIN\_USER}:${SYNAPSE\_SERVER\_NAME:-matrix.local}

&nbsp; - Create rooms and invite your selected users

===============================================================================

EOF

}



do\_status() {

&nbsp; ensure\_prereqs

&nbsp; echo ">> Container status:"

&nbsp; ${DOCKER} ps --format 'table {{.Names}}\\t{{.Status}}\\t{{.Ports}}' \\

&nbsp;   --filter "name=synapse" \\

&nbsp;   --filter "name=element-web" || true

}



tailscale\_serve() {

&nbsp; ensure\_tailscale



&nbsp; local port="${TAILSCALE\_PORT:-443}"



&nbsp; echo ">> Enabling Tailscale Serve on port ${port} -> 127.0.0.1:8080"

&nbsp; echo ">> If HTTPS certificates are not enabled for your tailnet, Tailscale will tell you."



&nbsp; sudo tailscale serve --bg --https="${port}" http://127.0.0.1:8080



&nbsp; echo

&nbsp; echo ">> Current Tailscale Serve status:"

&nbsp; tailscale serve status || true

}



case "${ACTION}" in

&nbsp; up)

&nbsp;   do\_up

&nbsp;   ;;

&nbsp; synapse-gen|gen-synapse)

&nbsp;   synapse\_gen

&nbsp;   ;;

&nbsp; create-users)

&nbsp;   create\_users

&nbsp;   ;;

&nbsp; status)

&nbsp;   do\_status

&nbsp;   ;;

&nbsp; tailscale-serve)

&nbsp;   tailscale\_serve

&nbsp;   ;;

&nbsp; \*)

&nbsp;   usage

&nbsp;   exit 1

&nbsp;   ;;

esac

