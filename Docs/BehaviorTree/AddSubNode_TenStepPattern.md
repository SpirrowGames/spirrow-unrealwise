---
id: spirrow-unrealwise:bt-add-subnode-ten-step-pattern
title: BehaviorTree AddSubNode() 正しいノード追加パターン
product: spirrow-unrealwise
type: note
status: active
version: 1.0
created: 2026-03-12
last_verified: 2026-09-10
supersedes: []
related: [spirrow-unrealwise:bt-decorator-service-attach-api, spirrow-unrealwise:bt-mcp-improvement-proposal-2026-03]
keywords: [BehaviorTree, AddSubNode, Decorator, Service, orphan, NodeInstance, UE5]
legacy_drive_id: [14zU0LeRDLy-Oy-GrbvULrkonyv6OjJ81Sl2DMyRfcSU]
---

# BehaviorTree AddSubNode() 正しいノード追加パターン

## 概要
UE5 BehaviorTreeでDecorator/Serviceをノードに追加する際の正しい10ステップ手順。

## UAIGraphNode::AddSubNode() の実装手順

### 10ステップシーケンス
1. **Modify()** — Undo用にグラフノードの状態を記録
2. **SubNodes配列にAdd** — サブノードを配列に追加
3. **サブノードのParentNode設定** — 親参照を設定
4. **NodeInstanceのオーナー設定** — `SetOuter()` でBehaviorTreeアセットを親に
5. **InitializeNode()** — ランタイムノード初期化（TreeAsset, OwnerComp参照設定）
6. **SubNodes配列内のインデックス設定** — 実行順序決定
7. **HasParentNodeフラグ設定** — Runtime側で親持ちを認識させる
8. **エラーメッセージ初期化** — ノード固有エラー状態リセット
9. **NotifyGraphChanged()** — エディタにグラフ変更を通知
10. **UpdateAsset()** — Editor→Runtimeの同期実行

### 重要な注意点
- **FAISchemaAction_NewSubNode::PerformAction()** が高レベルAPI。内部で `AddSubNode()` を呼ぶ
- **InsertSubNodeAt()** — 特定位置への挿入。index引数の上位ビットでDecorator/Service区別
- ステップの順序を間違えると `NodeInstance == nullptr` になりorphan判定される

## MCP実装への適用
```
// 推奨: FAISchemaAction_NewSubNode相当の処理をMCPツールで実装
1. クラスパス検証
2. NewObject<UBTDecorator_XXX>() でランタイムノード作成
3. AddSubNode() の全10ステップを実行
4. NodeInstance != nullptr を確認
5. MarkPackageDirty() でアセット変更フラグ設定
```

## 移行時の注記（2026-09-10）

Drive 原本（`14zU0LeRDLy-Oy-GrbvULrkonyv6OjJ81Sl2DMyRfcSU`）の逐語移行。
2026-03-12 に TrapxTrapCpp の `InvestigationReports/` で作られた 10 件の 1 つ。

**元は TrapxTrapCpp フォルダにあったが、内容は TrapxTrapCpp のゲーム実装ではなく
SpirrowUnrealWise の BehaviorTree MCP ツールの実装知識である。** ゲーム開発中に BT ツールを
デバッグした副産物が、作業していたプロジェクトのフォルダに置かれたもの
（[[platform:reconciliation-trapxtrapcpp]] §3）。移行にあたり本来の所属へ寄せた。

**この内容は移行前の repo に存在しなかった。** `Docs/` を `AddSubNode` で検索して 0 hit。

### 本書の提案は、そのとおりには実装されていない

現行の `MCPGameProject/Plugins/SpirrowBridge/Source/SpirrowBridge/Private/Commands/SpirrowBridgeAICommands_BTNodeCreation.cpp`
は `AddSubNode()` を呼ばない。`FGraphNodeCreator不使用` と明記した別経路で
Decorator（:805）と Service（:956）を作っている。理由もその場のコメントにある:

> Decorator は `TargetGraphNode->Decorators` 配列にのみ存在すべき。
> `BTGraph->Nodes` に追加すると `UpdateAsset()` で二重処理される。

∴ **本書は「採用されなかった提案」ではなく「なぜ別の道を選んだかを読むための前提」** である。
10 ステップが何をしているかを知らないと、それを迂回した実装の安全性を評価できない。

`Docs/Architecture/PrimitiveLayerMigration.md` も「Decorator と Service は
`FGraphNodeCreator` を使わないので 2 層 primitive の対象外」と明記しており、
**この領域は今も手書きのまま**である。
