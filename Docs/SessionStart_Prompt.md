# SpirrowUnrealWise - 新規チャット開始プロンプト

## 概要

SpirrowUnrealWise は Unreal Engine 5 と MCP（Model Context Protocol）を接続し、LLM から UE エディタを操作するためのツール群です。

## プロジェクト情報

- **プロジェクトルート**: `C:\Users\takahito ito\Documents\Unreal Projects\spirrow-unrealwise`
- **ドキュメント**: `.claude/claude.md`、`AGENTS.md`、`FEATURE_STATUS.md`
- **現在のフェーズ**: Phase 0.6.0 - 制御フロー・ユーティリティノードツール追加完了

## 会話開始時の手順

1. プロジェクトコンテキストを取得:
```
get_project_context("SpirrowUnrealWise")
```

2. 必要に応じてドキュメントを確認:
- `AGENTS.md` - 全体ガイドライン、ノード配置ルール、コマンド追加手順
- `FEATURE_STATUS.md` - 機能一覧、テスト状況、更新履歴
- `.claude/claude.md` - Claude.ai と Claude Code の役割分担

## 現在の状況（2025-12-25）

### 最新の実装

**Node Tools v0.6.0**（20ツール）:
- ✅ 基本ノード操作（8ツール）
- ✅ 変数・分岐（6ツール）
- ✅ 制御フロー: add_sequence_node, add_delay_node, add_print_string_node
- ⚠️ 未対応: add_math_node, add_comparison_node, add_foreach_loop_node

### 既知の問題

1. **Math/Comparison ノード**
   - 問題: "Failed to find math function" エラー
   - 原因: `UK2Node_CommutativeAssociativeBinaryOperator` を使う必要がある
   - 修正プロンプト: `Docs/NodeTools_MathComparison_Fix_Prompt.md`

2. **ForEach Loop ノード**
   - 問題: Blueprint マクロとして実装されているため K2Node では作成不可
   - ワークアラウンド: UE エディタで手動追加

### Sequence ノードのピン名

重要: ピン名はアンダースコア区切りの小文字
- 入力: `execute`
- 出力: `then_0`, `then_1`, `then_2`, ...

## 次のタスク候補

1. Math/Comparison ノードの修正（優先度: 中）
2. ForEach Loop の代替実装検討
3. 既存ノードツールの統合テスト
4. 新機能の追加（ユーザー要望に応じて）

## ファイル構成

```
spirrow-unrealwise/
├── .claude/claude.md          # Claude.ai/Code 役割分担
├── AGENTS.md                  # AIエージェント向けガイドライン
├── FEATURE_STATUS.md          # 機能ステータス一覧
├── Docs/
│   ├── Tools/
│   │   ├── README.md          # ツール一覧
│   │   └── node_tools.md      # ノードツール詳細ドキュメント
│   └── NodeTools_MathComparison_Fix_Prompt.md  # 修正プロンプト
├── Python/
│   └── tools/
│       └── node_tools.py      # Python MCP ツール定義
└── Plugins/SpirrowBridge/
    └── Source/SpirrowBridge/
        ├── Public/Commands/
        │   └── SpirrowBridgeBlueprintNodeCommands.h
        └── Private/Commands/
            └── SpirrowBridgeBlueprintNodeCommands.cpp
```

## 作業フロー

1. Claude.ai で機能設計・仕様策定
2. 仕様確定後、実装プロンプトを作成（`Docs/{FeatureName}_Prompt.md`）
3. Claude Code で実装（C++ / Python）
4. ビルド・再起動
5. MCP ツールでテスト
6. ドキュメント更新

## 会話終了時

```python
update_project_context(
    project_name="SpirrowUnrealWise",
    current_phase="現在のフェーズ",
    recent_changes=["変更点1", "変更点2"],
    next_tasks=["次のタスク1", "次のタスク2"],
    known_issues=["既知の問題"],
    notes="補足メモ"
)
```
