#!/usr/bin/env bash

# https://gist.github.com/mohanpedala/1e2ff5661761d3abd0385e8223e16425
set -euo pipefail

IP_PORT="${1:-172.20.0.4:3004}"
MYSQL_CONT="${MYSQL_CONT:-mysqlserver}"
MYSQL_DB="${MYSQL_DB:-test_rest_DB}"
MYSQL_USER="${MYSQL_USER:-dev_admin}"
MYSQL_PASS="${MYSQL_PASS:-dev_admin}"

die()
{ 
    echo "Failure $*" >&2;
    exit 1; 
}

http()
{
  # $1=METHOD $2=URL [$3=JSON]
  local method="$1" url="$2" data="${3:-}"
  local body code
  body="$(mktemp)"
  if [[ "$method" == "GET" || "$method" == "DELETE" ]]; then
    code=$(curl -sS -o "$body" -w '%{http_code}' -X "$method" "http://$IP_PORT$url") || true
  else
    code=$(curl -sS -o "$body" -w '%{http_code}' -X "$method" "http://$IP_PORT$url" \
            -H 'Content-Type: application/json' --data "$data") || true
  fi
  echo "$code:$body"
}

expect_code()
{
  # $1=expected $2=code:body_path [$3=hint]
  local exp="$1" pair="$2" hint="${3:-}"
  local code="${pair%%:*}" bodyf="${pair#*:}"

  if [[ "$code" != "$exp" ]]; then
    echo " Expected $exp, got $code. $hint" >&2
    echo "— Body —" >&2
    cat "$bodyf" >&2
    echo >&2
    exit 1
  fi

  echo "Success: $exp" >&2     # <-- log to stderr
  printf '%s\n' "$bodyf"       # <-- ONLY the path on stdout
}

contains()
{ 
    grep -q "$1" "$2" || die "Body does not contain '$1'";
    echo "Success: contains '$1'";
}


mysql_exec()
{
    sudo docker exec -i "$MYSQL_CONT" \
    mysql -u"$MYSQL_USER" -p"$MYSQL_PASS" "$MYSQL_DB"
}

init_db()
{
    cat <<'SQL' | mysql_exec

CREATE TABLE IF NOT EXISTS example_users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    email VARCHAR(255) NOT NULL UNIQUE,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP 
);

INSERT INTO example_users (id, name, email) VALUES
    (1, 'Jean', 'jean.jean@email.com'),
    (2, 'Antoine', 'antoine.antoine@email.com');
SQL
    echo "Success: DB initialized"
}

drop_db_objects()
{
    cat <<'SQL' | mysql_exec
DROP TABLE IF EXISTS example_users;
SQL
    echo "Success: DB objects dropped"
}

wait_http()
{
    local url="http://$IP_PORT/health"
    for i in {1..50}; do
        if curl -sS --max-time 1 "$url" >/dev/null 2>&1; then
            echo "Success: Server is up"
            return 0
        fi
        sleep 0.1
    done
    echo "Failure: Couldn't confirm /health; proceeding anyway…"
}



# ---------- run test ----------

drop_db_objects
echo "Init DB on $MYSQL_CONT/$MYSQL_DB"
init_db

echo "Waiting for server at $IP_PORT"
# wait_http

echo "GET /exampleusers"
pair=$(http GET "/exampleusers")
body=$(expect_code 200 "$pair" "GET /users failed")

contains 'Jean' "$body"
contains 'Antoine' "$body"

echo "GET /exampleusers/1"
pair=$(http GET "/exampleusers/1")
body=$(expect_code 200 "$pair" "GET /users/1 failed")
contains 'Jean' "$body"

echo "POST /exampleusers"
pair=$(http POST "/exampleusers" '{"name":"Charlie","email":"charlie.charlie@email.com"}')
expect_code 201 "$pair" "POST /users failed"

echo "GET /exampleusers (after create)"
pair=$(http GET "/exampleusers")
body=$(expect_code 200 "$pair")
contains 'Charlie' "$body"

NEW_ID=3

echo "PUT /exampleusers/$NEW_ID"
pair=$(http PUT "/exampleusers/$NEW_ID" '{"name":"Charles","email":"charles@x"}')
code="${pair%%:*}"
if [[ "$code" != "200" && "$code" != "201" ]]; then
  echo "Failure Expected 200/201 on update, got $code";
  cat "${pair#*:}";
  exit 1
fi
echo "update ($code)"

echo "GET /exampleusers/$NEW_ID (after update)"
pair=$(http GET "/exampleusers/$NEW_ID")
body=$(expect_code 200 "$pair")
contains 'Charles' "$body"

echo "DELETE /exampleusers/$NEW_ID"
pair=$(http DELETE "/exampleusers/$NEW_ID")
expect_code 204 "$pair" "DELETE failed"

echo "GET /exampleusers/$NEW_ID (should 404)"
pair=$(http GET "/exampleusers/$NEW_ID")
expect_code 404 "$pair"

echo "Route smoke tests passed."
