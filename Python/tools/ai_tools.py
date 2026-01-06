"""
AI Tools for Unreal MCP - BehaviorTree and Blackboard operations.

This module provides tools for working with Unreal Engine's AI systems.
"""

import logging
from typing import Dict, Any, List, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")


def register_ai_tools(mcp: FastMCP):
	"""Register AI tools with the MCP server."""

	# ===== Blackboard Tools =====

	@mcp.tool()
	def create_blackboard(
		ctx: Context,
		name: str,
		path: str = "/Game/AI/Blackboards"
	) -> Dict[str, Any]:
		"""
		Create a new Blackboard Data Asset.

		Args:
			name: Name of the Blackboard (e.g., "BB_Enemy", "BB_Player")
			path: Content browser path for the asset

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- asset_path: Path to the created asset
			- name: Name of the created Blackboard

		Example:
			create_blackboard(
				name="BB_Enemy",
				path="/Game/AI/Blackboards"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {"name": name, "path": path}
			logger.info(f"Creating Blackboard: {name}")
			response = unreal.send_command("create_blackboard", params)

			if response and response.get("success"):
				logger.info(f"Created Blackboard: {response.get('asset_path')}")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error creating Blackboard: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def add_blackboard_key(
		ctx: Context,
		blackboard_name: str,
		key_name: str,
		key_type: str,
		path: str = "/Game/AI/Blackboards",
		instance_synced: bool = False,
		base_class: Optional[str] = None
	) -> Dict[str, Any]:
		"""
		Add a key to an existing Blackboard.

		Args:
			blackboard_name: Name of the target Blackboard
			key_name: Name of the key to add (e.g., "TargetActor", "PatrolIndex")
			key_type: Type of the key:
				- "Bool": Boolean value
				- "Int": Integer value
				- "Float": Float value
				- "String": String value
				- "Name": FName value
				- "Vector": 3D Vector
				- "Rotator": Rotation
				- "Object": UObject reference
				- "Class": UClass reference
				- "Enum": Enumeration value
			path: Content browser path where the Blackboard is located
			instance_synced: Whether to sync this key across instances
			base_class: For Object/Class types, the allowed base class (e.g., "Actor", "Pawn")

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- blackboard_name: Name of the Blackboard
			- key_name: Name of the added key
			- total_keys: Total number of keys after addition

		Examples:
			# Add a target actor reference
			add_blackboard_key(
				blackboard_name="BB_Enemy",
				key_name="TargetActor",
				key_type="Object",
				base_class="Actor"
			)

			# Add a patrol index
			add_blackboard_key(
				blackboard_name="BB_Enemy",
				key_name="PatrolIndex",
				key_type="Int"
			)

			# Add a destination vector
			add_blackboard_key(
				blackboard_name="BB_Enemy",
				key_name="MoveToLocation",
				key_type="Vector"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"blackboard_name": blackboard_name,
				"key_name": key_name,
				"key_type": key_type,
				"path": path,
				"instance_synced": instance_synced
			}
			if base_class:
				params["base_class"] = base_class

			logger.info(f"Adding key '{key_name}' ({key_type}) to Blackboard '{blackboard_name}'")
			response = unreal.send_command("add_blackboard_key", params)

			if response and response.get("success"):
				logger.info(f"Added key: {key_name}, total keys: {response.get('total_keys')}")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error adding Blackboard key: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def remove_blackboard_key(
		ctx: Context,
		blackboard_name: str,
		key_name: str,
		path: str = "/Game/AI/Blackboards"
	) -> Dict[str, Any]:
		"""
		Remove a key from a Blackboard.

		Args:
			blackboard_name: Name of the target Blackboard
			key_name: Name of the key to remove
			path: Content browser path where the Blackboard is located

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- blackboard_name: Name of the Blackboard
			- removed_key: Name of the removed key
			- remaining_keys: Number of remaining keys

		Example:
			remove_blackboard_key(
				blackboard_name="BB_Enemy",
				key_name="OldKey"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"blackboard_name": blackboard_name,
				"key_name": key_name,
				"path": path
			}

			logger.info(f"Removing key '{key_name}' from Blackboard '{blackboard_name}'")
			response = unreal.send_command("remove_blackboard_key", params)

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error removing Blackboard key: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def list_blackboard_keys(
		ctx: Context,
		blackboard_name: str,
		path: str = "/Game/AI/Blackboards"
	) -> Dict[str, Any]:
		"""
		List all keys in a Blackboard.

		Args:
			blackboard_name: Name of the target Blackboard
			path: Content browser path where the Blackboard is located

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- blackboard_name: Name of the Blackboard
			- keys: List of key objects with name, type, and properties
			- count: Total number of keys

		Example:
			list_blackboard_keys(blackboard_name="BB_Enemy")
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"blackboard_name": blackboard_name,
				"path": path
			}

			logger.info(f"Listing keys for Blackboard '{blackboard_name}'")
			response = unreal.send_command("list_blackboard_keys", params)

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error listing Blackboard keys: {e}")
			return {"success": False, "error": str(e)}

	# ===== BehaviorTree Tools =====

	@mcp.tool()
	def create_behavior_tree(
		ctx: Context,
		name: str,
		path: str = "/Game/AI/BehaviorTrees",
		blackboard_name: Optional[str] = None,
		blackboard_path: str = "/Game/AI/Blackboards"
	) -> Dict[str, Any]:
		"""
		Create a new BehaviorTree Asset.

		Args:
			name: Name of the BehaviorTree (e.g., "BT_Enemy", "BT_Patrol")
			path: Content browser path for the asset
			blackboard_name: Optional Blackboard to link (e.g., "BB_Enemy")
			blackboard_path: Path where the Blackboard is located

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- asset_path: Path to the created asset
			- name: Name of the created BehaviorTree
			- has_blackboard: Whether a Blackboard was linked
			- blackboard_name: Name of the linked Blackboard (if any)

		Examples:
			# Create BT without Blackboard
			create_behavior_tree(name="BT_SimplePatrol")

			# Create BT with linked Blackboard
			create_behavior_tree(
				name="BT_Enemy",
				blackboard_name="BB_Enemy",
				path="/Game/AI/BehaviorTrees"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"name": name,
				"path": path,
				"blackboard_name": blackboard_name or "",
				"blackboard_path": blackboard_path
			}

			logger.info(f"Creating BehaviorTree: {name}")
			response = unreal.send_command("create_behavior_tree", params)

			if response and response.get("success"):
				logger.info(f"Created BehaviorTree: {response.get('asset_path')}")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error creating BehaviorTree: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def set_behavior_tree_blackboard(
		ctx: Context,
		behavior_tree_name: str,
		blackboard_name: str,
		behavior_tree_path: str = "/Game/AI/BehaviorTrees",
		blackboard_path: str = "/Game/AI/Blackboards"
	) -> Dict[str, Any]:
		"""
		Set the Blackboard asset for an existing BehaviorTree.

		Args:
			behavior_tree_name: Name of the target BehaviorTree
			blackboard_name: Name of the Blackboard to set
			behavior_tree_path: Path where the BehaviorTree is located
			blackboard_path: Path where the Blackboard is located

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- behavior_tree_name: Name of the BehaviorTree
			- blackboard_name: Name of the linked Blackboard

		Example:
			set_behavior_tree_blackboard(
				behavior_tree_name="BT_Enemy",
				blackboard_name="BB_Enemy"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"behavior_tree_name": behavior_tree_name,
				"blackboard_name": blackboard_name,
				"behavior_tree_path": behavior_tree_path,
				"blackboard_path": blackboard_path
			}

			logger.info(f"Setting Blackboard '{blackboard_name}' for BT '{behavior_tree_name}'")
			response = unreal.send_command("set_behavior_tree_blackboard", params)

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error setting BehaviorTree Blackboard: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def get_behavior_tree_structure(
		ctx: Context,
		name: str,
		path: str = "/Game/AI/BehaviorTrees"
	) -> Dict[str, Any]:
		"""
		Get the structure of a BehaviorTree.

		Args:
			name: Name of the BehaviorTree
			path: Content browser path where the BehaviorTree is located

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- name: Name of the BehaviorTree
			- blackboard_name: Name of the linked Blackboard
			- blackboard_key_count: Number of Blackboard keys
			- has_root_node: Whether the tree has a root node

		Example:
			get_behavior_tree_structure(name="BT_Enemy")
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {"name": name, "path": path}

			logger.info(f"Getting structure for BehaviorTree '{name}'")
			response = unreal.send_command("get_behavior_tree_structure", params)

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error getting BehaviorTree structure: {e}")
			return {"success": False, "error": str(e)}

	# ===== BT Node Operation Tools (Phase G) =====

	@mcp.tool()
	def add_bt_composite_node(
		ctx: Context,
		behavior_tree_name: str,
		node_type: str,
		path: str = "/Game/AI/BehaviorTrees",
		node_name: Optional[str] = None
	) -> Dict[str, Any]:
		"""
		Add a composite node (Selector/Sequence/SimpleParallel) to a BehaviorTree.

		Composite nodes control the flow of execution through their children.
		After creating, use connect_bt_nodes() to add the node to the tree.

		Args:
			behavior_tree_name: Name of the target BehaviorTree
			node_type: Type of composite node:
				- "Selector": Tries children in order until one succeeds
				- "Sequence": Runs children in order until one fails
				- "SimpleParallel": Runs main task with background tasks
			path: Content browser path where the BehaviorTree is located
			node_name: Optional display name for the node in the editor

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- behavior_tree_name: Name of the BehaviorTree
			- node_id: Unique ID of the created node (use for connect_bt_nodes)
			- node_type: Type string that was specified
			- node_class: Actual UClass name
			- node_name: Display name (if specified)

		Examples:
			# Create a Selector node (tries children until one succeeds)
			result = add_bt_composite_node(
				behavior_tree_name="BT_Enemy",
				node_type="Selector",
				node_name="Attack Or Flee"
			)
			# Use result["node_id"] to connect this node

			# Create a Sequence node (runs children until one fails)
			add_bt_composite_node(
				behavior_tree_name="BT_Patrol",
				node_type="Sequence",
				node_name="Patrol Loop"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"behavior_tree_name": behavior_tree_name,
				"node_type": node_type,
				"path": path
			}
			if node_name:
				params["node_name"] = node_name

			logger.info(f"Adding composite node ({node_type}) to BT '{behavior_tree_name}'")
			response = unreal.send_command("add_bt_composite_node", params)

			if response and response.get("success"):
				logger.info(f"Created node: {response.get('node_id')}")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error adding composite node: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def add_bt_task_node(
		ctx: Context,
		behavior_tree_name: str,
		task_type: str,
		path: str = "/Game/AI/BehaviorTrees",
		node_name: Optional[str] = None
	) -> Dict[str, Any]:
		"""
		Add a task node to a BehaviorTree.

		Task nodes are leaf nodes that perform actions.
		After creating, use connect_bt_nodes() to add the node to the tree.

		Args:
			behavior_tree_name: Name of the target BehaviorTree
			task_type: Type of task node:
				Built-in tasks:
				- "BTTask_MoveTo": Move to a location or actor
				- "BTTask_MoveDirectlyToward": Move directly toward target
				- "BTTask_Wait": Wait for specified seconds
				- "BTTask_WaitBlackboardTime": Wait using blackboard time value
				- "BTTask_PlaySound": Play a sound
				- "BTTask_PlayAnimation": Play an animation
				- "BTTask_RotateToFaceBBEntry": Rotate to face blackboard entry
				- "BTTask_RunBehavior": Run a sub-BehaviorTree
				- "BTTask_RunBehaviorDynamic": Run a dynamic sub-BehaviorTree

				Custom Blueprint tasks:
				- Use the Blueprint name (e.g., "MyCustomTask")
				- Must exist at /Game/AI/Tasks/
			path: Content browser path where the BehaviorTree is located
			node_name: Optional display name for the node in the editor

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- behavior_tree_name: Name of the BehaviorTree
			- node_id: Unique ID of the created node (use for connect_bt_nodes)
			- task_type: Type string that was specified
			- node_class: Actual UClass name
			- node_name: Display name (if specified)

		Examples:
			# Add MoveTo task
			result = add_bt_task_node(
				behavior_tree_name="BT_Enemy",
				task_type="BTTask_MoveTo",
				node_name="Move To Player"
			)

			# Add Wait task
			add_bt_task_node(
				behavior_tree_name="BT_Patrol",
				task_type="BTTask_Wait",
				node_name="Wait 3 Seconds"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"behavior_tree_name": behavior_tree_name,
				"task_type": task_type,
				"path": path
			}
			if node_name:
				params["node_name"] = node_name

			logger.info(f"Adding task node ({task_type}) to BT '{behavior_tree_name}'")
			response = unreal.send_command("add_bt_task_node", params)

			if response and response.get("success"):
				logger.info(f"Created task node: {response.get('node_id')}")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error adding task node: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def add_bt_decorator_node(
		ctx: Context,
		behavior_tree_name: str,
		decorator_type: str,
		target_node_id: str,
		path: str = "/Game/AI/BehaviorTrees",
		node_name: Optional[str] = None
	) -> Dict[str, Any]:
		"""
		Add a decorator (condition) to a BehaviorTree node.

		Decorators add conditions to control node execution.
		They can be attached to Composite or Task nodes.

		Args:
			behavior_tree_name: Name of the target BehaviorTree
			decorator_type: Type of decorator:
				- "BTDecorator_Blackboard": Check blackboard value condition
				- "BTDecorator_CompareBBEntries": Compare two blackboard entries
				- "BTDecorator_Cooldown": Limit execution frequency
				- "BTDecorator_DoesPathExist": Check if path exists to target
				- "BTDecorator_ForceSuccess": Force child to return success
				- "BTDecorator_IsAtLocation": Check if at location
				- "BTDecorator_Loop": Loop child execution
				- "BTDecorator_TagCooldown": Gameplay tag based cooldown
				- "BTDecorator_TimeLimit": Set time limit for child

				Custom Blueprint decorators:
				- Use the Blueprint name
				- Must exist at /Game/AI/Decorators/
			target_node_id: ID of the node to attach decorator to (from add_bt_composite_node or add_bt_task_node)
			path: Content browser path where the BehaviorTree is located
			node_name: Optional display name for the decorator

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- behavior_tree_name: Name of the BehaviorTree
			- node_id: Unique ID of the created decorator
			- decorator_type: Type string that was specified
			- target_node_id: ID of the node it was attached to
			- node_name: Display name (if specified)

		Examples:
			# Add Blackboard condition decorator
			add_bt_decorator_node(
				behavior_tree_name="BT_Enemy",
				decorator_type="BTDecorator_Blackboard",
				target_node_id="BTComposite_Selector_0",
				node_name="Has Target?"
			)

			# Add Cooldown decorator
			add_bt_decorator_node(
				behavior_tree_name="BT_Enemy",
				decorator_type="BTDecorator_Cooldown",
				target_node_id="BTTask_PlaySound_0",
				node_name="5 Second Cooldown"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"behavior_tree_name": behavior_tree_name,
				"decorator_type": decorator_type,
				"target_node_id": target_node_id,
				"path": path
			}
			if node_name:
				params["node_name"] = node_name

			logger.info(f"Adding decorator ({decorator_type}) to node '{target_node_id}'")
			response = unreal.send_command("add_bt_decorator_node", params)

			if response and response.get("success"):
				logger.info(f"Created decorator: {response.get('node_id')}")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error adding decorator node: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def add_bt_service_node(
		ctx: Context,
		behavior_tree_name: str,
		service_type: str,
		target_node_id: str,
		path: str = "/Game/AI/BehaviorTrees",
		node_name: Optional[str] = None
	) -> Dict[str, Any]:
		"""
		Add a service (periodic update) to a BehaviorTree composite node.

		Services run periodically while their parent composite node is active.
		They can only be attached to Composite nodes (Selector/Sequence/SimpleParallel).

		Args:
			behavior_tree_name: Name of the target BehaviorTree
			service_type: Type of service:
				- "BTService_DefaultFocus": Set AI focus target
				- "BTService_RunEQS": Run Environment Query System query
				- "BTService_BlackboardBase": Base class for blackboard updates

				Custom Blueprint services:
				- Use the Blueprint name
				- Must exist at /Game/AI/Services/
			target_node_id: ID of the composite node to attach service to
			path: Content browser path where the BehaviorTree is located
			node_name: Optional display name for the service

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- behavior_tree_name: Name of the BehaviorTree
			- node_id: Unique ID of the created service
			- service_type: Type string that was specified
			- target_node_id: ID of the composite node it was attached to
			- node_name: Display name (if specified)

		Examples:
			# Add DefaultFocus service
			add_bt_service_node(
				behavior_tree_name="BT_Enemy",
				service_type="BTService_DefaultFocus",
				target_node_id="BTComposite_Selector_0",
				node_name="Focus On Target"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"behavior_tree_name": behavior_tree_name,
				"service_type": service_type,
				"target_node_id": target_node_id,
				"path": path
			}
			if node_name:
				params["node_name"] = node_name

			logger.info(f"Adding service ({service_type}) to composite '{target_node_id}'")
			response = unreal.send_command("add_bt_service_node", params)

			if response and response.get("success"):
				logger.info(f"Created service: {response.get('node_id')}")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error adding service node: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def connect_bt_nodes(
		ctx: Context,
		behavior_tree_name: str,
		parent_node_id: str,
		child_node_id: str,
		path: str = "/Game/AI/BehaviorTrees",
		child_index: int = -1
	) -> Dict[str, Any]:
		"""
		Connect two BehaviorTree nodes (parent-child relationship).

		This establishes the tree hierarchy. Parent must be a Composite node.
		Use "Root" as parent_node_id to set the root node of the tree.

		Args:
			behavior_tree_name: Name of the target BehaviorTree
			parent_node_id: ID of the parent node (must be Composite), or "Root"
			child_node_id: ID of the child node to connect
			path: Content browser path where the BehaviorTree is located
			child_index: Index to insert child at (-1 = append to end)

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- behavior_tree_name: Name of the BehaviorTree
			- parent_node_id: ID of the parent node
			- child_node_id: ID of the child node
			- child_index: Index where child was inserted (if specified)

		Examples:
			# Set root node
			connect_bt_nodes(
				behavior_tree_name="BT_Enemy",
				parent_node_id="Root",
				child_node_id="BTComposite_Selector_0"
			)

			# Connect task to selector
			connect_bt_nodes(
				behavior_tree_name="BT_Enemy",
				parent_node_id="BTComposite_Selector_0",
				child_node_id="BTTask_MoveTo_0"
			)

			# Insert at specific index
			connect_bt_nodes(
				behavior_tree_name="BT_Enemy",
				parent_node_id="BTComposite_Sequence_0",
				child_node_id="BTTask_Wait_0",
				child_index=0
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"behavior_tree_name": behavior_tree_name,
				"parent_node_id": parent_node_id,
				"child_node_id": child_node_id,
				"path": path,
				"child_index": child_index
			}

			logger.info(f"Connecting '{child_node_id}' to '{parent_node_id}' in BT '{behavior_tree_name}'")
			response = unreal.send_command("connect_bt_nodes", params)

			if response and response.get("success"):
				logger.info(f"Connected nodes successfully")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error connecting BT nodes: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def set_bt_node_property(
		ctx: Context,
		behavior_tree_name: str,
		node_id: str,
		property_name: str,
		property_value: Any,
		path: str = "/Game/AI/BehaviorTrees"
	) -> Dict[str, Any]:
		"""
		Set a property on a BehaviorTree node using reflection.

		This allows configuring node behavior (e.g., wait duration, blackboard keys).

		Args:
			behavior_tree_name: Name of the target BehaviorTree
			node_id: ID of the node to modify
			property_name: Name of the property to set
			property_value: Value to set (type depends on property - bool/int/float/string/object)
			path: Content browser path where the BehaviorTree is located

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- behavior_tree_name: Name of the BehaviorTree
			- node_id: ID of the modified node
			- property_name: Name of the property that was set

		Examples:
			# Set wait duration on a Wait task
			set_bt_node_property(
				behavior_tree_name="BT_Patrol",
				node_id="BTTask_Wait_0",
				property_name="WaitTime",
				property_value=3.0
			)

			# Set blackboard key on a MoveTo task
			set_bt_node_property(
				behavior_tree_name="BT_Enemy",
				node_id="BTTask_MoveTo_0",
				property_name="BlackboardKey",
				property_value={"SelectedKeyName": "TargetLocation"}
			)

			# Set acceptable radius
			set_bt_node_property(
				behavior_tree_name="BT_Enemy",
				node_id="BTTask_MoveTo_0",
				property_name="AcceptableRadius",
				property_value=100.0
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"behavior_tree_name": behavior_tree_name,
				"node_id": node_id,
				"property_name": property_name,
				"property_value": property_value,
				"path": path
			}

			logger.info(f"Setting property '{property_name}' on node '{node_id}'")
			response = unreal.send_command("set_bt_node_property", params)

			if response and response.get("success"):
				logger.info(f"Property set successfully")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error setting BT node property: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def delete_bt_node(
		ctx: Context,
		behavior_tree_name: str,
		node_id: str,
		path: str = "/Game/AI/BehaviorTrees"
	) -> Dict[str, Any]:
		"""
		Delete a node from a BehaviorTree.

		This removes the node from all parent relationships.
		If deleting the root node, it will be unset (tree becomes empty).

		Args:
			behavior_tree_name: Name of the target BehaviorTree
			node_id: ID of the node to delete
			path: Content browser path where the BehaviorTree is located

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- behavior_tree_name: Name of the BehaviorTree
			- deleted_node_id: ID of the deleted node

		Examples:
			# Delete a task node
			delete_bt_node(
				behavior_tree_name="BT_Enemy",
				node_id="BTTask_Wait_0"
			)

			# Delete a composite node (and its subtree)
			delete_bt_node(
				behavior_tree_name="BT_Patrol",
				node_id="BTComposite_Sequence_0"
			)
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"behavior_tree_name": behavior_tree_name,
				"node_id": node_id,
				"path": path
			}

			logger.info(f"Deleting node '{node_id}' from BT '{behavior_tree_name}'")
			response = unreal.send_command("delete_bt_node", params)

			if response and response.get("success"):
				logger.info(f"Node deleted successfully")

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error deleting BT node: {e}")
			return {"success": False, "error": str(e)}

	@mcp.tool()
	def list_bt_node_types(
		ctx: Context,
		category: str = "all"
	) -> Dict[str, Any]:
		"""
		List available BehaviorTree node types with descriptions.

		This helps discover what node types can be used with add_bt_*_node functions.

		Args:
			category: Filter by category:
				- "all": All node types (default)
				- "composite": Composite nodes only (Selector/Sequence/SimpleParallel)
				- "task": Task nodes only
				- "decorator": Decorator nodes only
				- "service": Service nodes only

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- composite_types: List of composite node types with descriptions
			- task_types: List of task node types with descriptions
			- decorator_types: List of decorator node types with descriptions
			- service_types: List of service node types with descriptions

		Examples:
			# List all node types
			list_bt_node_types()

			# List only task types
			list_bt_node_types(category="task")

			# List only decorator types
			list_bt_node_types(category="decorator")
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {"category": category}

			logger.info(f"Listing BT node types (category={category})")
			response = unreal.send_command("list_bt_node_types", params)

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error listing BT node types: {e}")
			return {"success": False, "error": str(e)}

	# ===== Utility Tools =====

	@mcp.tool()
	def list_ai_assets(
		ctx: Context,
		asset_type: str = "all",
		path_filter: Optional[str] = None
	) -> Dict[str, Any]:
		"""
		List AI-related assets (BehaviorTrees, Blackboards) in the project.

		Args:
			asset_type: Filter by asset type:
				- "all": All AI assets (default)
				- "behavior_tree": BehaviorTree assets only
				- "blackboard": Blackboard Data assets only
			path_filter: Filter by content path (e.g., "/Game/AI/")

		Returns:
			Dict containing:
			- success: Whether the operation succeeded
			- behavior_trees: List of BehaviorTree assets
			- blackboards: List of Blackboard assets
			- total_behavior_trees: Count of BehaviorTrees
			- total_blackboards: Count of Blackboards

		Example:
			list_ai_assets(asset_type="all", path_filter="/Game/AI/")
		"""
		from unreal_mcp_server import get_unreal_connection

		try:
			unreal = get_unreal_connection()
			if not unreal:
				return {"success": False, "error": "Failed to connect to Unreal Engine"}

			params = {
				"asset_type": asset_type,
				"path_filter": path_filter or ""
			}

			logger.info(f"Listing AI assets (type={asset_type})")
			response = unreal.send_command("list_ai_assets", params)

			return response or {"success": False, "error": "No response from Unreal Engine"}

		except Exception as e:
			logger.error(f"Error listing AI assets: {e}")
			return {"success": False, "error": str(e)}
