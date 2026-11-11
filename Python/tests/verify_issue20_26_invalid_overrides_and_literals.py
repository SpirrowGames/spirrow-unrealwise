"""Verification for Issues #20 and #26.

#20  add_blueprint_event_node refuses a parent function that has a return
     value or out parameters (it needs a function-graph override) and adds no
     node. The error names add_function_override, and that command then
     creates the override. A plain BlueprintImplementableEvent is still placed.

#26  1. A literal for a pin that cannot hold one (by-ref / container input) is
        an error, and add_blueprint_function_node leaves no node behind.
     2. Vector / Rotator / real literals keep double precision.

The literal checks are cross-checked against the compiler when the editor's
compile_blueprint reports results (Issue #19). For every probe pin:
  - the bridge accepts the literal  -> the Blueprint must compile;
  - the bridge rejects the literal  -> the same node, unwired and without the
    literal, must fail to compile.
That ties "rejected" to "the engine really cannot take it" instead of to this
script's own expectations. Without #19 those checks are reported as SKIP.

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
        raise RuntimeError(f"create_blueprint {name} ({parent}) failed: {r}")


def nodes(name, **flt):
    return data(bp("find_blueprint_nodes", name, **flt)).get("nodes", [])


def pin(name, node_id, pin_name):
    for n in nodes(name):
        if n["node_id"] == node_id:
            for p in n["pins"]:
                if p["name"] == pin_name:
                    return p
    return {}


def compile_status(name):
    """(supported, ok, message). supported is False when compile_blueprint
    does not report results (Issue #19 not in this build)."""
    r = bp("compile_blueprint", name)
    if is_error(r):
        return ("failed to compile" in err(r)), False, err(r)
    d = data(r)
    return ("status" in d), d.get("status") in ("UpToDate", "UpToDateWithWarnings"), str(d)


# --------------------------------------------------------------------------
# Issue #20
# --------------------------------------------------------------------------
def test_event_overrides():
    print("\n--- #20) event overrides ---")
    cases = [
        ("BP_Issue20_GameMode", "GameModeBase", "ChoosePlayerStart", "return value, BlueprintNativeEvent"),
        ("BP_Issue20_Character", "Character", "CanJumpInternal", "return value + const, BlueprintNativeEvent"),
        ("BP_Issue20_CamMgr", "PlayerCameraManager", "BlueprintUpdateCamera", "return value + out params, BlueprintImplementableEvent"),
    ]
    for name, parent, fn, what in cases:
        recreate(name, parent)
        before = len(nodes(name))
        r = bp("add_blueprint_event_node", name, event_name=fn)
        print(f"  {parent}.{fn}: {err(r)[:200]}")
        check(f"{parent}.{fn} ({what}) is rejected", is_error(r), str(r))
        check(f"{parent}.{fn}: message points at a function graph", "function graph" in err(r), err(r))
        check(f"{parent}.{fn}: message names add_function_override", "add_function_override" in err(r), err(r))
        check(f"{parent}.{fn}: no node was added", len(nodes(name)) == before, f"{before} -> {len(nodes(name))}")
        # The command the message names does create the override.
        r = bp("add_function_override", name, function_name=fn)
        check(f"{parent}.{fn}: add_function_override then succeeds",
              not is_error(r) and data(r).get("graph_name") == fn and bool(data(r).get("result_node_id")), str(r))

    # Still works: an implementable event with no outputs.
    name = "BP_Issue20_GameMode"
    r = bp("add_blueprint_event_node", name, event_name="K2_PostLogin", node_position=[0, 400])
    d = data(r)
    check("GameModeBase.K2_PostLogin (no outputs) is placed", not is_error(r) and d.get("is_override") is True, str(r))
    supported, ok, msg = compile_status(name)
    if supported:
        check("the Blueprint with that override compiles", ok, msg)
    else:
        skip("the Blueprint with that override compiles", "compile_blueprint does not report results in this build")


# --------------------------------------------------------------------------
# Issue #26
# --------------------------------------------------------------------------
def begin_play(name):
    found = nodes(name, node_type="Event", event_type="ReceiveBeginPlay")
    if found:
        return found[0]["node_id"]
    return data(bp("add_blueprint_event_node", name, event_name="ReceiveBeginPlay"))["node_id"]


# (label, target class, function, pin, literal, expected: True = bridge should accept)
PROBES = [
    ("by-value FVector", "Actor", "K2_SetActorLocation", "NewLocation", [1.0, 2.0, 3.0], True),
    ("const FString& (PrintString)", "KismetSystemLibrary", "PrintString", "InString", "hello", True),
    ("UPARAM(ref) FVector&", "KismetMathLibrary", "Vector_Normalize", "A", [1.0, 2.0, 3.0], False),
    ("const TArray<FString>&", "KismetStringLibrary", "JoinStringArray", "SourceArray", "x", False),
]


def wire_into_execution(name, ev, node_id):
    """Make the compiler look at the node. A node with exec pins is chained
    after BeginPlay. A pure node is pruned unless something consumes it, so
    its ReturnValue (a string for the probes above) feeds a PrintString."""
    if pin(name, node_id, "execute"):
        bp("connect_blueprint_nodes", name, source_node_id=ev, source_pin="then",
           target_node_id=node_id, target_pin="execute")
        return
    pr = data(bp("add_print_string_node", name, message="", node_position=[600, 0]))["node_id"]
    bp("connect_blueprint_nodes", name, source_node_id=ev, source_pin="then",
       target_node_id=pr, target_pin="execute")
    bp("connect_blueprint_nodes", name, source_node_id=node_id, source_pin="ReturnValue",
       target_node_id=pr, target_pin="InString")


def test_literal_acceptance():
    print("\n--- #26-1) literals on pins that cannot hold one ---")
    for i, (label, target, fn, pin_name, literal, expect_accept) in enumerate(PROBES):
        name = f"BP_Issue26_Probe{i}"
        recreate(name, "Actor")
        ev = begin_play(name)
        before = len(nodes(name, node_type="Function"))

        r = bp("add_blueprint_function_node", name, target=target, function_name=fn,
               params={pin_name: literal}, node_position=[300, 0])
        accepted = not is_error(r)
        after = len(nodes(name, node_type="Function"))
        if not accepted:
            print(f"  {label}: {err(r)[:220]}")
        check(f"{label}: literal is {'accepted' if expect_accept else 'rejected'}", accepted == expect_accept, str(r))

        if accepted:
            node_id = data(r)["node_id"]
        else:
            check(f"{label}: rejected call leaves no node behind", after == before, f"{before} -> {after}")
            check(f"{label}: message names the pin", pin_name in err(r), err(r))
            # Place the node without the literal for the remaining checks.
            r2 = bp("add_blueprint_function_node", name, target=target, function_name=fn, node_position=[300, 0])
            node_id = data(r2).get("node_id")
            check(f"{label}: node without the literal is placed", bool(node_id), str(r2))
            r3 = bp("set_node_pin_value", name, node_id=node_id, pin_name=pin_name,
                    pin_value=literal if isinstance(literal, str) else ",".join(str(v) for v in literal))
            check(f"{label}: set_node_pin_value is rejected too", is_error(r3), str(r3))

        # Cross-check against the compiler.
        wire_into_execution(name, ev, node_id)
        supported, ok, msg = compile_status(name)
        cname = f"{label}: compiler agrees ({'compiles' if accepted else 'unwired pin fails to compile'})"
        if not supported:
            skip(cname, "compile_blueprint does not report results in this build")
        else:
            check(cname, ok == accepted, msg[:300])


def test_precision():
    print("\n--- #26-2) literal precision ---")
    name = "BP_Issue26_Precision"
    recreate(name, "Actor")
    ev = begin_play(name)

    r = bp("add_blueprint_function_node", name, target="Actor", function_name="K2_SetActorLocationAndRotation",
           params={"NewLocation": [300.0, 0.0, 302.012643], "NewRotation": [10.5, -20.25, 30.123456789]},
           node_position=[300, 0])
    check("vector + rotator literals accepted", not is_error(r), str(r))
    node_id = data(r).get("node_id")
    loc = pin(name, node_id, "NewLocation").get("default_value")
    rot = pin(name, node_id, "NewRotation").get("default_value")
    print(f"  NewLocation default: {loc}\n  NewRotation default: {rot}")
    check("vector Z keeps 302.012643 (was 302.012634 via float)",
          loc is not None and [float(v) for v in loc.split(",")] == [300.0, 0.0, 302.012643], str(loc))
    check("rotator components keep double precision",
          rot is not None and [float(v) for v in rot.split(",")] == [10.5, -20.25, 30.123456789], str(rot))
    bp("connect_blueprint_nodes", name, source_node_id=ev, source_pin="then",
       target_node_id=node_id, target_pin="execute")

    r = bp("add_blueprint_function_node", name, target="KismetMathLibrary", function_name="Multiply_DoubleDouble",
           params={"A": 302.012643123456, "B": 2}, node_position=[300, 300])
    check("real literals accepted", not is_error(r), str(r))
    mul = data(r).get("node_id")
    a = pin(name, mul, "A").get("default_value")
    b = pin(name, mul, "B").get("default_value")
    print(f"  A default: {a}   B default: {b}")
    check("real literal keeps 302.012643123456", a is not None and float(a) == 302.012643123456, str(a))
    check("whole real literal is written with a decimal point", b == "2.0", str(b))

    r = bp("set_node_pin_value", name, node_id=mul, pin_name="B", pin_value="0.1234567890123")
    b = pin(name, mul, "B").get("default_value")
    check("set_node_pin_value keeps 0.1234567890123", not is_error(r) and b is not None and float(b) == 0.1234567890123, str(b))

    supported, ok, msg = compile_status(name)
    if supported:
        check("Blueprint with these literals compiles", ok, msg[:300])
    else:
        skip("Blueprint with these literals compiles", "compile_blueprint does not report results in this build")


def main():
    print("=" * 60)
    print("Issue #20 / #26 verification")
    print("=" * 60)
    for fn in (test_event_overrides, test_literal_acceptance, test_precision):
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
