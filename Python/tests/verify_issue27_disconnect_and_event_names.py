"""Verification for Issue #27.

1. disconnect_blueprint_nodes reaches its handler (it used to stop at the
   top-level dispatcher with "Unknown command") and breaks links in all three
   modes: one link, one pin, whole node.
2. find_blueprint_nodes matches an event by the name the editor shows
   (BeginPlay / "Event BeginPlay") as well as by the internal UFunction name
   (ReceiveBeginPlay). The checks do not depend on the editor's display
   language: the first run of this script was against a Japanese editor,
   where the node title is "イベント BeginPlay".

Prereq: MCPGameProject editor running with the rebuilt SpirrowBridge plugin.
The test Blueprint is recreated on every run, so the script is repeatable.
"""
import sys, os
try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_server import get_unreal_connection


BP_NAME = "BP_Issue27_Verify"
BP_PATH = "/Game/Test"

RESULTS = []


def data(r):
    if not isinstance(r, dict):
        return {}
    inner = r.get("result", r)
    return inner.get("data", inner) if isinstance(inner, dict) else {}


def ok(r):
    return isinstance(r, dict) and r.get("status") != "error" and data(r).get("success") is not False


def call(command, **params):
    unreal = get_unreal_connection()
    if not unreal:
        raise RuntimeError("no connection to the editor")
    params.setdefault("blueprint_name", BP_NAME)
    params.setdefault("path", BP_PATH)
    r = unreal.send_command(command, params)
    if r is None:
        raise RuntimeError(f"{command}: no response")
    return r


def check(name, cond, detail=""):
    RESULTS.append((name, bool(cond)))
    print(f"  [{'PASS' if cond else 'FAIL'}] {name}" + (f"  ({detail})" if detail and not cond else ""))


def pin_connected(node_id, pin_name):
    for n in data(call("find_blueprint_nodes")).get("nodes", []):
        if n["node_id"] == node_id:
            for p in n["pins"]:
                if p["name"] == pin_name:
                    return p["connected"]
    return None


def setup():
    unreal = get_unreal_connection()
    unreal.send_command("delete_asset", {"asset_path": f"{BP_PATH}/{BP_NAME}"})
    r = unreal.send_command("create_blueprint", {"name": BP_NAME, "parent_class": "Actor", "path": BP_PATH})
    if not ok(r):
        raise RuntimeError(f"create_blueprint failed: {r}")

    # A new Actor Blueprint already carries the editor's default BeginPlay /
    # ActorBeginOverlap / Tick event nodes, so look BeginPlay up by its
    # internal name (which worked before this fix too) instead of adding it.
    found = data(call("find_blueprint_nodes", node_type="Event", event_type="ReceiveBeginPlay")).get("nodes", [])
    ev = found[0] if found else data(call("add_blueprint_event_node", event_name="ReceiveBeginPlay", node_position=[0, 0]))
    p1 = data(call("add_print_string_node", message="one", node_position=[300, 0]))
    p2 = data(call("add_print_string_node", message="two", node_position=[600, 0]))
    ids = ev.get("node_id"), p1.get("node_id"), p2.get("node_id")
    if not all(ids):
        raise RuntimeError(f"node creation failed: {ev} {p1} {p2}")
    return ids


def connect(src, src_pin, dst, dst_pin):
    r = call("connect_blueprint_nodes", source_node_id=src, source_pin=src_pin,
             target_node_id=dst, target_pin=dst_pin)
    if not ok(r):
        raise RuntimeError(f"connect failed: {r}")


def test_disconnect(ev, p1, p2):
    print("\n--- disconnect_blueprint_nodes ---")

    # Mode 1: one specific link
    connect(ev, "then", p1, "execute")
    r = call("disconnect_blueprint_nodes", node_id=ev, pin_name="then",
             target_node_id=p1, target_pin="execute")
    check("routed (no 'Unknown command')", "Unknown command" not in str(r), str(r))
    check("single link: disconnected_count == 1", data(r).get("disconnected_count") == 1, str(r))
    check("single link: pin reads back unconnected", pin_connected(ev, "then") is False)

    # Mode 2: every link on one pin
    connect(ev, "then", p1, "execute")
    r = call("disconnect_blueprint_nodes", node_id=ev, pin_name="then")
    check("pin: disconnected_count == 1", data(r).get("disconnected_count") == 1, str(r))
    check("pin: pin reads back unconnected", pin_connected(ev, "then") is False)

    # Mode 3: every link on the node (p1 has one inbound and one outbound link)
    connect(ev, "then", p1, "execute")
    connect(p1, "then", p2, "execute")
    r = call("disconnect_blueprint_nodes", node_id=p1)
    check("node: disconnected_count == 2", data(r).get("disconnected_count") == 2, str(r))
    check("node: upstream pin unconnected", pin_connected(ev, "then") is False)
    check("node: downstream pin unconnected", pin_connected(p2, "execute") is False)

    r = call("disconnect_blueprint_nodes", node_id="00000000000000000000000000000000")
    check("unknown node_id is an error", not ok(r), str(r))


def test_event_names(ev):
    print("\n--- find_blueprint_nodes event_type ---")
    for name in ("ReceiveBeginPlay", "BeginPlay", "Event BeginPlay", "beginplay", "EventBeginPlay"):
        nodes = data(call("find_blueprint_nodes", node_type="Event", event_type=name)).get("nodes", [])
        hit = [n for n in nodes if n["node_id"] == ev]
        check(f"event_type={name!r} finds the BeginPlay node", len(hit) == 1 and len(nodes) == 1, f"{len(nodes)} nodes")
        if hit and name == "BeginPlay":
            check("name stays the internal name", hit[0].get("name") == "ReceiveBeginPlay", str(hit[0].get("name")))
            check("display_name is the unlocalized editor name", hit[0].get("display_name") == "BeginPlay", str(hit[0].get("display_name")))

    nodes = data(call("find_blueprint_nodes", node_type="Event", event_type="Tick")).get("nodes", [])
    check("event_type='Tick' finds only ReceiveTick", [n.get("name") for n in nodes] == ["ReceiveTick"],
          str([n.get("name") for n in nodes]))

    for name in ("Receive", "Play", "NoSuchEvent"):
        nodes = data(call("find_blueprint_nodes", node_type="Event", event_type=name)).get("nodes", [])
        check(f"event_type={name!r} finds nothing", len(nodes) == 0, f"{len(nodes)} nodes")

    nodes = data(call("find_blueprint_nodes", node_type="Event")).get("nodes", [])
    check("no event_type still lists the event", any(n["node_id"] == ev for n in nodes))


def main():
    print("=" * 60)
    print("Issue #27 verification")
    print("=" * 60)
    try:
        ev, p1, p2 = setup()
        test_disconnect(ev, p1, p2)
        test_event_names(ev)
    except Exception as e:
        print(f"  EXCEPTION: {e}")
        RESULTS.append(("ran to completion", False))
    passed = sum(1 for _, c in RESULTS if c)
    print(f"\n{passed}/{len(RESULTS)} passed")
    return 0 if RESULTS and passed == len(RESULTS) else 1


if __name__ == "__main__":
    sys.exit(main())
