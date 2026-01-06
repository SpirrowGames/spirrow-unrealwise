# spirrow-unrealwise 機能ステータス

## 概要

このドキュメントは、MCPツールの動作確認状況と今後追加予定の機能をまとめたものです。

> **バージョン**: Phase G (BehaviorTree Node Operations完了)  
> **ステータス**: Beta  
> **最終更新**: 2026-01-06

---

## 確認済み機能

### Actor操作

| ツール | 状態 | 備考 |
|--------|------|------|
| `get_actors_in_level` | ✅ 動作OK | レベル内の全アクター取得 |
| `find_actors_by_name` | ✅ 動作OK | パターンマッチ検索 |
| `spawn_actor` | ✅ 動作OK | アクター作成のみ、メッシュ設定は別途必要 |
| `delete_actor` | ✅ 動作OK | |
| `set_actor_transform` | ✅ 動作OK | location/rotation/scale対応 |
| `get_actor_properties` | ✅ 動作OK | |
| `set_actor_property` | ✅ 動作OK | アクター自体のプロパティを設定。rationale対応 |
| `set_actor_component_property` | ✅ 動作OK | アクターのコンポーネントのプロパティを設定。rationale対応 |
| `rename_actor` | ✅ 動作OK | アクター名変更（ActorLabel/Name両対応） |
| `get_actor_components` | ✅ 動作OK | アクターのコンポーネント一覧取得 |

### Blueprint操作

| ツール | 状態 | 備考 |
|--------|------|------|
| `create_blueprint` | ✅ 動作OK | pathパラメータ対応（デフォルト: /Game/Blueprints） |
| `spawn_blueprint_actor` | ✅ 動作OK | pathパラメータ対応（デフォルト: /Game/Blueprints） |
| `add_component_to_blueprint` | ✅ 動作OK | pathパラメータ対応（デフォルト: /Game/Blueprints） |
| `set_static_mesh_properties` | ✅ 動作OK | Engine標準メッシュで確認。pathパラメータ対応 |
| `set_component_property` | ✅ 動作OK | pathパラメータ対応 |
| `set_physics_properties` | ✅ 動作OK | pathパラメータ対応 |
| `compile_blueprint` | ✅ 動作OK | pathパラメータ対応（デフォルト: /Game/Blueprints） |
| `set_blueprint_property` | ✅ 動作OK | pathパラメータ対応 |

### BPノードグラフ操作

| ツール | 状態 | 備考 |
|--------|------|------|
| `add_blueprint_event_node` | ✅ 動作OK | ReceiveBeginPlay確認。pathパラメータ対応 |
| `add_blueprint_input_action_node` | ✅ 動作OK | pathパラメータ対応 |
| `add_blueprint_function_node` | ✅ 動作OK | target指定が重要（self, KismetSystemLibrary等）。pathパラメータ対応 |
| `connect_blueprint_nodes` | ✅ 動作OK | ピン名: then → execute。pathパラメータ対応 |
| `add_blueprint_variable` | ✅ 動作OK | pathパラメータ対応 |
| `add_blueprint_get_self_component_reference` | ✅ 動作OK | pathパラメータ対応 |
| `add_blueprint_self_reference` | ✅ 動作OK | pathパラメータ対応 |
| `find_blueprint_nodes` | ✅ 動作OK | pathパラメータ対応 |

### UMG Widget操作

#### Phase 1-4: 全29ツール実装完了 ✅

| カテゴリ | ツール数 | 状態 |
|---------|---------|------|
| Core | 3 | ✅ 動作OK |
| Basic | 4 | ✅ 動作OK |
| Interactive | 7 | ✅ 動作OK |
| Layout | 7 | ✅ 実装完了 |
| Variable/Function | 5 | ✅ 実装完了 |
| Animation | 4 | ✅ 実装完了 |

### RAG連携

| ツール | 状態 | 備考 |
|--------|------|------|
| `search_knowledge` | ✅ 動作OK | RAGサーバー連携、意味検索対応 |
| `add_knowledge` | ✅ 動作OK | ナレッジ追加、カテゴリ・タグ対応 |
| `list_knowledge` | ✅ 動作OK | 登録済みナレッジ一覧取得 |
| `delete_knowledge` | ✅ 動作OK | ID指定でナレッジ削除 |

