# spirrow-unrealwise 機能ステータス

> **バージョン**: v0.9.8 (JSON Property Value — set_widget_element_property が array / object / number / bool 受理)
> **ステータス**: Beta
> **最終更新**: 2026-04-22

---

## アーキテクチャ

**v0.9.1** でメタツール化を実施。161個の個別ツールを **25個** (14メタツール + 1ヘルプ + 10スタンドアロン) に統合。
コンテキスト消費量を ~170K → ~22K tokens に削減。
**v0.9.2** でBTノードの堅牢性向上・自動修復機能を追加。
**v0.9.3** で `blueprint` / `blueprint_node` メタツールに Level Blueprint (LSB) 対応、外部クラスの UPROPERTY Set/Get ノード対応 (explicit API + `add_blueprint_function_node` 透過的 fallback)、typed Get Subsystem ノード、`set_node_pin_value` の Class/Object ピン対応を追加。

### 使い方
```python
# カテゴリ(command="コマンド名", params={...})
editor(command="spawn_actor", params={"name": "MyLight", "type": "PointLight"})

# パラメータ確認
help(category="editor")                              # カテゴリ内コマンド一覧
help(category="editor", command="spawn_actor")       # パラメータ詳細
```

---

## 機能サマリー

### メタツール (14カテゴリ = 148コマンド)

| メタツール | 説明 | コマンド数 | 状態 |
|-----------|------|-----------|------|
| `editor` | Actor操作、トランスフォーム、プロパティ、レベル作成/保存/オープン、WorldSettings 🆕 | 17 | ✅ |
| `blueprint` | BP作成、コンパイル、プロパティ、DataAsset (LSB対応) 🆕 | 21 | ✅ |
| `blueprint_node` | イベント、関数、変数、フロー制御、数学 (LSB + 外部UPROPERTY + typed Subsystem) 🆕 | 24 | ✅ |
| `umg_widget` | テキスト、画像、ボタン、スライダー、Border 等 (全て parent_name 対応 🆕 v0.9.7) | 18 | ✅ |
| `umg_layout` | VBox/HBox、WidgetSwitcher、ScrollBox、リペアレント (reparent 整合性強化 🆕 v0.9.7) | 6 | ✅ |
| `umg_variable` | Widget変数、関数、イベント | 5 | ✅ |
| `umg_animation` | アニメーション、トラック、キーフレーム | 4 | ✅ |
| `project` | Input Mapping、アセット、フォルダ、テクスチャ | 13 | ✅ |
| `ai` | Blackboard、BehaviorTree、BTノード、BT修復 | 22 | ✅ |
| `perception` | AI視覚、聴覚、ダメージ感知 | 6 | ✅ |
| `eqs` | Environment Query System | 5 | ✅ |
| `gas` | Gameplay Tags、Effect、Ability | 8 | ✅ |
| `material` | マテリアルテンプレート、作成 | 6 | ✅ |
| `config` | Unreal Config読み書き | 3 | ✅ |

### スタンドアロンツール (10個 + 1 help)

| ツール | 説明 | 状態 |
|--------|------|------|
| `help` | メタツールのパラメータドキュメント | ✅ |
| `search_knowledge` | RAG知識ベース検索 | ✅ |
| `add_knowledge` | RAGにナレッジ追加 | ✅ |
| `list_knowledge` | ナレッジ一覧 | ✅ |
| `delete_knowledge` | ナレッジ削除 | ✅ |
| `get_project_context` | プロジェクトコンテキスト取得 | ✅ |
| `update_project_context` | プロジェクトコンテキスト更新 | ✅ |
| `find_relevant_nodes` | 関連ノード検索 | ✅ |
| `get_ai_image_server_status` | AI画像サーバー状態 | ✅ |
| `generate_image` | AI画像生成 | ✅ |
| `generate_and_import_texture` | AI画像生成+UEインポート | ✅ |

| | 数 |
|---|---|
| **MCP登録ツール合計** | **25** |
| **内包コマンド合計** | **158** |

---

## 詳細ステータス

### Actor操作 (10)
`get_actors_in_level`, `find_actors_by_name`, `spawn_actor`, `delete_actor`, `set_actor_transform`, `get_actor_properties`, `set_actor_property`, `set_actor_component_property`, `rename_actor`, `get_actor_components`

**spawn_actor 対応タイプ:**
- Basic: `StaticMeshActor`, `PointLight`, `SpotLight`, `DirectionalLight`, `CameraActor`
- Volumes: `NavMeshBoundsVolume`, `TriggerVolume`, `BlockingVolume`, `KillZVolume`, `PhysicsVolume`, `PostProcessVolume`, `AudioVolume`, `LightmassImportanceVolume`

### レベルライフサイクル 🆕 (3)
- `create_level` — 新規 `.umap` をディスクに作成し、エディタを新レベルに切り替え (UEditorLevelLibrary::NewLevel / NewLevelFromTemplate)
  - `template="default"` (UE 5.7 既定 = World Partition 有効)
  - `template="empty"` (`/Engine/Maps/Templates/Empty` = 非WP)
  - `template="/Game/..."` (任意の既存レベルアセットをテンプレートに使用)
  - `overwrite=true` で既存レベル上書き可
- `save_current_level` — 現在エディタで開いているレベルをディスクに保存 (UEditorLoadingAndSavingUtils::SaveCurrentLevel)
- `open_level` — 指定された `.umap` をエディタで開く (UEditorLevelLibrary::LoadLevel)。dirty な状態で呼ぶと UE 側の save dialog が出るため、必要なら事前に `save_current_level` を呼ぶ

```python
editor(command="create_level", params={"name": "MyMap", "path": "/Game/Maps"})
editor(command="save_current_level")
editor(command="open_level", params={"level_path": "/Game/Maps/MyMap"})
```

作成後はそのまま `blueprint_node(command=..., params={"target_type": "level_blueprint", "level_path": "/Game/Maps/MyMap", ...})` で LSB 編集可能。

