"""Exactness test of the PC port's savestates (docs/savestates.md): run it after changing src/port/port_state.cpp,
the scheduler in port_runtime.cpp, or what a state holds.

    python tools/savestate_test.py [--at 3200,5200,7500,10000] [--more 200] [--iso PATH] [--no-build] [--keep]

One straight run plays the playthrough's input (tools/playthrough.py) from boot to the last point + MORE frames;
it saves a state at every point N and a screenshot at every N + MORE. Then, for every point, a new process loads
the state of N and runs to N + MORE. Compared for each point:
  - every [state] and [script] log line of the frames between (game state: script calls, game mode, battle, map node);
  - the screenshot at N + MORE, byte for byte.
A loaded run must match the straight run in both. Unpaced runs repeat exactly, so there is no noise to allow for.
Prints one line per point and SAVESTATE OK / FAIL; exits 0/1. Files: build/savestate_test/ (removed on success
unless --keep). Needs the game data (orig/RM7P4F, MONSTERLAB_DATA or --iso). The default points are the main
menu, the village map, the walk into the first enemy (the arena level loads right after the state) and the middle of
the first battle; the castle after the village is --at 29000 (a run of
the whole playthrough: about 25 minutes with the renderer on).
"""
import argparse, os, re, shutil, struct, subprocess, sys, zlib
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import playthrough

ROOT = playthrough.ROOT
OUT = os.path.join(ROOT, "build", "savestate_test")
EXE = os.path.join(OUT, "ml_savestate.exe")
FRAME = playthrough.FRAME


def png_pixels(path):
    """-> (width, height, raw bytes) of an 8-bit RGB or RGBA PNG."""
    with open(path, "rb") as f:
        d = f.read()
    pos, idat, w, h, ct = 8, b"", 0, 0, 2
    while pos < len(d):
        n, t = struct.unpack(">I4s", d[pos:pos + 8])
        body = d[pos + 8:pos + 8 + n]
        pos += 12 + n
        if t == b"IHDR":
            w, h, _, ct = struct.unpack(">IIBB", body[:10])
        elif t == b"IDAT":
            idat += body
    raw, bpp = zlib.decompress(idat), {2: 3, 6: 4}[ct]
    stride, rows, prev, p = w * bpp, [], bytearray(w * bpp), 0
    for _ in range(h):
        f, line = raw[p], bytearray(raw[p + 1:p + 1 + stride])
        p += 1 + stride
        if f == 1:
            for i in range(bpp, stride):
                line[i] = (line[i] + line[i - bpp]) & 255
        elif f == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 255
        elif f == 3:
            for i in range(stride):
                line[i] = (line[i] + (((line[i - bpp] if i >= bpp else 0) + prev[i]) >> 1)) & 255
        elif f == 4:
            for i in range(stride):
                a, b, c = (line[i - bpp] if i >= bpp else 0), prev[i], (prev[i - bpp] if i >= bpp else 0)
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        rows.append(line)
        prev = line
    return w, h, b"".join(rows)


def log_after(path, first, last):
    """The [state] and [script] lines of frames first+1 .. last-1. (Lines of frame `last` are what the game does after
    that frame was shown: the loaded run stops there, the straight run goes on.)"""
    out, held = [], []
    with open(path, encoding="utf-8", errors="replace") as f:
        for s in f:
            if not s.startswith(("[state]", "[script]")):
                continue
            m = FRAME.match(s)
            if not m:                       # "(xN more)": printed when the next call comes, so it goes with that one
                held.append(s.rstrip())
                continue
            if first < int(m.group(1) or m.group(2)) < last:
                out += held + [s.rstrip()]
            held = []
    return out


def game(tag, frames, extra, iso, seconds):
    cmd = [EXE, "--fast", "--no-audio", "--no-movies", "--no-ini", "--no-mods", "--press", "A@2600-2610",
           "--res", f"{playthrough.WIDTH}x{playthrough.HEIGHT}", "--frames", str(frames), "--seconds", str(seconds),
           "--screenshot", os.path.join(OUT, tag + ".png")] + extra
    for st in playthrough.STAGES:
        for p in st.press:
            cmd += ["--press", p]
        for p in st.pointer:
            xy, at = p.split("@")
            x, y = (int(v) for v in xy.split(","))
            cmd += ["--pointer", f"{round(x * playthrough.WIDTH / 1920)},{round(y * playthrough.HEIGHT / 1080)}@{at}"]
    if iso:
        cmd += ["--iso", iso]
    env = {k: v for k, v in os.environ.items() if not k.startswith("PORT_")}
    env.update(PORT_SCRIPT_TRACE="1", PORT_STATE_LOG="1", MONSTERLAB_SAVE=os.path.join(OUT, "save_" + tag))
    shutil.rmtree(env["MONSTERLAB_SAVE"], ignore_errors=True)
    log = os.path.join(OUT, tag + ".log")
    with open(log, "w", encoding="utf-8", errors="replace") as f:
        code = subprocess.run(cmd, cwd=ROOT, env=env, stdout=f, stderr=subprocess.STDOUT).returncode
    return code, log


