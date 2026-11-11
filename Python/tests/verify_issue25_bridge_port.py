"""Verification for Issue #25: the bridge port is configurable per editor.

This script checks ONE expectation about ONE port and is meant to be run
several times while editors are started with different settings:

  verify_issue25_bridge_port.py --port 55558 --project wt-b
      the editor listening on 55558 is the one whose .uproject path contains
      "wt-b" (get_project_info.project_file)

  verify_issue25_bridge_port.py --port 55559 --closed
      nothing is listening on 55559

  verify_issue25_bridge_port.py --port 55557 --project wt-a --repeat 20
      20 separate connections to 55557 all reach the same editor. This is the
      check for the original bug: with SO_REUSEADDR on Windows a second editor
      could bind the same port and connections went to either one.

The scenario that was run for the PR (two checkouts A and B, both built with
this change):
  1. A with no option            -> 55557 is A
  2. B with no option            -> B logs "Failed to bind listener socket";
                                    55557 is still always A
  3. B with -SpirrowBridgePort=55558          -> 55558 is B, 55557 is A
  4. B with [SpirrowBridge] Port=55559 in Config/DefaultEditor.ini
                                              -> 55559 is B
  5. B with that ini AND -SpirrowBridgePort=55558 -> 55558 is B, 55559 closed
  6. B with -SpirrowBridgePort=70000          -> warning, falls back to 55557

Exit code 0 when the expectation holds.
"""
import argparse, json, socket, sys

try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass


def send(port, command, params=None, timeout=10.0):
    with socket.create_connection(("127.0.0.1", port), timeout=timeout) as s:
        s.sendall(json.dumps({"type": command, "params": params or {}}).encode("utf-8"))
        s.settimeout(timeout)
        chunks = []
        while True:
            try:
                chunk = s.recv(65536)
            except socket.timeout:
                break
            if not chunk:
                break
            chunks.append(chunk)
            try:
                return json.loads(b"".join(chunks).decode("utf-8"))
            except ValueError:
                continue
    return json.loads(b"".join(chunks).decode("utf-8"))


def project_file(port):
    r = send(port, "get_project_info")
    inner = r.get("result", r)
    return inner.get("project_file", "")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, required=True)
    ap.add_argument("--project", help="substring expected in project_file")
    ap.add_argument("--closed", action="store_true", help="expect nothing listening")
    ap.add_argument("--repeat", type=int, default=1)
    args = ap.parse_args()

    if args.closed:
        try:
            send(args.port, "ping", timeout=3.0)
        except OSError as e:
            print(f"[PASS] port {args.port} is closed ({type(e).__name__})")
            return 0
        print(f"[FAIL] port {args.port} answered but should be closed")
        return 1

    seen = {}
    for _ in range(args.repeat):
        try:
            pf = project_file(args.port)
        except OSError as e:
            print(f"[FAIL] port {args.port}: {e}")
            return 1
        seen[pf] = seen.get(pf, 0) + 1

    ok = len(seen) == 1 and (not args.project or args.project.lower() in next(iter(seen)).lower())
    for pf, n in seen.items():
        print(f"  port {args.port} -> {pf}  x{n}")
    print(f"[{'PASS' if ok else 'FAIL'}] port {args.port} is {args.project or 'one editor'} on {args.repeat} connection(s)")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
