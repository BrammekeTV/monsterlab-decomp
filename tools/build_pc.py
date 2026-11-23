"""Build the PC executable locally with MSVC (CI builds the same through CMake).

    python tools/build_pc.py [--debug] [--run [args...]]

Compiles every source (32-bit, the flags of tools/check.py), generates the link glue with
tools/port_link.py and links build/pc/monsterlab.exe. See docs/port_runtime.md.
  --debug   keep debug information (/Z7, /DEBUG)
  --noopt   build the PC layer without optimisation too (/Od everywhere)
  --run     run the exe afterwards; the remaining arguments are passed to it

The PC layer (src/port/: renderer, GX interpreter, audio mixer, loader) is built with /O2. The
decompiled game code stays at /Od for now: with /O2 everywhere the boot crashes in fn_800DC070,
so some of it still misbehaves when optimised. Optimising the PC layer alone takes a headless
boot from about 34 to about 46 frames per second.
"""
import glob, os, subprocess, sys
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
    for old in glob.glob(os.path.join(OBJ, "*.obj")):
        os.remove(old)
    port_dir = os.path.join(ROOT, "src", "port") + os.sep
    port_opt = " /Od" if "--noopt" in argv else " /O2 /DNDEBUG"
    groups = [([p for p in files if not p.startswith(port_dir)], " /Od"),
              ([p for p in files if p.startswith(port_dir)], port_opt)]
    compile_lines = ['@type nul >"' + OUT + '\\compile.log"']
    for n, (group, opt) in enumerate(groups):
        if not group:
            continue
        rsp = os.path.join(OUT, f"files{n}.rsp")
        with open(rsp, "w") as f:
            f.write("\n".join('"' + p + '"' for p in group))
        compile_lines += ["@cl /MP " + " ".join(check.FLAGS) + opt + (" /Z7" if debug else "") + ' /Fo"' + OBJ + '\\\\" @"' + rsp + '" >>"' + OUT + '\\compile.log" 2>&1',
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
    r = subprocess.run(["cmd", "/c", bat])
    if r.returncode:
        log = {1: "compile.log", 3: "link.log"}.get(r.returncode)
        if log:
            lines = [l for l in open(os.path.join(OUT, log), errors="replace").read().splitlines() if "error" in l]
            print("\n".join(lines[:40]))
            print(f"{len(lines)} error lines: build/pc/{log}")
        sys.exit(r.returncode)
    print("built", exe)
    if run_args is not None:
        sys.exit(subprocess.run([exe] + run_args, cwd=ROOT).returncode)


if __name__ == "__main__":
    main()
