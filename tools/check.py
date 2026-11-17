"""Compile source files locally with MSVC (syntax/type check only, like the CI build).

    python tools/check.py [file.cpp ...]     (default: files changed since the last commit)

Needs Visual Studio Build Tools with the C++ workload. Uses the same flags as CMakeLists.txt.
"""
import glob, os, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VSWHERE = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
FLAGS = ["/nologo", "/c", "/std:c++17", "/W3", "/permissive-", "/utf-8",
         "/wd4244", "/wd4267", "/wd4305", "/D_CRT_SECURE_NO_WARNINGS", "/I", '"' + os.path.join(ROOT, "include") + '"']


def vcvars():
    if not os.path.exists(VSWHERE):
        sys.exit("vswhere.exe not found: install Visual Studio Build Tools (C++ workload)")
    path = subprocess.run([VSWHERE, "-latest", "-products", "*", "-requires",
                           "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
                          capture_output=True, text=True).stdout.strip()
    bat = os.path.join(path, "VC", "Auxiliary", "Build", "vcvars64.bat")
    if not path or not os.path.exists(bat):
        sys.exit("MSVC C++ tools not found (install the 'Desktop development with C++' workload)")
    return bat


def changed():
    out = subprocess.run(["git", "status", "--porcelain"], cwd=ROOT, capture_output=True, text=True).stdout
    paths = [l[3:].strip().strip('"') for l in out.splitlines()]
    if any(p.endswith(".h") for p in paths):   # a header changed: check everything
        return sorted(glob.glob(os.path.join(ROOT, "src", "**", "*.cpp"), recursive=True))
    return [os.path.join(ROOT, p) for p in paths if p.endswith((".cpp", ".c")) and os.path.exists(os.path.join(ROOT, p))]


def main():
    files = [os.path.abspath(f) for f in sys.argv[1:]] or changed()
    if not files:
        print("nothing to check")
        return 0
    bat = vcvars()
    with tempfile.TemporaryDirectory() as out:
        rsp = os.path.join(out, "files.rsp")
        with open(rsp, "w") as f:
            f.write("\n".join('"' + p + '"' for p in files))
        script = os.path.join(out, "run.bat")
        lines = ['@call "' + bat + '" >nul',
                 "@cl " + " ".join(FLAGS) + ' /Fo"' + out + '\\\\" @"' + rsp + '"']
        with open(script, "w") as f:
            f.write("\r\n".join(lines) + "\r\n")
        r = subprocess.run(["cmd", "/c", script], capture_output=True, text=True)
    out_lines = r.stdout.splitlines() + r.stderr.splitlines()
    errors = [l for l in out_lines if " error " in l or "fatal error" in l]
    for l in errors[:40]:
        print(l)
    if r.returncode != 0 and not errors:
        print("\n".join(out_lines[-20:]))
    print(f"{len(files)} files, {len(errors)} errors")
    return 1 if errors or r.returncode != 0 else 0


if __name__ == "__main__":
    sys.exit(main())
