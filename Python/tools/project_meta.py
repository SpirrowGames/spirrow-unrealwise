"""Project meta-tool for SpirrowBridge."""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")

COMMANDS = {
    "create_input_mapping": "create_input_mapping",
    "create_input_action": "create_input_action",
    "create_input_mapping_context": "create_input_mapping_context",
    "add_action_to_mapping_context": "add_action_to_mapping_context",
    "delete_asset": "delete_asset",
    "add_mapping_context_to_blueprint": "add_mapping_context_to_blueprint",
    "set_default_mapping_context": "set_default_mapping_context",
    "asset_exists": "asset_exists",
    "create_content_folder": "create_content_folder",
    "list_assets_in_folder": "list_assets_in_folder",
    "import_texture": "import_texture",
    "get_project_info": "get_project_info",
    "find_asset_references": "find_asset_references",
}


def register_project_meta_tool(mcp: FastMCP):
    """Register the project meta-tool."""

    @mcp.tool()
    def project(ctx: Context, command: str, params: Dict[str, Any] = {}) -> Dict[str, Any]:
        """Project: input mapping, assets, folders, textures, project info.
        Commands: create_input_mapping, create_input_action,
        create_input_mapping_context, add_action_to_mapping_context,
        delete_asset, add_mapping_context_to_blueprint,
        set_default_mapping_context, asset_exists, create_content_folder,
        list_assets_in_folder, import_texture, get_project_info,
        find_asset_references
        Use help("project", "command_name") for params.
        """
        from tools.meta_utils import execute_command

        # import_texture: handle connection errors specially
        if command == "import_texture":
            from unreal_mcp_server import get_unreal_connection
            try:
                unreal = get_unreal_connection()
                if not unreal:
                    return {"success": False, "message": "Failed to connect to Unreal Engine"}
                response = unreal.send_command("import_texture", params)
                if not response:
                    return {"success": False, "message": "No response from Unreal Engine"}
                return response
            except (ConnectionResetError, BrokenPipeError) as e:
                logger.warning(f"Connection reset during import_texture (may still succeed): {e}")
                return {
                    "success": True,
                    "message": "Import command sent. Connection was reset (this is normal for large imports). Check Unreal Editor to verify."
                }
            except Exception as e:
                return {"success": False, "message": f"Error importing texture: {e}"}

        return execute_command(COMMANDS, command, params)

    logger.info("Project meta-tool registered")
