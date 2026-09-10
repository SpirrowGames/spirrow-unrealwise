---
id: spirrow-unrealwise:bt-fgraphnodecreator-raii
title: BehaviorTree FGraphNodeCreator RAII パターン詳解
product: spirrow-unrealwise
type: note
status: active
version: 1.0
created: 2026-03-12
last_verified: 2026-09-10
supersedes: []
related: [spirrow-unrealwise:bt-two-layer-architecture]
keywords: [BehaviorTree, FGraphNodeCreator, RAII, Finalize, NodeInstance, UE5]
legacy_drive_id: [1_AWKq6VmmYlNEHKP6u1_Y3hcK1-hmBS6CI1qzDFf9AM]
---

# BehaviorTree FGraphNodeCreator RAII パターン詳解

## 概要
UE5でEdGraphノードを安全に作成するためのRAIIヘルパー `FGraphNodeCreator` の使い方。

## FGraphNodeCreator テンプレート
```cpp
template<typename NodeClass>
struct FGraphNodeCreator
{
    FGraphNodeCreator(UEdGraph& Graph);
    NodeClass* CreateNode(bool bSelectNewNode = true);
    ~FGraphNodeCreator(); // → Finalize() を呼ぶ
};
```

## Finalize() の処理内容
デストラクタから呼ばれ、以下を順に実行:
1. `CreateNewGuid()` — ノードに一意IDを付与
2. `PostPlacedNewNode()` — ノード配置後の初期化（BT固有の初期化を含む）
3. `AllocateDefaultPins()` — 入出力ピンの生成
4. グラフへのノード登録
5. `NotifyGraphChanged()` — グラフ変更通知

## 使用パターン
```cpp
// スコープで囲むことでFinalize()の呼び忘れを防ぐ
{
    FGraphNodeCreator<UBehaviorTreeGraphNode_Composite> Creator(*BTGraph);
    UBehaviorTreeGraphNode_Composite* GraphNode = Creator.CreateNode(false);

    // NodeInstanceはFinalize()前に設定必須
    GraphNode->NodeInstance = NewObject<UBTCompositeNode_Sequence>(BTAsset);
    GraphNode->NodePosX = 100;
    GraphNode->NodePosY = 200;

    // スコープ終了 → デストラクタ → Finalize()
}
```

## 注意点
- `NodeInstance` を設定せずにスコープを抜けると、空のノードが作られorphan判定される
- `CreateNode(false)` でbSelectNewNode=falseにすると、エディタ選択状態を変更しない
- MCP実装では `FGraphNodeCreator` を直接使えないため、Finalize()相当の処理を手動で行う必要がある

## 移行時の注記（2026-09-10）

Drive 原本（`1_AWKq6VmmYlNEHKP6u1_Y3hcK1-hmBS6CI1qzDFf9AM`）の逐語移行。
2026-03-12 の TrapxTrapCpp `InvestigationReports/` 10 件の 1 つ
（誤配置の経緯は [[spirrow-unrealwise:bt-add-subnode-ten-step-pattern]] の注記と同じ）。

**本書が説明している不変条件は、その後 primitive レイヤーとして制度化された。**
`Docs/Architecture/PrimitiveLayerMigration.md` が

> The "`FGraphNodeCreator` → `Finalize` → runtime `NodeInstance` with `GraphNode` as Outer"
> invariant was enforced only by prompt-level docs

を "BT 2-layer creation bug" として挙げ、`SpirrowBridgePrimitives::SafeCreateBTGraphAndRuntimeNode`
に封じ込めたうえで、`FGraphNodeCreator<>` の直呼びを CI lint で禁止したと
`Docs/CHANGELOG.md` が記録している。

∴ **本書「注意点」の最終行は現行では逆になっている** —
「MCP 実装では直接使えないので手動で Finalize 相当を行う」ではなく、
**手動でやらないことが規約**。ただし Decorator / Service はその primitive の対象外なので、
手書き経路は残っている。

本書は、その primitive が何を封じているのかを読むための原典にあたる。
`Finalize()` が内部で 5 つ何をしているかを知らずに primitive を迂回すると、
CI lint が守っている不変条件を素通りできてしまう。
