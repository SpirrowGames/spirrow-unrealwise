"""
Phase H Test: AIPerception & EQS Tools
Tests for AI Perception and Environment Query System functionality.

Prerequisites:
- Unreal Editor running with SpirrowBridge plugin
- MCPGameProject loaded
"""

import sys
import os
import time

# Add the Python directory to path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from unreal_mcp_server import get_unreal_connection


def print_result(test_name: str, result: dict):
    """Print test result in a readable format."""
    success = result.get("success", False)
    status = "✅ PASS" if success else "❌ FAIL"
    print(f"\n{status}: {test_name}")
    
    if success:
        # Print key info
        for key in ["blueprint_name", "asset_path", "component_name", "sense_type", 
                    "generator_type", "test_type", "generator_index", "test_index",
                    "total_count", "queries"]:
            if key in result:
                value = result[key]
                if key == "queries" and isinstance(value, list):
                    print(f"  {key}: {len(value)} items")
                    for q in value[:3]:  # Show first 3
                        print(f"    - {q.get('name', 'Unknown')}")
                else:
                    print(f"  {key}: {value}")
    else:
        print(f"  error: {result.get('error', 'Unknown error')}")
        if "error_code" in result:
            print(f"  error_code: {result.get('error_code')}")
    
    return success


def generate_unique_name(base_name: str) -> str:
    """Generate unique name with timestamp."""
    timestamp = int(time.time()) % 10000
    return f"{base_name}_{timestamp}"


def test_ai_perception():
    """Test AIPerception tools."""
    print("\n" + "="*60)
    print("Testing AIPerception Tools (Phase H-1)")
    print("="*60)
    
    unreal = get_unreal_connection()
    if not unreal:
        print("❌ Failed to connect to Unreal Engine")
        return False
    
    results = []
    test_path = "/Game/Blueprints/Test"
    controller_name = generate_unique_name("BP_TestAIController")
    character_name = generate_unique_name("BP_TestCharacter")
    
    # 1. Create test AIController Blueprint
    print("\n--- Setup: Creating test Blueprints ---")
    result = unreal.send_command("create_blueprint", {
        "name": controller_name,
        "parent_class": "AIController",
        "path": test_path
    })
    print_result("Create AIController Blueprint", result)
    
    result = unreal.send_command("create_blueprint", {
        "name": character_name,
        "parent_class": "Character",
        "path": test_path
    })
    print_result("Create Character Blueprint", result)
    
    # 2. Add AIPerceptionComponent
    print("\n--- Test: add_ai_perception_component ---")
    result = unreal.send_command("add_ai_perception_component", {
        "blueprint_name": controller_name,
        "component_name": "AIPerceptionComponent",
        "path": test_path
    })
    results.append(print_result("Add AIPerceptionComponent", result))
    
    # 3. Configure Sight sense
    print("\n--- Test: configure_sight_sense ---")
    result = unreal.send_command("configure_sight_sense", {
        "blueprint_name": controller_name,
        "sight_radius": 3000.0,
        "lose_sight_radius": 3500.0,
        "peripheral_vision_angle": 75.0,
        "detection_by_affiliation": {
            "enemies": True,
            "neutrals": True,
            "friendlies": False
        },
        "auto_success_range": 500.0,
        "max_age": 5.0,
        "path": test_path
    })
    results.append(print_result("Configure Sight Sense", result))
    
    # 4. Configure Hearing sense
    print("\n--- Test: configure_hearing_sense ---")
    result = unreal.send_command("configure_hearing_sense", {
        "blueprint_name": controller_name,
        "hearing_range": 5000.0,
        "detection_by_affiliation": {
            "enemies": True,
            "neutrals": False,
            "friendlies": False
        },
        "max_age": 3.0,
        "path": test_path
    })
    results.append(print_result("Configure Hearing Sense", result))
    
    # 5. Configure Damage sense
    print("\n--- Test: configure_damage_sense ---")
    result = unreal.send_command("configure_damage_sense", {
        "blueprint_name": controller_name,
        "max_age": 10.0,
        "path": test_path
    })
    results.append(print_result("Configure Damage Sense", result))
    
    # 6. Set dominant sense
    print("\n--- Test: set_perception_dominant_sense ---")
    result = unreal.send_command("set_perception_dominant_sense", {
        "blueprint_name": controller_name,
        "sense_type": "Sight",
        "path": test_path
    })
    results.append(print_result("Set Dominant Sense to Sight", result))
    
    # 7. Add perception stimuli source to character
    print("\n--- Test: add_perception_stimuli_source ---")
    result = unreal.send_command("add_perception_stimuli_source", {
        "blueprint_name": character_name,
        "register_as_source_for": ["Sight", "Hearing"],
        "auto_register": True,
        "path": test_path
    })
    results.append(print_result("Add Perception Stimuli Source", result))
    
    # Summary
    passed = sum(results)
    total = len(results)
    print(f"\n--- AIPerception Results: {passed}/{total} passed ---")
    
    return all(results)


