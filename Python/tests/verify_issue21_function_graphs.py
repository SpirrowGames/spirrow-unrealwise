"""Verification for Issue #21: function graphs.

- A) add_function_override creates a function-graph override for parent
     functions with a return value (Character.CanJumpInternal,
     GameModeBase.ChoosePlayerStart): entry + result nodes with the parent's
     signature. Lookup by editor name (CanJump) works. Event-style functions,
     unknown functions and a second override are errors.
- B) add_function_graph creates a user function with typed inputs / outputs
     (containers included), optionally pure; bad input is rejected before
     anything is created.
- C) node commands take graph_name: add / connect / find / set pin / delete
     work inside the function graph, the event graph is untouched, an unknown
     graph_name is an error that lists the graphs.
- D) list_blueprint_graphs reports the event graph and the function graphs.
- E) when compile_blueprint reports results (Issue #19):
     E1 the override that returns a literal compiles
     E2 the user functions compile and can be called from the event graph;
        the pure one has no exec pins on its call node

Everything uses engine classes only.
Prereq: MCPGameProject editor running with the rebuilt SpirrowBridge plugin.
"""
import sys, os
try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_server import get_unreal_connection


BP_PATH = "/Game/Test"
RESULTS = []
SKIPPED = []


def data(r):
    if not isinstance(r, dict):
        return {}
    inner = r.get("result", r)
    return inner.get("data", inner) if isinstance(inner, dict) else {}


def is_error(r):
    return not isinstance(r, dict) or r.get("status") == "error" or data(r).get("success") is False


def err(r):
    return str(r.get("error", "")) if isinstance(r, dict) else ""


def send(command, params):
    unreal = get_unreal_connection()
    if not unreal:
        raise RuntimeError("no connection to the editor")
    r = unreal.send_command(command, params)
    if r is None:
        raise RuntimeError(f"{command}: no response (editor crashed or timed out)")
    return r


def bp(command, name, **params):
    params.update({"blueprint_name": name, "path": BP_PATH})
    return send(command, params)


def check(name, cond, detail=""):
    RESULTS.append((name, bool(cond)))
    print(f"  [{'PASS' if cond else 'FAIL'}] {name}" + (f"  ({detail})" if detail and not cond else ""))


def skip(name, why):
    SKIPPED.append(name)
    print(f"  [SKIP] {name}  ({why})")


def recreate(name, parent):
    send("delete_asset", {"asset_path": f"{BP_PATH}/{name}"})
    r = send("create_blueprint", {"name": name, "parent_class": parent, "path": BP_PATH})
    if is_error(r):
        raise RuntimeError(f"create_blueprint {name} failed: {r}")


def nodes(name, graph=None, **flt):
    if graph:
        flt["graph_name"] = graph
    return data(bp("find_blueprint_nodes", name, **flt)).get("nodes", [])


def pin(name, graph, node_id, pin_name):
    for n in nodes(name, graph):
        if n["node_id"] == node_id:
            for p in n["pins"]:
                if p["name"] == pin_name:
                    return p
    return {}


def pin_types(pins):
    return {p["name"]: p["full_type"] for p in pins or []}


def compile_result(name):
    r = bp("compile_blueprint", name)
    if is_error(r):
        return ("failed to compile" in err(r)), False, err(r)
    d = data(r)
    return ("status" in d), d.get("status") in ("UpToDate", "UpToDateWithWarnings"), str(d)


def ensure_exec(name, graph, entry, result):
    """Entry 'then' -> result 'execute', unless the engine already linked them."""
    if not pin(name, graph, result, "execute").get("connected"):
        r = bp("connect_blueprint_nodes", name, graph_name=graph, source_node_id=entry, source_pin="then",
               target_node_id=result, target_pin="execute")
        return not is_error(r)
    return True


