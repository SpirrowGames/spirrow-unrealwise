---
id: spirrow-unrealwise:bt-bugfix-prompt
title: BT BugFix Decorator重複/Delete/ListNodes
product: spirrow-unrealwise
type: brief
status: active
version: 1.0
created: 2026-01-09
last_verified: 2026-09-10
supersedes: []
related: []
keywords: [BehaviorTree, バグ修正, Decorator, プロンプト]
legacy_drive_id: [1Crr_zwyWjE7jw6U9E6ud8kMT35qH9v-6j2z1YjSxpik]
---

# Claude Code 引き継ぎ指示書: BehaviorTree バグ修正

## コンテキスト
- **プロジェクト**: spirrow-unrealwise
- **フェーズ**: Phase H (v0.8.4) → v0.8.5
- **関連Issue**: SpirrowUnrealWise_BT_Issues.md

## 概要

TrapxTrapCppプロジェクトでBehaviorTree構築中に発見された3つの問題を修正する。

| # | 問題 | 深刻度 |
|---|------|--------|
| 1 | Decoratorが意図しないノード（ROOT）にも付与される | 高 |
| 2 | delete_bt_nodeで削除成功と返っても実際にはノードが残る | 高 |
| 3 | ノード一覧取得APIがない（デバッグ困難） | 中 |

---

## 前提条件

### 関連ファイル
- `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeAICommands_BTNodeCreation.cpp`
- `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeAICommands_BTNodeOperations.cpp`
- `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Public/Commands/SpirrowBridgeAICommands.h`
- `Python/tools/ai_tools.py`

### 現在の実装状態
- Graph-Based実装（UE 5.7対応）
- `FGraphNodeCreator`パターンでノード作成
- `BTGraph->UpdateAsset()`でランタイムツリー再構築

---

## 問題1: Decorator重複付与の修正

### 原因分析

`HandleAddBTDecoratorNode`で以下の処理を行っている:

```cpp
// 現在のコード
TargetGraphNode->Decorators.Add(DecoratorGraphNode);
DecoratorGraphNode->ParentNode = Cast<UAIGraphNode>(TargetGraphNode);
```

**推測される原因**:
1. `UpdateAsset()`内でランタイムノードのDecorator登録が別途行われている
2. グラフノードの`Decorators`配列とランタイムノードの`Decorators`配列が不整合
3. ROOTノードがデフォルトターゲットとして扱われている可能性

### 修正方針

1. **`UpdateAsset()`呼び出し前にランタイムノードの整合性を確認**
2. **Decoratorのランタイム側オーナー設定を明示的に行う**
3. **デバッグログを追加して重複の原因を特定**

### 修正内容

`SpirrowBridgeAICommands_BTNodeCreation.cpp` の `HandleAddBTDecoratorNode`:

```cpp
// ★ 修正: ランタイムノードのオーナー設定を明示的に行う ★
UBTNode* TargetRuntimeNode = Cast<UBTNode>(TargetGraphNode->NodeInstance);
if (TargetRuntimeNode)
{
    // デコレータのオーナーを設定
    RuntimeDecorator->InitializeFromAsset(*BehaviorTree);
    
    // ★ 重要: ランタイムノード側にはDecoratorを追加しない ★
    // UpdateAsset()がグラフからランタイムツリーを再構築するため
}

// グラフノードのDecorators配列にのみ追加
TargetGraphNode->Decorators.Add(DecoratorGraphNode);
DecoratorGraphNode->ParentNode = Cast<UAIGraphNode>(TargetGraphNode);

// ★ デバッグログ追加 ★
UE_LOG(LogTemp, Display, TEXT("Added decorator %s to graph node %s (Decorators count: %d)"),
    *NodeId, *TargetNodeId, TargetGraphNode->Decorators.Num());
```

### 追加調査

`UpdateAsset()`の挙動を確認するため、呼び出し前後でノード状態をログ出力:

