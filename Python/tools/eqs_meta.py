"""EQS (Environment Query System) meta-tool for SpirrowBridge."""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")

COMMANDS = {
    "create_eqs_query": "create_eqs_query",
    "add_eqs_generator": "add_eqs_generator",
    "add_eqs_test": "add_eqs_test",
    "set_eqs_test_property": "set_eqs_test_property",
    "list_eqs_assets": "list_eqs_assets",
}


def register_eqs_meta_tool(mcp: FastMCP):
    """Register the eqs meta-tool."""

    @mcp.tool()
    def eqs(ctx: Context, command: str, params: Dict[str, Any] = {}) -> Dict[str, Any]:
        """EQS: create queries, add generators and tests.
        Commands: create_eqs_query, add_eqs_generator, add_eqs_test,
        set_eqs_test_property, list_eqs_assets
        Use help("eqs", "command_name") for params.
        """
        from tools.meta_utils import execute_command

        # Strip None values for optional params
        params = {k: v for k, v in params.items() if v is not None}

        return execute_command(COMMANDS, command, params)

    logger.info("EQS meta-tool registered")
