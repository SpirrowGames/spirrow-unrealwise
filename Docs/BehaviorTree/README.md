---
id: spirrow-unrealwise:bt-investigation-index
title: BehaviorTree 調査ノート索引（2026-03 UE5 ソース解析）
product: spirrow-unrealwise
type: brief
status: active
version: 1.0
created: 2026-09-10
last_verified: 2026-09-10
supersedes: []
related: [spirrow-unrealwise:bt-mcp-improvement-proposal-2026-03]
keywords: [BehaviorTree, 索引, UE5, 調査ノート, CodePyxis, ue-investigator]
---

# BehaviorTree 調査ノート索引

2026-03-12 に CodePyxis (ue-investigator) で UE5 のソースを解析して作られた 10 件。
**BT MCP ツールがノードを壊していた原因を追った記録**で、9 件が UE5 側の API 解析、
1 件がそれに基づく改善提案。

もとは `TrapxTrapCpp/InvestigationReports/` にあった。ゲーム開発中に BT ツールを
デバッグした副産物が作業中プロジェクトのフォルダに置かれたもので、内容は
TrapxTrapCpp のゲーム実装ではなく本 repo の実装知識である。2026-09-10 の
Drive → Git 移行で本来の所属へ寄せた（[[platform:reconciliation-trapxtrapcpp]] §3）。

## 読む順

**症状から入る場合**（ノードが消えた / Class not found が出た）:

1. [ClassNotFound_RootCause.md](ClassNotFound_RootCause.md) — 症状から真因までの 5 段の連鎖
2. [NodeDisappearance_Mechanism.md](NodeDisappearance_Mechanism.md) — 消失の実行主体 `RemoveOrphanedNodes()`
3. [CustomClassLoading_PathResolution.md](CustomClassLoading_PathResolution.md) — クラスパスの書式と逆解決

**構造から入る場合**（ノードを作る / 繋ぐコードを書く）:

4. [TwoLayerArchitecture_EditorRuntimeSync.md](TwoLayerArchitecture_EditorRuntimeSync.md) — Editor 層と Runtime 層、`UpdateAsset()`
5. [FGraphNodeCreator_RAII.md](FGraphNodeCreator_RAII.md) — ノード生成の RAII と `Finalize()` の中身
6. [MakeLinkTo_PinConnection.md](MakeLinkTo_PinConnection.md) — ピン接続と、省略すると遅れて壊れる 3 つの後処理
7. [CompositeChildren_FBTCompositeChild.md](CompositeChildren_FBTCompositeChild.md) — 子ノードと Decorator の所属
8. [DecoratorService_AttachAPI.md](DecoratorService_AttachAPI.md) — Decorator / Service の追加 API
9. [AddSubNode_TenStepPattern.md](AddSubNode_TenStepPattern.md) — `AddSubNode()` の 10 ステップ

**当時の判断を読む場合**:

10. [MCP_ImprovementProposal_2026-03.md](MCP_ImprovementProposal_2026-03.md) — 5 つの改善提案と、その後どうなったか（`archived`）

## 現行実装との関係（2026-09-10 時点）

これらは **2026-03 時点の解析**である。その後 repo 側が進んでいるので、
そのまま実装指示として読まないこと。各ノートの「移行時の注記」に個別の差分を書いてある。
まとめると:

| 論点 | 現行 |
|---|---|
| `FGraphNodeCreator` の 2 層不変条件 | `SpirrowBridgePrimitives::SafeCreateBTGraphAndRuntimeNode` に封じ込め、直呼びは CI lint で禁止（`Docs/Architecture/PrimitiveLayerMigration.md`） |
| **Decorator / Service** | **上記 primitive の対象外**。`FGraphNodeCreator不使用` の手書き経路のまま（`SpirrowBridgeAICommands_BTNodeCreation.cpp:805 / :956`） |
| `AddSubNode()` の 10 ステップ | 呼んでいない。`TargetGraphNode->Decorators` に直接入れる（`BTGraph->Nodes` に足すと `UpdateAsset()` で二重処理されるため） |
| orphan 検出 | 実装済み（`Docs/BrokenBTNodes_Detection_Fix.md`） |

∴ **9 件の UE5 API 解析は今も有効**（UE5 側の話なので陳腐化しない）。
**提案書 1 件だけが役目を終えている**ので `archived`。
