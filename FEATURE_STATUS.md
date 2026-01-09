# spirrow-unrealwise 機能ステータス

> **バージョン**: Phase H (v0.8.3)
> **ステータス**: Beta
> **最終更新**: 2026-01-08

---

## 機能サマリー

| カテゴリ | ツール数 | 状態 |
|---------|---------|------|
| Actor操作 | 10 | ✅ |
| Blueprint操作 | 8 | ✅ |
| BPノードグラフ | 8 | ✅ |
| UMG Widget | 29 | ✅ |
| Enhanced Input | 5 | ✅ |
| GAS | 8 | ✅ |
| AI (BT/BB) | 17 | ✅ |
| AI Perception | 6 | ✅ |
| EQS | 5 | ✅ |
| Material | 5 | ✅ |
| Config | 3 | ✅ |
| RAG | 4 | ✅ |
| **合計** | **108** | |

---

## 詳細ステータス

### Actor操作 (10)
`get_actors_in_level`, `find_actors_by_name`, `spawn_actor`, `delete_actor`, `set_actor_transform`, `get_actor_properties`, `set_actor_property`, `set_actor_component_property`, `rename_actor`, `get_actor_components`

**spawn_actor 対応タイプ:**
- Basic: `StaticMeshActor`, `PointLight`, `SpotLight`, `DirectionalLight`, `CameraActor`
- Volumes: `NavMeshBoundsVolume`, `TriggerVolume`, `BlockingVolume`, `KillZVolume`, `PhysicsVolume`, `PostProcessVolume`, `AudioVolume`, `LightmassImportanceVolume`

### Blueprint操作 (8)
`create_blueprint`, `spawn_blueprint_actor`, `add_component_to_blueprint`, `set_static_mesh_properties`, `set_component_property`, `set_physics_properties`, `compile_blueprint`, `set_blueprint_property`

### BPノードグラフ (8)
`add_blueprint_event_node`, `add_blueprint_input_action_node`, `add_blueprint_function_node`, `connect_blueprint_nodes`, `add_blueprint_variable`, `add_blueprint_get_self_component_reference`, `add_blueprint_self_reference`, `find_blueprint_nodes`

### UMG Widget (29)
- **Core (3)**: create, viewport, anchor
- **Basic (4)**: text, image, progressbar
- **Interactive (7)**: button, slider, checkbox, combobox, editabletext, spinbox, scrollbox
- **Layout (7)**: vertical/horizontal box, slot, reparent, remove
- **Variable/Function (5)**: variable, array, function, event, binding
- **Animation (4)**: create, track, keyframe, list

### Enhanced Input (5)
`create_input_action`, `create_input_mapping_context`, `add_action_to_mapping_context`, `add_mapping_context_to_blueprint`, `set_default_mapping_context`

### GAS (8)
`add_gameplay_tags`, `list_gameplay_tags`, `remove_gameplay_tag`, `list_gas_assets`, `create_gameplay_effect`, `create_gameplay_ability`, `create_gas_character`, `set_ability_system_defaults`

### AI - BehaviorTree/Blackboard (17)
- **Blackboard (4)**: `create_blackboard`, `add_blackboard_key`, `remove_blackboard_key`, `list_blackboard_keys`
- **BehaviorTree (3)**: `create_behavior_tree`, `set_behavior_tree_blackboard`, `get_behavior_tree_structure`
- **BTノード操作 (8)**: `add_bt_composite_node`, `add_bt_task_node`, `add_bt_decorator_node`, `add_bt_service_node`, `connect_bt_nodes`, `set_bt_node_property`, `delete_bt_node`, `list_bt_node_types`
- **ユーティリティ (1)**: `list_ai_assets`

### AI Perception (6) 🆕
`add_ai_perception_component`, `configure_sight_sense`, `configure_hearing_sense`, `configure_damage_sense`, `set_perception_dominant_sense`, `add_perception_stimuli_source`

### EQS (5) 🆕
`create_eqs_query`, `add_eqs_generator`, `add_eqs_test`, `set_eqs_test_property`, `list_eqs_assets`

### Material (5)
`list_material_templates`, `get_material_template`, `save_material_template`, `delete_material_template`, `create_material_from_template`, `create_simple_material`

### Config (3)
`get_config_value`, `set_config_value`, `list_config_sections`

### RAG知識ベース (4)
`search_knowledge`, `add_knowledge`, `list_knowledge`, `delete_knowledge`

---

## 最新の更新

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
