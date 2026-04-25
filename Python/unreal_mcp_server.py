"""
Unreal Engine MCP Server

A simple MCP server for interacting with Unreal Engine.
"""

import logging
import socket
import sys
import json
import os
from contextlib import asynccontextmanager
from typing import AsyncIterator, Dict, Any, Optional
from mcp.server.fastmcp import FastMCP
from dotenv import load_dotenv

# Load environment variables from .env file
# Priority: 1. Environment variables (highest)
#           2. .env file
#           3. Hardcoded defaults (lowest)
load_dotenv()

# Configure logging with more detailed format
logging.basicConfig(
    level=logging.DEBUG,  # Change to DEBUG level for more details
    format='%(asctime)s - %(name)s - %(levelname)s - [%(filename)s:%(lineno)d] - %(message)s',
    handlers=[
        logging.FileHandler('unreal_mcp.log', encoding='utf-8'),
        # logging.StreamHandler(sys.stdout) # Remove this handler to unexpected non-whitespace characters in JSON
    ]
)
logger = logging.getLogger("SpirrowBridge")

# Configuration - can be overridden via environment variables or .env file
UNREAL_HOST = os.getenv("UNREAL_HOST", "127.0.0.1")
UNREAL_PORT = int(os.getenv("UNREAL_PORT", "55557"))

# Log configuration on startup
logger.info(f"Configuration loaded - UNREAL_HOST: {UNREAL_HOST}, UNREAL_PORT: {UNREAL_PORT}")

class UnrealConnection:
    """Connection to an Unreal Engine instance."""
    
    def __init__(self):
        """Initialize the connection."""
        self.socket = None
        self.connected = False
    
    def connect(self) -> bool:
        """Connect to the Unreal Engine instance."""
        try:
            # Close any existing socket
            if self.socket:
                try:
                    self.socket.close()
                except:
                    pass
                self.socket = None
            
            logger.info(f"Connecting to Unreal at {UNREAL_HOST}:{UNREAL_PORT}...")
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.settimeout(30)  # 30 second timeout for heavy operations
            
            # Set socket options for better stability
            self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            
            # Set larger buffer sizes
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 65536)
            
            self.socket.connect((UNREAL_HOST, UNREAL_PORT))
            self.connected = True
            logger.info("Connected to Unreal Engine")
            return True
            
        except Exception as e:
            logger.error(f"Failed to connect to Unreal: {e}")
            self.connected = False
            return False
    
    def disconnect(self):
        """Disconnect from the Unreal Engine instance."""
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
        self.socket = None
        self.connected = False

    def receive_full_response(self, sock, buffer_size=4096) -> bytes:
        """Receive a complete response from Unreal, handling chunked data.

        v0.9.9 BUG-3 fix: tolerate UTF-8 multi-byte boundary splits across recv()
        chunks. Previously `data.decode('utf-8')` would raise UnicodeDecodeError
        when the current byte buffer ended mid-character (common with Japanese
        text), the outer except would re-raise, and the connection would die
        with "utf-8 codec can't decode byte 0xe3" even though more bytes were
        pending. Now we retry on UnicodeDecodeError — partial UTF-8 just means
        more bytes are coming.
        """
        chunks = []
        sock.settimeout(30)  # 30 second timeout for heavy operations
        try:
            while True:
                chunk = sock.recv(buffer_size)
                if not chunk:
                    if not chunks:
                        raise Exception("Connection closed before receiving data")
                    break
                chunks.append(chunk)

                # Process the data received so far
                data = b''.join(chunks)
                try:
                    decoded_data = data.decode('utf-8')
                except UnicodeDecodeError:
                    # Likely partial UTF-8 multi-byte sequence at tail; wait for next recv()
                    logger.debug("Partial UTF-8 at buffer tail, waiting for more data...")
                    continue

                # Try to parse as JSON to check if complete
                try:
                    json.loads(decoded_data)
                    logger.info(f"Received complete response ({len(data)} bytes)")
                    return data
                except json.JSONDecodeError:
                    # Not complete JSON yet, continue reading
                    logger.debug(f"Received partial response, waiting for more data...")
                    continue
                except Exception as e:
                    logger.warning(f"Error processing response chunk: {str(e)}")
                    continue
        except socket.timeout:
            logger.warning("Socket timeout during receive")
            if chunks:
                # If we have some data already, try to use it
                data = b''.join(chunks)
                try:
                    json.loads(data.decode('utf-8', errors='replace'))
                    logger.info(f"Using partial response after timeout ({len(data)} bytes)")
                    return data
                except:
                    pass
            raise Exception("Timeout receiving Unreal response")
        except Exception as e:
            logger.error(f"Error during receive: {str(e)}")
            raise
    
    def send_command(self, command: str, params: Dict[str, Any] = None) -> Optional[Dict[str, Any]]:
        """Send a command to Unreal Engine and get the response."""
        # Always reconnect for each command, since Unreal closes the connection after each command
        # This is different from Unity which keeps connections alive
        if self.socket:
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
            self.connected = False
        
        if not self.connect():
            logger.error("Failed to connect to Unreal Engine for command")
            return None
        
        try:
            # Match Unity's command format exactly
            command_obj = {
                "type": command,  # Use "type" instead of "command"
                "params": params or {}  # Use Unity's params or {} pattern
            }
            
            # Send without newline, exactly like Unity
            command_json = json.dumps(command_obj)
            logger.info(f"Sending command: {command_json}")
            self.socket.sendall(command_json.encode('utf-8'))
            
            # Read response using improved handler
            response_data = self.receive_full_response(self.socket)
            response = json.loads(response_data.decode('utf-8'))
            
            # Log complete response for debugging
            logger.info(f"Complete response from Unreal: {response}")
            
            # Check for both error formats: {"status": "error", ...} and {"success": false, ...}
            if response.get("status") == "error":
                error_message = response.get("error") or response.get("message", "Unknown Unreal error")
                logger.error(f"Unreal error (status=error): {error_message}")
                # We want to preserve the original error structure but ensure error is accessible
                if "error" not in response:
                    response["error"] = error_message
            elif response.get("success") is False:
                # This format uses {"success": false, "error": "message"} or {"success": false, "message": "message"}
                error_message = response.get("error") or response.get("message", "Unknown Unreal error")
                logger.error(f"Unreal error (success=false): {error_message}")
                # Convert to the standard format expected by higher layers
                response = {
                    "status": "error",
                    "error": error_message
                }
            
            # Always close the connection after command is complete
            # since Unreal will close it on its side anyway
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
            self.connected = False
            
            return response
            
        except Exception as e:
            logger.error(f"Error sending command: {e}")
            # Always reset connection state on any error
            self.connected = False
            try:
                self.socket.close()
            except:
                pass
            self.socket = None
            return {
                "status": "error",
                "error": str(e)
            }

