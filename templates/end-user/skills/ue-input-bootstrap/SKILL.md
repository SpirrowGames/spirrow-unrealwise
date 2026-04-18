---
name: ue-input-bootstrap
description: Set up Enhanced Input via Spirrow-UnrealWise MCP — create InputAction(s), an InputMappingContext, bind keys, and set the IMC as default on the player BP so input works immediately. Use when the user asks to add controls / keybindings / input handling.
---

# Input bootstrap (Spirrow-UnrealWise)

End-to-end Enhanced Input setup. UE 5.x dropped legacy ActionMappings — Enhanced Input is the only path now.

## When to use

- 「ジャンプの入力追加して」
- 「移動と射撃の入力割り当てて」
- 「Pause キー追加」

If the user just wants to bind one new action to an existing IMC, skip steps 1-3 and go straight to `add_action_to_mapping_context`.

## Inputs to gather

| Param | Required | Notes |
|---|---|---|
| Action name(s) | ✅ | PascalCase. Conventional prefix: `IA_` (e.g. `IA_Jump`, `IA_Move`) |
| Action value type | ✅ per action | `Bool` (button), `Axis1D` (analog/scroll), `Axis2D` (movement/look), `Axis3D` (rare) |
| Key bindings | ✅ per action | UE FKey strings: `SpaceBar`, `LeftMouseButton`, `W`, `A`, `S`, `D`, `Gamepad_FaceButton_Bottom`, etc. Multiple keys per action OK |
| IMC name | ✅ | PascalCase, prefix `IMC_` (e.g. `IMC_Default`) |
| Player BP | ❌ (recommended) | If yes, IMC will be set as default on BeginPlay |
| `path` | ❌ | default `/Game/Input` |

## Standard recipe

```python
# 1. Create one or more InputActions
project(command="create_input_action", params={
    "action_name": "IA_Jump",
    "value_type": "Bool",   # Bool / Axis1D / Axis2D / Axis3D
    "path": "/Game/Input"
})
project(command="create_input_action", params={
    "action_name": "IA_Move",
    "value_type": "Axis2D",
    "path": "/Game/Input"
})
# ... repeat for each action

# 2. Create the InputMappingContext
project(command="create_input_mapping_context", params={
    "context_name": "IMC_Default",
    "path": "/Game/Input"
})

# 3. Bind keys to actions inside the IMC
project(command="add_action_to_mapping_context", params={
    "context_name": "IMC_Default",
    "action_name": "IA_Jump",
    "key": "SpaceBar",
    "path": "/Game/Input"
})
# Repeat per (action, key) pair. For Axis2D actions like IA_Move, bind 4 keys (W/A/S/D) with appropriate Modifiers (Negate / Swizzle) — that's typically done in UE editor; this MCP path does basic key binding only

# 4. Set IMC as default on the player BP (so it activates on possess)
project(command="set_default_mapping_context", params={
    "blueprint_name": "<player_bp>",
    "context_name": "IMC_Default",
    "priority": 0,
    "blueprint_path": "<player_bp_path>"
})

# 5. Compile the player BP
blueprint(command="compile_blueprint", params={
    "blueprint_name": "<player_bp>",
    "path": "<player_bp_path>"
})
```

## Common variants

### Movement (WASD + look)
```python
project(command="create_input_action", params={"action_name": "IA_Move", "value_type": "Axis2D", ...})
project(command="create_input_action", params={"action_name": "IA_Look", "value_type": "Axis2D", ...})
project(command="create_input_mapping_context", params={"context_name": "IMC_Player", ...})
# WASD bindings (Y axis: W=+1 / S=-1, X axis: D=+1 / A=-1) — basic key bind:
project(command="add_action_to_mapping_context", params={"context_name": "IMC_Player", "action_name": "IA_Move", "key": "W", ...})
# Note: full WASD with Modifiers (Negate / Swizzle) is more complex; surface to user that fine-tuning may need UE editor
project(command="add_action_to_mapping_context", params={"context_name": "IMC_Player", "action_name": "IA_Look", "key": "Mouse2D", ...})
```

### Single action add to existing IMC
```python
# Skip 1-3. Just:
project(command="create_input_action", params={"action_name": "IA_Pause", "value_type": "Bool", ...})
project(command="add_action_to_mapping_context", params={
    "context_name": "IMC_Default",  # existing
    "action_name": "IA_Pause",
    "key": "Escape",
    "path": "/Game/Input"
})
```

## Wiring input to BP logic

After bootstrap, the user can react to input in the player BP via:

```python
# Spawn an InputAction event node in the player BP graph
blueprint_node(command="add_blueprint_input_action_node", params={
    "blueprint_name": "<player_bp>",
    "action_name": "IA_Jump",
    "path": "<player_bp_path>"
})
# → returns a node with Triggered/Started/Completed pins; chain function calls from these
```

## Pitfalls

1. **Enhanced Input plugin 必須** — UE 5.x はデフォルト有効だがプロジェクトによっては無効化されている。事前確認 (Edit > Plugins > Enhanced Input)
2. **`set_default_mapping_context` は player BP に IMC を bind するだけ** — Pawn が possess されないと IMC は活性化しない。GameMode の `DefaultPawnClass` が player BP を指していること必須
3. **Axis2D の方向性** — 単純 key bind では W = +Y、S は別途 Negate Modifier 必要。MCP コマンドでは Modifier 設定は限定的、UE 側で詰めることを user に告知
4. **`key` の文字列フォーマット** — UE FKey 名 (例: `SpaceBar`, `LeftMouseButton`, `Gamepad_FaceButton_Bottom`)。`Space` (×) ではなく `SpaceBar` (○)
5. **path 慣例** — `/Game/Input` 推奨、プロジェクト規約と違えば明示
6. **複数 IMC** — 優先度 (`priority`) で重ね掛け可能。Pause 中だけ別 IMC など

## After bootstrap — what to suggest next

- 「IA_Jump で Character の Jump() 呼ぶ?」→ `blueprint_node(command="add_blueprint_input_action_node", ...)` + `add_blueprint_function_node`
- 「Move を Character の AddMovementInput に流す?」→ 同上 + ベクトル分解
- 「Pause メニュー表示?」→ `ue-hud-bootstrap` で WBP 用意 → Set Input Mode UI Only / Set Game Paused

## Reference docs

- `docs/SPIRROW_CHEATSHEET.md` § project (Input Mapping)
- UE 5 Enhanced Input docs: https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine
- `CLAUDE.md` § "Spirrow-UnrealWise の使い方"
