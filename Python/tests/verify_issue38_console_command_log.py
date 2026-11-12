"""Verification for Issue #38.

exec_console_command returned an empty "output" for commands that report
through the log instead of the output device they are handed - `py` above all
(print / unreal.log go to LogPython). The response now carries the log lines
written while the command ran, as "log".

Prereq: MCPGameProject editor running with the rebuilt SpirrowBridge plugin
and the PythonScriptPlugin enabled (it is by default).
"""
import sys, os, time
try:
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
except Exception:
    pass
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from unreal_mcp_server import get_unreal_connection


STAMP = time.strftime("%H%M%S")
RESULTS = []


def data(r):
    if not isinstance(r, dict):
        return {}
    inner = r.get("result", r)
    return inner.get("data", inner) if isinstance(inner, dict) else {}


def ok(r):
    return isinstance(r, dict) and r.get("status") != "error" and data(r).get("success") is not False


def console(command, **params):
    unreal = get_unreal_connection()
    if not unreal:
        raise RuntimeError("no connection to the editor")
    r = unreal.send_command("exec_console_command", {"command": command, "target": "editor", **params})
    if r is None:
        raise RuntimeError("exec_console_command: no response")
    return r


def check(name, cond, detail=""):
    RESULTS.append((name, bool(cond)))
    print(f"  [{'PASS' if cond else 'FAIL'}] {name}" + (f"  ({detail})" if detail and not cond else ""))


def messages(d, category=None):
    return [l["message"] for l in d.get("log", []) if category is None or l.get("category") == category]


def wait_for_python(timeout_s=120):
    """Right after the editor starts, the Python plugin is not initialized yet
    and the engine queues `py` commands to run later instead of running them
    (FPythonScriptPlugin::Exec_Runtime -> DeferredCommands). Such a command
    logs after the response, so wait until `py` runs inline."""
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        d = data(console("py print('I38' + 'READY')"))
        if any("I38READY" in m for m in messages(d, "LogPython")):
            return True
        time.sleep(2)
    return False


def main():
    print("=" * 70)
    print("Issue #38 verification")
    print("=" * 70)

    check("the editor runs py commands inline (Python initialized)", wait_for_python())

    print("\n--- py: the value the script computed comes back ---")
    # The expected text (the product) is not part of the command string.
    r = console(f"py print('I38-' + str(6 * 7) + '-{STAMP}')")
    d = data(r)
    expect = f"I38-42-{STAMP}"
    check("command succeeds", ok(r), str(r))
    check("'log' is a list", isinstance(d.get("log"), list), str(d))
    check("the printed value is in log", any(expect in m for m in messages(d)), str(d.get("log")))
    check("it is attributed to LogPython", any(expect in m for m in messages(d, "LogPython")), str(d.get("log")))
    check("'output' is still present", "output" in d, str(d))
    check("no errors counted", d.get("log_error_count") == 0, str(d))
    check("log_line_count matches", d.get("log_line_count") == len(d.get("log", [])) and d.get("log_truncated") is False, str(d))

    print("\n--- py: an engine call's return value (the case from the issue) ---")
    d = data(console("py import unreal; unreal.log('I38RET ' + str(unreal.SystemLibrary.get_project_directory() != ''))"))
    check("return value readable from the response", any("I38RET True" in m for m in messages(d)), str(d.get("log")))

    print("\n--- verbosity and failure are visible without parsing text ---")
    d = data(console(f"py import unreal; unreal.log_warning('I38WARN-{STAMP}')"))
    # The command is echoed as a "Cmd" line, so look at LogPython lines only.
    warn = [l for l in d.get("log", []) if f"I38WARN-{STAMP}" in l["message"] and l.get("category") == "LogPython"]
    check("warning line has verbosity 'Warning'", bool(warn) and warn[0].get("verbosity") == "Warning", str(d.get("log")))
    check("log_warning_count >= 1", d.get("log_warning_count", 0) >= 1, str(d))

    r = console("py raise RuntimeError('I38BOOM')")
    d = data(r)
    check("a raising script: log_error_count >= 1", d.get("log_error_count", 0) >= 1, str(d))
    check("a raising script: the exception text is in log", any("I38BOOM" in m for m in messages(d, "LogPython")), str(d.get("log")))
    check("a raising script: error lines have verbosity 'Error'",
          any(l.get("verbosity") == "Error" for l in d.get("log", [])), str(d.get("log")))

    print("\n--- limits and opt-out ---")
    d = data(console("py [print('I38LINE', i) for i in range(10)]", max_log_lines=3))
    check("max_log_lines caps the list", len(d.get("log", [])) == 3, str(len(d.get("log", []))))
    check("log_truncated is set", d.get("log_truncated") is True and d.get("log_line_count", 0) >= 10, str(d))

    d = data(console("py print('I38OFF')", capture_log=False))
    check("capture_log=false omits 'log'", "log" not in d and "log_line_count" not in d, str(d))
    check("capture_log=false still returns output", "output" in d and "exec_returned_true" in d, str(d))

    print("\n--- nothing leaks between commands ---")
    d = data(console(f"py print('I38SECOND-{STAMP}')"))
    check("an earlier command's lines are not repeated",
          not any("I38LINE" in m or "I38BOOM" in m for m in messages(d)), str(d.get("log")))
    check("the capture device is removed afterwards (editor still answers)", ok(console("py pass")))

    passed = sum(1 for _, c in RESULTS if c)
    print("\n" + "=" * 70)
    print(f"{passed}/{len(RESULTS)} checks passed")
    for name, c in RESULTS:
        if not c:
            print(f"  FAILED: {name}")
    return 0 if passed == len(RESULTS) and RESULTS else 1


if __name__ == "__main__":
    sys.exit(main())