```cpp
// FinalizeAndSaveBTGraph 内に追加
UE_LOG(LogTemp, Display, TEXT("=== Before UpdateAsset ==="));
for (UEdGraphNode* Node : BTGraph->Nodes)
{
    UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(Node);
    if (BTNode)
    {
        UE_LOG(LogTemp, Display, TEXT("Node: %s, Decorators: %d"),
            BTNode->NodeInstance ? *BTNode->NodeInstance->GetName() : TEXT("null"),
            BTNode->Decorators.Num());
    }
}

BTGraph->UpdateAsset();

UE_LOG(LogTemp, Display, TEXT("=== After UpdateAsset ==="));
// 同様のログ出力
```

---

## 問題2: ノード削除が不完全な問題の修正

### 原因分析

現在の`HandleDeleteBTNode`:

```cpp
// 全てのピン接続を解除
for (UEdGraphPin* Pin : GraphNode->Pins)
{
    Pin->BreakAllPinLinks();
}

// グラフからノードを削除
BTGraph->RemoveNode(GraphNode);
```

**問題点**:
1. `NodeInstance`（ランタイムノード）の明示的な削除がない
2. 子ノードの接続解除処理がない
3. `RemoveNode`後の参照クリアが不完全

### 修正内容

`SpirrowBridgeAICommands_BTNodeOperations.cpp` の `HandleDeleteBTNode`:

```cpp
TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleDeleteBTNode(
    const TSharedPtr<FJsonObject>& Params)
{
    // ... パラメータ取得（既存コード） ...

    // ★ グラフノード取得 ★
    UBehaviorTreeGraphNode* GraphNode = FindGraphNodeByIdInternal(BTGraph, NodeId);
    if (!GraphNode)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeNotFound,
            FString::Printf(TEXT("Graph node not found: %s"), *NodeId));
    }

    // ★ 修正1: ランタイムノードの参照を保持 ★
    UBTNode* RuntimeNode = Cast<UBTNode>(GraphNode->NodeInstance);

    // ★ 修正2: 子ノードの入力ピン接続も解除 ★
    for (UEdGraphPin* Pin : GraphNode->Pins)
    {
        if (Pin->Direction == EGPD_Output)
        {
            // 子ノードへの接続を全て解除
            for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
            {
                LinkedPin->BreakLinkTo(Pin);
            }
        }
        Pin->BreakAllPinLinks();
    }

    // ★ 修正3: デコレータ/サービスの削除処理を強化 ★
    UBehaviorTreeGraphNode_Decorator* DecoratorNode = Cast<UBehaviorTreeGraphNode_Decorator>(GraphNode);
    UBehaviorTreeGraphNode_Service* ServiceNode = Cast<UBehaviorTreeGraphNode_Service>(GraphNode);

    if (DecoratorNode)
    {
        // 親ノードのDecorators配列から削除
        if (DecoratorNode->ParentNode)
        {
            UBehaviorTreeGraphNode* ParentBTNode = Cast<UBehaviorTreeGraphNode>(DecoratorNode->ParentNode);
            if (ParentBTNode)
            {
                ParentBTNode->Decorators.Remove(DecoratorNode);
            }
        }
        // ParentNode参照をクリア
        DecoratorNode->ParentNode = nullptr;
    }
    else if (ServiceNode)
    {
        // 親ノードのServices配列から削除
        if (ServiceNode->ParentNode)
        {
            UBehaviorTreeGraphNode_Composite* ParentComposite = 
                Cast<UBehaviorTreeGraphNode_Composite>(ServiceNode->ParentNode);
            if (ParentComposite)
            {
                ParentComposite->Services.Remove(ServiceNode);
            }
        }
        ServiceNode->ParentNode = nullptr;
    }
    else
    {
        // ★ 修正4: Composite/Taskノードの場合、付属するDecorator/Serviceも削除 ★
        // Decorators削除
        for (UBehaviorTreeGraphNode* Decorator : GraphNode->Decorators)
        {
            if (Decorator)
            {
                BTGraph->RemoveNode(Decorator);
            }
        }
        GraphNode->Decorators.Empty();

        // Services削除（Compositeノードの場合）
        UBehaviorTreeGraphNode_Composite* CompositeNode = 
            Cast<UBehaviorTreeGraphNode_Composite>(GraphNode);
        if (CompositeNode)
        {
            for (UBehaviorTreeGraphNode_Service* Service : CompositeNode->Services)
            {
                if (Service)
                {
                    BTGraph->RemoveNode(Service);
                }
            }
            CompositeNode->Services.Empty();
        }
    }

    // ★ 修正5: NodeInstanceの参照をクリア ★
    GraphNode->NodeInstance = nullptr;

    // ★ グラフからノードを削除 ★
    BTGraph->RemoveNode(GraphNode);

    // ★ 修正6: ランタイムノードをマークして次のUpdateAssetで除外されるようにする ★
    if (RuntimeNode)
    {
        RuntimeNode->MarkAsGarbage();
    }

    // ★ グラフ更新と保存 ★
    FinalizeAndSaveBTGraphInternal(BTGraph, BehaviorTree);

    // レスポンス
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
    Result->SetStringField(TEXT("deleted_node_id"), NodeId);
    return Result;
}
```

