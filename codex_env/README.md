# Codex Environment

`codex_env` provides a controlled execution environment for running Codex against this repository. The pattern is a capability-gated host command bridge: Codex stays inside a Docker container, while selected host-side commands are exposed through an SSH forced-command gateway and a sudo allowlist.

This lets an LLM run builds, tests, smoke checks, and Docker workflow commands without giving it unrestricted host shell access.

## What This Directory Contains

```txt
codex_env/
  Dockerfile.codex                  Codex container image
  README.md                         This guide
  bash_script/
    helper_script.sh                Host-side setup and bridge functions
    codex_whitelist.sh              Generates /usr/local/bin/codex-devctl
  bin/
    devctl                          Codex-side wrapper that calls the SSH gateway
```

## Architecture

There are three execution layers:

1. Codex container

   The LLM runs inside `codex_agent_cont`, built from `Dockerfile.codex`. It has developer tools such as `git`, `curl`, `jq`, `rg`, `ssh`, and the Codex CLI, but it does not get direct host control.

2. Host command controller

   The host installs `/usr/local/bin/codex-devctl` from `bash_script/codex_whitelist.sh`. This script contains the allowed Docker, build, test, database, frontend, and smoke-check commands.

3. SSH forced-command bridge

   The container calls `codex_env/bin/devctl`. That wrapper SSHes to `codex-runner@host.docker.internal`. The runner key is forced to execute `/usr/local/bin/codex-ssh-gateway`, which only forwards approved command names to `sudo /usr/local/bin/codex-devctl`.

The result is a narrow operational bridge:

```txt
Codex container -> bin/devctl -> SSH forced command -> sudo allowlist -> codex-devctl -> host Docker/project commands
```

## Security Model

The bridge is designed around least privilege:

- Codex does not receive an unrestricted host shell.
- The SSH key is restricted with `command="/usr/local/bin/codex-ssh-gateway"`.
- The host gateway accepts only known command names.
- `sudo` is limited to specific `/usr/local/bin/codex-devctl ...` invocations.
- Container names and HTTP target aliases are resolved by the whitelist script, not arbitrary user input.
- Host-side Docker and project operations remain auditable through a small command surface.

This is not a general-purpose security sandbox. Treat it as a practical capability gate for development automation.

## Requirements

On the host:

- Docker
- OpenSSH server reachable from the Codex container through `host.docker.internal`
- `sudo`
- `bash`
- `visudo`
- The project checked out at the path expected by the helper script

The helper currently assumes:

```txt
Codex image:       codex_agent_img
Codex container:   codex_agent_cont
Docker network:    sqlRest
Container workdir: /workspace
Codex home volume: $HOME/repos/eps
Runner user:       codex-runner
```

## Initial Setup

Run these commands from the host, inside `codex_env/`:

```bash
source bash_script/helper_script.sh
codex_env_build_img
codex_env_login
```

`codex_env_login` creates and starts the Codex container. It mounts the current directory into `/workspace`, attaches the container to the `sqlRest` Docker network, and adds `host.docker.internal` as a host-gateway alias.

Useful container lifecycle helpers:

```bash
codex_env_start      # Start and attach to an existing Codex container
codex_env_shell      # Open a shell in the running Codex container
codex_env_stop       # Stop the Codex container
codex_env_rm         # Remove the stopped Codex container
codex_env_reset      # Force-remove the Codex container
```

## Build The Command Bridge

From the host, after sourcing `helper_script.sh`:

```bash
sudo bash bash_script/codex_whitelist.sh
codex_env_build_capability_gated_command_bridge
```

That combined flow installs or refreshes:

- `/usr/local/bin/codex-devctl`
- sudoers rules for the host user and `codex-runner`
- the `codex-runner` user if missing
- the SSH keypair under `$HOME/.local/share/codex-env/secrets/`
- the forced SSH gateway at `/usr/local/bin/codex-ssh-gateway`
- `codex_env/bin/devctl`
- the private SSH key and known hosts file inside the Codex container

If the repository path in `/usr/local/bin/codex-devctl` needs to be corrected after regeneration, run:

```bash
codex_env_fix_project_dir_in_whitelist
```

## Refresh After Changing The Whitelist

When `bash_script/codex_whitelist.sh` or the allowed command list changes, refresh both host-side and Codex-side pieces:

```bash
source bash_script/helper_script.sh
sudo bash bash_script/codex_whitelist.sh
codex_env_build_capability_gated_command_bridge
```

If only the local wrapper changed, this is enough:

```bash
codex_env_create_codex_side_devctl
```

## Codex-Side Usage

Inside the Codex container, use:

```bash
codex_env/bin/devctl <command> [args...]
```

Examples:

```bash
codex_env/bin/devctl ps
codex_env/bin/devctl front-typecheck
codex_env/bin/devctl front-build
codex_env/bin/devctl test-doc-search cont_llvm_mysql_crow:3004 epstein 5 0
codex_env/bin/devctl front-smoke cont_eps_front:3000
```

The wrapper prints a denial message if the requested command is not in its local allowlist.

## Current Codex-Side Allowed Commands

`codex_env/bin/devctl` currently allows:

```txt
build-dev
run-dev
start-dev
stop-dev
rm-dev
ps
logs-rest
logs-front
logs-mysql
ip
build
build-full
build-beta
unit-tests
run-api
front-lint
front-typecheck
front-build
front-test
init-docs-db
drop-docs-db
migration-counts
test-docs-read-all
test-docs-read-page
test-doc-by-id
test-doc-pages
test-doc-search
front-smoke
```

