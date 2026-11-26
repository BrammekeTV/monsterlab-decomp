"""Black hole head: packs the output of make_assets.py into a mod folder the PC port can load.

    python tools/mods/blackhole/make_assets.py
    python tools/mods/blackhole/build_mod.py [--assets DIR] [--out MODS_DIR]

Writes MODS_DIR/BlackHoleHead/parts/p_hd_sha077.part (default MODS_DIR: build/pc/mods, never committed).
Needs numpy and Pillow. Nothing of the game is read and nothing of the game is in the pack: it holds this
mod's own vertices, draw commands and pictures, and says where in the zone of a head of the player's own
disc (the "template", p_hd_sha002) the port writes them when that zone is in memory for the black hole.

What the port keeps of the template: its skeleton, animations, material (lit, environment map, rim light),
the matrix commands at the start of its display list and its one vertex colour.

The .part file (little-endian; src/port/port_mod_parts.cpp reads it):
  "MLP1", u32 version (1)
  char key[32]                  model key (p_hd_sha077)
  u32 templateRow, modelNumber  part definition row that is copied, the new row's model number
  u32 templateHash              name hash of the template's model, as it stands in its zone
  u32 zone[2], zoneSize         the template's two zones of shared_parts.sir (one per fighter) and their size
  u32 defIdOffset               where the zone holds that hash
  char name[4][48]              the names of the four tiers
  u32 blockCount, then blocks {u32 kind, u32 offset, u32 size, data padded to 4 bytes}
     kind 0: bytes written at zone + offset (vertex arrays, draw commands and texture data, in the console's layout)
     kind 1: pairs {u32 where, u32 target}: the big-endian word at zone + where becomes the address of zone + target
"""
import argparse
import json
import os
import struct
import zlib

import numpy as np
from PIL import Image

KEY = "p_hd_sha077"
TEMPLATE_KEY = "p_hd_sha002"
TEMPLATE_ROW = 73               # PartDef row of p_hd_sha002 (set sha, number 2)
MODEL_NUMBER = 77
ZONES = (23, 24)
ZONE_SIZE = 0xA54F4
DEF_ID = 0x1A8
NAMES = ("Black Hole", "Dense Black Hole", "Massive Black Hole", "Supermassive Black Hole")   # tiers 1 to 4

# The template's layout (PAL RM7P4F, zones 23 and 24 are the same apart from addresses).
POS, NRM, UV = 0x1AB20, 0x1B450, 0x1BA28     # arrays: ours start where the template's start (uv: the same place)
ARRAY_END = (0x1BA24, 0x1BA24, 0x1C400)      # positions and normals end before the colour, uvs before the list
LIST = 0x1C400                               # display list: 0x42 bytes of array and format commands,
LIST_SETUP = 0x42                            # then 0x45 bytes of matrix commands (kept), then the draws
LIST_DRAWS = 0x1C487
LIST_END = 0x1F200
TEX_COLOUR, TEX_GLOSS, TEX_BYTES = 0xD00, 0xE340, 0xAAC0   # 256x256 CMPR, 7 levels each
BONE, TEX_MTX = 9, 39           # matrix numbers a vertex names: the template's skull (position, and its environment map)
SCALE = 2.6                     # model units -> the head's units
OFFSET = (0.0, -0.05, 0.0)


def string_hash(s):
    return zlib.crc32(s.upper().encode("latin1")) & 0xFFFFFFFF


