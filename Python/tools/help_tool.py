"""Help tool for SpirrowBridge meta-tools."""

import logging
from typing import Dict, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")


def register_help_tool(mcp: FastMCP):
    """Register the help tool with the MCP server."""

    @mcp.tool()
    def help(
        ctx: Context,
        category: str,
        command: Optional[str] = None
    ) -> Dict[str, Any]:
        """Get parameter docs for SpirrowBridge commands.

        Categories: editor, blueprint, blueprint_node, umg_widget, umg_layout,
        umg_variable, umg_animation, project, ai, perception, eqs, gas, material, config

        Args:
            category: Tool category name
            command: Specific command name (optional - omit for category overview)
        """
        from tools.command_schemas import COMMAND_SCHEMAS

        if category not in COMMAND_SCHEMAS:
            return {
                "error": f"Unknown category: {category}",
                "available_categories": list(COMMAND_SCHEMAS.keys())
            }

        cat_schema = COMMAND_SCHEMAS[category]

        if command is None:
            # Return category overview
            commands = {}
            for cmd_name, cmd_info in cat_schema.items():
                commands[cmd_name] = cmd_info["brief"]
            return {
                "category": category,
                "commands": commands,
                "total": len(commands),
                "usage": f'{category}(command="<command_name>", params={{...}})'
            }

        if command not in cat_schema:
            return {
                "error": f"Unknown command: {command}",
                "available_commands": list(cat_schema.keys())
            }

        cmd_info = cat_schema[command]
        return {
            "category": category,
            "command": command,
            "brief": cmd_info["brief"],
            "params": cmd_info["params"],
            "usage": f'{category}(command="{command}", params={{...}})'
        }

    logger.info("Help tool registered successfully")