The host-side `/usr/local/bin/codex-devctl` also contains broader maintenance commands such as `exec`, `exec-sh`, `exec-rest`, `exec-front`, `exec-mysql`, `shell-rest`, `shell-front`, and `mysql`, but they are not exposed by the current Codex-side wrapper unless the wrapper and gateway allowlists are regenerated to include them.

## Command Reference

### Development Containers

```bash
codex_env/bin/devctl build-dev     # Build REST image and pull MySQL image
codex_env/bin/devctl run-dev       # Create/start MySQL and REST containers
codex_env/bin/devctl start-dev     # Start MySQL and REST containers
codex_env/bin/devctl stop-dev      # Stop MySQL and REST containers
codex_env/bin/devctl rm-dev        # Remove MySQL and REST containers
codex_env/bin/devctl ps            # List REST, frontend, and MySQL containers
```

### Logs And Addresses

```bash
codex_env/bin/devctl logs-rest
codex_env/bin/devctl logs-front
codex_env/bin/devctl logs-mysql
codex_env/bin/devctl ip rest
codex_env/bin/devctl ip front
codex_env/bin/devctl ip mysql
```

### REST API Build And Tests

```bash
codex_env/bin/devctl build
codex_env/bin/devctl build-full
codex_env/bin/devctl build-beta
codex_env/bin/devctl unit-tests
codex_env/bin/devctl run-api
```

### Frontend Checks

```bash
codex_env/bin/devctl front-lint
codex_env/bin/devctl front-typecheck
codex_env/bin/devctl front-build
codex_env/bin/devctl front-test
codex_env/bin/devctl front-smoke cont_eps_front:3000
```

`front-lint` may prompt for ESLint setup if the frontend has not been configured for linting. `front-test` may fail with no tests if no Jest tests exist.

### Document Database Helpers

```bash
codex_env/bin/devctl init-docs-db
codex_env/bin/devctl drop-docs-db
codex_env/bin/devctl migration-counts
```

### Document REST Smoke Tests

```bash
codex_env/bin/devctl test-docs-read-all [target]
codex_env/bin/devctl test-docs-read-page [target] [limit] [offset]
codex_env/bin/devctl test-doc-by-id [target] <document_id>
codex_env/bin/devctl test-doc-pages [target] <document_id>
codex_env/bin/devctl test-doc-search [target] <query> [limit] [offset]
```

Targets may be host-published addresses or Docker-style aliases. The whitelist resolves common container targets to host-published ports for host-side execution:

```txt
cont_llvm_mysql_crow:3004 -> 127.0.0.1:3004
cont_eps_front:3000       -> 127.0.0.1:8084
```

Examples:

```bash
codex_env/bin/devctl test-docs-read-page cont_llvm_mysql_crow:3004 10 0
codex_env/bin/devctl test-doc-search 127.0.0.1:3004 "flight logs" 10 0
codex_env/bin/devctl front-smoke cont_eps_front:3000
codex_env/bin/devctl front-smoke 127.0.0.1:8084
```

## Host-Side Whitelist Details

`bash_script/codex_whitelist.sh` generates `/usr/local/bin/codex-devctl`. Important defaults inside that generated script:

```txt
PROJECT_DIR=/workspace/rest_api
FRONTEND_DIR=/workspace/front_end
REST_CONTAINER=cont_llvm_mysql_crow
FRONT_CONTAINER=cont_eps_front
MYSQL_CONTAINER=mysqlserver
NETWORK_NAME=sqlRest
API_PORT=3004
FRONT_PORT=3000
FRONT_HOST_PORT=8084
```

`codex_env_fix_project_dir_in_whitelist` rewrites `PROJECT_DIR` and `FRONTEND_DIR` to the host checkout path under `$HOME/repos/eps`.

## Troubleshooting

### `Denied local command`

The command is not in `codex_env/bin/devctl`. Regenerate the Codex-side wrapper after updating the helper script:

```bash
codex_env_create_codex_side_devctl
```

### `Denied SSH command`

The forced SSH gateway does not allow that command. Rebuild the bridge after updating gateway allowlists:

```bash
codex_env_build_capability_gated_command_bridge
```

### `sudo: a password is required`

The sudoers rule for the host user or `codex-runner` is missing or stale:

```bash
codex_env_install_codex_devctl_sudoers
codex_env_install_codex_runner_devctl_sudoers
```

### SSH key errors

Refresh the SSH bridge and remount the key into the running container:

```bash
codex_env_build_ssh_bridge
codex_env_force_ssh_gateway
codex_env_mount_ssh_key
```

### Wrong project path in host commands

Run:

```bash
codex_env_fix_project_dir_in_whitelist
```

Then retry the `devctl` command.

## Maintenance Notes

- Keep `helper_script.sh`, `codex_whitelist.sh`, `codex_env/bin/devctl`, and the SSH gateway allowlist in sync.
- Prefer adding narrow commands over exposing shell access.
- After changing command names, rebuild the bridge and rerun a smoke command such as `codex_env/bin/devctl ps`.
- Avoid placing secrets in this directory. The runner key is generated under `$HOME/.local/share/codex-env/secrets/` and copied into the Codex container only when the bridge is built.

## Short Description

A capability-gated host command bridge for LLM development agents: Codex runs in a Docker container and can execute only whitelisted host operations through a forced SSH command, a dedicated `codex-runner` user, and sudoers-limited `codex-devctl` commands.