def main():
    ap = argparse.ArgumentParser(description="exactness test of the PC port's savestates")
    ap.add_argument("--at", default="3200,5200,7500,10000", help="frames to save and load at (default: menu, village map, just before the arena loads, first battle)")
    ap.add_argument("--more", type=int, default=200, help="frames to run after each point (default 200)")
    ap.add_argument("--iso", help="disc image or extracted disc (default: orig/RM7P4F or MONSTERLAB_DATA)")
    ap.add_argument("--no-build", action="store_true")
    ap.add_argument("--keep", action="store_true", help="keep build/savestate_test after a pass")
    a = ap.parse_args()
    points = sorted(int(v) for v in a.at.split(","))
    if not a.no_build and not playthrough.build():
        return 1
    shutil.rmtree(OUT, ignore_errors=True)
    os.makedirs(OUT)
    shutil.copy2(os.path.join(playthrough.PC, "monsterlab.exe"), EXE)
    shutil.copy2(os.path.join(playthrough.PC, "monsterlab.map"), os.path.splitext(EXE)[0] + ".map")
    extra = ["--shot-at", ",".join(str(n + a.more) for n in points)]
    for n in points:
        extra += ["--save-state", os.path.join(OUT, f"f{n}.mlst") + f"@{n}"]
    code, straight = game("straight", points[-1] + a.more, extra, a.iso, 3000)
    if code:
        print(f"SAVESTATE FAIL: the straight run ended with exit code {code} ({os.path.relpath(straight, ROOT)})")
        return 1
    fails = 0
    for n in points:
        end = n + a.more
        state = os.path.join(OUT, f"f{n}.mlst")
        if not os.path.exists(state):
            print(f"  FAIL  frame {n:>6}: no state was written (see {os.path.relpath(straight, ROOT)})")
            fails += 1
            continue
        code, loaded = game(f"load{n}", end, ["--load-state", state], a.iso, 600)
        want, got = log_after(straight, n, end), log_after(loaded, n, end)
        notes = []
        if code:
            notes.append(f"the loaded run ended with exit code {code}")
        if want != got:
            at = next((i for i, (x, y) in enumerate(zip(want, got)) if x != y), min(len(want), len(got)))
            notes.append(f"log differs at line {at} of {len(want)}/{len(got)}: '{(want[at] if at < len(want) else '(end)')[:90]}' "
                         f"against '{(got[at] if at < len(got) else '(end)')[:90]}'")
        try:
            a_px, b_px = png_pixels(os.path.join(OUT, f"straight_{end}.png")), png_pixels(os.path.join(OUT, f"load{n}.png"))
            if a_px[:2] != b_px[:2]:
                notes.append("pictures differ in size")
            else:
                differ = sum(1 for x, y in zip(a_px[2], b_px[2]) if x != y)
                if differ:
                    worst = max(abs(x - y) for x, y in zip(a_px[2], b_px[2]))
                    notes.append(f"picture: {differ} of {len(a_px[2])} bytes differ, by up to {worst}")
        except (OSError, KeyError, zlib.error) as e:
            notes.append(f"no picture to compare ({e})")
        size = os.path.getsize(state) / 1048576
        if notes:
            fails += 1
            print(f"  FAIL  frame {n:>6} -> {end}: " + "; ".join(notes))
        else:
            print(f"  PASS  frame {n:>6} -> {end}: {len(want)} log lines and the picture identical (state {size:.1f} MB)")
    if fails:
        print(f"SAVESTATE FAIL: {fails} of {len(points)} point(s); files in {os.path.relpath(OUT, ROOT)}")
        return 1
    print(f"SAVESTATE OK: {len(points)} point(s), {a.more} frames after each")
    if not a.keep:
        shutil.rmtree(OUT, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
