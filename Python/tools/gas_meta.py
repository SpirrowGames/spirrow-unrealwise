"""GAS (Gameplay Ability System) meta-tool for SpirrowBridge."""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")

COMMANDS = {
    "add_gameplay_tags": "add_gameplay_tags",
    "list_gameplay_tags": "list_gameplay_tags",
    "remove_gameplay_tag": "remove_gameplay_tag",
    "list_gas_assets": "list_gas_assets",
    "create_gameplay_effect": "create_gameplay_effect",
    "create_gas_character": "create_gas_character",
    "set_ability_system_defaults": "set_ability_system_defaults",
    "create_gameplay_ability": "create_gameplay_ability",
}

RATIONALE_COMMANDS = {
    "create_gameplay_effect": "gas_effect",
    "create_gas_character": "gas_character",
    "create_gameplay_ability": "gas_ability",
}


def register_gas_meta_tool(mcp: FastMCP):
    """Register the gas meta-tool."""

    @mcp.tool()
    def gas(ctx: Context, command: str, params: Dict[str, Any] = {}) -> Dict[str, Any]:
        """GAS: gameplay tags, effects, abilities, ASC characters.
        Commands: add_gameplay_tags, list_gameplay_tags, remove_gameplay_tag,
        list_gas_assets, create_gameplay_effect, create_gas_character,
        set_ability_system_defaults, create_gameplay_ability
        Use help("gas", "command_name") for params.
        """
        from tools.meta_utils import execute_command

        # Handle None -> empty values for list/string params
        if command in ("create_gameplay_effect", "create_gas_character",
                       "set_ability_system_defaults", "create_gameplay_ability"):
            for key, val in list(params.items()):
                if val is None:
                    if key in ("modifiers", "application_tags", "removal_tags",
                               "default_abilities", "default_effects",
                               "ability_tags", "cancel_abilities_with_tags",
                               "block_abilities_with_tags", "activation_owned_tags",
                               "activation_required_tags", "activation_blocked_tags"):
                        params[key] = []
                    elif key in ("cost_effect", "cooldown_effect"):
                        params[key] = ""
                    elif key in ("duration_magnitude", "period"):
                        params[key] = 0

        # list commands: handle None path_filter/filter_prefix
        if command in ("list_gas_assets", "list_gameplay_tags"):
            for key in ("path_filter", "filter_prefix"):
                if key in params and params[key] is None:
                    params[key] = ""

        # Strip remaining None values
        params = {k: v for k, v in params.items() if v is not None}

        # comments default for add_gameplay_tags
        if command == "add_gameplay_tags" and "comments" not in params:
            params["comments"] = {}

        return execute_command(COMMANDS, command, params, RATIONALE_COMMANDS)

    logger.info("GAS meta-tool registered")
