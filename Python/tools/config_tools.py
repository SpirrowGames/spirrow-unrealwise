"""
Config Tools for Unreal MCP.

This module provides tools for reading and writing project configuration files (ini files).
"""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("SpirrowBridge")

def register_config_tools(mcp: FastMCP):
    """Register config tools with the MCP server."""

    @mcp.tool()
    def get_config_value(
        ctx: Context,
        section: str,
        key: str,
        config_file: str = "DefaultEngine"
    ) -> Dict[str, Any]:
        """
        Get a value from a project config file.

        Args:
            section: Config section (e.g., "/Script/EngineSettings.GameMapsSettings")
            key: Key name (e.g., "GlobalDefaultGameMode")
            config_file: Config file name - DefaultEngine, DefaultGame, DefaultEditor, DefaultInput

        Returns:
            Dict containing the config value

        Example:
            get_config_value(
                section="/Script/EngineSettings.GameMapsSettings",
                key="GlobalDefaultGameMode"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "config_file": config_file,
                "section": section,
                "key": key
            }

            logger.info(f"Getting config value [{section}] {key} from {config_file}")
            response = unreal.send_command("get_config_value", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Config value: {response.get('value', 'N/A')}")
            return response

        except Exception as e:
            error_msg = f"Error getting config value: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_config_value(
        ctx: Context,
        section: str,
        key: str,
        value: str,
        config_file: str = "DefaultEngine"
    ) -> Dict[str, Any]:
        """
        Set a value in a project config file.

        Args:
            section: Config section (e.g., "/Script/EngineSettings.GameMapsSettings")
            key: Key name (e.g., "GlobalDefaultGameMode")
            value: Value to set
            config_file: Config file name - DefaultEngine, DefaultGame, DefaultEditor, DefaultInput

        Returns:
            Dict containing success status

        Example:
            set_config_value(
                section="/Script/EngineSettings.GameMapsSettings",
                key="GlobalDefaultGameMode",
                value="/Game/MyGame/BP_GameMode.BP_GameMode_C"
            )

        Note:
            - Blueprint classes must end with _C suffix
            - Some settings require editor restart to take effect
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "config_file": config_file,
                "section": section,
                "key": key,
                "value": value
            }

            logger.info(f"Setting config value [{section}] {key} = {value} in {config_file}")
            response = unreal.send_command("set_config_value", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Config value set successfully: {response.get('file_path', 'N/A')}")
            return response

        except Exception as e:
            error_msg = f"Error setting config value: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def list_config_sections(
        ctx: Context,
        config_file: str = "DefaultEngine"
    ) -> Dict[str, Any]:
        """
        List all sections in a config file.

        Args:
            config_file: Config file name - DefaultEngine, DefaultGame, DefaultEditor, DefaultInput

        Returns:
            Dict containing list of section names

        Example:
            list_config_sections(config_file="DefaultEngine")
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "config_file": config_file
            }

            logger.info(f"Listing config sections in {config_file}")
            response = unreal.send_command("list_config_sections", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            section_count = response.get('count', 0)
            logger.info(f"Found {section_count} sections in {config_file}")
            return response

        except Exception as e:
            error_msg = f"Error listing config sections: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
