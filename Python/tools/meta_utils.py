"""Shared utilities for SpirrowBridge meta-tools."""

import logging
from typing import Dict, Any, Optional

logger = logging.getLogger("SpirrowBridge")


def execute_command(
    command_map: Dict[str, str],
    command: str,
    params: Dict[str, Any],
    rationale_commands: Optional[Dict[str, str]] = None
) -> Dict[str, Any]:
    """Execute a command via the Unreal bridge.

    Args:
        command_map: Maps Python command names to C++ command types
        command: The command name to execute
        params: Parameters to pass to the command
        rationale_commands: Maps command names to rationale categories (if rationale supported)

    Returns:
        Response from Unreal Engine
    """
    if command not in command_map:
        return {
            "error": f"Unknown command: {command}",
            "available_commands": list(command_map.keys()),
            "hint": 'Use help(category="<category>") to see available commands'
        }

    from unreal_mcp_server import get_unreal_connection

    # Extract rationale before sending to C++
    rationale = params.pop("rationale", None)

    try:
        unreal = get_unreal_connection()
        if not unreal:
            logger.error("Failed to connect to Unreal Engine")
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        cpp_command = command_map[command]
        logger.info(f"Executing {command} -> {cpp_command} with params: {params}")
        response = unreal.send_command(cpp_command, params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        # Record rationale if provided and operation was successful
        if (rationale and rationale_commands and command in rationale_commands
                and response.get("status") != "error"):
            try:
                from tools.rag_tools import record_rationale
                record_rationale(
                    action=command,
                    details=params,
                    rationale=rationale,
                    category=rationale_commands[command]
                )
            except Exception as e:
                logger.warning(f"Failed to record rationale: {e}")

        logger.info(f"{command} response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error executing {command}: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
