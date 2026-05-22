# LLM capability_gated_command_bridge

## Description 

It's a sandboxed LLM with a whitelisted host command gateway.

A sandboxed LLM host-command bridge that gives the model limited operational access through a forced SSH command, a dedicated runner user, and a sudoers allowlist.
The LLM stays inside its container, while only explicitly approved commands can be forwarded to the host for Docker, build, test, and smoke-check workflows.

Characterics:

  - SSH command gateway: the LLM container can only reach the host through a forced SSH command.
  - Command allowlist / whitelist: only approved commands are forwarded.
  - Least-privilege runner user: codex-runner is the controlled host identity.
  - NOPASSWD sudo command allowlist: sudo is allowed only for specific codex-devctl invocations.
  - Container-to-host control bridge: the LLM runs isolated in Docker but can operate selected host Docker workflows.


## Installation 

From the host, in `./codex\_env/` dir, run:

#### Build the env
source bash_script/helper_script.sh	
```bash
codex_env_build_img
codex_env_start
```
You can log in codex.

#### Build the command bridge
from the host:
if codex-runner user does not exit, run: 
```bash
codex_env_create_codex_user()
```

and then

```bash
sudo bash bash_script/codex_whitelist.sh
codex_env_build_capability_gated_command_bridge
```

Don't forget to run  ``codex_env_create_codex_side_devctl`` if you update the whitelist.

The Core Team
Lorris BELUS - Developer


