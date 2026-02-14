# 機能名: 壊れたBehaviorTreeノード検出・修復機能

## 概要

BehaviorTreeの開発中、コンパイルエラーやDecoratorの再追加などにより、NodeInstanceがnullになった「壊れたノード」が発生することがあります。これらのノードはEditor上で赤いエラーマークとして表示され、通常の`list_bt_nodes`では検出できません。

この機能は以下を提供します:
- **壊れたノードの検出**: NodeInstance == nullのノードをすべて検出
- **壊れたノードの自動削除**: 検出された壊れたノードを自動的にクリーンアップ

## 問題の背景

### 従来の`list_bt_nodes`の制限

**SpirrowBridgeAICommands_BTNodeOperations.cpp:1159および1176**で、NodeInstanceがnullのノードは自動的に除外されていました:

```cpp
// 1159行目
if (!BTNode || !BTNode->NodeInstance) continue;

// 1176行目 (Decoratorのループ内)
if (Decorator && Decorator->NodeInstance)  // nullは追加されない
```

これにより、壊れたノードは通常のツールでは検出不可能でした。

## 実装内容

### 1. C++側 (Unreal Plugin)

#### SpirrowBridgeAICommands.h

新しいコマンドハンドラーを追加:

```cpp
/**
 * Detect broken nodes (nodes with null NodeInstance) in a BehaviorTree.
 */
TSharedPtr<FJsonObject> HandleDetectBrokenBTNodes(const TSharedPtr<FJsonObject>& Params);

/**
 * Delete broken nodes (nodes with null NodeInstance) from a BehaviorTree.
 */
TSharedPtr<FJsonObject> HandleDeleteBrokenBTNodes(const TSharedPtr<FJsonObject>& Params);
```

#### SpirrowBridgeAICommands_BTNodeOperations.cpp

**HandleDetectBrokenBTNodes**:
- BehaviorTreeGraph内の全ノードを走査
- NodeInstance == nullのノードを検出
- Composite、Task、Decorator、Serviceすべてに対応
- 検出結果をJSON配列で返却

**HandleDeleteBrokenBTNodes**:
- 壊れたノードを収集
- 各ノードに対して以下の処理を実行:
  1. RuntimeNodeの参照を保持
  2. すべてのピン接続を解除（`BreakAllPinLinks`）
  3. Decorator/Serviceの場合、親ノードの配列から削除
  4. `NodeInstance`をnullにクリア
  5. `BTGraph->RemoveNode()`でグラフから削除
  6. RuntimeNodeを`MarkAsGarbage()`
- `FinalizeAndSaveBTGraphInternal()`でグラフ更新と保存
- 削除されたノードの詳細を返却

**重要な修正（v0.9.0-fix1）:**
既存の`HandleDeleteBTNode`と同じロジックを適用することで、エディタ表示からの完全削除を実現しました。

#### コマンドルーティング (SpirrowBridge.cpp / SpirrowBridgeAICommands.cpp)

新しいコマンド`detect_broken_bt_nodes`と`delete_broken_bt_nodes`を登録。

### 2. Python側 (MCP Tool)

#### ai_tools.py

**detect_broken_bt_nodes**:
```python
@mcp.tool()
def detect_broken_bt_nodes(
    ctx: Context,
    behavior_tree_name: str,
    path: str = "/Game/AI/BehaviorTrees"
) -> Dict[str, Any]:
```

- BehaviorTreeの壊れたノードをリスト表示
- 各ノードの種類、クラス名、位置情報を返却

**fix_broken_bt_nodes**:
```python
@mcp.tool()
def fix_broken_bt_nodes(
    ctx: Context,
    behavior_tree_name: str,
    path: str = "/Game/AI/BehaviorTrees"
) -> Dict[str, Any]:
```

- 壊れたノードを自動削除
- 削除されたノードの詳細を返却
- **警告**: 元に戻せない操作のため、バックアップ推奨

## 使用方法

### 1. 壊れたノードの検出

```python
# Claude.ai / Claude Codeから実行
result = detect_broken_bt_nodes(
    behavior_tree_name="BT_AIFighter",
    path="/Game/AI/BehaviorTrees"
)

# 結果を確認
if result["broken_count"] > 0:
    print(f"Found {result['broken_count']} broken nodes!")
    for node in result["broken_nodes"]:
        print(f"- {node['node_type']}: {node['node_class']} at {node['position']}")
```

### 2. 壊れたノードの削除

```python
# まず検出
detect_result = detect_broken_bt_nodes(behavior_tree_name="BT_AIFighter")

# 壊れたノードが見つかった場合のみ削除
if detect_result["broken_count"] > 0:
    fix_result = fix_broken_bt_nodes(behavior_tree_name="BT_AIFighter")
    print(f"Deleted {fix_result['deleted_count']} broken nodes")
```