### AI操作 (BehaviorTree / Blackboard) 🆕

#### Blackboard

| ツール | 状態 | 備考 |
|--------|------|------|
| `create_blackboard` | ✅ 実装完了 | Blackboard Data Asset作成。テスト完備 |
| `add_blackboard_key` | ✅ 実装完了 | キー追加（Bool/Int/Float/String/Name/Vector/Rotator/Object/Class/Enum対応）。テスト完備 |
| `remove_blackboard_key` | ✅ 実装完了 | キー削除。テスト完備 |
| `list_blackboard_keys` | ✅ 実装完了 | キー一覧取得。テスト完備 |

#### BehaviorTree

| ツール | 状態 | 備考 |
|--------|------|------|
| `create_behavior_tree` | ✅ 実装完了 | BehaviorTree Asset作成、Blackboard連携可能。テスト完備 |
| `set_behavior_tree_blackboard` | ✅ 実装完了 | BTにBlackboard設定。テスト完備 |
| `get_behavior_tree_structure` | ✅ 実装完了 | BT構造情報取得。テスト完備 |

#### BehaviorTree Node Operations (Phase G) 🆕

| ツール | 状態 | 備考 |
|--------|------|------|
| `add_bt_composite_node` | ✅ 実装完了 | Selector/Sequence/SimpleParallel追加 |
| `add_bt_task_node` | ✅ 実装完了 | MoveTo/Wait等9タスク + カスタムBP対応 |
| `add_bt_decorator_node` | ✅ 実装完了 | Blackboard/Cooldown等9デコレータ + カスタムBP対応 |
| `add_bt_service_node` | ✅ 実装完了 | DefaultFocus/RunEQS等3サービス + カスタムBP対応 |
| `connect_bt_nodes` | ✅ 実装完了 | 親子接続、Root設定、挿入位置指定可能 |
| `set_bt_node_property` | ✅ 実装完了 | リフレクション経由でノードプロパティ設定 |
| `delete_bt_node` | ✅ 実装完了 | ノード削除（再帰的に全参照から削除） |
| `list_bt_node_types` | ✅ 実装完了 | 利用可能なノードタイプ一覧取得、カテゴリ指定可能 |

#### ユーティリティ

| ツール | 状態 | 備考 |
|--------|------|------|
| `list_ai_assets` | ✅ 実装完了 | AI関連アセット（BT/BB）一覧取得、パスフィルタ対応。テスト完備 |

---

## 最新の更新履歴

### 2026-01-06: Phase G - BehaviorTree Node Operations 実装完了 + UE 5.6+ API対応 ✅ 🆕

**実装内容**:
- **8つの新MCPツール追加**: BehaviorTreeノードグラフをプログラマティックに構築
- **C++リファクタリング**: AICommandsを6ファイルに分割（保守性向上）
- **C++実装総量**: 1,805行（6ファイル分割構成）
- **Python実装**: ai_tools.py拡張（455行 → 1,015行）
- **UE 5.6+ API互換性対応**: Decorator格納方式変更、TryGetField API変更に対応 🆕

**Phase G新規ツール (8個)**:
| カテゴリ | ツール | 説明 |
|---------|--------|------|
| Node Creation | `add_bt_composite_node` | Selector/Sequence/SimpleParallel追加 |
| Node Creation | `add_bt_task_node` | MoveTo/Wait等9タスク + カスタムBP対応 |
| Node Creation | `add_bt_decorator_node` | Blackboard/Cooldown等9デコレータ + カスタムBP対応 |
| Node Creation | `add_bt_service_node` | DefaultFocus/RunEQS等3サービス + カスタムBP対応 |
| Node Operation | `connect_bt_nodes` | 親子接続、Root設定、挿入位置指定 |
| Node Operation | `set_bt_node_property` | リフレクション経由プロパティ設定 |
| Node Operation | `delete_bt_node` | ノード削除（再帰的に全参照削除） |
| Utility | `list_bt_node_types` | 利用可能なノードタイプ一覧 |

