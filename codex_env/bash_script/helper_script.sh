#!/usr/bin/env bash
set -euo pipefail

CODEX_IMAGE="codex_agent_img"
CODEX_CONTAINER="codex_agent_cont"
CODEX_NETWORK="sqlRest"
CODEX_WORKDIR="/workspace"

CODEX_HOME_DIR="$HOME/repos/eps"

codex_env_build_img()
{
   sudo docker build \
        -f Dockerfile.codex \
        --build-arg UID="$(id -u)" \
        --build-arg GID="$(id -g)" \
        -t "$CODEX_IMAGE" \
        .
}

codex_env_start()
{
    mkdir -p "$CODEX_HOME_DIR"

    sudo docker run -it \
        --name "$CODEX_CONTAINER" \
        --network "$CODEX_NETWORK" \
        --add-host=host.docker.internal:host-gateway \
        --user "$(id -u):$(id -g)" \
        -v "$PWD:$CODEX_WORKDIR" \
        -v "$CODEX_HOME_DIR:/home/codex/.codex" \
        -w "$CODEX_WORKDIR" \
        "$CODEX_IMAGE" \
        bash
}

codex_env_shell()
{
    sudo docker exec -it "$CODEX_CONTAINER" bash
}

codex_env_stop()
{
    sudo docker stop "$CODEX_CONTAINER"
}

codex_env_rm()
{
    sudo docker rm "$CODEX_CONTAINER"
}

codex_env_reset()
{
    sudo docker rm -f "$CODEX_CONTAINER" 2>/dev/null || true
}

codex_env_login()
{
    codex_env_start
}

codex_env_fix_project_dir_in_whitelist()
{
    CURRENT_HOME="$HOME"
    sudo sed -i "s|/workspace/rest_api|$CURRENT_HOME/repos/eps/rest_api|g" /usr/local/bin/codex-devctl
    sudo sed -i "s|/workspace/front_end|$CURRENT_HOME/repos/eps/front_end|g" /usr/local/bin/codex-devctl
}


codex_env_codex_devctl_sudoers_line()
{
    local sudo_user="${1:-${USER}}"
    local codex_devctl="${2:-/usr/local/bin/codex-devctl}"

    printf '%s ALL=(root) NOPASSWD: ' "$sudo_user"
    printf '%s' "${codex_devctl} build-dev"
    printf ', %s' "${codex_devctl} run-dev"
    printf ', %s' "${codex_devctl} start-dev"
    printf ', %s' "${codex_devctl} stop-dev"
    printf ', %s' "${codex_devctl} rm-dev"
    printf ', %s' "${codex_devctl} ps"
    printf ', %s' "${codex_devctl} logs-rest"
    printf ', %s' "${codex_devctl} logs-front"
    printf ', %s' "${codex_devctl} logs-mysql"
    printf ', %s' "${codex_devctl} ip"
    printf ', %s' "${codex_devctl} ip *"
    printf ', %s' "${codex_devctl} exec *"
    printf ', %s' "${codex_devctl} exec-sh *"
    printf ', %s' "${codex_devctl} exec-rest *"
    printf ', %s' "${codex_devctl} exec-front *"
    printf ', %s' "${codex_devctl} exec-mysql *"
    printf ', %s' "${codex_devctl} build"
    printf ', %s' "${codex_devctl} build-full"
    printf ', %s' "${codex_devctl} build-beta"
    printf ', %s' "${codex_devctl} unit-tests"
    printf ', %s' "${codex_devctl} run-api"
    printf ', %s' "${codex_devctl} front-lint"
    printf ', %s' "${codex_devctl} front-typecheck"
    printf ', %s' "${codex_devctl} front-build"
    printf ', %s' "${codex_devctl} front-test"
    printf ', %s' "${codex_devctl} shell-rest"
    printf ', %s' "${codex_devctl} shell-front"
    printf ', %s' "${codex_devctl} mysql"
    printf ', %s' "${codex_devctl} init-docs-db"
    printf ', %s' "${codex_devctl} drop-docs-db"
    printf ', %s' "${codex_devctl} migration-counts"
    printf ', %s' "${codex_devctl} test-docs-read-all"
    printf ', %s' "${codex_devctl} test-docs-read-all *"
    printf ', %s' "${codex_devctl} test-docs-read-page *"
    printf ', %s' "${codex_devctl} test-doc-by-id *"
    printf ', %s' "${codex_devctl} test-doc-pages *"
    printf ', %s' "${codex_devctl} test-doc-search *"
    printf ', %s' "${codex_devctl} front-smoke"
    printf ', %s' "${codex_devctl} front-smoke *"
    printf '\n'
}

