"""
RAG Tools for Unreal MCP.

This module provides tools for interacting with an external RAG (Retrieval-Augmented Generation) server
to store and retrieve project knowledge.
"""

import logging
import os
import json
from datetime import datetime
from typing import Dict, Any, Optional, List
import requests
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("SpirrowBridge")

# Get RAG server URL from environment variable or use default
RAG_SERVER_URL = os.environ.get("RAG_SERVER_URL", "http://localhost:8100")

def record_rationale(
    action: str,
    details: Dict[str, Any],
    rationale: str,
    category: str = "decision"
) -> None:
    """
    Record design rationale to RAG server (internal function).

    Args:
        action: Action name (e.g., "create_blueprint")
        details: Action details (e.g., {"name": "BP_Enemy", "parent": "Character"})
        rationale: Design rationale
        category: RAG category (default: "decision")
    """
    if not rationale:
        return

    try:
        # Format as document
        document = f"[{action}]\n"
        document += f"Details: {json.dumps(details, ensure_ascii=False)}\n"
        document += f"Rationale: {rationale}"

        # Generate tags from action name
        tags = action.replace("_", ",")

        url = f"{RAG_SERVER_URL}/knowledge/add"
        payload = {
            "document": document,
            "category": category,
            "tags": tags
        }

        response = requests.post(url, json=payload, timeout=5)
        if response.ok:
            logger.info(f"Rationale recorded for {action}")
        else:
            logger.warning(f"Failed to record rationale: {response.status_code}")

    except Exception as e:
        # Log warning but don't fail the main operation
        logger.warning(f"Failed to record rationale: {e}")

def search_knowledge_internal(
    query: str,
    category: Optional[str] = None,
    n_results: int = 5
) -> Optional[Dict[str, Any]]:
    """
    Internal synchronous version of search_knowledge.
    Can be called from other tools without async context.

    Returns:
        Dict with 'documents', 'metadatas', 'distances' or None on error
    """
    try:
        url = f"{RAG_SERVER_URL}/knowledge/search"
        payload = {
            "query": query,
            "n_results": n_results
        }
        if category:
            payload["category"] = category

        response = requests.post(url, json=payload, timeout=10)
        response.raise_for_status()

        data = response.json()

        # Extract ChromaDB-style format
        results = data.get("results", [])
        return {
            "documents": [r.get("document", "") for r in results],
            "metadatas": [r.get("metadata", {}) for r in results],
            "distances": [r.get("distance", 0.0) for r in results]
        }

    except Exception as e:
        logger.error(f"search_knowledge_internal error: {e}")
        return None

def add_knowledge_internal(
    document: str,
    category: str,
    tags: Optional[str] = None,
    doc_id: Optional[str] = None
) -> Dict[str, Any]:
    """
    Internal synchronous version of add_knowledge.
    Can be called from other tools without async context.
    """
    try:
        url = f"{RAG_SERVER_URL}/knowledge/add"
        payload = {
            "document": document,
            "category": category
        }
        if tags:
            payload["tags"] = tags
        if doc_id:
            payload["id"] = doc_id

        response = requests.post(url, json=payload, timeout=10)
        response.raise_for_status()

        data = response.json()
        return {
            "success": True,
            "status": data.get("status", "added"),
            "id": data.get("id")
        }

    except Exception as e:
        logger.error(f"add_knowledge_internal error: {e}")
        return {"success": False, "error": str(e)}

