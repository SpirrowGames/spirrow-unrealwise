"""AI Perception meta-tool for SpirrowBridge."""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")

COMMANDS = {
    "add_ai_perception_component": "add_ai_perception_component",
    "configure_sight_sense": "configure_sight_sense",
    "configure_hearing_sense": "configure_hearing_sense",
    "configure_damage_sense": "configure_damage_sense",
    "set_perception_dominant_sense": "set_perception_dominant_sense",
    "add_perception_stimuli_source": "add_perception_stimuli_source",
}


def register_perception_meta_tool(mcp: FastMCP):
    """Register the perception meta-tool."""

    @mcp.tool()
    def perception(ctx: Context, command: str, params: Dict[str, Any] = {}) -> Dict[str, Any]:
        """AI Perception: sight, hearing, damage senses, stimuli sources.
        Commands: add_ai_perception_component, configure_sight_sense,
        configure_hearing_sense, configure_damage_sense,
        set_perception_dominant_sense, add_perception_stimuli_source
        Use help("perception", "command_name") for params.
        """
        from tools.meta_utils import execute_command

        # Strip None values for optional params
        params = {k: v for k, v in params.items() if v is not None}

        return execute_command(COMMANDS, command, params)

    logger.info("Perception meta-tool registered")
