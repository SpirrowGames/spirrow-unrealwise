# 機能名: プロジェクトコンテキスト管理機能 実装

## 概要

長い開発会話でコンテキスト溢れが発生する問題を軽減するため、RAGベースのプロジェクトコンテキスト管理機能を追加する。

## 実装ステップ

| Step | 内容 | ファイル |
|------|------|----------|
| 1 | インポート追加 | `tools/rag_tools.py` |
| 2 | 内部関数2つ追加 | `tools/rag_tools.py` |
| 3 | `get_project_context` ツール追加 | `tools/rag_tools.py` |
| 4 | `update_project_context` ツール追加 | `tools/rag_tools.py` |

各ステップの詳細は以下のファイルを参照:

- [Step1_Import.md](ProjectContext/Step1_Import.md)
- [Step2_InternalFunctions.md](ProjectContext/Step2_InternalFunctions.md)
- [Step3_GetProjectContext.md](ProjectContext/Step3_GetProjectContext.md)
- [Step4_UpdateProjectContext.md](ProjectContext/Step4_UpdateProjectContext.md)

## テスト手順

[Step5_Test.md](ProjectContext/Step5_Test.md) を参照

## データ構造（参考）

```json
{
  "project_name": "TrapxTrapCpp",
  "current_phase": "Phase 3 - トラップ解除システム",
  "recent_changes": ["DisarmProgressWidget実装完了"],
  "known_issues": ["マルチプレイ時のレプリケーション未対応"],
  "next_tasks": ["解除キャンセル処理"],
  "notes": "Phase 2のシューティングシステムは完了済み",
  "updated_at": "2025-01-15T12:30:00"
}
```
