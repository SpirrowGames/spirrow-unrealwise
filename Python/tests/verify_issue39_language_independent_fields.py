"""Verification for Issue #39.

Engine-originated text in responses (compiler messages, the schema's reason for
refusing a connection, node titles) comes back in the editor's display
language. A caller needs something else to test. This checks the fields that
do not depend on the language:

1. Error responses keep "error_code" and "details" (the bridge used to forward
   only the message string).
2. compile_blueprint: messages[] with severity / node_id / node_name / graph.
3. connect_blueprint_nodes: details.reason and the two pin types.
4. get_blueprint_graph: "name" next to "title".

No check here reads a message text, so the script passes in any editor
language. It prints the texts it got, to show what the language was.

Prereq: MCPGameProject editor running with the rebuilt SpirrowBridge plugin.
The test Blueprint is recreated on every run.
"""
import sys, os
try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_server import get_unreal_connection


BP_PATH = "/Game/Test"
BP_NAME = "BP_Issue39_Verify"

RESULTS = []


def data(r):
    if not isinstance(r, dict):
        return {}
    inner = r.get("result", r)
    return inner.get("data", inner) if isinstance(inner, dict) else {}


def is_error(r):
    return not isinstance(r, dict) or r.get("status") == "error" or data(r).get("success") is False


def send(command, params):
    unreal = get_unreal_connection()
    if not unreal:
        raise RuntimeError("no connection to the editor")
    r = unreal.send_command(command, params)
    if r is None:
        raise RuntimeError(f"{command}: no response (editor crashed or timed out)")
    return r


def bp(command, **params):
    params.update({"blueprint_name": BP_NAME, "path": BP_PATH})
    return send(command, params)


def check(name, cond, detail=""):
    RESULTS.append((name, bool(cond)))
    print(f"  [{'PASS' if cond else 'FAIL'}] {name}" + (f"  ({detail})" if detail and not cond else ""))


def node_id(r):
    nid = data(r).get("node_id")
    if not nid:
        raise RuntimeError(f"node creation failed: {r}")
    return nid


def setup():
    send("delete_asset", {"asset_path": f"{BP_PATH}/{BP_NAME}"})
    r = send("create_blueprint", {"name": BP_NAME, "parent_class": "Actor", "path": BP_PATH})
    if is_error(r):
        raise RuntimeError(f"create_blueprint failed: {r}")
    found = data(bp("find_blueprint_nodes", node_type="Event", event_type="ReceiveBeginPlay")).get("nodes", [])
    ev = found[0]["node_id"] if found else node_id(bp("add_blueprint_event_node", event_name="ReceiveBeginPlay"))
    return ev


def test_graph_names(ev, call_node):
    print("\n--- get_blueprint_graph: 'name' next to 'title' ---")
    d = data(bp("get_blueprint_graph"))
    # This command wraps its payload in one more "result" object.
    nodes = d.get("nodes") or d.get("result", {}).get("nodes", [])
    by_id = {n.get("id"): n for n in nodes}
    check("every node has a non-empty name", bool(nodes) and all(n.get("name") for n in nodes),
          str([n.get("name") for n in nodes]))
    check("every node still has a title", all("title" in n for n in nodes))
    check("BeginPlay node: name == ReceiveBeginPlay", by_id.get(ev, {}).get("name") == "ReceiveBeginPlay",
          str(by_id.get(ev)))
    check("function node: name == Vector_Normalize", by_id.get(call_node, {}).get("name") == "Vector_Normalize",
          str(by_id.get(call_node, {}).get("name")))
    print(f"    title as shown by this editor: {by_id.get(ev, {}).get('title')!r}")


def test_compile_messages(ev, bad):
    print("\n--- compile_blueprint: structured messages ---")
    r = bp("compile_blueprint")
    check("clean compile: 'messages' is a list", not is_error(r) and isinstance(data(r).get("messages"), list), str(r))

    # Vector_Normalize takes its vector by reference; with nothing wired into
    # it, the compiler refuses the node.
    bp("connect_blueprint_nodes", source_node_id=ev, source_pin="then", target_node_id=bad, target_pin="execute")
    r = bp("compile_blueprint")
    details = r.get("details") if isinstance(r, dict) else None
    check("compile error is an error response", is_error(r), str(r))
    check("error_code 1201 reaches the caller", isinstance(r, dict) and r.get("error_code") == 1201, str(r.get("error_code")))
    check("details reaches the caller", isinstance(details, dict), str(r)[:300])
    details = details or {}
    check("details.status == Error", details.get("status") == "Error", str(details.get("status")))
    check("details.num_errors >= 1", details.get("num_errors", 0) >= 1, str(details.get("num_errors")))
    msgs = details.get("messages", [])
    errors = [m for m in msgs if m.get("severity") == "error"]
    check("messages has as many errors as num_errors", len(errors) == details.get("num_errors") and len(errors) > 0,
          f"{len(errors)} vs {details.get('num_errors')}")
    on_node = [m for m in errors if m.get("node_id") == bad]
    check("an error names the offending node by id", len(on_node) >= 1, str(msgs))
    m = on_node[0] if on_node else {}
    check("... node_name == Vector_Normalize", m.get("node_name") == "Vector_Normalize", str(m))
    check("... node_class == K2Node_CallFunction", m.get("node_class") == "K2Node_CallFunction", str(m))
    check("... graph == EventGraph", m.get("graph") == "EventGraph", str(m))
    check("every message has severity and message",
          all(x.get("severity") in ("error", "warning", "performance_warning", "info") and "message" in x for x in msgs),
          str(msgs))
    for x in msgs:
        print(f"    [{x.get('severity')}] node={x.get('node_name')} pin={x.get('pin_name')} text={x.get('message')!r}")

    bp("disconnect_blueprint_nodes", node_id=bad)
    r = bp("compile_blueprint")
    check("compiles again once the node is unwired", not is_error(r), str(r))


