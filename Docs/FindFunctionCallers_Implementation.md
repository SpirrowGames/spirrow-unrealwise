# Blueprint参照検索機能 (find_function_callers) 実装

## 概要

C++またはBlueprint関数の呼び出し元を検索する機能。
C++関数のシグネチャ変更や削除の前に、影響を受けるBlueprintを特定できます。

## 背景・目的

SpirrowUnrealWiseはC++コードを修正できますが、その関数がどのBlueprintから使用されているか把握できませんでした。
リファクタリングや関数削除の前に依存関係を可視化するため、この機能を実装しました。

## 実装仕様

### パラメータ

| パラメータ名 | 型 | 必須 | デフォルト | 説明 |
|-------------|-----|------|-----------|------|
| function_name | string | 必須 | - | 検索対象の関数名 |
| class_name | string | オプション | - | クラス名でフィルタ |
| path_filter | string | オプション | - | 検索対象パス（例: `/Game/Characters/`） |
| include_blueprint_functions | bool | オプション | true | Blueprint関数も含めるか |

### 戻り値

```json
{
  "success": true,
  "function_name": "SetVisibility",
  "usages": [
    {
      "blueprint": "DatasmithLayer",
      "blueprint_path": "/Game/Blueprints/DatasmithLayer",
      "graph": "UpdateHierarchy",
      "node_title": "SetVisibility",
      "node_position": {"x": 256, "y": 128},
      "owning_class": "SceneComponent"
    }
  ],
  "total_count": 126,
  "search_method": "DirectGraphTraversal",
  "search_time_ms": 13.25,
  "blueprints_searched": 85
}
```

## 実装内容

### 1. C++側（SpirrowBridge Plugin）

#### ファイル: `SpirrowBridgeProjectCommands.h`
- `HandleFindFunctionCallers` メソッド宣言を追加

#### ファイル: `SpirrowBridgeProjectCommands.cpp`

**インクルード:**
```cpp
#include "FindInBlueprintManager.h"  // Blueprint検索用
#include "Async/Async.h"              // 非同期処理用
```

**実装概要:**
1. AssetRegistry から全Blueprint一覧を取得
2. `path_filter` でBlueprintをフィルタリング
3. 各Blueprintのグラフを走査
4. `UK2Node_CallFunction` ノードを検索
5. 関数名・クラス名でマッチング
6. 結果をJSON形式で返却

**検索アルゴリズム:**
```cpp
// 各Blueprintを走査
for (const FAssetData& AssetData : BlueprintAssets)
{
    UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());

    // 全グラフを取得
    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);

    // 各グラフのノードを検索
    for (UEdGraph* Graph : Graphs)
    {
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node);
            UFunction* Function = CallNode->GetTargetFunction();

            // 関数名をマッチング
            if (Function->GetName() == FunctionName)
            {
                // ヒット！結果に追加
            }
        }
    }
}
```

#### ファイル: `SpirrowBridge.cpp`
- メインディスパッチャーに `find_function_callers` を登録

### 2. Python側（MCP Server）

#### ファイル: `tools/blueprint_tools.py`

```python
@mcp.tool()
def find_cpp_function_in_blueprints(
    ctx: Context,
    function_name: str,
    class_name: Optional[str] = None,
    path_filter: Optional[str] = None,
    include_blueprint_functions: bool = True
) -> Dict[str, Any]:
    """
    C++またはBlueprint関数の呼び出し元を検索
    """
    unreal = get_unreal_connection()

    params = {"function_name": function_name}
    if class_name:
        params["class_name"] = class_name
    if path_filter:
        params["path_filter"] = path_filter
    params["include_blueprint_functions"] = include_blueprint_functions

    response = unreal.send_command("find_function_callers", params)
    return response
```

## テスト結果

### 環境
- Unreal Engine: 5.7
- テストプロジェクト: MCPGameProject
- Blueprintアセット数: 85個

### パフォーマンス

| メトリック | 値 |
|-----------|-----|
| 検索速度 | 11-14ms（全85 Blueprint） |
| メモリ使用 | 低（Blueprintを順次読み込み） |
| 成功率 | 100% |

### テストケース

#### テスト1: PrintString（5件ヒット）
```
検索時間: 13.85ms
結果:
  - StandardMacros (Create and Assign MID) - KismetSystemLibrary
  - RenderToTexture_LevelBP (Phys Drop Event Graph) - KismetSystemLibrary
  - ImportedSequencesHelper (AddAnimationSampleUI) - KismetSystemLibrary
  + 2件
```

#### テスト2: SetVisibility（126件ヒット）
```
検索時間: 13.25ms
結果:
  - DatasmithLayer (UpdateHierarchy) - SceneComponent
  - DatasmithSelector (EventGraph) - SceneComponent
  - FS_AnchorField_Generic (SetFalloffVisibility) - SceneComponent
  + 123件
```

#### テスト3: 存在しない関数（0件）
```
検索時間: 12.31ms
結果: 正常に0件を返却
```

## 使用例

### 基本的な使用
```python
# 関数名だけで検索
find_cpp_function_in_blueprints(
    function_name="DealDamage"
)
```

