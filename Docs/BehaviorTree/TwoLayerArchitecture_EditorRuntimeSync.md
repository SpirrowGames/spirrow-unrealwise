---
id: spirrow-unrealwise:bt-two-layer-architecture
title: BehaviorTree 2層アーキテクチャとEditor-Runtime同期
product: spirrow-unrealwise
type: note
status: active
version: 1.0
created: 2026-03-12
last_verified: 2026-09-10
supersedes: []
related: [spirrow-unrealwise:bt-fgraphnodecreator-raii, spirrow-unrealwise:bt-node-disappearance-mechanism]
keywords: [BehaviorTree, 2層アーキテクチャ, UpdateAsset, NodeInstance, Editor, Runtime, UE5]
legacy_drive_id: [1wQbzXtlfpF8pugY3GtplXEbkrqQbKfl0MDmtR7zb1yo]
---

# BehaviorTree 2層アーキテクチャとEditor-Runtime同期

## 概要
UE5 BehaviorTreeはEditor層とRuntime層の2層構造を持ち、両者の同期が正しく行われないとノード消失やデータ破損が発生する。

## 2層構造

### Editor層 (UEdGraph系)
- `UBehaviorTreeGraph` — グラフ全体
- `UBehaviorTreeGraphNode` — 各ノードのエディタ表現
- `UBehaviorTreeGraphNode_Composite` — Composite専用
- `UBehaviorTreeGraphNode_Task` — Task専用
- `UBehaviorTreeGraphNode_SubNode` — Decorator/Service

### Runtime層 (UBTNode系)
- `UBTCompositeNode` — Selector, Sequence等
- `UBTTaskNode` — MoveTo, Wait等
- `UBTDecorator` — Blackboard, Cooldown等
- `UBTService` — DefaultFocus等

### 同期の仕組み
- Editor → Runtime: `UpdateAsset()` が `UBehaviorTreeGraphNode::NodeInstance` を通じて同期
- Runtime → Editor: `CreateGraph()` / `SpawnNodeFromTemplate()` がRuntime情報からEditorノード生成

## FGraphNodeCreator::Finalize() RAIIパターン
```cpp
{
    FGraphNodeCreator<UBehaviorTreeGraphNode_Composite> Creator(*Graph);
    UBehaviorTreeGraphNode_Composite* GraphNode = Creator.CreateNode();
    GraphNode->NodeInstance = NewObject<UBTCompositeNode_Sequence>();
    // Creator のデストラクタで Finalize() が呼ばれる
    // → NotifyGraphChanged(), ノードの登録処理が自動実行
}
```

### 重要ポイント
- `Finalize()` はRAIIスコープ終了時に自動呼び出し
- `Finalize()` 前に `NodeInstance` を設定しておく必要がある
- `Finalize()` 内で `CreateNewGuid()`, `PostPlacedNewNode()`, `AllocateDefaultPins()` が実行される

## MCP実装での注意
- ノード作成は必ず Editor層 + Runtime層 の両方を作成する
- `NodeInstance` の設定は `Finalize()` 前に行う
- `UpdateAsset()` を最後に呼んで同期を確定する

## 移行時の注記（2026-09-10）

Drive 原本（`1wQbzXtlfpF8pugY3GtplXEbkrqQbKfl0MDmtR7zb1yo`）の逐語移行。
2026-03-12 の TrapxTrapCpp `InvestigationReports/` 10 件の 1 つ
（誤配置の経緯は [[spirrow-unrealwise:bt-add-subnode-ten-step-pattern]] の注記と同じ）。

**この 2 層構造が、後に primitive レイヤーとして制度化された不変条件の出所である。**
`Docs/Architecture/PrimitiveLayerMigration.md` は "BT 2-layer creation bug" を
**「prompt レベルの文書でしか強制されていなかった」**問題として挙げ、
`SpirrowBridgePrimitives::SafeCreateBTGraphAndRuntimeNode` に封じ込めた。
本書はその「prompt レベルの文書」にあたる時期の分析である。

∴ **本書の価値は、primitive が守っている不変条件の *理由* にある。**
「Editor 層と Runtime 層の両方を作る」「`NodeInstance` は `Finalize()` 前」という 2 行が、
なぜ守らないと壊れるのか（片方だけだと `NodeInstance == nullptr` → orphan 判定 → ノード消失）は
[[spirrow-unrealwise:bt-node-disappearance-mechanism]] と合わせて読むと繋がる。
