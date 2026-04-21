---
name: ue-hud-bootstrap
description: Bootstrap an UMG HUD via Spirrow-UnrealWise MCP — create the WidgetBlueprint, lay out common elements (text/progressbar/buttons in a vertical box), and wire it into the player BP's BeginPlay so it shows on viewport. Use when the user asks for a HUD / heads-up display / on-screen UI.
---

# HUD bootstrap (Spirrow-UnrealWise)

End-to-end HUD: WidgetBlueprint asset + layout + auto-add to viewport on game start.

## When to use

- 「ヘルスバーと弾数表示の HUD 作って」
- 「Pause メニュー UI 作って」
- 「タイトル画面の Widget 作って」

If the user wants a **GameMode-managed** HUD (`AHUD` C++ class), this skill is wrong — UMG WidgetBlueprints are the modern path and what these MCP commands target.

## Inputs to gather

| Param | Required | Notes |
|---|---|---|
| `widget_name` | ✅ | PascalCase. Conventional prefix `WBP_` (e.g. `WBP_HUD_Game`) |
| `path` | ❌ | default `/Game/UI` |
| Layout root | ❌ | usually `VerticalBox` (stacked) or `HorizontalBox` (row). Default: VerticalBox |
| Elements | ❌ | List of (kind, name) — common: `TextBlock` (label), `ProgressBar` (health/ammo), `Button`, `Image`, `EditableTextBox` |
| Bind to player BP | ❌ | If yes, need the player BP's name + path to add `add_widget_to_viewport` on BeginPlay |

## Standard recipe

```python
# 1. Create the WidgetBlueprint asset
umg_widget(command="create_umg_widget_blueprint", params={
    "widget_name": "<widget_name>",
    "path": "<path>",  # default /Game/UI
    "rationale": "<why this HUD exists>"
})

# 2. Add a root layout container
umg_layout(command="add_vertical_box_to_widget", params={
    "widget_name": "<widget_name>",
    "vbox_name": "RootVBox",
    "path": "<path>"
})

# 3. Add elements (each goes into RootVBox by default if it's the only container)
umg_widget(command="add_text_block_to_widget", params={
    "widget_name": "<widget_name>",
    "text_name": "HealthLabel",
    "text": "Health",
    "path": "<path>"
})
umg_widget(command="add_progressbar_to_widget", params={
    "widget_name": "<widget_name>",
    "progressbar_name": "HealthBar",
    "percent": 1.0,
    "path": "<path>"
})
# ... repeat for each element

# 4. (Optional) Add a variable to drive the bar at runtime
umg_variable(command="add_widget_variable", params={
    "widget_name": "<widget_name>",
    "variable_name": "CurrentHealth",
    "variable_type": "float",
    "default_value": 1.0,
    "path": "<path>"
})

# 5. Compile the widget BP
blueprint(command="compile_blueprint", params={"blueprint_name": "<widget_name>", "path": "<path>"})

# 6. Wire into player BP's BeginPlay (only if user wants auto-display)
blueprint_node(command="add_blueprint_event_node", params={
    "blueprint_name": "<player_bp>",
    "event_name": "BeginPlay",
    "path": "<player_bp_path>"
})
# Then add a CreateWidget + AddToViewport chain. The high-level shortcut:
umg_widget(command="add_widget_to_viewport", params={
    "blueprint_name": "<player_bp>",
    "widget_class": "<widget_name>",   # the WBP we just created
    "path": "<player_bp_path>"
})
blueprint(command="compile_blueprint", params={"blueprint_name": "<player_bp>", "path": "<player_bp_path>"})
```

## Common variants

### Health + Ammo HUD
```python
umg_widget(command="create_umg_widget_blueprint", params={"widget_name": "WBP_HUD_Game", "path": "/Game/UI"})
umg_layout(command="add_vertical_box_to_widget", params={"widget_name": "WBP_HUD_Game", "vbox_name": "Root", ...})
umg_widget(command="add_progressbar_to_widget", params={"widget_name": "WBP_HUD_Game", "progressbar_name": "Health", ...})
umg_widget(command="add_text_block_to_widget", params={"widget_name": "WBP_HUD_Game", "text_name": "AmmoLabel", "text": "30 / 90", ...})
```

### Pause menu (Buttons)
```python
umg_widget(command="create_umg_widget_blueprint", params={"widget_name": "WBP_PauseMenu", ...})
umg_layout(command="add_vertical_box_to_widget", params={"widget_name": "WBP_PauseMenu", "vbox_name": "Root", ...})
umg_widget(command="add_button_to_widget", params={"widget_name": "WBP_PauseMenu", "button_name": "ResumeButton", "label": "Resume", ...})
umg_widget(command="add_button_to_widget", params={"widget_name": "WBP_PauseMenu", "button_name": "QuitButton", "label": "Quit", ...})
# Buttons need OnClicked events — wire via umg_variable or blueprint_node
```

## Pitfalls

1. **Layout container 必須** — root に何も置かずに add_text_block すると「どこに親付けされるか不明確」になり想定外の配置になる。先に `add_vertical_box_to_widget` 等
2. **コンパイル忘れ** — Widget BP もコンパイル必要 (`blueprint(command="compile_blueprint", ...)` を共用)
3. **add_widget_to_viewport の widget_class** — 完全パスでなく、widget BP 名 (例: `WBP_HUD_Game`) を渡せば内部で path 解決
4. **Variable バインド** — `add_widget_variable` で変数を作っただけでは ProgressBar に紐付かない。`umg_variable` の追加コマンド (例: `set_widget_property_binding`) で property binding が必要 (該当コマンドが無ければ Designer で手動)
5. **path のデフォルト** — Widget は `/Game/UI` 慣例だがプロジェクト規約と違うなら明示
6. **Pause メニュー時の Input Mode** — Add to viewport だけでは入力不可。BP 側で `Set Input Mode UI Only` + `Set Game Paused` を組み合わせる必要あり (この skill のスコープ外、別途 blueprint_node で組む)

## After scaffold — what to suggest next

- 「Health の動的更新?」→ Player BP に変数バインド or PropertyBinding (Designer)
- 「ボタンクリック処理?」→ `umg_variable(command="add_widget_function_to_event", ...)` 等で OnClicked
- 「アニメーション付ける?」→ `umg_animation(command="create_widget_animation", ...)` 系列
- 「複雑レイアウト?」→ `umg_layout` の HBox / Overlay / ScrollBox を追加し reparent

## Reference docs

- `docs/SPIRROW_CHEATSHEET.md` § umg_widget / umg_layout / umg_variable / umg_animation
- `CLAUDE.md` § "Spirrow-UnrealWise の使い方"