### WorldSettings 🆕 (2)
現在開いているエディタワールドの `AWorldSettings` (レベルごとのシングルトン) を読み書き。
- `get_world_settings` — `properties` 配列で任意のフィールドを指定、省略時は curated preset (9個) を返す
  - Curated preset: `DefaultGameMode` (UI 上の "GameMode Override") / `DefaultPhysicsVolumeClass` / `KillZ` / `KillZDamageType` / `WorldToMeters` / `GlobalGravityZ` / `TimeDilation` / `bEnableWorldBoundsChecks` / `bEnableWorldComposition`
  - 存在しないプロパティ名は `unknown_properties` 配列に入る (エラーにしない)
- `set_world_properties` — dict で一括設定。`SetObjectProperty` をラップして class picker (`TSubclassOf`) / primitive / struct を統一処理
  - 部分成功 OK — 個別の失敗は `failed` 配列へ、`applied` に成功リスト
  - 1つでも applied があれば level を dirty マーク → `save_current_level` で永続化
  - 全失敗時のみ `success=false`

```python
editor(command="get_world_settings")  # curated preset
editor(command="get_world_settings", params={"properties": ["KillZ", "WorldToMeters"]})
editor(command="set_world_properties", params={"properties": {
    "KillZ": -5000,
    "DefaultGameMode": "/Game/Blueprints/BP_GM_Default.BP_GM_Default_C",
    "bEnableWorldBoundsChecks": True
}})
editor(command="save_current_level")
```

### Blueprint操作 (17)
`create_blueprint`, `spawn_blueprint_actor`, `add_component_to_blueprint`, `set_static_mesh_properties`, `set_component_property`, `set_physics_properties`, `compile_blueprint`, `set_blueprint_property`, `create_data_asset`, `set_class_property`, `set_object_property`, `get_blueprint_properties`, `set_struct_property`, `set_data_asset_property`, `get_data_asset_properties` 🆕, `batch_set_properties`, `find_cpp_function_in_blueprints`

**TMap プロパティサポート** 🆕: `set_data_asset_property` / `batch_set_properties` で `TMap<FName/FString, int32/float/double/bool/FString/FName>` の読み書きに対応
- 全体上書き: `property_name="MapProp"`, `property_value={"Key": value, ...}`
- 個別エントリ: `property_name="MapProp.KeyName"`, `property_value=value`
- エントリ削除: `property_name="MapProp.KeyName"`, `property_value=null`

### BPノードグラフ (24)

- **イベント/関数** (3): `add_blueprint_event_node`, `add_blueprint_input_action_node`, `add_blueprint_function_node`
- **接続/検索/編集** (6): `connect_blueprint_nodes`, `disconnect_blueprint_nodes`, `find_blueprint_nodes`, `set_node_pin_value`, `delete_blueprint_node`, `move_blueprint_node`
- **変数/参照** (5): `add_blueprint_variable`, `add_variable_get_node`, `add_variable_set_node`, `add_blueprint_get_self_component_reference`, `add_blueprint_self_reference`
- **制御フロー/数学** (7): `add_branch_node`, `add_sequence_node`, `add_delay_node`, `add_forloop_with_break_node`, `add_print_string_node`, `add_math_node`, `add_comparison_node`
- **外部UPROPERTY / Subsystem** 🆕 (3): `add_external_property_set_node`, `add_external_property_get_node`, `add_get_subsystem_node`

**add_blueprint_event_node**: BlueprintImplementableEvent オーバーライド対応
**add_blueprint_function_node**: `Set<Prop>`/`Get<Prop>` 関数名が見つからない場合、同名の外部 UPROPERTY Set/Get ノードに自動 fallback 🆕
**set_node_pin_value**: Class/SoftClass/Object/SoftObject/Interface ピンに対応 (`Pin->DefaultObject` + `ReconstructNode`) 🆕

### Level Blueprint対応 🆕 (v0.9.3)
`blueprint` / `blueprint_node` メタツールの以下コマンドは、`target_type="level_blueprint"` パラメータで **Level Script Blueprint** (レベルブループリント) を対象にできます。`blueprint_name` / `path` は無視されます。

- **ノード編集** (`blueprint_node`): `add_blueprint_event_node`, `add_blueprint_function_node`, `connect_blueprint_nodes`, `disconnect_blueprint_nodes`, `find_blueprint_nodes`, `set_node_pin_value`, `delete_blueprint_node`, `move_blueprint_node`, `add_blueprint_variable`, `add_variable_get_node`, `add_variable_set_node`, `add_blueprint_self_reference`, `add_branch_node`, `add_sequence_node`, `add_delay_node`, `add_forloop_with_break_node`, `add_print_string_node`, `add_math_node`, `add_comparison_node`, `add_external_property_set_node` 🆕, `add_external_property_get_node` 🆕
- **BP操作** (`blueprint`): `compile_blueprint`, `get_blueprint_graph`

**level_path** パラメータ (省略可): 特定のレベルアセットを指定 (例: `"/Game/Maps/MyMap"`)。省略時は現在編集中のレベル。

```python
# 現在のレベルのLSBにBeginPlayイベントを追加
blueprint_node(command="add_blueprint_event_node", params={
    "target_type": "level_blueprint",
    "event_name": "ReceiveBeginPlay"
})

# LSBをコンパイル
blueprint(command="compile_blueprint", params={"target_type": "level_blueprint"})
```

### Typed Get Subsystem ノード 🆕 (v0.9.3)
`SubsystemBlueprintLibrary::GetGameInstanceSubsystem(Class)` を経由すると Class ピンが string としてしか渡せず、戻り値の型が narrow されない問題がある。UE標準の `UK2Node_GetSubsystem` は `Initialize(UClass*)` で **クラスを node 生成時に焼き込む** ため、ReturnValue が最初から typed (`UVoxelCollapseSubsystem*` 等) になる。

**新規コマンド**:
- `add_get_subsystem_node` — typed K2Node_GetSubsystem を生成。`subsystem_kind` で 4 種類 (GameInstance / World / Engine / LocalPlayer) に対応:

| subsystem_kind | 生成される K2 node | 要件 (ExpectedBase) |
|---|---|---|
| `GameInstance` (default) | `UK2Node_GetSubsystem` | `UGameInstanceSubsystem` サブクラス |
| `World` | `UK2Node_GetSubsystem` | `UWorldSubsystem` サブクラス |
| `Engine` | `UK2Node_GetEngineSubsystem` | `UEngineSubsystem` サブクラス |
| `LocalPlayer` | `UK2Node_GetSubsystemFromPC` | `ULocalPlayerSubsystem` サブクラス |

```python
# typed subsystem → Cast 不要、下流の Set ノードに直接接続可能
sub = blueprint_node(command="add_get_subsystem_node", params={
    "target_type": "level_blueprint",
    "subsystem_class": "/Script/VoxelRuntime.VoxelCollapseSubsystem",
    "subsystem_kind": "GameInstance",
    "node_position": [400, 0]
})
```

### set_node_pin_value: Class/Object ピン対応 🆕 (v0.9.3)
`set_node_pin_value` が K2 の Class/SoftClass/Object/SoftObject/Interface ピンを正しく扱うようになった。内部でピンカテゴリを判定し、`Pin->DefaultValue` (文字列) と `Pin->DefaultObject` (UObject*) を自動的に使い分ける。

- **Class/SoftClass ピン**: `pin_value` にクラスパス (`/Script/VoxelRuntime.VoxelCollapseSubsystem`) または bare class name (`VoxelCollapseSubsystem`) を渡す → `FindClassByNameAnywhere` で解決、`K2Schema->TrySetDefaultObject` で設定、`ReconstructNode()` で下流ピン型を narrow。
- **Object/SoftObject/Interface ピン**: `pin_value` にアセットパス (`/Game/Textures/T_Grass.T_Grass`) を渡す → `LoadObject` で解決。
- **Class 型検証**: ピンが持つ `PinSubCategoryObject` (期待される base class) に対して解決されたクラスが `IsChildOf` かチェック。不一致時は `PropertyTypeMismatch` エラーを返す。

### 外部UPROPERTY Set/Get ノード 🆕 (v0.9.3)
`UPROPERTY(BlueprintReadWrite)` を **別クラス (Subsystemなど)** 経由で Set/Get するノードは、K2内部的には `UK2Node_CallFunction` ではなく `UK2Node_VariableSet/Get` の外部メンバ参照。`add_blueprint_function_node` では表現できない。

**新規コマンド** (明示的API):
- `add_external_property_set_node` — 外部クラスのUPROPERTYにSetノード追加 (BlueprintReadWrite必須)
- `add_external_property_get_node` — 外部クラスのUPROPERTYにGetノード追加 (BlueprintVisible必須)

**透過的 fallback**: `add_blueprint_function_node` に `function_name="SetMaxClusterForPhysics"` など `Set<Prop>` / `Get<Prop>` / `K2_Set<Prop>` / `K2_Get<Prop>` の関数名を渡した時、関数が見つからなければ自動的にプロパティSet/Getノードにフォールバック。既存呼び出しはそのまま動作。

```python
# Level BlueprintでSubsystemのUPROPERTYにSet
blueprint_node(command="add_external_property_set_node", params={
    "target_type": "level_blueprint",
    "target_class": "VoxelCollapseSubsystem",
    "property_name": "MaxClusterForPhysics",
    "node_position": [900, 0]
})
# → VariableSetノードが生成される。MaxClusterForPhysics入力ピンにset_node_pin_valueで値を設定可能。
```

### UMG Widget (32)
- **Core (3)**: create, viewport, anchor
- **Basic (5)**: text, image, progressbar, `add_border_to_widget` 🆕
- **Interactive (7)**: button, slider, checkbox, combobox, editabletext, spinbox, scrollbox
- **Layout (9)**: vertical/horizontal box, `add_widget_switcher_to_widget` 🆕, slot, reparent, remove, `get_widget_element_property`
- **Variable/Function (5)**: variable, array, function, event, binding
- **Animation (4)**: create, track, keyframe, list

**set_widget_slot_property 拡張 🆕 (v0.9.6)**: `anchor_min` / `anchor_max` (0-1 UV) と `offset_left` / `offset_top` / `offset_right` / `offset_bottom` (FMargin LTRB) を追加。`anchor` プリセット文字列は後方互換維持。
**create_umg_widget_blueprint.parent_class 汎用化 🆕 (v0.9.6)**: `/Script/Module.Class`、`/Game/Path.Asset_C`、bare name、全て受理。UUserWidget 継承チェック + 解決不能時は `ClassNotFound` (1211) / 型不一致は `InvalidParamValue` (1005) をハードエラー返却。

**get_widget_elements強化**: `include_properties`, `class_filter`, `property_filter`, `exclude_default_values` オプション追加 🆕
**set_widget_element_property強化**: ネストプロパティ対応（`Brush.TintColor` 形式） 🆕

### Enhanced Input (8)
`create_input_action`, `create_input_mapping_context`, `add_action_to_mapping_context`, `add_mapping_context_to_blueprint`, `set_default_mapping_context`, `get_input_mapping_context` 🆕, `get_input_action` 🆕, `remove_action_from_mapping_context` 🆕

**add_action_to_mapping_context強化**: Scalarモディファイア（オブジェクト形式）対応 🆕

### GAS (8)
`add_gameplay_tags`, `list_gameplay_tags`, `remove_gameplay_tag`, `list_gas_assets`, `create_gameplay_effect`, `create_gameplay_ability`, `create_gas_character`, `set_ability_system_defaults`

### AI - BehaviorTree/Blackboard (22)
- **Blackboard (4)**: `create_blackboard`, `add_blackboard_key`, `remove_blackboard_key`, `list_blackboard_keys`
- **BehaviorTree (3)**: `create_behavior_tree`, `set_behavior_tree_blackboard`, `get_behavior_tree_structure`
- **BTノード操作 (8)**: `add_bt_composite_node`, `add_bt_task_node`, `add_bt_decorator_node`, `add_bt_service_node`, `connect_bt_nodes`, `set_bt_node_property`, `delete_bt_node`, `list_bt_node_types`
- **BTノード位置 (2)**: `set_bt_node_position`, `auto_layout_bt`
- **BT健全性 (3)**: `detect_broken_bt_nodes` 🆕, `fix_broken_bt_nodes` 🆕, `repair_broken_bt_nodes` 🆕
- **BTデバッグ (1)**: `list_bt_nodes`
- **ユーティリティ (1)**: `list_ai_assets`

