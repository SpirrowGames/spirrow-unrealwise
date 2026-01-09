# BehaviorTree グラフベースノード作成 - 修正指示書 v2
# UE5.7対応 - RAGサーバ知見統合版

## 概要

BehaviorTree MCPツールの実装において、ノードがエディタで表示されない・保存後に消失する問題を解決するための修正指示書。

### 問題の症状
- MCP経由で作成したBTノードが保存・再読み込み後に消失
- "Class {ClassName} not found, make sure it's saved!" エラー
- エディタグラフにノードが表示されない

### 根本原因
UEのBTエディタは**2層構造**を採用:
1. **グラフノード層** (UBehaviorTreeGraphNode派生) - エディタ表示・シリアライズ担当
2. **ランタイムノード層** (UBTNode派生) - 実行ロジック担当

現行実装はランタイムノードのみを直接操作しているため、グラフ層が欠落しシリアライズに失敗している。

---

## アーキテクチャ概要

### クラス階層 (UE5.7)

```
UEdGraphNode
  └─ UAIGraphNode (AIGraph module)
       └─ UBehaviorTreeGraphNode (BehaviorTreeEditor module)
            ├─ UBehaviorTreeGraphNode_Root
            ├─ UBehaviorTreeGraphNode_Composite
            │     └─ UBehaviorTreeGraphNode_SimpleParallel
            ├─ UBehaviorTreeGraphNode_Task
            │     └─ UBehaviorTreeGraphNode_SubtreeTask
            ├─ UBehaviorTreeGraphNode_Decorator
            │     └─ UBehaviorTreeGraphNode_CompositeDecorator
            └─ UBehaviorTreeGraphNode_Service
```

### 重要プロパティ

**UAIGraphNode:**
- `FGraphNodeClassData ClassData` - ノードクラスのメタデータ
- `UObject* NodeInstance` - ランタイムノードインスタンス (UBTNode派生)
- `UAIGraphNode* ParentNode` - 親ノード参照
- `TArray<UAIGraphNode*> SubNodes` - サブノード配列

**UBehaviorTreeGraphNode:**
- `TArray<UBehaviorTreeGraphNode*> Decorators` - デコレータノード配列
- `TArray<UBehaviorTreeGraphNode*> Services` - サービスノード配列
- `uint32 bInjectedNode : 1` - サブツリーから注入されたノード
- `uint32 bRootLevel : 1` - ルートレベルノードフラグ

---

## 必要なモジュール依存関係

### Build.cs追加項目

```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core",
    "CoreUObject",
    "Engine",
    "AIModule",
    "GameplayTasks",
    // ★追加必須★
    "AIGraph",           // UAIGraphNode, FGraphNodeClassData
    "BehaviorTreeEditor" // UBehaviorTreeGraph, UBehaviorTreeGraphNode_*
});
```

### 必要なヘッダファイル

```cpp
// グラフ基盤
#include "EdGraph/EdGraph.h"

// AIグラフ
#include "AIGraphTypes.h"  // FGraphNodeClassData

// BehaviorTreeエディタ
#include "BehaviorTreeGraph.h"
#include "BehaviorTreeGraphNode.h"
#include "BehaviorTreeGraphNode_Root.h"
#include "BehaviorTreeGraphNode_Composite.h"
#include "BehaviorTreeGraphNode_SimpleParallel.h"
#include "BehaviorTreeGraphNode_Task.h"
#include "BehaviorTreeGraphNode_SubtreeTask.h"
#include "BehaviorTreeGraphNode_Decorator.h"
#include "BehaviorTreeGraphNode_Service.h"
#include "EdGraphSchema_BehaviorTree.h"
```

---

## 正しい実装パターン

### 1. グラフの取得/作成

```cpp
UBehaviorTreeGraph* GetOrCreateBTGraph(UBehaviorTree* BehaviorTree)
{
    UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BehaviorTree->BTGraph);
    
    if (!BTGraph)
    {
        // グラフを新規作成
        BTGraph = NewObject<UBehaviorTreeGraph>(
            BehaviorTree, 
            TEXT("BTGraph"), 
            RF_Transactional
        );
        BehaviorTree->BTGraph = BTGraph;
        
        // スキーマ設定
        BTGraph->Schema = UEdGraphSchema_BehaviorTree::StaticClass();
        
        // デフォルトノード（Root）を作成
        const UEdGraphSchema* Schema = BTGraph->GetSchema();
        Schema->CreateDefaultNodesForGraph(*BTGraph);
    }
    
    return BTGraph;
}
```

