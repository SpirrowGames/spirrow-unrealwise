# 機能名: Project Context 取得/更新機能の修正

## 概要

`get_project_context` と `update_project_context` が正しく動作しない問題を修正する。
`update_project_context` は成功を返すが、`get_project_context` でデータが見つからない。

## 問題の原因

### 現在の実装（動作しない）

```python
# rag_tools.py - get_project_context_internal
def get_project_context_internal(project_name: str) -> Optional[Dict[str, Any]]:
    category = f"project-context:{project_name}"
    try:
        url = f"{RAG_SERVER_URL}/knowledge/search"
        payload = {
            "query": project_name,
            "n_results": 1,
            "category": category  # ← RAGサーバーがこのフィルタを無視している
        }
        response = requests.post(url, json=payload, timeout=10)
        # ...
```

**問題点**:
- RAGサーバーの `/knowledge/search` APIがカテゴリフィルタをサポートしていない
- 全文検索で関係ないドキュメントが返される
- 目的のproject-contextドキュメントが見つからない

### update_project_context の問題

```python
# 古いコンテキストの削除は list API で正しく動作するが
# 新規追加時にカテゴリだけで検索できないため取得に失敗する
```

## 修正内容

### 1. get_project_context_internal の修正

**ファイル**: `Python/tools/rag_tools.py`

```python
def get_project_context_internal(project_name: str) -> Optional[Dict[str, Any]]:
    """
    プロジェクトコンテキストを取得する内部関数。
    doc_id `project_context_{project_name}` で直接検索する。
    """
    target_id = f"project_context_{project_name}"
    try:
        # list API で全エントリを取得し、IDでフィルタ
        url = f"{RAG_SERVER_URL}/knowledge/list"
        response = requests.get(url, timeout=10)
        
        if not response.ok:
            logger.warning(f"Failed to list knowledge: {response.status_code}")
            return None
            
        data = response.json()
        items = data.get("items", [])
        
        # IDが一致するエントリを検索
        for item in items:
            if item.get("id") == target_id:
                doc = item.get("document", "")
                try:
                    return json.loads(doc)
                except json.JSONDecodeError:
                    logger.warning(f"Failed to parse project context as JSON: {doc}")
                    return {"raw_content": doc}
        
        return None
        
    except Exception as e:
        logger.error(f"get_project_context_internal error: {e}")
        return None
```

### 2. update_project_context ツールの修正

**ファイル**: `Python/tools/rag_tools.py`

```python
@mcp.tool()
def update_project_context(
    ctx: Context,
    project_name: str,
    current_phase: str,
    recent_changes: List[str],
    known_issues: Optional[List[str]] = None,
    next_tasks: Optional[List[str]] = None,
    notes: Optional[str] = None
) -> Dict[str, Any]:
    """
    プロジェクトコンテキストを上書き更新。
    固定のdoc_idを使用して確実に取得できるようにする。
    """
    doc_id = f"project_context_{project_name}"
    
    try:
        # 1. 既存のコンテキストを削除（IDで直接削除）
        delete_url = f"{RAG_SERVER_URL}/knowledge/{doc_id}"
        try:
            requests.delete(delete_url, timeout=5)
            logger.info(f"Deleted existing context: {doc_id}")
        except Exception:
            pass  # 存在しない場合は無視
        
        # 2. 新しいコンテキストを作成
        context_data = {
            "project_name": project_name,
            "current_phase": current_phase,
            "recent_changes": recent_changes,
            "known_issues": known_issues or [],
            "next_tasks": next_tasks or [],
            "notes": notes or "",
            "updated_at": datetime.now().isoformat()
        }
        
        # 3. RAGに保存（固定のdoc_idを使用）
        document = json.dumps(context_data, ensure_ascii=False, indent=2)
        
        url = f"{RAG_SERVER_URL}/knowledge/add"
        payload = {
            "document": document,
            "category": "project-context",
            "tags": f"project,context,{project_name}",
            "id": doc_id  # ← 固定のdoc_idを指定
        }
        
        response = requests.post(url, json=payload, timeout=10)
        response.raise_for_status()
        
        logger.info(f"Project context updated for {project_name} with id {doc_id}")
        
        return {
            "success": True,
            "project_name": project_name,
            "current_phase": current_phase,
            "updated_at": context_data["updated_at"],
            "doc_id": doc_id,
            "message": f"Project context for '{project_name}' updated successfully"
        }
        
    except Exception as e:
        error_msg = f"Failed to update project context: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### 3. delete_project_context_internal の修正

```python
def delete_project_context_internal(project_name: str) -> bool:
    """
    既存のプロジェクトコンテキストを削除する内部関数。
    doc_idで直接削除する。
    """
    doc_id = f"project_context_{project_name}"
    try:
        url = f"{RAG_SERVER_URL}/knowledge/{doc_id}"
        response = requests.delete(url, timeout=10)
        
        if response.ok:
            logger.info(f"Deleted project context: {doc_id}")
            return True
        else:
            logger.warning(f"Failed to delete project context: {response.status_code}")
            return False
        
    except Exception as e:
        logger.error(f"delete_project_context_internal error: {e}")
        return False
