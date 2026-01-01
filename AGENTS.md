# AGENTS.md - spirrow-unrealwise MCP

このドキュメントは、AIエージェントがspirrow-unrealwise MCPツールを使用する際のガイドラインです。

---

## 概要

spirrow-unrealwiseは、Unreal Engine 5とMCP（Model Context Protocol）を接続し、LLMからUEエディタを操作するためのツール群です。

**主な用途**:
- Blueprint作成・編集
- アクター操作
- ノードグラフ構築
- UMG Widget作成

---

## ノード配置ルール

### 基本設定

```
水平間隔: 300px
垂直間隔: 150px
起点: [0, 0]
```

### レイアウトパターン

#### 1. 直列（Linear）

```
[Event] → [Node A] → [Node B] → [Node C]
[0, 0]    [300, 0]   [600, 0]   [900, 0]
```

```python
# 計算式
x = index * 300
y = 0
```

#### 2. 分岐（Branch）

```
              → [Node B] [300, 0]
[Event] → [Branch]
              → [Node C] [300, 150]
[0, 0]    [300, 0]
```

分岐後のノードは下方向（+Y）に展開:
```python
# 分岐先の計算
x = parent_x + 300
y = branch_index * 150
```

#### 3. 合流（Merge）

```
[Node A] →
              → [Node C]
[Node B] →
```

合流ノードは最も下の入力ノードのY座標 + 75（中央揃え）に配置。

#### 4. 複雑なグラフ

複数の分岐・合流がある場合:
1. 左から右へ処理順に配置
2. 分岐は下方向に展開
3. 合流点は分岐の中央Y座標に配置
4. 重なりそうな場合は垂直間隔を広げる（150 → 200）

---

## Blueprint作成のベストプラクティス

### 命名規則

| 種類 | プレフィックス | 例 |
|------|---------------|-----|
| Actor Blueprint | `BP_` | `BP_Enemy`, `BP_Projectile` |
| Widget Blueprint | `WBP_` | `WBP_MainMenu`, `WBP_HUD` |
| Component | なし（説明的な名前） | `CubeMesh`, `RootCollision` |

### 作成フロー

#### Actor Blueprint の場合
1. **Blueprint作成**: `create_blueprint`
2. **コンポーネント追加**: `add_component_to_blueprint`
3. **メッシュ/プロパティ設定**: `set_static_mesh_properties`, `set_component_property`
4. **イベントノード追加**: `add_blueprint_event_node`
5. **関数ノード追加**: `add_blueprint_function_node`
6. **ノード接続**: `connect_blueprint_nodes`
7. **コンパイル**: `compile_blueprint`

#### Widget Blueprint の場合
1. **Widget Blueprint作成**: `create_blueprint` (parent_class に UserWidget 系を指定)
2. **Widget要素追加**: UMG Editor で手動追加（現在MCPツールは未対応）
3. **イベントノード追加**: `add_blueprint_event_node`
4. **関数ノード追加**: `add_blueprint_function_node`
5. **ノード接続**: `connect_blueprint_nodes`
6. **コンパイル**: `compile_blueprint`

### 関数ノードのtarget指定

| 関数の種類 | target | 例 |
|-----------|--------|-----|
| アクター自身のメソッド | `self` | `SetActorLocation` |
| Kismetライブラリ | `KismetSystemLibrary` | `PrintString`, `Delay` |
| Math系 | `KismetMathLibrary` | `Sin`, `Lerp` |
| コンポーネントのメソッド | コンポーネント名 | `MeshComponent` |

---

## ピン名リファレンス

### 実行ピン

| ノード種類 | 出力ピン | 入力ピン |
|-----------|---------|---------|
| Event | `then` | - |
| Function | `then` | `execute` |
| Branch | `True`, `False` | `execute` |

### 一般的なデータピン

| 型 | ピン名例 |
|----|---------|
| Boolean | `Condition`, `ReturnValue` |
| Float | `Value`, `DeltaTime` |
| Vector | `Location`, `Direction` |
| Actor | `Target`, `OtherActor` |

### Math/Comparisonノードのピン

| ノード種別 | 入力ピン | 出力ピン |
|-----------|---------|----------|
| Math (Add, Subtract, Multiply, Divide) | `A`, `B` | `ReturnValue` |
| Comparison (Greater, Less, Equal等) | `A`, `B` | `ReturnValue` (Boolean) |

