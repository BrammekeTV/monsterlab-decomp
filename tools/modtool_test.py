"""Test of the mod tool (ml_modtool.exe, docs/concepts/mod-tools.md) with the player's own disc.

    python tools/modtool_test.py [--iso PATH] [--shot FILE.png]

Builds incrementally, then runs `ml_modtool.exe --modtool-test` (opens catalog.sir, global_binary.sir, a
level, shared_texture.sir and shared_parts.sir and checks textures, models and a texture export) and draws the
tool headless into a PNG (default: a temporary folder outside the repository; never commit it, it shows game
data). With numpy and Pillow it also builds the black hole head sample (tools/mods/blackhole) into a temporary
folder and checks the .part viewer on it: the part over its template, its lights, and the reload on a change.
"""
import os, shutil, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    args = sys.argv[1:]
    data = []
    if "--iso" in args:
        data = ["--iso", args[args.index("--iso") + 1]]
    shot = args[args.index("--shot") + 1] if "--shot" in args else os.path.join(tempfile.gettempdir(), "modtool_test.png")
    r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "build_pc.py")])
    if r.returncode:
        sys.exit("MODTOOL FAIL: build")
    exe = os.path.join(ROOT, "build", "pc", "ml_modtool.exe")
    # The black hole head sample (tools/mods/blackhole, needs numpy and Pillow), built outside the repository.
    part = []
    work = tempfile.mkdtemp(prefix="modtool_part_")
    bh = os.path.join(ROOT, "tools", "mods", "blackhole")
    if subprocess.run([sys.executable, os.path.join(bh, "make_assets.py"), "--out", os.path.join(work, "assets")], capture_output=True).returncode == 0 and \
       subprocess.run([sys.executable, os.path.join(bh, "build_mod.py"), "--assets", os.path.join(work, "assets"), "--out", os.path.join(work, "mods")],
                      capture_output=True).returncode == 0:
        part = ["--modtool-open", os.path.join(work, "mods", "BlackHoleHead", "parts", "p_hd_sha077.part")]
    else:
        print("black hole sample not built (numpy and Pillow?): the .part viewer is not tested")
    r = subprocess.run([exe, "--modtool-test"] + part + data, cwd=ROOT, capture_output=True, text=True, timeout=300)
    print("\n".join(l for l in r.stderr.splitlines() if l.startswith("[modtool]")))
    shutil.rmtree(work, ignore_errors=True)
    if r.returncode:
        sys.exit("MODTOOL FAIL: --modtool-test")
    r = subprocess.run([exe, "--modtool-open", "streams/global_binary.sir", "--modtool-select", "table 22", "--modtool-row", "5",
                        "--modtool-shot", shot] + data, cwd=ROOT, timeout=300)
    if r.returncode or not os.path.exists(shot) or os.path.getsize(shot) < 10000:
        sys.exit("MODTOOL FAIL: screenshot")
    print("MODTOOL OK; picture:", shot)


if __name__ == "__main__":
    main()
