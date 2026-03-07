"""Blueprint meta-tool for SpirrowBridge."""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")

COMMANDS = {
    "create_blueprint": "create_blueprint",
    "add_component_to_blueprint": "add_component_to_blueprint",
    "set_static_mesh_properties": "set_static_mesh_properties",
    "set_component_property": "set_component_property",
    "set_physics_properties": "set_physics_properties",
    "compile_blueprint": "compile_blueprint",
    "set_blueprint_property": "set_blueprint_property",
    "duplicate_blueprint": "duplicate_blueprint",
    "get_blueprint_graph": "get_blueprint_graph",
    "scan_project_classes": "scan_project_classes",
    "set_blueprint_class_array": "set_blueprint_class_array",
    "set_struct_array_property": "set_struct_array_property",
    "create_data_asset": "create_data_asset",
    "set_class_property": "set_class_property",
    "set_object_property": "set_object_property",
    "get_blueprint_properties": "get_blueprint_properties",
    "set_struct_property": "set_struct_property",
    "set_data_asset_property": "set_data_asset_property",
    "get_data_asset_properties": "get_data_asset_properties",
    "batch_set_properties": "batch_set_properties",
    "find_cpp_function_in_blueprints": "find_function_callers",
}

RATIONALE_COMMANDS = {
    "create_blueprint": "blueprint",
    "add_component_to_blueprint": "component",
    "set_physics_properties": "physics",
}


def register_blueprint_meta_tool(mcp: FastMCP):
    """Register the blueprint meta-tool."""

    @mcp.tool()
    def blueprint(ctx: Context, command: str, params: Dict[str, Any] = {}) -> Dict[str, Any]:
        """Blueprint: create, compile, properties, components, data assets, scanning.
        Commands: create_blueprint, add_component_to_blueprint, set_static_mesh_properties,
        set_component_property, set_physics_properties, compile_blueprint,
        set_blueprint_property, duplicate_blueprint, get_blueprint_graph,
        scan_project_classes, set_blueprint_class_array, set_struct_array_property,
        create_data_asset, set_class_property, set_object_property,
        get_blueprint_properties, set_struct_property, set_data_asset_property,
        get_data_asset_properties, batch_set_properties, find_cpp_function_in_blueprints
        Use help("blueprint", "command_name") for params.
        """
        from tools.meta_utils import execute_command

        # add_component_to_blueprint: validate vector arrays
        if command == "add_component_to_blueprint":
            for key in ("location", "rotation", "scale"):
                if key in params:
                    val = params[key]
                    if not isinstance(val, list) or len(val) != 3:
                        return {"success": False, "message": f"Invalid {key}: must be [x, y, z]"}
                    params[key] = [float(v) for v in val]

        # set_physics_properties: ensure float types
        if command == "set_physics_properties":
            for key in ("mass", "linear_damping", "angular_damping"):
                if key in params:
                    params[key] = float(params[key])

        # duplicate_blueprint: default destination_path to source_path
        if command == "duplicate_blueprint":
            if "destination_path" not in params or params["destination_path"] is None:
                params["destination_path"] = params.get("source_path", "/Game/Blueprints")

        return execute_command(COMMANDS, command, params, RATIONALE_COMMANDS)

    logger.info("Blueprint meta-tool registered")
