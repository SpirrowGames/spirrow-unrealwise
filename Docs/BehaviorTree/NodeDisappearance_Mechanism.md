---
id: spirrow-unrealwise:bt-node-disappearance-mechanism
title: BehaviorTree ノード消失メカニズムと防止策
product: spirrow-unrealwise
type: note
status: active
version: 1.0
created: 2026-03-12
last_verified: 2026-09-10
supersedes: []
related: [spirrow-unrealwise:bt-class-not-found-root-cause, spirrow-unrealwise:bt-two-layer-architecture]
keywords: [BehaviorTree, RemoveOrphanedNodes, orphan, NodeInstance, UpdateAsset, ノード消失, UE5]
legacy_drive_id: [1E3sKfmA7DpXXPUQmpR3JuMQ5vwXOmCKos3l578u4DMs]
---

# BehaviorTree ノード消失メカニズムと防止策

## 問題概要
BehaviorTreeエディタでノードが突然消える現象のメカニズム解析。

## メカニズム: RemoveOrphanedNodes()
`UAIGraph::RemoveOrphanedNodes()` がグラフ更新時に自動実行される。

### 判定ロジック
```
for each AIGraphNode in graph:
    if node.NodeInstance == nullptr:
        // ClassDataが空でない = 以前は有効だったが解決失敗
        → orphanと判定、グラフから削除
```

### トリガーポイント
- アセットロード時
- エディタでのグラフ再構築時（`UpdateAsset()` 呼び出し時）
- undo/redo操作後の整合性チェック

## MCP実装での防止策
1. **ノード作成直後の検証**: `NodeInstance` が正常に設定されたか確認
2. **クラスパスの事前検証**: `StaticLoadClass()` で解決可能か確認してからノード作成
3. **Undo対応**: `Modify()` を確実に呼んでからグラフ変更を行う
4. **UpdateAsset()呼び出しタイミング**: グラフ変更完了後に1回だけ呼ぶ（途中で呼ぶとorphan判定される可能性）

## 関連ソースコード
- `AIGraph.cpp` — RemoveOrphanedNodes(), UpdateAsset()
- `AIGraphNode.cpp` — NodeInstance管理
- `BehaviorTreeGraph.cpp` — BT固有のグラフ更新処理

## 移行時の注記（2026-09-10）

Drive 原本（`1E3sKfmA7DpXXPUQmpR3JuMQ5vwXOmCKos3l578u4DMs`）の逐語移行。
2026-03-12 の TrapxTrapCpp `InvestigationReports/` 10 件の 1 つ
（誤配置の経緯は [[spirrow-unrealwise:bt-add-subnode-ten-step-pattern]] の注記と同じ）。

**防止策 4 が本書で一番実務的である。** 「`UpdateAsset()` はグラフ変更完了後に 1 回だけ。
途中で呼ぶと orphan 判定される可能性」— つまり **同期を「こまめに」呼ぶという善意が
ノードを消す**。他の 3 つは「やり忘れるな」だが、これだけは「やりすぎるな」で、
方向が逆なので特に間違えやすい。

[[spirrow-unrealwise:bt-makelinkto-pin-connection]] の「複数接続を一括で行う場合、
全接続完了後に 1 回だけ呼ぶ方が効率的」は、効率の話として書かれているが、
本書と合わせると**正しさの話でもある**ことが分かる。

repo の `Docs/BrokenBTNodes_Detection_Fix.md` が実装した検出・修復機能は、
本書が解析した `RemoveOrphanedNodes()` の判定条件（`NodeInstance == nullptr`）を
そのまま検出条件に使っている。
