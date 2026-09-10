---
id: spirrow-unrealwise:bt-decorator-service-attach-api
title: BehaviorTree Decorator/Service アタッチAPI詳解
product: spirrow-unrealwise
type: note
status: active
version: 1.0
created: 2026-03-12
last_verified: 2026-09-10
supersedes: []
related: [spirrow-unrealwise:bt-add-subnode-ten-step-pattern, spirrow-unrealwise:bt-composite-children-fbtcompositechild]
keywords: [BehaviorTree, Decorator, Service, FAISchemaAction_NewSubNode, InsertSubNodeAt, SubNodes, UE5]
legacy_drive_id: [13rZ_h_8V7zUiTlUPuXVEo6bYHeVWAYvJEpyyMGx-vVo]
---

# BehaviorTree Decorator/Service アタッチAPI詳解

## 概要
BehaviorTreeノードにDecorator/Serviceをプログラムから追加する方法の詳細。

## 高レベルAPI: FAISchemaAction_NewSubNode

### PerformAction() の処理フロー
1. ユーザが右クリック→「Add Decorator/Service」選択
2. `FAISchemaAction_NewSubNode::PerformAction()` が呼ばれる
3. 内部で `UAIGraphNode::AddSubNode()` を呼び出し
4. AddSubNode() が10ステップの追加処理を実行

### InsertSubNodeAt() のインデックスエンコーディング
```cpp
void UAIGraphNode::InsertSubNodeAt(UAIGraphNode* SubNode, int32 DropIndex)
```
- `DropIndex` の上位ビットでDecorator/Serviceを区別
- ドラッグ&ドロップでの並び替え時に使用
- 具体的なビットマスク: 実装依存だがDecoratorが先、Serviceが後

## Runtime側の対応関係
| Editor | Runtime |
|--------|---------|
| UBehaviorTreeGraphNode_SubNode (Decorator) | UBTDecorator |
| UBehaviorTreeGraphNode_SubNode (Service) | UBTService |
| SubNodes配列 | FBTCompositeChild::Decorators/Services |

## MCP実装パターン
```
// Decorator追加
1. UBTDecorator_Xxx* Decorator = NewObject<UBTDecorator_Xxx>(BTAsset);
2. UBehaviorTreeGraphNode_SubNode* SubGraphNode = CreateSubNodeGraphNode();
3. SubGraphNode->NodeInstance = Decorator;
4. ParentGraphNode->AddSubNode(SubGraphNode);
5. UpdateAsset();

// Service追加も同様だがUBTService_Xxxを使用
```

## 注意点
- SubNodeのNodeInstanceタイプ（UBTDecorator vs UBTService）で自動分類される
- AddSubNode()内でParentNode参照が設定される
- 追加後のSubNodes配列順序 = エディタ表示順序 = 実行優先度

## 移行時の注記（2026-09-10）

Drive 原本（`13rZ_h_8V7zUiTlUPuXVEo6bYHeVWAYvJEpyyMGx-vVo`）の逐語移行。
2026-03-12 の TrapxTrapCpp `InvestigationReports/` 10 件の 1 つ
（誤配置の経緯は [[spirrow-unrealwise:bt-add-subnode-ten-step-pattern]] の注記と同じ）。

**「Runtime側の対応関係」の表が本書の要点である。** Editor 側は Decorator も Service も
同じ `UBehaviorTreeGraphNode_SubNode` で、**`NodeInstance` の型でしか区別されない**。
つまり型を間違えて代入しても Editor 層では気付けず、`FBTCompositeChild` のどちらの配列に
入るかが変わって初めて実行時に効く。

`InsertSubNodeAt()` の「`DropIndex` の上位ビットで Decorator/Service を区別」も
同じ性質の落とし穴で、素直に index を渡すと分類が変わる。本書自身が
「具体的なビットマスク: 実装依存」と留保しているので、**そこは実測が要る**まま。

現行実装（`SpirrowBridgeAICommands_BTNodeCreation.cpp:805 / :956`）は
`AddSubNode()` も `InsertSubNodeAt()` も使わず、`NewObject` で Editor / Runtime の
両ノードを作って `TargetGraphNode->Decorators` に直接入れる経路を採っている。
∴ 本書の「MCP実装パターン」は現行と異なる。差分の理由は
[[spirrow-unrealwise:bt-add-subnode-ten-step-pattern]] の注記にある。
