"""Build the PC executable locally with MSVC (CI builds the same through CMake).

    python tools/build_pc.py [--debug] [--noopt] [--clean] [--run [args...]]

Compiles every source (32-bit, the flags of tools/check.py), generates the link glue with
tools/port_link.py and links build/pc/monsterlab.exe. See docs/port_runtime.md.
  --debug   keep debug information (/Z7, /DEBUG)
  --noopt   build without optimisation (/Od everywhere), for stepping through code in a debugger
  --clean   recompile everything (otherwise only sources newer than their object are recompiled)
  --run     run the exe afterwards; the remaining arguments are passed to it

Incremental: build/pc/obj/stamp.json holds a signature of every header (include/, src/, third_party/),
the compiler flags and the file list. When any of them differs from the last build, everything is
recompiled; otherwise only the sources changed since their object was built. The link is skipped
when nothing was recompiled and the exe is newer than the objects and the link inputs.

Everything is built with /O2 (--noopt: /Od everywhere). The decompiled game code used to stay at /Od
because some of it broke when optimised; tools/opt_bisect.py found them (3 callers of fn_8018D68C
with a 2-float buffer for its 12 bytes, and generated leaf functions returning u8/u16 that callers
read as int). CI cannot boot the game (no game data), so use tools/opt_bisect.py test ALL locally.
"""
import glob, hashlib, json, os, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import check

ROOT = check.ROOT
OUT = os.path.join(ROOT, "build", "pc")
OBJ = os.path.join(OUT, "obj")
# The Wii address space is mapped at 0x80000000 and 0x90000000, so the exe needs the upper 2 GB
# (/LARGEADDRESSAWARE) and a fixed base (/FIXED): port_gen.obj holds absolute addresses.
LINK_FLAGS = ["/NOLOGO", "/FIXED", "/BASE:0x400000", "/DYNAMICBASE:NO", "/LARGEADDRESSAWARE",
              "/STACK:0x400000", "/SUBSYSTEM:CONSOLE", "/INCREMENTAL:NO", "/MAP"]


def main():
    argv = sys.argv[1:]
    run_args = None
    if "--run" in argv:
        i = argv.index("--run")
        run_args, argv = argv[i + 1:], argv[:i]
    debug = "--debug" in argv
    os.makedirs(OBJ, exist_ok=True)
    files = sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True))
    names = {}
    for p in files:
        b = os.path.basename(p).lower()
        if b in names:
            sys.exit(f"two sources named {b}: {p} and {names[b]}")
        names[b] = p
    port_opt = " /Od" if "--noopt" in argv else " /O2 /DNDEBUG"
    extra = (" /Z7" if debug else "")
    # Full rebuild when a header, the flags or the file list changed since the last build.
    sig = hashlib.sha1()
    for d in ("include", "src", "third_party"):
        for h in sorted(glob.glob(os.path.join(ROOT, d, "**", "*.h*"), recursive=True)):
            st = os.stat(h)
            sig.update(f"{os.path.relpath(h, ROOT)}|{st.st_size}|{st.st_mtime_ns};".encode())
    sig.update(("|".join(check.FLAGS) + port_opt + extra + ";" + ";".join(os.path.relpath(p, ROOT) for p in files)).encode())
    stamp = os.path.join(OBJ, "stamp.json")
    try:
        old_sig = json.load(open(stamp)).get("sig")
    except (OSError, ValueError):
        old_sig = None
    full = "--clean" in argv or old_sig != sig.hexdigest()
    if full:
        for old in glob.glob(os.path.join(OBJ, "*.obj")):
            os.remove(old)
        if os.path.exists(stamp):
            os.remove(stamp)
    else:
        wanted = {os.path.splitext(os.path.basename(p))[0].lower() for p in files}
        for old in glob.glob(os.path.join(OBJ, "*.obj")):   # a source that was removed or renamed
            if os.path.splitext(os.path.basename(old))[0].lower() not in wanted:
                os.remove(old)
    def stale(p):
        o = os.path.join(OBJ, os.path.splitext(os.path.basename(p))[0] + ".obj")
        return not os.path.exists(o) or os.path.getmtime(p) > os.path.getmtime(o)
    # Every source is built with the same flags (/O2 since #113; --noopt for /Od).
    groups = [([p for p in files if stale(p)], port_opt)]
    n_compile = sum(len(g) for g, _ in groups)
    print(f"{'full rebuild' if full else 'incremental'}: {n_compile} of {len(files)} sources to compile")
    compile_lines = ['@type nul >"' + OUT + '\compile.log"']
    for n, (group, opt) in enumerate(groups):
        if not group:
            continue
        rsp = os.path.join(OUT, f"files{n}.rsp")
        with open(rsp, "w") as f:
            f.write("\n".join('"' + p + '"' for p in group))
        compile_lines += ["@cl /MP " + " ".join(check.FLAGS) + opt + extra + ' /Fo"' + OBJ + '\\\\" @"' + rsp + '" >>"' + OUT + '\compile.log" 2>&1',
                          "@if errorlevel 1 exit /b 1"]
    exe = os.path.join(OUT, "monsterlab.exe")
    gen = '"' + sys.executable + '" "' + os.path.join(ROOT, "tools", "port_link.py") + '" --objs "' + OBJ + '" --out "' + OUT + '"'
    bat = os.path.join(OUT, "build.bat")
    with open(bat, "w") as f:
        f.write("\r\n".join([
            '@call "' + check.vcvars() + '" >nul 2>nul'] + compile_lines + [
            "@" + gen,
            "@if errorlevel 1 exit /b 2",
            '@link ' + " ".join(LINK_FLAGS) + (" /DEBUG" if debug else "") + ' /OUT:"' + exe + '" "' + OBJ + '\*.obj" "' + OUT + '\port_gen.obj" >"' + OUT + '\link.log" 2>&1',
            "@if errorlevel 1 exit /b 3",
            "@exit /b 0"]) + "\r\n")
    objs = glob.glob(os.path.join(OBJ, "*.obj"))
    inputs = objs + [os.path.join(ROOT, "tools", "port_link.py")] + glob.glob(os.path.join(ROOT, "config", "RM7P4F", "*.txt"))
    if (not n_compile and os.path.exists(exe) and os.path.exists(os.path.join(OUT, "port_gen.obj"))
            and os.path.getmtime(exe) > max(os.path.getmtime(f) for f in inputs)):
        print("up to date", exe)
        return finish(exe, run_args)
    r = subprocess.run(["cmd", "/c", bat])
    if r.returncode:
        log = {1: "compile.log", 3: "link.log"}.get(r.returncode)
        if log:
            lines = [l for l in open(os.path.join(OUT, log), errors="replace").read().splitlines() if "error" in l]
            print("\n".join(lines[:40]))
            print(f"{len(lines)} error lines: build/pc/{log}")
        sys.exit(r.returncode)
    json.dump({"sig": sig.hexdigest()}, open(stamp, "w"))
    print("built", exe)
    finish(exe, run_args)


def finish(exe, run_args):
    if run_args is not None:
        sys.exit(subprocess.run([exe] + run_args, cwd=ROOT).returncode)


if __name__ == "__main__":
    main()
