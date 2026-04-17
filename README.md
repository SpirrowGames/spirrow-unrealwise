# SpirrowUnrealWise

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.5+-blue)](https://www.unrealengine.com/)
[![Python](https://img.shields.io/badge/Python-3.11+-green)](https://www.python.org/)
[![MCP](https://img.shields.io/badge/MCP-Model%20Context%20Protocol-purple)](https://modelcontextprotocol.io/)
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)

**[日本語版はこちら / Japanese](README_ja.md)**

An MCP (Model Context Protocol) server that bridges AI assistants (Claude) with Unreal Engine 5. Control Blueprints, design levels, create UI, and build AI systems using natural language.

## Features (25 MCP Tools / 155 Commands)

Since **v0.9.1**, all tools are consolidated into **meta-tools** for dramatically reduced context usage (~170K → ~22K tokens).

| Meta-Tool | Commands | Description |
|-----------|----------|-------------|
| `editor` | 15 | Spawn, transform, properties, components, level (.umap) create/save/open 🆕 |
| `blueprint` | 21 | Create, compile, properties, DataAsset (Level BP support) 🆕 |
| `blueprint_node` | 24 | Event nodes, functions, variables, flow control (Level BP + external UPROPERTY + typed Subsystem) 🆕 |
| `umg_widget` | 18 | Text, image, button, slider, etc. |
| `umg_layout` | 5 | VBox/HBox, ScrollBox, reparent |
| `umg_variable` | 5 | Widget variables, functions, events |
| `umg_animation` | 4 | Animations, tracks, keyframes |
| `project` | 13 | Input Mapping, assets, folders, textures |
| `ai` | 22 | BehaviorTree, Blackboard, BT repair |
| `perception` | 6 | Sight, Hearing, Damage sensing |
| `eqs` | 5 | Environment Query System |
| `gas` | 8 | GameplayTags, Effects, Abilities |
| `material` | 6 | Template-based material creation |
| `config` | 3 | INI file read/write |

**+ 10 standalone tools**: RAG (4), AI Image (3), Project Context (2), Related Nodes (1)

> See [FEATURE_STATUS.md](FEATURE_STATUS.md) for detailed feature documentation.

---

## Quick Start

### Requirements

- Unreal Engine 5.5+
- Python 3.11+ with [uv](https://github.com/astral-sh/uv)
- Claude Desktop or Claude Code

### Installation

```bash
# 1. Clone the repository
git clone https://github.com/SpirrowGames/spirrow-unrealwise.git
cd spirrow-unrealwise

# 2. Install Python dependencies
cd Python && uv sync

# 3. Copy the UE plugin to your project
# Copy: MCPGameProject/Plugins/SpirrowBridge → YourProject/Plugins/
```

### Claude Desktop Configuration

Add to your `claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "spirrow-unrealwise": {
      "command": "uv",
      "args": ["--directory", "C:/path/to/spirrow-unrealwise/Python", "run", "python", "unreal_mcp_server.py"],
      "env": {
        "SPIRROW_UE_HOST": "127.0.0.1",
        "SPIRROW_UE_PORT": "8080"
      }
    }
  }
}
```

### Verify Installation

1. Launch Unreal Editor with SpirrowBridge plugin enabled
2. Open Claude Desktop and try: "List all actors in the current level"

---

## Usage Examples

```
"Create an Actor Blueprint called BP_Enemy"

"Add a SphereComponent to BP_Enemy with radius 500"

"Create a Widget Blueprint WBP_HUD and add a centered ProgressBar"

"Create a BehaviorTree BT_Enemy and add a Selector node"

"Set up AIPerception with sight sense, 2000 unit range, 90 degree FOV"
```

---

## Project Structure

```
spirrow-unrealwise/
├── Python/                    # MCP Server
│   ├── unreal_mcp_server.py   # Main server entry
│   ├── tools/                 # Tool definitions (14 meta + standalone)
│   └── tests/                 # Test suite
├── MCPGameProject/Plugins/    # UE Plugin
│   └── SpirrowBridge/         # Editor module
├── Docs/                      # Documentation
└── templates/                 # Material templates
```

---

## Development

### Running Tests

```bash
cd Python && python tests/run_tests.py
```

### Adding New Commands

See [Docs/PATTERNS.md](Docs/PATTERNS.md) for implementation patterns and guidelines.

### Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `SPIRROW_UE_HOST` | `127.0.0.1` | Unreal Editor host |
| `SPIRROW_UE_PORT` | `8080` | Unreal Editor port |
| `AI_IMAGE_SERVER_URL` | `http://localhost:7860` | Stable Diffusion server |

---

## Version History

**v0.9.2 (Beta)** - 2026-03-13
- BT node robustness: null pointer guards on all BT node creation
- New `repair_broken_bt_nodes` command for NodeInstance re-resolution
- Compile safety improvements

**v0.9.1 (Beta)** - 2026-03-07
- Meta-Tool Architecture: 161 individual tools → 25 MCP tools (14 meta + 1 help + 10 standalone)
- Context token reduction: ~170K → ~22K (saves ~148K tokens)

**v0.9.0 (Beta)** - 2026-02-15
- BehaviorTree Health Check: `detect_broken_bt_nodes` and `fix_broken_bt_nodes`
- Automatically detect and remove broken nodes (null NodeInstance) from BehaviorTrees

**v0.8.11 (Beta)** - 2026-01-26
- Added `find_cpp_function_in_blueprints` - Search for function callers across Blueprints

**v0.8.10 (Beta)** - 2026-01-12
- AI Image Generation integration (Stable Diffusion Forge)
- Asset utility tools (texture import, folder management)

> See [Docs/CHANGELOG.md](Docs/CHANGELOG.md) for full history.

---

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

---

## License

MIT License - see [LICENSE](LICENSE) for details.

---

## Links

- [Model Context Protocol](https://modelcontextprotocol.io/)
- [Unreal Engine Documentation](https://docs.unrealengine.com/)
- [Claude](https://claude.ai/)
