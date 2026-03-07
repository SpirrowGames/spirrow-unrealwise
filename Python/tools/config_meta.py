"""Config meta-tool for SpirrowBridge."""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")

COMMANDS = {
    "get_config_value": "get_config_value",
    "set_config_value": "set_config_value",
    "list_config_sections": "list_config_sections",
}


def register_config_meta_tool(mcp: FastMCP):
    """Register the config meta-tool."""

    @mcp.tool()
    def config(ctx: Context, command: str, params: Dict[str, Any] = {}) -> Dict[str, Any]:
        """Config: read/write Unreal config files (DefaultEngine, etc.).
        Commands: get_config_value, set_config_value, list_config_sections
        Use help("config", "command_name") for params.
        """
        from tools.meta_utils import execute_command
        return execute_command(COMMANDS, command, params)

    logger.info("Config meta-tool registered")
