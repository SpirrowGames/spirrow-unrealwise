# SpirrowBridge — Developer Cheatsheet

> **For**: contributors editing `spirrow-unrealwise`. Maps each meta-tool category to the Python entry file and C++ command file(s) so you know **where to add** a new command.
>
> **Companion docs**: [`AGENTS.md`](../AGENTS.md) (workflow rules) · [`Docs/IMPLEMENTATION_SUMMARY.md`](IMPLEMENTATION_SUMMARY.md) (file-level prose) · [`FEATURE_STATUS.md`](../FEATURE_STATUS.md) (user-visible command list)
>
> **Generated against**: v0.9.8 · 25 MCP tools · 158 commands · UE 5.7

---

## How to read

For each meta-tool: which **Python `*_meta.py`** registers it, what **C++ class(es)** implement the handlers, and which **`SpirrowBridge.cpp` ExecuteCommand else-if branch** routes it.

Adding a new command means touching exactly:
1. One Python `*_meta.py` (`COMMANDS` dict + docstring)
2. One Python entry in `command_schemas.py`
3. One C++ `SpirrowBridge*Commands.{h,cpp}` (declare + implement + dispatch in `HandleCommand`)
4. **`SpirrowBridge.cpp::ExecuteCommand`** — add command name to the OR-chain for the matching else-if branch
5. Build + kill MCP + docs (see `add-mcp-command` skill)

---

## Meta-tool → file map

### Core editor / level

| Meta-tool | Commands | Python | C++ class | C++ files |
|---|---|---|---|---|
| `editor` | 17 | `editor_meta.py` | `FSpirrowBridgeEditorCommands` + `FSpirrowBridgeLevelCommands` | `SpirrowBridgeEditorCommands.cpp` (actor / spawn / properties / asset rename / viewport), `SpirrowBridgeLevelCommands.cpp` (create_level / save_current_level / open_level / get_world_settings / set_world_properties) |

> Note: `editor` is the only meta-tool routed to **two** handler classes. The `else if` in `SpirrowBridge.cpp` has separate branches (one for actor commands → `EditorCommands`, one for level/worldsettings commands → `LevelCommands`).

### Blueprint family

| Meta-tool | Commands | Python | C++ router | C++ implementation files |
|---|---|---|---|---|
| `blueprint` | 21 | `blueprint_meta.py` | `FSpirrowBridgeBlueprintCommands` (`SpirrowBridgeBlueprintCommands.cpp`, ~1.7 KB) | `SpirrowBridgeBlueprintCoreCommands.cpp` (create / compile / spawn / duplicate / graph), `SpirrowBridgeBlueprintComponentCommands.cpp` (components / static mesh / physics), `SpirrowBridgeBlueprintPropertyCommands.cpp` (class scan / property arrays / DataAsset read) |
| `blueprint_node` | 24 | `node_meta.py` | `FSpirrowBridgeBlueprintNodeCommands` (`SpirrowBridgeBlueprintNodeCommands.cpp`, ~1.7 KB) | `SpirrowBridgeBlueprintNodeCoreCommands.cpp` (connect / find / event / function), `SpirrowBridgeBlueprintNodeVariableCommands.cpp` (variable get/set, external UPROPERTY, get-subsystem), `SpirrowBridgeBlueprintNodeControlFlowCommands.cpp` (branch / sequence / delay / loop / math / comparison / print) |

### UMG family

| Meta-tool | Commands | Python | C++ router | C++ implementation files |
|---|---|---|---|---|
| `umg_widget` | 19 | `umg_meta.py` | `FSpirrowBridgeUMGWidgetCommands` (`SpirrowBridgeUMGWidgetCommands.cpp`, ~1.5 KB) | `SpirrowBridgeUMGWidgetCoreCommands.cpp` (create widget / viewport / anchor + parent_class generalized in v0.9.6), `SpirrowBridgeUMGWidgetBasicCommands.cpp` (text / image / progressbar / **border** 🆕 v0.9.6), `SpirrowBridgeUMGWidgetInteractiveCommands.cpp` (button / slider / checkbox / etc.) |
| `umg_layout` | 6 | `umg_meta.py` | — | `SpirrowBridgeUMGLayoutCommands.cpp` (vbox / hbox / **widget_switcher** 🆕 v0.9.6 / scrollbox / reparent / get-elements / set-slot [extended with anchor_min/max + LTRB offsets 🆕 v0.9.6] / remove) |
| `umg_variable` | 5 | `umg_meta.py` | — | `SpirrowBridgeUMGVariableCommands.cpp` (widget variables / array vars / functions / events) |
| `umg_animation` | 4 | `umg_meta.py` | — | `SpirrowBridgeUMGAnimationCommands.cpp` (create animation / track / keyframe / list) |

### AI family

| Meta-tool | Commands | Python | C++ router | C++ implementation files |
|---|---|---|---|---|
| `ai` | 22 | `ai_meta.py` | `FSpirrowBridgeAICommands` (`SpirrowBridgeAICommands.cpp`, ~1.5 KB) | `SpirrowBridgeAICommands_Blackboard.cpp` (blackboard ops), `SpirrowBridgeAICommands_BehaviorTree.cpp` (BT asset ops), `SpirrowBridgeAICommands_BTNodeCreation.cpp` (BT node spawn + auto-position), `SpirrowBridgeAICommands_BTNodeOperations.cpp` (BT node edit / repair), `SpirrowBridgeAICommands_BTNodeHelpers.cpp` (shared helpers) |
| `perception` | 6 | `perception_meta.py` | — | `SpirrowBridgeAIPerceptionCommands.cpp` (sight / hearing / damage / config / properties) |
| `eqs` | 5 | `eqs_meta.py` | — | `SpirrowBridgeEQSCommands.cpp` (query / generator / test) |

