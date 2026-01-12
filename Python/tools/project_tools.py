"""
Project Tools for Unreal MCP.

This module provides tools for managing project-wide settings and configuration.
"""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("SpirrowBridge")

def register_project_tools(mcp: FastMCP):
    """Register project tools with the MCP server."""
    
    @mcp.tool()
    def create_input_mapping(
        ctx: Context,
        action_name: str,
        key: str,
        input_type: str = "Action"
    ) -> Dict[str, Any]:
        """
        Create an input mapping for the project.
        
        Args:
            action_name: Name of the input action
            key: Key to bind (SpaceBar, LeftMouseButton, etc.)
            input_type: Type of input mapping (Action or Axis)
            
        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "action_name": action_name,
                "key": key,
                "input_type": input_type
            }
            
            logger.info(f"Creating input mapping '{action_name}' with key '{key}'")
            response = unreal.send_command("create_input_mapping", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Input mapping creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error creating input mapping: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def create_input_action(
        ctx: Context,
        action_name: str,
        value_type: str = "Digital",
        path: str = "/Game/Input"
    ) -> Dict[str, Any]:
        """
        Create an Enhanced Input Action asset.

        Args:
            action_name: Name of the input action (e.g., "IA_Jump")
            value_type: Type of input value - Digital, Axis1D, Axis2D, Axis3D
            path: Content browser path for the asset

        Returns:
            Dict containing the created asset path
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "action_name": action_name,
                "value_type": value_type,
                "path": path
            }

            logger.info(f"Creating input action '{action_name}' with value type '{value_type}' at '{path}'")
            response = unreal.send_command("create_input_action", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Input action creation response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating input action: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def create_input_mapping_context(
        ctx: Context,
        context_name: str,
        path: str = "/Game/Input"
    ) -> Dict[str, Any]:
        """
        Create an Enhanced Input Mapping Context asset.

        Args:
            context_name: Name of the mapping context (e.g., "IMC_Default")
            path: Content browser path for the asset

        Returns:
            Dict containing the created asset path
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "context_name": context_name,
                "path": path
            }

            logger.info(f"Creating input mapping context '{context_name}' at '{path}'")
            response = unreal.send_command("create_input_mapping_context", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Input mapping context creation response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating input mapping context: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_action_to_mapping_context(
        ctx: Context,
        context_name: str,
        action_name: str,
        key: str,
        trigger_type: str = "Down",
        modifiers: list = None,
        context_path: str = "/Game/Input",
        action_path: str = "/Game/Input"
    ) -> Dict[str, Any]:
        """
        Add an Input Action mapping to an Input Mapping Context.

        Args:
            context_name: Name of the mapping context
            action_name: Name of the input action to map
            key: Key to bind (SpaceBar, LeftShift, W, A, S, D, MouseX, MouseY, etc.)
            trigger_type: Trigger type - Down, Pressed, Released, Hold, HoldAndRelease
            modifiers: List of modifiers - Negate, SwizzleYXZ, DeadZone, etc.
            context_path: Path where the IMC asset is located
            action_path: Path where the IA asset is located

        Returns:
            Dict containing mapping result
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "context_name": context_name,
                "action_name": action_name,
                "key": key,
                "trigger_type": trigger_type,
                "modifiers": modifiers or [],
                "context_path": context_path,
                "action_path": action_path
            }

            logger.info(f"Adding action '{action_name}' to mapping context '{context_name}' with key '{key}'")
            response = unreal.send_command("add_action_to_mapping_context", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Action mapping response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding action to mapping context: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def delete_asset(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """
        Delete an asset from the Content Browser.

        Args:
            asset_path: Full path to the asset (e.g., "/Game/Input/IA_TestJump")

        Returns:
            Dict containing the deleted asset path or error
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "asset_path": asset_path
            }

            logger.info(f"Deleting asset '{asset_path}'")
            response = unreal.send_command("delete_asset", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Asset deletion response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error deleting asset: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_mapping_context_to_blueprint(
        ctx: Context,
        blueprint_name: str,
        context_name: str,
        priority: int = 0,
        context_path: str = "/Game/Input",
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Add AddMappingContext nodes to a Blueprint's BeginPlay.

        Creates the following node chain:
        BeginPlay → GetController → CastToPlayerController →
        GetEnhancedInputLocalPlayerSubsystem → AddMappingContext

        Args:
            blueprint_name: Name of the target Blueprint
            context_name: Name of the InputMappingContext (e.g., "IMC_Default")
            priority: Mapping context priority (default: 0)
            context_path: Path where the IMC asset is located
            path: Content browser path where the blueprint is located

        Returns:
            Dict containing success status, created node IDs, and message

        Example:
            add_mapping_context_to_blueprint(
                blueprint_name="BP_PlayerCharacter",
                context_name="IMC_Default",
                priority=0,
                context_path="/Game/TrapxTrap/Input",
                path="/Game/TrapxTrap/Blueprints/Characters"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "context_name": context_name,
                "priority": priority,
                "context_path": context_path,
                "path": path
            }

            logger.info(f"Adding mapping context '{context_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_mapping_context_to_blueprint", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Add mapping context response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding mapping context to blueprint: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_default_mapping_context(
        ctx: Context,
        blueprint_name: str,
        context_name: str,
        priority: int = 0,
        context_path: str = "/Game/Input",
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Set the default Input Mapping Context for a PlayerController or Character Blueprint.

        For PlayerController: Sets DefaultMappingContexts property directly
        For Character/Pawn: Uses add_mapping_context_to_blueprint internally

        Args:
            blueprint_name: Name of the target Blueprint (PlayerController or Character)
            context_name: Name of the InputMappingContext (e.g., "IMC_Default")
            priority: Mapping context priority (default: 0)
            context_path: Path where the IMC asset is located
            path: Content browser path where the blueprint is located

        Returns:
            Dict containing success status and message

        Example:
            set_default_mapping_context(
                blueprint_name="BP_PlayerController",
                context_name="IMC_Default",
                context_path="/Game/TrapxTrap/Input",
                path="/Game/TrapxTrap/Blueprints/Controllers"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "context_name": context_name,
                "priority": priority,
                "context_path": context_path,
                "path": path
            }

            logger.info(f"Setting default mapping context '{context_name}' for blueprint '{blueprint_name}'")
            response = unreal.send_command("set_default_mapping_context", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set default mapping context response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting default mapping context: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    # ===== Asset Utility Tools =====

    @mcp.tool()
    def asset_exists(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """
        Check if an asset exists at the specified path.

        Args:
            asset_path: Full content browser path to the asset
                        (e.g., "/Game/Textures/T_Icon.T_Icon")

        Returns:
            Dict with 'exists' boolean indicating if the asset exists

        Example:
            asset_exists(asset_path="/Game/Data/Weapons/DA_Pistol.DA_Pistol")
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {"asset_path": asset_path}

            logger.info(f"Checking if asset exists: '{asset_path}'")
            response = unreal.send_command("asset_exists", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Asset exists response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error checking asset existence: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def create_content_folder(
        ctx: Context,
        folder_path: str
    ) -> Dict[str, Any]:
        """
        Create a folder in the Content Browser.

        Args:
            folder_path: Full content path for the folder
                         (e.g., "/Game/Data/Weapons")

        Returns:
            Dict with success status

        Note:
            Creates all intermediate folders if they don't exist.

        Example:
            create_content_folder(folder_path="/Game/TrapxTrap/UI/Icons")
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {"folder_path": folder_path}

            logger.info(f"Creating content folder: '{folder_path}'")
            response = unreal.send_command("create_content_folder", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Create folder response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating content folder: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def list_assets_in_folder(
        ctx: Context,
        folder_path: str,
        class_filter: str = None,
        recursive: bool = False
    ) -> Dict[str, Any]:
        """
        List all assets in a content browser folder.

        Args:
            folder_path: Content browser path (e.g., "/Game/Textures")
            class_filter: Filter by asset class name (e.g., "Texture2D", "Blueprint", "DataAsset")
            recursive: Include assets in subfolders (default: False)

        Returns:
            Dict with list of assets containing name, path, and class for each

        Example:
            list_assets_in_folder(
                folder_path="/Game/Data/Weapons",
                class_filter="DataAsset",
                recursive=True
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "folder_path": folder_path,
                "recursive": recursive
            }
            if class_filter:
                params["class_filter"] = class_filter

            logger.info(f"Listing assets in folder: '{folder_path}'")
            response = unreal.send_command("list_assets_in_folder", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"List assets response: found {response.get('count', 0)} assets")
            return response

        except Exception as e:
            error_msg = f"Error listing assets in folder: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def import_texture(
        ctx: Context,
        source: str,
        asset_name: str,
        destination_path: str = "/Game/Textures",
        source_type: str = "file",
        compression: str = "Default",
        srgb: bool = True,
        lod_group: str = "World"
    ) -> Dict[str, Any]:
        """
        Import a texture file into the Content Browser.

        Args:
            source: File path (for source_type="file") or Base64 string (for source_type="base64")
            asset_name: Name for the imported texture asset (e.g., "T_Icon_Weapon")
            destination_path: Content browser path (default: "/Game/Textures")
            source_type: "file" or "base64" (default: "file")
            compression: Compression setting - "Default", "Normalmap", "Masks", "Grayscale", "UI", "BC7"
            srgb: Whether texture is sRGB (default: True, set False for normal maps)
            lod_group: LOD group - "World", "WorldNormalMap", "UI", "Lightmap", "Shadowmap"

        Returns:
            Dict with success status and imported asset path

        Example:
            # Import from file
            import_texture(
                source="C:/Assets/weapon_icon.png",
                asset_name="T_Icon_Pistol",
                destination_path="/Game/Textures/UI"
            )

            # Import normal map
            import_texture(
                source="C:/Assets/brick_normal.png",
                asset_name="T_Brick_N",
                compression="Normalmap",
                srgb=False
            )

            # Import from Base64 (for AI-generated images)
            import_texture(
                source=base64_image_data,
                source_type="base64",
                asset_name="T_Generated",
                destination_path="/Game/Generated"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "source": source,
                "asset_name": asset_name,
                "destination_path": destination_path,
                "source_type": source_type,
                "compression": compression,
                "srgb": srgb,
                "lod_group": lod_group
            }

            logger.info(f"Importing texture '{asset_name}' to '{destination_path}' (source_type={source_type})")
            logger.info(f"import_texture: Sending command to Unreal...")
            response = unreal.send_command("import_texture", params)
            logger.info(f"import_texture: Received response: {response}")

            if not response:
                return {"success": False, "message": "No response from Unreal Engine - Editor may have crashed"}

            logger.info(f"Import texture response: {response}")
            return response

        except ConnectionResetError as e:
            error_msg = f"Connection reset by Unreal Engine (Editor likely crashed): {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg, "hint": "Check Unreal Editor logs for crash details"}
        except BrokenPipeError as e:
            error_msg = f"Broken pipe to Unreal Engine (Editor likely crashed): {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg, "hint": "Check Unreal Editor logs for crash details"}
        except Exception as e:
            error_msg = f"Error importing texture: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def get_project_info(
        ctx: Context
    ) -> Dict[str, Any]:
        """
        Get information about the current Unreal project.

        Returns:
            Dict containing:
            - project_name: Name of the project
            - engine_version: Unreal Engine version
            - project_dir: Project directory path
            - content_dir: Content directory path
            - saved_dir: Saved directory path

        Example:
            info = get_project_info()
            print(f"Project: {info['project_name']}, UE {info['engine_version']}")
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info("Getting project info")
            response = unreal.send_command("get_project_info", {})

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Project info: {response.get('project_name', 'Unknown')}")
            return response

        except Exception as e:
            error_msg = f"Error getting project info: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def find_asset_references(
        ctx: Context,
        asset_path: str
    ) -> Dict[str, Any]:
        """
        Find assets that reference the specified asset (and assets it depends on).

        Args:
            asset_path: Full content browser path to the asset
                        (e.g., "/Game/Textures/T_Icon")

        Returns:
            Dict containing:
            - referencers: List of assets that reference this asset
            - referencers_count: Number of referencers
            - dependencies: List of assets this asset depends on
            - dependencies_count: Number of dependencies

        Example:
            refs = find_asset_references(asset_path="/Game/Data/DA_Pistol")
            print(f"Referenced by {refs['referencers_count']} assets")
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {"asset_path": asset_path}

            logger.info(f"Finding references for asset: '{asset_path}'")
            response = unreal.send_command("find_asset_references", params)

            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Found {response.get('referencers_count', 0)} referencers, {response.get('dependencies_count', 0)} dependencies")
            return response

        except Exception as e:
            error_msg = f"Error finding asset references: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    logger.info("Project tools registered successfully") 