**対応BTノードタイプ**:
- **Composite (3種)**: Selector, Sequence, SimpleParallel
- **Task (9種)**: MoveTo, MoveDirectlyToward, Wait, WaitBlackboardTime, PlaySound, PlayAnimation, RotateToFaceBBEntry, RunBehavior, RunBehaviorDynamic
- **Decorator (9種)**: Blackboard, CompareBBEntries, Cooldown, DoesPathExist, ForceSuccess, IsAtLocation, Loop, TagCooldown, TimeLimit
- **Service (3種)**: DefaultFocus, RunEQS, BlackboardBase
- **カスタムBlueprint対応**: `/Game/AI/{Tasks|Decorators|Services}/`から自動検索

**C++リファクタリング (6ファイル構成)**:
```
MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/
├── SpirrowBridgeAICommands.cpp (155行) - ルーター
├── SpirrowBridgeAICommands_Blackboard.cpp (340行) - Phase F: Blackboard
├── SpirrowBridgeAICommands_BehaviorTree.cpp (260行) - Phase F: BehaviorTree
├── SpirrowBridgeAICommands_BTNodeHelpers.cpp (240行) - Phase G: ヘルパー関数
├── SpirrowBridgeAICommands_BTNodeCreation.cpp (360行) - Phase G: Add handlers
└── SpirrowBridgeAICommands_BTNodeOperations.cpp (450行) - Phase G: Op handlers
```

**統合作業**:
- SpirrowBridge.cpp: Phase G 8コマンドのルーティング追加
- ai_tools.py: 8ツール実装（詳細なdocstring、使用例付き）
- 全ファイル構文チェック完了

**技術的特徴**:
- **ノードID**: `UObject::GetName()`使用（例: "BTComposite_Selector_0"）
- **接続方式**: `FBTCompositeChild`構造体（EdGraphピンではなくデータ構造）
- **再帰検索**: Lambda関数によるBTツリー階層探索
- **カスタムBP対応**: 規約ベースパス検索（`/Game/AI/Tasks/MyTask.MyTask`）
- **プロパティ設定**: `FSpirrowBridgeCommonUtils::SetObjectProperty()`でリフレクション

**Phase F+G統合 (全17ツール)**:
- Phase F: Blackboard/BehaviorTree作成（8ツール）
- Phase G: BT Node Operations（8ツール）
- Utility: AI Asset管理（1ツール）

**UE 5.6+ API互換性対応** 🆕:
- **問題**: UE 5.5向けプロンプトで実装したが、両プロジェクトはUE 5.7使用
- **原因1**: Decoratorの格納方法が変更（`Node->Decorators` → `Child.Decorators`）
- **原因2**: `FJsonObject::TryGetField`のシグネチャ変更
- **修正内容**:
  - `BTNodeHelpers.cpp`: Decorator検索を`Child.Decorators`経由に変更
  - `BTNodeCreation.cpp`: Decorator追加を再帰的に`FBTCompositeChild`検索するロジックに書き直し
  - `BTNodeOperations.cpp`: Decorator削除を`Child.Decorators`から削除、`TryGetField`を新API使用
  - 両プロジェクト（spirrow-unrealwise、TrapxTrapCpp）に適用済み
- **技術詳細**:
  - **旧API** (UE 5.5): `CompositeNode->Decorators.Add(Decorator)`
  - **新API** (UE 5.6+): `Parent->Children[i].Decorators.Add(Decorator)`
  - **TryGetField旧**: `Params->TryGetField(TEXT("key"), OutPtr)`
  - **TryGetField新**: `TSharedPtr<FJsonValue> Value = Params->TryGetField(TEXT("key"))`

**Python側整理** (追加作業 2026-01-06):
- `spirrow_unrealwise/` パッケージ構造を削除、フラットな `tools/` 構造に統一
- `tools/ai_tools.py` をPhase G版に更新（`from unreal_mcp_server import` 使用）
- MCPサーバー再起動後、全Phase Gツール動作確認完了