def test_eqs():
    """Test EQS tools."""
    print("\n" + "="*60)
    print("Testing EQS Tools (Phase H-2)")
    print("="*60)
    
    unreal = get_unreal_connection()
    if not unreal:
        print("❌ Failed to connect to Unreal Engine")
        return False
    
    results = []
    test_path = "/Game/AI/EQS/Test"
    query_name = generate_unique_name("EQS_TestQuery")
    
    # 1. Create EQS Query
    print("\n--- Test: create_eqs_query ---")
    result = unreal.send_command("create_eqs_query", {
        "name": query_name,
        "path": test_path
    })
    results.append(print_result("Create EQS Query", result))
    
    # 2. Add SimpleGrid generator
    print("\n--- Test: add_eqs_generator (SimpleGrid) ---")
    result = unreal.send_command("add_eqs_generator", {
        "query_name": query_name,
        "generator_type": "SimpleGrid",
        "grid_size": 2000.0,
        "space_between": 200.0,
        "generate_around": "Querier",
        "path": test_path
    })
    results.append(print_result("Add SimpleGrid Generator", result))
    
    # 3. Add Distance test with scoring_factor (テスト作成時に設定)
    print("\n--- Test: add_eqs_test (Distance with scoring_factor=1.5) ---")
    result = unreal.send_command("add_eqs_test", {
        "query_name": query_name,
        "test_type": "Distance",
        "generator_index": 0,
        "distance_to": "Querier",
        "test_purpose": "Score",
        "scoring_equation": "Linear",
        "scoring_factor": 1.5,  # ScoringFactorをテスト作成時に設定
        "path": test_path
    })
    results.append(print_result("Add Distance Test (scoring_factor=1.5)", result))
    
    # 4. Add Trace test with scoring_factor
    print("\n--- Test: add_eqs_test (Trace with scoring_factor=2.0) ---")
    result = unreal.send_command("add_eqs_test", {
        "query_name": query_name,
        "test_type": "Trace",
        "generator_index": 0,
        "trace_from": "Item",
        "trace_to": "Querier",
        "trace_channel": "Visibility",
        "test_purpose": "Filter",
        "scoring_factor": 2.0,  # Filterでもscoring_factorは設定可能
        "path": test_path
    })
    results.append(print_result("Add Trace Test (scoring_factor=2.0)", result))
    
    # 5. Create another query with Donut generator
    print("\n--- Test: add_eqs_generator (Donut) ---")
    query_name2 = generate_unique_name("EQS_TestDonut")
    result = unreal.send_command("create_eqs_query", {
        "name": query_name2,
        "path": test_path
    })
    print_result("Create Second EQS Query", result)
    
    result = unreal.send_command("add_eqs_generator", {
        "query_name": query_name2,
        "generator_type": "Donut",
        "inner_radius": 500.0,
        "outer_radius": 1500.0,
        "generate_around": "Querier",
        "path": test_path
    })
    results.append(print_result("Add Donut Generator", result))
    
    # 6. Create OnCircle query
    print("\n--- Test: add_eqs_generator (OnCircle) ---")
    query_name3 = generate_unique_name("EQS_TestCircle")
    result = unreal.send_command("create_eqs_query", {
        "name": query_name3,
        "path": test_path
    })
    print_result("Create Third EQS Query", result)
    
    result = unreal.send_command("add_eqs_generator", {
        "query_name": query_name3,
        "generator_type": "OnCircle",
        "circle_radius": 800.0,
        "number_of_points": 12,
        "generate_around": "Querier",
        "path": test_path
    })
    results.append(print_result("Add OnCircle Generator", result))
    
    # 7. List EQS assets
    print("\n--- Test: list_eqs_assets ---")
    result = unreal.send_command("list_eqs_assets", {
        "path_filter": test_path
    })
    results.append(print_result("List EQS Assets", result))
    
    # Summary
    passed = sum(results)
    total = len(results)
    print(f"\n--- EQS Results: {passed}/{total} passed ---")
    
    return all(results)


def cleanup_test_assets():
    """Clean up test assets (optional)."""
    print("\n" + "="*60)
    print("Cleanup (Optional)")
    print("="*60)
    
    unreal = get_unreal_connection()
    if not unreal:
        return
    
    # List and delete test assets
    result = unreal.send_command("list_eqs_assets", {"path_filter": "/Game/AI/EQS/Test"})
    if result.get("success"):
        for query in result.get("queries", []):
            asset_path = query.get("asset_path", "").split(".")[0]  # Remove .AssetName suffix
            if asset_path:
                del_result = unreal.send_command("delete_asset", {"asset_path": asset_path})
                status = "✅" if del_result.get("success") else "⚠️"
                print(f"{status} Delete {asset_path}")


def main():
    """Run all Phase H tests."""
    print("\n" + "="*60)
    print("Phase H Test Suite: AIPerception & EQS")
    print("="*60)
    
    perception_ok = test_ai_perception()
    eqs_ok = test_eqs()
    
    print("\n" + "="*60)
    print("Final Results")
    print("="*60)
    print(f"AIPerception: {'✅ PASS' if perception_ok else '❌ FAIL'}")
    print(f"EQS:          {'✅ PASS' if eqs_ok else '❌ FAIL'}")
    
    if perception_ok and eqs_ok:
        print("\n🎉 All Phase H tests passed!")
        return 0
    else:
        print("\n⚠️ Some tests failed. Check the output above.")
        return 1


if __name__ == "__main__":
    sys.exit(main())