### Project / Config / Material / GAS

| Meta-tool | Commands | Python | C++ implementation files |
|---|---|---|---|
| `project` | 13 | `project_meta.py` | `SpirrowBridgeProjectCommands.cpp` (input mapping / IA / IMC / asset utils / textures / find references) |
| `config` | 3 | `config_meta.py` | `SpirrowBridgeConfigCommands.cpp` (INI read / write / list) |
| `material` | 6 (4 RAG + 2 C++) | `material_meta.py` (async) | `SpirrowBridgeMaterialCommands.cpp` (create from template) — RAG-only commands hit the standalone material RAG service |
| `gas` | 8 | `gas_meta.py` | `SpirrowBridgeGASCommands.cpp` (gameplay tags / effects / abilities / attribute set / replication) |

---

## Standalone tools (no C++ bridge)

These bypass the meta-tool / C++ pipeline entirely.

| Tool | Python file | Backend |
|---|---|---|
| `search_knowledge` / `add_knowledge` / `list_knowledge` / `delete_knowledge` | `knowledge_tools.py` | RAG service |
| `find_relevant_nodes` | `rag_tools.py` | RAG service |
| `get_project_context` / `update_project_context` | `rag_tools.py` | RAG service |
| `get_ai_image_server_status` / `generate_image` / `generate_and_import_texture` | `image_gen_tools.py` | AI image server (with optional UE import via SpirrowBridge) |
| `help` | `help_tool.py` | reads `command_schemas.py` |

---

## Critical helpers in `CommonUtils`

When implementing a new C++ handler, **prefer these over reinventing**:

| Need | API | Location |
|---|---|---|
| Standard error response | `CreateErrorResponse(message)` / `CreateErrorResponse(code, message)` / `CreateErrorResponse(code, message, details)` | `CommonUtils.h:106-112` |
| Standard success response | `CreateSuccessResponse(Data)` (wraps under `"data"` key) | `CommonUtils.h:115` |
| Param validation | `ValidateRequiredString(Params, name, out)`, `GetOptionalString/Int/Bool/Float`, etc. | `CommonUtils.h` |
| Reflection-based property write | `SetObjectProperty(Object, name, jsonValue, outErr)` — handles `FClassProperty` (TSubclassOf), primitives, structs, enums, objects | `CommonUtils.cpp:949` |
| Class name resolution | `FindClassByNameAnywhere(name)` — handles bare name / U+A prefix / `/Script/Engine.X` / full path | `CommonUtils.cpp:~1472` (v0.9.3) |
| Blueprint validation by name+path | `ValidateBlueprint(name, path, outBP)` | `CommonUtils.h` |
| Blueprint OR Level Script Blueprint resolution | `ResolveTargetBlueprint(Params, outBP)` — checks `target_type=="level_blueprint"` and optional `level_path`, falls back to `ValidateBlueprint` for normal BPs | `CommonUtils.cpp:~1608` (v0.9.3) |
| Blueprint compile (crash-safe) | `SafeCompileBlueprint(BP)` — replaces `RegenerateSkeletonOnly + CompileBlueprint` to avoid SCS-after-add crash | `CommonUtils.cpp` (v0.8.x bugfix) |
| Editor world | `GEditor->GetEditorWorldContext().World()` | UE API direct, no wrapper |
| Asset existence check | `UEditorAssetLibrary::DoesAssetExist(path)` | UE API direct |

---

## Build & restart cheats

```bash
# Build editor (UE 5.7). UE Editor must be CLOSED.
cd "C:/Users/owner/Documents/Unreal Projects/spirrow-unrealwise"
cmd.exe //C ".\\build_editor.bat"
# Expect: Result: Succeeded

# Kill MCP server (after Python edits) — see restart-mcp-server skill
powershell -NoProfile -Command "Get-CimInstance Win32_Process | Where-Object { \$_.Name -in @('python.exe','uv.exe') -and \$_.CommandLine -match 'unreal_mcp_server|spirrow-unrealwise' } | Select-Object ProcessId,Name,ParentProcessId | Format-Table"
cmd.exe //C "taskkill /F /PID <uv_parent_pid>"
```

---

## Routing branches in `SpirrowBridge.cpp::ExecuteCommand`

Quick lookup for which OR-chain to extend:

```
get_actors_in_level / spawn_actor / ...   → EditorCommands
create_level / save_current_level / open_level / get_world_settings / set_world_properties → LevelCommands
create_blueprint / compile_blueprint / ... → BlueprintCommands (router → splits to Core/Component/Property)
add_blueprint_event_node / connect_... / ... → BlueprintNodeCommands (router → splits to Core/Variable/ControlFlow)
create_umg_widget_blueprint / add_text_to_widget / ... → UMGWidgetCommands (router → splits)
add_vertical_box_to_widget / ...           → UMGLayoutCommands
create_widget_animation / ...              → UMGAnimationCommands
add_widget_variable / ...                  → UMGVariableCommands
create_input_action / ...                  → ProjectCommands
add_gameplay_tags / ...                    → GASCommands
create_material / ...                      → MaterialCommands
get_config_value / ...                     → ConfigCommands
create_behavior_tree / add_bb_key / ...    → AICommands (router → splits to Blackboard/BT/BTNode*)
configure_ai_perception_sight / ...        → AIPerceptionCommands
register_eqs_query / ...                   → EQSCommands
```

If your new command doesn't fit any existing branch, you're either creating a new meta-tool (separate workflow) or you misclassified — re-pick from the table above.