codex_env_install_codex_devctl_sudoers()
{
    local sudo_user="${1:-${USER}}"
    local codex_devctl="${2:-/usr/local/bin/codex-devctl}"
    local sudoers_file="/etc/sudoers.d/codex-devctl-${sudo_user}"
    local tmp_file

    tmp_file="$(mktemp)"
    codex_env_codex_devctl_sudoers_line "$sudo_user" "$codex_devctl" > "$tmp_file"

    sudo chown root:root "$tmp_file"
    sudo chmod 0440 "$tmp_file"
    sudo visudo -cf "$tmp_file"
    sudo install -o root -g root -m 0440 "$tmp_file" "$sudoers_file"
    rm -f "$tmp_file"

    echo "Installed sudoers rule: $sudoers_file"
}

codex_env_create_codex_side_devctl()
{
    local wrapper_path="${1:-bin/devctl}"
    local runner_host="${CODEX_RUNNER_HOST:-host.docker.internal}"
    local runner_user="${CODEX_RUNNER_USER:-codex-runner}"

    mkdir -p "$(dirname "$wrapper_path")"

    cat > "$wrapper_path" <<SCRIPT
#!/usr/bin/env bash
set -euo pipefail

cmd="\${1:-}"
shift || true

runner_key="\${CODEX_RUNNER_KEY:-/home/codex/.ssh/id_ed25519}"
known_hosts="\${CODEX_KNOWN_HOSTS:-/home/codex/.ssh/known_hosts}"
ssh_args=(-o BatchMode=yes)

if [[ -r "\$runner_key" ]]; then
  ssh_args+=(-i "\$runner_key")
fi
if [[ -r "\$known_hosts" ]]; then
  ssh_args+=(-o UserKnownHostsFile="\$known_hosts")
fi

case "\$cmd" in
  build-dev|run-dev|start-dev|stop-dev|rm-dev|ps|logs-rest|logs-front|logs-mysql|ip|build|build-full|build-beta|unit-tests|run-api|front-lint|front-typecheck|front-build|front-test|init-docs-db|drop-docs-db|migration-counts|test-docs-read-all|test-docs-read-page|test-doc-by-id|test-doc-pages|test-doc-search|front-smoke)
    ssh "\${ssh_args[@]}" ${runner_user}@${runner_host} "\$cmd" "\$@"
    ;;
  *)
    echo "Denied local command: \$cmd" >&2
    echo "Allowed: build-dev run-dev start-dev stop-dev rm-dev ps logs-rest logs-front logs-mysql ip build build-full build-beta unit-tests run-api front-lint front-typecheck front-build front-test init-docs-db drop-docs-db migration-counts test-docs-read-all test-docs-read-page test-doc-by-id test-doc-pages test-doc-search front-smoke" >&2
    exit 2
    ;;
esac
SCRIPT

    chmod +x "$wrapper_path"
    echo "Created Codex-side devctl wrapper: $wrapper_path"
}


codex_env_create_codex_user()
{
    sudo useradd -m -s /bin/bash codex-runner
}

codex_env_build_ssh_bridge()
{
    local CODEX_RUNNER="codex-runner"
    local CODEX_SECRET_DIR="$HOME/.local/share/codex-env/secrets"
    local CODEX_KEY="$CODEX_SECRET_DIR/codex_runner_key"

    if ! id "$CODEX_RUNNER" >/dev/null 2>&1; then
        sudo useradd -m -s /bin/bash "$CODEX_RUNNER"
    fi

    mkdir -p "$CODEX_SECRET_DIR"
    chmod 700 "$CODEX_SECRET_DIR"

    if [ ! -f "$CODEX_KEY" ]; then
        ssh-keygen -t ed25519 -f "$CODEX_KEY" -N ""
    fi

    chmod 600 "$CODEX_KEY"
    chmod 644 "$CODEX_KEY.pub"

    sudo mkdir -p "/home/$CODEX_RUNNER/.ssh"

    sudo tee "/home/$CODEX_RUNNER/.ssh/authorized_keys" >/dev/null < "$CODEX_KEY.pub"

    sudo chown -R "$CODEX_RUNNER:$CODEX_RUNNER" "/home/$CODEX_RUNNER/.ssh"
    sudo chmod 700 "/home/$CODEX_RUNNER/.ssh"
    sudo chmod 600 "/home/$CODEX_RUNNER/.ssh/authorized_keys"
}

