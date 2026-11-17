"""Compute decompilation progress and write docs/data/progress.json.

Only needs files that are in the repo (symbols.txt + src/), so it also runs in CI.

    python tools/progress.py            # write docs/data/progress.json
    python tools/progress.py --record   # also append today's numbers to docs/data/history.json

A function counts as decompiled when a source file under src/ has a comment
`// 0xADDRESS` naming its start address.
"""
import datetime
import glob
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SYMS = os.path.join(ROOT, "config", "RM7P4F", "symbols.txt")
OUT_DIR = os.path.join(ROOT, "docs", "data")

GAME_START, GAME_END = 0x80006B40, 0x8022FEC4   # game code; zlib, SDK/MSL/TRK after GAME_END
TEXT_END = 0x80345AE0

# Coarse map of the library half of .text, from identified SDK symbols.
LIB_REGIONS = [
    (0x8022FEC4, 0x80232AF8, "zlib 1.2.3"),
    (0x80232AF8, 0x80241000, "OS"), (0x80241000, 0x80243400, "EXI/SI"),
    (0x80243400, 0x80244000, "DB"), (0x80244000, 0x80253000, "GX"),
    (0x80253000, 0x80257700, "DVD"), (0x80257700, 0x80264000, "AI/AX/MEM"),
    (0x80264000, 0x8026E000, "NAND/SC/IPC"), (0x8026E000, 0x802A4000, "WPAD/KPAD/BT"),
    (0x802A4000, 0x80323600, "Network (DWC/SSL/NHTTP/NWC24)"),
    (0x80323600, 0x8033F400, "MSL C/C++ runtime"), (0x8033F400, TEXT_END, "MetroTRK"),
]

SYM_RE = re.compile(r"^(\S+) = (\.\w+):0x([0-9A-Fa-f]+); // type:function size:0x([0-9A-Fa-f]+)")
DECOMP_RE = re.compile(r"^\s*//\s*0x(80[0-9A-Fa-f]{6})\s*$")


def load_functions():
    funcs = []
    for line in open(SYMS):
        m = SYM_RE.match(line)
        if m and m.group(2) in (".text", ".init"):
            funcs.append({
                "name": m.group(1), "addr": int(m.group(3), 16), "size": int(m.group(4), 16),
            })
    funcs.sort(key=lambda f: f["addr"])
    return funcs


def load_decompiled():
    found = {}
    for path in glob.glob(os.path.join(ROOT, "src", "**", "*.c*"), recursive=True):
        rel = os.path.relpath(path, ROOT).replace("\\", "/")
        lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
        for i, line in enumerate(lines):
            m = DECOMP_RE.match(line)
            if not m:
                continue
            # Must be followed by a definition, not a forward declaration.
            nxt = next((l.strip() for l in lines[i + 1:] if l.strip()), "")
            if nxt.endswith(";"):
                continue
            found.setdefault(int(m.group(1), 16), rel)
    return found


def is_named(name):
    return not (name.startswith("fn_") or name.startswith("gap_"))


def git_info():
    try:
        sha = subprocess.check_output(["git", "rev-parse", "--short", "HEAD"], cwd=ROOT, text=True).strip()
        date = subprocess.check_output(["git", "log", "-1", "--format=%cI"], cwd=ROOT, text=True).strip()
        return sha, date
    except Exception:
        return None, None


def main():
    funcs = load_functions()
    decomp = load_decompiled()

    def summarize(fs):
        return {
            "functions": len(fs),
            "bytes": sum(f["size"] for f in fs),
            "named": sum(1 for f in fs if is_named(f["name"])),
            "named_bytes": sum(f["size"] for f in fs if is_named(f["name"])),
            "decompiled": sum(1 for f in fs if f["addr"] in decomp),
            "decompiled_bytes": sum(f["size"] for f in fs if f["addr"] in decomp),
        }

    game = [f for f in funcs if GAME_START <= f["addr"] < GAME_END]
    lib = [f for f in funcs if f["addr"] >= GAME_END or f["addr"] < GAME_START]

    # Memory map: fixed-size buckets of the game region with status ratios.
    buckets, n = [], 256
    step = (GAME_END - GAME_START) / n
    for i in range(n):
        lo, hi = GAME_START + i * step, GAME_START + (i + 1) * step
        fs = [f for f in game if lo <= f["addr"] < hi]
        total = sum(f["size"] for f in fs) or 1
        buckets.append([
            round(sum(f["size"] for f in fs if f["addr"] in decomp) / total, 3),
            round(sum(f["size"] for f in fs if is_named(f["name"])) / total, 3),
        ])

    regions = []
    for lo, hi, name in LIB_REGIONS:
        fs = [f for f in lib if lo <= f["addr"] < hi]
        regions.append({"name": name, "start": f"{lo:08X}", "end": f"{hi:08X}", **summarize(fs)})

    files = {}
    for f in game + lib:
        if f["addr"] in decomp:
            files.setdefault(decomp[f["addr"]], []).append(
                {"name": f["name"], "addr": f"{f['addr']:08X}", "size": f["size"]})

    sha, date = git_info()
    out = {
        "game_id": "RM7P4F",
        "generated": datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds"),
        "commit": sha, "commit_date": date,
        "game": summarize(game), "library": summarize(lib), "all": summarize(funcs),
        "game_range": [f"{GAME_START:08X}", f"{GAME_END:08X}"],
        "memory_map": buckets, "library_regions": regions,
        "files": [{"path": k, "functions": v} for k, v in sorted(files.items())],
    }
    os.makedirs(OUT_DIR, exist_ok=True)
    with open(os.path.join(OUT_DIR, "progress.json"), "w", newline="\n") as fh:
        json.dump(out, fh, indent=1)

    if "--record" in sys.argv:
        hp = os.path.join(OUT_DIR, "history.json")
        hist = json.load(open(hp)) if os.path.exists(hp) else []
        today = datetime.date.today().isoformat()
        entry = {"date": today, "game": out["game"], "library": out["library"]}
        hist = [h for h in hist if h["date"] != today] + [entry]
        with open(hp, "w", newline="\n") as fh:
            json.dump(hist, fh, indent=1)

    g = out["game"]
    print(f"game: {g['decompiled']}/{g['functions']} functions decompiled "
          f"({100 * g['decompiled_bytes'] / g['bytes']:.2f}% of bytes), {g['named']} named")
    l = out["library"]
    print(f"library: {l['named']}/{l['functions']} identified")


if __name__ == "__main__":
    main()
