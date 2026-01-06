"""
EQS Tools for Unreal MCP - Environment Query System operations.

This module provides tools for working with Unreal Engine's EQS (Environment Query System).
Phase H-2 implementation.
"""

import logging
from typing import Dict, Any, List, Optional
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("SpirrowBridge")


def register_eqs_tools(mcp: FastMCP):
    """Register EQS tools with the MCP server."""

    @mcp.tool()
    def create_eqs_query(
        ctx: Context,
        name: str,
        path: str = "/Game/AI/EQS"
    ) -> Dict[str, Any]:
        """
        Create a new EQS (Environment Query System) Query asset.

        EQS queries are used by AI to evaluate and score locations or actors
        in the environment for decision making.

        Args:
            name: Name of the EQS Query (e.g., "EQS_FindCover", "EQS_PatrolPoints")
            path: Content browser path for the asset

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - name: Name of the created query
            - asset_path: Full path to the created asset
            - option_count: Number of options (initially 0)

        Example:
            create_eqs_query(
                name="EQS_FindAmbushPoint",
                path="/Game/AI/EQS"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {"name": name, "path": path}
            logger.info(f"Creating EQS Query: {name}")
            response = unreal.send_command("create_eqs_query", params)

            if response and response.get("success"):
                logger.info(f"Created EQS Query: {response.get('asset_path')}")

            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error creating EQS Query: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def add_eqs_generator(
        ctx: Context,
        query_name: str,
        generator_type: str,
        grid_size: float = 1000.0,
        space_between: float = 100.0,
        inner_radius: float = 300.0,
        outer_radius: float = 1000.0,
        circle_radius: float = 500.0,
        number_of_points: int = 8,
        searched_actor_class: Optional[str] = None,
        generate_around: str = "Querier",
        path: str = "/Game/AI/EQS"
    ) -> Dict[str, Any]:
        """
        Add a Generator to an EQS Query.

        Generators create the initial set of items (locations or actors) that
        tests will evaluate.

        Args:
            query_name: Name of the target EQS Query
            generator_type: Type of generator to add:
                - "SimpleGrid": Grid of points around a context
                - "Donut": Ring of points with inner/outer radius
                - "OnCircle": Points on a circle perimeter
                - "ActorsOfClass": All actors of a specific class
                - "CurrentLocation": Single point at current location
                - "PathingGrid": Grid using navigation mesh
            grid_size: Size of grid for SimpleGrid/PathingGrid (default: 1000)
            space_between: Spacing between points (default: 100)
            inner_radius: Inner radius for Donut (default: 300)
            outer_radius: Outer radius for Donut (default: 1000)
            circle_radius: Radius for OnCircle (default: 500)
            number_of_points: Number of points for OnCircle (default: 8)
            searched_actor_class: Class path for ActorsOfClass generator
            generate_around: Context to generate around (default: "Querier")
            path: Content browser path where the EQS Query is located

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - query_name: Name of the modified query
            - generator_type: Type of generator added
            - generator_index: Index of the new generator

        Example:
            # Create a grid of points around the AI
            add_eqs_generator(
                query_name="EQS_FindCover",
                generator_type="SimpleGrid",
                grid_size=2000.0,
                space_between=200.0,
                path="/Game/AI/EQS"
            )

            # Create a donut around the AI
            add_eqs_generator(
                query_name="EQS_FlankPositions",
                generator_type="Donut",
                inner_radius=500.0,
                outer_radius=1500.0,
                path="/Game/AI/EQS"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "query_name": query_name,
                "generator_type": generator_type,
                "grid_size": grid_size,
                "space_between": space_between,
                "inner_radius": inner_radius,
                "outer_radius": outer_radius,
                "circle_radius": circle_radius,
                "number_of_points": number_of_points,
                "generate_around": generate_around,
                "path": path
            }

            if searched_actor_class:
                params["searched_actor_class"] = searched_actor_class

            logger.info(f"Adding {generator_type} generator to {query_name}")
            response = unreal.send_command("add_eqs_generator", params)

            if response and response.get("success"):
                logger.info(f"Added generator at index {response.get('generator_index')}")

            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error adding EQS generator: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def add_eqs_test(
        ctx: Context,
        query_name: str,
        test_type: str,
        generator_index: int = 0,
        distance_to: str = "Querier",
        trace_from: str = "Querier",
        trace_to: str = "Item",
        trace_channel: str = "Visibility",
        test_purpose: str = "Score",
        scoring_equation: str = "Linear",
        scoring_factor: float = 1.0,
        path: str = "/Game/AI/EQS"
    ) -> Dict[str, Any]:
        """
        Add a Test to an EQS Query Generator.

        Tests evaluate and score each item from the generator. They can filter
        out invalid items or assign scores for ranking.

        Args:
            query_name: Name of the target EQS Query
            test_type: Type of test to add:
                - "Distance": Measure distance to a context
                - "Trace": Check line-of-sight visibility
                - "Dot": Calculate dot product (direction)
                - "Pathfinding"/"PathExists": Check navigation path exists
                - "GameplayTags": Compare gameplay tags
                - "Overlap": Check for overlapping geometry
            generator_index: Index of the generator to add test to (default: 0)
            distance_to: Context for distance measurement (default: "Querier")
            trace_from: Start context for trace (default: "Querier")
            trace_to: End context for trace (default: "Item")
            trace_channel: Collision channel for trace (default: "Visibility")
            test_purpose: How to use test results:
                - "Score": Use for scoring only
                - "Filter": Use for filtering only
                - "FilterAndScore": Use for both
            scoring_equation: How to calculate scores:
                - "Linear": Proportional scoring
                - "Square": Squared scoring
                - "InverseLinear": Inverse proportional
                - "Constant": Fixed score
                - "SquareRoot": Square root scoring
            scoring_factor: Multiplier for scores (default: 1.0)
            path: Content browser path where the EQS Query is located

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - query_name: Name of the modified query
            - test_type: Type of test added
            - generator_index: Index of the generator
            - test_index: Index of the new test

        Example:
            # Add distance test - prefer locations far from player
            add_eqs_test(
                query_name="EQS_FindCover",
                test_type="Distance",
                distance_to="Player",
                test_purpose="Score",
                scoring_equation="Linear",
                scoring_factor=1.0,
                path="/Game/AI/EQS"
            )

            # Add trace test - filter locations with line of sight to target
            add_eqs_test(
                query_name="EQS_FindCover",
                test_type="Trace",
                trace_from="Item",
                trace_to="EnemyActor",
                test_purpose="Filter",
                path="/Game/AI/EQS"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "query_name": query_name,
                "test_type": test_type,
                "generator_index": generator_index,
                "distance_to": distance_to,
                "trace_from": trace_from,
                "trace_to": trace_to,
                "trace_channel": trace_channel,
                "test_purpose": test_purpose,
                "scoring_equation": scoring_equation,
                "scoring_factor": scoring_factor,
                "path": path
            }

            logger.info(f"Adding {test_type} test to {query_name}")
            response = unreal.send_command("add_eqs_test", params)

            if response and response.get("success"):
                logger.info(f"Added test at index {response.get('test_index')}")

            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error adding EQS test: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def set_eqs_test_property(
        ctx: Context,
        query_name: str,
        generator_index: int,
        test_index: int,
        property_name: str,
        property_value: Any,
        path: str = "/Game/AI/EQS"
    ) -> Dict[str, Any]:
        """
        Set a property on an EQS Test.

        Allows fine-tuning of test behavior through property modification.

        Args:
            query_name: Name of the target EQS Query
            generator_index: Index of the generator containing the test
            test_index: Index of the test to modify
            property_name: Name of the property to set
            property_value: Value to set for the property
            path: Content browser path where the EQS Query is located

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - query_name: Name of the modified query
            - property_name: Name of the modified property
            - generator_index: Index of the generator
            - test_index: Index of the test

        Example:
            set_eqs_test_property(
                query_name="EQS_FindCover",
                generator_index=0,
                test_index=0,
                property_name="ScoringFactor",
                property_value=2.0,
                path="/Game/AI/EQS"
            )
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {
                "query_name": query_name,
                "generator_index": generator_index,
                "test_index": test_index,
                "property_name": property_name,
                "property_value": property_value,
                "path": path
            }

            logger.info(f"Setting {property_name} on {query_name} test")
            response = unreal.send_command("set_eqs_test_property", params)

            if response and response.get("success"):
                logger.info(f"Set property {property_name}")

            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error setting EQS test property: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def list_eqs_assets(
        ctx: Context,
        path_filter: Optional[str] = None
    ) -> Dict[str, Any]:
        """
        List EQS Query assets in the project.

        Args:
            path_filter: Optional path to filter results (e.g., "/Game/AI/EQS/")

        Returns:
            Dict containing:
            - success: Whether the operation succeeded
            - queries: List of EQS query info objects:
                - name: Query asset name
                - path: Package path
                - asset_path: Full asset path
                - generator_count: Number of generators
                - test_count: Total number of tests
            - total_count: Total number of queries found

        Example:
            # List all EQS queries
            list_eqs_assets()

            # List queries in a specific folder
            list_eqs_assets(path_filter="/Game/AI/EQS/Combat")
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            params = {}
            if path_filter:
                params["path_filter"] = path_filter

            logger.info("Listing EQS assets")
            response = unreal.send_command("list_eqs_assets", params)

            if response and response.get("success"):
                count = response.get("total_count", 0)
                logger.info(f"Found {count} EQS queries")

            return response or {"success": False, "error": "No response from Unreal Engine"}

        except Exception as e:
            logger.error(f"Error listing EQS assets: {e}")
            return {"success": False, "error": str(e)}