# Global connection state
_unreal_connection: UnrealConnection = None

def get_unreal_connection() -> Optional[UnrealConnection]:
    """Get the connection to Unreal Engine."""
    global _unreal_connection
    try:
        if _unreal_connection is None:
            _unreal_connection = UnrealConnection()
            if not _unreal_connection.connect():
                logger.warning("Could not connect to Unreal Engine")
                _unreal_connection = None
        else:
            # Verify connection is still valid with a ping-like test
            try:
                # Simple test by sending an empty buffer to check if socket is still connected
                _unreal_connection.socket.sendall(b'\x00')
                logger.debug("Connection verified with ping test")
            except Exception as e:
                logger.warning(f"Existing connection failed: {e}")
                _unreal_connection.disconnect()
                _unreal_connection = None
                # Try to reconnect
                _unreal_connection = UnrealConnection()
                if not _unreal_connection.connect():
                    logger.warning("Could not reconnect to Unreal Engine")
                    _unreal_connection = None
                else:
                    logger.info("Successfully reconnected to Unreal Engine")
        
        return _unreal_connection
    except Exception as e:
        logger.error(f"Error getting Unreal connection: {e}")
        return None

@asynccontextmanager
async def server_lifespan(server: FastMCP) -> AsyncIterator[Dict[str, Any]]:
    """Handle server startup and shutdown."""
    global _unreal_connection
    logger.info("SpirrowBridge server starting up")
    try:
        _unreal_connection = get_unreal_connection()
        if _unreal_connection:
            logger.info("Connected to Unreal Engine on startup")
        else:
            logger.warning("Could not connect to Unreal Engine on startup")
    except Exception as e:
        logger.error(f"Error connecting to Unreal Engine on startup: {e}")
        _unreal_connection = None
    
    try:
        yield {}
    finally:
        if _unreal_connection:
            _unreal_connection.disconnect()
            _unreal_connection = None
        logger.info("Unreal MCP server shut down")

# Initialize server
mcp = FastMCP(
    "SpirrowBridge",
    lifespan=server_lifespan
)

# Import and register meta-tools (15 categories + 1 help)
from tools.help_tool import register_help_tool
from tools.editor_meta import register_editor_meta_tool
from tools.blueprint_meta import register_blueprint_meta_tool
from tools.node_meta import register_node_meta_tool
from tools.umg_meta import register_umg_meta_tools
from tools.project_meta import register_project_meta_tool
from tools.ai_meta import register_ai_meta_tool
from tools.perception_meta import register_perception_meta_tool
from tools.eqs_meta import register_eqs_meta_tool
from tools.gas_meta import register_gas_meta_tool
from tools.material_meta import register_material_meta_tool
from tools.config_meta import register_config_meta_tool
from tools.pie_meta import register_pie_meta_tool