**ビルド状況**: ✅ UE 5.6/5.7互換、構文チェック完了、Unreal Editor起動時に自動ビルド

---

### 2026-01-05: Phase F - AI (BehaviorTree / Blackboard) ツール実装・テスト完了 ✅

**実装内容**:
- **8つの新MCPツール追加**: AI開発に必須のBehaviorTree/Blackboard操作
- **C++実装**: SpirrowBridgeAICommands (674行)
- **Python実装**: ai_tools.py (455行、正しいインポートパス使用)
- **テスト実装**: test_ai_tools.py (16テスト)
- **Build.cs更新**: AIModule依存追加

**新規ツール**:
| カテゴリ | ツール | 説明 |
|---------|--------|------|
| Blackboard | `create_blackboard` | Blackboard Data Asset作成 |
| Blackboard | `add_blackboard_key` | キー追加（10タイプ対応） |
| Blackboard | `remove_blackboard_key` | キー削除 |
| Blackboard | `list_blackboard_keys` | キー一覧取得 |
| BehaviorTree | `create_behavior_tree` | BehaviorTree Asset作成 |
| BehaviorTree | `set_behavior_tree_blackboard` | BTにBlackboard紐付け |
| BehaviorTree | `get_behavior_tree_structure` | BT構造情報取得 |
| Utility | `list_ai_assets` | AI関連アセット一覧 |

**対応Blackboardキータイプ (10種)**:
- プリミティブ: Bool, Int, Float, String, Name
- 数学: Vector, Rotator
- 参照: Object, Class, Enum

**新規ファイル**:
```
MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/
├── Public/Commands/SpirrowBridgeAICommands.h (95行)
└── Private/Commands/SpirrowBridgeAICommands.cpp (674行)

Python/
├── tools/ai_tools.py (455行)
└── tests/test_ai_tools.py (16テスト)
```

**統合作業**:
- SpirrowBridge.h/.cpp: AICommands統合
- SpirrowBridge.Build.cs: AIModule依存追加
- unreal_mcp_server.py: `from tools.ai_tools import register_ai_tools`
- ValidateRequiredString使用箇所を正しいパターンに修正（11箇所）

**テスト実装 (test_ai_tools.py)**:
| テストクラス | テスト数 | 内容 |
|-------------|---------|------|
| `TestBlackboard` | 8 | Blackboard作成、キー追加（Bool/Int/Float/Vector/Object）、キー削除、一覧取得 |
| `TestBehaviorTree` | 4 | BehaviorTree作成、Blackboard連携、Blackboard設定、構造取得 |
| `TestAIUtility` | 3 | AIアセット一覧（全て/Blackboardのみ/BehaviorTreeのみ） |
| `TestAIIntegration` | 1 | 完全なAIシステム作成（Blackboard+BehaviorTree統合） |

**テスト実行方法**:
```bash
cd Python/tests
python run_tests.py -m ai      # AIテストのみ
python run_tests.py -m ai -v   # 詳細出力
```

**ビルド状況**: ✅ コンパイル成功（3.68秒）

---

### 2026-01-03: Phase E - 全Commandsエラーハンドリング統一 🆕

**完了内容**:
- 全18 CommandsファイルにESpirrowErrorCode使用を統一
- SpirrowBridgeCommonUtils.hに不足エラーコード12個追加

**追加エラーコード**:
| カテゴリ | コード | 説明 |
|---------|------|------|
| General (1000-1099) | `UnknownCommand` | 不明なコマンド |
| General | `InvalidParameter` | 無効なパラメータ |
| General | `OperationFailed` | 操作失敗 |
| General | `SystemError` | システムエラー |
| Blueprint (1200-1299) | `GraphNotFound` | グラフが見つからない |
| Blueprint | `NodeNotFound` | ノードが見つからない |
| Blueprint | `ClassNotFound` | クラスが見つからない |
| Blueprint | `InvalidOperation` | 無効な操作 |
| Actor (1400-1499) | `ComponentCreationFailed` | コンポーネント作成失敗 |
| Config (1600-1699) | `ConfigKeyNotFound` | 設定キーが見つからない |
| Config | `FileWriteFailed` | ファイル書き込み失敗 |
| Config | `FileReadFailed` | ファイル読み取り失敗 |

