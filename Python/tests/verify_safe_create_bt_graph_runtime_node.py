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
    # NOTE: this test returns (ok, node_id); every exit must be a 2-tuple or the
    # caller's `okA, parent_id = ...` unpack raises TypeError instead of reporting FAIL.
    if not unreal: return False, None
    if not ensure_bt(unreal): return False, None

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


def test_C_simple_parallel_creation_succeeds(parent_node_id):
    """SimpleParallel takes a different graph-node type (UBehaviorTreeGraphNode_SimpleParallel)
    and therefore a different explicit instantiation of the primitive (v0.11.1 migration)."""
    print("\n--- C) add_bt_composite_node (SimpleParallel) via primitive ---")
    if not parent_node_id:
        print("  SKIP: no parent_node_id from test A"); return False, None
    unreal = get_unreal_connection()
    if not unreal: return False, None

    r = unreal.send_command("add_bt_composite_node", {
        "behavior_tree_name": BT_NAME,
        "path": BT_PATH,
        "node_type": "SimpleParallel",
        "parent_node_id": parent_node_id,
        "node_name": "PrimitiveVerify_Parallel",
    })
    print(f"  status={r.get('status')}")
    data = get_data(r)
    node_id = data.get("node_id")
    print(f"  node_id={node_id}, node_class={data.get('node_class')}")
    ok = r.get("status") == "success" and bool(node_id)
    print("  PASS" if ok else f"  FAIL: {data}")
    return ok, node_id


def test_D_child_task_attaches_to_simple_parallel(parent_node_id):
    print("\n--- D) add_bt_task_node under SimpleParallel confirms its 2-layer wiring ---")
    if not parent_node_id:
        print("  SKIP: no parent_node_id from test C"); return False
    unreal = get_unreal_connection()
    if not unreal: return False

    r = unreal.send_command("add_bt_task_node", {
        "behavior_tree_name": BT_NAME,
        "path": BT_PATH,
        "task_type": "BTTask_Wait",
        "parent_node_id": parent_node_id,
        "node_name": "PrimitiveVerify_ParallelWait",
    })
    print(f"  status={r.get('status')}")
    data = get_data(r)
    print(f"  task node_id={data.get('node_id')}, connected={data.get('connected')}")
    ok = r.get("status") == "success" and data.get("connected") is True
    print("  PASS" if ok else f"  FAIL (parent connection requires 2-layer invariant): {data}")
    return ok


def test_E_subtree_task_creation_succeeds(parent_node_id):
    """BTTask_RunBehavior routes through the bIsSubtreeTask branch
    (UBehaviorTreeGraphNode_SubtreeTask), the 4th explicit instantiation."""
    print("\n--- E) add_bt_task_node (BTTask_RunBehavior = subtree branch) via primitive ---")
    if not parent_node_id:
        print("  SKIP: no parent_node_id from test A"); return False
    unreal = get_unreal_connection()
    if not unreal: return False

    r = unreal.send_command("add_bt_task_node", {
        "behavior_tree_name": BT_NAME,
        "path": BT_PATH,
        "task_type": "BTTask_RunBehavior",
        "parent_node_id": parent_node_id,
        "node_name": "PrimitiveVerify_Subtree",
    })
    print(f"  status={r.get('status')}")
    data = get_data(r)
    print(f"  subtree node_id={data.get('node_id')}, connected={data.get('connected')}")
    ok = r.get("status") == "success" and bool(data.get("node_id"))
    print("  PASS" if ok else f"  FAIL: {data}")
    return ok


def test_F_list_bt_nodes_includes_all():
    print("\n--- F) list_bt_nodes shows every node created via the primitive ---")
    unreal = get_unreal_connection()
    if not unreal: return False

    r = unreal.send_command("list_bt_nodes", {
        "behavior_tree_name": BT_NAME,
        "path": BT_PATH,
    })
    print(f"  status={r.get('status')}")
    data = get_data(r)
    raw = str(data)
    expected = [
        "BTComposite_Sequence",        # A) Composite instantiation
        "BTTask_Wait",                 # B/D) Task instantiation
        "BTComposite_SimpleParallel",  # C) SimpleParallel instantiation
        "BTTask_RunBehavior",          # E) SubtreeTask instantiation
    ]
    missing = [e for e in expected if e not in raw]
    ok = not missing
    print("  PASS" if ok else f"  FAIL: missing {missing} (first 500 chars): {raw[:500]}")
    return ok


def main():
    print("=" * 60)
    print("Issue #14 primitive verification: SafeCreateBTGraphAndRuntimeNode")
    print("Covers all 4 explicit instantiations (v0.11.1: all BT branches migrated)")
    print("=" * 60)

    okA, seq_id = test_A_composite_creation_succeeds()
    okB = test_B_child_task_attaches_to_composite(seq_id) if okA else False
    okC, parallel_id = test_C_simple_parallel_creation_succeeds(seq_id) if okA else (False, None)
    okD = test_D_child_task_attaches_to_simple_parallel(parallel_id) if okC else False
    okE = test_E_subtree_task_creation_succeeds(seq_id) if okA else False
    okF = test_F_list_bt_nodes_includes_all() if okA else False

    print("\n" + "=" * 60); print("SUMMARY"); print("=" * 60)
    summary = [
        ("A) Composite creation succeeds",        okA),
        ("B) child task connects (2-layer)",      okB),
        ("C) SimpleParallel creation succeeds",   okC),
        ("D) SimpleParallel child connects",      okD),
        ("E) SubtreeTask creation succeeds",      okE),
        ("F) list_bt_nodes contains all 4 kinds", okF),
    ]
    for name, ok in summary:
        print(f"  [{'PASS' if ok else 'FAIL'}] {name}")
    passed = sum(1 for _, ok in summary if ok)
    print(f"\n{passed}/{len(summary)} passed")
    return 0 if passed == len(summary) else 1


if __name__ == "__main__":
    sys.exit(main())