## レスポンス形式

### detect_broken_bt_nodes

```json
{
  "success": true,
  "behavior_tree_name": "BT_AIFighter",
  "broken_nodes": [
    {
      "node_type": "Decorator",
      "node_class": "UBehaviorTreeGraphNode_Decorator",
      "position": [100, 200],
      "attached_to": "BTComposite_Selector_0"
    }
  ],
  "broken_count": 1,
  "message": "Found 1 broken node(s) with null NodeInstance"
}
```

### fix_broken_bt_nodes

```json
{
  "success": true,
  "behavior_tree_name": "BT_AIFighter",
  "deleted_nodes": [
    {
      "node_type": "Decorator",
      "node_class": "UBehaviorTreeGraphNode_Decorator"
    }
  ],
  "deleted_count": 1,
  "message": "Deleted 1 broken node(s)"
}
```

## テスト手順

### 前提条件
- Unreal Engine 5.5が起動している
- MCPサーバーが実行中
- BehaviorTreeに壊れたノード（赤エラーマーク）が存在する

### テストステップ

1. **壊れたノードの検出**
   ```python
   result = detect_broken_bt_nodes(behavior_tree_name="BT_AIFighter")
   assert result["success"] == True
   assert result["broken_count"] > 0
   ```

2. **壊れたノードの削除**
   ```python
   fix_result = fix_broken_bt_nodes(behavior_tree_name="BT_AIFighter")
   assert fix_result["success"] == True
   assert fix_result["deleted_count"] == result["broken_count"]
   ```

3. **削除後の確認**
   ```python
   verify_result = detect_broken_bt_nodes(behavior_tree_name="BT_AIFighter")
   assert verify_result["broken_count"] == 0
   ```

4. **Editor確認**
   - Unreal EditorでBehaviorTreeを開く
   - 赤エラーマークのノードが消えていることを確認

## 注意事項

1. **元に戻せない操作**
   - `fix_broken_bt_nodes`は削除操作のため、実行前にバックアップ推奨
   - Git/Perforceなどのバージョン管理を推奨

2. **NodeInstanceがnullの原因**
   - コンパイルエラー後の残骸
   - Decoratorの削除→再追加の繰り返し
   - Blueprintクラスの削除後の参照エラー

3. **検出されるノードタイプ**
   - Composite (Selector/Sequence/SimpleParallel)
   - Task
   - Decorator
   - Service

## トラブルシューティング

### 問題: 壊れたノードが検出されない
- **原因**: BehaviorTreeがまだロードされていない
- **解決策**: Unreal EditorでBehaviorTreeを一度開いてから実行

### 問題: 削除後もEditorに赤エラーマークが残る（v0.9.0-fix1で修正済み）
- **原因**: ピン接続や親配列からの削除が不完全だった
- **解決策**:
  - v0.9.0-fix1以降を使用
  - BehaviorTreeを閉じて再度開く
  - Editor再起動

### 問題: "BehaviorTree has no graph" エラー
- **原因**: BehaviorTreeアセットが破損している
- **解決策**: 新しいBehaviorTreeを作成し、ノードを手動で移行

### 問題: 削除後にクラッシュする
- **原因**: RuntimeNodeの参照が残っている
- **解決策**: v0.9.0-fix1では`MarkAsGarbage`で解決済み

## 補足事項

### 将来の拡張案

1. **プレビューモード**: 削除前に影響範囲を表示
2. **バッチ処理**: 複数のBehaviorTreeを一度に処理
3. **自動バックアップ**: 削除前に自動スナップショット作成
4. **修復オプション**: 削除ではなく、NodeInstanceの再作成を試みる

### 関連ファイル

- **C++ Implementation**:
  - `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Public/Commands/SpirrowBridgeAICommands.h`
  - `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeAICommands_BTNodeOperations.cpp`
  - `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/SpirrowBridge.cpp`
  - `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeAICommands.cpp`

- **Python Implementation**:
  - `Python/tools/ai_tools.py`

## バージョン履歴

- **v0.9.0** (2026-02-15): 初回リリース
  - `detect_broken_bt_nodes` 実装
  - `fix_broken_bt_nodes` 実装
  - Decorator/Service内の壊れたノードも検出可能

- **v0.9.0-fix1** (2026-02-15): エディタ表示削除問題を修正
  - `HandleDeleteBTNode`と同じ削除ロジックを適用
  - ピン接続の解除処理を追加
  - Decorator/Serviceの親配列からの削除処理を追加
  - `FinalizeAndSaveBTGraphInternal`呼び出しを追加
  - RuntimeNodeの`MarkAsGarbage`処理を追加
