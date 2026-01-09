# BehaviorTree ノード位置設定機能 - 実装指示書 v1
# UE5.7対応

## 概要

BehaviorTree MCPツールで作成したノードが全て同じ位置（0, 0）に重なって表示される問題を解決するための実装指示書。

### 問題の症状
- MCP経由で作成したBTノードがエディタで全て重なって表示される
- 手動でノードを整理する必要がある
- 複雑なBTを作成すると視認性が著しく低下

### 原因
現在のノード作成ツールでは`UEdGraphNode`の位置プロパティ（`NodePosX`, `NodePosY`）を設定していない。

---

## 解決策

### 方針1: 既存ツールに位置パラメータを追加（推奨）

以下のツールに`node_position`パラメータを追加：
- `add_bt_composite_node`
- `add_bt_task_node`
- `add_bt_decorator_node`
- `add_bt_service_node`

### 方針2: 新規ツールを追加

- `set_bt_node_position` - 作成済みノードの位置を変更
- `auto_layout_bt` - BTグラフ全体を自動レイアウト

---

## アーキテクチャ

### グラフノードの位置プロパティ

`UEdGraphNode`（全てのグラフノードの基底クラス）には以下の位置プロパティがある：

```cpp
// Engine/Source/Runtime/Engine/Classes/EdGraph/EdGraphNode.h

UPROPERTY()
int32 NodePosX;

UPROPERTY()
int32 NodePosY;
```

`UBehaviorTreeGraphNode`は`UEdGraphNode`を継承しているため、これらのプロパティを直接設定可能。

### グラフノードへのアクセス

BTノードIDからグラフノードを取得する既存のヘルパー関数を活用：

```cpp
// 既存の実装（BehaviorTreeHelpers.cpp等に存在するはず）
UBehaviorTreeGraphNode* FindGraphNodeById(UBehaviorTreeGraph* BTGraph, const FString& NodeId);
```

---

## 実装パターン

### 1. ノード作成時に位置を設定（方針1）

`FGraphNodeCreator::Finalize()`の後に位置を設定：

```cpp
UBehaviorTreeGraphNode_Composite* CreateCompositeNodeWithPosition(
    UBehaviorTreeGraph* BTGraph,
    UClass* CompositeClass,
    const FString& NodeName,
    int32 PosX,
    int32 PosY)
{
    FGraphNodeCreator<UBehaviorTreeGraphNode_Composite> NodeCreator(*BTGraph);
    UBehaviorTreeGraphNode_Composite* GraphNode = NodeCreator.CreateNode();
    
    // ランタイムノード作成
    UBTCompositeNode* RuntimeNode = NewObject<UBTCompositeNode>(
        GraphNode, CompositeClass, NAME_None, RF_Transactional);
    
    GraphNode->NodeInstance = RuntimeNode;
    GraphNode->ClassData = FGraphNodeClassData(CompositeClass, TEXT(""));
    
    if (!NodeName.IsEmpty())
    {
        RuntimeNode->NodeName = NodeName;
    }
    
    NodeCreator.Finalize();
    
    // ★位置を設定★
    GraphNode->NodePosX = PosX;
    GraphNode->NodePosY = PosY;
    
    return GraphNode;
}
```

### 2. 作成済みノードの位置を変更（方針2）

```cpp
bool SetBTNodePosition(
    UBehaviorTree* BehaviorTree,
    const FString& NodeId,
    int32 PosX,
    int32 PosY)
{
    UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BehaviorTree->BTGraph);
    if (!BTGraph)
    {
        return false;
    }
    
    // ノードIDからグラフノードを検索
    UBehaviorTreeGraphNode* GraphNode = FindGraphNodeById(BTGraph, NodeId);
    if (!GraphNode)
    {
        return false;
    }
    
    // 位置を設定
    GraphNode->NodePosX = PosX;
    GraphNode->NodePosY = PosY;
    
    // グラフ更新を通知
    BTGraph->NotifyGraphChanged();
    
    return true;
}
```

### 3. ノードIDからグラフノードを検索

