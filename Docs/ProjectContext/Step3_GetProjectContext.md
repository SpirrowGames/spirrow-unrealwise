# Step 3: get_project_context ツール追加

## 対象ファイル

`Python/tools/rag_tools.py`

## 追加場所

`register_rag_tools` 関数内、`delete_knowledge` ツールの後に追加

## 追加コード

```python
@mcp.tool()
def get_project_context(
    ctx: Context,
    project_name: str
) -> Dict[str, Any]:
    """
    プロジェクトの現状サマリを取得。
    会話開始時に呼び出して文脈を復元する。
    
    Args:
        project_name: プロジェクト名（例: "TrapxTrapCpp"）
    
    Returns:
        プロジェクトコンテキスト（存在しない場合はnot_foundを返す）
    
    Example:
        get_project_context("TrapxTrapCpp")
    """
    try:
        context = get_project_context_internal(project_name)
        
        if context:
            logger.info(f"Project context retrieved for {project_name}")
            return {
                "success": True,
                "found": True,
                "context": context
            }
        else:
            logger.info(f"No project context found for {project_name}")
            return {
                "success": True,
                "found": False,
                "message": f"No context found for project '{project_name}'. Use update_project_context to create one."
            }
            
    except Exception as e:
        error_msg = f"Failed to get project context: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

## 確認方法

MCPサーバー再起動後、ツール一覧に `get_project_context` が表示されることを確認
