"""Blueprint node meta-tool for SpirrowBridge."""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")

COMMANDS = {
    "add_blueprint_event_node": "add_blueprint_event_node",
    "add_blueprint_input_action_node": "add_blueprint_input_action_node",
    "add_blueprint_function_node": "add_blueprint_function_node",
    "connect_blueprint_nodes": "connect_blueprint_nodes",
    "add_blueprint_variable": "add_blueprint_variable",
    "add_blueprint_get_self_component_reference": "add_blueprint_get_self_component_reference",
    "add_blueprint_self_reference": "add_blueprint_self_reference",
    "find_blueprint_nodes": "find_blueprint_nodes",
    "set_node_pin_value": "set_node_pin_value",
    "add_variable_get_node": "add_variable_get_node",
    "add_variable_set_node": "add_variable_set_node",
    "add_branch_node": "add_branch_node",
    "delete_blueprint_node": "delete_node",
    "move_blueprint_node": "move_node",
    "add_sequence_node": "add_sequence_node",
    "add_delay_node": "add_delay_node",
    "add_foreach_loop_node": "add_foreach_loop_node",  # deprecated
    "add_forloop_with_break_node": "add_forloop_with_break_node",
    "add_print_string_node": "add_print_string_node",
    "add_math_node": "add_math_node",
    "add_comparison_node": "add_comparison_node",
    "add_external_property_set_node": "add_external_property_set_node",
    "add_external_property_get_node": "add_external_property_get_node",
    "add_get_subsystem_node": "add_get_subsystem_node",
}

RATIONALE_COMMANDS = {
    "add_blueprint_event_node": "blueprint_event",
    "add_blueprint_function_node": "blueprint_function",
    "add_blueprint_variable": "blueprint_variable",
}


def register_node_meta_tool(mcp: FastMCP):
    """Register the blueprint_node meta-tool."""

    @mcp.tool()
    def blueprint_node(ctx: Context, command: str, params: Dict[str, Any] = {}) -> Dict[str, Any]:
        """Blueprint nodes: events, functions, variables, flow control, math.
        Commands: add_blueprint_event_node, add_blueprint_input_action_node,
        add_blueprint_function_node, connect_blueprint_nodes, add_blueprint_variable,
        add_blueprint_get_self_component_reference, add_blueprint_self_reference,
        find_blueprint_nodes, set_node_pin_value, add_variable_get_node,
        add_variable_set_node, add_branch_node, delete_blueprint_node,
        move_blueprint_node, add_sequence_node, add_delay_node,
        add_forloop_with_break_node, add_print_string_node,
        add_math_node, add_comparison_node,
        add_external_property_set_node, add_external_property_get_node,
        add_get_subsystem_node

        Level Blueprint support: every command in this tool accepts optional
        target_type="level_blueprint" to edit the current level's Level Script
        Blueprint instead of a regular Blueprint. blueprint_name is ignored in
        that mode. Optional level_path selects a specific level asset (e.g.
        "/Game/Maps/MyMap"); omit for the currently edited level.

        External UPROPERTY Set/Get: add_external_property_set_node /
        add_external_property_get_node spawn UK2Node_VariableSet/Get with an
        *external* member reference — use these (or pass a "Set<PropName>" /
        "Get<PropName>" function_name to add_blueprint_function_node which
        will transparently fall back) to set/read a UPROPERTY on another
        object such as a Subsystem's BlueprintReadWrite field.

        Typed subsystem access: add_get_subsystem_node spawns a
        UK2Node_GetSubsystem (or Engine/LocalPlayer variant) with the target
        subsystem class baked in at construction. The ReturnValue pin is
        strongly typed (e.g. VoxelCollapseSubsystem*), so downstream Cast
        nodes are unnecessary. Prefer this over routing through
        SubsystemBlueprintLibrary.GetGameInstanceSubsystem + Cast.

        Class/Object pin defaults: set_node_pin_value now handles Class,
        SoftClass, Object, SoftObject and Interface pins. Pass the class
        path (e.g. "/Script/VoxelRuntime.VoxelCollapseSubsystem") or bare
        class name for Class pins; asset path for Object pins. The handler
        uses K2Schema->TrySetDefaultObject and reconstructs the node so
        downstream pin types are narrowed correctly.

        Use help("blueprint_node", "command_name") for params.
        """
        # Handle deprecated command
        if command == "add_foreach_loop_node":
            return {
                "success": False,
                "message": "add_foreach_loop_node is deprecated. Use add_forloop_with_break_node instead."
            }

        from tools.meta_utils import execute_command

        # set_node_pin_value: convert bool to lowercase string
        if command == "set_node_pin_value" and "pin_value" in params:
            val = params["pin_value"]
            if isinstance(val, bool):
                params["pin_value"] = "true" if val else "false"
            else:
                params["pin_value"] = str(val)

        return execute_command(COMMANDS, command, params, RATIONALE_COMMANDS)

    logger.info("Blueprint node meta-tool registered")
