"""Verification for Issue #33.

1. spawn_blueprint_actor places a Blueprint that exists only in memory. It
   used to answer "Blueprint ... not found" because the handler asked whether
   the package was on disk.
2. save_asset writes an asset to disk (there was no save command at all).
3. The Blueprint's BeginPlay logic really runs in PIE: this is the check that
   could not be written while the bug was there.

Prereq: MCPGameProject editor running with the rebuilt SpirrowBridge plugin.
Every run uses a new Blueprint name and removes the file it saved, so the
script is repeatable and leaves the Content folder as it found it.
"""
import sys, os, time
try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_server import get_unreal_connection


STAMP = time.strftime("%H%M%S")
BP_NAME = f"BP_Issue33_{STAMP}"
BP_PATH = "/Game/Test"
ACTOR = f"Issue33Actor_{STAMP}"
# The marker is assembled inside the Blueprint from two halves, so the full
# string never appears in a bridge command and can only reach the log by the
# graph actually running.
HALF_A = f"I33A{STAMP}"
HALF_B = f"I33B{STAMP}"
MARKER = HALF_A + HALF_B

RESULTS = []


def data(r):
    if not isinstance(r, dict):
        return {}
    inner = r.get("result", r)
    return inner.get("data", inner) if isinstance(inner, dict) else {}


def ok(r):
    return isinstance(r, dict) and r.get("status") != "error" and data(r).get("success") is not False


def raw(command, **params):
    unreal = get_unreal_connection()
    if not unreal:
        raise RuntimeError("no connection to the editor")
    r = unreal.send_command(command, params)
    if r is None:
        raise RuntimeError(f"{command}: no response")
    return r


def call(command, **params):
    params.setdefault("blueprint_name", BP_NAME)
    params.setdefault("path", BP_PATH)
    return raw(command, **params)


def check(name, cond, detail=""):
    RESULTS.append((name, bool(cond)))
    print(f"  [{'PASS' if cond else 'FAIL'}] {name}" + (f"  ({detail})" if detail and not cond else ""))


def must(r, what):
    if not ok(r):
        raise RuntimeError(f"{what} failed: {r}")
    return data(r)


def build_blueprint():
    must(raw("create_blueprint", name=BP_NAME, parent_class="Actor", path=BP_PATH), "create_blueprint")
    found = data(call("find_blueprint_nodes", node_type="Event", event_type="ReceiveBeginPlay")).get("nodes", [])
    ev = found[0] if found else must(call("add_blueprint_event_node", event_name="ReceiveBeginPlay", node_position=[0, 0]), "event")
    append = must(call("add_blueprint_function_node", target="KismetStringLibrary", function_name="Concat_StrStr",
                       node_position=[300, 200]), "Concat_StrStr")
    pr = must(call("add_print_string_node", message="placeholder", node_position=[600, 0]), "print")
    must(call("set_node_pin_value", node_id=append["node_id"], pin_name="A", pin_value=HALF_A), "pin A")
    must(call("set_node_pin_value", node_id=append["node_id"], pin_name="B", pin_value=HALF_B), "pin B")
    must(call("connect_blueprint_nodes", source_node_id=append["node_id"], source_pin="ReturnValue",
              target_node_id=pr["node_id"], target_pin="InString"), "connect string")
    must(call("connect_blueprint_nodes", source_node_id=ev["node_id"], source_pin="then",
              target_node_id=pr["node_id"], target_pin="execute"), "connect exec")
    must(call("compile_blueprint"), "compile_blueprint")


def test_spawn_unsaved():
    print("\n--- spawn_blueprint_actor on a Blueprint that was never saved ---")
    r = call("spawn_blueprint_actor", actor_name=ACTOR, location=[0, 0, 300])
    check("spawn succeeds", ok(r), str(r))
    check("no 'not found'", "not found" not in str(r), str(r))
    names = [a.get("name") for a in data(raw("find_actors_by_name", pattern=ACTOR)).get("actors", [])]
    check("the actor is in the level", ACTOR in names, str(names))

    r = call("spawn_blueprint_actor", actor_name=ACTOR)
    check("a taken actor name is an error", not ok(r) and "already exists" in str(r), str(r))
    check("editor still answers after the refused spawn", ok(raw("ping")) or "pong" in str(raw("ping")))

    r = raw("spawn_blueprint_actor", blueprint_name="BP_Issue33_DoesNotExist", actor_name="Nope", path=BP_PATH)
    check("a missing Blueprint is still an error", not ok(r) and "not found" in str(r), str(r))


def test_save_asset():
    print("\n--- save_asset ---")
    asset = f"{BP_PATH}/{BP_NAME}"
    r = raw("save_asset", asset_path=asset)
    d = data(r)
    check("routed (no 'Unknown command')", "Unknown" not in str(r), str(r))
    check("save succeeds", ok(r) and d.get("saved") is True, str(r))
    check("was not on disk before", d.get("was_on_disk") is False, str(d))
    filename = d.get("filename", "")
    check("the .uasset exists on disk", bool(filename) and os.path.isfile(filename), filename)
    check("package is clean afterwards", d.get("dirty") is False, str(d))

    d2 = data(raw("save_asset", asset_path=f"{asset}.{BP_NAME}", only_if_dirty=True))
    check("only_if_dirty skips a clean asset (object path accepted)", d2.get("saved") is False and d2.get("was_on_disk") is True, str(d2))

    r = raw("save_asset", asset_path="/Game/Test/BP_Issue33_DoesNotExist")
    check("unknown asset is an error", not ok(r), str(r))
    r = raw("save_asset", asset_path="not a path")
    check("malformed path is an error", not ok(r), str(r))
    return filename


def test_runs_in_pie():
    print("\n--- the Blueprint's BeginPlay runs in PIE ---")
    def marker_lines():
        entries = data(raw("search_ue_log", keyword=MARKER, lines=20000)).get("lines", [])
        # The search command itself is echoed into the log with the keyword
        # in it, so only a line PrintString wrote counts.
        return [e for e in entries if MARKER in str(e) and "LogBlueprintUserMessages" in str(e)]

    check("marker is not in the log before PIE", len(marker_lines()) == 0, str(marker_lines()[:2]))
    r = raw("start_pie")
    check("start_pie accepted", ok(r), str(r))
    running = False
    for _ in range(60):
        time.sleep(1)
        if data(raw("get_pie_state")).get("state") == "running":
            running = True
            break
    check("PIE is running", running)
    hits = []
    for _ in range(20):
        time.sleep(1)
        hits = marker_lines()
        if hits:
            break
    check("BeginPlay printed the marker", len(hits) > 0)
    if hits:
        print(f"    log: {str(hits[0])[:200]}")
    raw("stop_pie")
    for _ in range(30):
        time.sleep(1)
        if data(raw("get_pie_state")).get("state") == "stopped":
            break


def main():
    print("=" * 70)
    print(f"Issue #33 verification  (Blueprint {BP_NAME})")
    print("=" * 70)
    filename = ""
    try:
        build_blueprint()
        test_spawn_unsaved()
        test_runs_in_pie()
        filename = test_save_asset()
    finally:
        raw("delete_actor", name=ACTOR)
        if filename and os.path.isfile(filename):
            os.remove(filename)

    passed = sum(1 for _, c in RESULTS if c)
    print("\n" + "=" * 70)
    print(f"{passed}/{len(RESULTS)} checks passed")
    for name, c in RESULTS:
        if not c:
            print(f"  FAILED: {name}")
    return 0 if passed == len(RESULTS) and RESULTS else 1


if __name__ == "__main__":
    sys.exit(main())
