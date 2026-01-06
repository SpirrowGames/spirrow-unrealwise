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