---

## 問題3: ノード一覧取得API（list_bt_nodes）の新規実装

### API仕様

**Python側**:
```python
@mcp.tool()
async def list_bt_nodes(
    behavior_tree_name: str,
    path: str = "/Game/AI/BehaviorTrees"
) -> dict:
    """
    List all nodes in a BehaviorTree with their hierarchy.
    
    Returns:
        Dict containing:
        - success: Whether the operation succeeded
        - behavior_tree_name: Name of the BehaviorTree
        - nodes: List of node objects with id, type, class, name, parent, children, decorators, services
        - total_nodes: Total number of nodes
    """
```

**レスポンス例**:
```json
{
    "success": true,
    "behavior_tree_name": "BT_AIFighter",
    "root_node": {
        "id": "Root",
        "type": "Root",
        "position": [0, 0],
        "children": ["BTComposite_Selector_0"]
    },
    "nodes": [
        {
            "id": "BTComposite_Selector_0",
            "type": "Composite",
            "class": "BTComposite_Selector",
            "name": "MainSelector",
            "position": [0, 150],
            "parent": "Root",
            "children": ["BTComposite_Sequence_0", "BTTask_Wait_0"],
            "decorators": [],
            "services": ["BTService_DefaultFocus_0"]
        },
        {
            "id": "BTComposite_Sequence_0",
            "type": "Composite",
            "class": "BTComposite_Sequence",
            "name": "CombatSequence",
            "position": [-150, 300],
            "parent": "BTComposite_Selector_0",
            "children": ["BTTask_MoveTo_0"],
            "decorators": ["BTDecorator_Blackboard_0"],
            "services": []
        },
        {
            "id": "BTDecorator_Blackboard_0",
            "type": "Decorator",
            "class": "BTDecorator_Blackboard",
            "name": "CheckLineOfSight",
            "position": [-150, 240],
            "attached_to": "BTComposite_Sequence_0"
        },
        {
            "id": "BTTask_MoveTo_0",
            "type": "Task",
            "class": "BTTask_MoveTo",
            "name": "MoveToTarget",
            "position": [-150, 450],
            "parent": "BTComposite_Sequence_0"
        }
    ],
    "total_nodes": 4
}
```

### C++実装

`SpirrowBridgeAICommands_BTNodeOperations.cpp` に追加:

```cpp
// ===== Helper: Build node info JSON =====
static TSharedPtr<FJsonObject> BuildNodeInfoJson(
    UBehaviorTreeGraphNode* GraphNode,
    const FString& ParentId = TEXT(""))
{
    TSharedPtr<FJsonObject> NodeInfo = MakeShareable(new FJsonObject());
    
    if (!GraphNode || !GraphNode->NodeInstance)
    {
        return NodeInfo;
    }
    
    UBTNode* RuntimeNode = Cast<UBTNode>(GraphNode->NodeInstance);
    
    // 基本情報
    NodeInfo->SetStringField(TEXT("id"), RuntimeNode->GetName());
    NodeInfo->SetStringField(TEXT("class"), RuntimeNode->GetClass()->GetName());
    
    // ノード名（カスタム名がある場合）
    if (!RuntimeNode->NodeName.IsEmpty())
    {
        NodeInfo->SetStringField(TEXT("name"), RuntimeNode->NodeName);
    }
    
    // 位置
    TArray<TSharedPtr<FJsonValue>> PosArray;
    PosArray.Add(MakeShareable(new FJsonValueNumber(GraphNode->NodePosX)));
    PosArray.Add(MakeShareable(new FJsonValueNumber(GraphNode->NodePosY)));
    NodeInfo->SetArrayField(TEXT("position"), PosArray);
    
    // タイプ判定
    if (Cast<UBehaviorTreeGraphNode_Composite>(GraphNode))
    {
        NodeInfo->SetStringField(TEXT("type"), TEXT("Composite"));
    }
    else if (Cast<UBehaviorTreeGraphNode_Task>(GraphNode))
    {
        NodeInfo->SetStringField(TEXT("type"), TEXT("Task"));
    }
    else if (Cast<UBehaviorTreeGraphNode_Decorator>(GraphNode))
    {
        NodeInfo->SetStringField(TEXT("type"), TEXT("Decorator"));
        
        // attached_to
        UBehaviorTreeGraphNode_Decorator* DecNode = 
            Cast<UBehaviorTreeGraphNode_Decorator>(GraphNode);
        if (DecNode->ParentNode)
        {
            UBehaviorTreeGraphNode* ParentBTNode = 
                Cast<UBehaviorTreeGraphNode>(DecNode->ParentNode);
            if (ParentBTNode && ParentBTNode->NodeInstance)
            {
                NodeInfo->SetStringField(TEXT("attached_to"), 
                    ParentBTNode->NodeInstance->GetName());
            }
        }
        return NodeInfo; // Decoratorは子を持たない
    }
    else if (Cast<UBehaviorTreeGraphNode_Service>(GraphNode))
    {
        NodeInfo->SetStringField(TEXT("type"), TEXT("Service"));
        
        // attached_to
        UBehaviorTreeGraphNode_Service* SvcNode = 
            Cast<UBehaviorTreeGraphNode_Service>(GraphNode);
        if (SvcNode->ParentNode)
        {
            UBehaviorTreeGraphNode* ParentBTNode = 
                Cast<UBehaviorTreeGraphNode>(SvcNode->ParentNode);
            if (ParentBTNode && ParentBTNode->NodeInstance)
            {
                NodeInfo->SetStringField(TEXT("attached_to"), 
                    ParentBTNode->NodeInstance->GetName());
            }
        }
        return NodeInfo; // Serviceは子を持たない
    }
    
    // 親ノードID
    if (!ParentId.IsEmpty())
    {
        NodeInfo->SetStringField(TEXT("parent"), ParentId);
    }
    
    // 子ノードID一覧
    TArray<TSharedPtr<FJsonValue>> ChildrenArray;
    for (UEdGraphPin* Pin : GraphNode->Pins)
    {
        if (Pin->Direction == EGPD_Output)
        {
            for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
            {
                UBehaviorTreeGraphNode* ChildNode = 
                    Cast<UBehaviorTreeGraphNode>(LinkedPin->GetOwningNode());
                if (ChildNode && ChildNode->NodeInstance)
                {
                    ChildrenArray.Add(MakeShareable(
                        new FJsonValueString(ChildNode->NodeInstance->GetName())));
                }
            }
        }
    }
    NodeInfo->SetArrayField(TEXT("children"), ChildrenArray);
    
    // Decorator一覧
    TArray<TSharedPtr<FJsonValue>> DecoratorsArray;
    for (UBehaviorTreeGraphNode* Decorator : GraphNode->Decorators)
    {
        if (Decorator && Decorator->NodeInstance)
        {
            DecoratorsArray.Add(MakeShareable(
                new FJsonValueString(Decorator->NodeInstance->GetName())));
        }
    }
    NodeInfo->SetArrayField(TEXT("decorators"), DecoratorsArray);
    
    // Service一覧（Compositeノードのみ）
    TArray<TSharedPtr<FJsonValue>> ServicesArray;
    UBehaviorTreeGraphNode_Composite* CompositeNode = 
        Cast<UBehaviorTreeGraphNode_Composite>(GraphNode);
    if (CompositeNode)
    {
        for (UBehaviorTreeGraphNode_Service* Service : CompositeNode->Services)
        {
            if (Service && Service->NodeInstance)
            {
                ServicesArray.Add(MakeShareable(
                    new FJsonValueString(Service->NodeInstance->GetName())));
            }
        }
    }
    NodeInfo->SetArrayField(TEXT("services"), ServicesArray);
    
    return NodeInfo;
}

// ===== list_bt_nodes Handler =====
TSharedPtr<FJsonObject> FSpirrowBridgeAICommands::HandleListBTNodes(
    const TSharedPtr<FJsonObject>& Params)
{
    // パラメータ取得
    FString BehaviorTreeName;
    TSharedPtr<FJsonObject> NameError = FSpirrowBridgeCommonUtils::ValidateRequiredString(
        Params, TEXT("behavior_tree_name"), BehaviorTreeName);
    if (NameError) return NameError;

    FString Path;
    FSpirrowBridgeCommonUtils::GetOptionalString(
        Params, TEXT("path"), Path, TEXT("/Game/AI/BehaviorTrees"));

    // BehaviorTree取得
    UBehaviorTree* BehaviorTree = FindBehaviorTreeAsset(BehaviorTreeName, Path);
    if (!BehaviorTree)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::AssetNotFound,
            FString::Printf(TEXT("BehaviorTree not found: %s at %s"), *BehaviorTreeName, *Path));
    }

    // Graph取得
    UBehaviorTreeGraph* BTGraph = GetBTGraph(BehaviorTree);
    if (!BTGraph)
    {
        return FSpirrowBridgeCommonUtils::CreateErrorResponse(
            ESpirrowErrorCode::NodeCreationFailed,
            TEXT("BehaviorTree has no graph"));
    }

    // Rootノード情報
    TSharedPtr<FJsonObject> RootInfo = MakeShareable(new FJsonObject());
    UBehaviorTreeGraphNode_Root* RootNode = FindRootGraphNodeInternal(BTGraph);
    if (RootNode)
    {
        RootInfo->SetStringField(TEXT("id"), TEXT("Root"));
        RootInfo->SetStringField(TEXT("type"), TEXT("Root"));
        
        TArray<TSharedPtr<FJsonValue>> RootPosArray;
        RootPosArray.Add(MakeShareable(new FJsonValueNumber(RootNode->NodePosX)));
        RootPosArray.Add(MakeShareable(new FJsonValueNumber(RootNode->NodePosY)));
        RootInfo->SetArrayField(TEXT("position"), RootPosArray);
        
        // Rootの子ノード
        TArray<TSharedPtr<FJsonValue>> RootChildrenArray;
        for (UEdGraphPin* Pin : RootNode->Pins)
        {
            if (Pin->Direction == EGPD_Output)
            {
                for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                {
                    UBehaviorTreeGraphNode* ChildNode = 
                        Cast<UBehaviorTreeGraphNode>(LinkedPin->GetOwningNode());
                    if (ChildNode && ChildNode->NodeInstance)
                    {
                        RootChildrenArray.Add(MakeShareable(
                            new FJsonValueString(ChildNode->NodeInstance->GetName())));
                    }
                }
            }
        }
        RootInfo->SetArrayField(TEXT("children"), RootChildrenArray);
    }

    // 全ノード情報を収集
    TArray<TSharedPtr<FJsonValue>> NodesArray;
    TMap<UBehaviorTreeGraphNode*, FString> NodeParentMap;
    
    // まず親子関係をマッピング
    for (UEdGraphNode* EdNode : BTGraph->Nodes)
    {
        UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(EdNode);
        if (!BTNode) continue;
        
        // Rootノードの子は親が"Root"
        if (RootNode)
        {
            for (UEdGraphPin* Pin : RootNode->Pins)
            {
                if (Pin->Direction == EGPD_Output)
                {
                    for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                    {
                        if (LinkedPin->GetOwningNode() == BTNode)
                        {
                            NodeParentMap.Add(BTNode, TEXT("Root"));
                        }
                    }
                }
            }
        }
        
        // 通常の親子関係
        for (UEdGraphPin* Pin : BTNode->Pins)
        {
            if (Pin->Direction == EGPD_Output)
            {
                for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                {
                    UBehaviorTreeGraphNode* ChildNode = 
                        Cast<UBehaviorTreeGraphNode>(LinkedPin->GetOwningNode());
                    if (ChildNode && BTNode->NodeInstance)
                    {
                        NodeParentMap.Add(ChildNode, BTNode->NodeInstance->GetName());
                    }
                }
            }
        }
    }
    
    // ノード情報をJSON化
    int32 TotalNodes = 0;
    for (UEdGraphNode* EdNode : BTGraph->Nodes)
    {
        UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(EdNode);
        if (!BTNode || !BTNode->NodeInstance) continue;
        
        // Rootノードはスキップ（別途出力済み）
        if (Cast<UBehaviorTreeGraphNode_Root>(BTNode)) continue;
        
        FString ParentId = NodeParentMap.FindRef(BTNode);
        TSharedPtr<FJsonObject> NodeInfo = BuildNodeInfoJson(BTNode, ParentId);
        NodesArray.Add(MakeShareable(new FJsonValueObject(NodeInfo)));
        TotalNodes++;
        
        // Decoratorも追加
        for (UBehaviorTreeGraphNode* Decorator : BTNode->Decorators)
        {
            if (Decorator && Decorator->NodeInstance)
            {
                TSharedPtr<FJsonObject> DecInfo = BuildNodeInfoJson(Decorator);
                NodesArray.Add(MakeShareable(new FJsonValueObject(DecInfo)));
                TotalNodes++;
            }
        }
        
        // Serviceも追加（Compositeノードの場合）
        UBehaviorTreeGraphNode_Composite* CompositeNode = 
            Cast<UBehaviorTreeGraphNode_Composite>(BTNode);
        if (CompositeNode)
        {
            for (UBehaviorTreeGraphNode_Service* Service : CompositeNode->Services)
            {
                if (Service && Service->NodeInstance)
                {
                    TSharedPtr<FJsonObject> SvcInfo = BuildNodeInfoJson(Service);
                    NodesArray.Add(MakeShareable(new FJsonValueObject(SvcInfo)));
                    TotalNodes++;
                }
            }
        }
    }

    // レスポンス
    TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject());
    Result->SetBoolField(TEXT("success"), true);
    Result->SetStringField(TEXT("behavior_tree_name"), BehaviorTreeName);
    Result->SetObjectField(TEXT("root_node"), RootInfo);
    Result->SetArrayField(TEXT("nodes"), NodesArray);
    Result->SetNumberField(TEXT("total_nodes"), TotalNodes);
    return Result;
}
```

