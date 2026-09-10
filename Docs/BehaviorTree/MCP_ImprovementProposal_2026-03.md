---
id: spirrow-unrealwise:bt-mcp-improvement-proposal-2026-03
title: SpirrowUnrealWise BehaviorTree MCP実装改善提案（2026-03）
product: spirrow-unrealwise
type: note
status: archived
version: 1.0
created: 2026-03-12
last_verified: 2026-09-10
supersedes: []
related: [spirrow-unrealwise:bt-add-subnode-ten-step-pattern, spirrow-unrealwise:bt-class-not-found-root-cause, spirrow-unrealwise:bt-node-disappearance-mechanism]
keywords: [BehaviorTree, MCP, 改善提案, bt_validate_tree, CodePyxis, ue-investigator]
legacy_drive_id: [1k0ti78DRj3wVifxMwsmoYbvPcsun0pU3uoWyQDy4Geg]
---

# SpirrowUnrealWise BehaviorTree MCP実装改善提案

## 概要
CodePyxis (ue-investigator) によるUE5ソースコード解析結果に基づく、SpirrowUnrealWise MCPツールの改善提案。

## 現状の問題
SpirrowUnrealWiseのBTノード追加処理で以下の問題が発生していた:
1. 「Class not found」エラー → ノード消失
2. Decorator/Serviceが正しくアタッチされない
3. ピン接続後にRuntime側に反映されない

## 改善提案

### 1. ノード作成ツールの修正
- `AddSubNode()` の全10ステップを忠実に実装する
- `FGraphNodeCreator::Finalize()` 相当の処理を漏れなく実行
- `NodeInstance` 設定の順序を厳守（Finalize前に設定）

### 2. クラスパス検証の追加
- ノード作成前に `StaticLoadClass()` 相当のチェックを実行
- C++: `/Script/Module.Class` 形式の妥当性検証
- BP: `/Game/Path/Asset_C` のアセット存在確認
- 失敗時は作成を中止しエラーメッセージを返す

### 3. 同期処理の確実な実行
- `NotifyGraphChanged()` + `UpdateAsset()` のペアを全変更操作後に実行
- バッチ操作時は最後に1回だけ呼ぶ（パフォーマンス改善）
- `Modify()` を変更前に必ず呼ぶ（Undo対応）

### 4. orphan検出の改善
- `RemoveOrphanedNodes()` の自動実行に依存せず、MCPツール側で事前検証
- ノード作成後に `NodeInstance != nullptr` を検証する安全チェック追加
- 問題検出時はノード削除ではなくエラー報告

### 5. 新規MCPツール提案
- `bt_validate_tree` — ツリー全体の整合性チェック（orphan検出、ClassData検証）
- `bt_add_subnode_safe` — 安全なDecorator/Service追加（全10ステップ保証）
- `bt_connect_nodes_safe` — 安全なピン接続（後処理保証）
- `bt_get_class_list` — 利用可能なBTノードクラス一覧取得

## 実装優先度
1. **最優先**: AddSubNode() 10ステップ実装 — 現在の主要バグの根本原因
2. **高**: クラスパス検証 — "Class not found" の予防
3. **中**: 同期処理の確実な実行 — データ不整合の予防
4. **低**: 新規ツール — 開発効率向上

## 移行時の注記（2026-09-10）

Drive 原本（`1k0ti78DRj3wVifxMwsmoYbvPcsun0pU3uoWyQDy4Geg`）の逐語移行。
2026-03-12 の TrapxTrapCpp `InvestigationReports/` 10 件のうち、**唯一の提案書**
（他 9 件は UE5 側の API 解析）。誤配置の経緯は
[[spirrow-unrealwise:bt-add-subnode-ten-step-pattern]] の注記と同じ。

**`status: archived` の理由: 提案としては役目を終えている。** 5 項目の帰結:

| 提案 | その後 |
|---|---|
| 1. `AddSubNode()` の 10 ステップを忠実に実装 | **採用されなかった。** 現行は `AddSubNode()` を呼ばず、`FGraphNodeCreator不使用` の別経路で Decorator / Service を作る（`SpirrowBridgeAICommands_BTNodeCreation.cpp:805 / :956`）。理由は「`BTGraph->Nodes` に追加すると `UpdateAsset()` で二重処理される」 |
| 2. クラスパス検証 | `Docs/CustomBTNodeClass_Support_Prompt.md` がカスタム C++ BT ノード対応として存在 |
| 3. 同期処理の確実な実行 | `FGraphNodeCreator` → `Finalize` の不変条件は `SpirrowBridgePrimitives::SafeCreateBTGraphAndRuntimeNode` に封じ込められ、直呼びは CI lint で禁止（`Docs/Architecture/PrimitiveLayerMigration.md` / `Docs/CHANGELOG.md`）。ただし **Decorator / Service は対象外**と明記されている |
| 4. orphan 検出の改善 | **実装された。** `Docs/BrokenBTNodes_Detection_Fix.md` = 「NodeInstance が null の壊れたノードを検出・修復」。本書の「自動削除ではなくエラー報告」に対応 |
| 5. 新規 MCP ツール 4 種 | `bt_validate_tree` / `bt_add_subnode_safe` / `bt_connect_nodes_safe` / `bt_get_class_list` — **この名前では実装されていない** |

∴ 最優先とされた項目 1 が採用されず、最も優先度が低いとされた項目 4 が実装された。
**提案の優先度と実際の解決順序が逆転している**のが本書の記録価値である。

### repo 側の別文書と混同しないこと

`TrapxTrapCpp/Docs/SpirrowUnrealWise_Enhancement_Request.md`（7197 B、2026-01-12）は
題名が似ているが**別文書**である。本文照合の結果、内容の重複は**ゼロ**:

- 向こう: `import_texture` / `asset_exists` / Content Browser 操作 / バッチ操作 /
  外部 API 連携 / DataAsset プロパティ改善（BT の記述なし）
- 本書: BT ノード生成のバグ 3 件と、その解析に基づく 5 提案

[[platform:reconciliation-trapxtrapcpp]] §3.2 が「対応する可能性が高いが本文照合していない」と
保留していた 1 件がこれで、**照合の結果 対応しない**と確定した。∴ 移行対象は 9 件ではなく 10 件。