### クラスフィルタ付き
```python
# 特定クラスの関数に絞り込み
find_cpp_function_in_blueprints(
    function_name="TakeDamage",
    class_name="ACharacter"
)
```

### パスフィルタ付き
```python
# 特定フォルダ内のBlueprintのみ検索
find_cpp_function_in_blueprints(
    function_name="Attack",
    path_filter="/Game/Characters/"
)
```

### C++関数のみ検索
```python
# Blueprint関数を除外
find_cpp_function_in_blueprints(
    function_name="BeginPlay",
    include_blueprint_functions=False
)
```

## ユースケース

### 1. リファクタリング前の影響調査
```
状況: C++関数 DealDamage のシグネチャを変更したい
手順:
1. find_cpp_function_in_blueprints(function_name="DealDamage")
2. 全ての呼び出し元Blueprintを特定
3. 影響範囲を把握してから変更実施
```

### 2. 未使用関数の特定
```
状況: 使われていないC++関数を削除したい
手順:
1. find_cpp_function_in_blueprints(function_name="OldFunction")
2. total_count が 0 なら安全に削除可能
3. ヒットがあれば先に参照を削除
```

### 3. 依存関係の可視化
```
状況: 特定のクラスがどこから使われているか調査
手順:
1. find_cpp_function_in_blueprints(class_name="MyCharacterClass")
2. 全ての使用箇所を把握
3. ドキュメント化や設計見直しに活用
```

## 技術的な詳細

### 検索方式の選定

**採用: 直接グラフ走査**
- Blueprint グラフを直接読み込んで走査
- 確実に全ての関数呼び出しを検出
- パフォーマンスも十分（85BP を 12ms）

**未採用: FFindInBlueprintSearchManager**
- Unreal標準の検索マネージャー
- タイムアウトリスクがあるため見送り
- 将来的にフォールバック実装として追加可能

### パフォーマンス最適化

1. **path_filter による事前フィルタリング**
   - AssetRegistry で取得したリストを事前に絞り込み
   - 検索対象Blueprintを削減

2. **順次読み込み**
   - Blueprintを一度に全てロードせず順次処理
   - メモリ使用量を抑制

3. **早期リターン**
   - ノードタイプの判定で早期に除外
   - 不要な処理をスキップ

## 制約・注意点

### 1. Event ノードは検出対象外
- `BeginPlay` などの Event は `UK2Node_Event` として実装
- `UK2Node_CallFunction` ではないため検出されない
- これは仕様通りの動作

### 2. Blueprint読み込みコスト
- 大量のBlueprint（数百個）がある場合は検索時間が増加
- path_filter の使用を推奨

### 3. Delegate呼び出し
- Delegate経由の関数呼び出しは検出されない
- 直接的な `UK2Node_CallFunction` のみが対象

## 今後の拡張可能性

### 1. 非同期検索の実装
```cpp
// AsyncTask でバックグラウンド処理
// UI側で進捗表示が可能に
AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [&]() {
    // 検索処理
});
```

### 2. 変数参照検索
```python
find_variable_usage(
    variable_name="Health",
    blueprint_path="/Game/Characters/BP_Player"
)
```

### 3. クラス参照検索
```python
find_class_in_blueprints(
    class_name="AMyCharacter",
    include_derived_classes=True
)
```

### 4. FFindInBlueprintSearchManager の活用
```cpp
// Unreal標準の検索システムを活用
// インデックスベースの高速検索
FFindInBlueprintSearchManager::Get().QuerySearchResults(Query);
```

## トラブルシューティング

### コンパイルエラー: FindInBlueprintManager.h が見つからない
```
原因: モジュール依存関係の不足
解決: SpirrowBridge.Build.cs に UnrealEd モジュールを追加
```

### 検索結果が0件
```
確認項目:
1. Unreal Editor のログを確認
   LogTemp: SpirrowBridge: Searching for function 'XXX' in N Blueprints
2. 関数名のスペルミスをチェック
3. path_filter が厳しすぎないか確認
4. Blueprint内で実際に関数が呼ばれているか確認
```

### 検索が遅い
```
対策:
1. path_filter で検索範囲を限定
2. 不要なBlueprintは検索対象から除外
3. Blueprintの数を確認（blueprints_searched の値）
```

## 関連ドキュメント

- [SpirrowBridge Plugin Architecture](../MCPGameProject/Plugins/SpirrowBridge/README.md)
- [MCP Tool Development Guide](../Python/README.md)
- [Blueprint Node Reference (Unreal Docs)](https://docs.unrealengine.com/)

## 変更履歴

| バージョン | 日付 | 変更内容 |
|-----------|------|---------|
| v1.0.0 | 2025-01-26 | 初回実装: 基本的な関数検索機能 |

## 実装者ノート

- 直接グラフ走査方式を採用したことで、確実性とパフォーマンスを両立
- 85個のBlueprintを12msで検索できるため、path_filterなしでも実用的
- 将来的には検索結果のキャッシュ機能を追加すると更に高速化可能
- Event ノードの検索も追加すると、より包括的な依存関係分析が可能になる
