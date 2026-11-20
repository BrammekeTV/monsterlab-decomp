"""Build and run the PC loader's test tool (tests/sir_loader_test.cpp, src/port/sir_loader.cpp).

    python tools/loader_check.py [path/to/global_binary.sir]

1. Compiles the test tool with MSVC (Win32, the CI flags) into build/loader_test/.
2. Runs its self-test: a synthetic big-endian file built in memory (CI runs the same).
3. If the real file is there (default: orig/RM7P4F/files/streams/global_binary.sir; also looked
   for in the main checkout when run from a git worktree), loads it with the C++ loader and
   compares every header slot, table descriptor and row with the Python reference decoder
   (`tools/sir_decoder.py --gamedb`), which reads the big-endian bytes directly.

Game data and the dumps stay local (orig/, build/); nothing here is committed.
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import check            # vcvars(), FLAGS
import sir_decoder

REL = os.path.join("orig", "RM7P4F", "files", "streams", "global_binary.sir")
SAMPLES = ("part 0 ", "part 57 ", "part 185 ", "action 0 ", "action 200 ", "action 371 ",
           "tuning 0 ", "tuning 100 ", "tuning 214 ", "row 39 0 ", "row 41 0 ")


def find_data():
    if len(sys.argv) > 1:
        return sys.argv[1]
    roots = [ROOT]
    # In a worktree, orig/ only exists in the main checkout.
    r = subprocess.run(["git", "rev-parse", "--path-format=absolute", "--git-common-dir"], cwd=ROOT,
                       capture_output=True, text=True)
    if r.returncode == 0 and r.stdout.strip():
        roots.append(os.path.dirname(r.stdout.strip()))
    for root in roots:
        p = os.path.join(root, REL)
        if os.path.exists(p):
            return p
    return None


def build():
    out = os.path.join(ROOT, "build", "loader_test")
    os.makedirs(out, exist_ok=True)
    exe = os.path.join(out, "sir_loader_test.exe")
    flags = [f for f in check.FLAGS if f != "/c"]
    srcs = [os.path.join(ROOT, "tests", "sir_loader_test.cpp"), os.path.join(ROOT, "src", "port", "sir_loader.cpp")]
    script = os.path.join(out, "build.bat")
    with open(script, "w") as f:
        f.write('@call "' + check.vcvars() + '" >nul\r\n')
        f.write("@cl " + " ".join(flags) + " /O2 /Fo\"" + out + '\\\\" /Fe"' + exe + '" ' +
                " ".join('"' + s + '"' for s in srcs) + "\r\n")
    r = subprocess.run(["cmd", "/c", script], capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout[-4000:], r.stderr[-2000:])
        sys.exit("build failed")
    return exe


def main():
    exe = build()
    r = subprocess.run([exe, "--selftest"], capture_output=True, text=True)
    print(r.stdout.strip())
    if r.returncode != 0:
        return 1

    data = find_data()
    if data is None:
        print("real-file check skipped: " + REL + " not found")
        return 0
    r = subprocess.run([exe, "--dump", data], capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout[-2000:], r.stderr[-2000:])
        return 1
    got = r.stdout.splitlines()
    report = [l for l in got if l.startswith("report ")]
    got = [l for l in got if not l.startswith("report ")]
    want = sir_decoder.gamedb_dump(data)

    bad = [(i, w, g) for i, (w, g) in enumerate(zip(want, got)) if w != g]
    for i, w, g in bad[:10]:
        print(f"line {i + 1}:\n  reference {w}\n  loader    {g}")
    kinds = {}
    for l in want:
        kinds[l.split(" ", 1)[0]] = kinds.get(l.split(" ", 1)[0], 0) + 1
    print("compared: " + ", ".join(f"{n} {k}" for k, n in kinds.items()))
    for s in SAMPLES:
        for l in got:
            if l.startswith(s):
                print("  " + l)
                break
    print("loader " + (report[0] if report else "report missing"))
    if bad or len(want) != len(got):
        print(f"MISMATCH: {len(bad)} differing lines, {len(want)} reference vs {len(got)} loader lines")
        return 1
    print(f"OK: all {len(want)} lines match the reference decoder")
    return 0


if __name__ == "__main__":
    sys.exit(main())
