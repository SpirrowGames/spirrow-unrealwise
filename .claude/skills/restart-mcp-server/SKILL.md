---
name: restart-mcp-server
description: Kill the running SpirrowBridge MCP server processes (Python + uv) so the next MCP tool call spawns a fresh process with reloaded code. Use after any edit under Python/ — especially Python/tools/*_meta.py, command_schemas.py, meta_utils.py, help_tool.py, or unreal_mcp_server.py.
---

# Restart MCP server

## Why

The MCP server reads Python source **once on process startup** to build its `COMMANDS` dict (in `*_meta.py` `register_*_meta_tool()`). Python `import` does **not** hot-reload edited files. So the moment you change a Python file, the running process is stale: new commands you added return `Unknown command: <name>` to the user, and schema changes don't show up in `help()`.

Killing the process is the fix. The MCP client (Claude Code / Claude Desktop) auto-spawns a new server on the next tool call.

## When to run

After editing any of:
- `Python/tools/*_meta.py` — meta-tool COMMANDS dict, docstring, pre-processing
- `Python/tools/command_schemas.py` — schema definitions
- `Python/tools/meta_utils.py`, `help_tool.py` — shared infrastructure
- `Python/unreal_mcp_server.py` — server bootstrap / tool registration

**Not needed** for: C++ changes (those need `build_editor.bat` instead), markdown/docs, configuration files.

## Procedure (Windows)

### Step 1 — Enumerate

```bash
powershell -NoProfile -Command "Get-CimInstance Win32_Process | Where-Object { \$_.Name -in @('python.exe', 'uv.exe') -and \$_.CommandLine -match 'unreal_mcp_server|spirrow-unrealwise' } | Select-Object ProcessId, ParentProcessId, Name | Format-Table -AutoSize"
```

You'll typically see a tree per active client (Claude Code / Desktop):

```
ProcessId  ParentProcessId  Name
---------  ---------------  ----
NNNNN      <client>         uv.exe         ← parent
NNNNN      <uv pid>         python.exe     ← child
NNNNN      <python pid>     python.exe     ← grandchild (uv-managed venv)
```

If multiple MCP clients are open you'll see multiple trees.

### Step 2 — Kill

Kill the **parent `uv.exe` PID(s)**. Children cascade automatically.

```bash
cmd.exe //C "taskkill /F /PID <uv_pid_1> /PID <uv_pid_2>"
```

### Step 3 — Verify

```bash
powershell -NoProfile -Command "Get-CimInstance Win32_Process | Where-Object { \$_.Name -in @('python.exe', 'uv.exe') -and \$_.CommandLine -match 'unreal_mcp_server|spirrow-unrealwise' } | Measure-Object | Select-Object -ExpandProperty Count"
```

Expected: `0`. If non-zero, kill the remaining PIDs individually with `taskkill /F /PID <pid>`.

## After kill

Don't tell the user to restart anything. The next MCP tool call (whatever they ask Claude to do next) will trigger the client to spawn a fresh server, which reads the latest Python source. Just confirm in your reply that the kill is done, e.g.:

> MCP サーバー全プロセス kill 済 (次のツール呼び出しで自動再 spawn)

## Edge cases

- **No matching processes**: Either the user has no MCP client open (fine — nothing to do), or the regex missed a non-standard command line. Try the broader grep `\$_.CommandLine -match 'mcp|unreal'` to inspect.
- **Persistent children after parent kill**: Run a second `taskkill` round on remaining PIDs. This sometimes happens if the parent was already orphaned.
- **Permission denied**: Check that you're running as the same user that started the server. SYSTEM-owned processes need elevated shell.

## Reference

The mandatory restart rule is documented in:
- `CLAUDE.md` § "⚠️ 必須運用ルール" — the canonical statement
- `AGENTS.md` § "⚠️ 必須運用ルール" — same content, contributor-facing