### 2. Rootノードの取得

```cpp
UBehaviorTreeGraphNode_Root* FindRootNode(UBehaviorTreeGraph* BTGraph)
{
    for (UEdGraphNode* Node : BTGraph->Nodes)
    {
        if (UBehaviorTreeGraphNode_Root* RootNode = Cast<UBehaviorTreeGraphNode_Root>(Node))
        {
            return RootNode;
        }
    }
    return nullptr;
}
```

### 3. コンポジットノードの作成

```cpp
UBehaviorTreeGraphNode_Composite* CreateCompositeNode(
    UBehaviorTreeGraph* BTGraph,
    UClass* CompositeClass,  // UBTComposite_Selector::StaticClass() など
    const FString& NodeName = TEXT(""))
{
    // ★重要: FGraphNodeCreator を使用（RAIIパターン）★
    FGraphNodeCreator<UBehaviorTreeGraphNode_Composite> NodeCreator(*BTGraph);
    UBehaviorTreeGraphNode_Composite* GraphNode = NodeCreator.CreateNode();
    
    // ランタイムノード作成（★Outer は GraphNode にする★）
    UBTCompositeNode* RuntimeNode = NewObject<UBTCompositeNode>(
        GraphNode,           // ★重要: GraphNode を Outer に★
        CompositeClass, 
        NAME_None, 
        RF_Transactional     // ★重要: RF_Transactional フラグ★
    );
    
    // グラフノードにランタイムノードを関連付け
    GraphNode->NodeInstance = RuntimeNode;
    GraphNode->ClassData = FGraphNodeClassData(CompositeClass, TEXT(""));
    
    // ノード名設定
    if (!NodeName.IsEmpty())
    {
        RuntimeNode->NodeName = NodeName;
    }
    
    // ★重要: Finalize() を必ず呼ぶ（AllocateDefaultPins() が実行される）★
    NodeCreator.Finalize();
    
    return GraphNode;
}
```

### 4. タスクノードの作成

```cpp
UBehaviorTreeGraphNode_Task* CreateTaskNode(
    UBehaviorTreeGraph* BTGraph,
    UClass* TaskClass,  // UBTTask_Wait::StaticClass() など
    const FString& NodeName = TEXT(""))
{
    // SimpleParallel用サブタスクか通常タスクかを判定
    bool bIsSubtreeTask = TaskClass->IsChildOf(UBTTask_RunBehavior::StaticClass());
    
    UBehaviorTreeGraphNode_Task* GraphNode = nullptr;
    
    if (bIsSubtreeTask)
    {
        FGraphNodeCreator<UBehaviorTreeGraphNode_SubtreeTask> NodeCreator(*BTGraph);
        GraphNode = NodeCreator.CreateNode();
        
        UBTTaskNode* RuntimeNode = NewObject<UBTTaskNode>(
            GraphNode, TaskClass, NAME_None, RF_Transactional);
        
        GraphNode->NodeInstance = RuntimeNode;
        GraphNode->ClassData = FGraphNodeClassData(TaskClass, TEXT(""));
        
        NodeCreator.Finalize();
    }
    else
    {
        FGraphNodeCreator<UBehaviorTreeGraphNode_Task> NodeCreator(*BTGraph);
        GraphNode = NodeCreator.CreateNode();
        
        UBTTaskNode* RuntimeNode = NewObject<UBTTaskNode>(
            GraphNode, TaskClass, NAME_None, RF_Transactional);
        
        GraphNode->NodeInstance = RuntimeNode;
        GraphNode->ClassData = FGraphNodeClassData(TaskClass, TEXT(""));
        
        if (!NodeName.IsEmpty())
        {
            RuntimeNode->NodeName = NodeName;
        }
        
        NodeCreator.Finalize();
    }
    
    return GraphNode;
}
```

### 5. デコレータノードの作成

