"""
RAG Tools for Unreal MCP.

This module provides tools for interacting with an external RAG (Retrieval-Augmented Generation) server
to store and retrieve project knowledge.
"""

import logging
import os
import json
from typing import Dict, Any, Optional
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

    logger.info(f"RAG tools registered successfully (RAG_SERVER_URL: {RAG_SERVER_URL})")
