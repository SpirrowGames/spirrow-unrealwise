"""
Blueprint Tools for Unreal MCP.

This module provides tools for creating and manipulating Blueprint assets in Unreal Engine.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("SpirrowBridge")

def register_blueprint_tools(mcp: FastMCP):
    """Register Blueprint tools with the MCP server."""
    
    @mcp.tool()
    def create_blueprint(
        ctx: Context,
        name: str,
        parent_class: str,
        path: str = "/Game/Blueprints",
        rationale: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Create a new Blueprint class.

        Args:
            name: Name of the blueprint (e.g., "BP_PlayerCharacter")
            parent_class: Parent class to inherit from (e.g., "Actor", "Character", "PlayerCharacterBase")
            path: Content browser path for the asset (e.g., "/Game/TrapxTrap/Blueprints/Characters")
            rationale: Design rationale - why this blueprint is being created (auto-saved to knowledge base)

        Returns:
            Dict containing the created blueprint path
        """
        # Import inside function to avoid circular imports
        from unreal_mcp_server import get_unreal_connection
        from tools.rag_tools import record_rationale

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("create_blueprint", {
                "name": name,
                "parent_class": parent_class,
                "path": path
            })

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            # Record rationale if provided and operation was successful
            if response.get("success", True) and rationale:
                record_rationale(
                    action="create_blueprint",
                    details={"name": name, "parent_class": parent_class, "path": path},
                    rationale=rationale,
                    category="blueprint"
                )

            logger.info(f"Blueprint creation response: {response}")
            return response or {}

        except Exception as e:
            error_msg = f"Error creating blueprint: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_component_to_blueprint(
        ctx: Context,
        blueprint_name: str,
        component_type: str,
        component_name: str,
        location: List[float] = [],
        rotation: List[float] = [],
        scale: List[float] = [],
        component_properties: Dict[str, Any] = {},
        path: str = "/Game/Blueprints",
        rationale: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Add a component to a Blueprint.

        Args:
            blueprint_name: Name of the target Blueprint
            component_type: Type of component to add (use component class name without U prefix)
            component_name: Name for the new component
            location: [X, Y, Z] coordinates for component's position
            rotation: [Pitch, Yaw, Roll] values for component's rotation
            scale: [X, Y, Z] values for component's scale
            component_properties: Additional properties to set on the component
            path: Content browser path where the blueprint is located (default: "/Game/Blueprints")
            rationale: Design rationale - why this component is being added (auto-saved to knowledge base)

        Returns:
            Information about the added component
        """
        from unreal_mcp_server import get_unreal_connection
        from tools.rag_tools import record_rationale
        
        try:
            # Ensure all parameters are properly formatted
            params = {
                "blueprint_name": blueprint_name,
                "component_type": component_type,
                "component_name": component_name,
                "location": location or [0.0, 0.0, 0.0],
                "rotation": rotation or [0.0, 0.0, 0.0],
                "scale": scale or [1.0, 1.0, 1.0],
                "path": path
            }
            
            # Add component_properties if provided
            if component_properties and len(component_properties) > 0:
                params["component_properties"] = component_properties
            
            # Validate location, rotation, and scale formats
            for param_name in ["location", "rotation", "scale"]:
                param_value = params[param_name]
                if not isinstance(param_value, list) or len(param_value) != 3:
                    logger.error(f"Invalid {param_name} format: {param_value}. Must be a list of 3 float values.")
                    return {"success": False, "message": f"Invalid {param_name} format. Must be a list of 3 float values."}
                # Ensure all values are float
                params[param_name] = [float(val) for val in param_value]
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
                
            logger.info(f"Adding component to blueprint with params: {params}")
            response = unreal.send_command("add_component_to_blueprint", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            # Record rationale if provided and operation was successful
            if response.get("success", True) and rationale:
                record_rationale(
                    action="add_component_to_blueprint",
                    details={
                        "blueprint_name": blueprint_name,
                        "component_type": component_type,
                        "component_name": component_name
                    },
                    rationale=rationale,
                    category="component"
                )

            logger.info(f"Component addition response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding component to blueprint: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def set_static_mesh_properties(
        ctx: Context,
        blueprint_name: str,
        component_name: str,
        static_mesh: str = "/Engine/BasicShapes/Cube.Cube",
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Set static mesh properties on a StaticMeshComponent.

        Args:
            blueprint_name: Name of the target Blueprint
            component_name: Name of the StaticMeshComponent
            static_mesh: Path to the static mesh asset (e.g., "/Engine/BasicShapes/Cube.Cube")
            path: Content browser path where the blueprint is located (default: "/Game/Blueprints")

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
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "static_mesh": static_mesh,
                "path": path
            }
            
            logger.info(f"Setting static mesh properties with params: {params}")
            response = unreal.send_command("set_static_mesh_properties", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Set static mesh properties response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error setting static mesh properties: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def set_component_property(
        ctx: Context,
        blueprint_name: str,
        component_name: str,
        property_name: str,
        property_value,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Set a property on a component in a Blueprint.

        Args:
            blueprint_name: Name of the target Blueprint
            component_name: Name of the component
            property_name: Name of the property to set
            property_value: Value to set the property to
            path: Content browser path where the blueprint is located (default: "/Game/Blueprints")

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
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "property_name": property_name,
                "property_value": property_value,
                "path": path
            }
            
            logger.info(f"Setting component property with params: {params}")
            response = unreal.send_command("set_component_property", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Set component property response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error setting component property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def set_physics_properties(
        ctx: Context,
        blueprint_name: str,
        component_name: str,
        simulate_physics: bool = True,
        gravity_enabled: bool = True,
        mass: float = 1.0,
        linear_damping: float = 0.01,
        angular_damping: float = 0.0,
        path: str = "/Game/Blueprints",
        rationale: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Set physics properties on a component.

        Args:
            blueprint_name: Name of the target Blueprint
            component_name: Name of the component
            simulate_physics: Whether to enable physics simulation
            gravity_enabled: Whether gravity is enabled
            mass: Mass of the component
            linear_damping: Linear damping factor
            angular_damping: Angular damping factor
            path: Content browser path where the blueprint is located (default: "/Game/Blueprints")
            rationale: Design rationale - why these physics properties are being set (auto-saved to knowledge base)

        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection
        from tools.rag_tools import record_rationale

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "simulate_physics": simulate_physics,
                "gravity_enabled": gravity_enabled,
                "mass": float(mass),
                "linear_damping": float(linear_damping),
                "angular_damping": float(angular_damping),
                "path": path
            }
            
            logger.info(f"Setting physics properties with params: {params}")
            response = unreal.send_command("set_physics_properties", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            # Record rationale if provided and operation was successful
            if response.get("success", True) and rationale:
                record_rationale(
                    action="set_physics_properties",
                    details={
                        "blueprint_name": blueprint_name,
                        "component_name": component_name,
                        "simulate_physics": simulate_physics,
                        "mass": mass
                    },
                    rationale=rationale,
                    category="physics"
                )

            logger.info(f"Set physics properties response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error setting physics properties: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def compile_blueprint(
        ctx: Context,
        blueprint_name: str,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Compile a Blueprint.

        Args:
            blueprint_name: Name of the blueprint to compile
            path: Content browser path where the blueprint is located (e.g., "/Game/TrapxTrap/Blueprints/Characters")

        Returns:
            Dict containing compilation result
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "path": path
            }
            
            logger.info(f"Compiling blueprint: {blueprint_name}")
            response = unreal.send_command("compile_blueprint", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Compile blueprint response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error compiling blueprint: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_blueprint_property(
        ctx: Context,
        blueprint_name: str,
        property_name: str,
        property_value,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Set a property on a Blueprint class default object.

        Args:
            blueprint_name: Name of the target Blueprint
            property_name: Name of the property to set
            property_value: Value to set the property to
            path: Content browser path where the blueprint is located (default: "/Game/Blueprints")

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
                "blueprint_name": blueprint_name,
                "property_name": property_name,
                "property_value": property_value,
                "path": path
            }
            
            logger.info(f"Setting blueprint property with params: {params}")
            response = unreal.send_command("set_blueprint_property", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Set blueprint property response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error setting blueprint property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    # @mcp.tool() commented out, just use set_component_property instead
    def set_pawn_properties(
        ctx: Context,
        blueprint_name: str,
        auto_possess_player: str = "",
        use_controller_rotation_yaw: bool = None,
        use_controller_rotation_pitch: bool = None,
        use_controller_rotation_roll: bool = None,
        can_be_damaged: bool = None
    ) -> Dict[str, Any]:
        """
        Set common Pawn properties on a Blueprint.
        This is a utility function that sets multiple pawn-related properties at once.
        
        Args:
            blueprint_name: Name of the target Blueprint (must be a Pawn or Character)
            auto_possess_player: Auto possess player setting (None, "Disabled", "Player0", "Player1", etc.)
            use_controller_rotation_yaw: Whether the pawn should use the controller's yaw rotation
            use_controller_rotation_pitch: Whether the pawn should use the controller's pitch rotation
            use_controller_rotation_roll: Whether the pawn should use the controller's roll rotation
            can_be_damaged: Whether the pawn can be damaged
            
        Returns:
            Response indicating success or failure with detailed results for each property
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            # Define the properties to set
            properties = {}
            if auto_possess_player and auto_possess_player != "":
                properties["auto_possess_player"] = auto_possess_player
            
            # Only include boolean properties if they were explicitly set
            if use_controller_rotation_yaw is not None:
                properties["bUseControllerRotationYaw"] = use_controller_rotation_yaw
            if use_controller_rotation_pitch is not None:
                properties["bUseControllerRotationPitch"] = use_controller_rotation_pitch
            if use_controller_rotation_roll is not None:
                properties["bUseControllerRotationRoll"] = use_controller_rotation_roll
            if can_be_damaged is not None:
                properties["bCanBeDamaged"] = can_be_damaged
                
            if not properties:
                logger.warning("No properties specified to set")
                return {"success": True, "message": "No properties specified to set", "results": {}}
            
            # Set each property using the generic set_blueprint_property function
            results = {}
            overall_success = True
            
            for prop_name, prop_value in properties.items():
                params = {
                    "blueprint_name": blueprint_name,
                    "property_name": prop_name,
                    "property_value": prop_value
                }
                
                logger.info(f"Setting pawn property {prop_name} to {prop_value}")
                response = unreal.send_command("set_blueprint_property", params)
                
                if not response:
                    logger.error(f"No response from Unreal Engine for property {prop_name}")
                    results[prop_name] = {"success": False, "message": "No response from Unreal Engine"}
                    overall_success = False
                    continue
                
                results[prop_name] = response
                if not response.get("success", False):
                    overall_success = False
            
            return {
                "success": overall_success,
                "message": "Pawn properties set" if overall_success else "Some pawn properties failed to set",
                "results": results
            }
            
        except Exception as e:
            error_msg = f"Error setting pawn properties: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def duplicate_blueprint(
        ctx: Context,
        source_name: str,
        new_name: str,
        source_path: str = "/Game/Blueprints",
        destination_path: str = None
    ) -> Dict[str, Any]:
        """
        Duplicate an existing Blueprint asset.

        Args:
            source_name: Name of the Blueprint to duplicate (e.g., "BP_Enemy")
            new_name: Name for the new duplicated Blueprint (e.g., "BP_Enemy_Variant")
            source_path: Content path where the source Blueprint is located (default: "/Game/Blueprints")
            destination_path: Content path where the duplicated Blueprint will be created (default: same as source_path)

        Returns:
            dict: Result containing success status and new Blueprint path

        Example:
            duplicate_blueprint(
                source_name="BP_Enemy",
                new_name="BP_Boss",
                source_path="/Game/Blueprints/Characters",
                destination_path="/Game/Blueprints/Bosses"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            # Default destination_path to source_path if not specified
            if destination_path is None:
                destination_path = source_path

            params = {
                "source_name": source_name,
                "new_name": new_name,
                "source_path": source_path,
                "destination_path": destination_path
            }

            response = unreal.send_command("duplicate_blueprint", params)
            return response

        except Exception as e:
            error_msg = f"Error duplicating blueprint: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def get_blueprint_graph(
        ctx: Context,
        blueprint_name: str,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Get the node graph structure of an existing Blueprint.

        Args:
            blueprint_name: Name of the Blueprint to analyze
            path: Content browser path where the Blueprint is located (default: "/Game/Blueprints")

        Returns:
            dict: Result containing nodes, connections, variables, and components

        Example:
            get_blueprint_graph(
                blueprint_name="BP_Enemy",
                path="/Game/Blueprints/Characters"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "path": path
            }

            response = unreal.send_command("get_blueprint_graph", params)
            return response

        except Exception as e:
            error_msg = f"Error getting blueprint graph: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def scan_project_classes(
        ctx: Context,
        class_type: str = "all",
        parent_class: str = None,
        module_filter: str = None,
        path_filter: str = None,
        include_engine: bool = False,
        exclude_reinst: bool = True,
        blueprint_type: str = None
    ) -> Dict[str, Any]:
        """
        Scan the project for C++ classes and Blueprint assets.

        This is useful for discovering available classes to inherit from,
        finding existing Blueprints, or understanding the project structure.

        Args:
            class_type: Type of classes to scan - "cpp", "blueprint", or "all" (default)
            parent_class: Filter by parent class (e.g., "Actor", "Character", "Pawn")
            module_filter: Filter C++ classes by module name (e.g., "TrapxTrapCpp")
            path_filter: Filter Blueprints by content path (e.g., "/Game/TrapxTrap/")
            include_engine: Whether to include Engine classes (default: False)
            exclude_reinst: Whether to exclude REINST_* classes from Live Coding (default: True)
            blueprint_type: Filter Blueprints by type:
                - "actor": Actor-derived Blueprints (excludes widgets, anims)
                - "widget": UserWidget-derived Blueprints (UI)
                - "anim": AnimInstance-derived Blueprints (Animation Blueprints)
                - "controlrig": ControlRig Blueprints
                - "interface": Blueprint Interfaces (BPI_*)
                - "gamemode": GameMode Blueprints
                - "controller": Controller Blueprints (Player/AI)
                - "character": Character Blueprints
                - "pawn": Pawn Blueprints
                - None: No filter (default)

        Returns:
            Dict containing:
            - cpp_classes: List of C++ classes with name, path, parent, module
            - blueprints: List of Blueprint assets with name, path, parent
            - total_cpp: Count of C++ classes found
            - total_blueprints: Count of Blueprints found

        Examples:
            # Get all project classes (excluding REINST by default)
            scan_project_classes()

            # Get only Widget Blueprints
            scan_project_classes(class_type="blueprint", blueprint_type="widget")

            # Get Animation Blueprints
            scan_project_classes(class_type="blueprint", blueprint_type="anim")

            # Get Character Blueprints in a specific folder
            scan_project_classes(
                class_type="blueprint",
                blueprint_type="character",
                path_filter="/Game/TrapxTrap/"
            )

            # Include REINST classes (for debugging)
            scan_project_classes(exclude_reinst=False)
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "class_type": class_type,
                "include_engine": include_engine,
                "exclude_reinst": exclude_reinst
            }

            if parent_class:
                params["parent_class"] = parent_class
            if module_filter:
                params["module_filter"] = module_filter
            if path_filter:
                params["path_filter"] = path_filter
            if blueprint_type:
                params["blueprint_type"] = blueprint_type

            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            logger.info(f"Scanning project classes with filters: {params}")
            response = unreal.send_command("scan_project_classes", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Found {response.get('total_cpp', 0)} C++ classes and {response.get('total_blueprints', 0)} Blueprints")
            return response

        except Exception as e:
            error_msg = f"Error scanning project classes: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_blueprint_class_array(
        ctx: Context,
        blueprint_name: str,
        property_name: str,
        class_paths: List[str],
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Set a TSubclassOf array property on a Blueprint.

        This tool allows you to configure Blueprint properties that are arrays of class references,
        such as TrapInventory (list of trap classes) or DefaultAbilities (list of ability classes).

        Args:
            blueprint_name: Name of the target Blueprint (e.g., "BP_PlayerCharacter")
            property_name: Name of the array property (e.g., "TrapInventory", "DefaultAbilities")
            class_paths: List of Blueprint class paths. Each path should end with "_C" for Blueprint classes.
                        Example: "/Game/TrapxTrap/Blueprints/Traps/BP_ExplosionTrap.BP_ExplosionTrap_C"
            path: Content browser path where the blueprint is located (default: "/Game/Blueprints")

        Returns:
            Dict containing success status, property name, and number of classes set

        Example:
            # Set trap inventory for player character
            set_blueprint_class_array(
                blueprint_name="BP_PlayerCharacter",
                property_name="TrapInventory",
                class_paths=[
                    "/Game/TrapxTrap/Blueprints/Traps/BP_ExplosionTrap.BP_ExplosionTrap_C",
                    "/Game/TrapxTrap/Blueprints/Traps/BP_SpikeTrap.BP_SpikeTrap_C",
                    "/Game/TrapxTrap/Blueprints/Traps/BP_FreezeTrap.BP_FreezeTrap_C"
                ],
                path="/Game/TrapxTrap/Blueprints/Characters"
            )

            # Set default abilities for a character
            set_blueprint_class_array(
                blueprint_name="BP_Enemy",
                property_name="DefaultAbilities",
                class_paths=[
                    "/Game/Abilities/GA_BasicAttack.GA_BasicAttack_C",
                    "/Game/Abilities/GA_Defend.GA_Defend_C"
                ],
                path="/Game/Characters"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "property_name": property_name,
                "class_paths": class_paths,
                "path": path
            }

            logger.info(f"Setting Blueprint class array '{property_name}' on '{blueprint_name}' with {len(class_paths)} classes")
            response = unreal.send_command("set_blueprint_class_array", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set Blueprint class array response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting Blueprint class array: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_struct_array_property(
        ctx: Context,
        blueprint_name: str,
        property_name: str,
        values: List[Dict[str, Any]],
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Set a struct array property on a Blueprint.

        This tool allows you to configure Blueprint properties that are arrays of structs,
        such as TrapInventory (list of FTrapInventoryEntry) or custom struct arrays.

        Args:
            blueprint_name: Name of the target Blueprint (e.g., "BP_PlayerCharacter")
            property_name: Name of the array property (e.g., "TrapInventory")
            values: List of dictionaries, where each dictionary represents a struct.
                    Keys are field names, values are the field values.
                    - TSubclassOf fields: Use full class path ending with "_C"
                    - Numeric fields: Use numbers
                    - Boolean fields: Use true/false
                    - String fields: Use strings
            path: Content browser path where the blueprint is located (default: "/Game/Blueprints")

        Returns:
            Dict containing success status, property name, and number of elements set

        Example:
            # Set trap inventory for player character
            set_struct_array_property(
                blueprint_name="BP_PlayerCharacter",
                property_name="TrapInventory",
                values=[
                    {
                        "TrapClass": "/Game/TrapxTrap/Blueprints/Traps/BP_ExplosionTrap.BP_ExplosionTrap_C",
                        "InitialCount": 3,
                        "MaxCount": 5,
                        "CurrentCount": 0
                    }
                ],
                path="/Game/TrapxTrap/Blueprints/Characters"
            )
        """
        from unreal_mcp_server import get_unreal_connection
        import json

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "property_name": property_name,
                "values": values,  # JSON serializable list of dicts
                "path": path
            }

            logger.info(f"Setting struct array property '{property_name}' on '{blueprint_name}' with {len(values)} elements")
            response = unreal.send_command("set_struct_array_property", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set struct array property response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting struct array property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    # ===== New Property Tools (v0.8.8) =====

    @mcp.tool()
    def create_data_asset(
        ctx: Context,
        name: str,
        parent_class: str,
        path: str = "/Game/Data",
        initial_values: Dict[str, Any] = {}
    ) -> Dict[str, Any]:
        """
        Create a new DataAsset instance in the Content Browser.

        This tool creates instances of UDataAsset-derived classes, which are
        commonly used for data-driven design (game configuration, weapon stats, etc.).

        Args:
            name: Name of the data asset (e.g., "DA_Pistol", "DA_EnemyConfig")
            parent_class: The UDataAsset-derived class to instantiate.
                         Can be specified as:
                         - Simple name: "PrimaryDataAsset", "FirearmData"
                         - Full path: "/Script/TrapxTrapCpp.UFirearmData"
            path: Content browser path for the asset (default: "/Game/Data")
            initial_values: Optional dict of property names to values to set on creation

        Returns:
            Dict containing:
            - success: bool
            - asset_path: Full path to created asset
            - name: Asset name
            - parent_class: Resolved parent class name

        Example:
            # Create a simple DataAsset
            create_data_asset(
                name="DA_BossConfig",
                parent_class="PrimaryDataAsset",
                path="/Game/Data/Enemies"
            )

            # Create a custom DataAsset with initial values
            create_data_asset(
                name="DA_Pistol",
                parent_class="/Script/TrapxTrapCpp.UFirearmData",
                path="/Game/TrapxTrap/Data/Weapons",
                initial_values={"WeaponName": "Pistol", "BaseDamage": 25}
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "name": name,
                "parent_class": parent_class,
                "path": path
            }

            if initial_values:
                params["initial_values"] = initial_values

            logger.info(f"Creating DataAsset '{name}' of type '{parent_class}' at '{path}'")
            response = unreal.send_command("create_data_asset", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Create DataAsset response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating DataAsset: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_class_property(
        ctx: Context,
        blueprint_name: str,
        property_name: str,
        class_path: str,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Set a TSubclassOf property to a class reference.

        This tool sets single class reference properties (not arrays).
        For arrays of class references, use set_blueprint_class_array instead.

        Args:
            blueprint_name: Name of the target Blueprint (e.g., "BP_Spawner")
            property_name: Name of the TSubclassOf property (e.g., "SpawnClass")
            class_path: Full path to the class.
                       For Blueprints: "/Game/Blueprints/BP_Enemy.BP_Enemy_C"
                       For C++ classes: "/Script/Engine.Actor"
                       Use "None" or empty string to clear the reference
            path: Content browser path where the blueprint is located

        Returns:
            Dict containing success status and set values

        Example:
            # Set a Blueprint class reference
            set_class_property(
                blueprint_name="BP_Spawner",
                property_name="EnemyClass",
                class_path="/Game/Blueprints/Enemies/BP_Zombie.BP_Zombie_C",
                path="/Game/Blueprints/Systems"
            )

            # Set a C++ class reference
            set_class_property(
                blueprint_name="BP_GameMode",
                property_name="DefaultPawnClass",
                class_path="/Script/Engine.DefaultPawn"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "property_name": property_name,
                "class_path": class_path,
                "path": path
            }

            logger.info(f"Setting class property '{property_name}' on '{blueprint_name}' to '{class_path}'")
            response = unreal.send_command("set_class_property", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set class property response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting class property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_object_property(
        ctx: Context,
        blueprint_name: str,
        property_name: str,
        asset_path: str,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Set a UObject/TObjectPtr property to an asset reference.

        This tool sets object reference properties to point to assets like
        DataAssets, Materials, Textures, StaticMeshes, etc.

        Args:
            blueprint_name: Name of the target Blueprint
            property_name: Name of the object property to set
            asset_path: Full path to the asset.
                       Format: "/Game/Path/AssetName.AssetName"
                       Use "None" or empty string to clear the reference
            path: Content browser path where the blueprint is located

        Returns:
            Dict containing success status and set values

        Example:
            # Set a DataAsset reference
            set_object_property(
                blueprint_name="BP_PlayerCharacter",
                property_name="DefaultWeaponData",
                asset_path="/Game/Data/Weapons/DA_Pistol.DA_Pistol",
                path="/Game/Blueprints/Characters"
            )

            # Set a Material reference
            set_object_property(
                blueprint_name="BP_Trap",
                property_name="TrapMaterial",
                asset_path="/Game/Materials/M_Trap.M_Trap"
            )

            # Clear a reference
            set_object_property(
                blueprint_name="BP_Enemy",
                property_name="OptionalData",
                asset_path="None"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "property_name": property_name,
                "asset_path": asset_path,
                "path": path
            }

            logger.info(f"Setting object property '{property_name}' on '{blueprint_name}' to '{asset_path}'")
            response = unreal.send_command("set_object_property", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set object property response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting object property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def get_blueprint_properties(
        ctx: Context,
        blueprint_name: str,
        path: str = "/Game/Blueprints",
        include_inherited: bool = True,
        category_filter: str = None
    ) -> Dict[str, Any]:
        """
        Get a list of all configurable properties on a Blueprint.

        This tool is useful for discovering what properties can be set on a Blueprint
        and their types before using set_blueprint_property or other property tools.

        Args:
            blueprint_name: Name of the Blueprint to inspect
            path: Content browser path where the blueprint is located
            include_inherited: Include properties from parent classes (default: True)
            category_filter: Only return properties in this category (optional)

        Returns:
            Dict containing:
            - success: bool
            - blueprint_name: str
            - properties: List of property info objects, each containing:
                - name: Property name
                - type: Friendly type name (e.g., "TSubclassOf<AActor>", "Int32")
                - cpp_type: C++ type string
                - category: Property category
                - default_value: Current default value as string
                - is_editable: Can be edited in the editor
                - is_blueprint_visible: Visible to Blueprints
                - is_blueprint_read_only: Read-only in Blueprints
                - owner_class: Class that defines this property
            - total: Total count of properties

        Example:
            # Get all properties
            get_blueprint_properties(
                blueprint_name="BP_PlayerCharacter",
                path="/Game/Blueprints/Characters"
            )

            # Get only properties in "Combat" category
            get_blueprint_properties(
                blueprint_name="BP_Enemy",
                category_filter="Combat"
            )

            # Get only Blueprint-defined properties (not inherited)
            get_blueprint_properties(
                blueprint_name="BP_Weapon",
                include_inherited=False
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "path": path,
                "include_inherited": include_inherited
            }

            if category_filter:
                params["category_filter"] = category_filter

            logger.info(f"Getting properties for Blueprint '{blueprint_name}'")
            response = unreal.send_command("get_blueprint_properties", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Found {response.get('total', 0)} properties")
            return response

        except Exception as e:
            error_msg = f"Error getting Blueprint properties: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_struct_property(
        ctx: Context,
        blueprint_name: str,
        property_name: str,
        index: int,
        values: Dict[str, Any],
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Update specific fields of a struct array element (partial update).

        Unlike set_struct_array_property which replaces the entire array,
        this tool updates only the specified fields of a single array element.

        Args:
            blueprint_name: Name of the target Blueprint
            property_name: Name of the struct array property
            index: Array index to update (0-based)
            values: Dict of field names to new values (only specified fields are updated)
                   - For class references: Use full path "/Game/BP.BP_C"
                   - For object references: Use full path "/Game/Asset.Asset"
                   - For numbers: Use int or float
                   - For strings: Use string
                   - For bools: Use true/false
            path: Content browser path where the blueprint is located

        Returns:
            Dict containing:
            - success: bool
            - updated_fields: List of successfully updated field names
            - updated_fields_count: Number of fields updated
            - errors: List of any errors encountered (if any)

        Example:
            # Update only specific fields of inventory slot 0
            set_struct_property(
                blueprint_name="BP_PlayerCharacter",
                property_name="InventorySlots",
                index=0,
                values={
                    "MaxCount": 10,
                    "CooldownTime": 3.0,
                    "WeaponData": "/Game/Data/DA_Pistol.DA_Pistol"
                },
                path="/Game/Blueprints/Characters"
            )

            # Update a single field
            set_struct_property(
                blueprint_name="BP_Enemy",
                property_name="AbilitySlots",
                index=2,
                values={"IsEnabled": False}
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": blueprint_name,
                "property_name": property_name,
                "index": index,
                "values": values,
                "path": path
            }

            logger.info(f"Updating struct property '{property_name}[{index}]' on '{blueprint_name}' with {len(values)} fields")
            response = unreal.send_command("set_struct_property", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set struct property response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting struct property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_data_asset_property(
        ctx: Context,
        asset_name: str,
        property_name: str,
        property_value,
        path: str = "/Game/Data"
    ) -> Dict[str, Any]:
        """
        Set a property on a DataAsset.

        This tool sets properties on existing DataAsset instances, similar to
        set_blueprint_property but for DataAssets.

        Args:
            asset_name: Name of the DataAsset (e.g., "DA_Pistol")
            property_name: Name of the property to set
            property_value: Value to set. Supports:
                           - Basic types: int, float, bool, string
                           - Class references: "/Game/BP.BP_C" for TSubclassOf
                           - Object references: "/Game/Asset.Asset" for UObject*
            path: Content browser path where the DataAsset is located (default: "/Game/Data")

        Returns:
            Dict containing success status and asset info

        Example:
            # Set a simple property
            set_data_asset_property(
                asset_name="DA_Pistol",
                property_name="BaseDamage",
                property_value=50,
                path="/Game/Data/Weapons"
            )

            # Set a class reference (TSubclassOf)
            set_data_asset_property(
                asset_name="DA_Pistol",
                property_name="ProjectileClass",
                property_value="/Game/Blueprints/BP_Bullet.BP_Bullet_C"
            )

            # Set an object reference (UObject*)
            set_data_asset_property(
                asset_name="DA_Pistol",
                property_name="FireSound",
                property_value="/Game/Audio/SFX_Fire.SFX_Fire"
            )

            # Set a string property
            set_data_asset_property(
                asset_name="DA_Pistol",
                property_name="WeaponName",
                property_value="Standard Pistol"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "asset_name": asset_name,
                "property_name": property_name,
                "property_value": property_value,
                "path": path
            }

            logger.info(f"Setting DataAsset property '{property_name}' on '{asset_name}'")
            response = unreal.send_command("set_data_asset_property", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set DataAsset property response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting DataAsset property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    logger.info("Blueprint tools registered successfully") 