### AI Perception (6)
`add_ai_perception_component`, `configure_sight_sense`, `configure_hearing_sense`, `configure_damage_sense`, `set_perception_dominant_sense`, `add_perception_stimuli_source`

### EQS (5)
`create_eqs_query`, `add_eqs_generator`, `add_eqs_test`, `set_eqs_test_property`, `list_eqs_assets`

### Material (5)
`list_material_templates`, `get_material_template`, `save_material_template`, `delete_material_template`, `create_material_from_template`, `create_simple_material`

### Config (3)
`get_config_value`, `set_config_value`, `list_config_sections`

### Asset Utility (7) 🆕
`asset_exists`, `create_content_folder`, `list_assets_in_folder`, `import_texture`, `get_project_info`, `find_asset_references`, `delete_asset`

**import_texture特徴:**
- ファイルパスまたはBase64データからインポート
- 圧縮設定、sRGB、LODグループ指定可能
- 画像生成AI連携に対応

### RAG知識ベース (4)
`search_knowledge`, `add_knowledge`, `list_knowledge`, `delete_knowledge`

### AI Image Generation (3) 🆕
`get_ai_image_server_status`, `generate_image`, `generate_and_import_texture`

**機能:**
- Stable Diffusion Forge API連携
- Base64画像生成
- Unrealテクスチャ直接インポート
- プリセット: `game_icon`, `texture_tileable`, `concept_art`, `character_portrait`

**設定:**
- 環境変数: `AI_IMAGE_SERVER_URL` (デフォルト: `http://localhost:7860`)

**使用例:**
```python
# サーバー状態確認
get_ai_image_server_status()

# 画像生成のみ
generate_image(
    prompt="sword icon, fantasy RPG, golden handle",
    preset="game_icon"
)

# 生成 + Unrealインポート
generate_and_import_texture(
    prompt="health potion icon, red liquid, glass bottle",
    asset_name="T_Icon_HealthPotion",
    destination_path="/Game/UI/Icons",
    preset="game_icon",
    compression="UI"
)
```

---

## 最新の更新

### 2026-04-22: JSON Property Value — set_widget_element_property 型拡張 (v0.9.8) 🆕

**BUG-6 修正**: `set_widget_element_property.property_value` が string のみ受理だった制限を撤廃。

| 旧 (v0.9.7 まで) | 新 (v0.9.8) |
|---|---|
| `property_value: "1"` (string stringified) | `property_value: 1` (int 直接) |
| ColorAndOpacity が `"[1,0.5,0.2,1]"` string パース | `[1.0, 0.5, 0.2, 1.0]` 配列直接 |
| FMargin (Padding) は設定不可 | `[16, 8, 16, 8]` 配列直接 |
| FVector / FVector2D struct 不可 | `[x, y]` / `[x, y, z]` 配列 |
| 色・padding・スタイル系が MCP 経由で触れない | 触れる |

**実装**: `HandleSetWidgetElementProperty` に非 string 分岐を追加。既存の string-based fast path (Visibility / Text / Justification / Percent / ImportText fallback) は完全後方互換で保持。非 string 値は `FSpirrowBridgeCommonUtils::SetObjectProperty` にデリゲート (既存の FLinearColor / FMargin / FVector / FRotator / FColor / FTransform struct ハンドリングを再利用)。

**ColorAndOpacity 特例**: `UTextBlock::ColorAndOpacity` は `FSlateColor` (FLinearColor ラッパー)、`UImage::ColorAndOpacity` は `FLinearColor` 直接。`[r,g,b,a]` array が渡された場合は明示的な変換を経由して両方対応。

**nested path 制限**: `"Brush.TintColor"` のようなネストパスは現状 `ImportText_Direct` 経由のため string のみ。非 string を渡すと `InvalidParamType` エラー返却。

**コマンド数**: 変更なし (**158**)。既存コマンドの入力型拡張のみ。

---

### 2026-04-22: Reparent Safety — BUG fixes + 全 add_*_to_widget に parent_name (v0.9.7)

**WBP_MainMenu 実装検証で判明した致命的バグの修正 + 全 add_*_to_widget への parent_name 追加**:

| 変更 | 内容 |
|------|------|
| **BUG-1 修正 (reparent_widget_element)** 🔴 | 旧実装は `OldParent->RemoveChild` は呼ぶが `Modify()` / `MarkBlueprintAsStructurallyModified` 未呼出のため serialization で double-parent 状態が残っていた。v0.9.7 では UMG Designer の drag-drop reparent と同じ canonical pattern に揃える (`WidgetBP/WidgetTree/Element/OldParent/NewParent` 全てに `Modify()` → `RemoveChild` → `AddChild` → defensive post-check → `MarkBlueprintAsStructurallyModified` → `CompileBlueprint`)。旧親からの切断が確認できない場合は integrity check エラーを返す (無言成功を廃止) |
| **BUG-5 修正 (element_name ambiguity)** 🔴 | `UWidgetTree::FindWidget(FName)` は**最後に一致した widget**を返す (WidgetTree.cpp:32-44) 挙動が ue-investigator で確認された。BUG-1 で生じた同名 widget 重複時に意図しない widget が操作される問題に対応。`set_widget_element_property` / `set_widget_slot_property` / `remove_widget_element` / `reparent_widget_element` に optional `parent_name` を追加 → 検索範囲を指定パネル配下の sub-tree に絞れる (第一一致を返す決定論的な動作) |
| **BUG-2 廃止 (add_text_block_to_widget)** 🔴 | help は `widget_name` 必須と表示するが実装は `blueprint_name` を要求し `/Game/Widgets/` ハードコードパスしか受けない壊れた legacy コマンド。v0.9.7 で削除。`add_text_to_widget` への移行を推奨 (完全な互換な上位互換) |
| **FR-1 新機能 (全 add_*_to_widget に parent_name)** 🆕 | 9 個の leaf widget 追加コマンド (button / text / image / progressbar / slider / checkbox / combobox / editabletext / spinbox / scrollbox) が optional `parent_name` を受け取れるように。**reparent 依存を排除**し BUG-1 に遭遇する機会を根本的に減らす。共通ヘルパ `FSpirrowBridgeUMGWidgetCoreCommands::ResolveAddTarget` で実装を DRY 化 |
| **FR-4 (bind_widget_to_variable 削除)** | help list に載っていたが C++ 実装がない stale 参照を削除 |

