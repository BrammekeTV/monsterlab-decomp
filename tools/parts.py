"""Dump the monster part definitions from global_binary.sir (needs the extracted game).

    python tools/parts.py [lang]      -> build/parts_<lang>.tsv  (lang: en, fr, ge, it, sp)

How the data is found (see src/game/part_query.cpp):
  * global_binary.sir section 1 is a memory image loaded at 0x90400000; its 4 KB header
    (section 0) holds pointers. Header slot 0xDC8 is the game database (gGameDb, 0x805FBBC0).
  * gGameDb->data (+0xC) + 0x5B78 points at the part definition table, count at +0x5B7C.
  * Each definition is 0x50 bytes (PartDef in include/game/part.h). Its four tier names are
    text-key hashes, mapped to text ids by the sorted table at 0x8036D370 in main.dol, and
    text id N is line N+1 of files/text/str_<lang>.txt.
Output stays in build/ (game data, never committed).
"""
import struct
import sys

GB = "orig/RM7P4F/files/streams/global_binary.sir"
DOL = "orig/RM7P4F/sys/main.dol"
BASE = 0x90400000
TEXT_HASH_TABLE = 0x8036D370
TEXT_HASH_COUNT = 0x10B2


def dol_reader():
    dol = open(DOL, "rb").read()
    secs = []
    for k in range(18):
        off, addr, size = (struct.unpack(">I", dol[o + 4 * k:o + 4 * k + 4])[0] for o in (0, 0x48, 0x90))
        if size:
            secs.append((addr, off, size))

    def read(a, n):
        for b, o, s in secs:
            if b <= a < b + s:
                return dol[o + a - b:o + a - b + n]
        raise ValueError(hex(a))
    return read


def main():
    lang = sys.argv[1] if len(sys.argv) > 1 else "en"
    read = dol_reader()
    tab = read(TEXT_HASH_TABLE, TEXT_HASH_COUNT * 8)
    hash2id = {struct.unpack_from(">I", tab, i * 8)[0]: struct.unpack_from(">I", tab, i * 8 + 4)[0]
               for i in range(TEXT_HASH_COUNT)}
    lines = open(f"orig/RM7P4F/files/text/str_{lang}.txt", "rb").read().decode("latin1").split("\r\n")

    def text(h):
        i = hash2id.get(h)
        return "" if i is None else lines[i + 1]

    gb = open(GB, "rb").read()
    hdr = gb[0x800:0x1800]
    sec = gb[0x1800:]

    def word(addr):
        return struct.unpack_from(">I", sec, addr - BASE)[0]

    db = struct.unpack_from(">I", hdr, 0xDC8)[0]
    data = word(db + 0xC)
    table, count = word(data + 0x5B78), word(data + 0x5B7C)

    slots = {0: "head", 1: "legs", 2: "arm", 4: "torso"}
    disciplines = {0: "mech", 1: "bio", 2: "alchemy"}
    rows = ["index\ttier1\ttier2\ttier3\ttier4\tslot\tdiscipline\taction1\taction2\traw (+0x20..+0x4F)"]
    for i in range(count):
        r = struct.unpack_from(">20I", sec, table - BASE + i * 0x50)
        names = [text(h) for h in r[1:5]]
        slot = slots.get(r[6], str(r[6]))
        disc = disciplines.get(r[7], str(r[7]))
        raw = " ".join(f"{x:08x}" for x in r[8:])
        rows.append(f"{r[0]}\t" + "\t".join(names) + f"\t{slot}\t{disc}\t{r[5]}\t{r[11]}\t{raw}")
    out = f"build/parts_{lang}.tsv"
    open(out, "w", encoding="utf-8").write("\n".join(rows) + "\n")
    print(f"{count} part definitions -> {out}")


if __name__ == "__main__":
    main()
