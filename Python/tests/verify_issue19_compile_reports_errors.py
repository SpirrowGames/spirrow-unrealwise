"""Verification for Issue #19: compile_blueprint runs the compiler and reports
the result instead of always answering success.

Cases:
- A) a clean Blueprint compiles: status=UpToDate, no errors
- B) compiling it a second time (the "already compiled" path that used to be
     skipped) still reports status=UpToDate
- C) a Blueprint with a real compiler error is an error response that carries
     the compiler's message. The error is an unwired by-ref input on
     KismetMathLibrary::Vector_Normalize ("Normalize In Place"), built only
     from engine types.
- D) deleting the offending node makes the same Blueprint compile again
- E) the 2026-04 crash recipe (Character Blueprint, add a component, compile,
     compile again) completes; see Docs/Architecture/SafeCompileBlueprint_Bugfix.md

Prereq: MCPGameProject editor running with the rebuilt SpirrowBridge plugin.
Test Blueprints are recreated on every run.
"""
import sys, os
try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_server import get_unreal_connection


BP_PATH = "/Game/Test"
BP_CLEAN = "BP_Issue19_Clean"
BP_BROKEN = "BP_Issue19_Broken"
BP_CHAR = "BP_Issue19_Character"

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


def bp(command, name, **params):
    params.update({"blueprint_name": name, "path": BP_PATH})
    return send(command, params)


def check(name, cond, detail=""):
    RESULTS.append((name, bool(cond)))
    print(f"  [{'PASS' if cond else 'FAIL'}] {name}" + (f"  ({detail})" if detail and not cond else ""))


def recreate(name, parent):
    send("delete_asset", {"asset_path": f"{BP_PATH}/{name}"})
    r = send("create_blueprint", {"name": name, "parent_class": parent, "path": BP_PATH})
    if is_error(r):
        raise RuntimeError(f"create_blueprint {name} failed: {r}")


def node_id(r):
    nid = data(r).get("node_id")
    if not nid:
        raise RuntimeError(f"node creation failed: {r}")
    return nid


def begin_play(name):
    """A new Actor Blueprint already has a BeginPlay event node; reuse it."""
    found = data(bp("find_blueprint_nodes", name, node_type="Event", event_type="ReceiveBeginPlay")).get("nodes", [])
    if found:
        return found[0]["node_id"]
    return node_id(bp("add_blueprint_event_node", name, event_name="ReceiveBeginPlay"))


def test_clean():
    print("\n--- A/B) clean Blueprint ---")
    recreate(BP_CLEAN, "Actor")
    ev = begin_play(BP_CLEAN)
    pr = node_id(bp("add_print_string_node", BP_CLEAN, message="hello", node_position=[300, 0]))
    bp("connect_blueprint_nodes", BP_CLEAN, source_node_id=ev, source_pin="then",
       target_node_id=pr, target_pin="execute")

    for label in ("A) first compile", "B) second compile"):
        r = bp("compile_blueprint", BP_CLEAN)
        d = data(r)
        check(f"{label}: success", not is_error(r), str(r))
        check(f"{label}: compiled is true", d.get("compiled") is True, str(d))
        check(f"{label}: status == UpToDate", d.get("status") == "UpToDate", str(d.get("status")))
        check(f"{label}: no errors reported", d.get("num_errors") == 0 and d.get("errors") == [], str(d))


def test_broken():
    print("\n--- C/D) Blueprint with a compiler error ---")
    recreate(BP_BROKEN, "Actor")
    ev = begin_play(BP_BROKEN)
    bad = node_id(bp("add_blueprint_function_node", BP_BROKEN, target="KismetMathLibrary",
                     function_name="Vector_Normalize", node_position=[300, 0]))
    bp("connect_blueprint_nodes", BP_BROKEN, source_node_id=ev, source_pin="then",
       target_node_id=bad, target_pin="execute")

    r = bp("compile_blueprint", BP_BROKEN)
    msg = str(r.get("error", "")) if isinstance(r, dict) else ""
    print(f"  error: {msg[:400]}")
    check("C) compile error is an error response", is_error(r), str(r))
    check("C) message says status=Error", "status=Error" in msg, msg)
    check("C) message carries the compiler error text", "[E1]" in msg and "by ref" in msg.lower(), msg)

    r = bp("compile_blueprint", BP_BROKEN)
    check("C) still an error on recompile", is_error(r), str(r))

    r = bp("delete_node", BP_BROKEN, node_id=bad)
    check("D) offending node deleted", not is_error(r), str(r))
    r = bp("compile_blueprint", BP_BROKEN)
    check("D) compiles after the fix", not is_error(r) and data(r).get("status") == "UpToDate", str(r))


def test_character_component():
    print("\n--- E) Character Blueprint + component + compile x2 ---")
    recreate(BP_CHAR, "Character")
    r = bp("compile_blueprint", BP_CHAR)
    check("E) compile before component", not is_error(r), str(r))
    r = bp("add_component_to_blueprint", BP_CHAR, component_type="StaticMeshComponent",
           component_name="Issue19Mesh")
    check("E) component added", not is_error(r), str(r))
    for i in (1, 2):
        r = bp("compile_blueprint", BP_CHAR)
        check(f"E) compile #{i} after component: status UpToDate",
              not is_error(r) and data(r).get("status") == "UpToDate", str(r))
    r = send("ping", {})
    check("E) editor still answers", isinstance(r, dict) and r.get("status") != "error", str(r))


def main():
    print("=" * 60)
    print("Issue #19 verification")
    print("=" * 60)
    for fn in (test_clean, test_broken, test_character_component):
        try:
            fn()
        except Exception as e:
            print(f"  EXCEPTION in {fn.__name__}: {e}")
            RESULTS.append((f"{fn.__name__} ran to completion", False))
    passed = sum(1 for _, c in RESULTS if c)
    print(f"\n{passed}/{len(RESULTS)} passed")
    return 0 if RESULTS and passed == len(RESULTS) else 1


if __name__ == "__main__":
    sys.exit(main())