**コマンド数**: 159 → **158** (-1: add_text_block_to_widget 廃止。bind_widget_to_variable は元々 non-functional だったので実質の機能減はなし)
**umg_widget コマンド数**: 19 → **18**

**UE 5.7 ソース解析**: `ue-investigator` で 5 件の深層調査を実施 (`UPanelWidget::RemoveChildAt` 完全性確認 / UMG Designer drag-drop canonical pattern / `UWidgetTree::FindWidget` 最後一致挙動 / `GetAllSourceWidgets` 配列保持 / `CompileBlueprint` が WidgetTree 整合性 rebuild しない確認)。BUG-1/BUG-5 の原因特定とほぼ全ての修正方針がこの解析から得られた。

---

### 2026-04-21: UMG Extensions — WidgetSwitcher / Border / 明示的 Anchors / parent_class 汎用化 (v0.9.6)

**WBP レイアウト表現力を拡張**:

| 変更 | 内容 |
|------|------|
| **add_widget_switcher_to_widget** 🆕 | `umg_layout` に UWidgetSwitcher コンテナ追加。`active_widget_index` でデザイン時のアクティブページ指定、`parent_name` で任意パネルへのネスト対応。ランタイム切替は `set_widget_element_property(property_name="ActiveWidgetIndex", ...)` (リフレクション fallback で既に動作) |
| **add_border_to_widget** 🆕 | `umg_widget` に UBorder 追加 (単一子コンテナ + 背景ブラシ)。`brush_color` / `content_color_and_opacity` / `padding` (FMargin LTRB) / `horizontal_alignment` / `vertical_alignment` を露出。`parent_name` で任意パネルへのネスト対応 |
| **set_widget_slot_property** 拡張 🆕 | CanvasPanelSlot に対して `anchor_min` / `anchor_max` (0-1 UV 空間で任意の FAnchors) と `offset_left/top/right/bottom` (FMargin LTRB 差分更新) を追加。既存の `anchor` プリセット文字列は優先されて後方互換。全画面ストレッチ `anchor_min=[0,0], anchor_max=[1,1]` が表現可能に |
| **create_umg_widget_blueprint.parent_class** 汎用化 🆕 | 従来は `"UserWidget"` ハードコード + 不安全な FindFirstObject + 無音 fallback だった。v0.9.6 では `/Script/Module.Class` (C++ 派生)、`/Game/Path.Asset_C` (Blueprint 派生)、bare name を全て受理。解決不能は `ClassNotFound` (1211)、UUserWidget 非継承は `InvalidParamValue` (1005) のハードエラー。成功時の `parent_class` レスポンスは `GetPathName()` で解決済みフルパスを返す |

**コマンド数**: 157 → **159** (+2)
**umg_widget コマンド数**: 18 → **19** / **umg_layout コマンド数**: 5 → **6**

---

### 2026-04-17: WorldSettings Configuration (v0.9.5)

**レベル毎の `AWorldSettings` を Unrealwise から読み書き可能に**:

| 変更 | 内容 |
|------|------|
| **get_world_settings** 🆕 | 現在のエディタワールドの AWorldSettings を取得。`properties` 省略時は curated preset (9個: DefaultGameMode / DefaultPhysicsVolumeClass / KillZ / KillZDamageType / WorldToMeters / GlobalGravityZ / TimeDilation / bEnableWorldBoundsChecks / bEnableWorldComposition)、指定時はそのフィールドだけ返す。存在しない名前は `unknown_properties` 配列に格納 (エラーにしない) |
| **set_world_properties** 🆕 | dict 形式で一括設定。既存の `FSpirrowBridgeCommonUtils::SetObjectProperty` をラップし class picker / primitive / struct を統一処理。**部分成功** (applied / failed 配列)、1つでも成功なら level を dirty マーク。全失敗時のみ `success=false` |
| **対象** | 現在開いているエディタワールドのみ。別レベルの WorldSettings を編集したい場合は `open_level` (v0.9.4) で先に切り替える |
| **永続化** | 設定後は自動保存しない。`save_current_level` を呼ぶまではメモリ上の変更のみ |

**コマンド数**: 155 → **157** (+2)
**editor コマンド数**: 15 → **17**

---

### 2026-04-17: Level Lifecycle (v0.9.4)

**`.umap` (Level) をUnrealwise経由で create / save / open 可能に**:

| 変更 | 内容 |
|------|------|
| **create_level** 🆕 | `editor(command="create_level", params={"name": ..., "path": ..., "template": ..., "overwrite": ...})` で新規 Level を作成。`UEditorLevelLibrary::NewLevel` / `NewLevelFromTemplate` をラップ。ディスクへの保存およびエディタの新レベル切り替えは UE 側が自動実行 |
| **save_current_level** 🆕 | `editor(command="save_current_level")` で現在開いているレベルを保存 (`UEditorLoadingAndSavingUtils::SaveCurrentLevel`) |
| **open_level** 🆕 | `editor(command="open_level", params={"level_path": "/Game/..."})` で既存 `.umap` をエディタで開く (`UEditorLevelLibrary::LoadLevel`)。dirty な状態で呼ぶと UE 側で save dialog が出るので、必要なら事前に `save_current_level` |
| **template 解決** | `"default"` (UE 5.7 既定 = WP有効) / `"empty"` (`/Engine/Maps/Templates/Empty` = 非WP) / `/Game/...` 明示パス (任意の既存レベルをテンプレートに) |
| **overwrite** | `false` (既定) では既存レベルがあれば fail、`true` で `UEditorAssetLibrary::DeleteAsset` 後に再作成 |
| **LSB 連携** | 作成直後に v0.9.3 の `target_type="level_blueprint"` + `level_path=作成したパス` ルートがそのまま使える。create → 即 LSB 編集がワンショットで完結 |

