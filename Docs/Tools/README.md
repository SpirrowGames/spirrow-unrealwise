# Unreal MCP Tools

This document is an index to all the tools supported by SpirrowUnrealWise.

## Tool Categories

### Core Tools
- [Actor Tools](actor_tools.md) - Actor manipulation (spawn, delete, transform, properties)
- [Editor Tools](editor_tools.md) - Editor utilities (asset management, level operations)
- [Blueprint Tools](blueprint_tools.md) - Blueprint creation and management

### Blueprint Graph Tools
- [Node Tools](node_tools.md) - Blueprint node operations
  - Event nodes (BeginPlay, Tick, Input Actions)
  - Function call nodes
  - Variable nodes (Get/Set)
  - Branch (if/else) nodes
  - Node connections
  - Pin value setting
  - Node deletion/movement

### Input & Project Tools
- [Project Tools](project_tools.md) - Input System (Enhanced Input & Legacy)

## Quick Reference

### Node Tools (20 tools)

| Tool | Description |
|------|-------------|
| `add_blueprint_event_node` | Add event node (BeginPlay, Tick, etc.) |
| `add_blueprint_input_action_node` | Add input action event node |
| `add_blueprint_function_node` | Add function call node |
| `connect_blueprint_nodes` | Connect two nodes |
| `add_blueprint_variable` | Add variable to Blueprint |
| `add_blueprint_get_self_component_reference` | Get component reference node |
| `add_blueprint_self_reference` | Get self reference node |
| `find_blueprint_nodes` | Find nodes in graph |
| `set_node_pin_value` | Set default value on pin |
| `add_variable_get_node` | Add variable get node |
| `add_variable_set_node` | Add variable set node |
| `add_branch_node` | Add branch (if/else) node |
| `delete_blueprint_node` | Delete node from graph |
| `move_blueprint_node` | Move node position |
| `add_sequence_node` | Add sequence node for multiple execution branches |
| `add_delay_node` | Add delay node for timed execution |
| `add_foreach_loop_node` | Add ForEach loop node (not yet supported) |
| `add_print_string_node` | Add PrintString node for debug output |
| `add_math_node` | Add math operation node (not yet supported) |
| `add_comparison_node` | Add comparison node (not yet supported) |

## Version History

- **v0.6.0** - Added control flow and utility node tools (add_sequence_node, add_delay_node, add_print_string_node). Math/comparison nodes pending implementation.
- **v0.5.0** - Added node manipulation tools (set_node_pin_value, add_variable_get_node, add_variable_set_node, add_branch_node, delete_blueprint_node, move_blueprint_node)
- **v0.4.0** - Added UMG widget tools (Phase 1-4)
- **v0.3.0** - Added GAS (Gameplay Ability System) tools
- **v0.2.0** - Added Blueprint node tools
- **v0.1.0** - Initial release with actor and editor tools
