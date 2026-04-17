---
name: add-mcp-command
description: Add a new MCP command to SpirrowBridge end-to-end (Python meta-tool entry + schema + C++ handler + routing + build + MCP restart + docs + commit/PR). Use when the user asks to add a command/feature to the Unreal MCP server.
---

# Add MCP command — guided workflow

You are extending **SpirrowBridge** (the C++ UE plugin + Python MCP server). Adding a command touches Python (meta-tool dispatch + schema), C++ (handler + central routing), build, the running MCP process, and docs. Skipping a step = a confused future user.

## Before you start

Confirm with the user (or infer from context):
- **Command name** (snake_case, e.g. `do_thing`)
- **Meta-tool** it belongs to: `editor` / `blueprint` / `blueprint_node` / `umg_widget` / `umg_layout` / `umg_animation` / `umg_variable` / `project` / `ai` / `perception` / `eqs` / `gas` / `material` / `config` — pick the one whose existing commands are conceptually closest. If none fits, propose a new meta-tool category instead (different workflow).
- **Params**: name, type, required vs optional + default
- **What the C++ handler does** at the UE API level (which UE class/function)

If unsure on any of these, ask the user before coding.

## The 7 steps (do in this order)

### 1. C++ handler header

`MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Public/Commands/SpirrowBridge<Category>Commands.h`

Add a `Handle<CommandName>` private method. If the file/class doesn't exist for your category, create it (mirror `SpirrowBridgeLevelCommands.h` for the minimal pattern).

### 2. C++ handler implementation

`MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridge<Category>Commands.cpp`

- Add `if (CommandType == TEXT("<command>"))` to `HandleCommand`
- Implement `Handle<CommandName>`. Use:
  - `FSpirrowBridgeCommonUtils::CreateErrorResponse(...)` / `CreateSuccessResponse(Data)` for return
  - `FSpirrowBridgeCommonUtils::SetObjectProperty(Object, Name, JsonValue, Err)` for any UPROPERTY write (handles class/object/struct/primitive)
  - `FSpirrowBridgeCommonUtils::ResolveTargetBlueprint(Params, OutBP)` if the command edits a Blueprint AND should support `target_type="level_blueprint"`
  - `GEditor->GetEditorWorldContext().World()` for "current editor world"
- Validate required params at top, fail fast with `CreateErrorResponse`

### 3. Central routing — **the step that's easy to forget**

`MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/SpirrowBridge.cpp`

Find the `else if (CommandType == TEXT("...") || CommandType == TEXT("..."))` block for your category in `ExecuteCommand`. Add your command name to that OR-chain. If routing isn't added here, the user gets `Unknown command: xxx` no matter how good the handler is.

If the handler class is brand new: also add `#include "Commands/SpirrowBridge<Category>Commands.h"` + `TSharedPtr<F...> <Category>Commands` member in `Public/SpirrowBridge.h`, plus `MakeShared` in constructor and `.Reset()` in destructor.

### 4. Build the editor

```bash
cd "C:/Users/owner/Documents/Unreal Projects/spirrow-unrealwise"
cmd.exe //C ".\\build_editor.bat"
```

Pre-flight: **UE Editor must be closed** (Live Coding will block the build with `Unable to build while Live Coding is active`). If it's open, ask the user to close it or press `Ctrl+Alt+F11` for Live Coding rebuild.

Expected output: `Result: Succeeded`. Deprecation warnings (e.g. `UEditorLevelLibrary` C4996) are non-fatal — note them but don't block.

### 5. Python meta-tool entry

`Python/tools/<category>_meta.py`

- Add to the `COMMANDS` dict: `"<command>": "<command>"` (Python name → C++ command name; usually identical)
- Add the command to the meta-tool `@mcp.tool()` docstring `Commands:` list
- If the command needs special pre-processing (vector validation, type coercion, default-fill), add it inside the `def <category>(...)` function before `execute_command(...)` — see `editor_meta.py` `spawn_actor` for the `location/rotation` validation pattern

### 6. Python schema

`Python/tools/command_schemas.py`

Add an entry under the matching meta-tool's section:

```python
"<command>": {
    "brief": "<one-line summary>",
    "params": {
        "<param>": {"type": "<str|int|float|bool|list[float]|dict>",
                    "required": True,  # or "default": <value>
                    "desc": "<what it does>"},
        ...
    },
},
```

Update the `# <CATEGORY> (N commands)` count comment at the top of that section.

