"""Build and run the PC loader's test tool (tests/sir_loader_test.cpp, src/port/sir_loader.cpp).

    python tools/loader_check.py [path/to/global_binary.sir]

1. Compiles the test tool with MSVC (Win32, the CI flags) into build/loader_test/.
2. Runs its self-test: a synthetic big-endian file built in memory (CI runs the same).
3. If the real file is there (default: orig/RM7P4F/files/streams/global_binary.sir; also looked
   for in the main checkout when run from a git worktree), loads it with the C++ loader and
   compares every header slot, table descriptor and row, and the hand-typed image objects (input
   table, textures, string table, action table and its records, UI root and fonts, screen state
   machines, effect definitions, object pools), with
   the Python reference decoder
   (`tools/sir_decoder.py --gamedb --image`), which reads the big-endian bytes directly.
4. The same for ../catalog.sir (`--catalog`), plus a check that the catalog agrees with the
   section table inside each of the .sir files it lists.
5. The same for every level file next to it (`--level`): the `*_level.sir`, the arenas, the castle
   rooms and the trouble scope map.

Game data and the dumps stay local (orig/, build/); nothing here is committed.
"""
import struct
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import check            # vcvars(), FLAGS
import sir_decoder

REL = os.path.join("orig", "RM7P4F", "files", "streams", "global_binary.sir")
SAMPLES = ("part 0 ", "part 57 ", "part 185 ", "action 0 ", "action 200 ", "action 371 ",
           "tuning 0 ", "tuning 100 ", "tuning 214 ", "row 39 0 ", "row 41 0 ",
           "input ", "inputseq 50 ", "inputstep img+0x28b0 ", "texlist ", "texture 1 ", "slottexture 0x824 ",
           "strings ", "string 100 ", "actions ", "actionrec 955 ", "actionprog 2 ",
           "fxbolts ", "fxbolt 0 ", "fxtrails ", "fxtrail 0 ", "pool 0x010 ", "poolnode 0x418 31 ",
           "uiroot ", "uifont 1 ", "uifontobj ", "hud ", "fsm ", "fsmdef ", "fsmstate ", "fsmnode ", "fsmtrans ",
           "fsmsharedtrans ")


def find_data():
    if len(sys.argv) > 1:
        return sys.argv[1]
    roots = [ROOT]
    # In a worktree, orig/ only exists in the main checkout.
    r = subprocess.run(["git", "rev-parse", "--path-format=absolute", "--git-common-dir"], cwd=ROOT,
                       capture_output=True, text=True)
    if r.returncode == 0 and r.stdout.strip():
        roots.append(os.path.dirname(r.stdout.strip()))
    for root in roots:
        p = os.path.join(root, REL)
        if os.path.exists(p):
            return p
    return None


def build():
    out = os.path.join(ROOT, "build", "loader_test")
    os.makedirs(out, exist_ok=True)
    exe = os.path.join(out, "sir_loader_test.exe")
    flags = [f for f in check.FLAGS if f != "/c"]
    srcs = [os.path.join(ROOT, "tests", "sir_loader_test.cpp"), os.path.join(ROOT, "src", "port", "sir_loader.cpp")]
    script = os.path.join(out, "build.bat")
    with open(script, "w") as f:
        f.write('@call "' + check.vcvars() + '" >nul\r\n')
        f.write("@cl " + " ".join(flags) + " /O2 /Fo\"" + out + '\\\\" /Fe"' + exe + '" ' +
                " ".join('"' + s + '"' for s in srcs) + "\r\n")
    r = subprocess.run(["cmd", "/c", script], capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout[-4000:], r.stderr[-2000:])
        sys.exit("build failed")
    return exe


def compare(exe, mode, path, want, samples):
    """Runs the C++ loader on `path` and compares its dump with the reference lines `want`."""
    r = subprocess.run([exe, mode, path], capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout[-2000:], r.stderr[-2000:])
        return False
    got = r.stdout.splitlines()
    report = [l for l in got if l.startswith("report ")]
    got = [l for l in got if not l.startswith("report ")]
    bad = [(i, w, g) for i, (w, g) in enumerate(zip(want, got)) if w != g]
    for i, w, g in bad[:10]:
        print(f"line {i + 1}:\n  reference {w}\n  loader    {g}")
    kinds = {}
    for l in want:
        kinds[l.split(" ", 1)[0]] = kinds.get(l.split(" ", 1)[0], 0) + 1
    print(os.path.basename(path) + ": compared " + ", ".join(f"{n} {k}" for k, n in kinds.items()))
    for s in samples:
        for l in got:
            if l.startswith(s):
                print("  " + l)
                break
    if report:
        print("  loader " + report[0])
    if bad or len(want) != len(got):
        print(f"MISMATCH: {len(bad)} differing lines, {len(want)} reference vs {len(got)} loader lines")
        return False
    print(f"  OK: all {len(want)} lines match the reference decoder")
    return True


def is_level_file(path):
    """A header block and the images of zone 0 and zone 1: the *_level.sir files, the arenas, the
    castle rooms and the trouble scope map (read from the file's own section table). The rooms'
    packed copies (*C.sir) are left out."""
    name = os.path.basename(path).lower()
    if not name.endswith(".sir") or name == "global_binary.sir" or os.path.getsize(path) > 64 << 20:
        return False
    with open(path, "rb") as f:
        head = f.read(0x40 + 24 * 8)
    magic, version, count = struct.unpack_from(">4sII", head, 0)
    if magic != b"PNP0" or count > 8:
        return False
    entries = [struct.unpack_from(">iIIII", head, 0x40 + i * 24) for i in range(count)]
    if any(packed != 0 for t, off, size, unk, packed in entries):
        return False                       # a room's packed copy (*C.sir): the loader works on unpacked sections
    return sorted(e[0] for e in entries if e[1] != 0) == [-1, 0, 1]


def main():
    exe = build()
    r = subprocess.run([exe, "--selftest"], capture_output=True, text=True)
    print(r.stdout.strip())
    if r.returncode != 0:
        return 1

    data = find_data()
    if data is None:
        print("real-file check skipped: " + REL + " not found")
        return 0
    ok = compare(exe, "--dump", data, sir_decoder.gamedb_dump(data) + sir_decoder.image_dump(data), SAMPLES)

    streams = os.path.dirname(data)
    catalog = os.path.join(os.path.dirname(streams), "catalog.sir")
    if os.path.exists(catalog):
        ok = compare(exe, "--catalog", catalog, sir_decoder.catalog_dump(catalog),
                     ("catalog ", "namehash ", "lookup global_binary.sir -1", "lookup global_binary.sir 0")) and ok
        # Data sanity (Python only): the catalog must agree with the section table in each file.
        checked, problems = sir_decoder.catalog_crosscheck(catalog, streams)
        for p in problems[:10]:
            print("  " + p)
        print(f"  catalog vs section tables of {checked} files: " + ("OK" if not problems else f"{len(problems)} problems"))
        ok = ok and not problems
    else:
        print("catalog check skipped: catalog.sir not found next to streams/")

    # Texture sections of shared_texture.sir (`--texture`).
    path = os.path.join(streams, "shared_texture.sir")
    if os.path.exists(path):
        ok = compare(exe, "--texture", path, sir_decoder.texture_stream_dump(path), ()) and ok

    # Level files: header block and the HUD of zone 0 (`--level`).
    for name in sorted(os.listdir(streams)):
        if is_level_file(os.path.join(streams, name)):
            path = os.path.join(streams, name)
            ok = compare(exe, "--level", path, sir_decoder.level_dump(path), ("levelhdr ", "hud ") if name == "empty_level.sir" else ()) and ok
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
