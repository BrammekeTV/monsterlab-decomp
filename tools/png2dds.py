"""Converts a texture pack's PNGs to DDS with mip levels, for the PC port's mods (docs/modding.md).

    python tools/png2dds.py PATH... [--format auto|bc1|bc3|bc7] [--no-mips] [--out DIR] [--force] [--delete-png]

PATH is a PNG or a folder (searched with its sub-folders), for example build/pc/mods/MyPack. Each
name.png gets a name.dds next to it (or under --out, keeping the sub-folders), so the matching by name
or hash stays the same. --format auto (the default) takes BC7 for a picture with transparency and BC1
for an opaque one. A .dds that is newer than its .png is left alone (--force converts it again).

The encoder is written here (numpy and Pillow, no outside program): BC1 and BC3 as usual, BC7 with
its mode 6 only (one pair of 7+1 bit RGBA end points per 4x4 block, 16 steps), which is close to the
original for painted textures and quick to encode.
"""
import argparse, os, struct, sys
from concurrent.futures import ProcessPoolExecutor

import numpy as np
from PIL import Image

BC7_WEIGHTS = np.array([0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64], np.float32) / 64.0
CHUNK = 16384                                  # blocks encoded at a time


def fit_line(p, steps, quantize):
    """Fits end points to the pixels of each block. p: (n, 16, c) floats; steps: the palette positions (0..1,
    ascending) between the two end points; quantize(e0, e1) gives the end points as the format stores them
    (codes0, codes1, value0, value1). Returns codes0, codes1 and the step number of each pixel."""
    mean = p.mean(axis=1, keepdims=True)
    d = p - mean
    cov = np.einsum("nki,nkj->nij", d, d)
    axis = cov[np.arange(len(p)), np.argmax(np.einsum("nii->ni", cov), axis=1)]      # a start close to the main axis
    for _ in range(8):
        axis = np.einsum("nij,nj->ni", cov, axis)
        axis /= np.maximum(np.abs(axis).max(axis=1, keepdims=True), 1e-9)
    axis /= np.maximum(np.linalg.norm(axis, axis=1, keepdims=True), 1e-9)
    t = np.einsum("nki,ni->nk", d, axis)
    e0 = mean[:, 0] + axis * t.min(axis=1, keepdims=True)
    e1 = mean[:, 0] + axis * t.max(axis=1, keepdims=True)
    mids = (steps[1:] + steps[:-1]) / 2
    best = None
    for attempt in range(3):                   # the principal axis, then two least-squares refinements
        c0, c1, v0, v1 = quantize(np.clip(e0, 0, 255), np.clip(e1, 0, 255))
        line = v1 - v0
        pos = np.einsum("nki,ni->nk", p - v0[:, None], line) / np.maximum((line * line).sum(axis=1)[:, None], 1e-9)
        idx = np.searchsorted(mids, pos)
        w = steps[idx]
        err = ((v0[:, None] + line[:, None] * w[:, :, None] - p) ** 2).sum(axis=(1, 2))
        if best is None:
            best = [c0, c1, idx, err]
        else:
            better = err < best[3]
            for i, a in enumerate((c0, c1, idx, err)):
                best[i] = np.where(better.reshape((-1,) + (1,) * (a.ndim - 1)), a, best[i])
        if attempt == 2:
            break
        a, b = 1 - w, w
        aa, ab, bb = (a * a).sum(1), (a * b).sum(1), (b * b).sum(1)
        det = aa * bb - ab * ab
        ap, bp = np.einsum("nk,nki->ni", a, p), np.einsum("nk,nki->ni", b, p)
        ok = det > 1e-6
        det = np.where(ok, det, 1)
        e0 = np.where(ok[:, None], (bb[:, None] * ap - ab[:, None] * bp) / det[:, None], v0)
        e1 = np.where(ok[:, None], (aa[:, None] * bp - ab[:, None] * ap) / det[:, None], v1)
    return best[0], best[1], best[2]


def quant565(e0, e1):
    out = []
    for e in (e0, e1):
        r = np.rint(e[:, 0] * 31 / 255).astype(np.uint32)
        g = np.rint(e[:, 1] * 63 / 255).astype(np.uint32)
        b = np.rint(e[:, 2] * 31 / 255).astype(np.uint32)
        value = np.stack([r << 3 | r >> 2, g << 2 | g >> 4, b << 3 | b >> 2], axis=1).astype(np.float32)
        out.append((r << 11 | g << 5 | b, value))
    return out[0][0], out[1][0], out[0][1], out[1][1]