**新ファイル**: `SpirrowBridgeLevelCommands.h/.cpp` (2ファイル)
**コマンド数**: 152 → **155** (+3: `create_level`, `save_current_level`, `open_level`)
**editor コマンド数**: 12 → **15**

---

### 2026-04-12: Level Blueprint + External UPROPERTY + Typed Subsystem (v0.9.3)

**Blueprint / blueprint_node メタツールの3大強化**:

| 変更 | 内容 |
|------|------|
| **LSB target_type** | `blueprint` / `blueprint_node` の graph 編集系 23 コマンドが `target_type="level_blueprint"` + `level_path` でLevel Script Blueprintを直接編集可能に。`FSpirrowBridgeCommonUtils::ResolveTargetBlueprint` ヘルパーで統一解決 |
| **add_external_property_set_node** 🆕 | 外部クラスの UPROPERTY(BlueprintReadWrite) に対する `UK2Node_VariableSet` 生成。Subsystem フィールドなど典型ケースに対応 |
| **add_external_property_get_node** 🆕 | 同じく `UK2Node_VariableGet` 生成。BlueprintVisible が要件 |
| **add_blueprint_function_node fallback** | `function_name` が `Set<Prop>` / `Get<Prop>` / `K2_Set<Prop>` / `K2_Get<Prop>` で関数として見つからない場合、自動的に外部 UPROPERTY Set/Get ノードに fallback。既存呼び出しは壊れない |
| **add_get_subsystem_node** 🆕 | `UK2Node_GetSubsystem::Initialize(UClass*)` でクラスを焼き込んだ typed ノードを生成。`subsystem_kind` で GameInstance / World / Engine / LocalPlayer の4種類に対応。ReturnValue が strongly typed になるため Cast 不要 |
| **set_node_pin_value: Class/Object ピン対応** 🆕 | ピンカテゴリを判定して `Pin->DefaultValue` (プリミティブ) と `Pin->DefaultObject` (Class/SoftClass/Object/SoftObject/Interface) を自動選択。Class ピン設定時は `ReconstructNode()` で下流型を narrow。`PinSubCategoryObject` に対する型チェック付き |
| **CommonUtils ヘルパー追加** | `FindClassByNameAnywhere` (bare name / U/A プレフィックス自動ハンドル), `SpawnExternalPropertySetNode`, `SpawnExternalPropertyGetNode`, `ResolveTargetBlueprint` |
| **Unity build ambiguity fix** | `GenerateUniqueNodeName` が BTNodeOperations/BTNodeCreation 間で unity build 時に衝突していた問題を解決 (BTNodeCreation 側を `GenerateUniqueBTNodeNameForCreation` にリネーム) |

**コマンド数**: 149 → **152** (+3: `add_external_property_set_node`, `add_external_property_get_node`, `add_get_subsystem_node`)
**blueprint_node コマンド数**: 21 → **24**

---

### 2026-03-13: BT Robustness & Auto-Repair (v0.9.2)

**BTノード堅牢性の向上と自動修復機能**:

| 変更 | 内容 |
|------|------|
| `detect_broken_bt_nodes` | 壊れたBTノード（null NodeInstance）を検出 |
| `fix_broken_bt_nodes` | 壊れたBTノードを削除して修復 |
| `repair_broken_bt_nodes` | ClassDataからクラスを再解決しNodeInstanceを再生成 |
| Null Pointer Guards | 全BTノード作成時（Decorator/Service/Composite/Task）にnullチェック追加 |
| PostPlacedNewNode Guard | Decorator/ServiceのNodeInstance消失ガードを追加 |

**コマンド数**: 148 → **149** (+1: `repair_broken_bt_nodes`)

---

### 2026-03-07: Meta-Tool Architecture (v0.9.1)

**アーキテクチャ刷新**: 161個の個別MCP toolを14カテゴリのメタツールに統合。

| | Before | After |
|---|---|---|
| MCP ツール数 | 161 | **25** |
| 推定トークン消費 | ~170K | **~22K** |
| 会話に使えるコンテキスト | ~30K | **~178K** |
| C++ 変更 | - | **なし** |

**新規ファイル**:
- `Python/tools/command_schemas.py` - 全148コマンドのパラメータスキーマ
- `Python/tools/help_tool.py` - `help()` ツール
- `Python/tools/meta_utils.py` - メタツール共通ユーティリティ
- `Python/tools/*_meta.py` - 12個のメタツールファイル

**削除ファイル**: `editor_tools.py`, `blueprint_tools.py`, `node_tools.py`, `umg_tools.py`, `project_tools.py`, `ai_tools.py`, `perception_tools.py`, `eqs_tools.py`, `gas_tools.py`, `material_tools.py`, `config_tools.py`

**保持ファイル**: `rag_tools.py`, `knowledge_tools.py`, `image_gen_tools.py`, `error_codes.py`

---

### 2026-01-26: Blueprint Function Caller Search (v0.8.11)

**新規ツール追加 (1ツール)**:

| ツール | 機能 | 優先度 |
|--------|------|--------|
| `find_cpp_function_in_blueprints` | C++/Blueprint関数の呼び出し元を検索、依存関係可視化 | 高 |

**機能概要**:
- C++またはBlueprint関数がどのBlueprintから呼ばれているかを検索
- リファクタリング・関数削除前の影響範囲調査に活用
- 85個のBlueprintを約12msで高速検索
- 詳細情報: Blueprint名、グラフ名、ノード位置、所属クラス

**パラメータ**:
- `function_name` (必須): 検索対象の関数名
- `class_name` (オプション): クラス名でフィルタ
- `path_filter` (オプション): 検索対象パスで絞り込み
- `include_blueprint_functions` (オプション): Blueprint関数も含めるか

**使用例**:
```python
# 基本的な検索
find_cpp_function_in_blueprints(
    function_name="DealDamage"
)

# クラスとパスでフィルタ
find_cpp_function_in_blueprints(
    function_name="TakeDamage",
    class_name="ACharacter",
    path_filter="/Game/Characters/"
)
```