```cpp
UBehaviorTreeGraphNode* FindGraphNodeById(
    UBehaviorTreeGraph* BTGraph,
    const FString& NodeId)
{
    for (UEdGraphNode* Node : BTGraph->Nodes)
    {
        UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(Node);
        if (!BTNode) continue;
        
        // NodeInstanceのクラス名とインデックスからID生成してマッチング
        if (UBTNode* RuntimeNode = Cast<UBTNode>(BTNode->NodeInstance))
        {
            FString GeneratedId = GenerateNodeId(RuntimeNode);
            if (GeneratedId == NodeId)
            {
                return BTNode;
            }
        }
        
        // Decoratorsもチェック
        for (UBehaviorTreeGraphNode* DecNode : BTNode->Decorators)
        {
            if (UBTNode* RuntimeNode = Cast<UBTNode>(DecNode->NodeInstance))
            {
                FString GeneratedId = GenerateNodeId(RuntimeNode);
                if (GeneratedId == NodeId)
                {
                    return DecNode;
                }
            }
        }
        
        // Servicesもチェック
        for (UBehaviorTreeGraphNode* SvcNode : BTNode->Services)
        {
            if (UBTNode* RuntimeNode = Cast<UBTNode>(SvcNode->NodeInstance))
            {
                FString GeneratedId = GenerateNodeId(RuntimeNode);
                if (GeneratedId == NodeId)
                {
                    return SvcNode;
                }
            }
        }
    }
    
    return nullptr;
}
```

### 4. 自動レイアウト機能

階層構造に基づいて自動的に位置を計算：

```cpp
struct FBTLayoutConfig
{
    int32 HorizontalSpacing = 300;  // 兄弟ノード間の水平間隔
    int32 VerticalSpacing = 150;    // 親子ノード間の垂直間隔
    int32 DecoratorOffsetY = -60;   // デコレータのY方向オフセット
    int32 ServiceOffsetY = -30;     // サービスのY方向オフセット
};

void AutoLayoutBehaviorTree(
    UBehaviorTreeGraph* BTGraph,
    const FBTLayoutConfig& Config = FBTLayoutConfig())
{
    // Rootノードを取得
    UBehaviorTreeGraphNode_Root* RootNode = FindRootNode(BTGraph);
    if (!RootNode) return;
    
    // Rootの位置を基準点に設定
    RootNode->NodePosX = 0;
    RootNode->NodePosY = 0;
    
    // 再帰的に子ノードをレイアウト
    LayoutChildNodes(RootNode, 0, Config);
    
    BTGraph->NotifyGraphChanged();
}

void LayoutChildNodes(
    UBehaviorTreeGraphNode* ParentNode,
    int32 Depth,
    const FBTLayoutConfig& Config)
{
    // 出力ピンから接続された子ノードを取得
    TArray<UBehaviorTreeGraphNode*> ChildNodes;
    GetConnectedChildNodes(ParentNode, ChildNodes);
    
    if (ChildNodes.Num() == 0) return;
    
    // 子ノードの総幅を計算
    int32 TotalWidth = (ChildNodes.Num() - 1) * Config.HorizontalSpacing;
    int32 StartX = ParentNode->NodePosX - TotalWidth / 2;
    int32 ChildY = ParentNode->NodePosY + Config.VerticalSpacing;
    
    for (int32 i = 0; i < ChildNodes.Num(); ++i)
    {
        UBehaviorTreeGraphNode* ChildNode = ChildNodes[i];
        
        // 子ノードの位置を設定
        ChildNode->NodePosX = StartX + i * Config.HorizontalSpacing;
        ChildNode->NodePosY = ChildY;
        
        // デコレータの位置を設定
        int32 DecoratorY = ChildNode->NodePosY + Config.DecoratorOffsetY;
        for (int32 d = 0; d < ChildNode->Decorators.Num(); ++d)
        {
            ChildNode->Decorators[d]->NodePosX = ChildNode->NodePosX;
            ChildNode->Decorators[d]->NodePosY = DecoratorY - d * 40;
        }
        
        // サービスの位置を設定
        int32 ServiceY = ChildNode->NodePosY + Config.ServiceOffsetY;
        for (int32 s = 0; s < ChildNode->Services.Num(); ++s)
        {
            ChildNode->Services[s]->NodePosX = ChildNode->NodePosX + 100;
            ChildNode->Services[s]->NodePosY = ServiceY - s * 30;
        }
        
        // 再帰的に孫ノードをレイアウト
        LayoutChildNodes(ChildNode, Depth + 1, Config);
    }
}

void GetConnectedChildNodes(
    UBehaviorTreeGraphNode* ParentNode,
    TArray<UBehaviorTreeGraphNode*>& OutChildren)
{
    // 出力ピンを取得
    for (UEdGraphPin* Pin : ParentNode->Pins)
    {
        if (Pin->Direction == EGPD_Output)
        {
            for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
            {
                if (UBehaviorTreeGraphNode* ChildNode = Cast<UBehaviorTreeGraphNode>(LinkedPin->GetOwningNode()))
                {
                    OutChildren.Add(ChildNode);
                }
            }
        }
    }
}
```

---

## MCPツール仕様

