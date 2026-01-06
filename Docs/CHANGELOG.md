# 更新履歴 (Changelog)

このファイルはspirrow-unrealwiseの詳細な更新履歴をアーカイブしています。

---

## 2026-01-06: Phase H - AIPerception & EQS

**概要**: AI感知システムとEnvironment Query System操作11ツール追加

**AIPerception (6ツール)**:
- `add_ai_perception_component` - AIControllerにPerceptionComponent追加
- `configure_sight_sense` - 視覚設定（距離/角度/アフィリエーション）
- `configure_hearing_sense` - 聴覚設定
- `configure_damage_sense` - ダメージ感知設定
- `set_perception_dominant_sense` - 優先センス設定
- `add_perception_stimuli_source` - 被検知側コンポーネント追加

**EQS (5ツール)**:
- `create_eqs_query` - EQS Query Asset作成
- `add_eqs_generator` - Generator追加（SimpleGrid/Donut/OnCircle/ActorsOfClass等）
- `add_eqs_test` - Test追加（Distance/Trace/Dot等）+ scoring_factor対応
- `set_eqs_test_property` - Testプロパティ設定（基本型のみ）
- `list_eqs_assets` - EQSアセット一覧

**技術詳細**:
- C++: AIPerceptionCommands (18KB) + EQSCommands (16KB)
- Python: perception_tools.py + eqs_tools.py
- テスト: test_phase_h.py (13テスト)

**既知の制限**:
- `set_eqs_test_property`でStruct型（FAIDataProviderFloatValue）は未対応
- → `add_eqs_test`の`scoring_factor`パラメータで代替可能

---

## 2026-01-06: Phase G - BehaviorTree Node Operations

**概要**: BTノードグラフをプログラマティックに構築する8ツール追加

**新規ツール**:
- `add_bt_composite_node` - Selector/Sequence/SimpleParallel追加
- `add_bt_task_node` - MoveTo/Wait等9タスク + カスタムBP対応
- `add_bt_decorator_node` - Blackboard/Cooldown等9デコレータ
- `add_bt_service_node` - DefaultFocus/RunEQS等サービス
- `connect_bt_nodes` - 親子接続、Root設定
- `set_bt_node_property` - リフレクション経由プロパティ設定
- `delete_bt_node` - ノード削除
- `list_bt_node_types` - 利用可能ノードタイプ一覧

**技術詳細**:
- C++ AICommands 6ファイル分割構成（1,805行）
- UE 5.6+ API互換性対応（Decorator格納方式、TryGetField変更）
- Python `tools/` フラット構造に統一

---

## 2026-01-05: Phase F - AI (BehaviorTree / Blackboard)

**概要**: AI開発に必須のBehaviorTree/Blackboard操作8ツール

**新規ツール**:
- `create_blackboard` - Blackboard Data Asset作成
- `add_blackboard_key` - キー追加（10タイプ対応）
- `remove_blackboard_key` / `list_blackboard_keys`
- `create_behavior_tree` - BehaviorTree Asset作成
- `set_behavior_tree_blackboard` / `get_behavior_tree_structure`
- `list_ai_assets` - AI関連アセット一覧

**技術詳細**:
- C++: SpirrowBridgeAICommands (674行)
- Python: ai_tools.py (455行)
- テスト: test_ai_tools.py (16テスト)

---

## 2026-01-03: Phase E - エラーハンドリング統一

**概要**: 全18 CommandsファイルにESpirrowErrorCode使用を統一

**追加エラーコード (12個)**:
- General: UnknownCommand, InvalidParameter, OperationFailed, SystemError
- Blueprint: GraphNotFound, NodeNotFound, ClassNotFound, InvalidOperation
- Actor: ComponentCreationFailed
- Config: ConfigKeyNotFound, FileWriteFailed, FileReadFailed

---

## 2026-01-03: Phase 0.6.6 - UMGWidgetCommands分割

**概要**: SpirrowBridgeUMGWidgetCommands.cpp (64KB) を3ファイルに分割

**分割構成**:
- UMGWidgetCoreCommands.cpp (7KB) - 3関数
- UMGWidgetBasicCommands.cpp (17KB) - 4関数
- UMGWidgetInteractiveCommands.cpp (30KB) - 7関数

**削減効果**: 最大64KB → 30KB (53%削減)

---

## 2026-01-03: Phase 0.6.5 - BlueprintCommands分割

**概要**: Blueprint系2ファイル（計163KB）を6ファイルに分割

**Blueprint系分割**:
- BlueprintCoreCommands.cpp (23KB)
- BlueprintComponentCommands.cpp (26KB)
- BlueprintPropertyCommands.cpp (21KB)

**BlueprintNode系分割**:
- BlueprintNodeCoreCommands.cpp (24KB)
- BlueprintNodeVariableCommands.cpp (14KB)
- BlueprintNodeControlFlowCommands.cpp (21KB)

**削減効果**: 最大95KB → 26KB (73%削減)

---

## それ以前の履歴

### Phase 0.6.0 - GAS対応
- GameplayTags / GameplayEffect / GameplayAbility ツール

### Phase 0.5.0 - UMG Widget
- Widget Blueprint操作（29ツール）
- アニメーション、変数・関数バインディング

### Phase 0.4.0 - Enhanced Input
- Input Action / Mapping Context
- Config操作ツール

### Phase 0.3.0 - Blueprint Node
- ノードグラフ操作
- RAG知識ベース統合
