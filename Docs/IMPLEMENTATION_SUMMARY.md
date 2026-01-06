# SpirrowBridge 実装サマリ

C++ 実装の全体像。新しいセッション開始時の参照用。

> **最終更新**: 2026-01-06 | **バージョン**: Phase H

---

## ファイル構成 (29ファイル)

### Commands ディレクトリ

#### Blueprint 系 (3分割 + ルーター)

| ファイル | サイズ | 担当 |
|----------|--------|------|
| `BlueprintCoreCommands` | 23 KB | 作成/コンパイル/スポーン/複製/グラフ |
| `BlueprintComponentCommands` | 26 KB | コンポーネント/プロパティ/物理 |
| `BlueprintPropertyCommands` | 21 KB | クラススキャン/配列プロパティ |
| `BlueprintCommands` | 1.7 KB | ルーター |

#### BlueprintNode 系 (3分割 + ルーター)

| ファイル | サイズ | 担当 |
|----------|--------|------|
| `BlueprintNodeCoreCommands` | 24 KB | 接続/検索/イベント/関数 |
| `BlueprintNodeVariableCommands` | 14 KB | 変数/Get/Set/Self参照 |
| `BlueprintNodeControlFlowCommands` | 21 KB | Branch/Sequence/Loop/Math |
| `BlueprintNodeCommands` | 1.7 KB | ルーター |

#### UMG Widget 系 (3分割 + ルーター + 3独立)

| ファイル | サイズ | 担当 |
|----------|--------|------|
| `UMGWidgetCoreCommands` | 7 KB | Widget作成/Viewport/Anchor |
| `UMGWidgetBasicCommands` | 17 KB | Text/Image/ProgressBar |
| `UMGWidgetInteractiveCommands` | 30 KB | Button/Slider/CheckBox等 |
| `UMGWidgetCommands` | 1.5 KB | ルーター |
| `UMGVariableCommands` | 40 KB | 変数/バインディング |
| `UMGLayoutCommands` | 32 KB | レイアウト |
| `UMGAnimationCommands` | 23 KB | アニメーション |

#### AI 系 (5分割 + ルーター)

| ファイル | サイズ | 担当 |
|----------|--------|------|
| `AICommands` | 1.5 KB | ルーター |
| `AICommands_Blackboard` | 11 KB | Blackboard操作 |
| `AICommands_BehaviorTree` | 8.5 KB | BehaviorTree操作 |
| `AICommands_BTNodeHelpers` | 8 KB | BTノードヘルパー |
| `AICommands_BTNodeCreation` | 12 KB | BTノード追加 |
| `AICommands_BTNodeOperations` | 15 KB | BTノード操作 |

#### AI Perception & EQS 系 (Phase H)

| ファイル | サイズ | 担当 |
|----------|--------|------|
| `AIPerceptionCommands` | 18 KB | AIPerceptionComponent/Sense設定 |
| `EQSCommands` | 16 KB | EQS Query/Generator/Test |

#### その他

| ファイル | サイズ | 担当 |
|----------|--------|------|
| `GASCommands` | 55 KB | Gameplay Ability System |
| `CommonUtils` | 35 KB | 共通ユーティリティ |
| `EditorCommands` | 29 KB | アクター・エディタ |
| `ProjectCommands` | 25 KB | プロジェクト・入力 |
| `MaterialCommands` | 8 KB | マテリアル |
| `ConfigCommands` | 8 KB | Config (INI) |

---

## コマンドルーティング

### SpirrowBridge.cpp (メインルーター)

```
コマンド種別 → 担当ハンドラ
─────────────────────────────
create_blueprint, compile_* ... → BlueprintCommands
add_blueprint_event_node ...    → BlueprintNodeCommands
create_umg_widget_blueprint ... → UMGWidgetCommands
add_vertical_box_to_widget ...  → UMGLayoutCommands
create_widget_animation ...     → UMGAnimationCommands
add_widget_variable ...         → UMGVariableCommands
get_actors_in_level ...         → EditorCommands
create_input_action ...         → ProjectCommands
get_config_value ...            → ConfigCommands
add_gameplay_tags ...           → GASCommands
create_blackboard, add_bt_* ... → AICommands
add_ai_perception_component ... → AIPerceptionCommands
create_eqs_query, add_eqs_* ... → EQSCommands
create_simple_material          → MaterialCommands
```

