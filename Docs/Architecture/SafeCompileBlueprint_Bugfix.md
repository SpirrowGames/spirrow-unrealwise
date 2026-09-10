---
id: spirrow-unrealwise:safe-compile-blueprint-bugfix
title: SpirrowBridge SafeCompileBlueprint Bugfix (2026-04-02)
product: spirrow-unrealwise
type: note
status: active
version: 1.0
created: 2026-04-02
last_verified: 2026-09-10
supersedes: []
related: [spirrow-unrealwise:implementation-summary, spirrow-unrealwise:changelog]
keywords: [SafeCompileBlueprint, EXCEPTION_ACCESS_VIOLATION, GeneratedClass, SkeletonGeneratedClass, SCS, ue-investigator]
legacy_drive_id: [1d9iMtwkRjIPXnVeG6LrJLuQJW_NwXh3rOkDfIASTGNo]
---

# SpirrowBridge SafeCompileBlueprint Bugfix (2026-04-02)

## Problem
Character BP等にコンポーネント追加後、compile_blueprintやset_default_mapping_contextでEXCEPTION_ACCESS_VIOLATIONクラッシュ。

## Root Cause
UE5 BP compilation pipeline: GeneratedClass/SkeletonGeneratedClassポインタ不整合。SCSノード追加後、スケルトンクラスが古い構造のまま残り、CompileBlueprintがnullポインタ参照。ue-investigator (Qwen3.5-397B) による深層調査で特定。

## Solution
SafeCompileBlueprint() utility function:
- GeneratedClass == SkeletonGeneratedClass → null reset + GenerateBlueprintSkeleton + Compile
- Already compiled → MarkPackageDirty only (skip recompile to avoid crash)

## Modified Files (6 files, 20+ locations)
- SpirrowBridgeCommonUtils.h/cpp (new function)
- BlueprintComponentCommands, BlueprintCoreCommands, ProjectCommands
- AIPerceptionCommands (6), BlueprintPropertyCommands (5), GASCommands (5)

All RegenerateSkeletonOnly + CompileBlueprint patterns replaced with SafeCompileBlueprint.

## 移行時の注記（2026-09-10）

Drive 原本（`1d9iMtwkRjIPXnVeG6LrJLuQJW_NwXh3rOkDfIASTGNo`）の逐語移行。

**[[platform:reconciliation-unrealwise]] は「移行すべき新規文書は 0 件」と結論していたが、
本書は例外だった。** 同ブリーフ §4 が本書だけを「未確認」として残しており、対応先候補に
`Docs/Architecture/PrimitiveLayerMigration.md` を挙げつつ **「本文照合していない」「誤った
`legacy_drive_id` は旧 ID の解決先を間違えるので、空欄のままにする方がまし」**と警告していた。

照合した結果、**候補は誤りだった。** `PrimitiveLayerMigration.md` は本件を
「primitive レイヤーを作る動機になったバグ 3 件」の 1 行として挙げているだけで、
本件についての文書ではない。repo 内の他の言及も同様に一行:

| 場所 | 記述 |
|---|---|
| `Docs/Architecture/PrimitiveLayerMigration.md:16` | 動機となったバグの一覧に 1 行 |
| `Docs/DEV_CHEATSHEET.md:102` | 「`RegenerateSkeletonOnly + CompileBlueprint` を置き換える」の 1 行 |
| `Docs/IMPLEMENTATION_SUMMARY.md:170` | 関数一覧に 1 行 |
| `Docs/CHANGELOG.md:61` | 同じく動機の一覧に 1 行 |

∴ **repo が持っていないのは「なぜ落ちるのか」である。** 本書にしかない:

- **機序**: SCS ノード追加後、スケルトンクラスが古い構造のまま残り、`CompileBlueprint` が
  null ポインタを参照する
- **分岐が 2 つある理由**: `GeneratedClass == SkeletonGeneratedClass` のときだけ
  null reset + `GenerateBlueprintSkeleton` + Compile を行い、**既にコンパイル済みなら
  再コンパイルを避けて `MarkPackageDirty` のみ**。後者は「安全側に倒す」ではなく
  「再コンパイル自体がクラッシュ経路」という判断
- **調査手段**: ue-investigator (Qwen3.5-397B) による深層調査
- **影響範囲の実数**: 6 ファイル 20+ 箇所（AIPerceptionCommands 6 / BlueprintPropertyCommands 5 /
  GASCommands 5）

現行の `SafeCompileBlueprint` は v0.11.0 で `SpirrowBridgePrimitives` 名前空間へ移管され、
`FSpirrowBridgeCommonUtils` 側は thin wrapper として残っている
（`Docs/IMPLEMENTATION_SUMMARY.md` / `Docs/CHANGELOG.md`）。**関数の置き場所は変わったが、
本書が記録している 2 分岐の中身は変わっていない。**
