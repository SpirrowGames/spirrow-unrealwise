# Step 5: テスト

## 前提条件

- RAGサーバーが `localhost:8100` で起動している
- MCPサーバーを再起動済み

## テスト 1: 存在しないプロジェクトの取得

```python
get_project_context("TestProject")
```

**期待結果:**
```json
{
  "success": true,
  "found": false,
  "message": "No context found for project 'TestProject'. Use update_project_context to create one."
}
```

## テスト 2: コンテキスト作成

```python
update_project_context(
    project_name="TestProject",
    current_phase="Phase 1 - 初期設定",
    recent_changes=["プロジェクト作成"],
    next_tasks=["基本機能実装"]
)
```

**期待結果:**
```json
{
  "success": true,
  "project_name": "TestProject",
  "current_phase": "Phase 1 - 初期設定",
  "updated_at": "2025-...",
  "message": "Project context for 'TestProject' updated successfully"
}
```

## テスト 3: コンテキスト取得

```python
get_project_context("TestProject")
```

**期待結果:**
```json
{
  "success": true,
  "found": true,
  "context": {
    "project_name": "TestProject",
    "current_phase": "Phase 1 - 初期設定",
    "recent_changes": ["プロジェクト作成"],
    "known_issues": [],
    "next_tasks": ["基本機能実装"],
    "notes": "",
    "updated_at": "2025-..."
  }
}
```

## テスト 4: コンテキスト上書き

```python
update_project_context(
    project_name="TestProject",
    current_phase="Phase 2 - 機能実装",
    recent_changes=["機能A完了", "機能B完了"],
    known_issues=["バグX"],
    notes="順調に進行中"
)
```

再度 `get_project_context("TestProject")` を実行し、新しい内容に置き換わっていることを確認。

## クリーンアップ

テスト後、RAGから `TestProject` のコンテキストを削除:

```python
# list_knowledge() でIDを確認し、delete_knowledge() で削除
```
