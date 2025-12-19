"""
Material creation and template management tools for SpirrowUnrealWise MCP.
"""

import json
import logging
import os
from pathlib import Path
from typing import Dict, Any, Optional, List
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("SpirrowBridge")

# テンプレートディレクトリのパス
TEMPLATES_DIR = Path(__file__).parent.parent.parent / "templates" / "materials"


def load_builtin_templates() -> Dict[str, dict]:
    """Load all builtin templates from JSON files."""
    templates = {}
    if TEMPLATES_DIR.exists():
        for json_file in TEMPLATES_DIR.glob("*.json"):
            try:
                with open(json_file, "r", encoding="utf-8") as f:
                    template = json.load(f)
                    templates[template["name"]] = template
            except Exception as e:
                logger.error(f"Error loading template {json_file}: {e}")
    return templates


def get_builtin_template(name: str) -> Optional[dict]:
    """Get a specific builtin template by name."""
    template_file = TEMPLATES_DIR / f"{name}.json"
    if template_file.exists():
        with open(template_file, "r", encoding="utf-8") as f:
            return json.load(f)
    return None


def register_material_tools(mcp: FastMCP):
    """Register all material tools with the MCP server."""

    @mcp.tool()
    async def list_material_templates(
        ctx: Context,
        source: str = "all"
    ) -> Dict[str, Any]:
        """
        List available material templates.

        Args:
            source: Filter by source - "builtin", "user", or "all" (default)

        Returns:
            Dict containing list of templates with their descriptions
        """
        result = {
            "builtin": [],
            "user": []
        }

        # Load builtin templates
        if source in ["builtin", "all"]:
            builtin = load_builtin_templates()
            for name, template in builtin.items():
                result["builtin"].append({
                    "name": name,
                    "description": template.get("description", ""),
                    "blend_mode": template.get("blend_mode", "Opaque"),
                    "shading_model": template.get("shading_model", "DefaultLit"),
                    "two_sided": template.get("two_sided", False)
                })

        # Load user templates from RAG
        if source in ["user", "all"]:
            try:
                from tools.rag_tools import search_knowledge_internal
                rag_results = search_knowledge_internal(
                    query="",
                    category="material_template",
                    n_results=100
                )
                if rag_results and "documents" in rag_results:
                    for doc in rag_results["documents"]:
                        try:
                            template = json.loads(doc)
                            result["user"].append({
                                "name": template.get("name", "unknown"),
                                "description": template.get("description", ""),
                                "blend_mode": template.get("blend_mode", "Opaque"),
                                "shading_model": template.get("shading_model", "DefaultLit"),
                                "two_sided": template.get("two_sided", False)
                            })
                        except json.JSONDecodeError:
                            pass
            except Exception as e:
                result["user_error"] = str(e)

        result["total_builtin"] = len(result["builtin"])
        result["total_user"] = len(result["user"])
        return result

    @mcp.tool()
    async def get_material_template(
        ctx: Context,
        name: str
    ) -> Dict[str, Any]:
        """
        Get a material template by name.

        Args:
            name: Template name (checks builtin first, then user-defined)

        Returns:
            Dict containing template definition or error
        """
        # Check builtin first
        builtin = get_builtin_template(name)
        if builtin:
            return {
                "success": True,
                "source": "builtin",
                "template": builtin
            }

        # Check user-defined in RAG
        try:
            from tools.rag_tools import search_knowledge_internal
            rag_results = search_knowledge_internal(
                query=name,
                category="material_template",
                n_results=10
            )
            if rag_results and "documents" in rag_results:
                for doc in rag_results["documents"]:
                    try:
                        template = json.loads(doc)
                        if template.get("name") == name:
                            return {
                                "success": True,
                                "source": "user",
                                "template": template
                            }
                    except json.JSONDecodeError:
                        pass
        except Exception as e:
            return {"success": False, "error": f"RAG search failed: {e}"}

        return {"success": False, "error": f"Template not found: {name}"}

    @mcp.tool()
    async def save_material_template(
        ctx: Context,
        name: str,
        description: str = "",
        shading_model: str = "DefaultLit",
        blend_mode: str = "Opaque",
        two_sided: bool = False,
        base_color: List[float] = None,
        opacity: float = 1.0,
        emissive_color: List[float] = None,
        emissive_strength: float = 1.0,
        tags: str = ""
    ) -> Dict[str, Any]:
        """
        Save a user-defined material template to RAG.

        Args:
            name: Template name (must be unique)
            description: Description of the template
            shading_model: DefaultLit or Unlit
            blend_mode: Opaque, Translucent, or Masked
            two_sided: Whether to render both sides
            base_color: [R, G, B] values 0.0-1.0
            opacity: Opacity value 0.0-1.0 (for Translucent)
            emissive_color: [R, G, B] emissive color
            emissive_strength: Emissive intensity multiplier
            tags: Comma-separated tags for search

        Returns:
            Dict containing success status
        """
        template = {
            "name": name,
            "description": description,
            "shading_model": shading_model,
            "blend_mode": blend_mode,
            "two_sided": two_sided,
            "parameters": {}
        }

        if base_color:
            template["parameters"]["base_color"] = base_color
        if blend_mode == "Translucent":
            template["parameters"]["opacity"] = opacity
        if emissive_color:
            template["parameters"]["emissive_color"] = emissive_color
            template["parameters"]["emissive_strength"] = emissive_strength

        try:
            from tools.rag_tools import add_knowledge_internal
            result = add_knowledge_internal(
                document=json.dumps(template),
                category="material_template",
                tags=tags,
                doc_id=f"material_template_{name}"
            )
            return {
                "success": True,
                "name": name,
                "message": f"Template '{name}' saved successfully",
                "rag_result": result
            }
        except Exception as e:
            return {"success": False, "error": str(e)}

    @mcp.tool()
    async def delete_material_template(
        ctx: Context,
        name: str
    ) -> Dict[str, Any]:
        """
        Delete a user-defined material template.

        Args:
            name: Template name to delete

        Returns:
            Dict containing success status
        """
        # Cannot delete builtin templates
        if get_builtin_template(name):
            return {
                "success": False,
                "error": f"Cannot delete builtin template: {name}"
            }

        try:
            from tools.rag_tools import delete_knowledge_internal
            result = delete_knowledge_internal(doc_id=f"material_template_{name}")
            return {
                "success": True,
                "name": name,
                "message": f"Template '{name}' deleted successfully",
                "rag_result": result
            }
        except Exception as e:
            return {"success": False, "error": str(e)}

    @mcp.tool()
    async def create_material_from_template(
        ctx: Context,
        name: str,
        template: str,
        path: str = "/Game/Materials",
        color: List[float] = None,
        opacity: float = None,
        two_sided: bool = None
    ) -> Dict[str, Any]:
        """
        Create a material from a template.

        Args:
            name: Name for the new material (e.g., "M_DetectionSphere")
            template: Template name (builtin or user-defined)
            path: Content browser path for the material
            color: Override [R, G, B] color (0.0-1.0)
            opacity: Override opacity (0.0-1.0)
            two_sided: Override two-sided setting

        Returns:
            Dict containing created material path or error

        Example:
            create_material_from_template(
                name="M_DetectionSphere",
                template="unlit_translucent",
                path="/Game/TrapxTrap/Materials",
                color=[0.2, 0.5, 1.0],
                opacity=0.3,
                two_sided=True
            )
        """
        from unreal_mcp_server import get_unreal_connection

        # Get template
        template_result = await get_material_template(ctx, template)
        if not template_result.get("success"):
            return template_result

        template_data = template_result["template"]

        # Build material parameters with overrides
        params = {
            "name": name,
            "path": path,
            "shading_model": template_data.get("shading_model", "DefaultLit"),
            "blend_mode": template_data.get("blend_mode", "Opaque"),
            "two_sided": two_sided if two_sided is not None else template_data.get("two_sided", False)
        }

        # Handle color/emissive based on shading model
        template_params = template_data.get("parameters", {})

        if params["shading_model"] == "Unlit":
            # Unlit uses emissive_color
            params["emissive_color"] = color if color else template_params.get("emissive_color", [1.0, 1.0, 1.0])
        else:
            # DefaultLit uses base_color
            params["base_color"] = color if color else template_params.get("base_color", [1.0, 1.0, 1.0])
            if template_params.get("emissive_color"):
                params["emissive_color"] = template_params["emissive_color"]
                params["emissive_strength"] = template_params.get("emissive_strength", 1.0)

        # Opacity
        if params["blend_mode"] == "Translucent":
            params["opacity"] = opacity if opacity is not None else template_params.get("opacity", 0.5)

        # Send to Unreal
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Creating material from template with params: {params}")
            response = unreal.send_command("create_simple_material", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Create material response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating material from template: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    async def create_simple_material(
        ctx: Context,
        name: str,
        path: str = "/Game/Materials",
        shading_model: str = "DefaultLit",
        blend_mode: str = "Opaque",
        two_sided: bool = False,
        base_color: List[float] = None,
        opacity: float = 1.0,
        emissive_color: List[float] = None,
        emissive_strength: float = 1.0
    ) -> Dict[str, Any]:
        """
        Create a simple material with detailed settings.

        Args:
            name: Material name (e.g., "M_MyMaterial")
            path: Content browser path (default: "/Game/Materials")
            shading_model: "DefaultLit" or "Unlit"
            blend_mode: "Opaque", "Translucent", or "Masked"
            two_sided: Whether to render both sides of faces
            base_color: [R, G, B] base color (0.0-1.0), used for DefaultLit
            opacity: Opacity value (0.0-1.0), only for Translucent blend mode
            emissive_color: [R, G, B] emissive/glow color (0.0-1.0)
            emissive_strength: Emissive intensity multiplier

        Returns:
            Dict containing created material path or error

        Example:
            create_simple_material(
                name="M_GlowingRed",
                blend_mode="Translucent",
                shading_model="Unlit",
                emissive_color=[1.0, 0.0, 0.0],
                opacity=0.5,
                two_sided=True
            )
        """
        from unreal_mcp_server import get_unreal_connection

        params = {
            "name": name,
            "path": path,
            "shading_model": shading_model,
            "blend_mode": blend_mode,
            "two_sided": two_sided,
            "opacity": opacity
        }

        if base_color:
            params["base_color"] = base_color
        if emissive_color:
            params["emissive_color"] = emissive_color
            params["emissive_strength"] = emissive_strength

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Creating simple material with params: {params}")
            response = unreal.send_command("create_simple_material", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Create simple material response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating simple material: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    logger.info("Material tools registered successfully")