### ヘッダファイル追加

`SpirrowBridgeAICommands.h` に追加:

```cpp
// list_bt_nodes
static TSharedPtr<FJsonObject> HandleListBTNodes(const TSharedPtr<FJsonObject>& Params);
```

### ルーティング追加

`SpirrowBridgeAICommands.cpp` の `HandleCommand` に追加:

```cpp
else if (Command == TEXT("list_bt_nodes"))
{
    return HandleListBTNodes(Params);
}
```

`SpirrowBridge.cpp` の `ExecuteCommand` に追加:

```cpp
// 既存のAIコマンドリストに追加
else if (Command == TEXT("list_bt_nodes"))
{
    return FSpirrowBridgeAICommands::HandleCommand(Command, Params);
}
```

### Python側追加

`Python/tools/ai_tools.py` に追加:

```python
@mcp.tool()
async def list_bt_nodes(
    behavior_tree_name: str,
    path: str = "/Game/AI/BehaviorTrees"
) -> dict:
    """
    List all nodes in a BehaviorTree with their hierarchy and relationships.

    Args:
        behavior_tree_name: Name of the BehaviorTree
        path: Content browser path where the BehaviorTree is located

    Returns:
        Dict containing:
        - success: Whether the operation succeeded
        - behavior_tree_name: Name of the BehaviorTree
        - root_node: Root node info with children
        - nodes: List of all nodes with id, type, class, name, position, parent, children, decorators, services
        - total_nodes: Total number of nodes (excluding Root)

    Example:
        list_bt_nodes(
            behavior_tree_name="BT_Enemy",
            path="/Game/AI/BehaviorTrees"
        )
    """
    result = await send_command("list_bt_nodes", {
        "behavior_tree_name": behavior_tree_name,
        "path": path
    })
    return result
```

