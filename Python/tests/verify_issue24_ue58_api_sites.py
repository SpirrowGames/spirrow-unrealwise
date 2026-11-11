"""Verification for Issue #24: the call sites rewritten for UE 5.8 still work.

Run it against an editor built on each supported engine. It reports the engine
version it talked to, so the output says which engine the result is for.

- A) FJsonObject key sites: set_struct_array_property (dict form) writes every
     field, including the nested ones addressed by JSON key
- B) FJsonObject key sites: set_struct_property on one element reports the
     JSON key in updated_fields and writes the nested field
- C) FInputKeyEventArgs: simulate_pie_input press / release / tap succeed in a
     running PIE session and the editor keeps answering

Prereq: MCPGameProject editor running with the rebuilt SpirrowBridge plugin.
"""
import sys, os, time
try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_server import get_unreal_connection


BP_NAME = "BP_Issue24_Verify"
BP_PATH = "/Game/Test"
PARENT_CLASS = "/Script/MCPGameProject.StructArrayTestActor"

RESULTS = []


def data(r):
    if not isinstance(r, dict):
        return {}
    inner = r.get("result", r)
    return inner.get("data", inner) if isinstance(inner, dict) else {}


def is_error(r):
    return not isinstance(r, dict) or r.get("status") == "error" or data(r).get("success") is False


def send(command, params=None):
    unreal = get_unreal_connection()
    if not unreal:
        raise RuntimeError("no connection to the editor")
    r = unreal.send_command(command, params or {})
    if r is None:
        raise RuntimeError(f"{command}: no response (editor crashed or timed out)")
    return r


def check(name, cond, detail=""):
    RESULTS.append((name, bool(cond)))
    print(f"  [{'PASS' if cond else 'FAIL'}] {name}" + (f"  ({detail})" if detail and not cond else ""))


def rows_text():
    r = send("get_blueprint_properties", {"blueprint_name": BP_NAME, "path": BP_PATH})
    for p in data(r).get("properties", []) or []:
        if p.get("name") == "Rows":
            return p.get("value", "") or p.get("default_value", "")
    return ""


def test_json_key_sites():
    print("\n--- A/B) FJsonObject key sites ---")
    send("delete_asset", {"asset_path": f"{BP_PATH}/{BP_NAME}"})
    r = send("create_blueprint", {"name": BP_NAME, "parent_class": PARENT_CLASS, "path": BP_PATH})
    check("test Blueprint created", not is_error(r), str(r))

    r = send("set_struct_array_property", {
        "blueprint_name": BP_NAME, "path": BP_PATH, "property_name": "Rows",
        "values": [
            {"Coord": {"X": 1, "Y": 2, "Z": 3}, "Count": 7},
            {"Coord": {"X": 4, "Y": 5, "Z": 6}, "Count": 8},
        ],
    })
    check("A) set_struct_array_property succeeds", not is_error(r), str(r))
    raw = rows_text()
    check("A) fields addressed by JSON key were written",
          all(t in raw for t in ("X=1", "Y=2", "Z=3", "X=4", "Y=5", "Z=6", "Count=7", "Count=8")), raw[:300])

    r = send("set_struct_property", {
        "blueprint_name": BP_NAME, "path": BP_PATH, "property_name": "Rows", "index": 0,
        "values": {"Coord": {"X": 91, "Y": 92, "Z": 93}},
    })
    check("B) set_struct_property succeeds", not is_error(r), str(r))
    updated = data(r).get("updated_fields")
    check("B) updated_fields names the JSON key", updated == ["Coord"], str(updated))
    raw = rows_text()
    check("B) nested field updated", all(t in raw for t in ("X=91", "Y=92", "Z=93")), raw[:300])


def wait_pie(state, timeout=30):
    deadline = time.time() + timeout
    got = None
    while time.time() < deadline:
        got = data(send("get_pie_state")).get("state")
        if got == state:
            return True
        time.sleep(0.5)
    print(f"  (last PIE state: {got})")
    return False


def test_pie_input():
    print("\n--- C) FInputKeyEventArgs ---")
    r = send("start_pie", {})
    check("C) start_pie accepted", not is_error(r), str(r))
    try:
        check("C) PIE reaches running", wait_pie("running"))
        for event in ("press", "release", "tap"):
            r = send("simulate_pie_input", {"key": "SpaceBar", "event": event})
            check(f"C) simulate_pie_input {event}", not is_error(r) and data(r).get("key") == "SpaceBar", str(r))
        r = send("simulate_pie_input", {"key": "NotAKey"})
        check("C) unknown key is rejected", is_error(r), str(r))
    finally:
        send("stop_pie", {})
        check("C) PIE stops", wait_pie("stopped"))
    r = send("ping")
    check("C) editor still answers", not is_error(r), str(r))


def main():
    print("=" * 60)
    print("Issue #24 verification")
    info = data(send("get_project_info"))
    print(f"engine_version: {info.get('engine_version', info)}")
    print("=" * 60)
    for fn in (test_json_key_sites, test_pie_input):
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