### set_bt_node_position

作成済みのBTノードの位置を変更する。

**パラメータ:**

| 名前 | 型 | 必須 | 説明 |
|------|-----|------|------|
| behavior_tree_name | string | ✓ | 対象のBehaviorTree名 |
| node_id | string | ✓ | 対象ノードのID |
| position | [int, int] | ✓ | [X, Y] 位置 |
| path | string | - | BTのコンテンツパス（デフォルト: /Game/AI/BehaviorTrees） |

**戻り値:**

```json
{
    "success": true,
    "behavior_tree_name": "BT_MCPTest",
    "node_id": "BTComposite_Selector_0",
    "position": [0, 0]
}
```

### auto_layout_bt

BehaviorTreeグラフ全体を自動的にレイアウトする。

**パラメータ:**

| 名前 | 型 | 必須 | 説明 |
|------|-----|------|------|
| behavior_tree_name | string | ✓ | 対象のBehaviorTree名 |
| path | string | - | BTのコンテンツパス |
| horizontal_spacing | int | - | 兄弟ノード間の水平間隔（デフォルト: 300） |
| vertical_spacing | int | - | 親子ノード間の垂直間隔（デフォルト: 150） |

**戻り値:**

```json
{
    "success": true,
    "behavior_tree_name": "BT_MCPTest",
    "nodes_layouted": 6
}
```

### 既存ツールへのパラメータ追加

`add_bt_composite_node`, `add_bt_task_node`に以下を追加：

| 名前 | 型 | 必須 | 説明 |
|------|-----|------|------|
| node_position | [int, int] | - | [X, Y] 位置。未指定の場合は(0, 0) |

---

## 推奨レイアウト

標準的なBTのレイアウト例：

```
                        [Root] (0, 0)
                           |
                [Selector] (0, 150)
               /          |          \
    [Sequence]      [Sequence]      [Task]
    (-300, 300)     (0, 300)        (300, 300)
        |               |
    [Task]          [Task]
   (-300, 450)     (0, 450)
```

**推奨値:**
- Root: (0, 0)
- 第1階層: Y = 150
- 第2階層: Y = 300
- 第3階層: Y = 450
- 兄弟間隔: X = 300

---

## 実装チェックリスト

### Phase 1: 基本機能

- [ ] `set_bt_node_position`ツールを追加
- [ ] `FindGraphNodeById`ヘルパー関数を実装
- [ ] 位置設定後に`NotifyGraphChanged()`を呼ぶ

### Phase 2: 既存ツール改修

- [ ] `add_bt_composite_node`に`node_position`パラメータ追加
- [ ] `add_bt_task_node`に`node_position`パラメータ追加
- [ ] `add_bt_decorator_node`に`node_position`パラメータ追加
- [ ] `add_bt_service_node`に`node_position`パラメータ追加

### Phase 3: 自動レイアウト

- [ ] `auto_layout_bt`ツールを追加
- [ ] 子ノード取得ロジック実装
- [ ] 再帰的レイアウトアルゴリズム実装
- [ ] デコレータ/サービスの位置計算

---

## テストケース

### Test 1: 単一ノード位置設定

```python
# ノードを作成
result = add_bt_composite_node(
    behavior_tree_name="BT_Test",
    node_type="Selector",
    parent_node_id="Root",
    node_name="TestSelector"
)

# 位置を設定
result = set_bt_node_position(
    behavior_tree_name="BT_Test",
    node_id="BTComposite_Selector_0",
    position=[100, 200]
)

# エディタで位置を確認
```

### Test 2: 作成時に位置指定

```python
result = add_bt_composite_node(
    behavior_tree_name="BT_Test",
    node_type="Sequence",
    parent_node_id="BTComposite_Selector_0",
    node_name="ChildSequence",
    node_position=[-150, 300]
)
```

### Test 3: 自動レイアウト

```python
# 複数ノードを作成
add_bt_composite_node(...)
add_bt_composite_node(...)
add_bt_task_node(...)
add_bt_task_node(...)

# 自動レイアウト実行
result = auto_layout_bt(
    behavior_tree_name="BT_Test",
    horizontal_spacing=300,
    vertical_spacing=150
)

# エディタでレイアウトを確認
```

---

## 参考: UE5.7 ソースコード位置

| ファイル | パス |
|---|---|
| EdGraphNode.h | `Engine/Source/Runtime/Engine/Classes/EdGraph/` |
| BehaviorTreeGraphNode.h | `Engine/Source/Editor/BehaviorTreeEditor/Classes/` |

---

## 更新履歴

- v1.0 (2026-01-09): 初版作成
