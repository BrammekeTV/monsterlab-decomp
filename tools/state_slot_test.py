#!/usr/bin/env python3
"""Headless test of the player's savestate slots (F5/F8 path, docs/savestates.md): `--state-key` presses
save at one frame and load at a later one; the load starts a new process, which plays on to the frame limit.
Then the slot's file is made to look like another build's, and loading it must be refused while the game goes on.

    python tools/state_slot_test.py [--iso PATH]        (after python tools/build_pc.py)
"""
import argparse, os, shutil, struct, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, "build", "pc", "monsterlab.exe")
OUT = os.path.join(ROOT, "build", "state_slot_test")


def run(name, extra, iso, fast=True):
    env = dict(os.environ, MONSTERLAB_STATES=os.path.join(OUT, "slots"), MONSTERLAB_SAVE=os.path.join(OUT, "save"))
    cmd = [EXE, "--to-menu", "--no-audio", "--no-movies", "--no-ini", "--no-mods", "--res", "640x360",
           "--seconds", "300", "--screenshot", os.path.join(OUT, name + ".png"), "--screenshot-stats"] + extra
    if fast:                            # without it real time starts at frame 3300, as in the player's window
        cmd += ["--fast"]
    if iso:
        cmd += ["--iso", iso]
    r = subprocess.run(cmd, cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=600)
    log = r.stdout.decode("utf-8", "replace")
    with open(os.path.join(OUT, name + ".log"), "w", encoding="utf-8") as f:
        f.write(log)
    return r.returncode, log


def expect(fails, name, code, log, wanted):
    if code != 0:
        fails.append(f"{name}: exit code {code}")
    at = 0
    for text in wanted:
        found = log.find(text, at)
        if found < 0:
            fails.append(f"{name}: no '{text}' (in this order) in {os.path.join(OUT, name + '.log')}")
            return
        at = found + len(text)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--iso")
    args = ap.parse_args()
    shutil.rmtree(OUT, ignore_errors=True)
    os.makedirs(os.path.join(OUT, "save"))
    fails = []
    slot = os.path.join(OUT, "slots", "slot2.mlst")

    code, log = run("roundtrip", ["--frames", "3450", "--state-key", "next@3150", "--state-key", "save@3200",
                                  "--state-key", "load@3300"], args.iso)
    expect(fails, "roundtrip", code, log, ["State 2 saved (frame 3200)", "Loading state 2... (frame 3300)",
                                           "state: loaded frame 3200", "State 2 loaded (frame 3201)",
                                           "frame limit reached (3450 frames)"])
    if log.count("Loading state") != 1:
        fails.append(f"roundtrip: the load ran {log.count('Loading state')} times")
    if not os.path.exists(slot) or not os.path.exists(slot[:-4] + "thumb"):
        fails.append("roundtrip: no slot file or picture")

    code, log = run("paced", ["--frames", "3650", "--state-key", "save@3400", "--state-key", "load@3500"], args.iso, fast=False)
    expect(fails, "paced", code, log, ["State 1 saved", "state: loaded frame", "State 1 loaded", "frame limit reached (3650 frames)"])

    if os.path.exists(slot):
        with open(slot, "r+b") as f:                    # the header's build hash: magic, version, buildHash
            f.seek(8)
            (build,) = struct.unpack("<I", f.read(4))
            f.seek(8)
            f.write(struct.pack("<I", build ^ 1))
        code, log = run("otherbuild", ["--frames", "3300", "--state-key", "next@3150", "--state-key", "load@3200"], args.iso)
        expect(fails, "otherbuild", code, log, ["Incompatible state (different build) (frame 3200)",
                                                "frame limit reached (3300 frames)"])

    if fails:
        print("STATE SLOT TEST FAIL\n  " + "\n  ".join(fails))
        return 1
    print(f"STATE SLOT TEST PASS: saved, loaded in a new process, other build refused ({OUT})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