---

## エラーハンドリング

### よくあるエラーと対処

| エラー | 原因 | 対処 |
|--------|------|------|
| `Function not found in target self` | グローバル関数をselfで呼んだ | 適切なライブラリ名を指定 |
| `Timeout receiving Unreal response` | UE側の処理遅延/通信エラー | 再試行、またはUEエディタ確認 |
| `Property not found` | プロパティ名が間違っている | UEでプロパティ名を確認 |

### タイムアウト時の対応

1. UEエディタがフリーズしていないか確認
2. 操作自体は成功している可能性があるので、エディタで結果確認
3. 必要に応じて再試行

---

## 制限事項

### 現在対応していない操作

- 既存アクターへのStaticMesh直接設定（Blueprint経由で対応）
- `spawn_blueprint_actor` は不安定（タイムアウトの可能性）
- ノードのパラメータ設定（一部のみ対応）

### 推奨ワークフロー

複雑なBlueprintは:
1. MCPで骨格（ノード構成）を作成
2. UEエディタで詳細パラメータを調整

---

## 使用例

### 例1: BeginPlayでPrintString

```
1. create_blueprint("BP_Example", "Actor")
2. add_blueprint_event_node("BP_Example", "ReceiveBeginPlay", [0, 0])
3. add_blueprint_function_node("BP_Example", "PrintString", "KismetSystemLibrary", [300, 0])
4. connect_blueprint_nodes(source=BeginPlay, target=PrintString, "then" → "execute")
5. compile_blueprint("BP_Example")
```

### 例2: 物理キューブActor

```
1. create_blueprint("BP_PhysicsCube", "Actor")
2. add_component_to_blueprint("BP_PhysicsCube", "CubeMesh", "StaticMeshComponent")
3. set_static_mesh_properties("BP_PhysicsCube", "CubeMesh", "/Engine/BasicShapes/Cube.Cube")
4. set_physics_properties("BP_PhysicsCube", "CubeMesh", simulate_physics=true)
5. compile_blueprint("BP_PhysicsCube")
```

---

## Workflow Rules

### 機能実装フロー

```
1. Claude.ai で機能仕様を詳細に詰める
2. 仕様確定後、実装プロンプトを作成
   - ファイル名: Docs/{FeatureName}_Prompt.md
   - 例: Docs/AddProgressBarToWidget_Prompt.md
3. Claude Code で実装（Python / C++）
4. ビルド・再起動
5. 動作確認
6. ナレッジに学びを蓄積
```

### プロンプトドキュメントの構成

機能追加ごとに Claude Code 用の実装プロンプトを `Docs/` に保存する。

```markdown
# 機能名: 〇〇 実装

## 概要
機能の説明

## 仕様
詳細な仕様（表形式推奨）

## 実装内容

### 1. Python側 (`*.py`)
コードブロック

### 2. Unreal側 (`*.h` / `*.cpp`)
コードブロック

## テスト
テスト手順

## 補足事項
注意点等
```

### プロンプトドキュメントの利点

- 機能と実装内容の紐付けが明確
- 後から「なぜこの実装にしたのか」を追跡可能
- 同様の機能追加で再利用可能
- AI セッション間の引き継ぎがスムーズ

---

## 新しいコマンドの追加手順

### アーキテクチャ概要

```
[Python MCP Server]
       ↓ TCP
[SpirrowBridge.cpp] ExecuteCommand() ← コマンドルーティング（メイン）
       ↓
[各CommandHandler]
  - SpirrowBridgeEditorCommands.cpp   ← アクター操作
  - SpirrowBridgeBlueprintCommands.cpp ← Blueprint操作
  - SpirrowBridgeProjectCommands.cpp   ← プロジェクト操作
  - etc.
```

### チェックリスト（必須）

新しいコマンドを追加する際は、以下の **すべてのファイル** を更新すること：

| # | ファイル | 更新内容 | 場所 |
|---|----------|----------|------|
| 1 | `Commands/SpirrowBridge*Commands.h` | 関数宣言 | private セクション |
| 2 | `Commands/SpirrowBridge*Commands.cpp` | 関数実装 | ファイル末尾 |
| 3 | `Commands/SpirrowBridge*Commands.cpp` | HandleCommand内ルーティング | switch/if文 |
| 4 | **`SpirrowBridge.cpp`** | **ExecuteCommand内ルーティング** | **else if文** |
| 5 | `Python/tools/*_tools.py` | Python側ツール定義 | @mcp.tool() |