---

## 実装タスク

### 1. C++側修正

| # | ファイル | 作業内容 |
|---|----------|----------|
| 1-1 | `SpirrowBridgeAICommands_BTNodeCreation.cpp` | `HandleAddBTDecoratorNode`にデバッグログ追加 |
| 1-2 | `SpirrowBridgeAICommands_BTNodeOperations.cpp` | `HandleDeleteBTNode`の削除処理強化 |
| 1-3 | `SpirrowBridgeAICommands_BTNodeOperations.cpp` | `HandleListBTNodes`新規実装 |
| 1-4 | `SpirrowBridgeAICommands.h` | `HandleListBTNodes`宣言追加 |
| 1-5 | `SpirrowBridgeAICommands.cpp` | ルーティング追加 |
| 1-6 | `SpirrowBridge.cpp` | ExecuteCommandルーティング追加 |

### 2. Python側追加

| # | ファイル | 作業内容 |
|---|----------|----------|
| 2-1 | `Python/tools/ai_tools.py` | `list_bt_nodes`ツール追加 |

### 3. ドキュメント更新

| # | ファイル | 作業内容 |
|---|----------|----------|
| 3-1 | `FEATURE_STATUS.md` | list_bt_nodes追加、バージョン更新 |
| 3-2 | `Docs/IMPLEMENTATION_SUMMARY.md` | 必要に応じて更新 |

