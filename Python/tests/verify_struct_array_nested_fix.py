"""Verification for Issue #11 fix (v0.10.2): nested struct fields inside
TArray<USTRUCT> are now written through set_struct_array_property /
set_struct_property.

Prereq:
- MCPGameProject editor running with the rebuilt SpirrowBridge plugin
- AStructArrayTestActor (MCPGameProject test fixture) is compiled in

Tests:
- A) set_struct_array_property writes Coord (FIntVector via {X,Y,Z} object)
- B) set_struct_array_property accepts FIntVector via array [X,Y,Z]
- C) set_struct_array_property accepts FIntVector via UE text "(X=1,Y=0,Z=0)"
- D) set_struct_property updates an individual nested struct field
"""
import sys, os, json, time
try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_server import get_unreal_connection


BP_NAME = "BP_StructArrayTest_Verify"
BP_PATH = "/Game/Test"
PARENT_CLASS = "/Script/MCPGameProject.StructArrayTestActor"
TEST_MATERIAL = "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"


def get_data(r):
    if not isinstance(r, dict): return {}
    inner = r.get("result", r)
    return inner.get("data", inner) if isinstance(inner, dict) else {}


def ensure_test_blueprint(unreal):
    """Create the test BP if it doesn't exist. Idempotent."""
    r = unreal.send_command("create_blueprint", {
        "name": BP_NAME,
        "parent_class": PARENT_CLASS,
        "path": BP_PATH,
    })
    print(f"  create_blueprint: {r.get('status')} ({get_data(r).get('message', '')[:80]})")
    return r.get("status") == "success" or "already exists" in str(get_data(r)).lower()


def read_rows(unreal):
    """Pull current Rows value from the BP's CDO via get_blueprint_properties."""
    r = unreal.send_command("get_blueprint_properties", {
        "blueprint_name": BP_NAME,
        "path": BP_PATH,
    })
    data = get_data(r)
    props = data.get("properties", []) or []
    for p in props:
        if p.get("name") == "Rows":
            return p
    return None


def test_A_dict_form_coord_written():
    print("\n--- A) set_struct_array_property writes Coord via dict {X,Y,Z} ---")
    unreal = get_unreal_connection()
    if not unreal: return False
    if not ensure_test_blueprint(unreal): return False

    r = unreal.send_command("set_struct_array_property", {
        "blueprint_name": BP_NAME,
        "path": BP_PATH,
        "property_name": "Rows",
        "values": [
            {"Coord": {"X": 1, "Y": 2, "Z": 3}, "Position": {"X": 100.0, "Y": 200.0, "Z": 300.0},
             "Count": 7, "Material": TEST_MATERIAL},
            {"Coord": {"X": 4, "Y": 5, "Z": 6}, "Position": {"X": 400.0, "Y": 500.0, "Z": 600.0},
             "Count": 8, "Material": TEST_MATERIAL},
        ],
    })
    print(f"  set_struct_array_property → status={r.get('status')}")
    if r.get("status") != "success":
        print(f"  FAIL: {get_data(r)}"); return False

    rows = read_rows(unreal)
    if rows is None:
        print("  FAIL: Rows property not found in get_blueprint_properties"); return False

    raw = rows.get("value", "") or rows.get("default_value", "")
    print(f"  Rows raw: {raw[:300]}")

    # Bug symptom: Coord omitted (defaults to 0,0,0) → text representation excludes Coord=
    # Fix expected: text contains Coord=(X=1,Y=2,Z=3) and Coord=(X=4,Y=5,Z=6)
    ok = ("X=1" in raw and "Y=2" in raw and "Z=3" in raw and
          "X=4" in raw and "Y=5" in raw and "Z=6" in raw)
    if ok:
        print("  PASS: Coord values present in Rows")
    else:
        print("  FAIL: Coord values missing or default — fix not applied")
    return ok


