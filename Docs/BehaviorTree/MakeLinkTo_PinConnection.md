---
id: spirrow-unrealwise:bt-makelinkto-pin-connection
title: BehaviorTree MakeLinkTo() ピン接続と必須後処理
product: spirrow-unrealwise
type: note
status: active
version: 1.0
created: 2026-03-12
last_verified: 2026-09-10
supersedes: []
related: [spirrow-unrealwise:bt-two-layer-architecture, spirrow-unrealwise:bt-composite-children-fbtcompositechild]
keywords: [BehaviorTree, MakeLinkTo, UpdateAsset, NotifyGraphChanged, ピン接続, UE5]
legacy_drive_id: [14X9Bke4E_LXntkHZS7za_jNgTiyaylGo0hFl5-fWm3M]
---

# BehaviorTree MakeLinkTo() ピン接続と必須後処理

## 概要
UE5 BehaviorTreeでノード間のピン接続を行う際の正しい手順と、省略すると問題が発生する後処理。

## UEdGraphPin::MakeLinkTo() の使い方

### 基本手順
```cpp
// 1. 出力ピン(親)と入力ピン(子)を取得
UEdGraphPin* OutputPin = ParentNode->GetOutputPin();
UEdGraphPin* InputPin = ChildNode->GetInputPin();

// 2. 接続前にModify()
ParentNode->Modify();
ChildNode->Modify();

// 3. ピン接続
OutputPin->MakeLinkTo(InputPin);

// 4. 必須後処理
ParentNode->GetGraph()->NotifyGraphChanged();
UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(ParentNode->GetGraph());
BTGraph->UpdateAsset();
```

### 必須後処理 — 省略するとどうなるか
| 後処理 | 省略時の問題 |
|--------|-------------|
| `NotifyGraphChanged()` | エディタ表示が更新されない、レイアウト崩れ |
| `UpdateAsset()` | Editor層の接続がRuntime層に反映されない → 実行時にノードがスキップされる |
| `Modify()` | Undo不可能、エディタクラッシュの可能性 |

### Composite子ノードの順序管理
- `FBTCompositeChild` 構造体がCompositeの各子を保持
- ピンの接続順序 = 実行順序（左から右）
- 子の並び替えは `UBehaviorTreeGraphNode::UpdateChildOrder()` で反映

## MCP実装での注意
1. 接続前に両ノードの `Modify()` を必ず呼ぶ
2. `MakeLinkTo()` 後に `NotifyGraphChanged()` + `UpdateAsset()` を必ず呼ぶ
3. 複数接続を一括で行う場合、全接続完了後に1回だけ `NotifyGraphChanged()` + `UpdateAsset()` を呼ぶ方が効率的

## 移行時の注記（2026-09-10）

Drive 原本（`14X9Bke4E_LXntkHZS7za_jNgTiyaylGo0hFl5-fWm3M`）の逐語移行。
2026-03-12 の TrapxTrapCpp `InvestigationReports/` 10 件の 1 つ
（誤配置の経緯は [[spirrow-unrealwise:bt-add-subnode-ten-step-pattern]] の注記と同じ）。

**「省略するとどうなるか」の表が本書の芯である。** 3 つの後処理はどれも呼ばなくても
コンパイルは通り、エディタ上も一見正常に見える。壊れ方が「実行時にノードがスキップされる」
「Undo でクラッシュする」と**遅延して現れる**ため、呼び忘れと症状が結びつかない。
この対応表が無いと、症状から原因へ戻れない。

`MakeLinkTo` は移行前の repo の `Docs/` にも API 名としては 3 ファイルに登場していたが、
この 3 行の因果表はどこにも無かった。
