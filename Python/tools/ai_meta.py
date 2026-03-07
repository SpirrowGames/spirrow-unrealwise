"""AI (Blackboard, BehaviorTree) meta-tool for SpirrowBridge."""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")

COMMANDS = {
    "create_blackboard": "create_blackboard",
    "add_blackboard_key": "add_blackboard_key",
    "remove_blackboard_key": "remove_blackboard_key",
    "list_blackboard_keys": "list_blackboard_keys",
    "create_behavior_tree": "create_behavior_tree",
    "set_behavior_tree_blackboard": "set_behavior_tree_blackboard",
    "get_behavior_tree_structure": "get_behavior_tree_structure",
    "add_bt_composite_node": "add_bt_composite_node",
    "add_bt_task_node": "add_bt_task_node",
    "add_bt_decorator_node": "add_bt_decorator_node",
    "add_bt_service_node": "add_bt_service_node",
    "connect_bt_nodes": "connect_bt_nodes",
    "set_bt_node_property": "set_bt_node_property",
    "delete_bt_node": "delete_bt_node",
    "list_bt_node_types": "list_bt_node_types",
    "set_bt_node_position": "set_bt_node_position",
    "auto_layout_bt": "auto_layout_bt",
    "list_bt_nodes": "list_bt_nodes",
    "list_ai_assets": "list_ai_assets",
    "detect_broken_bt_nodes": "detect_broken_bt_nodes",
    "fix_broken_bt_nodes": "delete_broken_bt_nodes",
}


def register_ai_meta_tool(mcp: FastMCP):
    """Register the ai meta-tool."""

    @mcp.tool()
    def ai(ctx: Context, command: str, params: Dict[str, Any] = {}) -> Dict[str, Any]:
        """AI: blackboards, behavior trees, BT nodes, AI asset management.
        Commands: create_blackboard, add_blackboard_key, remove_blackboard_key,
        list_blackboard_keys, create_behavior_tree, set_behavior_tree_blackboard,
        get_behavior_tree_structure, add_bt_composite_node, add_bt_task_node,
        add_bt_decorator_node, add_bt_service_node, connect_bt_nodes,
        set_bt_node_property, delete_bt_node, list_bt_node_types,
        set_bt_node_position, auto_layout_bt, list_bt_nodes, list_ai_assets,
        detect_broken_bt_nodes, fix_broken_bt_nodes
        Use help("ai", "command_name") for params.
        """
        from tools.meta_utils import execute_command

        # create_behavior_tree: convert None blackboard_name to empty string
        if command == "create_behavior_tree":
            if params.get("blackboard_name") is None:
                params["blackboard_name"] = ""

        # list_ai_assets: handle None path_filter
        if command == "list_ai_assets":
            if params.get("path_filter") is None:
                params["path_filter"] = ""

        # Strip None values for optional params
        params = {k: v for k, v in params.items() if v is not None}

        return execute_command(COMMANDS, command, params)

    logger.info("AI meta-tool registered")
