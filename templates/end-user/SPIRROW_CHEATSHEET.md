# Spirrow-UnrealWise — End-User Cheatsheet

> **For**: TrapxTrapCpp 開発者がゲーム制作中に Spirrow-UnrealWise MCP を呼ぶ時の早見表。
>
> **Companion**: `.claude/skills/ue-*-bootstrap/SKILL.md` (具体的タスク手順) · `CLAUDE.md` § "Spirrow-UnrealWise の使い方" (常時ルール)
>
> **As of**: Spirrow-UnrealWise v0.9.9 — 25 MCP tools, 158 commands, UE 5.7

---

## メタツールパターン (重要)

すべてのコマンドは以下の形式で呼ぶ:

```python
<category>(command="<command_name>", params={"key": value, ...})
```

カテゴリは単一の MCP ツール、`command` でディスパッチする「メタツール」方式。例:
```python
editor(command="spawn_actor", params={"name": "MyLight", "type": "PointLight", "location": [0, 0, 200]})
blueprint(command="create_blueprint", params={"name": "BP_Enemy", "parent_class": "Character"})
```

**パラメータが分からない時は必ず `help` を先に**:
```python
help(category="blueprint")                          # カテゴリ内コマンド一覧
help(category="blueprint", command="create_blueprint")  # 特定コマンドの params 詳細
```

---

## 25 メタツール一覧

| メタツール | 主用途 | コマンド数 | 代表的コマンド |
|---|---|---|---|
| `editor` | アクター操作・レベル作成・WorldSettings | 17 | `spawn_actor` / `create_level` / `save_current_level` / `set_world_properties` |
| `blueprint` | BP 作成・コンパイル・プロパティ・DataAsset | 21 | `create_blueprint` / `add_component_to_blueprint` / `compile_blueprint` |
| `blueprint_node` | BP グラフのノード操作 | 24 | `add_blueprint_event_node` / `connect_blueprint_nodes` / `add_branch_node` |
| `umg_widget` | Widget BP・UI 要素 (v0.9.7: 全 add_*_to_widget に parent_name 対応) | 18 | `create_umg_widget_blueprint` / `add_text_to_widget` / `add_border_to_widget` / `add_widget_to_viewport` |
| `umg_layout` | UMG レイアウト (v0.9.7: reparent 整合性強化 + parent_name scope 対応) | 6 | `add_vertical_box_to_widget` / `add_widget_switcher_to_widget` / `reparent_widget_element` / `set_widget_slot_property` |
| `umg_variable` | Widget 変数・関数・イベント | 5 | `add_widget_variable` / `add_widget_function` |
| `umg_animation` | Widget アニメーション | 4 | `create_widget_animation` / `add_animation_keyframe` |
| `project` | Input Mapping・アセット・テクスチャ | 13 | `create_input_action` / `create_input_mapping_context` / `add_action_to_mapping_context` / `set_default_mapping_context` |
| `ai` | Behavior Tree・Blackboard・BT ノード | 22 | `create_behavior_tree` / `create_blackboard` / `add_bb_key` / `add_bt_selector` / `add_bt_task` |
| `perception` | AIPerceptionComponent | 6 | `configure_ai_perception_sight` / `configure_ai_perception_hearing` |
| `eqs` | Environment Query System | 5 | `create_eqs_query` / `add_eqs_generator` |
| `gas` | Gameplay Ability System | 8 | `add_gameplay_tags` / `create_gameplay_effect` / `create_gameplay_ability` |
| `material` | マテリアル作成 | 6 | `create_material_from_template` |
| `config` | INI 読み書き | 3 | `get_config_value` / `set_config_value` |

**スタンドアロン (10個 + help)**:
- RAG: `search_knowledge` / `add_knowledge` / `list_knowledge` / `delete_knowledge` / `find_relevant_nodes`
- Project Context: `get_project_context` / `update_project_context`
- AI 画像: `get_ai_image_server_status` / `generate_image` / `generate_and_import_texture`
- Help: `help`

---

## 典型ワークフロー

### A. 新規マップ + GameMode 設定

```
editor.create_level → editor.set_world_properties (DefaultGameMode + KillZ 等)
                    → editor.save_current_level
                    → blueprint_node (Level Script Blueprint) ※必要なら
```
詳細: `.claude/skills/ue-level-bootstrap/SKILL.md`

### B. 新規 Blueprint 作成

```
blueprint.create_blueprint
  ↓ (parent_class: Actor / Character / Pawn / GameModeBase 等)
blueprint.add_component_to_blueprint × N
  ↓
blueprint_node.add_blueprint_event_node (BeginPlay 等)
  ↓
blueprint.compile_blueprint  ★コンパイル必須
```
詳細: `.claude/skills/ue-blueprint-scaffold/SKILL.md`

### C. UMG HUD