### サブルーティング

Blueprint/BlueprintNode/UMGWidget/AICommands は内部で更に分割ファイルへ委譲。

---

## 新コマンド追加ガイド

### チェックリスト

1. `Commands/SpirrowBridge*Commands.h` - 関数宣言
2. `Commands/SpirrowBridge*Commands.cpp` - 関数実装
3. `Commands/SpirrowBridge*Commands.cpp` - HandleCommand内ルーティング
4. **`SpirrowBridge.cpp`** - ExecuteCommand内ルーティング ⚠️忘れがち
5. `Python/tools/*_tools.py` - MCPツール定義

### ハンドラ選択

| コマンド種別 | 追加先 |
|-------------|--------|
| BP作成・スポーン・グラフ | `BlueprintCoreCommands` |
| コンポーネント・物理設定 | `BlueprintComponentCommands` |
| クラススキャン・配列 | `BlueprintPropertyCommands` |
| ノード接続・イベント・関数 | `BlueprintNodeCoreCommands` |
| 変数・Get/Set・Self参照 | `BlueprintNodeVariableCommands` |
| Branch・Loop・Math | `BlueprintNodeControlFlowCommands` |
| Widget要素追加 | `UMGWidgetCommands` |
| レイアウト操作 | `UMGLayoutCommands` |
| アニメーション | `UMGAnimationCommands` |
| 変数・バインディング | `UMGVariableCommands` |
| AI Blackboard | `AICommands_Blackboard` |
| AI BehaviorTree | `AICommands_BehaviorTree` |
| AI BTノード追加 | `AICommands_BTNodeCreation` |
| AI BTノード操作 | `AICommands_BTNodeOperations` |
| GAS | `GASCommands` |
| アクター操作 | `EditorCommands` |
| 入力・プロジェクト | `ProjectCommands` |
| Config | `ConfigCommands` |
| マテリアル | `MaterialCommands` |

---

## 主要ユーティリティ (CommonUtils)

### バリデーション
- `ValidateRequiredString/Number/Bool` - 必須パラメータ検証
- `GetOptionalString/Number/Bool` - オプショナル取得
- `ValidateBlueprint/WidgetBlueprint` - アセット存在確認

### JSON
- `CreateErrorResponse` - エラーレスポンス作成
- `CreateSuccessResponse` - 成功レスポンス作成
- `GetVectorFromJson/GetLinearColorFromJson` - 型変換

### ノード操作
- `CreateEventNode/FunctionCallNode` - ノード作成
- `ConnectGraphNodes` - ノード接続
- `FindPin` - ピン検索

> エラーコード一覧: [ERROR_CODES.md](ERROR_CODES.md)

---

## UE 5.6+ API互換性

### Decorator格納方式の変更

```cpp
// 旧 (UE 5.5)
CompositeNode->Decorators.Add(Decorator);

// 新 (UE 5.6+)
for (FBTCompositeChild& Child : Parent->Children) {
    if (Child.ChildComposite == Target || Child.ChildTask == Target) {
        Child.Decorators.Add(Decorator);
    }
}
```

### TryGetField変更

```cpp
// 旧
Params->TryGetField(TEXT("key"), OutPtr);

// 新
TSharedPtr<FJsonValue> Value = Params->TryGetField(TEXT("key"));
```

---

## 分割候補

| ファイル | サイズ | 状態 |
|----------|--------|------|
| `GASCommands` | 55 KB | 📋 将来検討 |
| `UMGVariableCommands` | 40 KB | 📋 将来検討 |
| `CommonUtils` | 35 KB | 📋 将来検討 |
