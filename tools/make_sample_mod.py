"""Makes the sample mod (docs/modding.md). Everything in it is made here: no game data is in this repository.

    python tools/make_sample_mod.py [--out MODS_DIR] [--data EXTRACTED_DISC]

Writes <MODS_DIR>/SampleMod (default: build/pc/mods, where the port looks):
  mod.ini                          name, description, priority, enabled
  data/strings.json                changes the main menu's "New Game" (string 1552) to words of our own
  textures/MonsterLab_Logo.png     a striped picture drawn by this script, in place of the logo
  files/text/str_en.txt            only with --data: a copy of that file from YOUR extracted disc with
                                   one empty line added, to see a disc file being replaced. It is the
                                   game's own text: do not share it.
Needs Python only.
"""
import json
import os
import struct
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

MOD_INI = """[mod]
name = Sample mod
description = Renames New Game and paints a striped logo. Made by tools/make_sample_mod.py.
author = Monster Lab PC port
version = 1.0
; where a new mod goes in the list (a higher number is later, and wins); the Mods page changes it after that
priority = 0
; 0: a new mod starts switched off
enabled = 1
"""


def png(path, w, h, pixel):
    """An 8-bit RGBA PNG; pixel(x, y) gives (r, g, b, a)."""
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))
    rows = b"".join(b"\0" + bytes(c for x in range(w) for c in pixel(x, y)) for y in range(h))
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(rows, 9)) + chunk(b"IEND", b""))


def stripes(x, y):
    """Green and purple slanted stripes in an oval, see-through outside it."""
    if ((x - 127.5) / 127.5) ** 2 + ((y - 63.5) / 63.5) ** 2 > 1:
        return (0, 0, 0, 0)
    return (120, 220, 60, 255) if (x + y) // 16 % 2 else (110, 40, 150, 255)


def main():
    argv = sys.argv[1:]
    if "-h" in argv or "--help" in argv:
        print(__doc__)
        return 0
    out = argv[argv.index("--out") + 1] if "--out" in argv else os.path.join(ROOT, "build", "pc", "mods")
    data = argv[argv.index("--data") + 1] if "--data" in argv else None
    mod = os.path.join(out, "SampleMod")
    os.makedirs(os.path.join(mod, "data"), exist_ok=True)
    os.makedirs(os.path.join(mod, "textures"), exist_ok=True)
    with open(os.path.join(mod, "mod.ini"), "w", encoding="utf-8") as f:
        f.write(MOD_INI)
    with open(os.path.join(mod, "data", "strings.json"), "w", encoding="utf-8") as f:
        json.dump({"format": 1, "strings": {"1552": "Modded Game"}}, f, indent=2)
    png(os.path.join(mod, "textures", "MonsterLab_Logo.png"), 256, 128, stripes)
    if data:
        src = os.path.join(data, "files", "text", "str_en.txt")
        if not os.path.isfile(src):
            print(f"{src} is not there: --data takes an extracted disc folder (with sys/ and files/)")
            return 1
        os.makedirs(os.path.join(mod, "files", "text"), exist_ok=True)
        with open(src, "rb") as f:
            text = f.read()
        with open(os.path.join(mod, "files", "text", "str_en.txt"), "wb") as f:
            f.write(text + b"\r\n")
    print(f"sample mod written to {mod}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
