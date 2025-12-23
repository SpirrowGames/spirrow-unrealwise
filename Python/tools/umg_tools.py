"""
UMG Tools for Unreal MCP.

This module provides tools for creating and manipulating UMG Widget Blueprints in Unreal Engine.
"""

import logging
from typing import Dict, List, Any
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("SpirrowBridge")

def register_umg_tools(mcp: FastMCP):
    """Register UMG tools with the MCP server."""

    @mcp.tool()
    def create_umg_widget_blueprint(
        ctx: Context,
        widget_name: str,
        parent_class: str = "UserWidget",
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Create a new UMG Widget Blueprint.
        
        Args:
            widget_name: Name of the widget blueprint to create
            parent_class: Parent class for the widget (default: UserWidget)
            path: Content browser path where the widget should be created
            
        Returns:
            Dict containing success status and widget path
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "widget_name": widget_name,
                "parent_class": parent_class,
                "path": path
            }
            
            logger.info(f"Creating UMG Widget Blueprint with params: {params}")
            response = unreal.send_command("create_umg_widget_blueprint", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Create UMG Widget Blueprint response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error creating UMG Widget Blueprint: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_text_block_to_widget(
        ctx: Context,
        widget_name: str,
        text_block_name: str,
        text: str = "",
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 50.0],
        font_size: int = 12,
        color: List[float] = [1.0, 1.0, 1.0, 1.0]
    ) -> Dict[str, Any]:
        """
        Add a Text Block widget to a UMG Widget Blueprint.
        
        Args:
            widget_name: Name of the target Widget Blueprint
            text_block_name: Name to give the new Text Block
            text: Initial text content
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the text block
            font_size: Font size in points
            color: [R, G, B, A] color values (0.0 to 1.0)
            
        Returns:
            Dict containing success status and text block properties
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "widget_name": widget_name,
                "text_block_name": text_block_name,
                "text": text,
                "position": position,
                "size": size,
                "font_size": font_size,
                "color": color
            }
            
            logger.info(f"Adding Text Block to widget with params: {params}")
            response = unreal.send_command("add_text_block_to_widget", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Add Text Block response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding Text Block to widget: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_button_to_widget(
        ctx: Context,
        widget_name: str,
        button_name: str,
        text: str = "",
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 50.0],
        font_size: int = 12,
        color: List[float] = [1.0, 1.0, 1.0, 1.0],
        background_color: List[float] = [0.1, 0.1, 0.1, 1.0]
    ) -> Dict[str, Any]:
        """
        Add a Button widget to a UMG Widget Blueprint.
        
        Args:
            widget_name: Name of the target Widget Blueprint
            button_name: Name to give the new Button
            text: Text to display on the button
            position: [X, Y] position in the canvas panel
            size: [Width, Height] of the button
            font_size: Font size for button text
            color: [R, G, B, A] text color values (0.0 to 1.0)
            background_color: [R, G, B, A] button background color values (0.0 to 1.0)
            
        Returns:
            Dict containing success status and button properties
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "widget_name": widget_name,
                "button_name": button_name,
                "text": text,
                "position": position,
                "size": size,
                "font_size": font_size,
                "color": color,
                "background_color": background_color
            }
            
            logger.info(f"Adding Button to widget with params: {params}")
            response = unreal.send_command("add_button_to_widget", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Add Button response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding Button to widget: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def bind_widget_event(
        ctx: Context,
        widget_name: str,
        widget_component_name: str,
        event_name: str,
        function_name: str = ""
    ) -> Dict[str, Any]:
        """
        Bind an event on a widget component to a function.
        
        Args:
            widget_name: Name of the target Widget Blueprint
            widget_component_name: Name of the widget component (button, etc.)
            event_name: Name of the event to bind (OnClicked, etc.)
            function_name: Name of the function to create/bind to (defaults to f"{widget_component_name}_{event_name}")
            
        Returns:
            Dict containing success status and binding information
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            # If no function name provided, create one from component and event names
            if not function_name:
                function_name = f"{widget_component_name}_{event_name}"
            
            params = {
                "widget_name": widget_name,
                "widget_component_name": widget_component_name,
                "event_name": event_name,
                "function_name": function_name
            }
            
            logger.info(f"Binding widget event with params: {params}")
            response = unreal.send_command("bind_widget_event", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Bind widget event response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error binding widget event: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_widget_to_viewport(
        ctx: Context,
        widget_name: str,
        z_order: int = 0
    ) -> Dict[str, Any]:
        """
        Add a Widget Blueprint instance to the viewport.
        
        Args:
            widget_name: Name of the Widget Blueprint to add
            z_order: Z-order for the widget (higher numbers appear on top)
            
        Returns:
            Dict containing success status and widget instance information
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "widget_name": widget_name,
                "z_order": z_order
            }
            
            logger.info(f"Adding widget to viewport with params: {params}")
            response = unreal.send_command("add_widget_to_viewport", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Add widget to viewport response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding widget to viewport: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_text_block_binding(
        ctx: Context,
        widget_name: str,
        text_block_name: str,
        binding_property: str,
        binding_type: str = "Text"
    ) -> Dict[str, Any]:
        """
        Set up a property binding for a Text Block widget.
        
        Args:
            widget_name: Name of the target Widget Blueprint
            text_block_name: Name of the Text Block to bind
            binding_property: Name of the property to bind to
            binding_type: Type of binding (Text, Visibility, etc.)
            
        Returns:
            Dict containing success status and binding information
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "widget_name": widget_name,
                "text_block_name": text_block_name,
                "binding_property": binding_property,
                "binding_type": binding_type
            }
            
            logger.info(f"Setting text block binding with params: {params}")
            response = unreal.send_command("set_text_block_binding", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Set text block binding response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error setting text block binding: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_text_to_widget(
        ctx: Context,
        widget_name: str,
        text_name: str,
        text: str = "+",
        font_size: int = 32,
        color: List[float] = [1.0, 1.0, 1.0, 1.0],
        anchor: str = "Center",
        alignment: List[float] = [0.5, 0.5],
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Add a Text Block to a Widget Blueprint with enhanced options.

        Args:
            widget_name: Name of the Widget Blueprint
            text_name: Name for the new Text Block
            text: Display text (default: "+")
            font_size: Font size in points (default: 32)
            color: [R, G, B, A] color values 0.0-1.0 (default: white)
            anchor: Anchor position - "Center", "TopLeft", "TopCenter", "TopRight",
                    "MiddleLeft", "MiddleRight", "BottomLeft", "BottomCenter", "BottomRight" (default: "Center")
            alignment: [X, Y] alignment values 0.0-1.0 (default: [0.5, 0.5])
            path: Content browser path to the widget (default: "/Game/UI")

        Returns:
            Dict containing success status and text block properties

        Example:
            add_text_to_widget(
                widget_name="WBP_Crosshair",
                text_name="CrosshairText",
                text="+",
                font_size=32,
                color=[1.0, 1.0, 1.0, 1.0],
                anchor="Center",
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "text_name": text_name,
                "text": text,
                "font_size": font_size,
                "color": color,
                "anchor": anchor,
                "alignment": alignment,
                "path": path
            }

            logger.info(f"Adding Text to widget with params: {params}")
            response = unreal.send_command("add_text_to_widget", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Add Text to widget response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding Text to widget: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_image_to_widget(
        ctx: Context,
        widget_name: str,
        image_name: str,
        texture_path: str = "",
        size: List[float] = [32.0, 32.0],
        color_tint: List[float] = [1.0, 1.0, 1.0, 1.0],
        anchor: str = "Center",
        alignment: List[float] = [0.5, 0.5],
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Add an Image widget to a Widget Blueprint.

        Args:
            widget_name: Name of the Widget Blueprint
            image_name: Name for the new Image widget
            texture_path: Path to texture asset (e.g., "/Engine/EngineResources/DefaultTexture")
            size: [Width, Height] size in pixels (default: [32, 32])
            color_tint: [R, G, B, A] tint values 0.0-1.0 (default: white)
            anchor: Anchor position - "Center", "TopLeft", "TopCenter", "TopRight",
                    "MiddleLeft", "MiddleRight", "BottomLeft", "BottomCenter", "BottomRight" (default: "Center")
            alignment: [X, Y] alignment values 0.0-1.0 (default: [0.5, 0.5])
            path: Content browser path to the widget (default: "/Game/UI")

        Returns:
            Dict containing success status and image widget properties

        Example:
            add_image_to_widget(
                widget_name="WBP_HUD",
                image_name="CrosshairImage",
                texture_path="/Engine/EngineResources/Cursors/Crosshairs",
                size=[64, 64],
                color_tint=[1.0, 1.0, 1.0, 1.0],
                anchor="Center",
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "image_name": image_name,
                "texture_path": texture_path,
                "size": size,
                "color_tint": color_tint,
                "anchor": anchor,
                "alignment": alignment,
                "path": path
            }

            logger.info(f"Adding Image to widget with params: {params}")
            response = unreal.send_command("add_image_to_widget", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Add Image to widget response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding Image to widget: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_progressbar_to_widget(
        ctx: Context,
        widget_name: str,
        progressbar_name: str,
        percent: float = 0.0,
        fill_color: List[float] = [0.0, 0.5, 1.0, 1.0],
        background_color: List[float] = [0.1, 0.1, 0.1, 1.0],
        size: List[float] = [200.0, 20.0],
        anchor: str = "Center",
        alignment: List[float] = [0.5, 0.5],
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Add a ProgressBar widget to a Widget Blueprint.

        Args:
            widget_name: Name of the Widget Blueprint
            progressbar_name: Name for the new ProgressBar
            percent: Initial fill percentage 0.0-1.0 (default: 0.0)
            fill_color: [R, G, B, A] bar color values 0.0-1.0 (default: blue)
            background_color: [R, G, B, A] background color values 0.0-1.0 (default: dark gray)
            size: [Width, Height] size in pixels (default: [200, 20])
            anchor: Anchor position - "Center", "TopLeft", "TopCenter", "TopRight",
                    "MiddleLeft", "MiddleRight", "BottomLeft", "BottomCenter", "BottomRight" (default: "Center")
            alignment: [X, Y] alignment values 0.0-1.0 (default: [0.5, 0.5])
            path: Content browser path to the widget (default: "/Game/UI")

        Returns:
            Dict containing success status and progressbar properties

        Example:
            add_progressbar_to_widget(
                widget_name="WBP_DisarmProgress",
                progressbar_name="DisarmBar",
                percent=0.0,
                fill_color=[0.0, 1.0, 0.0, 1.0],
                size=[150, 15],
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "progressbar_name": progressbar_name,
                "percent": percent,
                "fill_color": fill_color,
                "background_color": background_color,
                "size": size,
                "anchor": anchor,
                "alignment": alignment,
                "path": path
            }

            logger.info(f"Adding ProgressBar to widget with params: {params}")
            response = unreal.send_command("add_progressbar_to_widget", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Add ProgressBar to widget response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding ProgressBar to widget: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    # Phase 1: Designer Operations
    @mcp.tool()
    def get_widget_elements(
        ctx: Context,
        widget_name: str,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Get all elements in a Widget Blueprint.

        Args:
            widget_name: Name of the Widget Blueprint
            path: Content browser path to the widget

        Returns:
            Dict containing list of elements with their names, types, and hierarchy

        Example:
            get_widget_elements(
                widget_name="WBP_TT_TrapSelector",
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "path": path
            }

            logger.info(f"Getting widget elements with params: {params}")
            response = unreal.send_command("get_widget_elements", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get widget elements response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting widget elements: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_widget_slot_property(
        ctx: Context,
        widget_name: str,
        element_name: str,
        position: List[float] = None,
        size: List[float] = None,
        anchor: str = None,
        alignment: List[float] = None,
        z_order: int = None,
        auto_size: bool = None,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Set Canvas Slot properties for a widget element.

        Args:
            widget_name: Name of the Widget Blueprint
            element_name: Name of the element to modify
            position: [X, Y] position offset from anchor
            size: [Width, Height] size override
            anchor: Anchor preset - "TopLeft", "TopCenter", "TopRight",
                    "MiddleLeft", "Center", "MiddleRight",
                    "BottomLeft", "BottomCenter", "BottomRight"
            alignment: [X, Y] alignment values 0.0-1.0
            z_order: Z-order for rendering priority
            auto_size: Whether to auto-size based on content
            path: Content browser path to the widget

        Returns:
            Dict containing success status and updated properties

        Example:
            set_widget_slot_property(
                widget_name="WBP_TT_TrapSelector",
                element_name="TrapIcon",
                position=[0, -100],
                size=[64, 64],
                anchor="BottomCenter",
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "element_name": element_name,
                "path": path
            }

            # Only include non-None parameters
            if position is not None:
                params["position"] = position
            if size is not None:
                params["size"] = size
            if anchor is not None:
                params["anchor"] = anchor
            if alignment is not None:
                params["alignment"] = alignment
            if z_order is not None:
                params["z_order"] = z_order
            if auto_size is not None:
                params["auto_size"] = auto_size

            logger.info(f"Setting widget slot property with params: {params}")
            response = unreal.send_command("set_widget_slot_property", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set widget slot property response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting widget slot property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_widget_element_property(
        ctx: Context,
        widget_name: str,
        element_name: str,
        property_name: str,
        property_value: str,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Set a property on a widget element.

        Args:
            widget_name: Name of the Widget Blueprint
            element_name: Name of the element to modify
            property_name: Name of the property to set
            property_value: Value to set (JSON string for complex types)
            path: Content browser path to the widget

        Returns:
            Dict containing success status

        Common properties:
            - Visibility: "Visible", "Hidden", "Collapsed", "HitTestInvisible", "SelfHitTestInvisible"
            - Text (TextBlock): "Hello World"
            - ColorAndOpacity: "[1.0, 0.5, 0.0, 1.0]" (RGBA)
            - Justification: "Left", "Center", "Right"

        Example:
            set_widget_element_property(
                widget_name="WBP_TT_TrapSelector",
                element_name="TrapNameText",
                property_name="Visibility",
                property_value="Hidden",
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "element_name": element_name,
                "property_name": property_name,
                "property_value": property_value,
                "path": path
            }

            logger.info(f"Setting widget element property with params: {params}")
            response = unreal.send_command("set_widget_element_property", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set widget element property response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting widget element property: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_vertical_box_to_widget(
        ctx: Context,
        widget_name: str,
        box_name: str,
        parent_name: str = None,
        anchor: str = "Center",
        alignment: List[float] = [0.5, 0.5],
        position: List[float] = [0, 0],
        size: List[float] = None,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Add a VerticalBox container to a Widget Blueprint.

        Args:
            widget_name: Name of the Widget Blueprint
            box_name: Name for the new VerticalBox
            parent_name: Parent element name (null for root canvas)
            anchor: Anchor preset
            alignment: [X, Y] alignment values
            position: [X, Y] position offset
            size: [Width, Height] size override (null for auto-size)
            path: Content browser path to the widget

        Returns:
            Dict containing success status and box properties

        Example:
            add_vertical_box_to_widget(
                widget_name="WBP_TT_TrapSelector",
                box_name="MainContainer",
                anchor="BottomCenter",
                alignment=[0.5, 1.0],
                position=[0, -50],
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "box_name": box_name,
                "anchor": anchor,
                "alignment": alignment,
                "position": position,
                "path": path
            }

            if parent_name is not None:
                params["parent_name"] = parent_name
            if size is not None:
                params["size"] = size

            logger.info(f"Adding VerticalBox to widget with params: {params}")
            response = unreal.send_command("add_vertical_box_to_widget", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Add VerticalBox response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding VerticalBox to widget: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_horizontal_box_to_widget(
        ctx: Context,
        widget_name: str,
        box_name: str,
        parent_name: str = None,
        anchor: str = "Center",
        alignment: List[float] = [0.5, 0.5],
        position: List[float] = [0, 0],
        size: List[float] = None,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Add a HorizontalBox container to a Widget Blueprint.

        Args:
            widget_name: Name of the Widget Blueprint
            box_name: Name for the new HorizontalBox
            parent_name: Parent element name (null for root canvas)
            anchor: Anchor preset
            alignment: [X, Y] alignment values
            position: [X, Y] position offset
            size: [Width, Height] size override (null for auto-size)
            path: Content browser path to the widget

        Returns:
            Dict containing success status and box properties

        Example:
            add_horizontal_box_to_widget(
                widget_name="WBP_TT_TrapSelector",
                box_name="IconRow",
                parent_name="MainContainer",
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "box_name": box_name,
                "anchor": anchor,
                "alignment": alignment,
                "position": position,
                "path": path
            }

            if parent_name is not None:
                params["parent_name"] = parent_name
            if size is not None:
                params["size"] = size

            logger.info(f"Adding HorizontalBox to widget with params: {params}")
            response = unreal.send_command("add_horizontal_box_to_widget", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Add HorizontalBox response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding HorizontalBox to widget: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def reparent_widget_element(
        ctx: Context,
        widget_name: str,
        element_name: str,
        new_parent_name: str,
        slot_index: int = -1,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Move a widget element to a new parent.

        Args:
            widget_name: Name of the Widget Blueprint
            element_name: Name of the element to move
            new_parent_name: Name of the new parent element
            slot_index: Index in parent's children (-1 for end)
            path: Content browser path to the widget

        Returns:
            Dict containing success status and new hierarchy

        Example:
            reparent_widget_element(
                widget_name="WBP_TT_TrapSelector",
                element_name="TrapIcon",
                new_parent_name="MainContainer",
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "element_name": element_name,
                "new_parent_name": new_parent_name,
                "slot_index": slot_index,
                "path": path
            }

            logger.info(f"Reparenting widget element with params: {params}")
            response = unreal.send_command("reparent_widget_element", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Reparent widget element response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error reparenting widget element: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def remove_widget_element(
        ctx: Context,
        widget_name: str,
        element_name: str,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Remove a widget element from the Widget Blueprint.

        Args:
            widget_name: Name of the Widget Blueprint
            element_name: Name of the element to remove
            path: Content browser path to the widget

        Returns:
            Dict containing success status

        Example:
            remove_widget_element(
                widget_name="WBP_TT_TrapSelector",
                element_name="OldElement",
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "element_name": element_name,
                "path": path
            }

            logger.info(f"Removing widget element with params: {params}")
            response = unreal.send_command("remove_widget_element", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Remove widget element response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error removing widget element: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    # Phase 2: Variable & Function Operations
    @mcp.tool()
    def add_widget_variable(
        ctx: Context,
        widget_name: str,
        variable_name: str,
        variable_type: str,
        default_value: str = None,
        is_exposed: bool = False,
        category: str = None,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Add a variable to a Widget Blueprint.

        Args:
            widget_name: Name of the Widget Blueprint
            variable_name: Name for the new variable (e.g., "CurrentTrapName")
            variable_type: Type of the variable - "Boolean", "Integer", "Float", "String",
                          "Vector", "Text", "TimerHandle", "Texture2D", "Object", etc.
            default_value: Default value for the variable (type-appropriate string)
            is_exposed: Whether to expose the variable in the editor
            category: Category name for the variable in the editor
            path: Content browser path to the widget

        Returns:
            Dict containing success status and variable details

        Supported Types:
            - Primitives: Boolean, Integer, Float, String, Name, Text
            - Math: Vector, Vector2D, Rotator, Transform
            - Visuals: LinearColor, Texture2D
            - System: TimerHandle, Object
            - Custom: Any Blueprint class or struct name

        Example:
            add_widget_variable(
                widget_name="WBP_TT_TrapSelector",
                variable_name="bIsVisible",
                variable_type="Boolean",
                default_value="false",
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "variable_name": variable_name,
                "variable_type": variable_type,
                "is_exposed": is_exposed,
                "path": path
            }

            if default_value is not None:
                params["default_value"] = default_value
            if category is not None:
                params["category"] = category

            logger.info(f"Adding widget variable with params: {params}")
            response = unreal.send_command("add_widget_variable", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Add widget variable response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding widget variable: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_widget_variable_default(
        ctx: Context,
        widget_name: str,
        variable_name: str,
        default_value: str,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Set the default value for a widget variable.

        Args:
            widget_name: Name of the Widget Blueprint
            variable_name: Name of the variable to modify
            default_value: Default value (type-appropriate string format)
                          - Boolean: "true" or "false"
                          - Integer: "123"
                          - Float: "1.5"
                          - String: "Hello"
                          - Vector: "(X=0,Y=0,Z=100)"
            path: Content browser path to the widget

        Returns:
            Dict containing success status

        Example:
            set_widget_variable_default(
                widget_name="WBP_TT_TrapSelector",
                variable_name="DefaultTrapName",
                default_value="Explosion Trap",
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "variable_name": variable_name,
                "default_value": default_value,
                "path": path
            }

            logger.info(f"Setting widget variable default with params: {params}")
            response = unreal.send_command("set_widget_variable_default", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Set widget variable default response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error setting widget variable default: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_widget_function(
        ctx: Context,
        widget_name: str,
        function_name: str,
        inputs: List[Dict[str, str]] = None,
        outputs: List[Dict[str, str]] = None,
        is_pure: bool = False,
        category: str = None,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Add a custom function to a Widget Blueprint.

        Args:
            widget_name: Name of the Widget Blueprint
            function_name: Name for the new function (e.g., "UpdateTrapSelection")
            inputs: List of input parameters, each with "name" and "type" keys
                   Example: [{"name": "TrapName", "type": "String"}, {"name": "TrapCount", "type": "Integer"}]
            outputs: List of output parameters (return values)
                    Example: [{"name": "Success", "type": "Boolean"}]
            is_pure: Whether this is a pure function (no execution pins, can be called anywhere)
            category: Category name for the function
            path: Content browser path to the widget

        Returns:
            Dict containing success status and function details

        Example:
            add_widget_function(
                widget_name="WBP_TT_TrapSelector",
                function_name="UpdateTrapDisplay",
                inputs=[
                    {"name": "TrapName", "type": "String"},
                    {"name": "TrapCount", "type": "Integer"},
                    {"name": "TrapIcon", "type": "Texture2D"}
                ],
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "function_name": function_name,
                "is_pure": is_pure,
                "path": path
            }

            if inputs is not None:
                params["inputs"] = inputs
            if outputs is not None:
                params["outputs"] = outputs
            if category is not None:
                params["category"] = category

            logger.info(f"Adding widget function with params: {params}")
            response = unreal.send_command("add_widget_function", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Add widget function response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding widget function: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_widget_event(
        ctx: Context,
        widget_name: str,
        event_name: str,
        inputs: List[Dict[str, str]] = None,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Add a custom event to a Widget Blueprint.

        Args:
            widget_name: Name of the Widget Blueprint
            event_name: Name for the new event (e.g., "OnTrapSelected")
            inputs: List of input parameters, each with "name" and "type" keys
                   Example: [{"name": "TrapIndex", "type": "Integer"}]
            path: Content browser path to the widget

        Returns:
            Dict containing success status and event details

        Example:
            add_widget_event(
                widget_name="WBP_TT_TrapSelector",
                event_name="OnTrapSelected",
                inputs=[{"name": "TrapIndex", "type": "Integer"}],
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "event_name": event_name,
                "path": path
            }

            if inputs is not None:
                params["inputs"] = inputs

            logger.info(f"Adding widget event with params: {params}")
            response = unreal.send_command("add_widget_event", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Add widget event response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding widget event: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def bind_widget_to_variable(
        ctx: Context,
        widget_name: str,
        element_name: str,
        property_name: str,
        variable_name: str,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Bind a widget element property to a variable.

        Creates a binding function that returns the variable value, which UMG
        will automatically call to update the widget property.

        Args:
            widget_name: Name of the Widget Blueprint
            element_name: Name of the widget element to bind (e.g., "TrapNameText")
            property_name: Property to bind (e.g., "Text", "Visibility", "Percent")
            variable_name: Name of the variable to bind to
            path: Content browser path to the widget

        Returns:
            Dict containing success status and binding function name

        Common bindings:
            - TextBlock.Text -> Text variable
            - TextBlock.Visibility -> enum variable
            - ProgressBar.Percent -> Float variable
            - Image.ColorAndOpacity -> LinearColor variable

        Note: Phase 2 creates the binding function. Full automatic binding may require
              manual setup in UMG editor or will be available in Phase 3.

        Example:
            # First create a variable
            add_widget_variable(
                widget_name="WBP_TT_TrapSelector",
                variable_name="CurrentTrapName",
                variable_type="Text",
                path="/Game/TrapxTrap/UI"
            )

            # Then bind it to a TextBlock
            bind_widget_to_variable(
                widget_name="WBP_TT_TrapSelector",
                element_name="TrapNameText",
                property_name="Text",
                variable_name="CurrentTrapName",
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "element_name": element_name,
                "property_name": property_name,
                "variable_name": variable_name,
                "path": path
            }

            logger.info(f"Binding widget to variable with params: {params}")
            response = unreal.send_command("bind_widget_to_variable", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Bind widget to variable response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error binding widget to variable: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    # Phase 3: Animation Operations
    @mcp.tool()
    def create_widget_animation(
        ctx: Context,
        widget_name: str,
        animation_name: str,
        length: float = 1.0,
        loop: bool = False,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Create a new Widget Animation in a Widget Blueprint.

        Args:
            widget_name: Name of the Widget Blueprint
            animation_name: Name for the new animation (e.g., "FadeIn", "SlideOut")
            length: Animation length in seconds (default: 1.0)
            loop: Whether the animation should loop (default: False)
            path: Content browser path to the widget

        Returns:
            Dict containing success status and animation details

        Example:
            # Create a 0.5 second fade-in animation
            create_widget_animation(
                widget_name="WBP_TT_TrapSelector",
                animation_name="FadeIn",
                length=0.5,
                loop=False,
                path="/Game/TrapxTrap/UI"
            )

            # Create a looping background pulse animation
            create_widget_animation(
                widget_name="WBP_MainMenu",
                animation_name="PulseBackground",
                length=2.0,
                loop=True,
                path="/Game/UI"
            )

        Note:
            After creating the animation, use add_animation_track and add_animation_keyframe
            to define the actual animation behavior (opacity changes, transforms, etc.)
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "animation_name": animation_name,
                "length": length,
                "loop": loop,
                "path": path
            }

            logger.info(f"Creating widget animation with params: {params}")
            response = unreal.send_command("create_widget_animation", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Create widget animation response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error creating widget animation: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_animation_track(
        ctx: Context,
        widget_name: str,
        animation_name: str,
        target_widget: str,
        property_name: str,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Add an animation track to a Widget Animation.

        Args:
            widget_name: Name of the Widget Blueprint
            animation_name: Name of the animation to add track to
            target_widget: Name of the widget element to animate (e.g., "MainContainer")
            property_name: Property to animate:
                - "Opacity" or "RenderOpacity": Float track for opacity
                - "ColorAndOpacity": Color track for tint color
            path: Content browser path to the widget (default: "/Game/UI")

        Returns:
            Dict containing success status and track details

        Example:
            add_animation_track(
                widget_name="WBP_TT_TrapSelector",
                animation_name="FadeIn",
                target_widget="MainContainer",
                property_name="Opacity",
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "animation_name": animation_name,
                "target_widget": target_widget,
                "property_name": property_name,
                "path": path
            }

            logger.info(f"Adding animation track with params: {params}")
            response = unreal.send_command("add_animation_track", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Add animation track response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding animation track: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_animation_keyframe(
        ctx: Context,
        widget_name: str,
        animation_name: str,
        target_widget: str,
        property_name: str,
        time: float,
        value: Any,
        interpolation: str = "Linear",
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Add a keyframe to an animation track.

        Args:
            widget_name: Name of the Widget Blueprint
            animation_name: Name of the animation
            target_widget: Name of the widget element being animated
            property_name: Property being animated (must match existing track)
            time: Time in seconds for the keyframe
            value: Value for the keyframe:
                - Opacity: float (0.0 - 1.0)
                - ColorAndOpacity: [R, G, B, A] array (each 0.0 - 1.0)
            interpolation: Interpolation mode - "Linear", "Cubic", or "Constant"
            path: Content browser path to the widget

        Returns:
            Dict containing success status and keyframe details

        Example:
            # Fade from transparent to opaque
            add_animation_keyframe(
                widget_name="WBP_TT_TrapSelector",
                animation_name="FadeIn",
                target_widget="MainContainer",
                property_name="Opacity",
                time=0.0,
                value=0.0,
                path="/Game/TrapxTrap/UI"
            )
            add_animation_keyframe(
                widget_name="WBP_TT_TrapSelector",
                animation_name="FadeIn",
                target_widget="MainContainer",
                property_name="Opacity",
                time=0.5,
                value=1.0,
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "animation_name": animation_name,
                "target_widget": target_widget,
                "property_name": property_name,
                "time": time,
                "value": value,
                "interpolation": interpolation,
                "path": path
            }

            logger.info(f"Adding animation keyframe with params: {params}")
            response = unreal.send_command("add_animation_keyframe", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Add animation keyframe response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding animation keyframe: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def get_widget_animations(
        ctx: Context,
        widget_name: str,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Get all animations in a Widget Blueprint.

        Args:
            widget_name: Name of the Widget Blueprint
            path: Content browser path to the widget

        Returns:
            Dict containing list of animations with their details including:
                - name: Animation name
                - length: Duration in seconds
                - is_looping: Whether the animation loops
                - track_count: Number of animation tracks
                - tracks: Array of track details (target_widget, property, type, keyframe_count)

        Example:
            get_widget_animations(
                widget_name="WBP_TT_TrapSelector",
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "path": path
            }

            logger.info(f"Getting widget animations with params: {params}")
            response = unreal.send_command("get_widget_animations", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Get widget animations response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error getting widget animations: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_widget_array_variable(
        ctx: Context,
        widget_name: str,
        variable_name: str,
        element_type: str,
        is_exposed: bool = False,
        category: str = None,
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Add an array variable to a Widget Blueprint.

        Args:
            widget_name: Name of the Widget Blueprint
            variable_name: Name for the new array variable (e.g., "TrapNames")
            element_type: Type of array elements - "Boolean", "Integer", "Float", "String",
                          "Text", "Vector", "Vector2D", "LinearColor", "Texture2D", "Object"
            is_exposed: Whether to expose the variable in the editor
            category: Category name for the variable in the editor
            path: Content browser path to the widget

        Returns:
            Dict containing success status and variable details

        Supported Element Types:
            - Primitives: Boolean, Integer, Float, String, Text
            - Math: Vector, Vector2D
            - Visuals: LinearColor, Texture2D
            - System: Object

        Example:
            # String array for trap names
            add_widget_array_variable(
                widget_name="WBP_TT_TrapSelector",
                variable_name="TrapNames",
                element_type="String",
                path="/Game/TrapxTrap/UI"
            )

            # Texture2D array for trap icons
            add_widget_array_variable(
                widget_name="WBP_TT_TrapSelector",
                variable_name="TrapIcons",
                element_type="Texture2D",
                path="/Game/TrapxTrap/UI"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "widget_name": widget_name,
                "variable_name": variable_name,
                "element_type": element_type,
                "is_exposed": is_exposed,
                "path": path
            }

            if category is not None:
                params["category"] = category

            logger.info(f"Adding widget array variable with params: {params}")
            response = unreal.send_command("add_widget_array_variable", params)

            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}

            logger.info(f"Add widget array variable response: {response}")
            return response

        except Exception as e:
            error_msg = f"Error adding widget array variable: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    logger.info("UMG tools registered successfully") 