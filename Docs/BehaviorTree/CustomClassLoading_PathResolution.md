---
id: spirrow-unrealwise:bt-custom-class-loading
title: BehaviorTree カスタムクラスロードとパス解決
product: spirrow-unrealwise
type: note
status: active
version: 1.0
created: 2026-03-12
last_verified: 2026-09-10
supersedes: []
related: [spirrow-unrealwise:bt-class-not-found-root-cause]
keywords: [BehaviorTree, StaticLoadClass, FGraphNodeClassData, クラスパス, Blueprint, UE5]
legacy_drive_id: [1qcp2S79JoHU6RaPAPyOOmwVNnJrfcggQ5o9LjckHX4g]
---

# BehaviorTree カスタムクラスロードとパス解決

## 概要
UE5 BehaviorTreeでカスタムノードクラスを正しく指定・ロードする方法。

## クラスパスの形式

### C++クラス
```
/Script/ModuleName.ClassName
```
例: `/Script/AIModule.UBTTask_MoveTo`

- `ModuleName` は .uproject/.uplugin の Module名と完全一致が必要
- 大文字小文字を区別する

### Blueprintクラス
```
/Game/Path/To/Asset_C
```
例: `/Game/AI/Tasks/BP_CustomTask_C`

- `_C` サフィックス必須（Classオブジェクト参照）
- アセット移動時にリダイレクタが作られるが、リダイレクタチェーン切れで解決失敗する場合あり

## FGraphNodeClassData のシリアライズ
```cpp
// 保存時
FString ToString() const {
    return FString::Printf(TEXT("%s %s"), *ClassName, *PackageName);
}

// ロード時
bool FGraphNodeClassHelper::FindClass(FGraphNodeClassData& Data) {
    UClass* Class = StaticLoadClass(UObject::StaticClass(), nullptr, *Data.GetClassPath());
    if (!Class) {
        Class = FindObject<UClass>(nullptr, *Data.GetClassPath());
    }
    return Class != nullptr;
}
```

## MCP実装での対策
1. **C++ノード**: モジュール一覧から有効なクラスパスを事前列挙
2. **BPノード**: `AssetRegistry` でアセット存在確認後にパス構築
3. **検証関数**: `StaticLoadClass()` 相当のチェックをMCPツール側で実装
4. **エラーハンドリング**: クラス解決失敗時はノード作成を中止し、ユーザに正しいパスを提示

## 移行時の注記（2026-09-10）

Drive 原本（`1qcp2S79JoHU6RaPAPyOOmwVNnJrfcggQ5o9LjckHX4g`）の逐語移行。
2026-03-12 の TrapxTrapCpp `InvestigationReports/` 10 件の 1 つ
（誤配置の経緯は [[spirrow-unrealwise:bt-add-subnode-ten-step-pattern]] の注記と同じ）。

**本書は「なぜクラスパスの書式ミスがノード消失になるのか」の前半である。**
書式（`/Script/Module.Class` と `/Game/Path/Asset_C`）と、ロード時に
`StaticLoadClass()` → `FindObject<UClass>()` の 2 段で逆解決される仕組みまでが本書。
その解決に失敗した後で何が起きるか（`NodeInstance = nullptr` → orphan 判定 → 自動削除）は
[[spirrow-unrealwise:bt-class-not-found-root-cause]] にある。

repo の `Docs/CustomBTNodeClass_Support_Prompt.md` はカスタム C++ BT ノード対応の
実装指示書だが、**クラスパスの書式そのものと `_C` サフィックスの理由、
リダイレクタチェーン切れの話は含まれていない**。`Class not found` は移行前の
`Docs/` 全体で 0 hit だった。