def test_connect_reason(ev, vec_node):
    print("\n--- connect_blueprint_nodes: reason code ---")
    flag = node_id(bp("add_blueprint_function_node", target="KismetMathLibrary", function_name="Not_PreBool",
                      node_position=[300, 300]))
    size = node_id(bp("add_blueprint_function_node", target="KismetMathLibrary", function_name="VSize",
                      node_position=[600, 300]))

    r = bp("connect_blueprint_nodes", source_node_id=flag, source_pin="ReturnValue", target_node_id=size, target_pin="A")
    d = r.get("details") or {}
    check("bool -> vector is refused", is_error(r), str(r))
    check("error_code 1215 reaches the caller", r.get("error_code") == 1215, str(r.get("error_code")))
    check("details.reason == schema_disallow", d.get("reason") == "schema_disallow", str(d))
    check("details.schema_response == CONNECT_RESPONSE_DISALLOW", d.get("schema_response") == "CONNECT_RESPONSE_DISALLOW", str(d))
    check("details.source_pin_type == bool", d.get("source_pin_type") == "bool", str(d))
    check("details.target_pin_type names Vector", "Vector" in str(d.get("target_pin_type")), str(d))
    check("details echoes node ids and pin names",
          d.get("source_node_id") == flag and d.get("target_node_id") == size
          and d.get("source_pin") == "ReturnValue" and d.get("target_pin") == "A", str(d))
    print(f"    schema text in this editor: {r.get('error')!r}")

    r = bp("connect_blueprint_nodes", source_node_id=flag, source_pin="NoSuchPin", target_node_id=size, target_pin="A")
    check("unknown source pin: reason == source_pin_not_found",
          is_error(r) and (r.get("details") or {}).get("reason") == "source_pin_not_found", str(r))
    r = bp("connect_blueprint_nodes", source_node_id=flag, source_pin="ReturnValue", target_node_id=size, target_pin="NoSuchPin")
    d = r.get("details") or {}
    check("unknown target pin: reason == target_pin_not_found", is_error(r) and d.get("reason") == "target_pin_not_found", str(r))
    check("unknown target pin: source_pin_type still given, no target_pin_type",
          d.get("source_pin_type") == "bool" and "target_pin_type" not in d, str(d))

    r = bp("connect_blueprint_nodes", source_node_id=ev, source_pin="then", target_node_id=vec_node, target_pin="execute")
    check("a valid connection still succeeds, without details", not is_error(r) and "details" not in r, str(r))
    bp("disconnect_blueprint_nodes", node_id=vec_node)


def test_other_errors_keep_code():
    print("\n--- error_code on an unrelated error ---")
    r = send("compile_blueprint", {"blueprint_name": "BP_Issue39_DoesNotExist", "path": BP_PATH})
    check("missing Blueprint: error_code 1200", is_error(r) and r.get("error_code") == 1200, str(r))
    check("missing Blueprint: details.blueprint_name", (r.get("details") or {}).get("blueprint_name") == "BP_Issue39_DoesNotExist", str(r))
    r = send("ping", {})
    check("success responses are unchanged (no error_code)", r.get("status") == "success" and "error_code" not in r, str(r))


def main():
    print("=" * 70)
    print("Issue #39 verification")
    print("=" * 70)
    ev = setup()
    bad = node_id(bp("add_blueprint_function_node", target="KismetMathLibrary", function_name="Vector_Normalize",
                     node_position=[300, 0]))
    test_graph_names(ev, bad)
    test_compile_messages(ev, bad)
    test_connect_reason(ev, bad)
    test_other_errors_keep_code()

    passed = sum(1 for _, c in RESULTS if c)
    print("\n" + "=" * 70)
    print(f"{passed}/{len(RESULTS)} checks passed")
    for name, c in RESULTS:
        if not c:
            print(f"  FAILED: {name}")
    return 0 if passed == len(RESULTS) and RESULTS else 1


if __name__ == "__main__":
    sys.exit(main())