def delete_knowledge_internal(doc_id: str) -> Dict[str, Any]:
    """
    Internal synchronous version of delete_knowledge.
    Can be called from other tools without async context.
    """
    try:
        url = f"{RAG_SERVER_URL}/knowledge/{doc_id}"

        response = requests.delete(url, timeout=10)
        response.raise_for_status()

        data = response.json()
        return {
            "success": True,
            "status": data.get("status", "deleted"),
            "id": data.get("id", doc_id)
        }

    except Exception as e:
        logger.error(f"delete_knowledge_internal error: {e}")
        return {"success": False, "error": str(e)}


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
    list_knowledge APIを使用してカテゴリでフィルタリングし、該当IDを削除。
    """
    target_category = f"project-context:{project_name}"
    try:
        # list_knowledge で全エントリを取得
        url = f"{RAG_SERVER_URL}/knowledge/list"
        response = requests.get(url, timeout=10)
        if not response.ok:
            return False
            
        data = response.json()
        items = data.get("items", [])
        
        # カテゴリが一致するエントリを削除
        deleted_count = 0
        for item in items:
            item_category = item.get("metadata", {}).get("category", "")
            if item_category == target_category:
                doc_id = item.get("id")
                if doc_id:
                    delete_url = f"{RAG_SERVER_URL}/knowledge/{doc_id}"
                    requests.delete(delete_url, timeout=5)
                    logger.info(f"Deleted old context: {doc_id}")
                    deleted_count += 1
        
        logger.info(f"Deleted {deleted_count} old context entries for {project_name}")
        return True
        
    except Exception as e:
        logger.error(f"delete_project_context_internal error: {e}")
        return False


def register_rag_tools(mcp: FastMCP):
    """Register RAG tools with the MCP server."""

    @mcp.tool()
    def search_knowledge(
        ctx: Context,
        query: str,
        n_results: int = 3,
        category: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Search the RAG knowledge base for relevant information.

        Args:
            query: Search query string
            n_results: Number of results to return (default: 3)
            category: Optional category filter (e.g., "ai", "blueprint", "gameplay")

        Returns:
            Search results with documents, metadata, and distances

        Example:
            search_knowledge("enemy behavior patterns", n_results=5, category="ai")
        """
        try:
            url = f"{RAG_SERVER_URL}/knowledge/search"
            payload = {
                "query": query,
                "n_results": n_results
            }
            if category:
                payload["category"] = category

            logger.info(f"Searching RAG knowledge base: query='{query}', n_results={n_results}, category={category}")

            response = requests.post(url, json=payload, timeout=10)
            response.raise_for_status()

            data = response.json()
            logger.info(f"RAG search returned {len(data.get('results', []))} results")

            return {
                "success": True,
                "query": data.get("query", query),
                "results": data.get("results", []),
                "count": len(data.get("results", []))
            }

        except requests.exceptions.ConnectionError as e:
            error_msg = f"Failed to connect to RAG server at {RAG_SERVER_URL}: {e}"
            logger.error(error_msg)
            return {
                "success": False,
                "message": error_msg,
                "hint": "Make sure RAG server is running and RAG_SERVER_URL is correct"
            }
        except requests.exceptions.Timeout as e:
            error_msg = f"RAG server request timed out: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
        except requests.exceptions.HTTPError as e:
            error_msg = f"RAG server returned error: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
        except Exception as e:
            error_msg = f"Error searching knowledge: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_knowledge(
        ctx: Context,
        document: str,
        category: str,
        tags: Optional[str] = None,
        doc_id: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Add a new knowledge entry to the RAG server.

        Args:
            document: The knowledge content to store
            category: Category for organization (e.g., "ai", "blueprint", "gameplay")
            tags: Optional comma-separated tags (e.g., "enemy,chase,behavior-tree")
            doc_id: Optional custom document ID

        Returns:
            Response with status and assigned document ID

        Example:
            add_knowledge(
                "Enemy AI uses Behavior Trees with chase and patrol nodes",
                category="ai",
                tags="enemy,behavior-tree,chase"
            )
        """
        try:
            url = f"{RAG_SERVER_URL}/knowledge/add"
            payload = {
                "document": document,
                "category": category
            }
            if tags:
                payload["tags"] = tags
            if doc_id:
                payload["id"] = doc_id

            logger.info(f"Adding knowledge to RAG: category='{category}', tags='{tags}'")

            response = requests.post(url, json=payload, timeout=10)
            response.raise_for_status()

            data = response.json()
            logger.info(f"Knowledge added successfully: {data}")

            return {
                "success": True,
                "status": data.get("status", "added"),
                "id": data.get("id"),
                "message": f"Knowledge added with ID: {data.get('id')}"
            }

        except requests.exceptions.ConnectionError as e:
            error_msg = f"Failed to connect to RAG server at {RAG_SERVER_URL}: {e}"
            logger.error(error_msg)
            return {
                "success": False,
                "message": error_msg,
                "hint": "Make sure RAG server is running and RAG_SERVER_URL is correct"
            }
        except requests.exceptions.Timeout as e:
            error_msg = f"RAG server request timed out: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
        except requests.exceptions.HTTPError as e:
            error_msg = f"RAG server returned error: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
        except Exception as e:
            error_msg = f"Error adding knowledge: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def list_knowledge(ctx: Context) -> Dict[str, Any]:
        """
        List all knowledge entries in the RAG server.

        Returns:
            List of all knowledge entries with their IDs, documents, and metadata

        Example:
            list_knowledge()
        """
        try:
            url = f"{RAG_SERVER_URL}/knowledge/list"

            logger.info("Listing all knowledge from RAG server")

            response = requests.get(url, timeout=10)
            response.raise_for_status()

            data = response.json()
            logger.info(f"RAG server returned {data.get('count', 0)} knowledge entries")

            return {
                "success": True,
                "count": data.get("count", 0),
                "items": data.get("items", [])
            }

        except requests.exceptions.ConnectionError as e:
            error_msg = f"Failed to connect to RAG server at {RAG_SERVER_URL}: {e}"
            logger.error(error_msg)
            return {
                "success": False,
                "message": error_msg,
                "hint": "Make sure RAG server is running and RAG_SERVER_URL is correct"
            }
        except requests.exceptions.Timeout as e:
            error_msg = f"RAG server request timed out: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
        except requests.exceptions.HTTPError as e:
            error_msg = f"RAG server returned error: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
        except Exception as e:
            error_msg = f"Error listing knowledge: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def delete_knowledge(
        ctx: Context,
        doc_id: str
    ) -> Dict[str, Any]:
        """
        Delete a knowledge entry from the RAG server.

        Args:
            doc_id: ID of the document to delete

        Returns:
            Response indicating success or failure

        Example:
            delete_knowledge("doc_123")
        """
        try:
            url = f"{RAG_SERVER_URL}/knowledge/{doc_id}"

            logger.info(f"Deleting knowledge from RAG: doc_id='{doc_id}'")

            response = requests.delete(url, timeout=10)
            response.raise_for_status()

            data = response.json()
            logger.info(f"Knowledge deleted successfully: {data}")

            return {
                "success": True,
                "status": data.get("status", "deleted"),
                "id": data.get("id", doc_id),
                "message": f"Knowledge with ID {doc_id} deleted successfully"
            }

        except requests.exceptions.ConnectionError as e:
            error_msg = f"Failed to connect to RAG server at {RAG_SERVER_URL}: {e}"
            logger.error(error_msg)
            return {
                "success": False,
                "message": error_msg,
                "hint": "Make sure RAG server is running and RAG_SERVER_URL is correct"
            }
        except requests.exceptions.Timeout as e:
            error_msg = f"RAG server request timed out: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
        except requests.exceptions.HTTPError as e:
            error_msg = f"RAG server returned error: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
        except Exception as e:
            error_msg = f"Error deleting knowledge: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

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

    logger.info(f"RAG tools registered successfully (RAG_SERVER_URL: {RAG_SERVER_URL})")
