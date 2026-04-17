# SpirrowBridge 実装サマリ

C++ 実装の全体像。新しいセッション開始時の参照用。

> **最終更新**: 2026-04-17 | **バージョン**: v0.9.5 (WorldSettings Configuration)

---

## ファイル構成 (30ファイル)

### Commands ディレクトリ

#### Blueprint 系 (3分割 + ルーター)

| ファイル | サイズ | 担当 |
|----------|--------|------|
| `BlueprintCoreCommands` | 23 KB | 作成/コンパイル/スポーン/複製/グラフ |
| `BlueprintComponentCommands` | 26 KB | コンポーネント/プロパティ/物理 |
| `BlueprintPropertyCommands` | 24 KB | クラススキャン/配列プロパティ/DataAsset読取 |
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
| `UMGLayoutCommands` | 40 KB | レイアウト + プロパティ取得 |
| `UMGAnimationCommands` | 23 KB | アニメーション |

#### AI 系 (5分割 + ルーター)

| ファイル | サイズ | 担当 |
|----------|--------|------|
| `AICommands` | 1.5 KB | ルーター |
| `AICommands_Blackboard` | 11 KB | Blackboard操作 |
| `AICommands_BehaviorTree` | 8.5 KB | BehaviorTree操作 |
| `AICommands_BTNodeHelpers` | 8 KB | BTノードヘルパー |
| `AICommands_BTNodeCreation` | 14 KB | BTノード追加 + 自動位置計算 |
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
| `LevelCommands` 🆕 | 12 KB | レベル (.umap) ライフサイクル + WorldSettings (`create_level`, `save_current_level`, `open_level`, `get_world_settings`, `set_world_properties`) |
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
create_level / save_current_level / open_level / get_world_settings / set_world_properties  → LevelCommands  🆕
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

### プロパティ設定
- `SetObjectProperty` - UObject プロパティ設定（TMap対応: 全体上書き / ドット記法で個別エントリ操作）
- `SetStructPropertyValue` - 構造体プロパティ設定
- TMap サポート型: Key=`FName`/`FString`, Value=`int32`/`float`/`double`/`bool`/`FString`/`FName`

### ノード操作
- `CreateEventNode/FunctionCallNode` - ノード作成
- `ConnectGraphNodes` - ノード接続
- `FindPin` - ピン検索
- `SafeCompileBlueprint` - GeneratedClass/SkeletonGeneratedClass 不整合を修正して安全にコンパイル

### v0.9.3 追加ヘルパー
- `ResolveTargetBlueprint(Params, OutBlueprint)` - 通常BPとLevel Script Blueprintを統一解決。`target_type="level_blueprint"` + オプション `level_path` に対応。省略時は `GEditor->GetEditorWorldContext().World()->PersistentLevel->GetLevelScriptBlueprint(true)` から解決
- `FindClassByNameAnywhere(ClassName)` - bare name / U/A プレフィックス / フルパス / `/Script/Engine` プレフィックスを横断検索。ゲームモジュール内のクラスも見つける
- `SpawnExternalPropertySetNode(Graph, OwnerClass, PropName, Pos, OutError)` - 外部クラスの UPROPERTY(BlueprintReadWrite) に `UK2Node_VariableSet` 生成
- `SpawnExternalPropertyGetNode(Graph, OwnerClass, PropName, Pos, OutError)` - 同じく `UK2Node_VariableGet` 生成 (BlueprintVisible 必須)

> エラーコード一覧: [ERROR_CODES.md](ERROR_CODES.md)

---

## v0.9.5 新機能 (2026-04-17)

### LevelCommands 拡張: WorldSettings 2コマンド
- `HandleGetWorldSettings(Params)` — `editor(command="get_world_settings", params={...})`。`GEditor->GetEditorWorldContext().World()->GetWorldSettings()` で AWorldSettings を取得 → `properties` 配列 (省略時は curated preset 9個) で指定したフィールドを reflection で JSON 化
  - 内部 `SetPropertyValueOnJson` ヘルパーで型ごとに処理: `FBoolProperty` / `FFloatProperty` / `FDoubleProperty` / `FIntProperty` / `FInt64Property` / `FNameProperty` / `FStrProperty` は native JSON 型、それ以外 (Class/Object/Struct/Enum) は `ExportTextItem_Direct` で string
  - 存在しない名前は `unknown_properties` 配列へ (エラーにしない)