### 7. Restart the MCP server — **CLAUDE.md mandate**

After **any** Python edit you must kill the running MCP server, otherwise the user gets `Unknown command: <new-command>` because the old `COMMANDS` dict is cached in the running process. Use the `restart-mcp-server` skill, or run inline:

```bash
# Enumerate
powershell -NoProfile -Command "Get-CimInstance Win32_Process | Where-Object { \$_.Name -in @('python.exe','uv.exe') -and \$_.CommandLine -match 'unreal_mcp_server|spirrow-unrealwise' } | Select-Object ProcessId,ParentProcessId,Name | Format-Table -AutoSize"

# Kill the parent uv.exe PID(s); children cascade
cmd.exe //C "taskkill /F /PID <uv_pid>"

# Verify zero remaining
powershell -NoProfile -Command "Get-CimInstance Win32_Process | Where-Object { \$_.Name -in @('python.exe','uv.exe') -and \$_.CommandLine -match 'unreal_mcp_server|spirrow-unrealwise' } | Measure-Object | Select-Object -ExpandProperty Count"
# Expect: 0
```

The MCP client (Claude Code / Desktop) auto-spawns a fresh server on the next tool call.

### 8. Documentation

Update in this order (don't skip — the README is the front door):

| File | What to update |
|---|---|
| `FEATURE_STATUS.md` | Bump version (e.g. v0.9.5 → v0.9.6), bump category command count, total command count, add a "最新の更新" entry describing the change |
| `Docs/IMPLEMENTATION_SUMMARY.md` | Bump version line, update the C++ file size in the table if a file grew significantly, add to "v0.9.X 新機能" section |
| `README.md` / `README_ja.md` | Update `## Features (25 MCP Tools / N Commands)` total + meta-tool table row count |

### 9. Commit + push + PR

Use a feature branch, never push to main directly (the repo blocks it):

```bash
git checkout -b feat/v0.9.X-<short-name>
git add <files>
git commit -m "feat(SpirrowBridge): v0.9.X - <summary>

<body explaining why and what>

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"

git push -u origin feat/v0.9.X-<short-name>
gh pr create --base main --head feat/v0.9.X-<short-name> --title "..." --body "..."
```

The PR body should include a **Test plan** section with concrete `editor(command="...")` invocations the user can paste to verify.

## Validation checklist before declaring done

- [ ] Build succeeded (`Result: Succeeded` in build_editor.bat output)
- [ ] MCP server processes killed (verified count = 0)
- [ ] Python `COMMANDS` dict has the entry
- [ ] `command_schemas.py` has the schema and command count comment is correct
- [ ] `SpirrowBridge.cpp::ExecuteCommand` routes the new command
- [ ] FEATURE_STATUS / IMPLEMENTATION_SUMMARY / README updated
- [ ] PR opened with test plan

## Common failures

| Symptom | Cause | Fix |
|---|---|---|
| `Unknown command: xxx` from MCP | Step 3 (central routing) skipped, OR step 7 (kill MCP) skipped | Verify `SpirrowBridge.cpp` ExecuteCommand has the command in the OR-chain; kill MCP if Python edited |
| `Unable to build while Live Coding is active` | UE Editor open with Live Coding | Close editor, or use Ctrl+Alt+F11 in editor |
| `fatal error C1083: include file ... No such file or directory` | Wrong include path (e.g. `EditorLoadingAndSavingUtils.h` doesn't exist; the class is in `FileHelpers.h`) | Locate the actual header with `find "C:/Program Files/Epic Games/UE_5.7/Engine/Source" -name <header>.h` |
| Schema not picked up by `help()` | Step 7 (kill MCP) skipped | Kill MCP server |
| Push rejected on main | Repo policy: PRs only | Create feature branch, push, open PR via `gh pr create` |

## Reference: existing patterns

| Need | Look at |
|---|---|
| Minimal new handler class | `SpirrowBridgeLevelCommands.h/.cpp` (smallest, most modern) |
| LSB / blueprint dual-target | `BlueprintNodeCoreCommands` + `CommonUtils::ResolveTargetBlueprint` |
| Property write with class/struct support | `CommonUtils::SetObjectProperty` |
| Class name resolution (bare/U/full path) | `CommonUtils::FindClassByNameAnywhere` |
| Batch operation w/ partial success | `LevelCommands::HandleSetWorldProperties` (applied/failed arrays) |