```
umg_widget.create_umg_widget_blueprint
  ↓
umg_layout.add_vertical_box_to_widget (root)
  ↓
umg_widget.add_text_to_widget / add_progressbar_to_widget × N
  ↓
umg_variable.add_widget_variable (動的バインド用)
  ↓
blueprint.compile_blueprint (widget BP)
  ↓
umg_widget.add_widget_to_viewport (player BP の BeginPlay に挿入)
  ↓
blueprint.compile_blueprint (player BP)
```
詳細: `.claude/skills/ue-hud-bootstrap/SKILL.md`

### C-2. マルチページメニュー (WidgetSwitcher) 🆕 v0.9.6

```
umg_widget.create_umg_widget_blueprint
  ↓
umg_widget.add_border_to_widget (name="Backdrop", brush_color=[0,0,0,0.8])
  ↓ 背景を全画面ストレッチ
umg_layout.set_widget_slot_property (element_name="Backdrop",
    anchor_min=[0,0], anchor_max=[1,1],
    offset_left=0, offset_top=0, offset_right=0, offset_bottom=0)
  ↓
umg_layout.add_widget_switcher_to_widget (name="Pager", parent_name="Backdrop")
  ↓
umg_layout.add_vertical_box_to_widget (name="Page_Main", parent_name="Pager")
umg_layout.add_vertical_box_to_widget (name="Page_Options", parent_name="Pager")
umg_layout.add_vertical_box_to_widget (name="Page_Credits", parent_name="Pager")
  ↓ 各ページにボタン・テキスト
umg_widget.add_button_to_widget / add_text_to_widget × N
  ↓ ランタイム切替 (Widget BP 関数から)
umg_widget.set_widget_element_property (element_name="Pager",
    property_name="ActiveWidgetIndex", property_value="1")
```

**カスタム UserWidget 親クラス** 🆕 v0.9.6: C++ 派生クラスを親にしたい場合:
```
umg_widget.create_umg_widget_blueprint(
    widget_name="WBP_MainMenu",
    parent_class="/Script/MyGame.MyBaseMenuWidget"  # C++ クラスパス
)

# または既存 BP を親に (継承):
umg_widget.create_umg_widget_blueprint(
    widget_name="WBP_OptionsMenu",
    parent_class="/Game/UI/WBP_MainMenu.WBP_MainMenu_C"
)
```

### D. Enhanced Input

```
project.create_input_action × N (IA_Jump, IA_Move 等)
  ↓
project.create_input_mapping_context (IMC_Default)
  ↓
project.add_action_to_mapping_context × N (key bind)
  ↓
project.set_default_mapping_context (player BP に紐付け)
  ↓
blueprint.compile_blueprint (player BP)

# 入力に反応するロジック:
blueprint_node.add_blueprint_input_action_node (player BP graph に IA_Jump イベントを生やす)
```
詳細: `.claude/skills/ue-input-bootstrap/SKILL.md`

### E. AI (BehaviorTree + Blackboard)

```
ai.create_blackboard → ai.add_bb_key × N
ai.create_behavior_tree
ai.add_bt_selector / add_bt_sequence (composite)
ai.add_bt_task (action ノード)
ai.add_bt_decorator (条件)
ai.add_bt_service (定期処理)
```

### F. レベルブループリント (LSB) 編集

通常 BP 用コマンドの多くは `target_type="level_blueprint"` でレベルスクリプトに切り替え可能 (v0.9.3+):

```python
blueprint_node(command="add_blueprint_event_node", params={
    "target_type": "level_blueprint",
    "level_path": "/Game/Maps/MyMap",  # 省略時は現在開いているレベル
    "event_name": "BeginPlay"
})
```

対応コマンド: `add_blueprint_event_node` / `add_blueprint_function_node` / `connect_blueprint_nodes` / `disconnect_blueprint_nodes` / `find_blueprint_nodes` / `set_node_pin_value` / `delete_blueprint_node` / `move_blueprint_node` / `add_blueprint_variable` / `add_variable_get_node` / `add_variable_set_node` / `add_blueprint_self_reference` / `add_branch_node` / `add_sequence_node` / `add_delay_node` / `add_forloop_with_break_node` / `add_print_string_node` / `add_math_node` / `add_comparison_node` / `add_external_property_set_node` / `add_external_property_get_node` / `compile_blueprint` / `get_blueprint_graph`

---

## よく使うコマンド トップ 30

