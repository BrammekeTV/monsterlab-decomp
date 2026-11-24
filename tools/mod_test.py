"""Texture-mod test for the PC port: headless, in a temporary folder outside the repository.

    python tools/mod_test.py [--iso PATH] [--keep]

1. Runs to the main menu with --dump-textures and takes a screenshot (no mods).
2. Makes a mod there with one dumped texture tinted red (the 128x128 texture that changes the most
   of the picture is not known in advance, so it tries the largest few until the picture changes).
3. Runs again with that mod and compares the screenshots.
Passes when the log names the replaced texture and the picture got redder. Needs Pillow and the game
data (orig/RM7P4F or --iso). Nothing is written into the repository; --keep leaves the temporary
folder (dump, mod, before.png, after.png) and prints where it is.
"""
import os, shutil, subprocess, sys, tempfile
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, "build", "pc", "monsterlab.exe")


def run(args, log):
    p = subprocess.run([EXE, "--to-menu", "--frames", "3300", "--no-audio", "--no-ini", "--res", "640x480", "--seconds", "90", "--verbose"] + args,
                       cwd=ROOT, capture_output=True, text=True, errors="replace")
    out = p.stdout + p.stderr
    open(log, "w", errors="replace").write(out)
    return p.returncode == 0 and "frame limit reached (3300 frames)" in out, out


def redness(im):
    """Sum over the picture of how far red is above green and blue."""
    r, g, b = im.convert("RGB").split()
    hr, hg, hb = r.histogram(), g.histogram(), b.histogram()
    s = lambda h: sum(i * n for i, n in enumerate(h))
    return s(hr) - (s(hg) + s(hb)) / 2.0


def main():
    from PIL import Image
    argv = sys.argv[1:]
    data = ["--iso", argv[argv.index("--iso") + 1]] if "--iso" in argv else []
    tmp = tempfile.mkdtemp(prefix="monsterlab_modtest_")
    dump, before, after = os.path.join(tmp, "dump"), os.path.join(tmp, "before.png"), os.path.join(tmp, "after.png")
    ok, verdict = False, ""
    try:
        good, _ = run(["--no-mods", "--dump-textures", dump, "--screenshot", before] + data, os.path.join(tmp, "dump.log"))
        names = sorted(os.listdir(dump), key=lambda n: -os.path.getsize(os.path.join(dump, n))) if os.path.isdir(dump) else []
        if not good or not names or not os.path.exists(before):
            verdict = f"the dump run failed ({len(names)} textures)"
            return 1
        base = redness(Image.open(before))
        pixels = Image.open(before).size[0] * Image.open(before).size[1]
        for n in names[:12]:
            mods = os.path.join(tmp, "mods")
            shutil.rmtree(mods, ignore_errors=True)
            os.makedirs(os.path.join(mods, "RedTint", "textures"))
            im = Image.open(os.path.join(dump, n)).convert("RGBA")
            red = Image.new("RGBA", im.size, (255, 0, 0, 255))
            red.putalpha(im.getchannel("A"))
            Image.blend(im, red, 0.8).save(os.path.join(mods, "RedTint", "textures", "red_" + n[-20:]))   # compressed PNG, named by the hash only
            good, out = run(["--mods", mods, "--screenshot", after] + data, os.path.join(tmp, "mod.log"))
            h = n[-20:-4]
            if not good or f"mods: texture {h} comes from RedTint" not in out:
                verdict = f"{n}: the run failed or the texture was not replaced"
                return 1
            gain = (redness(Image.open(after)) - base) / pixels
            if gain > 1.0:                                   # on average more than one level redder per pixel
                ok, verdict = True, f"{n} replaced, picture redder by {gain:.1f} per pixel ({len(names)} textures dumped)"
                return 0
        verdict = "textures were replaced but the picture did not get redder"
        return 1
    finally:
        print(("MODTEST OK: " if ok else "MODTEST FAIL: ") + verdict + (f"  [{tmp}]" if "--keep" in argv or not ok else ""))
        if ok and "--keep" not in argv:
            shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
