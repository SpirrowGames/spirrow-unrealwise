"""Editor meta-tool for SpirrowBridge."""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")

# Python command name -> C++ command type
COMMANDS = {
    "get_actors_in_level": "get_actors_in_level",
    "find_actors_by_name": "find_actors_by_name",
    "spawn_actor": "spawn_actor",
    "delete_actor": "delete_actor",
    "set_actor_transform": "set_actor_transform",
    "get_actor_properties": "get_actor_properties",
    "set_actor_property": "set_actor_property",
    "set_actor_component_property": "set_actor_property",  # reuses same C++ command
    "get_actor_components": "get_actor_components",
    "rename_actor": "rename_actor",
    "spawn_blueprint_actor": "spawn_blueprint_actor",
    "rename_asset": "rename_asset",
    "create_level": "create_level",
    "save_current_level": "save_current_level",
    "open_level": "open_level",
}

RATIONALE_COMMANDS = {
    "spawn_actor": "level_design",
    "set_actor_property": "actor_property",
    "set_actor_component_property": "component_property",
}


def register_editor_meta_tool(mcp: FastMCP):
    """Register the editor meta-tool."""

    @mcp.tool()
    def editor(ctx: Context, command: str, params: Dict[str, Any] = {}) -> Dict[str, Any]:
        """Editor: actors, transforms, properties, components, assets, levels.
        Commands: get_actors_in_level, find_actors_by_name, spawn_actor, delete_actor,
        set_actor_transform, get_actor_properties, set_actor_property,
        set_actor_component_property, get_actor_components, rename_actor,
        spawn_blueprint_actor, rename_asset, create_level, save_current_level, open_level

        Level lifecycle:
        - create_level: create a new .umap and switch the editor to it
        - save_current_level: persist the currently-open level to disk
        - open_level: load an existing .umap into the editor (call save_current_level first if dirty)
        Use help("editor", "command_name") for params.
        """
        from tools.meta_utils import execute_command

        # spawn_actor / spawn_blueprint_actor: validate location/rotation arrays
        if command in ("spawn_actor", "spawn_blueprint_actor"):
            for key in ("location", "rotation"):
                if key in params:
                    val = params[key]
                    if not isinstance(val, list) or len(val) != 3:
                        return {"success": False, "message": f"Invalid {key}: must be [x, y, z]"}
                    params[key] = [float(v) for v in val]

        # spawn_actor: uppercase type
        if command == "spawn_actor" and "type" in params:
            params["type"] = params["type"].upper()

        # set_actor_component_property: remap param names for C++ command
        if command == "set_actor_component_property":
            if "actor_name" in params:
                params["name"] = params.pop("actor_name")

        return execute_command(COMMANDS, command, params, RATIONALE_COMMANDS)

    logger.info("Editor meta-tool registered")
