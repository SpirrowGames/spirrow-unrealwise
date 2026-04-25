"""PIE (Play-In-Editor) / runtime / log access meta-tool for SpirrowBridge.

Introduced in v0.10.0. Wraps PIE lifecycle, camera control, screenshot,
console exec, runtime introspection, input simulation, and log access.
"""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")

# Python command name -> C++ command type
COMMANDS = {
    # PIE lifecycle (6)
    "start_pie": "start_pie",
    "stop_pie": "stop_pie",
    "get_pie_state": "get_pie_state",
    "pause_pie": "pause_pie",
    "resume_pie": "resume_pie",
    "step_pie_frames": "step_pie_frames",

    # Camera + screenshot (4)
    "take_pie_screenshot": "take_pie_screenshot",
    "take_high_res_screenshot": "take_high_res_screenshot",
    "get_pie_camera": "get_pie_camera",
    "set_pie_camera": "set_pie_camera",

    # Debug cam + runtime control (3)
    "enable_debug_cam": "enable_debug_cam",
    "disable_debug_cam": "disable_debug_cam",
    "set_global_time_dilation": "set_global_time_dilation",

    # Console exec + input (2)
    "exec_console_command": "exec_console_command",
    "simulate_pie_input": "simulate_pie_input",

    # PIE world introspection (3)
    "get_pie_actors": "get_pie_actors",
    "find_pie_actors_by_class": "find_pie_actors_by_class",
    "get_pie_actor_properties": "get_pie_actor_properties",

    # Log access (7)
    "tail_ue_log": "tail_ue_log",
    "filter_ue_log": "filter_ue_log",
    "set_log_verbosity": "set_log_verbosity",
    "get_ue_log_path": "get_ue_log_path",
    "scan_ue_log_errors": "scan_ue_log_errors",
    "search_ue_log": "search_ue_log",
    "tail_editor_output_log": "tail_editor_output_log",
}


def register_pie_meta_tool(mcp: FastMCP):
    """Register the PIE meta-tool."""

    @mcp.tool()
    def pie(ctx: Context, command: str, params: Dict[str, Any] = {}) -> Dict[str, Any]:
        """PIE / runtime / log: Play-In-Editor lifecycle, camera, screenshot, console, logs.

        Commands: start_pie, stop_pie, get_pie_state, pause_pie, resume_pie, step_pie_frames,
        take_pie_screenshot, take_high_res_screenshot, get_pie_camera, set_pie_camera,
        enable_debug_cam, disable_debug_cam, set_global_time_dilation,
        exec_console_command, simulate_pie_input,
        get_pie_actors, find_pie_actors_by_class, get_pie_actor_properties,
        tail_ue_log, filter_ue_log, set_log_verbosity, get_ue_log_path,
        scan_ue_log_errors, search_ue_log, tail_editor_output_log

        PIE lifecycle:
        - start_pie: request PIE start (async; poll get_pie_state to confirm)
        - stop_pie: request end of PIE play map
        - get_pie_state: returns "stopped" / "running" / "paused" + time/player info
        - pause_pie / resume_pie: pause via APlayerController::SetPause
        - step_pie_frames: advance N frames while paused (Phase 2)

        Capture:
        - take_pie_screenshot: PNG of PIE viewport. params: {"filepath": "...png"}
        - take_high_res_screenshot: HighResShot N — params: {"multiplier": 1-10, "filepath": optional}

        Camera:
        - get_pie_camera / set_pie_camera: read/write player camera (location, rotation, fov).
          set teleports the possessed pawn — pass "use_debug_cam": true for free-look.

        Console / runtime:
        - exec_console_command: GEngine->Exec. params: {"command": "stat fps", "target": "pie"|"editor"}
        - set_global_time_dilation: scale game time. params: {"dilation": 0.5}
        - simulate_pie_input: APlayerController::InputKey. params: {"key": "Jump"|"W"|"LeftMouseButton", "event": "press"|"release"|"tap"}

        Introspection:
        - get_pie_actors: list all actors in GEditor->PlayWorld
        - find_pie_actors_by_class / get_pie_actor_properties: filter / inspect

        Logs (Saved/Logs/<Project>.log):
        - tail_ue_log: last N lines (raw)
        - filter_ue_log: filter by category prefix
        - search_ue_log: keyword/regex/severity/category filter, returns structured JSON
        - scan_ue_log_errors: only Error/Warning/Fatal/Ensure failed lines, structured
        - get_ue_log_path: absolute path of the active log file
        - tail_editor_output_log: in-memory Output Log panel buffer (pre-disk-flush)
        - set_log_verbosity: runtime "Log <Category> <Level>"

        Use help("pie", "command_name") for full param details.
        """
        from tools.meta_utils import execute_command
        return execute_command(COMMANDS, command, params)

    logger.info("PIE meta-tool registered")