⚠️ **重要**: #4 を忘れると「Unknown command」エラーになる！

### 追加例

新しいコマンド `get_actor_components` を追加する場合：

```cpp
// 1. SpirrowBridgeEditorCommands.h
private:
    TSharedPtr<FJsonObject> HandleGetActorComponents(const TSharedPtr<FJsonObject>& Params);

// 2. SpirrowBridgeEditorCommands.cpp (実装)
TSharedPtr<FJsonObject> FSpirrowBridgeEditorCommands::HandleGetActorComponents(...) { ... }

// 3. SpirrowBridgeEditorCommands.cpp (HandleCommand内)
else if (CommandType == TEXT("get_actor_components"))
{
    return HandleGetActorComponents(Params);
}

// 4. SpirrowBridge.cpp (ExecuteCommand内) ← 忘れがち！
else if (CommandType == TEXT("get_actors_in_level") || 
         ...
         CommandType == TEXT("get_actor_components") ||  // ← 追加
         ...)
{
    ResultJson = EditorCommands->HandleCommand(CommandType, Params);
}
```

```python
# 5. Python/tools/editor_tools.py
@mcp.tool()
def get_actor_components(ctx: Context, name: str) -> Dict[str, Any]:
    ...
```

### 過去の不具合事例

#### 2024-12-14: get_actor_components "Unknown command" エラー

**症状**: `get_actor_components` を呼び出すと "Unknown command: get_actor_components" エラー

**原因**: 
- SpirrowBridgeEditorCommands.h/cpp は正しく更新されていた
- SpirrowBridge.cpp の ExecuteCommand 内のルーティングが漏れていた

**教訓**:
- コマンドハンドラ内の HandleCommand だけでなく、SpirrowBridge.cpp の ExecuteCommand も更新が必要
- 「ソースコードは正しいのに動かない」場合、ルーティング漏れを疑う

---

## プラグインのビルドについて

### ビルドが必要な場合

- C++ ソースコードを変更した場合
- ヘッダーファイル（.h）を変更した場合
- 新しいファイルを追加した場合

### ビルド手順

1. **UE エディタを閉じる**（必須）
2. Visual Studio で `.sln` を開く
3. **Build → Rebuild Solution**
4. UE エディタを起動

⚠️ **Live Coding（Ctrl+Alt+F11）では新しい関数の追加は反映されない**

### ビルドが反映されない場合

以下のフォルダを削除してからビルド：
- `Plugins/SpirrowBridge/Binaries`
- `Plugins/SpirrowBridge/Intermediate`
- プロジェクトルートの `Intermediate`

---

## rationale パラメータ（設計根拠の自動記録）

### 概要

主要なMCPツールには `rationale` パラメータがあります。
設計判断の理由を記載すると、自動的にナレッジDBに蓄積されます。

### 対象ツール

| ツール | カテゴリ | 理由 |
|--------|----------|------|
| `create_blueprint` | blueprint | BP設計の根幹 |
| `add_component_to_blueprint` | component | コンポーネント選定理由 |
| `set_physics_properties` | physics | 物理設定の意図 |
| `spawn_actor` | level_design | アクター配置の意図 |
| `set_actor_property` | actor_property | アクタープロパティ設定の意図 |
| `set_actor_component_property` | component_property | コンポーネントプロパティ設定の意図 |
| `add_blueprint_event_node` | blueprint_event | イベント使用の意図 |
| `add_blueprint_function_node` | blueprint_logic | 関数呼び出しの理由 |
| `add_blueprint_variable` | blueprint_variable | 変数の役割 |
| `create_gameplay_effect` | gas_effect | GameplayEffect設計の意図（効果の用途、タグ戦略） |
| `create_gameplay_ability` | gas_ability | GameplayAbility設計の意図（アビリティの役割、発動条件） |
| `create_gas_character` | gas_character | GAS Character設計の意図（デフォルトAbilityとEffect選定理由） |

### 使用例