**対象ファイル (18ファイル)**:
- ActorCommands, BlueprintCommands, BlueprintNodeCommands
- BlueprintNodeFlowCommands, BlueprintNodeMathCommands, BlueprintNodeVariableCommands
- InputCommands, GASCommands, GASTagCommands, ConfigCommands
- UMGWidgetBasicCommands, UMGWidgetCoreCommands, UMGWidgetInteractiveCommands
- UMGLayoutCommands, UMGAnimationCommands, UMGVariableCommands
- MaterialCommands, ProjectCommands

**テスト結果**:
- TrapxTrapCppビルド成功 ✅
- MCPツールテスト全11項目パス ✅

---

### 2026-01-03: UMGWidgetCommands 分割リファクタリング完了 (Phase 0.6.6)

**完了内容**:
- `SpirrowBridgeUMGWidgetCommands.cpp` (64 KB) を3ファイルに分割
- ルーターパターン採用: UMGWidgetCommandsが3つのハンドラへ委譲

**新ファイル構成 - UMGWidget系**:
| ファイル | サイズ | 担当 |
|----------|--------|------|
| `SpirrowBridgeUMGWidgetCoreCommands.cpp` | 7 KB | CreateUMGWidgetBlueprint, AddWidgetToViewport, ParseAnchorPreset（3関数） |
| `SpirrowBridgeUMGWidgetBasicCommands.cpp` | 17 KB | AddTextToWidget, AddTextBlockToWidget, AddImageToWidget, AddProgressBarToWidget（4関数） |
| `SpirrowBridgeUMGWidgetInteractiveCommands.cpp` | 30 KB | AddButtonToWidget, AddSliderToWidget, AddCheckBoxToWidget, AddComboBoxToWidget, AddEditableTextToWidget, AddSpinBoxToWidget, AddScrollBoxToWidget（7関数） |
| `SpirrowBridgeUMGWidgetCommands.cpp` | 1.5 KB | ルーター |

**Python側修正**:
- `umg_tools.py`: `add_button_to_widget_v2` → `add_button_to_widget` コマンド名修正

**テスト結果**:
- 全11コマンド動作確認完了 ✅

**削減効果**:
- 最大ファイルサイズ: 64KB → 30KB (53%削減)
- Phase 0.6累計: Blueprint系6ファイル + UMG系3ファイル分割完了
- 全体最大ファイルサイズ: 166KB → 30KB (82%削減)

---

### 2026-01-03: BlueprintCommands 分割リファクタリング完了 (Phase 0.6.5)

**完了内容**:
- `SpirrowBridgeBlueprintCommands.cpp` (95 KB) を3ファイルに分割
- `SpirrowBridgeBlueprintNodeCommands.cpp` (68 KB) を3ファイルに分割
- オプションB採用: 各ルーターファイルから分割クラスへ委譲

**新ファイル構成 - Blueprint系**:
| ファイル | サイズ | 担当 |
|----------|--------|------|
| `SpirrowBridgeBlueprintCoreCommands.cpp` | 23 KB | 作成/コンパイル/スポーン/複製/グラフ取得（6関数） |
| `SpirrowBridgeBlueprintComponentCommands.cpp` | 26 KB | コンポーネント追加/プロパティ設定/物理（5関数） |
| `SpirrowBridgeBlueprintPropertyCommands.cpp` | 21 KB | クラススキャン/配列プロパティ（3関数） |
| `SpirrowBridgeBlueprintCommands.cpp` | 1.7 KB | ルーター |