# --------------------------------------------------------------------------
def test_overrides():
    print("\n--- A) function overrides ---")
    name = "BP_Issue21_Character"
    recreate(name, "Character")

    r = bp("add_function_override", name, function_name="CanJumpInternal")
    d = data(r)
    print(f"  CanJumpInternal: entry_pins={pin_types(d.get('entry_pins'))} result_pins={pin_types(d.get('result_pins'))}")
    check("A) Character.CanJumpInternal override created", not is_error(r) and d.get("is_override") is True, str(r))
    check("A) graph is named after the function", d.get("graph_name") == "CanJumpInternal", str(d.get("graph_name")))
    check("A) entry and result nodes exist", bool(d.get("entry_node_id")) and bool(d.get("result_node_id")), str(d))
    check("A) result node has the bool ReturnValue", pin_types(d.get("result_pins")).get("ReturnValue") == "bool",
          str(pin_types(d.get("result_pins"))))
    check("A) parent_class is Character", d.get("parent_class") == "Character", str(d.get("parent_class")))
    graph, entry, result = d.get("graph_name"), d.get("entry_node_id"), d.get("result_node_id")

    r = bp("add_function_override", name, function_name="CanJumpInternal")
    check("A) overriding it twice is an error", is_error(r) and "already overridden" in err(r), str(r))

    r = bp("set_node_pin_value", name, graph_name=graph, node_id=result, pin_name="ReturnValue", pin_value="true")
    check("A) ReturnValue literal set inside the function graph", not is_error(r), str(r))
    check("A) entry -> result exec is linked", ensure_exec(name, graph, entry, result))

    supported, ok, msg = compile_result(name)
    if supported:
        check("E1) Character with the CanJumpInternal override compiles", ok, msg[:400])
    else:
        skip("E1) Character with the CanJumpInternal override compiles", "compile_blueprint does not report results in this build")

    # Lookup by the editor name.
    name2 = "BP_Issue21_Character2"
    recreate(name2, "Character")
    r = bp("add_function_override", name2, function_name="CanJump")
    check("A) editor name 'CanJump' resolves to CanJumpInternal", not is_error(r) and data(r).get("graph_name") == "CanJumpInternal", str(r))

    gm = "BP_Issue21_GameMode"
    recreate(gm, "GameModeBase")
    r = bp("add_function_override", gm, function_name="ChoosePlayerStart")
    d = data(r)
    print(f"  ChoosePlayerStart: entry_pins={pin_types(d.get('entry_pins'))} result_pins={pin_types(d.get('result_pins'))}")
    check("A) GameModeBase.ChoosePlayerStart override created", not is_error(r), str(r))
    check("A) its entry has the Player parameter", "Player" in pin_types(d.get("entry_pins")), str(pin_types(d.get("entry_pins"))))
    check("A) its result returns an Actor", pin_types(d.get("result_pins")).get("ReturnValue") == "object:Actor",
          str(pin_types(d.get("result_pins"))))

    r = bp("add_function_override", gm, function_name="K2_PostLogin")
    check("A) event-style function is refused and points at add_blueprint_event_node",
          is_error(r) and "add_blueprint_event_node" in err(r), str(r))
    r = bp("add_function_override", gm, function_name="NoSuchFunction_Issue21")
    check("A) unknown function is an error", is_error(r), str(r))


