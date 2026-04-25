"""
Reproduction (2026-04-26): take_pie_screenshot and take_high_res_screenshot
do NOT capture the PIE pawn POV — they capture the (fixed) editor viewport.

Procedure:
  1. start_pie
  2. set_pie_camera to wildly different positions (3200,3200,5000) and (-50000,-50000,10000)
  3. take_pie_screenshot at each position
  4. take_high_res_screenshot at each position
  5. Compare PNGs pixel-wise.

Expected (if bug exists): screenshots are essentially identical despite
50km+ separation (proves editor viewport is captured, not PIE POV).

Expected (if bug fixed): screenshots differ significantly.
"""
import sys, os, time, tempfile, hashlib
try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_server import get_unreal_connection


def png_diff_pct(path_a, path_b):
    """Return (% pixels differ, mean per-channel abs diff). Resizes B to A's size if needed."""
    from PIL import Image, ImageChops
    a = Image.open(path_a).convert("RGB")
    b = Image.open(path_b).convert("RGB")
    if a.size != b.size:
        b = b.resize(a.size)
    diff = ImageChops.difference(a, b)
    pixels_a = list(a.getdata())
    pixels_b = list(b.getdata())
    n = len(pixels_a)
    differing = sum(1 for pa, pb in zip(pixels_a, pixels_b) if pa != pb)
    total_chan_diff = 0
    for pa, pb in zip(pixels_a, pixels_b):
        for ca, cb in zip(pa, pb):
            total_chan_diff += abs(ca - cb)
    return (differing / n) * 100.0, total_chan_diff / (n * 3)


def file_md5(p):
    h = hashlib.md5()
    with open(p, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()


def shoot(unreal, kind, location, rotation, out_path, label):
    print(f"\n  [{label}] set_pie_camera loc={location} rot={rotation}")
    r = unreal.send_command("set_pie_camera", {"location": location, "rotation": rotation})
    if r.get("status") != "success":
        print(f"  set_pie_camera failed: {r}")
        return False
    time.sleep(1.5)  # let teleport settle

    print(f"  [{label}] {kind} -> {out_path}")
    if os.path.exists(out_path):
        os.remove(out_path)

    if kind == "take_pie_screenshot":
        r = unreal.send_command(kind, {"filepath": out_path})
        if r.get("status") != "success":
            print(f"  {kind} failed: {r}"); return False
        return os.path.exists(out_path)
    else:
        # take_high_res_screenshot is async, poll for file
        r = unreal.send_command(kind, {"multiplier": 1, "filepath": out_path})
        if r.get("status") != "success":
            print(f"  {kind} failed: {r}"); return False
        waited = 0
        while waited < 60 and not os.path.exists(out_path):
            time.sleep(1.0); waited += 1.0
        if os.path.exists(out_path):
            print(f"  [{label}] hires PNG written after {waited:.0f}s")
            return True
        print(f"  [{label}] hires PNG NOT written after 60s")
        return False


def main():
    print("=" * 70)
    print("Repro: PIE POV not captured by take_pie_screenshot / HighResShot")
    print("=" * 70)

    unreal = get_unreal_connection()
    if not unreal:
        print("ERROR: cannot connect to MCP/UE"); return 1

    unreal.send_command("stop_pie", {})
    time.sleep(2)

    print("\nstart_pie ...")
    r = unreal.send_command("start_pie", {"spawn_at_player_start": True})
    if r.get("status") != "success":
        print(f"start_pie failed: {r}"); return 1
    time.sleep(4)  # let world finish loading

    tmp = tempfile.gettempdir()
    paths = {
        ("take_pie_screenshot", "near"): os.path.join(tmp, "repro_pov_near_pie.png"),
        ("take_pie_screenshot", "far"):  os.path.join(tmp, "repro_pov_far_pie.png"),
        ("take_high_res_screenshot", "near"): os.path.join(tmp, "repro_pov_near_hires.png"),
        ("take_high_res_screenshot", "far"):  os.path.join(tmp, "repro_pov_far_hires.png"),
    }

    ok = True
    # Pose A: near origin, low alt
    ok &= shoot(unreal, "take_pie_screenshot",      [3200, 3200, 5000],     [-45, 0, 0],  paths[("take_pie_screenshot", "near")], "PIE-near")
    # Pose B: 50km away, high alt
    ok &= shoot(unreal, "take_pie_screenshot",      [-50000, -50000, 10000], [-45, 45, 0], paths[("take_pie_screenshot", "far")],  "PIE-far")
    ok &= shoot(unreal, "take_high_res_screenshot", [3200, 3200, 5000],     [-45, 0, 0],  paths[("take_high_res_screenshot", "near")], "HR-near")
    ok &= shoot(unreal, "take_high_res_screenshot", [-50000, -50000, 10000], [-45, 45, 0], paths[("take_high_res_screenshot", "far")],  "HR-far")

    unreal.send_command("stop_pie", {})

    if not ok:
        print("\nERROR: at least one screenshot failed; see above")
        return 1

    print("\n" + "=" * 70)
    print("RESULT")
    print("=" * 70)
    for kind in ("take_pie_screenshot", "take_high_res_screenshot"):
        a = paths[(kind, "near")]; b = paths[(kind, "far")]
        md_a = file_md5(a); md_b = file_md5(b)
        print(f"\n[{kind}]")
        print(f"  near md5: {md_a}")
        print(f"  far  md5: {md_b}")
        if md_a == md_b:
            print(f"  IDENTICAL files — captures editor viewport (BUG present)")
        else:
            try:
                pct, mean = png_diff_pct(a, b)
                print(f"  diff: {pct:.2f}% pixels differ, mean per-channel diff {mean:.2f}/255")
                if pct < 1.0:
                    print(f"  -> < 1% differ: still effectively identical (BUG present)")
                else:
                    print(f"  -> significant difference: PIE POV likely captured (BUG fixed)")
            except ImportError:
                print("  Pillow not installed; cannot compute diff %")
    print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
