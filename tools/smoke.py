"""Smoke test for the PC port: run it before opening a PR that touches the port or game code.

    python tools/smoke.py [--iso PATH] [--no-build]

Builds incrementally (tools/build_pc.py), then runs headless (no window, no sound):
  1. --selftest
  2. --to-menu to frame 3300, with a screenshot (the main menu) and the script trace, with the sample
     mod (tools/make_sample_mod.py) made afresh in build/smoke/mods
  3. --fast-forward 2600 to frame 2600, with a screenshot (the title)
Passes when each run stops at its frame limit with exit code 0, the menu scripts ran
(UIScreen_Push seen), the game took the sample mod's text (and, from an extracted
disc, its file in place of the disc's) and neither screenshot is blank or black. Prints one verdict line with
timings and exits 0/1. Needs the game in orig/RM7P4F (or --iso PATH); the self-test needs none.
"""
import os, shutil, subprocess, sys, time
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, "build", "pc", "monsterlab.exe")
OUT = os.path.join(ROOT, "build", "smoke")


def lit(path):
    """Fraction of pixels that are not near-black, or None if the PNG cannot be read."""
    try:
        from PIL import Image
        im = Image.open(path).convert("L")
        h = im.histogram()
        return sum(h[24:]) / float(sum(h))
    except ImportError:
        return 1.0 if os.path.getsize(path) > 20000 else 0.0   # no Pillow: a flat PNG compresses to almost nothing
    except Exception:
        return None


def main():
    argv = sys.argv[1:]
    iso = argv[argv.index("--iso") + 1] if "--iso" in argv else None
    os.makedirs(OUT, exist_ok=True)
    fails, times = [], {}
    t0 = time.time()
    if "--no-build" not in argv:
        r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "build_pc.py")], cwd=ROOT, capture_output=True, text=True)
        times["build"] = time.time() - t0
        if r.returncode:
            print("SMOKE FAIL: build failed\n" + (r.stdout + r.stderr)[-1500:])
            return 1
    t = time.time()
    r = subprocess.run([EXE, "--selftest"], cwd=ROOT, capture_output=True, text=True)
    times["selftest"] = time.time() - t
    if r.returncode or "ok" not in (r.stderr.strip().splitlines() or [""])[-1]:
        fails.append("selftest")
    data = ["--iso", iso] if iso else []
    common = ["--no-audio", "--no-ini", "--res", "640x480", "--seconds", "90"] + data
    menu_png, title_png = os.path.join(OUT, "menu.png"), os.path.join(OUT, "title.png")
    for p in (menu_png, title_png):
        if os.path.exists(p):
            os.remove(p)
    # Its own empty save folder: the sample mod's part is written to save.modparts there, not to anyone's saves.
    save = os.path.join(OUT, "save")
    shutil.rmtree(save, ignore_errors=True)
    env = dict(os.environ, PORT_SCRIPT_TRACE="1", MONSTERLAB_SAVE=save)
    # The sample mod, for the menu run. Its file override is a copy of a text file of the extracted disc.
    mods = os.path.join(OUT, "mods")
    shutil.rmtree(mods, ignore_errors=True)
    extracted = None if iso else os.environ.get("MONSTERLAB_DATA") or os.path.join(ROOT, "orig", "RM7P4F")
    if extracted and not os.path.isfile(os.path.join(extracted, "files", "text", "str_en.txt")):
        extracted = None                                     # a disc image: nothing to copy the file from
    r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "make_sample_mod.py"), "--out", mods]
                       + (["--data", extracted] if extracted else []), cwd=ROOT, capture_output=True, text=True)
    if r.returncode:
        fails.append("make_sample_mod.py")
    runs = {"menu": ([EXE, "--to-menu", "--frames", "3300", "--screenshot", menu_png, "--mods", mods] + common, env, 3300),
            "title": ([EXE, "--fast-forward", "2600", "--frames", "2600", "--screenshot", title_png, "--no-mods"] + common, None, 2600)}
    outs = {}
    for name, (cmd, e, frames) in runs.items():          # one after the other: in parallel the two renderers only slow each other down
        t = time.time()
        p = subprocess.run(cmd, cwd=ROOT, env=e, capture_output=True, text=True, errors="replace")
        outs[name] = p.stdout + p.stderr
        times[name] = time.time() - t
        if p.returncode:
            fails.append(f"{name}: exit code {p.returncode}")
        if f"frame limit reached ({frames} frames)" not in outs[name]:
            fails.append(f"{name}: frame limit not reached")
    if "UIScreen_Push" not in outs["menu"]:
        fails.append("menu scripts did not run (no UIScreen_Push)")
    if "mods: SampleMod changes 1 string" not in outs["menu"]:
        fails.append("the sample mod's strings.json was not applied")
    if "mods: part SampleMod:sample_head is row 186" not in outs["menu"]:
        fails.append("the sample mod's table patch did not add its part")
    if extracted and "[port] mod: SampleMod overrides TEXT/STR_EN.TXT" not in outs["menu"]:
        fails.append("the sample mod's file did not replace the disc's")
    for name, png in (("menu", menu_png), ("title", title_png)):
        f = lit(png) if os.path.exists(png) else None
        if f is None or f < 0.005:
            fails.append(f"{name} screenshot {'missing' if f is None else 'blank or black'}")
    total = time.time() - t0
    timing = ", ".join(f"{k} {v:.0f}s" for k, v in times.items())
    if fails:
        for n, o in outs.items():
            open(os.path.join(OUT, n + ".log"), "w", errors="replace").write(o)
        print(f"SMOKE FAIL ({total:.0f}s: {timing}): " + "; ".join(fails) + f"  [logs: build/smoke/*.log]")
        return 1
    what = "text, part and file override" if extracted else "text and part"
    print(f"SMOKE OK ({total:.0f}s: {timing}); mod {what} picked up")
    return 0


if __name__ == "__main__":
    sys.exit(main())