```python
# 良い例: 設計判断の根拠を明記
create_blueprint(
    name="BP_Enemy",
    parent_class="Character",
    rationale="敵キャラ用。AIControllerで制御し、NavMeshで移動するためCharacterベース"
)

add_component_to_blueprint(
    blueprint_name="BP_Enemy",
    component_type="CapsuleComponent",
    component_name="HitBox",
    rationale="攻撃判定用。RootのCapsuleとは別に、武器ヒット検出専用"
)

set_physics_properties(
    blueprint_name="BP_Ball",
    component_name="Mesh",
    simulate_physics=True,
    mass=10.0,
    rationale="物理演算ボール。質量10kgで適度な弾み具合を実現"
)

spawn_actor(
    name="EnemySpawnPoint_01",
    type="TargetPoint",
    location=[1000, 500, 100],
    rationale="第1ウェーブの敵出現地点。プレイヤー初期位置から見えない位置"
)

# Blueprint ノード・変数
add_blueprint_event_node(
    blueprint_name="BP_Enemy",
    event_name="ReceiveBeginPlay",
    rationale="初期化処理。武器装備とAI起動を実行"
)

add_blueprint_function_node(
    blueprint_name="BP_Player",
    target="self",
    function_name="TakeDamage",
    rationale="ダメージ処理。HP減少とヒットエフェクトを再生"
)

add_blueprint_variable(
    blueprint_name="BP_Enemy",
    variable_name="CurrentHealth",
    variable_type="Float",
    rationale="現在のHP。0で死亡処理をトリガー"
)

# アクター・コンポーネントプロパティ
set_actor_property(
    name="MyLight",
    property_name="bHidden",
    property_value=False,
    rationale="ゲーム開始時は表示。イベント発生で点灯させる"
)

set_actor_component_property(
    actor_name="MyLight",
    component_name="LightComponent0",
    property_name="Intensity",
    property_value=5000,
    rationale="強調表示用。通常の2倍の明るさで注目を引く"
)
```

### 書くべき内容

- **なぜ**その選択をしたか
- 他の選択肢を**なぜ却下**したか
- 将来の**意図・拡張予定**

### 書かなくていい場合

- 機械的な操作（コンパイル、ノード接続など）
- 自明な選択（プレイヤーキャラにPlayerControllerなど）
- テスト・実験的な操作

### ナレッジDBでの活用

記録された rationale は、以下のように活用できます：

```python
# 過去の設計判断を検索
search_knowledge("敵キャラ 設計", category="blueprint")

# 物理設定の根拠を検索
search_knowledge("質量 設定", category="physics")
```

---

## 更新履歴

- 2026-01-01: find_blueprint_nodes修正完了。node_typeをオプショナル化、全ノードタイプ対応（Event/Function/Variable/Branch/Sequence/Macro/InputAction/Self）、詳細情報返却（node_id, node_type, node_class, name, position, pins）
- 2026-01-01: find_actors_by_name修正完了。レスポンス形式改善（success/pattern/count/actorsフィールド追加）でMCP表示問題を解決
- 2026-01-01: Math/Comparisonノード動作確認完了。add_math_node (Add/Subtract/Multiply/Divide)、add_comparison_node (Greater/Less/Equal等) がFloat/Int両対応で動作OK。ピン名: A, B, ReturnValue
- 2025-12-25: 制御フロー・ユーティリティノードツール追加（add_sequence_node, add_delay_node, add_print_string_node）
- 2025-12-16: create_blueprint で UUserWidget を親クラスとして指定可能に。Widget Blueprint の自動生成に対応
- 2025-12-15: GAS Phase 1-B 実装。GAS アセット一覧取得機能（list_gas_assets）を追加
- 2025-12-15: GAS Phase 1-A 実装。Gameplay Tags 管理機能（add_gameplay_tags, list_gameplay_tags, remove_gameplay_tag）を追加
- 2025-12-15: Config（ini）ファイル操作対応を追加。get_config_value, set_config_value, list_config_sections ツールを実装
- 2025-12-15: ObjectProperty（アセット参照）対応を追加。`TObjectPtr<T>`, `TSoftObjectPtr<T>`, `TSubclassOf<T>` 型のプロパティ設定が可能に
- 2025-12-15: set_actor_component_property ツールを追加、set_actor_property と合わせて rationale パラメータを追加
- 2025-12-15: node_tools.py に rationale パラメータを追加（3ツール）
- 2025-12-15: rationale パラメータ機能を追加（4ツール）
- 2025-12-14: 新しいコマンド追加手順、ビルドガイドを追加
- 2025-12-03: 初版作成
