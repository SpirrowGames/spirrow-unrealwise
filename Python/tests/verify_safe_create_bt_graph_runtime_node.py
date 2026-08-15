"""Verification for Issue #14 primitive: SafeCreateBTGraphAndRuntimeNode.

The migrated お手本 handler (HandleAddBTCompositeNode, regular-Composite branch
in SpirrowBridgeAICommands_BTNodeCreation.cpp) now routes through
SpirrowBridgePrimitives::SafeCreateBTGraphAndRuntimeNode. The 2-layer
invariant under test:

  1. GraphNode (UBehaviorTreeGraphNode_Composite) is created via
     FGraphNodeCreator inside the primitive.
  2. Runtime NodeInstance (UBTCompositeNode subclass) is created with
     the GraphNode as its Outer (NOT the BTGraph or BehaviorTree).
  3. FGraphNodeCreator::Finalize() is called *after* NodeInstance wiring,
     so AllocateDefaultPins runs with NodeInstance already attached.

If any of these are violated, child connection (which inspects
GraphNode->NodeInstance and parent pins) silently fails or crashes.

Prereq:
  - MCPGameProject editor running with rebuilt SpirrowBridge plugin
  - MCP server reachable via get_unreal_connection()
"""
import sys, os
try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_server import get_unreal_connection


BT_NAME = "BT_PrimitiveIOLayerVerify"
BT_PATH = "/Game/Test/AI"


def get_data(r):
    if not isinstance(r, dict): return {}
    inner = r.get("result", r)
    return inner.get("data", inner) if isinstance(inner, dict) else {}


def ensure_bt(unreal):
    r = unreal.send_command("create_behavior_tree", {
        "name": BT_NAME,
        "path": BT_PATH,
    })
    status = r.get("status")
    msg = str(get_data(r))
    print(f"  create_behavior_tree → status={status}")
    return status == "success" or "already exists" in msg.lower()


def test_A_composite_creation_succeeds():
    print("\n--- A) add_bt_composite_node (Sequence) returns success + node_id ---")
    unreal = get_unreal_connection()
    if not unreal: return False
    if not ensure_bt(unreal): return False

    r = unreal.send_command("add_bt_composite_node", {
        "behavior_tree_name": BT_NAME,
        "path": BT_PATH,
        "node_type": "Sequence",
        "parent_node_id": "Root",
        "node_name": "PrimitiveVerify_Seq",
    })
    print(f"  status={r.get('status')}")
    data = get_data(r)
    node_id = data.get("node_id")
    print(f"  node_id={node_id}, node_class={data.get('node_class')}")
    ok = r.get("status") == "success" and bool(node_id)
    print("  PASS" if ok else f"  FAIL: {data}")
    return ok, node_id


def test_B_child_task_attaches_to_composite(parent_node_id):
    print("\n--- B) add_bt_task_node under the composite confirms 2-layer wiring ---")
    if not parent_node_id:
        print("  SKIP: no parent_node_id from test A"); return False
    unreal = get_unreal_connection()
    if not unreal: return False

    r = unreal.send_command("add_bt_task_node", {
        "behavior_tree_name": BT_NAME,
        "path": BT_PATH,
        "task_type": "BTTask_Wait",
        "parent_node_id": parent_node_id,
        "node_name": "PrimitiveVerify_Wait",
    })
    print(f"  status={r.get('status')}")
    data = get_data(r)
    print(f"  task node_id={data.get('node_id')}, connected={data.get('connected')}")
    # connected=true requires GraphNode->NodeInstance to be valid (= 2-layer invariant)
    ok = r.get("status") == "success" and data.get("connected") is True
    print("  PASS" if ok else f"  FAIL (parent connection requires 2-layer invariant): {data}")
    return ok


def test_C_list_bt_nodes_includes_both():
    print("\n--- C) list_bt_nodes shows the composite + task created via primitive ---")
    unreal = get_unreal_connection()
    if not unreal: return False

    r = unreal.send_command("list_bt_nodes", {
        "behavior_tree_name": BT_NAME,
        "path": BT_PATH,
    })
    print(f"  status={r.get('status')}")
    data = get_data(r)
    raw = str(data)
    ok = ("BTComposite_Sequence" in raw) and ("BTTask_Wait" in raw)
    print("  PASS" if ok else f"  FAIL: nodes missing (first 500 chars): {raw[:500]}")
    return ok


def main():
    print("=" * 60)
    print("Issue #14 primitive verification: SafeCreateBTGraphAndRuntimeNode")
    print("=" * 60)

    okA, parent_id = test_A_composite_creation_succeeds()
    okB = test_B_child_task_attaches_to_composite(parent_id) if okA else False
    okC = test_C_list_bt_nodes_includes_both() if okA else False

    print("\n" + "=" * 60); print("SUMMARY"); print("=" * 60)
    summary = [
        ("A) composite creation succeeds",    okA),
        ("B) child task connects (2-layer)",  okB),
        ("C) list_bt_nodes contains both",    okC),
    ]
    for name, ok in summary:
        print(f"  [{'PASS' if ok else 'FAIL'}] {name}")
    passed = sum(1 for _, ok in summary if ok)
    print(f"\n{passed}/{len(summary)} passed")
    return 0 if passed == len(summary) else 1


if __name__ == "__main__":
    sys.exit(main())
