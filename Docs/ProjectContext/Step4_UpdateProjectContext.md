# Step 4: update_project_context ツール追加

## 対象ファイル

`Python/tools/rag_tools.py`

## 追加場所

`register_rag_tools` 関数内、`get_project_context` ツールの後に追加

## 追加コード

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
    古いコンテキストを削除し、新しいものを保存する。
    
    Args:
        project_name: プロジェクト名（例: "TrapxTrapCpp"）
        current_phase: 現在のフェーズ（例: "Phase 3 - トラップ解除システム"）
        recent_changes: 最近の変更点のリスト
        known_issues: 既知の問題のリスト（オプション）
        next_tasks: 次のタスクのリスト（オプション）
        notes: 補足メモ（オプション）
    
    Returns:
        更新結果
    
    Example:
        update_project_context(
            project_name="TrapxTrapCpp",
            current_phase="Phase 3 - トラップ解除システム",
            recent_changes=["DisarmProgressWidget実装完了"],
            next_tasks=["解除キャンセル処理"]
        )
    """
    category = f"project-context:{project_name}"
    
    try:
        # 1. 既存のコンテキストを削除
        delete_project_context_internal(project_name)
        
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
        
        # 3. RAGに保存
        document = json.dumps(context_data, ensure_ascii=False, indent=2)
        
        url = f"{RAG_SERVER_URL}/knowledge/add"
        payload = {
            "document": document,
            "category": category,
            "tags": f"project,context,{project_name}"
        }
        
        response = requests.post(url, json=payload, timeout=10)
        response.raise_for_status()
        
        logger.info(f"Project context updated for {project_name}")
        
        return {
            "success": True,
            "project_name": project_name,
            "current_phase": current_phase,
            "updated_at": context_data["updated_at"],
            "message": f"Project context for '{project_name}' updated successfully"
        }
        
    except Exception as e:
        error_msg = f"Failed to update project context: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

## 確認方法

MCPサーバー再起動後、ツール一覧に `update_project_context` が表示されることを確認