def test_user_function():
    print("\n--- B/C/D) user function graph ---")
    name = "BP_Issue21_Actor"
    recreate(name, "Actor")
    event_nodes_before = len(nodes(name))

    r = bp("add_function_graph", name, function_name="Bad", inputs=[{"name": "X", "type": "NoSuchType_Issue21"}])
    check("B) bad input type is rejected", is_error(r), str(r))
    graphs = [g["graph_name"] for g in data(bp("list_blueprint_graphs", name)).get("graphs", [])]
    check("B) ...and no graph was created", "Bad" not in graphs, str(graphs))
    r = bp("add_function_graph", name, function_name="Dup", inputs=[{"name": "X", "type": "Integer"}, {"name": "X", "type": "Float"}])
    check("B) duplicate pin names are rejected", is_error(r), str(r))
    r = bp("add_function_graph", name, function_name="K2_DestroyActor")
    check("B) the name of a parent-class function is refused", is_error(r) and "add_function_override" in err(r), str(r))

    r = bp("add_function_graph", name, function_name="ComputeScore",
           inputs=[{"name": "Base", "type": "Integer"}, {"name": "Bonus", "type": "Integer"},
                   {"name": "Tags", "type": "Array<String>"}],
           outputs=[{"name": "Total", "type": "Integer"}, {"name": "Lookup", "type": "Map<String,Vector>"}])
    d = data(r)
    print(f"  ComputeScore: entry_pins={pin_types(d.get('entry_pins'))} result_pins={pin_types(d.get('result_pins'))}")
    check("B) add_function_graph succeeds", not is_error(r) and d.get("is_override") is False, str(r))
    graph, entry, result = d.get("graph_name"), d.get("entry_node_id"), d.get("result_node_id")
    ein, rout = pin_types(d.get("entry_pins")), pin_types(d.get("result_pins"))
    check("B) inputs are on the entry node with their types",
          ein.get("Base") == "int" and ein.get("Bonus") == "int" and ein.get("Tags") == "Array<string>", str(ein))
    check("B) outputs are on the result node with their types",
          rout.get("Total") == "int" and rout.get("Lookup") == "Map<string,struct:Vector>", str(rout))
    r = bp("add_function_graph", name, function_name="ComputeScore")
    check("B) creating it twice is an error", is_error(r), str(r))

    r = bp("add_function_graph", name, function_name="GetTwice", is_pure=True,
           inputs=[{"name": "X", "type": "Integer"}], outputs=[{"name": "R", "type": "Integer"}])
    pure = data(r)
    check("B) pure function graph created", not is_error(r) and pure.get("is_pure") is True, str(r))
    if not is_error(r):
        mul = data(bp("add_blueprint_function_node", name, graph_name="GetTwice", target="KismetMathLibrary",
                      function_name="Multiply_IntInt", params={"B": 2}, node_position=[300, 100])).get("node_id")
        steps = [
            bp("connect_blueprint_nodes", name, graph_name="GetTwice", source_node_id=pure["entry_node_id"],
               source_pin="X", target_node_id=mul, target_pin="A"),
            bp("connect_blueprint_nodes", name, graph_name="GetTwice", source_node_id=mul,
               source_pin="ReturnValue", target_node_id=pure["result_node_id"], target_pin="R"),
        ]
        check("B) pure function body wired", not any(is_error(x) for x in steps), str([err(x) for x in steps if is_error(x)]))
        ensure_exec(name, "GetTwice", pure["entry_node_id"], pure["result_node_id"])

    # C) node commands inside the function graph
    found = nodes(name, graph)
    types = sorted(n["node_type"] for n in found)
    check("C) find_blueprint_nodes(graph_name) lists the entry and result nodes",
          "FunctionEntry" in types and "FunctionResult" in types, str(types))
    r = bp("add_blueprint_function_node", name, graph_name=graph, target="KismetMathLibrary",
           function_name="Add_IntInt", node_position=[300, 100])
    add = data(r).get("node_id")
    check("C) function node added inside the function graph", not is_error(r) and bool(add), str(r))
    check("C) ...it is in the function graph", any(n["node_id"] == add for n in nodes(name, graph)))
    check("C) ...and not in the event graph", len(nodes(name)) == event_nodes_before and not any(n["node_id"] == add for n in nodes(name)),
          f"{event_nodes_before} -> {len(nodes(name))}")

    steps = [
        bp("connect_blueprint_nodes", name, graph_name=graph, source_node_id=entry, source_pin="Base", target_node_id=add, target_pin="A"),
        bp("connect_blueprint_nodes", name, graph_name=graph, source_node_id=entry, source_pin="Bonus", target_node_id=add, target_pin="B"),
        bp("connect_blueprint_nodes", name, graph_name=graph, source_node_id=add, source_pin="ReturnValue", target_node_id=result, target_pin="Total"),
    ]
    check("C) parameters wired through Add into the result", not any(is_error(s) for s in steps),
          str([err(s) for s in steps if is_error(s)]))
    check("C) entry -> result exec is linked", ensure_exec(name, graph, entry, result))
    check("C) result pin reads back connected", pin(name, graph, result, "Total").get("connected") is True)

    # Without graph_name the same node ids are not found: the command looks in the event graph.
    r = bp("connect_blueprint_nodes", name, source_node_id=entry, source_pin="Base", target_node_id=add, target_pin="A")
    check("C) the same ids without graph_name are not found (event graph)", is_error(r), str(r))

    extra = data(bp("add_print_string_node", name, graph_name=graph, message="tmp", node_position=[300, 400])).get("node_id")
    r = bp("delete_node", name, graph_name=graph, node_id=extra)
    check("C) delete_node works inside the function graph",
          bool(extra) and not is_error(r) and not any(n["node_id"] == extra for n in nodes(name, graph)), str(r))

    r = bp("find_blueprint_nodes", name, graph_name="NoSuchGraph_Issue21")
    print(f"  unknown graph: {err(r)[:200]}")
    check("C) unknown graph_name is an error that lists the graphs", is_error(r) and "ComputeScore" in err(r), str(r))

    # D) list_blueprint_graphs
    listed = {g["graph_name"]: g["graph_type"] for g in data(bp("list_blueprint_graphs", name)).get("graphs", [])}
    check("D) list_blueprint_graphs has the event graph and the functions", listed.get("EventGraph") == "event"
          and listed.get("ComputeScore") == "function" and listed.get("GetTwice") == "function", str(listed))

    # E2) compile and call from the event graph
    supported, ok, msg = compile_result(name)
    if not supported:
        skip("E2) the user function compiles", "compile_blueprint does not report results in this build")
        skip("E2) it can be called from the event graph", "compile_blueprint does not report results in this build")
        return
    check("E2) the user function compiles", ok, msg[:400])
    r = bp("add_blueprint_function_node", name, target="", function_name="ComputeScore", node_position=[300, 0])
    call = data(r).get("node_id")
    check("E2) call node for the user function placed in the event graph", not is_error(r) and bool(call), str(r))
    if call:
        call_pins = {p["name"]: p for n in nodes(name) if n["node_id"] == call for p in n["pins"]}
        check("E2) the call node has the function's pins", all(k in call_pins for k in ("Base", "Bonus", "Tags", "Total", "Lookup")),
              str(sorted(call_pins)))
        check("E2) the impure call node has exec pins", "execute" in call_pins and "then" in call_pins, str(sorted(call_pins)))
    r = bp("add_blueprint_function_node", name, target="", function_name="GetTwice", node_position=[300, 300])
    pure_call = data(r).get("node_id")
    check("E2) call node for the pure function placed", not is_error(r) and bool(pure_call), str(r))
    if pure_call:
        pure_pins = {p["name"] for n in nodes(name) if n["node_id"] == pure_call for p in n["pins"]}
        check("E2) the pure call node has X and R and no exec pins",
              {"X", "R"} <= pure_pins and "execute" not in pure_pins and "then" not in pure_pins, str(sorted(pure_pins)))


def main():
    print("=" * 60)
    print("Issue #21 verification")
    print("=" * 60)
    for fn in (test_overrides, test_user_function):
        try:
            fn()
        except Exception as e:
            print(f"  EXCEPTION in {fn.__name__}: {e!r}")
            RESULTS.append((f"{fn.__name__} ran to completion", False))
    passed = sum(1 for _, c in RESULTS if c)
    print(f"\n{passed}/{len(RESULTS)} passed, {len(SKIPPED)} skipped")
    return 0 if RESULTS and passed == len(RESULTS) else 1


if __name__ == "__main__":
    sys.exit(main())