```

## 変更箇所まとめ

| 関数 | 変更内容 |
|------|----------|
| `get_project_context_internal` | search API → list API + IDフィルタに変更 |
| `update_project_context` | 固定のdoc_id (`project_context_{project_name}`) を使用 |
| `delete_project_context_internal` | doc_idで直接削除に簡略化 |

## 修正後のdoc_id命名規則

```
project_context_{project_name}
```

例:
- `project_context_SpirrowUnrealWise`
- `project_context_TrapxTrapCpp`

## テスト手順

### 1. 修正適用後にMCPサーバーを再起動

```bash
cd Python
# MCPサーバー再起動
```

### 2. update_project_context のテスト

```python
update_project_context(
    project_name="SpirrowUnrealWise",
    current_phase="Test Phase",
    recent_changes=["Test change 1", "Test change 2"],
    next_tasks=["Test task"],
    notes="Test note"
)
# 期待結果: success=True, doc_id="project_context_SpirrowUnrealWise"
```

### 3. get_project_context のテスト

```python
get_project_context("SpirrowUnrealWise")
# 期待結果: found=True, context に保存したデータが含まれる
```

### 4. 再度 update して上書き確認

```python
update_project_context(
    project_name="SpirrowUnrealWise",
    current_phase="Updated Phase",
    recent_changes=["Updated change"]
)

get_project_context("SpirrowUnrealWise")
# 期待結果: current_phase が "Updated Phase" に更新されている
```

### 5. list_knowledge で確認

```python
list_knowledge()
# 期待結果: id="project_context_SpirrowUnrealWise" のエントリが1つだけ存在
```

## 補足事項

### なぜカテゴリフィルタが効かないのか

RAGサーバー（simple_rag_server または同等）の `/knowledge/search` エンドポイントは、ChromaDBのセマンティック検索を使用している。`category` パラメータはメタデータとして保存されるが、検索時のフィルタリングには使用されていない可能性がある。

### 代替案: カテゴリフィルタの修正

RAGサーバー側で `where` 句を使ったメタデータフィルタリングを実装する方法もある：

```python
# RAGサーバー側の修正例
results = collection.query(
    query_texts=[query],
    n_results=n_results,
    where={"category": category} if category else None  # ← これを追加
)
```

ただし、RAGサーバーの修正が必要なため、本プロンプトでは固定doc_idによる解決策を採用。

## 優先度

高（project context機能が完全に動作していないため）

## 関連ファイル

- `Python/tools/rag_tools.py` - 修正対象
- `Python/unreal_mcp_server.py` - 変更不要
- `Docs/SessionStart_Prompt.md` - 使用方法のドキュメント（修正後に動作確認）
