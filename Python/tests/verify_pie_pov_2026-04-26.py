"""
Verify (2026-04-26 follow-up to PR #10): take_pie_pov_screenshot captures
the PIE pawn's actual viewpoint via SceneCapture2D, independent of viewport routing.

Procedure:
  1. start_pie
  2. set_pie_camera to 3 wildly different positions
  3. take_pie_pov_screenshot at each
  4. Verify all 3 PNGs differ significantly (mean per-channel diff > 5/255)
  5. As control: run the same with take_pie_screenshot — show that broken/uniform output
     is what the new command replaces.

PASS = take_pie_pov_screenshot pose pairs all differ meaningfully
       (proves PIE pawn POV is captured, not editor viewport)
"""
import sys, os, time, tempfile, hashlib
try: sys.stdout.reconfigure(encoding='utf-8', errors='replace')
except Exception: pass
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_server import get_unreal_connection


def file_md5(p):
    h = hashlib.md5()
    with open(p, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()


def png_mean_diff(path_a, path_b):
    from PIL import Image
    a = Image.open(path_a).convert("RGB")
    b = Image.open(path_b).convert("RGB")
    if a.size != b.size: b = b.resize(a.size)
    pa = list(a.getdata()); pb = list(b.getdata())
    total = 0
    for x, y in zip(pa, pb):
        for cx, cy in zip(x, y): total += abs(cx - cy)
    return total / (len(pa) * 3)


def shoot_pose(unreal, kind, loc, rot, out_path):
    if os.path.exists(out_path): os.remove(out_path)
    r = unreal.send_command("set_pie_camera", {"location": loc, "rotation": rot})
    if r.get("status") != "success":
        print(f"    set_pie_camera failed: {r}"); return False
    time.sleep(1.0)
    r = unreal.send_command(kind, {"filepath": out_path})
    if r.get("status") != "success":
        print(f"    {kind} failed: {r}"); return False
    if not os.path.exists(out_path):
        print(f"    {kind} did not produce file"); return False
    return True


def main():
    print("=" * 70)
    print("Verify take_pie_pov_screenshot captures PIE pawn POV")
    print("=" * 70)

    unreal = get_unreal_connection()
    if not unreal: print("ERROR: cannot connect"); return 1

    unreal.send_command("stop_pie", {}); time.sleep(2)
    print("\nstart_pie ...")
    r = unreal.send_command("start_pie", {"spawn_at_player_start": True})
    if r.get("status") != "success":
        print(f"start_pie failed: {r}"); return 1
    time.sleep(4)

    poses = [
        ("near_low",  [3200, 3200, 5000],     [-45,   0, 0]),
        ("far_high",  [-50000, -50000, 10000], [-45,  45, 0]),
        ("orbit",     [0, 8000, 2500],        [-15, 270, 0]),
    ]

    tmp = tempfile.gettempdir()
    pov_paths = {}
    pie_paths = {}

    print("\nCapturing with take_pie_pov_screenshot:")
    for name, loc, rot in poses:
        out = os.path.join(tmp, f"verify_pov_{name}.png")
        ok = shoot_pose(unreal, "take_pie_pov_screenshot", loc, rot, out)
        if not ok:
            print(f"  FAIL at pose {name}"); unreal.send_command("stop_pie", {}); return 1
        pov_paths[name] = out
        print(f"  [{name}] -> {out} ({os.path.getsize(out)} bytes, md5 {file_md5(out)[:8]})")

    print("\nCapturing with take_pie_screenshot (control / baseline):")
    for name, loc, rot in poses:
        out = os.path.join(tmp, f"verify_pieshot_{name}.png")
        ok = shoot_pose(unreal, "take_pie_screenshot", loc, rot, out)
        if not ok:
            print(f"  WARN take_pie_screenshot at pose {name} failed (continuing)")
            continue
        pie_paths[name] = out
        print(f"  [{name}] -> {out} ({os.path.getsize(out)} bytes, md5 {file_md5(out)[:8]})")

    unreal.send_command("stop_pie", {})

    print("\n" + "=" * 70)
    print("RESULT — pairwise mean per-channel diff (0-255 scale)")
    print("=" * 70)

    THRESHOLD = 5.0  # mean diff > 5/255 = ~2% mean delta = clearly different content

    print("\n[take_pie_pov_screenshot] (target — must differ across poses)")
    pov_pairs_pass = True
    names = [n for n, _, _ in poses]
    for i in range(len(names)):
        for j in range(i + 1, len(names)):
            a = names[i]; b = names[j]
            try:
                d = png_mean_diff(pov_paths[a], pov_paths[b])
            except ImportError:
                print("  Pillow not installed; cannot compute diff"); return 1
            mark = "PASS" if d > THRESHOLD else "FAIL"
            if d <= THRESHOLD: pov_pairs_pass = False
            print(f"  [{mark}] {a:>8s} vs {b:>8s}: mean diff {d:.2f}/255")

    if pie_paths:
        print("\n[take_pie_screenshot] (control — informational)")
        for i in range(len(names)):
            for j in range(i + 1, len(names)):
                a = names[i]; b = names[j]
                if a not in pie_paths or b not in pie_paths: continue
                try:
                    d = png_mean_diff(pie_paths[a], pie_paths[b])
                except ImportError:
                    break
                print(f"  [info] {a:>8s} vs {b:>8s}: mean diff {d:.2f}/255")

    print()
    if pov_pairs_pass:
        print("=> PASS: take_pie_pov_screenshot reflects PIE pawn position")
        return 0
    else:
        print("=> FAIL: at least one pose pair is too similar — POV not changing")
        return 1


if __name__ == "__main__":
    sys.exit(main())
