install -o root -g root -m 0755 /dev/stdin /usr/local/bin/codex-devctl <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="/workspace/rest_api"
FRONTEND_DIR="/workspace/front_end"
REST_CONTAINER="cont_llvm_mysql_crow"
FRONT_CONTAINER="cont_eps_front"
MYSQL_CONTAINER="mysqlserver"
NETWORK_NAME="sqlRest"
IMAGE_NAME="img_llvm_mysql_crow"
MYSQL_IMAGE="mysql:8.0"

MYSQL_ROOT_PASSWORD="${MYSQL_ROOT_PASSWORD:-your_root_password}"
MYSQL_DATABASE="${MYSQL_DATABASE:-test_rest_DB}"
MYSQL_USER="${MYSQL_USER:-dev_admin}"
MYSQL_PASSWORD="${MYSQL_PASSWORD:-dev_admin}"
API_PORT="${API_PORT:-3004}"
FRONT_PORT="${FRONT_PORT:-3000}"
API_HOST_PORT="${API_HOST_PORT:-$API_PORT}"
FRONT_HOST_PORT="${FRONT_HOST_PORT:-8084}"

cmd="${1:-}"
shift || true

cd "$PROJECT_DIR"

warn_default_creds()
{
  if [[ "$MYSQL_ROOT_PASSWORD" == "your_root_password" || "$MYSQL_PASSWORD" == "dev_admin" ]]; then
    echo "WARNING: using throwaway development MySQL credentials." >&2
  fi
}

container_running()
{
  docker inspect -f '{{.State.Running}}' "$1" 2>/dev/null | grep -qx true
}

container_exists()
{
  docker inspect "$1" >/dev/null 2>&1
}

resolve_container()
{
  case "${1:-}" in
    rest|api|"$REST_CONTAINER") echo "$REST_CONTAINER" ;;
    front|frontend|"$FRONT_CONTAINER") echo "$FRONT_CONTAINER" ;;
    mysql|db|"$MYSQL_CONTAINER") echo "$MYSQL_CONTAINER" ;;
    *)
      echo "Denied container: ${1:-}" >&2
      echo "Allowed containers: rest, front, mysql" >&2
      exit 2
      ;;
  esac
}

resolve_http_target()
{
  case "${1:-}" in
    rest|api|"$REST_CONTAINER"|"$REST_CONTAINER:$API_PORT")
      echo "127.0.0.1:$API_HOST_PORT"
      ;;
    front|frontend|"$FRONT_CONTAINER"|"$FRONT_CONTAINER:$FRONT_PORT")
      echo "127.0.0.1:$FRONT_HOST_PORT"
      ;;
    *)
      echo "${1:-}"
      ;;
  esac
}

