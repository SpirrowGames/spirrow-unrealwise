"""Verification for Issue #22: add_blueprint_function_node places a
UBlueprintAsyncActionBase factory as an async node with delegate exec outputs.

Uses the engine's own async action, UAsyncActionHandleSaveGame:
  AsyncLoadGameFromSlot / AsyncSaveGameToSlot -> delegate "Completed(SaveGame, bSuccess)"

Checks:
- A) the node is a K2Node_AsyncAction, the response lists exec_outputs
     ["then", "Completed"], and the node has the delegate's data pins
- B) params still set literals on the async node
- C) "Completed" can be wired to a PrintString and, when compile_blueprint
     reports results (Issue #19), the Blueprint compiles
- D) an ordinary function is still a K2Node_CallFunction with no async fields
- E) a BlueprintInternalUseOnly function that is not an async factory is
     placed as before but the response carries a warning

Prereq: MCPGameProject editor running with the rebuilt SpirrowBridge plugin.
"""
import sys, os
try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_server import get_unreal_connection


BP_NAME = "BP_Issue22_Verify"
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


def node(node_id):
    for n in data(bp("find_blueprint_nodes")).get("nodes", []):
        if n["node_id"] == node_id:
            return n
    return {}


def pins(node_id, direction=None, pin_type=None):
    return [p["name"] for p in node(node_id).get("pins", [])
            if (direction is None or p["direction"] == direction) and (pin_type is None or p["type"] == pin_type)]


def main():
    print("=" * 60)
    print("Issue #22 verification")
    print("=" * 60)
    try:
        send("delete_asset", {"asset_path": f"{BP_PATH}/{BP_NAME}"})
        r = send("create_blueprint", {"name": BP_NAME, "parent_class": "Actor", "path": BP_PATH})
        if is_error(r):
            raise RuntimeError(f"create_blueprint failed: {r}")
        found = data(bp("find_blueprint_nodes", node_type="Event", event_type="ReceiveBeginPlay")).get("nodes", [])
        ev = found[0]["node_id"] if found else data(bp("add_blueprint_event_node", event_name="ReceiveBeginPlay"))["node_id"]

        print("\n--- A/B) async factory ---")
        r = bp("add_blueprint_function_node", target="AsyncActionHandleSaveGame",
               function_name="AsyncLoadGameFromSlot", params={"SlotName": "Slot22", "UserIndex": 3},
               node_position=[300, 0])
        d = data(r)
        print(f"  response: { {k: d.get(k) for k in ('node_class', 'async_action', 'exec_outputs', 'warning')} }")
        check("A) call succeeds", not is_error(r), str(r))
        load = d.get("node_id")
        check("A) node_class is K2Node_AsyncAction", d.get("node_class") == "K2Node_AsyncAction", str(d.get("node_class")))
        check("A) response says async_action", d.get("async_action") is True, str(d))
        check("A) exec_outputs are then + Completed", d.get("exec_outputs") == ["then", "Completed"], str(d.get("exec_outputs")))
        check("A) graph node really is a K2Node_AsyncAction", node(load).get("node_class") == "K2Node_AsyncAction",
              str(node(load).get("node_class")))
        outs = pins(load, "Output")
        check("A) delegate data pins exist (SaveGame, bSuccess)", "SaveGame" in outs and "bSuccess" in outs, str(outs))
        check("A) no plain ReturnValue pin", "ReturnValue" not in outs, str(outs))
        check("A) no async warning on a real async node", "warning" not in d, str(d.get("warning")))

        by_name = {p["name"]: p for p in node(load).get("pins", [])}
        if "default_value" in by_name.get("SlotName", {}) or "default_value" in by_name.get("UserIndex", {}):
            check("B) params literals landed on the async node",
                  by_name["SlotName"].get("default_value") == "Slot22" and by_name["UserIndex"].get("default_value") == "3",
                  str({k: by_name[k].get("default_value") for k in ("SlotName", "UserIndex")}))
        else:
            SKIPPED.append("B")
            print("  [SKIP] B) params literals landed on the async node  (find_blueprint_nodes does not report default_value in this build)")

        print("\n--- C) wiring the delegate output ---")
        pr = data(bp("add_print_string_node", message="loaded", node_position=[700, 0]))["node_id"]
        r1 = bp("connect_blueprint_nodes", source_node_id=ev, source_pin="then", target_node_id=load, target_pin="execute")
        r2 = bp("connect_blueprint_nodes", source_node_id=load, source_pin="Completed", target_node_id=pr, target_pin="execute")
        check("C) BeginPlay -> async node", not is_error(r1), str(r1))
        check("C) Completed -> PrintString", not is_error(r2), str(r2))
        connected = {p["name"]: p["connected"] for p in node(load).get("pins", [])}
        check("C) Completed pin reads back connected", connected.get("Completed") is True, str(connected))
        r = bp("compile_blueprint")
        if "status" in data(r) or "failed to compile" in str(r.get("error", "")):
            check("C) Blueprint with the async node compiles", not is_error(r) and data(r).get("status") == "UpToDate", str(r)[:400])
        else:
            SKIPPED.append("C-compile")
            print("  [SKIP] C) Blueprint with the async node compiles  (compile_blueprint does not report results in this build)")

        print("\n--- D) ordinary function ---")
        r = bp("add_blueprint_function_node", target="KismetSystemLibrary", function_name="PrintString",
               node_position=[300, 300])
        d = data(r)
        check("D) PrintString is a K2Node_CallFunction", not is_error(r) and d.get("node_class") == "K2Node_CallFunction", str(d))
        check("D) no async fields, no warning", "async_action" not in d and "exec_outputs" not in d and "warning" not in d, str(d))

        print("\n--- E) BlueprintInternalUseOnly, not an async factory ---")
        r = bp("add_blueprint_function_node", target="GameplayStatics", function_name="BeginDeferredActorSpawnFromClass",
               node_position=[300, 600])
        d = data(r)
        print(f"  warning: {str(d.get('warning'))[:200]}")
        check("E) still placed as a call node", not is_error(r) and d.get("node_class") == "K2Node_CallFunction", str(r))
        check("E) response carries a warning", "BlueprintInternalUseOnly" in str(d.get("warning", "")), str(d))
    except Exception as e:
        print(f"  EXCEPTION: {e!r}")
        RESULTS.append(("ran to completion", False))

    passed = sum(1 for _, c in RESULTS if c)
    print(f"\n{passed}/{len(RESULTS)} passed, {len(SKIPPED)} skipped")
    return 0 if RESULTS and passed == len(RESULTS) else 1


if __name__ == "__main__":
    sys.exit(main())
