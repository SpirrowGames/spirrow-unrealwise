"""UMG (Widget Blueprint) meta-tools for SpirrowBridge.

Registers 4 meta-tools: umg_widget, umg_layout, umg_variable, umg_animation.
"""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")

# ── umg_widget commands ──
WIDGET_COMMANDS = {
    "create_umg_widget_blueprint": "create_umg_widget_blueprint",
    "add_text_block_to_widget": "add_text_block_to_widget",
    "bind_widget_event": "bind_widget_event",
    "add_widget_to_viewport": "add_widget_to_viewport",
    "set_text_block_binding": "set_text_block_binding",
    "add_text_to_widget": "add_text_to_widget",
    "add_image_to_widget": "add_image_to_widget",
    "add_progressbar_to_widget": "add_progressbar_to_widget",
    "add_border_to_widget": "add_border_to_widget",
    "get_widget_elements": "get_widget_elements",
    "set_widget_element_property": "set_widget_element_property",
    "add_button_to_widget": "add_button_to_widget",
    "bind_widget_component_event": "bind_widget_component_event",
    "add_slider_to_widget": "add_slider_to_widget",
    "add_checkbox_to_widget": "add_checkbox_to_widget",
    "add_combobox_to_widget": "add_combobox_to_widget",
    "add_editabletext_to_widget": "add_editabletext_to_widget",
    "add_spinbox_to_widget": "add_spinbox_to_widget",
    "remove_widget_element": "remove_widget_element",
    "bind_widget_to_variable": "bind_widget_to_variable",
}

# ── umg_layout commands ──
LAYOUT_COMMANDS = {
    "add_vertical_box_to_widget": "add_vertical_box_to_widget",
    "add_horizontal_box_to_widget": "add_horizontal_box_to_widget",
    "add_widget_switcher_to_widget": "add_widget_switcher_to_widget",
    "reparent_widget_element": "reparent_widget_element",
    "set_widget_slot_property": "set_widget_slot_property",
    "add_scrollbox_to_widget": "add_scrollbox_to_widget",
}

# ── umg_variable commands ──
VARIABLE_COMMANDS = {
    "add_widget_variable": "add_widget_variable",
    "set_widget_variable_default": "set_widget_variable_default",
    "add_widget_array_variable": "add_widget_array_variable",
    "add_widget_function": "add_widget_function",
    "add_widget_event": "add_widget_event",
}

# ── umg_animation commands ──
ANIMATION_COMMANDS = {
    "create_widget_animation": "create_widget_animation",
    "add_animation_track": "add_animation_track",
    "add_animation_keyframe": "add_animation_keyframe",
    "get_widget_animations": "get_widget_animations",
}


def _strip_none_values(params: Dict[str, Any]) -> Dict[str, Any]:
    """Remove None values from params (UMG tools only include non-None optional params)."""
    return {k: v for k, v in params.items() if v is not None}


def register_umg_meta_tools(mcp: FastMCP):
    """Register all 4 UMG meta-tools."""

    @mcp.tool()
    def umg_widget(ctx: Context, command: str, params: Dict[str, Any] = {}) -> Dict[str, Any]:
        """UMG widgets: create, text, image, button, slider, checkbox, combobox, border, etc.
        Commands: create_umg_widget_blueprint, add_text_block_to_widget,
        bind_widget_event, add_widget_to_viewport, set_text_block_binding,
        add_text_to_widget, add_image_to_widget, add_progressbar_to_widget,
        add_border_to_widget, get_widget_elements, set_widget_element_property,
        add_button_to_widget, bind_widget_component_event, add_slider_to_widget,
        add_checkbox_to_widget, add_combobox_to_widget, add_editabletext_to_widget,
        add_spinbox_to_widget, remove_widget_element, bind_widget_to_variable
        Use help("umg_widget", "command_name") for params.
        """
        from tools.meta_utils import execute_command
        params = _strip_none_values(params)

        # Auto-generate function_name for bind commands
        if command == "bind_widget_event" and not params.get("function_name"):
            widget_comp = params.get("widget_component_name", "")
            event = params.get("event_name", "")
            params["function_name"] = f"On{widget_comp}_{event}"

        if command == "bind_widget_component_event" and not params.get("function_name"):
            comp = params.get("component_name", "")
            event = params.get("event_type", "")
            params["function_name"] = f"On{comp}_{event}"

        return execute_command(WIDGET_COMMANDS, command, params)

    @mcp.tool()
    def umg_layout(ctx: Context, command: str, params: Dict[str, Any] = {}) -> Dict[str, Any]:
        """UMG layout: vertical/horizontal boxes, widget switchers, scroll boxes, reparenting, slots.
        Commands: add_vertical_box_to_widget, add_horizontal_box_to_widget,
        add_widget_switcher_to_widget, reparent_widget_element, set_widget_slot_property,
        add_scrollbox_to_widget
        Use help("umg_layout", "command_name") for params.
        """
        from tools.meta_utils import execute_command
        params = _strip_none_values(params)
        return execute_command(LAYOUT_COMMANDS, command, params)

    @mcp.tool()
    def umg_variable(ctx: Context, command: str, params: Dict[str, Any] = {}) -> Dict[str, Any]:
        """UMG variables/functions: add variables, set defaults, create functions/events.
        Commands: add_widget_variable, set_widget_variable_default,
        add_widget_array_variable, add_widget_function, add_widget_event
        Use help("umg_variable", "command_name") for params.
        """
        from tools.meta_utils import execute_command
        params = _strip_none_values(params)
        return execute_command(VARIABLE_COMMANDS, command, params)

    @mcp.tool()
    def umg_animation(ctx: Context, command: str, params: Dict[str, Any] = {}) -> Dict[str, Any]:
        """UMG animation: create animations, tracks, keyframes.
        Commands: create_widget_animation, add_animation_track,
        add_animation_keyframe, get_widget_animations
        Use help("umg_animation", "command_name") for params.
        """
        from tools.meta_utils import execute_command
        return execute_command(ANIMATION_COMMANDS, command, params)

    logger.info("UMG meta-tools registered (widget, layout, variable, animation)")
