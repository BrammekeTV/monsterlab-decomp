"""Find the sources that break when optimised.

    python tools/opt_bisect.py test <ALL|NONE|PORT|listfile> [--runs 2]
    python tools/opt_bisect.py bisect [--base PORT]     (delta-debug the game sources that fail at /O2)

Every source is compiled twice (cached in build/optbisect/o2 and /od, flags of tools/build_pc.py); a test
copies the chosen mix into build/optbisect/mix, links it with tools/port_link.py and boots it headless:
--frames 3200 --press A@2600-2610 --seconds 300 --no-audio, twice (runs are timing dependent).
Pass = the frame limit was reached and the log shows the main menu was reached (MENU_RE).
Needs the game data (MONSTERLAB_DATA or orig/RM7P4F found upwards from the exe or the cwd).
A listfile has one src/ path per line = optimised.
"""
import glob, os, re, shutil, subprocess, sys, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import check, build_pc

ROOT = check.ROOT
B = os.path.join(ROOT, "build", "optbisect")
MIX = os.path.join(B, "mix")
EXE = os.path.join(B, "monsterlab.exe")
MENU_RE = re.compile(r"\[script\] EnableMainMenuDebugOptions")   # the main menu script runs (PORT_SCRIPT_TRACE)
ARGS = ["--frames", "3200", "--press", "A@2600-2610", "--seconds", "300", "--no-audio"]
FILES = sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True))
rel = lambda p: os.path.relpath(p, ROOT).replace("\\", "/")
stem = lambda p: os.path.splitext(os.path.basename(p))[0] + ".obj"


def compile_variant(tag, flags):
    d = os.path.join(B, tag)
    os.makedirs(d, exist_ok=True)
    todo = [p for p in FILES if not os.path.exists(os.path.join(d, stem(p)))]   # delete an .obj to recompile it
    if not todo:
        return
    rsp = os.path.join(B, tag + ".rsp")
    open(rsp, "w").write("\n".join('"' + p + '"' for p in FILES))
    bat = os.path.join(B, tag + ".bat")
    open(bat, "w").write("@call \"%s\" >nul 2>nul\r\n@cl /MP %s %s /Fo\"%s\\\\\" @\"%s\" >\"%s\%s.log\" 2>&1\r\n" % (
        check.vcvars(), " ".join(check.FLAGS), flags, d, rsp, B, tag))
    if subprocess.run(["cmd", "/c", bat]).returncode:
        sys.exit("compile failed: build/optbisect/%s.log" % tag)


def link(opt):
    shutil.rmtree(MIX, ignore_errors=True)
    os.makedirs(MIX)
    for p in FILES:
        shutil.copy(os.path.join(B, "o2" if p in opt else "od", stem(p)), MIX)
    bat = os.path.join(B, "link.bat")
    open(bat, "w").write("@call \"%s\" >nul 2>nul\r\n@\"%s\" \"%s\" --objs \"%s\" --out \"%s\" >nul\r\n@if errorlevel 1 exit /b 2\r\n"
                         "@link %s /OUT:\"%s\" \"%s\*.obj\" \"%s\port_gen.obj\" >\"%s\link.log\" 2>&1\r\n@exit /b %%errorlevel%%\r\n" % (
        check.vcvars(), sys.executable, os.path.join(ROOT, "tools", "port_link.py"), MIX, B,
        " ".join(build_pc.LINK_FLAGS), EXE, MIX, B, B))
    if subprocess.run(["cmd", "/c", bat]).returncode:
        sys.exit("link failed: build/optbisect/link.log")


def boot():
    t = time.time()
    p = subprocess.run([EXE] + ARGS, cwd=ROOT, capture_output=True, text=True, errors="replace",
                       env=dict(os.environ, PORT_SCRIPT_TRACE="1"))
    log = p.stdout + p.stderr
    ok = "frame limit reached" in log and bool(MENU_RE.search(log))
    stop = [l.strip() for l in log.splitlines() if "STOP" in l or "crash" in l]
    return ok, time.time() - t, stop[:1]


def test(opt, runs=2, quiet=False):
    link(opt)
    res = [boot() for _ in range(runs)]
    ok = all(r[0] for r in res)
    if not quiet or not ok:
        print("opt=%d %s %s" % (len(opt), "PASS" if ok else "FAIL",
              " | ".join("%.1fs %.1ffps %s" % (r[1], 3200 / r[1], r[2][0] if r[2] else "") for r in res)), flush=True)
    return ok


def select(sel):
    if sel == "ALL": return set(FILES)
    if sel == "NONE": return set()
    if sel == "PORT": return {p for p in FILES if rel(p).startswith("src/port/")}
    want = {l.strip() for l in open(sel) if l.strip()}
    return {p for p in FILES if rel(p) in want}


def bisect(base):
    """Smallest set of files that, optimised on top of base, fail (ddmin on the failing set)."""
    game = [p for p in FILES if p not in base]
    cand = game
    n = 2
    while len(cand) >= 2:
        size = max(1, len(cand) // n)
        chunks = [cand[i:i + size] for i in range(0, len(cand), size)]
        for c in chunks:
            if not test(base | set(c), 1, True):
                cand, n = c, 2
                break
        else:
            hit = False
            for c in chunks:
                rest = [p for p in cand if p not in c]
                if rest and not test(base | set(rest), 1, True):
                    cand, n, hit = rest, max(n - 1, 2), True
                    break
            if not hit:
                if n >= len(cand): break
                n = min(len(cand), n * 2)
    print("failing set:", [rel(p) for p in cand])
    return cand


def main():
    a = sys.argv[1:]
    if not a: sys.exit(__doc__)
    os.makedirs(B, exist_ok=True)
    compile_variant("o2", "/O2 /DNDEBUG")
    compile_variant("od", "/Od")
    if a[0] == "test":
        runs = int(a[a.index("--runs") + 1]) if "--runs" in a else 2
        sys.exit(0 if test(select(a[1]), runs) else 1)
    if a[0] == "bisect":
        base = select(a[a.index("--base") + 1]) if "--base" in a else select("PORT")
        bisect(base)


if __name__ == "__main__":
    main()
