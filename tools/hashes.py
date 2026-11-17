"""Resolve StringHash constants (CRC-32 of the upper-cased string) back to names.

    python tools/hashes.py build      -> writes build/hashes.json from strings in main.dol
                                         (+ text files under orig/RM7P4F/files)
    python tools/hashes.py scan       -> lists hash constants used in the asm and their names
    python tools/hashes.py <0xHASH>   -> look up one hash

The dictionary is built from game data at build time and is never committed.
"""
import glob
import json
import os
import re
import sys
import zlib

DICT = "build/hashes.json"


def game_hash(s):
    return zlib.crc32(s.upper().encode("latin1")) & 0xFFFFFFFF


def strings_in(data, minlen=3):
    for m in re.finditer(rb"[\x20-\x7e]{%d,}" % minlen, data):
        yield m.group(0).decode("latin1")


def build():
    names = set()
    names.update(strings_in(open("orig/RM7P4F/sys/main.dol", "rb").read()))
    for f in glob.glob("orig/RM7P4F/files/**/*", recursive=True):
        if os.path.isfile(f) and os.path.getsize(f) < 64 * 1024 * 1024:
            try:
                names.update(strings_in(open(f, "rb").read(), 4))
            except OSError:
                pass
        # file names themselves are hashed too (paths without extension)
        base = os.path.relpath(f, "orig/RM7P4F/files").replace("/", "\\")
        names.add(base)
        names.add(os.path.splitext(base)[0])
        names.add(os.path.splitext(os.path.basename(f))[0])
    extra = set()
    for n in names:
        for part in re.split(r"[\s,;=\"'()\[\]{}<>]+", n):
            if 2 < len(part) < 128:
                extra.add(part)
    names |= extra
    table = {}
    for n in names:
        table.setdefault(f"{game_hash(n):08X}", n)
    json.dump(table, open(DICT, "w"))
    print(f"{len(table)} hashes")


def asm_constants():
    """Yield (function, value) for 32-bit constants built with lis + addi/subi/ori."""
    for f in glob.glob("build/RM7P4F/asm/*_text.s"):
        func = None
        hi = {}
        for line in open(f):
            if line.startswith(".fn "):
                func = line.split()[1].rstrip(",")
                hi = {}
                continue
            ins = re.sub(r"^/\*.*?\*/\s*", "", line).strip()
            m = re.match(r"lis (r\d+), (-?0x[0-9a-f]+)$", ins)
            if m:
                hi[m.group(1)] = int(m.group(2), 16) & 0xFFFF
                continue
            m = re.match(r"(addi|subi|ori) (r\d+), (r\d+), (-?0x[0-9a-f]+|\d+)$", ins)
            if m and m.group(3) in hi:
                v = int(m.group(4), 0)
                base = hi[m.group(3)] << 16
                if m.group(1) == "addi": val = (base + v) & 0xFFFFFFFF
                elif m.group(1) == "subi": val = (base - v) & 0xFFFFFFFF
                else: val = base | v
                yield func, val


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "build":
        return build()
    table = json.load(open(DICT))
    if len(sys.argv) > 1 and sys.argv[1] == "scan":
        hits = {}
        for func, val in asm_constants():
            name = table.get(f"{val:08X}")
            if name:
                hits.setdefault(f"{val:08X}", (name, set()))[1].add(func)
        for h, (name, funcs) in sorted(hits.items(), key=lambda x: x[1][0].lower()):
            print(f"0x{h} {name!r:50} {len(funcs)} fn")
        print(f"{len(hits)} distinct hashes resolved", file=sys.stderr)
        return
    for a in sys.argv[1:]:
        print(a, table.get(f"{int(a, 16):08X}"))


if __name__ == "__main__":
    main()
