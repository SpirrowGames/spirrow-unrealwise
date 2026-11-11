"""Verification for Issue #23.

- A) add_blueprint_variable accepts container types (Array<T>, Set<T>,
     Map<K,V>) and any struct / enum / class by name; resolved_type shows what
     the name was parsed into; bad types are errors
- B) add_make_container_node places Make Array / Make Set / Make Map, with
     the requested number of entries, untyped (wildcard) or typed
- C) connect_blueprint_nodes goes through the graph schema:
     C1 a wildcard Make Array takes the type of the pin it is connected to
     C2 incompatible pins are refused with the schema's reason, nothing is linked
     C3 a conversion node is inserted when the engine has one (int -> string)
     C4 the default BeginPlay node of a new Blueprint, which the editor places
        disabled, is enabled once wired (UEdGraphPin::MakeLinkTo already did
        this before the change; the check guards against losing it)
- D) end to end, when compile_blueprint reports results (Issue #19):
     D1 MakeArray(string literals) -> JoinStringArray -> PrintString compiles
     D2 a Map<String,Vector> variable set from a typed Make Map compiles

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


def recreate(name, parent="Actor"):
    send("delete_asset", {"asset_path": f"{BP_PATH}/{name}"})
    r = send("create_blueprint", {"name": name, "parent_class": parent, "path": BP_PATH})
    if is_error(r):
        raise RuntimeError(f"create_blueprint {name} failed: {r}")


def node(name, node_id):
    for n in data(bp("find_blueprint_nodes", name)).get("nodes", []):
        if n["node_id"] == node_id:
            return n
    return {}


def pin(name, node_id, pin_name):
    for p in node(name, node_id).get("pins", []):
        if p["name"] == pin_name:
            return p
    return {}


def begin_play(name):
    found = data(bp("find_blueprint_nodes", name, node_type="Event", event_type="ReceiveBeginPlay")).get("nodes", [])
    if found:
        return found[0]["node_id"]
    return data(bp("add_blueprint_event_node", name, event_name="ReceiveBeginPlay"))["node_id"]


def connect(name, src, src_pin, dst, dst_pin):
    return bp("connect_blueprint_nodes", name, source_node_id=src, source_pin=src_pin,
              target_node_id=dst, target_pin=dst_pin)


def new_id(r):
    nid = data(r).get("node_id")
    if not nid:
        raise RuntimeError(f"node creation failed: {r}")
    return nid


def compile_result(name):
    """(supported, ok, message)"""
    r = bp("compile_blueprint", name)
    if is_error(r):
        return ("failed to compile" in err(r)), False, err(r)
    d = data(r)
    return ("status" in d), d.get("status") in ("UpToDate", "UpToDateWithWarnings"), str(d)


# --------------------------------------------------------------------------
def test_variable_types():
    print("\n--- A) variable types ---")
    name = "BP_Issue23_Vars"
    recreate(name)
    good = [
        ("Boolean", "bool"),
        ("Integer", "int"),
        ("Float", "real:double"),
        ("String", "string"),
        ("Vector", "struct:Vector"),
        ("Transform", "struct:Transform"),
        ("FRotator", "struct:Rotator"),
        ("ECollisionChannel", "byte:ECollisionChannel"),
        ("Actor", "object:Actor"),
        ("Class<Actor>", "class:Actor"),
        ("Array<Integer>", "Array<int>"),
        ("Set<Name>", "Set<name>"),
        ("Array<Actor>", "Array<object:Actor>"),
        ("Map<String,Vector>", "Map<string,struct:Vector>"),
        ("Map<Name, Array<Integer>>", None),   # nested container -> error
        ("Map<String>", None),                 # map without a value type -> error
        ("NoSuchType_Issue23", None),
        ("Array<NoSuchType_Issue23>", None),
    ]
    for i, (type_name, expected) in enumerate(good):
        r = bp("add_blueprint_variable", name, variable_name=f"Var{i}", variable_type=type_name)
        if expected is None:
            check(f"{type_name!r} is rejected", is_error(r), str(r))
            if is_error(r):
                print(f"      {err(r)[:160]}")
        else:
            got = data(r).get("resolved_type")
            check(f"{type_name!r} -> {expected}", not is_error(r) and got == expected, f"{got} / {err(r)}")


def test_make_container_nodes():
    print("\n--- B) Make Array / Set / Map nodes ---")
    name = "BP_Issue23_Make"
    recreate(name)

    r = bp("add_make_container_node", name, container="array", num_entries=2, node_position=[0, 300])
    d = data(r)
    check("B) untyped array placed", not is_error(r) and d.get("node_class") == "K2Node_MakeArray", str(r))
    check("B) untyped array: 2 element pins", d.get("input_pins") == ["[0]", "[1]"], str(d.get("input_pins")))
    check("B) untyped array: output pin 'Array', wildcard", d.get("output_pin") == "Array" and d.get("typed") is False
          and "wildcard" in str(d.get("output_type")), str(d))

    r = bp("add_make_container_node", name, container="set", element_type="Name", node_position=[0, 500])
    d = data(r)
    check("B) typed set placed", not is_error(r) and d.get("node_class") == "K2Node_MakeSet"
          and d.get("output_type") == "Set<name>", str(r))

    r = bp("add_make_container_node", name, container="map", key_type="String", value_type="Vector",
           num_entries=2, node_position=[0, 700])
    d = data(r)
    check("B) typed map placed", not is_error(r) and d.get("node_class") == "K2Node_MakeMap", str(r))
    check("B) typed map: 2 key/value pairs", d.get("input_pins") == ["Key 0", "Value 0", "Key 1", "Value 1"], str(d.get("input_pins")))
    check("B) typed map: output is Map<string,struct:Vector>", d.get("output_type") == "Map<string,struct:Vector>", str(d.get("output_type")))
    map_id = d.get("node_id")
    check("B) typed map: key pin is string, value pin is Vector",
          pin(name, map_id, "Key 1").get("full_type") == "string" and pin(name, map_id, "Value 1").get("full_type") == "struct:Vector",
          f"{pin(name, map_id, 'Key 1').get('full_type')} / {pin(name, map_id, 'Value 1').get('full_type')}")

    check("B) unknown container is rejected", is_error(bp("add_make_container_node", name, container="list")))
    check("B) map with only key_type is rejected", is_error(bp("add_make_container_node", name, container="map", key_type="String")))
    check("B) unknown element type is rejected",
          is_error(bp("add_make_container_node", name, container="array", element_type="NoSuchType_Issue23")))


def test_schema_connect():
    print("\n--- C) connecting through the schema ---")
    name = "BP_Issue23_Connect"
    recreate(name)
    ev = begin_play(name)

    # C4 first, while BeginPlay is still untouched.
    before = node(name, ev).get("enabled")
    pr = new_id(bp("add_print_string_node", name, message="x", node_position=[900, 0]))
    r = connect(name, ev, "then", pr, "execute")
    after = node(name, ev).get("enabled")
    check("C4) default BeginPlay node starts disabled", before is False, str(before))
    check("C4) wiring it succeeds", not is_error(r), str(r))
    check("C4) BeginPlay is enabled once wired", after is True, str(after))

    # C1 wildcard resolution
    arr = new_id(bp("add_make_container_node", name, container="array", num_entries=2, node_position=[0, 300]))
    join = new_id(bp("add_blueprint_function_node", name, target="KismetStringLibrary",
                     function_name="JoinStringArray", node_position=[300, 300]))
    check("C1) element pin is a wildcard before connecting", pin(name, arr, "[0]").get("full_type") == "wildcard",
          str(pin(name, arr, "[0]").get("full_type")))
    r = connect(name, arr, "Array", join, "SourceArray")
    check("C1) Make Array -> JoinStringArray.SourceArray connects", not is_error(r), str(r))
    check("C1) output became Array<string>", pin(name, arr, "Array").get("full_type") == "Array<string>",
          str(pin(name, arr, "Array").get("full_type")))
    check("C1) element pins became string",
          pin(name, arr, "[0]").get("full_type") == "string" and pin(name, arr, "[1]").get("full_type") == "string",
          str(pin(name, arr, "[1]").get("full_type")))

    # C2 incompatible pins
    r = connect(name, arr, "Array", pr, "InString")
    print(f"  refusal: {err(r)[:200]}")
    check("C2) Array<string> -> string pin is refused", is_error(r), str(r))
    check("C2) the refusal carries a reason", len(err(r)) > len("Failed to connect Array -> InString: "), err(r))
    check("C2) nothing was linked", pin(name, pr, "InString").get("connected") is False, str(pin(name, pr, "InString")))
    check("C2) the earlier link is intact", pin(name, join, "SourceArray").get("connected") is True)

    r = connect(name, arr, "NoSuchPin", pr, "InString")
    check("C2) unknown pin name is an error naming the pin", is_error(r) and "NoSuchPin" in err(r), str(r))

    # C3 conversion node
    mul = new_id(bp("add_blueprint_function_node", name, target="KismetMathLibrary",
                    function_name="Multiply_IntInt", node_position=[300, 600]))
    pr2 = new_id(bp("add_print_string_node", name, message="y", node_position=[1200, 0]))
    count_before = len(data(bp("find_blueprint_nodes", name)).get("nodes", []))
    r = connect(name, mul, "ReturnValue", pr2, "InString")
    count_after = len(data(bp("find_blueprint_nodes", name)).get("nodes", []))
    check("C3) int -> string connects", not is_error(r), str(r))
    check("C3) response reports the conversion node", data(r).get("conversion_nodes_added") == 1, str(data(r)))
    check("C3) the graph gained exactly one node", count_after == count_before + 1, f"{count_before} -> {count_after}")


def test_end_to_end():
    print("\n--- D) end to end with the compiler ---")
    name = "BP_Issue23_E2E"
    recreate(name)
    supported, ok, msg = compile_result(name)
    if not supported:
        skip("D1) MakeArray -> JoinStringArray -> PrintString compiles", "compile_blueprint does not report results in this build")
        skip("D2) Map<String,Vector> variable set from Make Map compiles", "compile_blueprint does not report results in this build")
        return

    ev = begin_play(name)
    arr = new_id(bp("add_make_container_node", name, container="array", num_entries=2, node_position=[0, 300]))
    join = new_id(bp("add_blueprint_function_node", name, target="KismetStringLibrary",
                     function_name="JoinStringArray", node_position=[300, 300]))
    pr = new_id(bp("add_print_string_node", name, message="", node_position=[600, 0]))
    steps = [
        connect(name, arr, "Array", join, "SourceArray"),
        bp("set_node_pin_value", name, node_id=arr, pin_name="[0]", pin_value="a"),
        bp("set_node_pin_value", name, node_id=arr, pin_name="[1]", pin_value="b"),
        connect(name, join, "ReturnValue", pr, "InString"),
        connect(name, ev, "then", pr, "execute"),
    ]
    check("D1) graph built", not any(is_error(s) for s in steps), str([err(s) for s in steps if is_error(s)]))
    supported, ok, msg = compile_result(name)
    check("D1) MakeArray -> JoinStringArray -> PrintString compiles", ok, msg[:400])

    r = bp("add_blueprint_variable", name, variable_name="Points", variable_type="Map<String,Vector>")
    check("D2) Map<String,Vector> variable added", not is_error(r), str(r))
    supported, ok, msg = compile_result(name)
    check("D2) Blueprint with the map variable compiles", ok, msg[:400])
    setter = bp("add_variable_set_node", name, variable_name="Points", node_position=[900, 0])
    check("D2) set node for the map variable placed", not is_error(setter), str(setter))
    if not is_error(setter):
        set_id = new_id(setter)
        mk = new_id(bp("add_make_container_node", name, container="map", key_type="String", value_type="Vector",
                       node_position=[600, 300]))
        steps = [
            bp("set_node_pin_value", name, node_id=mk, pin_name="Key 0", pin_value="origin"),
            connect(name, mk, "Map", set_id, "Points"),
            connect(name, pr, "then", set_id, "execute"),
        ]
        check("D2) Make Map wired into the set node", not any(is_error(s) for s in steps),
              str([err(s) for s in steps if is_error(s)]))
        check("D2) set node input is Map<string,struct:Vector>",
              pin(name, set_id, "Points").get("full_type") == "Map<string,struct:Vector>",
              str(pin(name, set_id, "Points").get("full_type")))
        supported, ok, msg = compile_result(name)
        check("D2) Map<String,Vector> variable set from Make Map compiles", ok, msg[:400])


def main():
    print("=" * 60)
    print("Issue #23 verification")
    print("=" * 60)
    for fn in (test_variable_types, test_make_container_nodes, test_schema_connect, test_end_to_end):
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