- `HandleSetWorldProperties(Params)` — `editor(command="set_world_properties", params={"properties": {...}})`。dict を走査し `FSpirrowBridgeCommonUtils::SetObjectProperty(Settings, key, value, err)` で書込み
  - 個別失敗はトップレベルを fail させず `failed: [{property, error}]` に集める (`applied: [names]` と対) 。全失敗時のみ `success=false`
  - 1つでも applied なら `Settings->MarkPackageDirty()` + `PersistentLevel->MarkPackageDirty()` で level を dirty マーク (永続化は `save_current_level` に委ねる)
- Curated preset 9 フィールド (UE 5.7 `AWorldSettings` 準拠): `DefaultGameMode` (UI 上は "GameMode Override" と表示), `DefaultPhysicsVolumeClass`, `KillZ`, `KillZDamageType`, `WorldToMeters`, `GlobalGravityZ`, `TimeDilation`, `bEnableWorldBoundsChecks`, `bEnableWorldComposition`
- 追加 include: `GameFramework/WorldSettings.h`

### SpirrowBridge.cpp ルーティング拡張
- Level commands 分岐に `get_world_settings` / `set_world_properties` を OR 追加 (LevelCommands 側で dispatch)

---

## v0.9.4 新機能 (2026-04-17)

### LevelCommands (新規ファイル)
- `FSpirrowBridgeLevelCommands::HandleCreateLevel(Params)` — `editor(command="create_level", ...)` の実装。`UEditorLevelLibrary::NewLevel(AssetPath)` または `UEditorLevelLibrary::NewLevelFromTemplate(AssetPath, TemplatePath)` をラップ
  - `template="default"` → `NewLevel` (UE 5.7 既定 = WP 有効)
  - `template="empty"` → `NewLevelFromTemplate(.., "/Engine/Maps/Templates/Empty")` (非WP)
  - `template="/Game/..."` → 任意の既存レベルをテンプレートに使用 (事前に `UEditorAssetLibrary::DoesAssetExist` 検証)
  - 事前検証: `name` の不正文字 (スペース/スラッシュ/バックスラッシュ/コロン/ワイルドカード), `path` が `/Game/` 以下であること, 同名アセットが既存で `overwrite=false` なら fail。`overwrite=true` なら `UEditorAssetLibrary::DeleteAsset` 後に再作成
- `FSpirrowBridgeLevelCommands::HandleSaveCurrentLevel(Params)` — `editor(command="save_current_level")` の実装。`UEditorLoadingAndSavingUtils::SaveCurrentLevel()` (from `FileHelpers.h`) を呼ぶ。`GEditor->GetEditorWorldContext().World()` と PersistentLevel の null チェック付き
- `FSpirrowBridgeLevelCommands::HandleOpenLevel(Params)` — `editor(command="open_level", ...)` の実装。`UEditorLevelLibrary::LoadLevel(AssetPath)`。事前に `DoesAssetExist` で存在検証。dirty な状態で呼ぶと UE 側の save dialog が出る可能性あり (UEの既定挙動)
- **注意**: `UEditorLevelLibrary::{NewLevel, NewLevelFromTemplate, LoadLevel}` は UE 5.x で deprecation 警告が出るが、`EditorScriptingUtilities` モジュールは既に `Build.cs` の private dependency に含まれている (統一性優先)。将来ビルドが通らなくなったら `ULevelEditorSubsystem` に差し替え (別モジュール `LevelEditor` 追加要)

### SpirrowBridge.h/cpp 拡張
- `LevelCommands` メンバ (TSharedPtr) 追加
- `ExecuteCommand` で `create_level` / `save_current_level` / `open_level` を LevelCommands にルーティング

---

## v0.9.3 新機能 (2026-04-12)

### BlueprintNodeCoreCommands 拡張
- `HandleAddBlueprintFunctionCall`: `function_name` が `Set<Prop>` / `Get<Prop>` / `K2_Set<Prop>` / `K2_Get<Prop>` で関数として見つからない場合、外部クラスの UPROPERTY に自動 fallback (spawn `UK2Node_VariableSet`/`Get`)。レスポンスに `fallback_variable_node: true`, `fallback_kind`, `target_class`, `property_name` が付く
- `HandleSetNodePinValue`: Class/SoftClass/Object/SoftObject/Interface ピンに対応。内部でピンカテゴリを判定して `Pin->DefaultValue` (プリミティブ・struct) と `Pin->DefaultObject` (Class/Object、`K2Schema->TrySetDefaultObject` 経由) を自動選択。Class ピンは `PinSubCategoryObject` に対する `IsChildOf` 型チェック + 設定後に `Node->ReconstructNode()` で下流型を narrow
- 全 8 ハンドラが `ResolveTargetBlueprint` 経由でLSB対応 (`target_type="level_blueprint"`)