def bc1_blocks(rgb):
    """(n, 16, 3) -> (n, 8) bytes, four-colour mode."""
    c0, c1, pos = fit_line(rgb, np.array([0, 1 / 3, 2 / 3, 1], np.float32), quant565)
    swap = c0 < c1                             # four colours need the first end point to be the larger number
    c0, c1 = np.where(swap, c1, c0), np.where(swap, c0, c1)
    pos = np.where(swap[:, None], 3 - pos, pos)
    code = np.array([0, 2, 3, 1], np.uint32)[pos]
    code[c0 == c1] = 0
    bits = (code << (2 * np.arange(16, dtype=np.uint32))).sum(axis=1, dtype=np.uint64)
    out = np.empty(len(rgb), dtype=[("c0", "<u2"), ("c1", "<u2"), ("bits", "<u4")])
    out["c0"], out["c1"], out["bits"] = c0, c1, bits
    return out.view(np.uint8).reshape(-1, 8)


def alpha_blocks(a):
    """(n, 16) -> (n, 8) bytes: BC3's alpha half, eight values."""
    hi, lo = a.max(axis=1), a.min(axis=1)
    pos = np.rint((hi[:, None] - a) * 7 / np.maximum(hi - lo, 1)[:, None]).astype(np.uint64)
    code = np.array([0, 2, 3, 4, 5, 6, 7, 1], np.uint64)[pos]
    code[hi == lo] = 0
    bits = (code << (3 * np.arange(16, dtype=np.uint64))).sum(axis=1, dtype=np.uint64)
    word = hi.astype(np.uint64) | lo.astype(np.uint64) << 8 | bits << 16
    return word.astype("<u8").view(np.uint8).reshape(-1, 8)


def bc3_blocks(rgba):
    return np.concatenate([alpha_blocks(rgba[:, :, 3]), bc1_blocks(rgba[:, :, :3])], axis=1)


def quant7p(e0, e1):
    """BC7 mode 6 end points: 7 bits per channel and one low bit shared by the four channels of an end point."""
    out = []
    for e in (e0, e1):
        tries = []
        for bit in (0, 1):
            q = np.clip(np.rint((e - bit) / 2), 0, 127)
            tries.append((q, q * 2 + bit, ((q * 2 + bit - e) ** 2).sum(axis=1)))
        one = tries[1][2] < tries[0][2]
        one = np.where(e[:, 3] >= 254.5, True, np.where(e[:, 3] <= 0.5, False, one))     # opaque stays 255, clear stays 0
        q = np.where(one[:, None], tries[1][0], tries[0][0]).astype(np.uint64)
        code = np.concatenate([q, one[:, None].astype(np.uint64)], axis=1)
        out.append((code, np.where(one[:, None], tries[1][1], tries[0][1]).astype(np.float32)))
    return out[0][0], out[1][0], out[0][1], out[1][1]


def bc7_blocks(rgba):
    """(n, 16, 4) -> (n, 16) bytes, mode 6."""
    c0, c1, idx = fit_line(rgba, BC7_WEIGHTS, quant7p)
    swap = idx[:, 0] >= 8                      # the first pixel's index is stored without its top bit
    c0, c1 = np.where(swap[:, None], c1, c0), np.where(swap[:, None], c0, c1)
    idx = np.where(swap[:, None], 15 - idx, idx).astype(np.uint64)
    u = np.uint64
    lo = np.full(len(rgba), 1 << 6, u)         # mode 6
    shift = 7
    for ch in range(4):
        for c in (c0, c1):
            lo |= c[:, ch] << u(shift)
            shift += 7
    lo |= c0[:, 4] << u(63)
    hi = c1[:, 4] | idx[:, 0] << u(1)
    for i in range(1, 16):
        hi |= idx[:, i] << u(4 * i)
    return np.stack([lo, hi], axis=1).astype("<u8").view(np.uint8).reshape(-1, 16)