```cpp
UBehaviorTreeGraphNode_Decorator* CreateDecoratorNode(
    UBehaviorTreeGraph* BTGraph,
    UBehaviorTreeGraphNode* TargetNode,  // デコレータを追加する対象ノード
    UClass* DecoratorClass)
{
    FGraphNodeCreator<UBehaviorTreeGraphNode_Decorator> NodeCreator(*BTGraph);
    UBehaviorTreeGraphNode_Decorator* GraphNode = NodeCreator.CreateNode();
    
    UBTDecorator* RuntimeNode = NewObject<UBTDecorator>(
        GraphNode, DecoratorClass, NAME_None, RF_Transactional);
    
    GraphNode->NodeInstance = RuntimeNode;
    GraphNode->ClassData = FGraphNodeClassData(DecoratorClass, TEXT(""));
    
    NodeCreator.Finalize();
    
    // ターゲットノードのDecorators配列に追加
    TargetNode->Decorators.Add(GraphNode);
    GraphNode->ParentNode = Cast<UAIGraphNode>(TargetNode);
    
    return GraphNode;
}
```

### 6. サービスノードの作成

```cpp
UBehaviorTreeGraphNode_Service* CreateServiceNode(
    UBehaviorTreeGraph* BTGraph,
    UBehaviorTreeGraphNode* TargetNode,  // サービスを追加する対象ノード（Compositeのみ）
    UClass* ServiceClass)
{
    FGraphNodeCreator<UBehaviorTreeGraphNode_Service> NodeCreator(*BTGraph);
    UBehaviorTreeGraphNode_Service* GraphNode = NodeCreator.CreateNode();
    
    UBTService* RuntimeNode = NewObject<UBTService>(
        GraphNode, ServiceClass, NAME_None, RF_Transactional);
    
    GraphNode->NodeInstance = RuntimeNode;
    GraphNode->ClassData = FGraphNodeClassData(ServiceClass, TEXT(""));
    
    NodeCreator.Finalize();
    
    // ターゲットノードのServices配列に追加
    TargetNode->Services.Add(GraphNode);
    GraphNode->ParentNode = Cast<UAIGraphNode>(TargetNode);
    
    return GraphNode;
}
```

### 7. ノード接続（ピン経由）

```cpp
bool ConnectNodes(
    UBehaviorTreeGraphNode* ParentGraphNode,
    UBehaviorTreeGraphNode* ChildGraphNode,
    int32 ChildIndex = -1)
{
    // 出力ピン取得
    UEdGraphPin* ParentOutputPin = nullptr;
    for (UEdGraphPin* Pin : ParentGraphNode->Pins)
    {
        if (Pin->Direction == EGPD_Output)
        {
            ParentOutputPin = Pin;
            break;
        }
    }
    
    // 入力ピン取得
    UEdGraphPin* ChildInputPin = nullptr;
    for (UEdGraphPin* Pin : ChildGraphNode->Pins)
    {
        if (Pin->Direction == EGPD_Input)
        {
            ChildInputPin = Pin;
            break;
        }
    }
    
    if (!ParentOutputPin || !ChildInputPin)
    {
        return false;
    }
    
    // ★ピン接続は MakeLinkTo() を使用★
    ParentOutputPin->MakeLinkTo(ChildInputPin);
    
    return true;
}
```

### 8. グラフ更新と保存

```cpp
void FinalizeAndSaveGraph(UBehaviorTreeGraph* BTGraph, UBehaviorTree* BehaviorTree)
{
    // グラフ変更を通知
    BTGraph->NotifyGraphChanged();
    
    // ランタイムツリーを再構築
    BTGraph->UpdateAsset();
    
    // パッケージを汚す（保存可能に）
    BehaviorTree->MarkPackageDirty();
}
```

---

## ノードタイプ対応表

### コンポジットノード

| 型 | ランタイムクラス | グラフノードクラス |
|---|---|---|
| Selector | `UBTComposite_Selector` | `UBehaviorTreeGraphNode_Composite` |
| Sequence | `UBTComposite_Sequence` | `UBehaviorTreeGraphNode_Composite` |
| SimpleParallel | `UBTComposite_SimpleParallel` | `UBehaviorTreeGraphNode_SimpleParallel` |

### タスクノード