### BlueprintNodeVariableCommands 拡張 (+3 handlers)
- `HandleAddExternalPropertySetNode`: 外部クラスの UPROPERTY(BlueprintReadWrite) に `UK2Node_VariableSet` を生成。`target_class` + `property_name` を受け取り、`FindClassByNameAnywhere` → `SpawnExternalPropertySetNode` で生成
- `HandleAddExternalPropertyGetNode`: 同じく `UK2Node_VariableGet` を生成 (BlueprintVisible 要件)
- `HandleAddGetSubsystemNode`: `UK2Node_GetSubsystem::Initialize(UClass*)` でクラスを焼き込んだ typed ノードを生成。`subsystem_kind` に応じて 4 種類のK2ノードクラス (`UK2Node_GetSubsystem` / `UK2Node_GetEngineSubsystem` / `UK2Node_GetSubsystemFromPC`) を選択。`Initialize` を `AllocateDefaultPins` より前に呼ぶ順序が重要
- 全 6 ハンドラが `ResolveTargetBlueprint` 経由でLSB対応

### BlueprintNodeControlFlowCommands 拡張
- 全 7 ハンドラ (branch, sequence, delay, forloop_with_break, print_string, math, comparison) が `ResolveTargetBlueprint` 経由でLSB対応

### BlueprintCoreCommands 拡張
- `HandleCompileBlueprint`: `ResolveTargetBlueprint` 対応。LSB のコンパイルに使える
- `HandleGetBlueprintGraph`: 同じく `ResolveTargetBlueprint` 対応

### CommonUtils 拡張 (ResolveTargetBlueprint + 外部プロパティ/クラス探索ヘルパー)
(上記「v0.9.3 追加ヘルパー」セクション参照)

### AICommands_BTNodeCreation ビルド修正
- `GenerateUniqueNodeName` を `GenerateUniqueBTNodeNameForCreation` にリネーム。BTNodeOperations.cpp の同名関数 (anonymous namespace) と unity build 時に ambiguity を引き起こしていたため

### 新エラーコード活用
- `PropertyTypeMismatch`: Class ピン設定時の期待 base class チェック、Subsystem kind チェックで使用

---

## v0.8.7 新機能 (2026-01-10)

### UMGLayoutCommands 拡張
- `get_widget_element_property`: プロパティ値取得（ネストプロパティ対応）
- `set_widget_element_property`: ネストプロパティ対応（`Brush.TintColor` 形式）
- `get_widget_elements`: オプション追加（`include_properties`, `class_filter`, `property_filter`, `exclude_default_values`）

### BlueprintNodeCoreCommands 拡張
- `disconnect_blueprint_nodes`: ピン接続切断（3モード対応）
- `add_blueprint_event_node`: BlueprintImplementableEvent オーバーライド対応

### ProjectCommands 拡張 (Enhanced Input)
- `get_input_mapping_context`: IMC内容読み取り
- `get_input_action`: InputAction詳細取得
- `remove_action_from_mapping_context`: IMCからアクション削除
- `add_action_to_mapping_context`: Scalarモディファイア オブジェクト形式対応

### 新エラーコード
- `NodeAlreadyExists` (1217)
- `PropertyTypeMismatch` (1218)

---

## UE 5.6+ API互換性

### BTノード自動位置計算 (v0.8.6)

`AICommands_BTNodeCreation.cpp` にヘルパー関数を追加:

```cpp
// レイアウト定数
static constexpr int32 BT_HORIZONTAL_SPACING = 300;
static constexpr int32 BT_VERTICAL_SPACING = 150;

// 親ノードの既存子ノード数を取得
static int32 GetExistingChildCount(UBehaviorTreeGraphNode* ParentNode);

// 子ノードの自動位置を計算
static void CalculateChildNodePosition(
    UBehaviorTreeGraphNode* ParentNode,
    int32& OutPosX,
    int32& OutPosY);
```

**動作**:
- `parent_node_id` 指定時に自動計算
- Y = 親Y + 150px
- X = 親X + (兄弟数 × 300px)
- `node_position` 手動指定で上書き可能

---

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
