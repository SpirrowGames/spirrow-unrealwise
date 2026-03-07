"""Material meta-tool for SpirrowBridge.

NOTE: Material tools are async because some use RAG operations.
4 of 6 commands are RAG-only (no C++ bridge), 2 use C++ bridge.
"""

import json
import logging
from typing import Dict, Any, Optional, List
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")

# Only commands that go through C++ bridge
CPP_COMMANDS = {
    "create_material_from_template": "create_simple_material",
    "create_simple_material": "create_simple_material",
}

# Commands handled via RAG (no C++ bridge)
RAG_COMMANDS = {
    "list_material_templates",
    "get_material_template",
    "save_material_template",
    "delete_material_template",
}

ALL_COMMANDS = set(CPP_COMMANDS.keys()) | RAG_COMMANDS


def register_material_meta_tool(mcp: FastMCP):
    """Register the material meta-tool."""

    # Import template helpers from material_tools
    from pathlib import Path
    templates_dir = Path(__file__).parent.parent.parent / "templates" / "materials"

    def _load_builtin_templates() -> Dict[str, dict]:
        templates = {}
        if templates_dir.exists():
            for json_file in templates_dir.glob("*.json"):
                try:
                    with open(json_file, "r", encoding="utf-8") as f:
                        template = json.load(f)
                        templates[template["name"]] = template
                except Exception as e:
                    logger.error(f"Error loading template {json_file}: {e}")
        return templates

    def _get_builtin_template(name: str) -> Optional[dict]:
        template_file = templates_dir / f"{name}.json"
        if template_file.exists():
            with open(template_file, "r", encoding="utf-8") as f:
                return json.load(f)
        return None

    @mcp.tool()
    async def material(ctx: Context, command: str, params: Dict[str, Any] = {}) -> Dict[str, Any]:
        """Material: templates, creation from templates, simple material creation.
        Commands: list_material_templates, get_material_template,
        save_material_template, delete_material_template,
        create_material_from_template, create_simple_material
        Use help("material", "command_name") for params.
        """
        if command not in ALL_COMMANDS:
            return {
                "error": f"Unknown command: {command}",
                "available_commands": sorted(ALL_COMMANDS),
            }

        # ── RAG-only commands ──
        if command == "list_material_templates":
            source = params.get("source", "all")
            result = {"builtin": [], "user": []}
            if source in ("builtin", "all"):
                builtin = _load_builtin_templates()
                for name, tmpl in builtin.items():
                    result["builtin"].append({
                        "name": name,
                        "description": tmpl.get("description", ""),
                        "blend_mode": tmpl.get("blend_mode", "Opaque"),
                        "shading_model": tmpl.get("shading_model", "DefaultLit"),
                        "two_sided": tmpl.get("two_sided", False),
                    })
            if source in ("user", "all"):
                try:
                    from tools.rag_tools import search_knowledge_internal
                    rag_results = search_knowledge_internal(query="", category="material_template", n_results=100)
                    if rag_results and "documents" in rag_results:
                        for doc in rag_results["documents"]:
                            try:
                                tmpl = json.loads(doc)
                                result["user"].append({
                                    "name": tmpl.get("name", "unknown"),
                                    "description": tmpl.get("description", ""),
                                    "blend_mode": tmpl.get("blend_mode", "Opaque"),
                                    "shading_model": tmpl.get("shading_model", "DefaultLit"),
                                    "two_sided": tmpl.get("two_sided", False),
                                })
                            except json.JSONDecodeError:
                                pass
                except Exception as e:
                    result["user_error"] = str(e)
            result["total_builtin"] = len(result["builtin"])
            result["total_user"] = len(result["user"])
            return result

        if command == "get_material_template":
            name = params.get("name", "")
            builtin = _get_builtin_template(name)
            if builtin:
                return {"success": True, "source": "builtin", "template": builtin}
            try:
                from tools.rag_tools import search_knowledge_internal
                rag_results = search_knowledge_internal(query=name, category="material_template", n_results=10)
                if rag_results and "documents" in rag_results:
                    for doc in rag_results["documents"]:
                        try:
                            tmpl = json.loads(doc)
                            if tmpl.get("name") == name:
                                return {"success": True, "source": "user", "template": tmpl}
                        except json.JSONDecodeError:
                            pass
            except Exception as e:
                return {"success": False, "error": f"RAG search failed: {e}"}
            return {"success": False, "error": f"Template not found: {name}"}

        if command == "save_material_template":
            name = params.get("name", "")
            template = {
                "name": name,
                "description": params.get("description", ""),
                "shading_model": params.get("shading_model", "DefaultLit"),
                "blend_mode": params.get("blend_mode", "Opaque"),
                "two_sided": params.get("two_sided", False),
                "parameters": {},
            }
            if params.get("base_color"):
                template["parameters"]["base_color"] = params["base_color"]
            if template["blend_mode"] == "Translucent":
                template["parameters"]["opacity"] = params.get("opacity", 1.0)
            if params.get("emissive_color"):
                template["parameters"]["emissive_color"] = params["emissive_color"]
                template["parameters"]["emissive_strength"] = params.get("emissive_strength", 1.0)
            try:
                from tools.rag_tools import add_knowledge_internal
                result = add_knowledge_internal(
                    document=json.dumps(template),
                    category="material_template",
                    tags=params.get("tags", ""),
                    doc_id=f"material_template_{name}",
                )
                return {"success": True, "name": name, "message": f"Template '{name}' saved", "rag_result": result}
            except Exception as e:
                return {"success": False, "error": str(e)}

        if command == "delete_material_template":
            name = params.get("name", "")
            if _get_builtin_template(name):
                return {"success": False, "error": f"Cannot delete builtin template: {name}"}
            try:
                from tools.rag_tools import delete_knowledge_internal
                result = delete_knowledge_internal(doc_id=f"material_template_{name}")
                return {"success": True, "name": name, "message": f"Template '{name}' deleted", "rag_result": result}
            except Exception as e:
                return {"success": False, "error": str(e)}

        # ── C++ bridge commands ──
        if command == "create_material_from_template":
            template_name = params.get("template", "")
            # Get template data
            tmpl_result = await material(ctx, "get_material_template", {"name": template_name})
            if not tmpl_result.get("success"):
                return tmpl_result
            template_data = tmpl_result["template"]
            template_params = template_data.get("parameters", {})

            cpp_params = {
                "name": params.get("name", ""),
                "path": params.get("path", "/Game/Materials"),
                "shading_model": template_data.get("shading_model", "DefaultLit"),
                "blend_mode": template_data.get("blend_mode", "Opaque"),
                "two_sided": params.get("two_sided") if params.get("two_sided") is not None else template_data.get("two_sided", False),
            }
            color = params.get("color")
            if cpp_params["shading_model"] == "Unlit":
                cpp_params["emissive_color"] = color if color else template_params.get("emissive_color", [1, 1, 1])
            else:
                cpp_params["base_color"] = color if color else template_params.get("base_color", [1, 1, 1])
                if template_params.get("emissive_color"):
                    cpp_params["emissive_color"] = template_params["emissive_color"]
                    cpp_params["emissive_strength"] = template_params.get("emissive_strength", 1.0)
            if cpp_params["blend_mode"] == "Translucent":
                cpp_params["opacity"] = params.get("opacity") if params.get("opacity") is not None else template_params.get("opacity", 0.5)

            from unreal_mcp_server import get_unreal_connection
            try:
                unreal = get_unreal_connection()
                if not unreal:
                    return {"success": False, "message": "Failed to connect to Unreal Engine"}
                response = unreal.send_command("create_simple_material", cpp_params)
                return response or {"success": False, "message": "No response from Unreal Engine"}
            except Exception as e:
                return {"success": False, "message": f"Error: {e}"}

        if command == "create_simple_material":
            cpp_params = {
                "name": params.get("name", ""),
                "path": params.get("path", "/Game/Materials"),
                "shading_model": params.get("shading_model", "DefaultLit"),
                "blend_mode": params.get("blend_mode", "Opaque"),
                "two_sided": params.get("two_sided", False),
                "opacity": params.get("opacity", 1.0),
            }
            if params.get("base_color"):
                cpp_params["base_color"] = params["base_color"]
            if params.get("emissive_color"):
                cpp_params["emissive_color"] = params["emissive_color"]
                cpp_params["emissive_strength"] = params.get("emissive_strength", 1.0)

            from unreal_mcp_server import get_unreal_connection
            try:
                unreal = get_unreal_connection()
                if not unreal:
                    return {"success": False, "message": "Failed to connect to Unreal Engine"}
                response = unreal.send_command("create_simple_material", cpp_params)
                return response or {"success": False, "message": "No response from Unreal Engine"}
            except Exception as e:
                return {"success": False, "message": f"Error: {e}"}

    logger.info("Material meta-tool registered")
