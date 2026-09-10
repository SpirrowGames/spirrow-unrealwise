---
id: spirrow-unrealwise:bt-composite-children-fbtcompositechild
title: BehaviorTree Composite子ノード管理とFBTCompositeChild
product: spirrow-unrealwise
type: note
status: active
version: 1.0
created: 2026-03-12
last_verified: 2026-09-10
supersedes: []
related: [spirrow-unrealwise:bt-decorator-service-attach-api, spirrow-unrealwise:bt-makelinkto-pin-connection]
keywords: [BehaviorTree, FBTCompositeChild, Composite, Children, SpawnNodeFromTemplate, UE5]
legacy_drive_id: [15pbRrR6BTD6nQdcUE9LZf3qOaUW2_sqIPLED6vOj1ZE]
---

# BehaviorTree Composite子ノード管理とFBTCompositeChild

## 概要
UE5 BehaviorTreeのCompositeノード（Selector, Sequence等）が子ノードを管理する仕組み。

## FBTCompositeChild構造体
```cpp
struct FBTCompositeChild
{
    UBTCompositeNode* ChildComposite;  // 子がCompositeの場合
    UBTTaskNode* ChildTask;            // 子がTaskの場合
    TArray<UBTDecorator*> Decorators;  // この子に付くDecorator群
    TArray<UBTService*> Services;      // この子に付くService群
};
```

### 重要な設計
- `ChildComposite` と `ChildTask` は排他的（片方がnullptr）
- Decorator/Serviceは「子ごと」に管理される（ノードごとではない）
- 配列のインデックス = 実行順序（Selectorは先頭から評価、Sequenceは先頭から実行）

## Children配列の管理
- `UBTCompositeNode::Children` — TArray<FBTCompositeChild>
- エディタのピン接続順序がそのまま配列順序に反映
- `CollectAllNodeInstances()` で子孫ノードを再帰的に収集

## SpawnNodeFromTemplate()
`UBehaviorTreeGraph::SpawnNodeFromTemplate()` でRuntime→Editorのノード生成:
1. `FGraphNodeCreator` でEditorノード作成
2. `NodeInstance` にRuntimeノードをセット
3. 位置情報、ピン接続を復元
4. サブノード（Decorator/Service）も再帰的に生成

## MCP実装での注意
- 子ノード追加時はFBTCompositeChildを正しく構築
- ChildComposite/ChildTaskの排他設定を守る
- 追加後に `UpdateAsset()` で同期必須

## 移行時の注記（2026-09-10）

Drive 原本（`15pbRrR6BTD6nQdcUE9LZf3qOaUW2_sqIPLED6vOj1ZE`）の逐語移行。
2026-03-12 の TrapxTrapCpp `InvestigationReports/` 10 件の 1 つ
（誤配置の経緯は [[spirrow-unrealwise:bt-add-subnode-ten-step-pattern]] の注記と同じ）。

**「Decorator/Service は『子ごと』に管理される（ノードごとではない）」が
本書で最も間違えやすい点である。** 直感的には Decorator はノードに付いていると
思いたくなるが、実際は親 Composite の `Children[i].Decorators` に入る。
∴ 同じ Task ノードでも、**どの親のどのスロットに繋がっているかで Decorator の所属先が変わる**。

現行実装が `TargetGraphNode->Decorators` を直接扱い、`BTGraph->Nodes` には足さない
（`SpirrowBridgeAICommands_BTNodeCreation.cpp:805` の「`BTGraph->Nodes` に追加すると
`UpdateAsset()` で二重処理される」）のも、この構造が理由である。

`FBTCompositeChild` は移行前の repo の `Docs/` にも API 名としては 3 ファイルに
登場していたが、この排他性と所属の説明はどこにも無かった。