| # | コマンド | 用途 |
|---|---|---|
| 1 | `editor.spawn_actor` | StaticMeshActor / Light / Camera / Volume を配置 |
| 2 | `editor.spawn_blueprint_actor` | 既存 BP のインスタンスをレベルに配置 |
| 3 | `editor.set_actor_transform` | 位置・回転変更 |
| 4 | `editor.set_actor_property` | UPROPERTY 任意設定 |
| 5 | `editor.create_level` | 新規 .umap |
| 6 | `editor.save_current_level` | 現レベル保存 |
| 7 | `editor.open_level` | 既存 .umap 開く |
| 8 | `editor.set_world_properties` | GameMode Override / KillZ 等 |
| 9 | `blueprint.create_blueprint` | BP クラス作成 |
| 10 | `blueprint.add_component_to_blueprint` | コンポーネント追加 |
| 11 | `blueprint.compile_blueprint` | コンパイル (★忘れがち) |
| 12 | `blueprint.set_blueprint_property` | プロパティ値設定 |
| 13 | `blueprint.set_class_property` | TSubclassOf プロパティ設定 |
| 14 | `blueprint.create_data_asset` | DataAsset インスタンス作成 |
| 15 | `blueprint.set_data_asset_property` | DA プロパティ書込み (TMap 対応) |
| 16 | `blueprint.scan_project_classes` | 既存 BP/C++ クラス一覧 |
| 17 | `blueprint_node.add_blueprint_event_node` | BeginPlay/Tick 等 |
| 18 | `blueprint_node.add_blueprint_function_node` | 関数呼び出しノード |
| 19 | `blueprint_node.connect_blueprint_nodes` | ピン接続 |
| 20 | `blueprint_node.add_blueprint_variable` | BP 変数作成 |
| 21 | `blueprint_node.set_node_pin_value` | デフォルト値設定 |
| 22 | `umg_widget.create_umg_widget_blueprint` | Widget BP |
| 23 | `umg_widget.add_text_to_widget` | TextBlock |
| 24 | `umg_widget.add_progressbar_to_widget` | ProgressBar |
| 25 | `umg_widget.add_widget_to_viewport` | 画面表示 |
| 26 | `umg_layout.add_vertical_box_to_widget` | VBox レイアウト |
| 27 | `project.create_input_action` | Enhanced Input IA |
| 28 | `project.add_action_to_mapping_context` | Key bind |
| 29 | `ai.create_behavior_tree` | BT アセット |
| 30 | `ai.add_bt_task` | BT タスクノード |

---

## エラーハンドリング戦略

### `Unknown command: xxx`
→ Spirrow-UnrealWise 側 (server) の問題。タイポか、未実装か、サーバが古い。
- スペル確認 (`help` で正規名を見る)
- Spirrow-UnrealWise のバージョン確認 (リポの README で対応コマンド一覧)
- v0.9.x で追加されたばかりの機能なら server 側が古い可能性

### `<param> is required` / `Invalid params`
→ 必須パラメータ不足。`help(category, command)` で正しい params を確認

### `Blueprint not found: <name>`
→ 名前 or path のミス。`blueprint(command="scan_project_classes", params={"class_type": "blueprint"})` で実在確認

### `Compilation failed`
→ BP の整合性エラー。最近追加したコンポーネント/変数/ノードを疑う。`get_blueprint_graph` でノード状態確認

### Class picker 系 (`set_class_property` など) で「Class not found」
→ クラスパスのフォーマット確認:
- C++: `/Script/Engine.Character` (U/A プレフィックス無し)
- BP: `/Game/Blueprints/BP_Foo.BP_Foo_C` (末尾 `_C` が必須)

### 部分成功する系 (`set_world_properties`, `batch_set_properties`)
→ レスポンスの `applied: [...]` と `failed: [{property, error}]` を両方見る。`success=true` でも `failed` に項目があり得る

---

## エディタ確認すべきタイミング

MCP 経由の変更は便利だが、視覚確認が必要なケース:

- **Component 配置** — `add_component_to_blueprint` 後は SCS タブで親子関係 / Transform 確認
- **WorldSettings 変更** — Window → World Settings パネルで反映確認
- **UMG 配置** — Designer タブでレイアウト確認 (特に Modifier/Anchor 周り)
- **Input** — Project Settings → Enhanced Input、または PIE で実際に試す
- **BP コンパイル後** — Compiler Results に warning/error が出ていないか

---

## Spirrow-UnrealWise の制約 / 未対応

知っておくと事故が減る:

- **Modifier 付き Input bind** (Negate / Swizzle 等) — 簡易 key bind は可、複雑なものは UE Editor で詰める
- **UMG Property Binding** — `add_widget_variable` で変数は作れるが、ProgressBar の Percent 等への動的 binding は限定的。Designer で手動補完が必要なケースあり
- **Material グラフ編集** — `material.create_material_from_template` でテンプレート生成のみ。ノードグラフ細部は UE Editor
- **Animation Blueprint** — 専用コマンド無し。`blueprint(command="create_blueprint", params={"parent_class": "AnimInstance", ...})` で素体作成は可
- **Sequencer / Cinematic** — 未対応
- **Niagara** — 未対応
- **Data Validation** — `blueprint.compile_blueprint` の結果を必ず確認
