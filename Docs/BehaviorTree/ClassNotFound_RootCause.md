---
id: spirrow-unrealwise:bt-class-not-found-root-cause
title: BehaviorTree "Class not found" エラーの根本原因と解決策
product: spirrow-unrealwise
type: note
status: active
version: 1.0
created: 2026-03-12
last_verified: 2026-09-10
supersedes: []
related: [spirrow-unrealwise:bt-custom-class-loading, spirrow-unrealwise:bt-node-disappearance-mechanism]
keywords: [BehaviorTree, Class not found, FGraphNodeClassData, RemoveOrphanedNodes, ノード消失, UE5]
legacy_drive_id: [16rgrJBt4iK5svt5sDHYOOuDLLDHpYxPHIa2fZcmiVGY]
---

# BehaviorTree "Class not found" エラーの根本原因と解決策

## 問題概要
UE5 BehaviorTreeエディタで「Class not found」エラーが発生し、ノードが消失する問題。

## 根本原因
FGraphNodeClassDataのシリアライゼーションにおいて、ClassNameとPackageNameの組み合わせでクラスを解決する。
解決に失敗する主なケース：
- **C++クラス**: `/Script/ModuleName.ClassName` 形式が必要。モジュール名の不一致で失敗
- **Blueprintクラス**: `/Game/Path/Asset_C` 形式。アセット移動・リネームで参照切れ

### エラー発生フロー
1. `FGraphNodeClassData::ToString()` でシリアライズ
2. ロード時に `StaticLoadClass()` / `FindObject<UClass>()` で逆解決
3. 失敗時 → `NodeInstance = nullptr` となる
4. `UAIGraph::RemoveOrphanedNodes()` が `NodeInstance == nullptr` を孤立ノードと判定
5. ノードがグラフから自動削除 → ユーザには「ノードが消えた」ように見える

## 解決策
### MCP実装での対策
1. クラスパス指定時に `/Script/ModuleName.ClassName` 形式を厳格に検証
2. ノード作成後に `NodeInstance != nullptr` を必ず確認
3. BPクラス使用時はフルパスの存在チェックを事前実行
4. ロード後に `RemoveOrphanedNodes()` 相当のチェックでorphan検出を行い、自動削除ではなく警告を出す

## 関連ソースコード
- `AIGraphTypes.h/cpp` — FGraphNodeClassData定義
- `AIGraphNode.cpp` — UAIGraphNode::FindNewNodeClass()
- `AIGraph.cpp` — UAIGraph::RemoveOrphanedNodes()

## 調査元
CodePyxis (ue-investigator) による UE5ソースコード解析結果

## 移行時の注記（2026-09-10）

Drive 原本（`16rgrJBt4iK5svt5sDHYOOuDLLDHpYxPHIa2fZcmiVGY`）の逐語移行。
2026-03-12 の TrapxTrapCpp `InvestigationReports/` 10 件の 1 つ
（誤配置の経緯は [[spirrow-unrealwise:bt-add-subnode-ten-step-pattern]] の注記と同じ）。

**5 ステップの「エラー発生フロー」が本書の芯である。** ユーザに見える症状（ノードが消えた）と
真因（クラスパスの書式ミス）の間に 4 段の変換が挟まっており、しかも **UE 側の親切心
（orphan の自動削除）が最後の 1 段を担っている**。この連鎖を知らないと、消失を
「エディタのバグ」と誤診する。

**§解決策 4 は実装されている。** repo の `Docs/BrokenBTNodes_Detection_Fix.md` が
「NodeInstance が null の『壊れたノード』を検出・修復する機能」を提供しており、
本書が言う「自動削除ではなく警告を出す」に対応する。同書は「通常の `list_bt_nodes` では
検出できない」とも書いており、検出が別機能として要る理由も本書の 5 ステップで説明がつく。

`Class not found` は移行前の repo の `Docs/` 全体で **0 hit** だった。
症状名で検索しても何も出てこない状態だったことになる。