**パフォーマンス**:
- 検索速度: 11-14ms（85 Blueprints）
- メモリ効率: 低負荷（順次読み込み）
- 成功率: 100%

**実装詳細**: [Docs/FindFunctionCallers_Implementation.md](Docs/FindFunctionCallers_Implementation.md)

---

### 2026-01-12: AI Image Generation Integration (v0.8.10) 🆕

**新規ツール追加 (3ツール)**:

| ツール | 機能 | 優先度 |
|--------|------|--------|
| `get_ai_image_server_status` | AIサーバー疎通確認、モデル/サンプラー一覧取得 | 高 |
| `generate_image` | Stable Diffusion Forgeで画像生成（Base64返却） | 最優先 |
| `generate_and_import_texture` | 画像生成 + Unrealテクスチャインポート統合 | 最優先 |

**プリセット対応**:
- `game_icon`: 512x512, UIアイコン用
- `texture_tileable`: 1024x1024, シームレステクスチャ用
- `concept_art`: 768x512, コンセプトアート用
- `character_portrait`: 512x768, キャラクターポートレート用

**設定方法**:
- 環境変数 `AI_IMAGE_SERVER_URL` でエンドポイント変更可能
- `.env.example` に設定例追加

**バグ修正**:
- `import_texture`: TaskGraph RecursionGuard クラッシュ修正
  - 原因: `AsyncTask(GameThread)` 内で InterchangeEngine がTaskGraph操作を再帰実行
  - 解決: `FTSTicker` を使用してGameThread Tick外で実行（TaskGraphコンテキスト外）
  - 動作: テクスチャは自動保存され、Content Browserに即座に表示される

- `generate_and_import_texture`: ソケットバッファオーバーフロー修正
  - 原因: Base64データ（225KB+）を直接ソケット送信、UE側でハング
  - 解決: Python側で一時ファイル保存→ファイルパスのみ送信
  - 動作: 生成画像を一時ファイルに保存、インポート後に自動削除

### 2026-01-12: Asset Utility & Batch Operations (v0.8.9)

**新規ツール追加 (8ツール)**:

| ツール | 機能 | 優先度 |
|--------|------|--------|
| `import_texture` | テクスチャインポート（ファイル/Base64対応） | 最優先 |
| `asset_exists` | アセット存在確認 | 高 |
| `create_content_folder` | Content Browserフォルダ作成 | 高 |
| `list_assets_in_folder` | フォルダ内アセット一覧取得 | 高 |
| `get_project_info` | プロジェクト情報取得 | 中 |
| `find_asset_references` | アセット参照・依存関係検索 | 中 |
| `batch_set_properties` | 複数プロパティ一括設定 | 高 |

**バグ修正**:
- `import_texture`: TaskGraphクラッシュ修正（ImportAssetTasks → UTextureFactory直接インポート）
  - 原因: ImportAssetTasksが非GameThreadでアセット操作を実行
  - 解決: UTextureFactory::ImportObjectでGameThread上で同期実行

**ドキュメント改善**:
- `set_data_asset_property`: UTexture2D*, USoundBase*, UStaticMesh*などのオブジェクト参照設定例を追加

### 2026-01-11: DataAsset & プロパティ操作強化 (v0.8.8) 🆕

**新規ツール追加 (6ツール)**:

| ツール | 機能 | 優先度 |
|--------|------|--------|
| `create_data_asset` | UDataAsset派生クラスのインスタンスをContent Browserに作成 | 最優先 |
| `set_class_property` | TSubclassOf単体プロパティにクラス参照を設定 | 高 |
| `set_object_property` | UObject*/TObjectPtrプロパティにアセット参照を設定 | 高 |
| `get_blueprint_properties` | Blueprintの設定可能なプロパティと型を一覧取得 | 中 |
| `set_struct_property` | 構造体配列の個別要素を部分更新 | 中 |
| `set_data_asset_property` | DataAssetのプロパティを設定 | 高 |

**使用例**:
```python
# DataAsset作成
create_data_asset(
    name="DA_Pistol",
    parent_class="/Script/TrapxTrapCpp.UFirearmData",
    path="/Game/Data/Weapons"
)

# クラス参照設定
set_class_property(
    blueprint_name="BP_Spawner",
    property_name="EnemyClass",
    class_path="/Game/Blueprints/BP_Enemy.BP_Enemy_C"
)

# アセット参照設定
set_object_property(
    blueprint_name="BP_PlayerCharacter",
    property_name="DefaultWeaponData",
    asset_path="/Game/Data/DA_Pistol.DA_Pistol"
)

# プロパティ一覧取得
get_blueprint_properties(blueprint_name="BP_PlayerCharacter")

# 構造体配列の部分更新
set_struct_property(
    blueprint_name="BP_PlayerCharacter",
    property_name="InventorySlots",
    index=0,
    values={"MaxCount": 10, "WeaponData": "/Game/Data/DA_Pistol.DA_Pistol"}
)

# DataAssetのプロパティ設定
set_data_asset_property(
    asset_name="DA_Pistol",
    property_name="BaseDamage",
    property_value=50,
    path="/Game/Data/Weapons"
)
```

**解決する課題**:
- DataAssetのMCP経由作成が可能に
- DataAssetのプロパティ設定が可能に
- TSubclassOf単体プロパティ設定（配列版は既存）
- UObject*/TObjectPtr参照設定
- 構造体配列の部分更新（全体置換ではなく）

---

### 2026-01-10: MCP機能拡張 (v0.8.7)

**UMG Widget機能強化**:
- `get_widget_element_property`: Widget要素の任意プロパティ値を取得（ネストプロパティ対応）
- `get_widget_elements`: オプション追加
  - `include_properties`: プロパティ詳細を含める
  - `class_filter`: クラスでフィルタ (`["Button", "TextBlock"]`)
  - `property_filter`: 特定プロパティのみ取得
  - `exclude_default_values`: デフォルト値と同じプロパティを省略
- `set_widget_element_property`: ネストプロパティ対応 (`Brush.TintColor` 形式)

**Blueprintノード機能強化**:
- `disconnect_blueprint_nodes`: ノード間のピン接続を切断
  - 特定接続、特定ピン、ノード全体の3モード
