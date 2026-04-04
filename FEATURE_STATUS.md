# spirrow-unrealwise 機能ステータス

> **バージョン**: v0.9.2 (BT Robustness & Auto-Repair)
> **ステータス**: Beta
> **最終更新**: 2026-04-04

---

## アーキテクチャ

**v0.9.1** でメタツール化を実施。161個の個別ツールを **25個** (14メタツール + 1ヘルプ + 10スタンドアロン) に統合。
コンテキスト消費量を ~170K → ~22K tokens に削減。
**v0.9.2** でBTノードの堅牢性向上・自動修復機能を追加。

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
| `editor` | Actor操作、トランスフォーム、プロパティ | 12 | ✅ |
| `blueprint` | BP作成、コンパイル、プロパティ、DataAsset | 21 | ✅ |
| `blueprint_node` | イベント、関数、変数、フロー制御、数学 | 21 | ✅ |
| `umg_widget` | テキスト、画像、ボタン、スライダー等 | 18 | ✅ |
| `umg_layout` | VBox/HBox、ScrollBox、リペアレント | 5 | ✅ |
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
| **内包コマンド合計** | **149** |

---

## 詳細ステータス

### Actor操作 (10)
`get_actors_in_level`, `find_actors_by_name`, `spawn_actor`, `delete_actor`, `set_actor_transform`, `get_actor_properties`, `set_actor_property`, `set_actor_component_property`, `rename_actor`, `get_actor_components`

**spawn_actor 対応タイプ:**
- Basic: `StaticMeshActor`, `PointLight`, `SpotLight`, `DirectionalLight`, `CameraActor`
- Volumes: `NavMeshBoundsVolume`, `TriggerVolume`, `BlockingVolume`, `KillZVolume`, `PhysicsVolume`, `PostProcessVolume`, `AudioVolume`, `LightmassImportanceVolume`

### Blueprint操作 (17)
`create_blueprint`, `spawn_blueprint_actor`, `add_component_to_blueprint`, `set_static_mesh_properties`, `set_component_property`, `set_physics_properties`, `compile_blueprint`, `set_blueprint_property`, `create_data_asset`, `set_class_property`, `set_object_property`, `get_blueprint_properties`, `set_struct_property`, `set_data_asset_property`, `get_data_asset_properties` 🆕, `batch_set_properties`, `find_cpp_function_in_blueprints`

### BPノードグラフ (9)
`add_blueprint_event_node`, `add_blueprint_input_action_node`, `add_blueprint_function_node`, `connect_blueprint_nodes`, `disconnect_blueprint_nodes` 🆕, `add_blueprint_variable`, `add_blueprint_get_self_component_reference`, `add_blueprint_self_reference`, `find_blueprint_nodes`

**add_blueprint_event_node**: BlueprintImplementableEvent オーバーライド対応 🆕

### UMG Widget (30)
- **Core (3)**: create, viewport, anchor
- **Basic (4)**: text, image, progressbar
- **Interactive (7)**: button, slider, checkbox, combobox, editabletext, spinbox, scrollbox
- **Layout (8)**: vertical/horizontal box, slot, reparent, remove, `get_widget_element_property` 🆕
- **Variable/Function (5)**: variable, array, function, event, binding
- **Animation (4)**: create, track, keyframe, list

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

### 2026-03-13: BT Robustness & Auto-Repair (v0.9.2) 🆕

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
