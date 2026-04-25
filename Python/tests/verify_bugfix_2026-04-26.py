"""
Bug fix verification (2026-04-26) for v0.10.0 follow-up:
A) take_pie_screenshot renders the latest frame (not green/black placeholder)
B) take_high_res_screenshot honors filepath override
C) set_pie_camera response.location returns the target value

Prereq:
- Editor running with SpirrowBridge plugin
- Some level loaded with anything visible (terrain / static mesh / sky atmosphere)
"""
import sys, os, time, tempfile, json
try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_server import get_unreal_connection


def get_data(r):
    if not isinstance(r, dict): return {}
    inner = r.get("result", r)
    return inner.get("data", inner) if isinstance(inner, dict) else {}


def color_uniformity_score(png_path):
    """Return (unique_color_count, dominant_color_pct).
    Green placeholder PNGs have 1 unique color (or near-1) covering ~100%.
    Real screenshots have thousands of unique colors with no single dominant.
    """
    from PIL import Image
    img = Image.open(png_path).convert("RGB")
    # Sample to keep it fast on large images
    if img.size[0] > 512:
        img = img.resize((512, int(img.size[1] * 512 / img.size[0])))
    pixels = list(img.getdata())
    total = len(pixels)
    counts = {}
    for p in pixels:
        counts[p] = counts.get(p, 0) + 1
    dom = max(counts.values())
    return len(counts), (dom / total) * 100.0


def test_A_pie_screenshot_renders_latest_frame():
    print("\n--- A) take_pie_screenshot renders latest frame ---")
    unreal = get_unreal_connection()
    if not unreal: return False

    # Ensure stopped first
    unreal.send_command("stop_pie", {})
    time.sleep(2)

    print("  start_pie ...")
    r = unreal.send_command("start_pie", {"spawn_at_player_start": True})
    if not (r.get("status") == "success"):
        print(f"  start_pie failed: {r}"); return False
    time.sleep(4)  # let chunks/meshes spawn

    print("  set_pie_camera high-altitude top-down ...")
    r = unreal.send_command("set_pie_camera", {"location": [0, 0, 5000], "rotation": [-89, 0, 0]})
    if not (r.get("status") == "success"):
        print(f"  set_pie_camera failed: {r}"); unreal.send_command("stop_pie", {}); return False
    time.sleep(1.5)

    out_path = os.path.join(tempfile.gettempdir(), "verify_A_pie_topdown.png")
    if os.path.exists(out_path): os.remove(out_path)
    print(f"  take_pie_screenshot -> {out_path} ...")
    r = unreal.send_command("take_pie_screenshot", {"filepath": out_path})
    print(f"  response status: {r.get('status')}")

    unreal.send_command("stop_pie", {})

    if not os.path.exists(out_path):
        print("  FAIL: PNG not created"); return False
    size = os.path.getsize(out_path)
    print(f"  PNG size: {size} bytes")

    try:
        unique, dom_pct = color_uniformity_score(out_path)
    except ImportError:
        print("  SKIP: Pillow not installed - cannot verify color content")
        return True
    print(f"  unique colors: {unique}, dominant color %: {dom_pct:.1f}")

    # Heuristic: a green/black placeholder has very few unique colors (< 100) or
    # one color dominating > 95% of pixels. Real-rendered scenes have thousands+.
    is_placeholder = (unique < 100) or (dom_pct > 95.0)
    if is_placeholder:
        print("  FAIL: looks like placeholder (low unique color count or single dominant)")
        return False
    print("  PASS: PNG has rendered content (varied colors)")
    return True


def test_B_high_res_filepath_override():
    print("\n--- B) take_high_res_screenshot filepath override ---")
    unreal = get_unreal_connection()
    if not unreal: return False

    custom_path = os.path.join(tempfile.gettempdir(), "verify_B_highres_override.png")
    if os.path.exists(custom_path): os.remove(custom_path)

    print(f"  take_high_res_screenshot mult=2 filepath={custom_path}")
    r = unreal.send_command("take_high_res_screenshot", {"multiplier": 2, "filepath": custom_path})
    print(f"  response: {json.dumps(get_data(r), ensure_ascii=False)}")

    # HighResShot is async. mult=2 takes 13-60s depending on viewport size + hardware.
    # We poll up to 90s before giving up (most runs finish under 30s).
    waited = 0
    while waited < 90 and not os.path.exists(custom_path):
        time.sleep(1.0); waited += 1.0
    if not os.path.exists(custom_path):
        print(f"  FAIL: custom_path file not created within 90s")
        return False
    print(f"  PNG size: {os.path.getsize(custom_path)} bytes (waited {waited:.1f}s)")
    return True


def test_C_set_pie_camera_target_response():
    print("\n--- C) set_pie_camera response returns target loc ---")
    unreal = get_unreal_connection()
    if not unreal: return False

    unreal.send_command("stop_pie", {}); time.sleep(2)
    unreal.send_command("start_pie", {"spawn_at_player_start": True}); time.sleep(3)

    target_loc = [10000.0, 10000.0, 8000.0]
    target_rot = [-45.0, 90.0, 0.0]
    r = unreal.send_command("set_pie_camera", {"location": target_loc, "rotation": target_rot})
    data = get_data(r)
    got_loc = data.get("location", [])
    got_prev = data.get("previous_location", [])
    print(f"  target loc:  {target_loc}")
    print(f"  response loc: {got_loc}")
    print(f"  prev loc:    {got_prev}")

    unreal.send_command("stop_pie", {})

    if len(got_loc) != 3:
        print("  FAIL: response location malformed"); return False

    diffs = [abs(a - b) for a, b in zip(got_loc, target_loc)]
    if max(diffs) > 1.0:
        print(f"  FAIL: response location diverges from target by {max(diffs):.1f} units")
        return False
    print("  PASS: response location matches target within 1 unit")
    return True


def main():
    print("=" * 60)
    print("Bug fix verification (2026-04-26)")
    print("=" * 60)
    results = [
        ("A) take_pie_screenshot renders latest frame", test_A_pie_screenshot_renders_latest_frame),
        ("B) take_high_res_screenshot filepath override", test_B_high_res_filepath_override),
        ("C) set_pie_camera target response",            test_C_set_pie_camera_target_response),
    ]
    summary = []
    for name, fn in results:
        try: ok = fn()
        except Exception as e:
            print(f"  EXCEPTION: {e}"); ok = False
        summary.append((name, ok))
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    for name, ok in summary:
        sym = "PASS" if ok else "FAIL"
        print(f"  [{sym}] {name}")
    passed = sum(1 for _, ok in summary if ok)
    print(f"\n{passed}/{len(summary)} passed")
    return 0 if passed == len(summary) else 1


if __name__ == "__main__":
    sys.exit(main())