---

## 完了条件

- [ ] `add_bt_decorator_node`でターゲットノードにのみDecoratorが付与される
- [ ] `delete_bt_node`で完全にノードが削除される（残骸が残らない）
- [ ] `list_bt_nodes`で全ノードの一覧と階層構造が取得できる
- [ ] UEエディタでBT構造が正しく表示される
- [ ] 既存のBTツール（add_bt_composite_node等）が引き続き動作する
- [ ] ビルドエラーなし

---

## テスト手順

### テスト1: Decorator付与テスト

```python
# 1. 新規BT作成
create_behavior_tree(name="BT_Test", path="/Game/AI/Test")

# 2. Selector追加
add_bt_composite_node(
    behavior_tree_name="BT_Test",
    node_type="Selector",
    parent_node_id="Root",
    path="/Game/AI/Test"
)

# 3. Sequence追加
add_bt_composite_node(
    behavior_tree_name="BT_Test",
    node_type="Sequence",
    parent_node_id="BTComposite_Selector_0",
    path="/Game/AI/Test"
)

# 4. SequenceにDecorator追加
add_bt_decorator_node(
    behavior_tree_name="BT_Test",
    decorator_type="BTDecorator_Blackboard",
    target_node_id="BTComposite_Sequence_0",
    path="/Game/AI/Test"
)

# 5. ノード一覧確認
list_bt_nodes(behavior_tree_name="BT_Test", path="/Game/AI/Test")

# 期待結果: DecoratorはBTComposite_Sequence_0にのみ付与
```

### テスト2: ノード削除テスト

```python
# 1. ノード削除
delete_bt_node(
    behavior_tree_name="BT_Test",
    node_id="BTDecorator_Blackboard_0",
    path="/Game/AI/Test"
)

# 2. ノード一覧確認
list_bt_nodes(behavior_tree_name="BT_Test", path="/Game/AI/Test")

# 期待結果: BTDecorator_Blackboard_0が一覧に含まれない
```

### テスト3: UEエディタで目視確認

1. UEエディタでBT_Testを開く
2. DecoratorがSequenceノードにのみ付与されていることを確認
3. 削除後、残骸ノードがないことを確認

---

## 注意事項

1. **UpdateAsset()の挙動**: この関数はグラフからランタイムツリーを再構築するため、グラフノードの状態が正であることが前提
2. **RF_Transactionalフラグ**: ノード作成時に必ず設定する（Undo/Redo対応）
3. **MarkAsGarbage()**: 削除したランタイムノードをGC対象にマーク
4. **デバッグログ**: 問題が解決したらログレベルをVerboseに下げるか削除

---

## 更新履歴

| 日付 | 内容 |
|------|------|
| 2026-01-09 | 初版作成 |
