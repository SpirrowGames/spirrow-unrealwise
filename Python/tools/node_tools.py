"""
Blueprint Node Tools for Unreal MCP.

This module provides tools for manipulating Blueprint graph nodes and connections.
"""

import logging
from typing import Dict, List, Any, Optional
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("SpirrowBridge")

def register_blueprint_node_tools(mcp: FastMCP):
    """Register Blueprint node manipulation tools with the MCP server."""
    
    @mcp.tool()
    def add_blueprint_event_node(
        ctx: Context,
        blueprint_name: str,
        event_name: str,
        node_position = None,
        path: str = "/Game/Blueprints",
        rationale: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Add an event node to a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            event_name: Name of the event. Use 'Receive' prefix for standard events:
                       - 'ReceiveBeginPlay' for Begin Play
                       - 'ReceiveTick' for Tick
                       - etc.
            node_position: Optional [X, Y] position in the graph
            path: Content browser path where the blueprint is located (default: "/Game/Blueprints")
            rationale: Design rationale - why this event is being used (auto-saved to knowledge base)

        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        from tools.rag_tools import record_rationale

        try:
            # Handle default value within the method body
            if node_position is None:
                node_position = [0, 0]

            params = {
                "blueprint_name": blueprint_name,
                "event_name": event_name,
                "node_position": node_position,
                "path": path
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding event node '{event_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_event_node", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            # Record rationale if provided and operation was successful
            if response.get("success", True) and rationale:
                record_rationale(
                    action="add_blueprint_event_node",
                    details={
                        "blueprint_name": blueprint_name,
                        "event_name": event_name
                    },
                    rationale=rationale,
                    category="blueprint_event"
                )

            logger.info(f"Event node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding event node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_input_action_node(
        ctx: Context,
        blueprint_name: str,
        action_name: str,
        node_position = None,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Add an input action event node to a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            action_name: Name of the input action to respond to
            node_position: Optional [X, Y] position in the graph
            path: Content browser path where the blueprint is located (default: "/Game/Blueprints")

        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            # Handle default value within the method body
            if node_position is None:
                node_position = [0, 0]

            params = {
                "blueprint_name": blueprint_name,
                "action_name": action_name,
                "node_position": node_position,
                "path": path
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding input action node for '{action_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_input_action_node", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Input action node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding input action node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_function_node(
        ctx: Context,
        blueprint_name: str,
        target: str,
        function_name: str,
        params = None,
        node_position = None,
        path: str = "/Game/Blueprints",
        rationale: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Add a function call node to a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            target: Target object for the function (component name or self)
            function_name: Name of the function to call
            params: Optional parameters to set on the function node
            node_position: Optional [X, Y] position in the graph
            path: Content browser path where the blueprint is located (default: "/Game/Blueprints")
            rationale: Design rationale - why this function is being used (auto-saved to knowledge base)

        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection
        from tools.rag_tools import record_rationale

        try:
            # Handle default values within the method body
            if params is None:
                params = {}
            if node_position is None:
                node_position = [0, 0]

            command_params = {
                "blueprint_name": blueprint_name,
                "target": target,
                "function_name": function_name,
                "params": params,
                "node_position": node_position,
                "path": path
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding function node '{function_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_function_node", command_params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            # Record rationale if provided and operation was successful
            if response.get("success", True) and rationale:
                record_rationale(
                    action="add_blueprint_function_node",
                    details={
                        "blueprint_name": blueprint_name,
                        "target": target,
                        "function_name": function_name
                    },
                    rationale=rationale,
                    category="blueprint_logic"
                )

            logger.info(f"Function node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding function node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
            
    @mcp.tool()
    def connect_blueprint_nodes(
        ctx: Context,
        blueprint_name: str,
        source_node_id: str,
        source_pin: str,
        target_node_id: str,
        target_pin: str,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Connect two nodes in a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            source_node_id: ID of the source node
            source_pin: Name of the output pin on the source node
            target_node_id: ID of the target node
            target_pin: Name of the input pin on the target node
            path: Content browser path where the blueprint is located (default: "/Game/Blueprints")

        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "source_node_id": source_node_id,
                "source_pin": source_pin,
                "target_node_id": target_node_id,
                "target_pin": target_pin,
                "path": path
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Connecting nodes in blueprint '{blueprint_name}'")
            response = unreal.send_command("connect_blueprint_nodes", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Node connection response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error connecting nodes: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_variable(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        variable_type: str,
        is_exposed: bool = False,
        path: str = "/Game/Blueprints",
        rationale: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        Add a variable to a Blueprint.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable
            variable_type: Type of the variable (Boolean, Integer, Float, Vector, etc.)
            is_exposed: Whether to expose the variable to the editor
            path: Content browser path where the blueprint is located (default: "/Game/Blueprints")
            rationale: Design rationale - why this variable is needed (auto-saved to knowledge base)

        Returns:
            Response indicating success or failure
        """
        from unreal_mcp_server import get_unreal_connection
        from tools.rag_tools import record_rationale

        try:
            params = {
                "blueprint_name": blueprint_name,
                "variable_name": variable_name,
                "variable_type": variable_type,
                "is_exposed": is_exposed,
                "path": path
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding variable '{variable_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_variable", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            # Record rationale if provided and operation was successful
            if response.get("success", True) and rationale:
                record_rationale(
                    action="add_blueprint_variable",
                    details={
                        "blueprint_name": blueprint_name,
                        "variable_name": variable_name,
                        "variable_type": variable_type
                    },
                    rationale=rationale,
                    category="blueprint_variable"
                )

            logger.info(f"Variable creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding variable: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_get_self_component_reference(
        ctx: Context,
        blueprint_name: str,
        component_name: str,
        node_position = None,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Add a node that gets a reference to a component owned by the current Blueprint.
        This creates a node similar to what you get when dragging a component from the Components panel.

        Args:
            blueprint_name: Name of the target Blueprint
            component_name: Name of the component to get a reference to
            node_position: Optional [X, Y] position in the graph
            path: Content browser path where the blueprint is located (default: "/Game/Blueprints")

        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            # Handle None case explicitly in the function
            if node_position is None:
                node_position = [0, 0]

            params = {
                "blueprint_name": blueprint_name,
                "component_name": component_name,
                "node_position": node_position,
                "path": path
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding self component reference node for '{component_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_get_self_component_reference", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Self component reference node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding self component reference node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_blueprint_self_reference(
        ctx: Context,
        blueprint_name: str,
        node_position = None,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Add a 'Get Self' node to a Blueprint's event graph that returns a reference to this actor.

        Args:
            blueprint_name: Name of the target Blueprint
            node_position: Optional [X, Y] position in the graph
            path: Content browser path where the blueprint is located (default: "/Game/Blueprints")

        Returns:
            Response containing the node ID and success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            if node_position is None:
                node_position = [0, 0]

            params = {
                "blueprint_name": blueprint_name,
                "node_position": node_position,
                "path": path
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding self reference node to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_blueprint_self_reference", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Self reference node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding self reference node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def find_blueprint_nodes(
        ctx: Context,
        blueprint_name: str,
        node_type = None,
        event_type = None,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Find nodes in a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            node_type: Optional type of node to find (Event, Function, Variable, etc.)
            event_type: Optional specific event type to find (BeginPlay, Tick, etc.)
            path: Content browser path where the blueprint is located (default: "/Game/Blueprints")

        Returns:
            Response containing array of found node IDs and success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "node_type": node_type,
                "event_type": event_type,
                "path": path
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Finding nodes in blueprint '{blueprint_name}'")
            response = unreal.send_command("find_blueprint_nodes", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Node find response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error finding nodes: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    # ==================== NEW NODE MANIPULATION TOOLS ====================
    
    @mcp.tool()
    def set_node_pin_value(
        ctx: Context,
        blueprint_name: str,
        node_id: str,
        pin_name: str,
        pin_value: Any,  # Accept any type, will convert to string
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Set a default value on a node's pin.

        Args:
            blueprint_name: Name of the target Blueprint
            node_id: GUID of the node (from find_blueprint_nodes or node creation)
            pin_name: Name of the pin to set (e.g., "InString" for PrintString)
            pin_value: Value to set on the pin (string, number, or boolean - will be converted to string)
            path: Content browser path where the blueprint is located

        Returns:
            Response indicating success or failure

        Example:
            # Set the message for a PrintString node
            set_node_pin_value(
                blueprint_name="BP_Test",
                node_id="ABC123...",
                pin_name="InString",
                pin_value="Hello World!"
            )
            
            # Set a float value
            set_node_pin_value(
                blueprint_name="BP_Test",
                node_id="DEF456...",
                pin_name="Health",
                pin_value=100.0
            )
            
            # Set a boolean value
            set_node_pin_value(
                blueprint_name="BP_Test",
                node_id="GHI789...",
                pin_name="IsAlive",
                pin_value=True
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            # Convert pin_value to string (handles int, float, bool, etc.)
            if isinstance(pin_value, bool):
                pin_value_str = "true" if pin_value else "false"
            else:
                pin_value_str = str(pin_value)
            
            params = {
                "blueprint_name": blueprint_name,
                "node_id": node_id,
                "pin_name": pin_name,
                "pin_value": pin_value_str,
                "path": path
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Setting pin '{pin_name}' on node '{node_id}' to '{pin_value}'")
            response = unreal.send_command("set_node_pin_value", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Set pin value response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error setting pin value: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_variable_get_node(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        node_position = None,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Add a Variable Get node to retrieve a variable's value.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable to get (must exist in blueprint)
            node_position: Optional [X, Y] position in the graph
            path: Content browser path where the blueprint is located

        Returns:
            Response containing the node ID and success status

        Example:
            # First create a variable, then add a get node
            add_blueprint_variable(blueprint_name="BP_Test", variable_name="Health", variable_type="Float")
            add_variable_get_node(blueprint_name="BP_Test", variable_name="Health")
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            if node_position is None:
                node_position = [0, 0]

            params = {
                "blueprint_name": blueprint_name,
                "variable_name": variable_name,
                "node_position": node_position,
                "path": path
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding variable get node for '{variable_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_variable_get_node", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Variable get node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding variable get node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_variable_set_node(
        ctx: Context,
        blueprint_name: str,
        variable_name: str,
        node_position = None,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Add a Variable Set node to assign a value to a variable.

        Args:
            blueprint_name: Name of the target Blueprint
            variable_name: Name of the variable to set (must exist in blueprint)
            node_position: Optional [X, Y] position in the graph
            path: Content browser path where the blueprint is located

        Returns:
            Response containing the node ID and success status

        Example:
            # First create a variable, then add a set node
            add_blueprint_variable(blueprint_name="BP_Test", variable_name="Health", variable_type="Float")
            add_variable_set_node(blueprint_name="BP_Test", variable_name="Health")
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            if node_position is None:
                node_position = [0, 0]

            params = {
                "blueprint_name": blueprint_name,
                "variable_name": variable_name,
                "node_position": node_position,
                "path": path
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding variable set node for '{variable_name}' to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_variable_set_node", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Variable set node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding variable set node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def add_branch_node(
        ctx: Context,
        blueprint_name: str,
        node_position = None,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Add a Branch (if/else) node for conditional logic.

        Args:
            blueprint_name: Name of the target Blueprint
            node_position: Optional [X, Y] position in the graph
            path: Content browser path where the blueprint is located

        Returns:
            Response containing the node ID and success status

        Pins:
            Input:
                - execute: Execution input
                - Condition: Boolean condition to evaluate
            Output:
                - True: Execution continues here if condition is true
                - False: Execution continues here if condition is false
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            if node_position is None:
                node_position = [0, 0]

            params = {
                "blueprint_name": blueprint_name,
                "node_position": node_position,
                "path": path
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Adding branch node to blueprint '{blueprint_name}'")
            response = unreal.send_command("add_branch_node", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Branch node creation response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding branch node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def delete_blueprint_node(
        ctx: Context,
        blueprint_name: str,
        node_id: str,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Delete a node from a Blueprint's event graph.

        Args:
            blueprint_name: Name of the target Blueprint
            node_id: GUID of the node to delete
            path: Content browser path where the blueprint is located

        Returns:
            Response indicating success or failure

        Note:
            All connections to/from the node will be broken before deletion.
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "node_id": node_id,
                "path": path
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Deleting node '{node_id}' from blueprint '{blueprint_name}'")
            response = unreal.send_command("delete_node", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Node deletion response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error deleting node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    @mcp.tool()
    def move_blueprint_node(
        ctx: Context,
        blueprint_name: str,
        node_id: str,
        position: list,
        path: str = "/Game/Blueprints"
    ) -> Dict[str, Any]:
        """
        Move a node to a new position in the Blueprint graph.

        Args:
            blueprint_name: Name of the target Blueprint
            node_id: GUID of the node to move
            position: [X, Y] new position for the node
            path: Content browser path where the blueprint is located

        Returns:
            Response containing the new position and success status
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            params = {
                "blueprint_name": blueprint_name,
                "node_id": node_id,
                "position": position,
                "path": path
            }
            
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            logger.info(f"Moving node '{node_id}' to position {position}")
            response = unreal.send_command("move_node", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Node move response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error moving node: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}
    
    logger.info("Blueprint node tools registered successfully")