- `add_blueprint_event_node`: BlueprintImplementableEvent オーバーライド対応
  - 親クラスのBlueprintImplementableEventを自動検出
  - 正しいEventReference設定でオーバーライドノード作成

**Enhanced Input機能強化**:
- `get_input_mapping_context`: IMC内容読み取り
- `get_input_action`: InputAction詳細取得
- `remove_action_from_mapping_context`: IMCからアクション削除
- `add_action_to_mapping_context`: Scalarモディファイアのオブジェクト形式対応

### 2026-01-09: BTノード自動位置計算 (v0.8.6)
- 自動レイアウト機能: `parent_node_id` 指定時に自動的に最適位置を計算
- 対象ツール: `add_bt_composite_node`, `add_bt_task_node`

### 2026-01-09: BT Bug Fixes & list_bt_nodes (v0.8.5)
- **新ツール追加**:
  - `list_bt_nodes`: BT内の全ノード一覧と階層構造を取得（デバッグ用）
- **バグ修正**:
  - `delete_bt_node`: ノード削除処理を強化（子接続解除、Decorator/Service同時削除、RuntimeNode GC対応）
  - `add_bt_decorator_node`: デバッグログ追加（Decorator重複付与調査用）
- **内部改善**:
  - `FinalizeAndSaveBTGraph`: UpdateAsset前後のデバッグログ追加（Verbose）

### 2026-01-09: BT Node Position & Auto Layout (v0.8.4)
- **新ツール追加**:
  - `set_bt_node_position`: 個別ノードの位置設定
  - `auto_layout_bt`: BT全体の自動レイアウト
- **既存ツール拡張**:
  - `add_bt_composite_node`: `node_position` パラメータ追加
  - `add_bt_task_node`: `node_position` パラメータ追加
- **自動レイアウト機能**: 階層構造に基づく再帰的位置計算、デコレータ/サービスの位置も考慮

### 2026-01-08: BehaviorTree Graph-Based Node Creation (v0.8.3)
- **Graph-Based実装に全面移行**: BTノード作成をUE Editor Graph APIベースに変更
  - `FGraphNodeCreator`パターンを使用してグラフノードを作成
  - ノードがエディタ上で正しく表示・保存されるように
  - `BTGraph->UpdateAsset()`でランタイムツリーを自動生成
- **ノード作成**: `add_bt_composite_node`, `add_bt_task_node` - ノード作成後に`connect_bt_nodes`で接続
- **接続API**: `connect_bt_nodes`でピンベースの接続（Root/Composite→Child）
- **Decorator/Service**: ターゲットノードの`Decorators`/`Services`配列に追加
- **Python API更新**: `parent_node_id`, `child_index`パラメータ追加

### 2026-01-07: Volume Actor対応 (v0.8.2)
- **spawn_actor Volume対応**: 8種類のVolumeアクターをspawn_actorで生成可能に
  - NavMeshBoundsVolume, TriggerVolume, BlockingVolume, KillZVolume
  - PhysicsVolume, PostProcessVolume, AudioVolume, LightmassImportanceVolume
- **brush_size パラメータ追加**: Volumeのサイズを明示的に指定可能 `[X, Y, Z]`
- UActorFactory::CreateBrushForVolumeActorを使用して正しいbrush geometryを生成

### 2026-01-07: Blackboard BaseClass Fix (v0.8.1)
- **Blackboard BaseClass修正**: `add_blackboard_key`の`base_class`パラメータが正しく動作するよう修正
  - `Actor`等の短い名前でクラス検索が可能に（複数の検索方法を試行）
  - MoveTo タスクで Object型キーが選択可能に
- **構造体プロパティ対応**: `SetObjectProperty`に`FBlackboardKeySelector`, `FVector`, `FAIDataProviderFloatValue`等の構造体対応追加
- BTノードのBlackboardKey設定が可能に
- EQS Testの`set_eqs_test_property`でStruct型対応

### Phase H (2026-01-06)
- **AI Perception (6ツール)**: AIPerceptionComponent、Sight/Hearing/Damage Sense設定、StimuliSource
- **EQS (5ツール)**: Environment Query System、Generator/Test操作
- NavigationSystem依存追加

### Phase G (2026-01-06)
- BTノード操作8ツール追加（Selector/Sequence/Task/Decorator/Service）
- UE 5.6+ API互換性対応
- Python `tools/` フラット構造に統一

### Phase F (2026-01-05)
- BehaviorTree/Blackboard操作8ツール追加
- AIモジュール統合

### Phase E (2026-01-03)
- エラーハンドリング統一（18 Commands）
- Blueprint/UMGコマンド分割リファクタリング

> 詳細な更新履歴: [Docs/CHANGELOG.md](Docs/CHANGELOG.md)

---

## 重要な注意事項

### BehaviorTree ノードの自動位置計算 🆕

**v0.8.6 以降**: BTノード作成時に `parent_node_id` を指定すると、**自動的に最適な位置が計算されます**。

```python
# 位置指定不要！自動的に配置される
add_bt_composite_node(
    behavior_tree_name="BT_Example",
    node_type="Selector",
    parent_node_id="Root"  # ← これだけでOK
)
```

**自動計算ルール**:
- Y位置: 親ノード + 150px
- X位置: 兄弟ノードの数 × 300px

**手動で位置指定する場合**:
```python
add_bt_composite_node(
    behavior_tree_name="BT_Example",
    node_type="Selector",
    parent_node_id="Root",
    node_position=[0, 200]  # 手動指定も可能
)
```

**後から再レイアウトする場合**:
```python
auto_layout_bt(
    behavior_tree_name="BT_Example",
    path="/Game/AI/BehaviorTrees"
)
```

---

## テスト環境

| 項目 | 値 |
|------|-----|
| Unreal Engine | 5.7 (5.6+ API互換) |
| Python | 3.11+ |
| テスト数 | 40+ (pytest) |

テスト実行:
```bash
cd Python && python tests/run_tests.py
```

---

## 凡例

| 記号 | 意味 |
|------|------|
| ✅ | 動作確認済み |
| 🔲 | 未確認/未実装 |