def test_B_array_form_coord_written():
    print("\n--- B) set_struct_array_property accepts Coord via array [X,Y,Z] ---")
    unreal = get_unreal_connection()
    if not unreal: return False
    if not ensure_test_blueprint(unreal): return False

    r = unreal.send_command("set_struct_array_property", {
        "blueprint_name": BP_NAME,
        "path": BP_PATH,
        "property_name": "Rows",
        "values": [
            {"Coord": [11, 22, 33], "Count": 1},
        ],
    })
    print(f"  status={r.get('status')}")
    rows = read_rows(unreal)
    raw = (rows or {}).get("value", "") or (rows or {}).get("default_value", "")
    print(f"  Rows raw: {raw[:300]}")
    ok = "X=11" in raw and "Y=22" in raw and "Z=33" in raw
    print("  PASS" if ok else "  FAIL: array form for FIntVector not honored")
    return ok


def test_C_string_form_coord_written():
    print("\n--- C) set_struct_array_property accepts Coord via UE text \"(X=,Y=,Z=)\" ---")
    unreal = get_unreal_connection()
    if not unreal: return False
    if not ensure_test_blueprint(unreal): return False

    r = unreal.send_command("set_struct_array_property", {
        "blueprint_name": BP_NAME,
        "path": BP_PATH,
        "property_name": "Rows",
        "values": [
            {"Coord": "(X=77,Y=88,Z=99)", "Count": 2},
        ],
    })
    print(f"  status={r.get('status')}")
    rows = read_rows(unreal)
    raw = (rows or {}).get("value", "") or (rows or {}).get("default_value", "")
    print(f"  Rows raw: {raw[:300]}")
    ok = "X=77" in raw and "Y=88" in raw and "Z=99" in raw
    print("  PASS" if ok else "  FAIL: string ImportText_Direct fallback for FIntVector not honored")
    return ok


def test_D_set_struct_property_updates_nested():
    print("\n--- D) set_struct_property updates a nested FIntVector field on element[0] ---")
    unreal = get_unreal_connection()
    if not unreal: return False
    if not ensure_test_blueprint(unreal): return False

    # Seed Rows[0] with default (Coord=0,0,0)
    unreal.send_command("set_struct_array_property", {
        "blueprint_name": BP_NAME, "path": BP_PATH, "property_name": "Rows",
        "values": [{"Count": 0}],
    })

    # Now patch only Coord on element 0
    r = unreal.send_command("set_struct_property", {
        "blueprint_name": BP_NAME, "path": BP_PATH, "property_name": "Rows",
        "index": 0, "values": {"Coord": {"X": 555, "Y": 666, "Z": 777}},
    })
    print(f"  status={r.get('status')}")
    if r.get("status") != "success":
        print(f"  FAIL: {get_data(r)}"); return False

    rows = read_rows(unreal)
    raw = (rows or {}).get("value", "") or (rows or {}).get("default_value", "")
    print(f"  Rows raw: {raw[:300]}")
    ok = "X=555" in raw and "Y=666" in raw and "Z=777" in raw
    print("  PASS" if ok else "  FAIL: set_struct_property did not write nested Coord")
    return ok


def main():
    print("=" * 60)
    print("Issue #11 fix verification (v0.10.2)")
    print("=" * 60)
    tests = [
        ("A) Coord via dict {X,Y,Z}",     test_A_dict_form_coord_written),
        ("B) Coord via array [X,Y,Z]",    test_B_array_form_coord_written),
        ("C) Coord via UE text",          test_C_string_form_coord_written),
        ("D) set_struct_property nested", test_D_set_struct_property_updates_nested),
    ]
    summary = []
    for name, fn in tests:
        try: ok = fn()
        except Exception as e:
            print(f"  EXCEPTION: {e}"); ok = False
        summary.append((name, ok))
    print("\n" + "=" * 60); print("SUMMARY"); print("=" * 60)
    for name, ok in summary:
        print(f"  [{'PASS' if ok else 'FAIL'}] {name}")
    passed = sum(1 for _, ok in summary if ok)
    print(f"\n{passed}/{len(summary)} passed")
    return 0 if passed == len(summary) else 1


if __name__ == "__main__":
    sys.exit(main())
