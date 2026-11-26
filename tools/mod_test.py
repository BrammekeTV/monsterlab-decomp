"""Mod test for the PC port: headless, in a temporary folder outside the repository.

    python tools/mod_test.py [--iso PATH] [--keep]

1. Runs to the main menu with --dump-textures and takes a screenshot (no mods).
2. Makes a mod there with one dumped texture tinted red (the 128x128 texture that changes the most
   of the picture is not known in advance, so it tries the largest few until the picture changes).
   The same tinted texture is then converted with tools/png2dds.py (BC7, BC3, BC1) and must redden the menu as well.
3. Runs again with that mod and compares the screenshots; then the same texture by its name, and a data mod
   (--dump-data, a strings.json that changes menu text, a broken one).
Passes when the log names the replaced texture and the picture got redder. Needs Pillow and the game
data (orig/RM7P4F or --iso). Nothing is written into the repository; --keep leaves the temporary
folder (dump, mod, before.png, after.png) and prints where it is.
"""
import json, os, re, shutil, subprocess, sys, tempfile
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


def by_name(tmp, dump, names, data):
    """A texture whose name the dump gives (tex_WxH_fF_<name>_<hash>.png) is replaced by <name>.png. Returns an error text or ''."""
    from PIL import Image
    pat = re.compile(r"^tex_\d+x\d+_f\d+_(.+)_[0-9A-F]{16}\.png$")
    named = [(n, pat.match(n).group(1)) for n in names if pat.match(n)]
    if not named:
        return "the dump has no texture with a name"
    n, name = named[0]
    mods = os.path.join(tmp, "mods2")
    shutil.rmtree(mods, ignore_errors=True)
    os.makedirs(os.path.join(mods, "ByName", "textures", "sub"))
    Image.open(os.path.join(dump, n)).convert("RGBA").save(os.path.join(mods, "ByName", "textures", "sub", name + ".png"))
    good, out = run(["--mods", mods, "--screenshot", os.path.join(tmp, "byname.png")] + data, os.path.join(tmp, "byname.log"))
    if not good or f"comes from ByName by name" not in out or f"texture {name} (" not in out:
        return f"{name}: the texture was not replaced by its name"
    return ""


def dds_test(tmp, tinted, base, pixels, png_gain, data):
    """The red picture as DDS (tools/png2dds.py: automatic, BC7, BC3, BC1) holds the same colours and reddens the menu as the PNG did.
    Returns an error text or ''."""
    from PIL import Image, ImageChops, ImageStat
    opaque = Image.open(tinted).convert("RGBA").getchannel("A").getextrema()[0] == 255
    mods = os.path.join(tmp, "mods4")
    for fmt in ["auto", "bc7", "bc3"] + (["bc1"] if opaque else []):
        shutil.rmtree(mods, ignore_errors=True)
        folder = os.path.join(mods, "Dds", "textures")
        os.makedirs(folder)
        shutil.copy(tinted, folder)
        p = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "png2dds.py"), folder, "--format", fmt, "--delete-png"],
                           capture_output=True, text=True, errors="replace")
        if p.returncode or os.listdir(folder) != [os.path.basename(tinted)[:-4] + ".dds"]:
            return f"png2dds --format {fmt} failed: {p.stdout}{p.stderr}"
        decoded = Image.open(os.path.join(folder, os.listdir(folder)[0])).convert("RGBA")     # Pillow's own decoder
        off = max(ImageStat.Stat(ImageChops.difference(decoded, Image.open(tinted).convert("RGBA"))).mean)
        if off > 4.0:
            return f"png2dds --format {fmt}: the DDS differs from the PNG by {off:.1f} per pixel"
        shot = os.path.join(tmp, f"dds_{fmt}.png")
        good, out = run(["--mods", mods, "--screenshot", shot] + data, os.path.join(tmp, f"dds_{fmt}.log"))
        want = {"auto": "BC1" if opaque else "BC7"}.get(fmt, fmt.upper())
        if not good or f"comes from Dds (" not in out or f", {want}, " not in out:
            return f"DDS {fmt}: the texture was not replaced"
        gain = (redness(Image.open(shot)) - base) / pixels     # the menu's rain and lightning differ from run to run
        if gain < png_gain / 2:
            return f"DDS {fmt}: the picture is only {gain:.1f} per pixel redder (PNG: {png_gain:.1f})"
    return ""


def data_test(tmp, base_png, data):
    """--dump-data writes the text table; a mod's data/strings.json changes menu text; a broken file is reported and ignored.
    Returns an error text or ''."""
    from PIL import Image, ImageChops
    ddir = os.path.join(tmp, "data")
    p = subprocess.run([EXE, "--no-audio", "--no-ini", "--dump-data", ddir] + data, cwd=ROOT, capture_output=True, text=True, errors="replace")
    path = os.path.join(ddir, "strings.json")
    if not os.path.exists(path):
        return "--dump-data wrote no strings.json"
    table = json.load(open(path, encoding="utf-8"))
    strings = table["strings"]
    if table.get("count") != len(strings) or len(strings) < 1000:
        return f"the dumped text table looks wrong ({len(strings)} strings)"
    ids = [k for k, v in strings.items() if v in ("New Game", "Continue", "Options")]
    if len(ids) < 2:
        return "the menu strings were not found in the dump"
    mods = os.path.join(tmp, "mods3")
    shutil.rmtree(mods, ignore_errors=True)
    os.makedirs(os.path.join(mods, "Words", "data"))
    mod = os.path.join(mods, "Words", "data", "strings.json")
    json.dump({"format": 1, "strings": {k: "Zork " + strings[k] for k in ids}}, open(mod, "w"))
    shot = os.path.join(tmp, "words.png")
    good, out = run(["--mods", mods, "--screenshot", shot] + data, os.path.join(tmp, "words.log"))
    if not good or f"mods: Words changes {len(ids)} strings" not in out:
        return "the run failed or the strings were not changed"
    if ImageChops.difference(Image.open(shot).convert("RGB"), Image.open(base_png).convert("RGB")).getbbox() is None:
        return "the strings were changed but the menu picture did not"
    open(mod, "w").write('{"strings": {"99999": "x"}}')           # a mistake: reported with its place, the file ignored
    good, out = run(["--mods", mods] + data, os.path.join(tmp, "bad.log"))
    if not good or "line 1, column" not in out or "does not exist" not in out or "the file is ignored" not in out:
        return "a broken strings.json was not reported with its place"
    return ""


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
                bad = dds_test(tmp, os.path.join(mods, "RedTint", "textures", "red_" + n[-20:]), base, pixels, gain, data)
                if bad:
                    ok, verdict = False, bad
                    return 1
                verdict += "; dds ok"
                bad = by_name(tmp, dump, names, data)
                if bad:
                    ok, verdict = False, bad
                    return 1
                verdict += "; by name ok"
                bad = data_test(tmp, before, data)
                if bad:
                    ok, verdict = False, bad
                    return 1
                verdict += "; data ok"
                return 0
        verdict = "textures were replaced but the picture did not get redder"
        return 1
    finally:
        print(("MODTEST OK: " if ok else "MODTEST FAIL: ") + verdict + (f"  [{tmp}]" if "--keep" in argv or not ok else ""))
        if ok and "--keep" not in argv:
            shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