def blocks_of(px):
    """(h, w, 4) -> (n, 16, 4), the edge repeated where a level is not a multiple of 4."""
    h, w = px.shape[:2]
    px = np.pad(px, ((0, -h % 4), (0, -w % 4), (0, 0)), mode="edge")
    h, w = px.shape[:2]
    return px.reshape(h // 4, 4, w // 4, 4, 4).swapaxes(1, 2).reshape(-1, 16, 4)


def encode_level(px, fmt):
    blocks = blocks_of(px)
    out = []
    for at in range(0, len(blocks), CHUNK):
        b = blocks[at:at + CHUNK].astype(np.float32)
        out.append(bc1_blocks(b[:, :, :3]) if fmt == "bc1" else bc3_blocks(b) if fmt == "bc3" else bc7_blocks(b))
    return np.concatenate(out).tobytes()


def half(px):
    """The next mip level: 2x2 box; colour weighted by alpha, so clear pixels do not darken the edges."""
    h, w = px.shape[:2]
    nh, nw = max(h // 2, 1), max(w // 2, 1)
    ys = [np.minimum(np.arange(nh) * 2 + k, h - 1) for k in (0, 1)]
    xs = [np.minimum(np.arange(nw) * 2 + k, w - 1) for k in (0, 1)]
    four = np.stack([px[y][:, x] for y in ys for x in xs]).astype(np.float32)
    alpha = four[..., 3:]
    total = alpha.sum(axis=0)
    plain = four[..., :3].mean(axis=0)
    rgb = np.where(total > 0, (four[..., :3] * alpha).sum(axis=0) / np.maximum(total, 1), plain)
    return np.rint(np.concatenate([rgb, total / 4], axis=2)).astype(np.uint8)


def dds_header(w, h, levels, fmt):
    size = ((w + 3) // 4) * ((h + 3) // 4) * (8 if fmt == "bc1" else 16)
    flags = 0x1 | 0x2 | 0x4 | 0x1000 | 0x80000 | (0x20000 if levels > 1 else 0)
    caps = 0x1000 | (0x400008 if levels > 1 else 0)
    four = {"bc1": b"DXT1", "bc3": b"DXT5", "bc7": b"DX10"}[fmt]
    head = b"DDS " + struct.pack("<7I44x", 124, flags, h, w, size, 0, levels)
    head += struct.pack("<2I4s5I", 32, 4, four, 0, 0, 0, 0, 0)
    head += struct.pack("<5I", caps, 0, 0, 0, 0)
    if fmt == "bc7":
        head += struct.pack("<5I", 98, 3, 0, 1, 0)        # DXGI_FORMAT_BC7_UNORM, a 2D texture
    return head


def convert(src, dst, fmt="auto", mips=True):
    """Writes src (a picture) as the DDS dst; returns a line saying what was written."""
    im = Image.open(src).convert("RGBA")
    note = ""
    w, h = (im.width + 3) // 4 * 4, (im.height + 3) // 4 * 4
    if (w, h) != im.size:                      # block formats need multiples of 4; the game stretches the picture anyway
        note = f", resized from {im.width}x{im.height}"
        im = im.resize((w, h), Image.LANCZOS)
    px = np.asarray(im)
    if fmt == "auto":
        fmt = "bc7" if px[:, :, 3].min() < 255 else "bc1"
    levels = []
    while True:
        levels.append(encode_level(px, fmt))
        if not mips or px.shape[:2] == (1, 1):
            break
        px = half(px)
    os.makedirs(os.path.dirname(os.path.abspath(dst)), exist_ok=True)
    with open(dst + ".tmp", "wb") as f:
        f.write(dds_header(w, h, len(levels), fmt))
        f.write(b"".join(levels))
    os.replace(dst + ".tmp", dst)
    return f"{dst}: {w}x{h} {fmt.upper()}, {len(levels)} mip level{'s' if len(levels) > 1 else ''}{note}"


def job(args):
    src, dst, fmt, mips = args
    try:
        return True, convert(src, dst, fmt, mips)
    except Exception as e:                     # one broken picture does not stop the pack
        return False, f"{src}: {e}"


def main(argv=None):
    ap = argparse.ArgumentParser(description="Convert a texture pack's PNGs to DDS (BC1/BC3/BC7 with mips).")
    ap.add_argument("paths", nargs="+", help="PNG files or folders")
    ap.add_argument("--format", choices=["auto", "bc1", "bc3", "bc7"], default="auto")
    ap.add_argument("--no-mips", action="store_true")
    ap.add_argument("--out", help="write under this folder instead of next to each PNG")
    ap.add_argument("--force", action="store_true", help="convert even if the .dds is newer")
    ap.add_argument("--delete-png", action="store_true", help="delete each PNG once its .dds is written")
    a = ap.parse_args(argv)
    jobs, pngs, skipped = [], [], 0
    for path in a.paths:
        if os.path.isdir(path):
            found = [(os.path.join(d, n), os.path.relpath(os.path.join(d, n), path)) for d, _, names in os.walk(path) for n in names if n.lower().endswith(".png")]
        elif os.path.isfile(path):
            found = [(path, os.path.basename(path))]
        else:
            sys.exit(f"no such file or folder: {path}")
        for src, rel in found:
            dst = os.path.splitext(os.path.join(a.out, rel) if a.out else src)[0] + ".dds"
            if not a.force and os.path.exists(dst) and os.path.getmtime(dst) >= os.path.getmtime(src):
                skipped += 1
                continue
            jobs.append((src, dst, a.format, not a.no_mips))
    failed = 0
    with ProcessPoolExecutor(max_workers=min(len(jobs), os.cpu_count() or 1, 8) or 1) as pool:
        for (src, *_), (ok, line) in zip(jobs, pool.map(job, jobs)):
            print(line)
            if not ok:
                failed += 1
            elif a.delete_png:
                pngs.append(src)
    for src in pngs:
        os.remove(src)
    print(f"{len(jobs) - failed} converted, {skipped} up to date, {failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
