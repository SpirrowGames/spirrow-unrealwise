"""
AI Perception Tools for Unreal MCP - AIPerceptionComponent and Sense configuration.

This module provides tools for working with Unreal Engine's AI Perception system.
Phase H-1 implementation.
"""

import logging
from typing import Dict, Any, List, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")


def register_perception_tools(mcp: FastMCP):
    """Register AI Perception tools with the MCP server."""

    @mcp.tool()
    def add_ai_perception_component(
        ctx: Context,
        blueprint_name: str,
        component_name: str = "AIPerceptionComponent",
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Add AIPerceptionComponent to a Blueprint (typically AIController).

        This component enables AI to perceive the world through various senses
        like sight, hearing, and damage detection.

        Args:
            blueprint_name: Name of the target Blueprint (usually an AIController)
            component_name: Name for the component (default: "AIPerceptionComponent")
            path: Content browser path where the Blueprint is located

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - blueprint_name: Name of the modified Blueprint
            - component_name: Name of the added component
            - component_type: "AIPerceptionComponent"

        Example:
            add_ai_perception_component(
                blueprint_name="BP_EnemyAIController",
                path="/Game/AI/Controllers"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "path": path
            }
            logger.info(f"Adding AIPerceptionComponent to {blueprint_name}")
            response = unreal.send_command("add_ai_perception_component", params)

            if response and response.get("success"):
                logger.info(f"Added AIPerceptionComponent '{component_name}' to {blueprint_name}")

            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error adding AIPerceptionComponent: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def configure_sight_sense(
        ctx: Context,
        blueprint_name: str,
        component_name: str = "AIPerceptionComponent",
        sight_radius: float = 3000.0,
        lose_sight_radius: float = 3500.0,
        peripheral_vision_angle: float = 90.0,
        detection_by_affiliation: Optional[Dict[str, bool]] = None,
        auto_success_range: float = 500.0,
        max_age: float = 5.0,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Configure Sight sense on AIPerceptionComponent.

        Sight sense detects actors that are visible within a defined cone of vision.

        Args:
            blueprint_name: Name of the Blueprint containing AIPerceptionComponent
            component_name: Name of the AIPerceptionComponent
            sight_radius: Maximum detection distance in units (default: 3000)
            lose_sight_radius: Distance at which target is lost (default: 3500)
            peripheral_vision_angle: Half-angle of vision cone in degrees (default: 90)
            detection_by_affiliation: Which actors to detect:
                - enemies: Detect hostile actors (default: True)
                - neutrals: Detect neutral actors (default: True)
                - friendlies: Detect friendly actors (default: False)
            auto_success_range: Distance for automatic detection (default: 500)
            max_age: How long perception info is retained in seconds (default: 5)
            path: Content browser path where the Blueprint is located

        Returns:
            Dict containing configured sense parameters

        Example:
            configure_sight_sense(
                blueprint_name="BP_EnemyAIController",
                sight_radius=3000.0,
                peripheral_vision_angle=75.0,
                detection_by_affiliation={"enemies": True, "neutrals": False, "friendlies": False},
                path="/Game/AI/Controllers"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "sight_radius": sight_radius,
                "lose_sight_radius": lose_sight_radius,
                "peripheral_vision_angle": peripheral_vision_angle,
                "auto_success_range": auto_success_range,
                "max_age": max_age,
                "path": path
            }

            if detection_by_affiliation:
                params["detection_by_affiliation"] = detection_by_affiliation

            logger.info(f"Configuring Sight sense on {blueprint_name}")
            response = unreal.send_command("configure_sight_sense", params)

            if response and response.get("success"):
                logger.info(f"Configured Sight sense: radius={sight_radius}, angle={peripheral_vision_angle}")

            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error configuring Sight sense: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def configure_hearing_sense(
        ctx: Context,
        blueprint_name: str,
        component_name: str = "AIPerceptionComponent",
        hearing_range: float = 3000.0,
        detection_by_affiliation: Optional[Dict[str, bool]] = None,
        max_age: float = 5.0,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Configure Hearing sense on AIPerceptionComponent.

        Hearing sense detects sounds made by actors (requires ReportNoiseEvent).

        Args:
            blueprint_name: Name of the Blueprint containing AIPerceptionComponent
            component_name: Name of the AIPerceptionComponent
            hearing_range: Maximum hearing distance in units (default: 3000)
            detection_by_affiliation: Which actors to detect:
                - enemies: Detect hostile actors (default: True)
                - neutrals: Detect neutral actors (default: True)
                - friendlies: Detect friendly actors (default: False)
            max_age: How long perception info is retained in seconds (default: 5)
            path: Content browser path where the Blueprint is located

        Returns:
            Dict containing configured sense parameters

        Example:
            configure_hearing_sense(
                blueprint_name="BP_EnemyAIController",
                hearing_range=5000.0,
                path="/Game/AI/Controllers"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "hearing_range": hearing_range,
                "max_age": max_age,
                "path": path
            }

            if detection_by_affiliation:
                params["detection_by_affiliation"] = detection_by_affiliation

            logger.info(f"Configuring Hearing sense on {blueprint_name}")
            response = unreal.send_command("configure_hearing_sense", params)

            if response and response.get("success"):
                logger.info(f"Configured Hearing sense: range={hearing_range}")

            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error configuring Hearing sense: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def configure_damage_sense(
        ctx: Context,
        blueprint_name: str,
        component_name: str = "AIPerceptionComponent",
        max_age: float = 5.0,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Configure Damage sense on AIPerceptionComponent.

        Damage sense detects when the AI receives damage, providing information
        about the damage source.

        Args:
            blueprint_name: Name of the Blueprint containing AIPerceptionComponent
            component_name: Name of the AIPerceptionComponent
            max_age: How long perception info is retained in seconds (default: 5)
            path: Content browser path where the Blueprint is located

        Returns:
            Dict containing configured sense parameters

        Example:
            configure_damage_sense(
                blueprint_name="BP_EnemyAIController",
                max_age=10.0,
                path="/Game/AI/Controllers"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "max_age": max_age,
                "path": path
            }

            logger.info(f"Configuring Damage sense on {blueprint_name}")
            response = unreal.send_command("configure_damage_sense", params)

            if response and response.get("success"):
                logger.info(f"Configured Damage sense: max_age={max_age}")

            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error configuring Damage sense: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def set_perception_dominant_sense(
        ctx: Context,
        blueprint_name: str,
        sense_type: str,
        component_name: str = "AIPerceptionComponent",
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Set the dominant sense for AIPerceptionComponent.

        The dominant sense determines which sense takes priority when multiple
        senses detect the same target.

        Args:
            blueprint_name: Name of the Blueprint containing AIPerceptionComponent
            sense_type: Type of sense to set as dominant:
                - "Sight": Visual detection
                - "Hearing": Audio detection
                - "Damage": Damage detection
            component_name: Name of the AIPerceptionComponent
            path: Content browser path where the Blueprint is located

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - dominant_sense: The sense type set as dominant

        Example:
            set_perception_dominant_sense(
                blueprint_name="BP_EnemyAIController",
                sense_type="Sight",
                path="/Game/AI/Controllers"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "sense_type": sense_type,
                "component_name": component_name,
                "path": path
            }

            logger.info(f"Setting dominant sense to {sense_type} on {blueprint_name}")
            response = unreal.send_command("set_perception_dominant_sense", params)

            if response and response.get("success"):
                logger.info(f"Set dominant sense to {sense_type}")

            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error setting dominant sense: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def add_perception_stimuli_source(
        ctx: Context,
        blueprint_name: str,
        component_name: str = "AIPerceptionStimuliSourceComponent",
        register_as_source_for: Optional[List[str]] = None,
        auto_register: bool = True,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Add AIPerceptionStimuliSourceComponent to a Blueprint.

        This component makes an actor detectable by AI perception systems.
        Add this to actors that should be detected by AI (like the player character).

        Args:
            blueprint_name: Name of the target Blueprint (e.g., player character)
            component_name: Name for the component (default: "AIPerceptionStimuliSourceComponent")
            register_as_source_for: List of sense types to be detectable by:
                - "Sight": Make visible to sight sense
                - "Hearing": Make detectable by hearing sense
                Default is ["Sight"] if not specified
            auto_register: Whether to automatically register on BeginPlay (default: True)
            path: Content browser path where the Blueprint is located

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - blueprint_name: Name of the modified Blueprint
            - component_name: Name of the added component
            - registered_senses: List of senses the actor is registered for

        Example:
            add_perception_stimuli_source(
                blueprint_name="BP_PlayerCharacter",
                register_as_source_for=["Sight", "Hearing"],
                path="/Game/Characters"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "auto_register": auto_register,
                "path": path
            }

            if register_as_source_for:
                params["register_as_source_for"] = register_as_source_for

            logger.info(f"Adding AIPerceptionStimuliSourceComponent to {blueprint_name}")
            response = unreal.send_command("add_perception_stimuli_source", params)

            if response and response.get("success"):
                senses = response.get("registered_senses", [])
                logger.info(f"Added stimuli source to {blueprint_name}, senses: {senses}")

            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error adding perception stimuli source: {e}")
            return {"success": False, "error": str(e)}
