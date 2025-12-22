# Step 2: 内部関数追加

## 対象ファイル

`Python/tools/rag_tools.py`

## 追加場所

`delete_knowledge_internal` 関数の後に追加

## 追加コード

```python
def get_project_context_internal(project_name: str) -> Optional[Dict[str, Any]]:
    """
    プロジェクトコンテキストを取得する内部関数。
    カテゴリ `project-context:{project_name}` で検索し、最新1件を返す。
    """
    category = f"project-context:{project_name}"
    try:
        url = f"{RAG_SERVER_URL}/knowledge/search"
        payload = {
            "query": project_name,
            "n_results": 1,
            "category": category
        }
        
        response = requests.post(url, json=payload, timeout=10)
        response.raise_for_status()
        
        data = response.json()
        results = data.get("results", [])
        
        if results:
            doc = results[0].get("document", "")
            try:
                return json.loads(doc)
            except json.JSONDecodeError:
                logger.warning(f"Failed to parse project context as JSON: {doc}")
                return {"raw_content": doc}
        return None
        
    except Exception as e:
        logger.error(f"get_project_context_internal error: {e}")
        return None


def delete_project_context_internal(project_name: str) -> bool:
    """
    既存のプロジェクトコンテキストを削除する内部関数。
    """
    category = f"project-context:{project_name}"
    try:
        url = f"{RAG_SERVER_URL}/knowledge/search"
        payload = {
            "query": project_name,
            "n_results": 10,
            "category": category
        }
        
        response = requests.post(url, json=payload, timeout=10)
        if not response.ok:
            return False
            
        data = response.json()
        results = data.get("results", [])
        
        for result in results:
            doc_id = result.get("id")
            if doc_id:
                delete_url = f"{RAG_SERVER_URL}/knowledge/{doc_id}"
                requests.delete(delete_url, timeout=5)
                logger.info(f"Deleted old context: {doc_id}")
        
        return True
        
    except Exception as e:
        logger.error(f"delete_project_context_internal error: {e}")
        return False
```

## 確認方法

```bash
cd Python
python -c "from tools.rag_tools import get_project_context_internal, delete_project_context_internal; print('OK')"
```