| 型 | ランタイムクラス | グラフノードクラス |
|---|---|---|
| MoveTo | `UBTTask_MoveTo` | `UBehaviorTreeGraphNode_Task` |
| Wait | `UBTTask_Wait` | `UBehaviorTreeGraphNode_Task` |
| RunBehavior | `UBTTask_RunBehavior` | `UBehaviorTreeGraphNode_SubtreeTask` |
| カスタム | `UBTTaskNode`派生 | `UBehaviorTreeGraphNode_Task` |

### デコレータノード

| 型 | ランタイムクラス | グラフノードクラス |
|---|---|---|
| Blackboard | `UBTDecorator_Blackboard` | `UBehaviorTreeGraphNode_Decorator` |
| Cooldown | `UBTDecorator_Cooldown` | `UBehaviorTreeGraphNode_Decorator` |
| Loop | `UBTDecorator_Loop` | `UBehaviorTreeGraphNode_Decorator` |
| カスタム | `UBTDecorator`派生 | `UBehaviorTreeGraphNode_Decorator` |

### サービスノード

| 型 | ランタイムクラス | グラフノードクラス |
|---|---|---|
| DefaultFocus | `UBTService_DefaultFocus` | `UBehaviorTreeGraphNode_Service` |
| RunEQS | `UBTService_RunEQS` | `UBehaviorTreeGraphNode_Service` |
| カスタム | `UBTService`派生 | `UBehaviorTreeGraphNode_Service` |

---

## Blueprintクラスへの対応

カスタムBlueprintタスク/デコレータ/サービスを使用する場合:

```cpp
// Blueprintクラスパスから FGraphNodeClassData を作成
FString BlueprintPath = TEXT("/Game/AI/Tasks/BTTask_Custom");
FGraphNodeClassData ClassData(
    FTopLevelAssetPath(BlueprintPath), 
    TEXT("BTTask_Custom_C")  // _C サフィックス必須
);

GraphNode->ClassData = ClassData;

// クラスをロードしてランタイムノードを作成
UClass* BPClass = LoadClass<UBTTaskNode>(
    nullptr, 
    *FString::Printf(TEXT("%s.%s_C"), *BlueprintPath, *FPaths::GetBaseFilename(BlueprintPath))
);

if (BPClass)
{
    UBTTaskNode* RuntimeNode = NewObject<UBTTaskNode>(GraphNode, BPClass, NAME_None, RF_Transactional);
    GraphNode->NodeInstance = RuntimeNode;
}
```

---

## チェックリスト

### 実装時の確認事項

- [ ] `FGraphNodeCreator` を使用してグラフノードを作成している
- [ ] ランタイムノードの `Outer` は `GraphNode` を指定している
- [ ] `RF_Transactional` フラグを付けている
- [ ] `NodeCreator.Finalize()` を呼んでいる
- [ ] `ClassData` を正しく設定している
- [ ] ピン接続に `MakeLinkTo()` を使用している
- [ ] `BTGraph->NotifyGraphChanged()` または `UpdateAsset()` を呼んでいる

### デバッグ時の確認事項

- [ ] `BehaviorTree->BTGraph` が `UBehaviorTreeGraph` にキャストできる
- [ ] グラフにRootノードが存在する
- [ ] 各グラフノードの `NodeInstance` が nullptr でない
- [ ] 各グラフノードの `ClassData` が設定されている
- [ ] ピンが正しく接続されている

---

## 参考: UE5.7 ソースコード位置

| ファイル | パス |
|---|---|
| BehaviorTreeGraph.h | `Engine/Source/Editor/BehaviorTreeEditor/Classes/` |
| BehaviorTreeGraphNode.h | `Engine/Source/Editor/BehaviorTreeEditor/Classes/` |
| EdGraphSchema_BehaviorTree.h | `Engine/Source/Editor/BehaviorTreeEditor/Classes/` |
| AIGraphTypes.h | `Engine/Source/Editor/AIGraph/Classes/` |
| EdGraph.h (FGraphNodeCreator) | `Engine/Source/Runtime/Engine/Classes/EdGraph/` |

---

## 更新履歴

- v2.0 (2026-01-09): RAGサーバ知見統合、UE5.7対応、詳細実装パターン追加
- v1.0: 初版