run_exec()
{
  local target
  target="$(resolve_container "${1:-}")"
  shift || true
  if [[ $# -eq 0 ]]; then
    echo "Missing command for docker exec" >&2
    exit 2
  fi
  docker exec "$target" "$@"
}

run_exec_shell()
{
  local target
  target="$(resolve_container "${1:-}")"
  shift || true
  if [[ $# -eq 0 ]]; then
    echo "Missing shell command for docker exec" >&2
    exit 2
  fi
  docker exec "$target" bash -lc "$*"
}

case "$cmd" in
  build-dev)
    docker network create "$NETWORK_NAME" >/dev/null 2>&1 || true
    docker build -t "$IMAGE_NAME" .
    docker pull "$MYSQL_IMAGE"
    ;;

  run-dev)
    warn_default_creds
    docker network create "$NETWORK_NAME" >/dev/null 2>&1 || true
    if ! container_exists "$MYSQL_CONTAINER"; then
      docker run --name "$MYSQL_CONTAINER" --network "$NETWORK_NAME" \
        -e MYSQL_ROOT_PASSWORD="$MYSQL_ROOT_PASSWORD" \
        -e MYSQL_DATABASE="$MYSQL_DATABASE" \
        -e MYSQL_USER="$MYSQL_USER" \
        -e MYSQL_PASSWORD="$MYSQL_PASSWORD" \
        -v mysql_data_test_rest:/var/lib/mysql \
        -p 3306:3306 \
        -d "$MYSQL_IMAGE"
    elif ! container_running "$MYSQL_CONTAINER"; then
      docker start "$MYSQL_CONTAINER"
    fi
    if ! container_exists "$REST_CONTAINER"; then
      docker run -d --network "$NETWORK_NAME" -p "$API_PORT:$API_PORT" \
        -e MYSQL_PASSWORD="$MYSQL_PASSWORD" \
        -v "$PROJECT_DIR:/workspace" \
        --name "$REST_CONTAINER" "$IMAGE_NAME" sleep infinity
    elif ! container_running "$REST_CONTAINER"; then
      docker start "$REST_CONTAINER"
    fi
    ;;

  start-dev)
    docker start "$MYSQL_CONTAINER"
    docker start "$REST_CONTAINER"
    ;;

  stop-dev)
    docker stop "$REST_CONTAINER"
    docker stop "$MYSQL_CONTAINER"
    ;;

  rm-dev)
    docker rm "$REST_CONTAINER"
    docker rm "$MYSQL_CONTAINER"
    ;;

  ps)
    docker ps --filter "name=$MYSQL_CONTAINER" --filter "name=$REST_CONTAINER" --filter "name=$FRONT_CONTAINER"
    ;;

  logs-rest)
    docker logs --tail=200 "$REST_CONTAINER"
    ;;

  logs-front)
    docker logs --tail=200 "$FRONT_CONTAINER"
    ;;

  logs-mysql)
    docker logs --tail=200 "$MYSQL_CONTAINER"
    ;;

  ip)
    target="$(resolve_container "${1:-rest}")"
    docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "$target"
    ;;

  exec)
    run_exec "$@"
    ;;

  exec-sh)
    run_exec_shell "$@"
    ;;

  exec-rest)
    run_exec_shell rest "$@"
    ;;

  exec-front)
    run_exec_shell front "$@"
    ;;

  exec-mysql)
    run_exec_shell mysql "$@"
    ;;

  build)
    docker exec "$REST_CONTAINER" bash -lc 'cd /workspace && mkdir -p build && cd build && rm -rf ./* && cmake .. -DENABLE_BASIC_FLAGS=ON && make'
    ;;

  build-full)
    docker exec "$REST_CONTAINER" bash -lc 'cd /workspace && mkdir -p build && cd build && rm -rf ./* && cmake .. -DENABLE_FULL_FLAGS=ON && make'
    ;;

  build-beta)
    docker exec "$REST_CONTAINER" bash -lc 'cd /workspace && mkdir -p build && cd build && rm -rf ./* && cmake .. -DENABLE_BETA_FLAGS=ON && make'
    ;;

  unit-tests)
    docker exec "$REST_CONTAINER" bash -lc 'cd /workspace && mkdir -p build && cd build && rm -rf ./* && cmake .. -DENABLE_GTEST=ON && make repo_tests && ctest --test-dir . --output-on-failure'
    ;;

  run-api)
    docker exec "$REST_CONTAINER" bash -lc 'cd /workspace && ./build/rest_api'
    ;;

  front-lint)
    docker exec "$FRONT_CONTAINER" bash -lc 'cd /workspace && npm run lint'
    ;;

  front-typecheck)
    docker exec "$FRONT_CONTAINER" bash -lc 'cd /workspace && npx tsc --noEmit'
    ;;

  front-build)
    docker exec "$FRONT_CONTAINER" bash -lc 'cd /workspace && npm run build'
    ;;

  front-test)
    docker exec "$FRONT_CONTAINER" bash -lc 'cd /workspace && npm test'
    ;;

  shell-rest)
    docker exec -it "$REST_CONTAINER" bash
    ;;

  shell-front)
    docker exec -it "$FRONT_CONTAINER" bash
    ;;

  mysql)
    docker exec -it "$MYSQL_CONTAINER" mysql -u "$MYSQL_USER" "-p$MYSQL_PASSWORD" "$MYSQL_DATABASE"
    ;;

  init-docs-db)
    docker exec -i "$MYSQL_CONTAINER" mysql -u "$MYSQL_USER" "-p$MYSQL_PASSWORD" "$MYSQL_DATABASE" < "$PROJECT_DIR/db/schema.sql"
    ;;

  drop-docs-db)
    docker exec -i "$MYSQL_CONTAINER" mysql -u "$MYSQL_USER" "-p$MYSQL_PASSWORD" "$MYSQL_DATABASE" <<'SQL'
DROP TABLE IF EXISTS pages;
DROP TABLE IF EXISTS documents;
SQL
    ;;

  migration-counts)
    docker exec -i "$MYSQL_CONTAINER" mysql -u "$MYSQL_USER" "-p$MYSQL_PASSWORD" "$MYSQL_DATABASE" -N -e "
      SELECT 'documents', COUNT(*) FROM documents;
      SELECT 'pages', COUNT(*) FROM pages;
    "
    ;;

  test-docs-read-all)
    ip_port="$(resolve_http_target "${1:-127.0.0.1:$API_HOST_PORT}")"
    curl -X GET "http://$ip_port/courtdocuments"
    ;;

  test-docs-read-page)
    ip_port="$(resolve_http_target "${1:-127.0.0.1:$API_HOST_PORT}")"
    limit="${2:-20}"
    offset="${3:-0}"
    curl -X GET "http://$ip_port/courtdocuments?limit=$limit&offset=$offset"
    ;;

  test-doc-by-id)
    ip_port="$(resolve_http_target "${1:-127.0.0.1:$API_HOST_PORT}")"
    id="${2:?document id required}"
    curl -X GET "http://$ip_port/courtdocuments/$id"
    ;;

  test-doc-pages)
    ip_port="$(resolve_http_target "${1:-127.0.0.1:$API_HOST_PORT}")"
    document_id="${2:?document id required}"
    curl -X GET "http://$ip_port/courtdocuments/$document_id/pages"
    ;;

  test-doc-search)
    ip_port="$(resolve_http_target "${1:-127.0.0.1:$API_HOST_PORT}")"
    query="${2:?query required}"
    limit="${3:-20}"
    offset="${4:-0}"
    curl -G -X GET "http://$ip_port/courtdocuments/search" \
      --data-urlencode "q=$query" \
      --data-urlencode "limit=$limit" \
      --data-urlencode "offset=$offset"
    ;;

  front-smoke)
    ip_port="$(resolve_http_target "${1:-$FRONT_CONTAINER:$FRONT_PORT}")"
    curl -I "http://$ip_port"
    ;;

  *)
    echo "Denied command: $cmd" >&2
    echo "Allowed: build-dev run-dev start-dev stop-dev rm-dev ps logs-rest logs-front logs-mysql ip exec exec-sh exec-rest exec-front exec-mysql build build-full build-beta unit-tests run-api front-lint front-typecheck front-build front-test shell-rest shell-front mysql init-docs-db drop-docs-db migration-counts test-docs-read-all test-docs-read-page test-doc-by-id test-doc-pages test-doc-search front-smoke" >&2
    exit 2
    ;;
esac
SCRIPT
