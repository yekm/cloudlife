# Agent workflow

## Roles and defaults

| Role | Model | Reasoning | Responsibility |
| --- | --- | --- | --- |
| Primary | `gpt-5.6-sol` | high | Scope, architecture, coordination, integration, final validation |
| explorer | `gpt-5.6-luna` | medium | Locate code, trace simple flows, summarize evidence |
| implementer | `gpt-5.6-sol` | high | Bounded features and fixes |
| reviewer | `gpt-5.6-terra` | medium | Independent correctness and regression review |
| specialist | `gpt-5.6-sol` | high | Difficult graphics, synchronization, performance, architecture |

These are project choices, not benchmark claims. Actual availability depends on
the host. If a configured model is unavailable, report it and inherit the parent
model and effort instead of repeatedly retrying. Use medium for straightforward
implementation assignments; use xhigh for a specialist only after identifying a
specific unresolved reasoning problem. Avoid max/ultra by default.
Standalone role files pin their settings; adjust the file for persistent tuning.
Runtime-specific spawn overrides may not override a loaded role file.

## Delegation

- Delegate substantial work only when an independent subtask can run alongside
  useful local work. Do small edits and tightly coupled work in the primary agent.
- Keep at most three sub-agents active, and respect any lower runtime limit.
  Agents should return to the primary for further delegation rather than spawn
  grandchildren by default.
- Give each assignment an objective, relevant context, owned files, acceptance
  criteria, and expected output. Share only the history needed for the work.
- With generic spawning tools, explicitly pass the role's model and effort and
  provide its instructions. Use fresh or partial context when model overrides
  cannot be combined with full-history forks.
- Treat the filesystem as shared. Assign one writer per file. The primary owns
  integration files unless it delegates them. Never revert another agent's work.
- Give one agent ownership of each build directory; serialize builds in a shared
  directory. Read-only review is a task instruction, not a sandbox guarantee.
- The primary reviews delegated edits and completes relevant validation before
  reporting success. Do not launch agents merely to repeat completed checks.

## Cloudlife validation

- Follow the existing C++17 style and Easel abstractions in AGENTS.md.
- For rendering changes, check OpenGL resource lifetime and upload ordering.
  Verify macOS capability guards and Linux compute-shader paths where relevant.
- Configure a Debug build when needed, then use `cmake --build build --parallel`.
  Run focused tests or runtime checks appropriate to the change.
- A successful build does not verify rendering. Report which platforms and visual
  behavior were actually exercised. Do not claim Linux runtime coverage on macOS.
- Leave vendored dependencies and generated build artifacts out of changes.

## Activation and reuse

Project defaults live in `.codex/config.toml`; roles live in `.codex/agents/`.
Start a new task in the trusted project after installation and check its model
and reasoning selection. Explicit app/session choices may override defaults.
Editing these files does not change the model of an already running turn.

To reuse in another repository:

1. Copy `.codex/config.toml`, `.codex/agents/`, and this document. Merge any
   existing configuration instead of overwriting it.
2. Copy the Agent Coordination section from AGENTS.md into that project's
   AGENTS.md. Keep its existing project instructions.
3. Replace the Cloudlife validation section with the target project's checks.
4. Start a new task, trust the project if prompted, and verify model availability.

For personal defaults across repositories, merge the config into
`~/.codex/config.toml` and copy roles into `~/.codex/agents/`. Remove or adapt the
references to this document before using those roles globally. Project overrides
can remain local. No personal/global configuration is changed by this setup.

References:

- [Codex subagents](https://learn.chatgpt.com/docs/agent-configuration/subagents)
- [Configuration reference](https://learn.chatgpt.com/docs/config-file/config-reference)