# Standalone tools (no C++ bridge, kept as-is)
from tools.rag_tools import register_rag_tools
from tools.knowledge_tools import register_knowledge_tools
from tools.image_gen_tools import register_image_gen_tools

# Register meta-tools
register_help_tool(mcp)
register_editor_meta_tool(mcp)
register_blueprint_meta_tool(mcp)
register_node_meta_tool(mcp)
register_umg_meta_tools(mcp)
register_project_meta_tool(mcp)
register_ai_meta_tool(mcp)
register_perception_meta_tool(mcp)
register_eqs_meta_tool(mcp)
register_gas_meta_tool(mcp)
register_material_meta_tool(mcp)
register_config_meta_tool(mcp)
register_pie_meta_tool(mcp)

# Register standalone tools
register_rag_tools(mcp)
register_knowledge_tools(mcp)
register_image_gen_tools(mcp)

@mcp.prompt()
def info():
    """Information about available Unreal MCP tools and best practices."""
    return """
    # SpirrowBridge - Meta-Tool Architecture

    ## How to Use

    SpirrowBridge uses **meta-tools**: each category is a single tool with a `command` parameter.

    ### Pattern
    ```
    category(command="command_name", params={"key": "value", ...})
    ```

    ### Getting Help
    ```
    help(category="blueprint")                          # List all commands in category
    help(category="blueprint", command="create_blueprint")  # Get params for a command
    ```

    ## Available Meta-Tools (14 categories)

    | Tool | Description | Commands |
    |------|-------------|----------|
    | `editor` | Actors, transforms, properties, components | 12 |
    | `blueprint` | Create, compile, properties, data assets, scanning | 21 |
    | `blueprint_node` | Events, functions, variables, flow control, math | 21 |
    | `umg_widget` | Widgets: text, image, button, slider, checkbox | 19 |
    | `umg_layout` | Layout: vertical/horizontal boxes, scroll, reparent | 5 |
    | `umg_variable` | Widget variables, functions, events | 5 |
    | `umg_animation` | Widget animations, tracks, keyframes | 4 |
    | `project` | Input mapping, assets, folders, textures | 13 |
    | `ai` | Blackboards, behavior trees, BT nodes | 21 |
    | `perception` | AI sight, hearing, damage senses | 6 |
    | `eqs` | Environment Query System | 5 |
    | `gas` | Gameplay tags, effects, abilities | 8 |
    | `material` | Material templates and creation | 6 |
    | `config` | Read/write Unreal config files | 3 |

    ## Standalone Tools (unchanged)
    - `search_knowledge`, `add_knowledge`, `list_knowledge`, `delete_knowledge`
    - `get_project_context`, `update_project_context`
    - `find_relevant_nodes`
    - `get_ai_image_server_status`, `generate_image`, `generate_and_import_texture`

    ## Quick Examples
    ```
    editor(command="spawn_actor", params={"name": "MyLight", "type": "PointLight", "location": [0, 0, 200]})
    blueprint(command="create_blueprint", params={"name": "BP_Enemy", "parent_class": "Character"})
    blueprint_node(command="add_blueprint_event_node", params={"blueprint_name": "BP_Enemy", "event_name": "BeginPlay"})
    umg_widget(command="create_umg_widget_blueprint", params={"widget_name": "WBP_HUD"})
    ai(command="create_behavior_tree", params={"name": "BT_EnemyAI"})
    ```
    """

# Run the server
if __name__ == "__main__":
    import sys

    # コマンドライン引数でトランスポートを選択
    # --sse: SSEモード（開発用、サーバー単独起動）
    # デフォルト: stdioモード（Claude Desktop統合用）
    if "--sse" in sys.argv:
        import uvicorn

        port = 8000
        # --port=XXXX で任意のポート指定可能
        for arg in sys.argv:
            if arg.startswith("--port="):
                try:
                    port = int(arg.split("=")[1])
                except ValueError:
                    pass

        logger.info(f"Starting MCP server in SSE mode on port {port}")
        print(f"MCP Server running in SSE mode: http://localhost:{port}/sse")

        # FastMCPのSSEアプリケーションを取得して起動
        app = mcp.sse_app()
        uvicorn.run(app, host="0.0.0.0", port=port)
    else:
        logger.info("Starting MCP server in stdio mode")
        mcp.run(transport='stdio') 