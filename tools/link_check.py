"""Compile every source with MSVC (32-bit) and link all objects; the linker output is the
missing-symbols list.  python tools/link_check.py [--keep] ; log: build/link/link.log
Then: python tools/missing_symbols.py build/link/link.log   (CI does the same via CMake)."""
import glob, os, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import check

ROOT = check.ROOT
OUT = os.path.join(ROOT, "build", "link")


def main():
    os.makedirs(OUT, exist_ok=True)
    files = sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True))
    rsp = os.path.join(OUT, "files.rsp")
    with open(rsp, "w") as f:
        f.write("\n".join('"' + p + '"' for p in files))
    bat = os.path.join(OUT, "run.bat")
    with open(bat, "w") as f:
        f.write("\r\n".join([
            '@call "' + check.vcvars() + '" >nul',
            "@cl /MP " + " ".join(check.FLAGS) + ' /Fo"' + OUT + '\\\\" @"' + rsp + '" >"' + OUT + '\compile.log" 2>&1',
            "@if errorlevel 1 exit /b 1",
            '@link /NOLOGO /OUT:"' + OUT + '\monsterlab_link.exe" "' + OUT + '\*.obj" >"' + OUT + '\link.log" 2>&1',
            "@exit /b 0"]) + "\r\n")
    for old in glob.glob(os.path.join(OUT, "*.obj")):
        os.remove(old)
    r = subprocess.run(["cmd", "/c", bat])
    if r.returncode:
        sys.exit("compile failed: see build/link/compile.log")
    print("link log:", os.path.join(OUT, "link.log"))


if __name__ == "__main__":
    main()