def cmpr(rgb):
    """GX CMPR (DXT1 in 8x8 tiles of four blocks, big-endian) of an (h, w, 3) float picture; h and w multiples of 8
    (smaller levels are padded by repeating)."""
    h, w = rgb.shape[:2]
    if h < 8 or w < 8:
        rgb = np.tile(rgb, (8 // h, 8 // w, 1))
        h, w = 8, 8
    px = np.clip(rgb * 255 + 0.5, 0, 255).astype(np.int32)
    out = bytearray()
    for ty in range(0, h, 8):
        for tx in range(0, w, 8):
            for by, bx in ((0, 0), (0, 4), (4, 0), (4, 4)):
                b = px[ty + by:ty + by + 4, tx + bx:tx + bx + 4].reshape(16, 3)
                lo, hi = b.min(0), b.max(0)
                q = lambda c: ((c[0] >> 3) << 11) | ((c[1] >> 2) << 5) | (c[2] >> 3)
                c0, c1 = q(hi), q(lo)
                if c0 == c1:
                    if c0 == 0:
                        c0 = 1
                    else:
                        c1 = c0 - 1 if c0 & 0x1F else c0 - 0x20 if c0 & 0x7E0 else c0 - 0x800
                    idx = [0] * 16 if q(hi) == c0 else [1] * 16
                else:
                    e = lambda c: np.array([(c >> 11) * 255 // 31, ((c >> 5) & 63) * 255 // 63, (c & 31) * 255 // 31])
                    p0, p1 = e(c0), e(c1)
                    pal = np.array([p0, p1, (2 * p0 + p1) // 3, (p0 + 2 * p1) // 3])
                    idx = ((b[:, None, :] - pal[None]) ** 2).sum(2).argmin(1)
                bits = 0
                for i in idx:
                    bits = (bits << 2) | int(i)
                out += struct.pack(">HHI", c0, c1, bits)
    return bytes(out)


def cmpr_chain(img, levels=7):
    out = b""
    for l in range(levels):
        s = img.width >> l
        out += cmpr(np.asarray(img.resize((s, s), Image.LANCZOS).convert("RGB")) / 255.0)
    return out


def block(kind, off, data):
    return struct.pack("<III", kind, off, len(data)) + data + b"\0" * (-len(data) % 4)


def main():
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--assets", default=os.path.join(root, "build", "mods", "BlackHoleHead"))
    ap.add_argument("--out", default=os.path.join(root, "build", "pc", "mods"))
    a = ap.parse_args()
    m = json.load(open(os.path.join(a.assets, "model", "blackhole.json")))
    n = len(m["positions"])
    pos = (np.array(m["positions"]) * SCALE + np.array(OFFSET))
    assert np.abs(pos).max() < 1.99, "the model does not fit the position format (s16, 14 fraction bits)"
    pos_b = b"".join(struct.pack(">3h", *(int(round(c * 16384)) for c in p)) for p in pos)
    nrm_b = b"".join(struct.pack(">3b", *(max(-64, min(64, int(round(c * 64)))) for c in v)) for v in m["normals"])
    uv = np.array(m["uvs"])
    assert uv.min() >= 0 and uv.max() < 255
    uv_b = b"".join(struct.pack(">2H", *(int(round(c * 256)) for c in t)) for t in uv)
    assert POS + len(pos_b) <= NRM and NRM + len(nrm_b) <= ARRAY_END[1] and UV + len(uv_b) <= ARRAY_END[2], "arrays too big for the template"

    # The start of the display list: where the arrays are, what a vertex is made of, the formats.
    # A vertex: position matrix, texture matrix 1 (bytes), position, normal (16-bit indices), colour (8-bit), uv (16-bit).
    setup = bytearray()
    fix = []
    for reg, target in ((0xA0, POS), (0xA1, NRM), (0xA2, 0x1BA24), (0xA4, UV)):
        fix.append((LIST + len(setup) + 2, target))
        setup += struct.pack(">BBI", 0x08, reg, 0)
    setup += struct.pack(">BBI", 0x08, 0x50, 0x5E05) + struct.pack(">BBI", 0x08, 0x60, 0x3)
    for reg, stride in ((0xB0, 6), (0xB1, 3), (0xB2, 2), (0xB4, 4)):
        setup += struct.pack(">BBI", 0x08, reg, stride)
    # format 2: position xyz s16 >> 14, normal s8, colour RGB565, uv u16 >> 8 (the template's word with our position format)
    setup += struct.pack(">BBI", 0x08, 0x72, 0x50A00000 | 1 | (3 << 1) | (14 << 4) | (1 << 10))
    assert len(setup) == LIST_SETUP
    draws = bytearray()
    count = 0
    for group, strips in m["strips"].items():
        for s in strips:
            draws += struct.pack(">BH", 0x9A, len(s))
            for i in s:
                draws += struct.pack(">BBHHBH", BONE, TEX_MTX, i, i, 0, i)
            count += len(s)
    assert LIST_DRAWS + len(draws) <= LIST_END, "display list too big for the template"
    draws += b"\0" * (LIST_END - LIST_DRAWS - len(draws))              # GX no-ops to the end of the template's list

    tex = os.path.join(a.assets, "textures")
    colour = cmpr_chain(Image.open(os.path.join(tex, "bho_h_01.png")))
    gloss = cmpr_chain(Image.open(os.path.join(tex, "bho_h_01spec.png")))
    assert len(colour) == TEX_BYTES and len(gloss) == TEX_BYTES

    blocks = [block(0, POS, pos_b), block(0, NRM, nrm_b), block(0, UV, uv_b), block(0, LIST, bytes(setup)),
              block(0, LIST_DRAWS, bytes(draws)), block(0, TEX_COLOUR, colour), block(0, TEX_GLOSS, gloss),
              block(1, 0, b"".join(struct.pack("<II", w, t) for w, t in fix))]
    head = b"MLP1" + struct.pack("<I", 1) + KEY.encode().ljust(32, b"\0")
    head += struct.pack("<III", TEMPLATE_ROW, MODEL_NUMBER, string_hash(TEMPLATE_KEY + "_geoA"))
    head += struct.pack("<IIII", ZONES[0], ZONES[1], ZONE_SIZE, DEF_ID)
    head += b"".join(nm.encode().ljust(48, b"\0") for nm in NAMES)
    head += struct.pack("<I", len(blocks))
    out = os.path.join(a.out, "BlackHoleHead", "parts")
    os.makedirs(out, exist_ok=True)
    path = os.path.join(out, KEY + ".part")
    with open(path, "wb") as f:
        f.write(head + b"".join(blocks))
    print("black hole head: %d vertices, %d strip vertices, %d bytes -> %s" % (n, count, os.path.getsize(path), path))


if __name__ == "__main__":
    main()