codex_env_build_ssh_gateway()
{
sudo install -o root -g root -m 0755 /dev/stdin /usr/local/bin/codex-ssh-gateway <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

read -r -a argv <<< "${SSH_ORIGINAL_COMMAND:-}"
cmd="${argv[0]:-}"
args=()
if [[ ${#argv[@]} -gt 1 ]]; then
  args=("${argv[@]:1}")
fi

case "$cmd" in
  build-dev|run-dev|start-dev|stop-dev|rm-dev|ps|logs-rest|logs-front|logs-mysql|ip|build|build-full|build-beta|unit-tests|run-api|front-lint|front-typecheck|front-build|front-test|init-docs-db|drop-docs-db|migration-counts|test-docs-read-all|test-docs-read-page|test-doc-by-id|test-doc-pages|test-doc-search|front-smoke)
    sudo /usr/local/bin/codex-devctl "$cmd" "${args[@]}"
    ;;

  *)
    echo "Denied SSH command: ${SSH_ORIGINAL_COMMAND:-}" >&2
    exit 2
    ;;
esac
EOF
}

codex_env_force_ssh_gateway()
{
    CODEX_RUNNER="codex-runner"
    CODEX_KEY="$HOME/.local/share/codex-env/secrets/codex_runner_key"

    PUBKEY="$(cat "$CODEX_KEY.pub")"

    echo "command=\"/usr/local/bin/codex-ssh-gateway\",no-agent-forwarding,no-X11-forwarding,no-pty ${PUBKEY}" \
    | sudo tee "/home/$CODEX_RUNNER/.ssh/authorized_keys" >/dev/null

    sudo chown "$CODEX_RUNNER:$CODEX_RUNNER" "/home/$CODEX_RUNNER/.ssh/authorized_keys"
    sudo chmod 600 "/home/$CODEX_RUNNER/.ssh/authorized_keys"
    sudo chmod 700 "/home/$CODEX_RUNNER/.ssh"
}


codex_env_codex_runner_devctl_sudoers_line()
{
    local sudo_user="${1:-codex-runner}"
    local codex_devctl="${2:-/usr/local/bin/codex-devctl}"

    codex_env_codex_devctl_sudoers_line "$sudo_user" "$codex_devctl"
}


codex_env_install_codex_runner_devctl_sudoers()
{
    local sudo_user="${1:-codex-runner}"
    local codex_devctl="${2:-/usr/local/bin/codex-devctl}"
    local sudoers_file="/etc/sudoers.d/codex-runner-devctl"
    local tmp_file

    tmp_file="$(mktemp)"

    codex_env_codex_runner_devctl_sudoers_line \
        "$sudo_user" \
        "$codex_devctl" > "$tmp_file"

    sudo chown root:root "$tmp_file"
    sudo chmod 0440 "$tmp_file"

    sudo visudo -cf "$tmp_file"

    sudo install \
        -o root \
        -g root \
        -m 0440 \
        "$tmp_file" \
        "$sudoers_file"

    sudo rm -f "$tmp_file"

    sudo visudo -c

    echo "Installed sudoers rule: $sudoers_file"
}

codex_env_mount_ssh_key()
{
    local CODEX_CONTAINER="codex_agent_cont"
    local CODEX_SECRET_DIR="$HOME/.local/share/codex-env/secrets"
    local CODEX_KEY="$CODEX_SECRET_DIR/codex_runner_key"
    local CODEX_HOME="/home/codex"
    local CODEX_SSH_DIR="$CODEX_HOME/.ssh"

    if [ ! -f "$CODEX_KEY" ]; then
        echo "Missing private key: $CODEX_KEY" >&2
        echo "Run codex_env_build_ssh_bridge first." >&2
        return 1
    fi

    # Create ~/.ssh inside the running container.
    sudo docker exec -u 0 "$CODEX_CONTAINER" mkdir -p "$CODEX_SSH_DIR"

    # Copy the private key into the container.
    sudo docker cp "$CODEX_KEY" "$CODEX_CONTAINER:$CODEX_SSH_DIR/id_ed25519"

    # Fix ownership based on the container's current user.
    local CONTAINER_UID
    local CONTAINER_GID

    CONTAINER_UID="$(sudo docker exec "$CODEX_CONTAINER" id -u)"
    CONTAINER_GID="$(sudo docker exec "$CODEX_CONTAINER" id -g)"

    sudo docker exec -u 0 "$CODEX_CONTAINER" chown -R "$CONTAINER_UID:$CONTAINER_GID" "$CODEX_SSH_DIR"

    # Fix SSH permissions.
    sudo docker exec "$CODEX_CONTAINER" chmod 700 "$CODEX_SSH_DIR"
    sudo docker exec "$CODEX_CONTAINER" chmod 600 "$CODEX_SSH_DIR/id_ed25519"

    # Add host key.
    sudo docker exec "$CODEX_CONTAINER" sh -lc \
        "ssh-keyscan host.docker.internal >> '$CODEX_SSH_DIR/known_hosts'"

    sudo docker exec "$CODEX_CONTAINER" chmod 600 "$CODEX_SSH_DIR/known_hosts"
}

codex_env_build_capability_gated_command_bridge()
{
    codex_env_fix_project_dir_in_whitelist
    codex_env_install_codex_devctl_sudoers
    codex_env_create_codex_side_devctl
    codex_env_build_ssh_bridge
    codex_env_build_ssh_gateway
    codex_env_force_ssh_gateway
    codex_env_install_codex_runner_devctl_sudoers
    codex_env_mount_ssh_key
}