**新ファイル構成 - BlueprintNode系**:
| ファイル | サイズ | 担当 |
|----------|--------|------|
| `SpirrowBridgeBlueprintNodeCoreCommands.cpp` | 24 KB | 接続/検索/イベント/関数呼び出し（7関数） |
| `SpirrowBridgeBlueprintNodeVariableCommands.cpp` | 14 KB | 変数/Get/Set/Self参照/InputAction（6関数） |
| `SpirrowBridgeBlueprintNodeControlFlowCommands.cpp` | 21 KB | Branch/Sequence/Delay/Loop/Math/Print（8関数） |
| `SpirrowBridgeBlueprintNodeCommands.cpp` | 1.7 KB | ルーター |

**削減効果**:
- 最大ファイルサイズ: 95KB → 26KB (73%削減)
- 合計6ファイル追加、既存2ファイルはルーターに変換

---

## テスト環境

- **Unreal Engine**: 5.7 (5.6+ API互換対応済み)
- **プロジェクト**: TrapxTrapCpp, MCPGameProject (spirrow-unrealwise)
- **RAGサーバー**: AIサーバー :8100
- **最終確認日**: 2026-01-06

---

## 凡例

| 記号 | 意味 |
|------|------|
| ✅ | 動作確認済み |
| 🔲 | 未確認 |
| 🆕 | 新規追加 |

---

## Phase C: テスト自動化・エラーハンドリング強化 (進行中) 🆕

### Part 1: テストフレームワーク作成 ✅

**新規作成ファイル (Python/tests/)**:
| ファイル | 説明 |
|----------|------|
| `test_framework.py` | MCPクライアント & TestSuite基盤 |
| `conftest.py` | pytest fixtures |
| `test_umg_widgets.py` | UMG Widgetテスト (13テスト) |
| `test_blueprints.py` | Blueprintテスト (11テスト) |
| `test_ai_tools.py` | AI (BehaviorTree/Blackboard) テスト (16テスト) 🆕 |
| `run_tests.py` | CLIテストランナー |
| `smoke_test.py` | スタンドアロン スモークテスト |
| `README.md` | テストドキュメント |

**テスト実行方法**:
```bash
cd Python
pip install -e ".[test]"
python tests/smoke_test.py  # クイックテスト
python tests/run_tests.py   # 全テスト
python tests/run_tests.py -m umg  # UMGのみ
```

### Part 2: エラーハンドリング強化 ✅

**C++側 (SpirrowBridgeCommonUtils)**:

新規追加エラーコード体系:
```cpp
namespace ESpirrowErrorCode {
    // General (1000-1099): InvalidParams, MissingRequiredParam, etc.
    // Asset (1100-1199): AssetNotFound, AssetLoadFailed, etc.
    // Blueprint (1200-1299): BlueprintNotFound, NodeCreationFailed, etc.
    // Widget (1300-1399): WidgetNotFound, WidgetElementNotFound, etc.
    // Actor (1400-1499): ActorNotFound, ComponentNotFound, etc.
    // GAS (1500-1599): GameplayTagInvalid, etc.
}
```

新規バリデーション関数:
- `ValidateRequiredString()` - 必須文字列パラメータ検証
- `ValidateRequiredNumber()` - 必須数値パラメータ検証
- `ValidateRequiredBool()` - 必須ブール値パラメータ検証
- `GetOptionalString/Number/Bool()` - オプショナルパラメータ取得
- `ValidateBlueprint()` - Blueprint存在確認
- `ValidateWidgetBlueprint()` - Widget Blueprint存在確認
- `IsValidAssetPath()` - アセットパス形式検証
- `GetLinearColorFromJson()` - RGBA色値取得
- `LogCommandError/Warning/Info()` - ロギングユーティリティ

エラーレスポンス形式:
```json
{
    "success": false,
    "error_code": 1200,
    "error": "Blueprint not found: BP_Test at /Game/Test",
    "details": {
        "blueprint_name": "BP_Test",
        "path": "/Game/Test",
        "full_path": "/Game/Test/BP_Test.BP_Test"
    }
}
```

**Python側 (tools/error_codes.py)**:
- `ErrorCode` enum - C++と同期したエラーコード
- `SpirrowError` dataclass - 構造化エラー
- `parse_error_response()` - レスポンスからエラー解析
- `get_friendly_message()` - ユーザーフレンドリーメッセージ
