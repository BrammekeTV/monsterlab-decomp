# Contributed by the project owner (originally written for another project).
# See tools/sir_decoder.md for usage and format notes.
#!/usr/bin/env python3
"""
sir_decoder.py - extractor for Monster Lab (Wii) .sir archives ("PNP0" format)

Extracts from a .sir archive:
  * textures  -> PNG  (CMPR, RGBA8, RGB5A3, RGB565, I4, I8, IA4, IA8)
  * meshes    -> OBJ  (GX display lists decoded to triangles, with normals and UVs)
                 - skinned monster parts (e.g. p_to_mbo001_geo) are written at their
                   real size using the scale factor stored with each part
                 - static/level meshes are written as-is
  * skeletons -> JSON (bind-pose joint matrices of skinned parts, when present)
  * raw sections -> .bin (optional)

Requirements: Python 3.8+, numpy, Pillow  (pip install numpy pillow)

Usage:
  python sir_decoder.py mhartopia_arena2.sir                 # extract into ./mhartopia_arena2_out
  python sir_decoder.py shared_parts.sir -o D:\\rip          # choose output folder
  python sir_decoder.py some.sir --list                       # just list what's inside
  python sir_decoder.py some.sir --sections                   # also dump raw sections
  python sir_decoder.py some.sir --only mbo                   # only items whose name contains "mbo"
  python sir_decoder.py global_binary.sir --gamedb            # print the game database (text, to stdout)
  python sir_decoder.py global_binary.sir --image             # print input table, textures, string/action tables
  python sir_decoder.py catalog.sir --catalog                 # print the file table (text, to stdout)

Format notes (reverse-engineered):
  Archive header (big-endian): 'PNP0', u32 version, u32 section count, ...
  Section table at 0x40, 24 bytes per entry: i32 type, u32 offset, u32 size, u32, u32, u32 hash.
  Each section is a raw memory image that the game loads at a fixed address
  (e.g. 0x90980000); pointers inside it are absolute Wii addresses, so the load
  address is recovered automatically from texture/mesh pointers.
  Textures: 0x54-byte header: u32 0x54, u16 w, u16 h, u16 w, u16 h, ..., u32 mips,
  u8 format, ..., u32 data size, u32 data pointer, name at +0x35.
  Meshes: standard GX command streams (CP/XF/BP writes + indexed draw primitives).
"""
import argparse, json, os, re, struct, sys, collections
try:
    import numpy as np
except ImportError:      # only --gamedb works without numpy
    np = None

try:
    from PIL import Image
except ImportError:
    Image = None

PAD = b'\x7f\x80\x00\x01'

# ----------------------------------------------------------------------------- archive
def read_archive(path):
    data = open(path, 'rb').read()
    magic, version, count = struct.unpack_from('>4sII', data, 0)
    if magic != b'PNP0':
        raise SystemExit(f'{path}: not a PNP0 .sir archive (magic {magic!r})')
    secs = []
    for i in range(count):
        o = 0x40 + i * 24
        if o + 24 > len(data):
            break
        typ, off, size, a, b, h = struct.unpack_from('>iIIIII', data, o)
        if off == 0 or off + size > len(data):
            continue
        secs.append(dict(index=i, type=typ, offset=off, size=size, hash=h,
                         data=data[off:off + size]))
    return version, secs

def cstr(d, o, maxlen=128):
    e = d.find(b'\x00', o, o + maxlen)
    if e < 0:
        return None
    s = d[o:e]
    if not s or not all(32 <= c < 127 for c in s):
        return None
    return s.decode('ascii')

def safe(name):
    return re.sub(r'[^A-Za-z0-9_.\-]+', '_', name)[:120] or 'unnamed'

# ----------------------------------------------------------------------------- textures
TEX_FMT = {0x0: 'I4', 0x1: 'I8', 0x2: 'IA4', 0x3: 'IA8', 0x4: 'RGB565', 0x5: 'RGB5A3',
           0x6: 'RGBA8', 0x8: 'C4', 0x9: 'C8', 0xA: 'C14X2', 0xE: 'CMPR'}
BLOCK = {0x0: (8, 8, 4), 0x1: (8, 4, 8), 0x2: (8, 4, 8), 0x3: (4, 4, 16), 0x4: (4, 4, 16),
         0x5: (4, 4, 16), 0x6: (4, 4, 32), 0xE: (8, 8, 4)}

def tex_size(fmt, w, h):
    bw, bh, bpp = BLOCK[fmt]
    W = (w + bw - 1) // bw * bw; H = (h + bh - 1) // bh * bh
    return W * H * bpp // 8, W, H

def find_textures(d):
    out = []
    for m in re.finditer(rb'\x00\x00\x00\x54', d):
        o = m.start()
        if o % 4 or o + 0x40 > len(d):
            continue
        w, h, w2, h2 = struct.unpack_from('>HHHH', d, o + 4)
        if w != w2 or h != h2 or not (1 <= w <= 2048 and 1 <= h <= 2048):
            continue
        mips = struct.unpack_from('>I', d, o + 0x10)[0]
        fmt = d[o + 0x14]
        size, ptr = struct.unpack_from('>II', d, o + 0x18)
        if not (1 <= mips <= 12) or fmt not in TEX_FMT or not (0 < size < 0x1000000):
            continue
        if not (0x80000000 <= ptr < 0x94000000):
            continue
        name = cstr(d, o + 0x35)
        out.append(dict(hdr=o, w=w, h=h, mips=mips, fmt=fmt, size=size, ptr=ptr,
                        name=name or f'tex_{o:x}', name_end=(o + 0x35 + len(name) + 1) if name else o + 0x54))
    return out

def _blocks(raw, w, h, bw, bh, bpe):
    """Untile GX block layout -> (H, W, bpe) array of per-texel byte groups."""
    W = (w + bw - 1) // bw * bw; H = (h + bh - 1) // bh * bh
    n = W * H * bpe
    a = np.frombuffer(raw[:n], np.uint8)
    if a.size < n:
        a = np.pad(a, (0, n - a.size))
    a = a.reshape(H // bh, W // bw, bh, bw, bpe).transpose(0, 2, 1, 3, 4).reshape(H, W, bpe)
    return a[:h, :w]

def _c565(v):
    r = ((v >> 11) & 31) * 255 // 31; g = ((v >> 5) & 63) * 255 // 63; b = (v & 31) * 255 // 31
    return r, g, b

def decode_texture(raw, fmt, w, h):
    if fmt == 0xE:  # CMPR: 8x8 tiles of four 4x4 DXT1 sub-blocks
        W = (w + 7) // 8 * 8; H = (h + 7) // 8 * 8
        nb = (W // 4) * (H // 4)
        a = np.frombuffer(raw[:nb * 8].ljust(nb * 8, b'\x00'), np.uint8).reshape(H // 8, W // 8, 2, 2, 8)
        c0 = (a[..., 0].astype(np.uint32) << 8) | a[..., 1]; c1 = (a[..., 2].astype(np.uint32) << 8) | a[..., 3]
        bits = (a[..., 4].astype(np.uint32) << 24) | (a[..., 5].astype(np.uint32) << 16) | (a[..., 6].astype(np.uint32) << 8) | a[..., 7]
        p0 = np.stack(_c565(c0) + (np.full(c0.shape, 255),), -1).astype(np.int32)
        p1 = np.stack(_c565(c1) + (np.full(c1.shape, 255),), -1).astype(np.int32)
        gt = (c0 > c1)[..., None]
        p2 = np.where(gt, (2 * p0 + p1) // 3, (p0 + p1) // 2)
        p3 = np.where(gt, (p0 + 2 * p1) // 3, 0)
        pal = np.stack([p0, p1, p2, p3], -2)          # ty,tx,sy,sx,4,4
        img = np.zeros((H, W, 4), np.uint8)
        for py in range(4):
            for px in range(4):
                idx = (bits >> (30 - 2 * (py * 4 + px))) & 3
                col = np.take_along_axis(pal, idx[..., None, None].astype(np.int64), -2)[..., 0, :]
                # place: y = ty*8+sy*4+py, x = tx*8+sx*4+px
                img.reshape(H // 8, 2, 4, W // 8, 2, 4, 4)[:, :, py, :, :, px, :] = col.transpose(0, 2, 1, 3, 4)
        return img[:h, :w]
    if fmt == 0x6:  # RGBA8: 4x4 tiles, AR block then GB block
        W = (w + 3) // 4 * 4; H = (h + 3) // 4 * 4
        a = np.frombuffer(raw[:W * H * 4].ljust(W * H * 4, b'\x00'), np.uint8).reshape(H // 4, W // 4, 2, 16, 2)
        A = a[:, :, 0, :, 0]; R = a[:, :, 0, :, 1]; G = a[:, :, 1, :, 0]; B = a[:, :, 1, :, 1]
        img = np.stack([R, G, B, A], -1).reshape(H // 4, W // 4, 4, 4, 4).transpose(0, 2, 1, 3, 4).reshape(H, W, 4)
        return img[:h, :w]
    if fmt in (0x4, 0x5):
        t = _blocks(raw, w, h, 4, 4, 2); v = (t[..., 0].astype(np.uint32) << 8) | t[..., 1]
        if fmt == 0x4:
            r, g, b = _c565(v); a = np.full(v.shape, 255)
        else:
            opaque = (v & 0x8000) != 0
            r = np.where(opaque, ((v >> 10) & 31) * 255 // 31, ((v >> 8) & 15) * 17)
            g = np.where(opaque, ((v >> 5) & 31) * 255 // 31, ((v >> 4) & 15) * 17)
            b = np.where(opaque, (v & 31) * 255 // 31, (v & 15) * 17)
            a = np.where(opaque, 255, ((v >> 12) & 7) * 255 // 7)
        return np.stack([r, g, b, a], -1).astype(np.uint8)
    if fmt == 0x0:  # I4
        W = (w + 7) // 8 * 8; H = (h + 7) // 8 * 8
        a = np.frombuffer(raw[:W * H // 2].ljust(W * H // 2, b'\x00'), np.uint8).reshape(H // 8, W // 8, 8, 4)
        hi = (a >> 4) * 17; lo = (a & 15) * 17
        t = np.stack([hi, lo], -1).reshape(H // 8, W // 8, 8, 8).transpose(0, 2, 1, 3).reshape(H, W)[:h, :w]
        return np.stack([t, t, t, np.full(t.shape, 255, np.uint8)], -1).astype(np.uint8)
    if fmt == 0x1:  # I8
        t = _blocks(raw, w, h, 8, 4, 1)[..., 0]
        return np.stack([t, t, t, np.full(t.shape, 255, np.uint8)], -1)
    if fmt == 0x2:  # IA4
        t = _blocks(raw, w, h, 8, 4, 1)[..., 0]; i = (t & 15) * 17; a = (t >> 4) * 17
        return np.stack([i, i, i, a], -1).astype(np.uint8)
    if fmt == 0x3:  # IA8
        t = _blocks(raw, w, h, 4, 4, 2); a = t[..., 0]; i = t[..., 1]
        return np.stack([i, i, i, a], -1)
    return None  # palette formats not supported

# ----------------------------------------------------------------------------- load address
def detect_base(d, textures):
    votes = collections.Counter()
    for t in textures:
        data_off = (t['name_end'] + 31) & ~31
        votes[(t['ptr'] - data_off) & ~0xff] += 1
    # skinned part meshes: CP array base for positions vs header offset
    for H, hdr in find_part_headers(d):
        s = H + hdr[6]
        if d[s:s + 2] == b'\x08\xa0':
            a0 = struct.unpack_from('>I', d, s + 2)[0] | 0x80000000
            votes[(a0 - (H + hdr[4])) & ~0xff] += 3
    if not votes:
        return None
    return votes.most_common(1)[0][0]

# ----------------------------------------------------------------------------- meshes
def find_part_headers(d):
    """Skinned monster-part meshes: name, 0x7f800001 padding, then a header whose
    word[3]==0x60 (header size), word[4]=position array offset, word[6]=command stream offset,
    word[0]=total size."""
    res = []
    for m in re.finditer(re.escape(PAD), d):
        o = m.end()
        if o % 4 or d[o:o + 4] == PAD or o + 40 > len(d):
            continue
        h = struct.unpack_from('>10I', d, o)
        if h[3] == 0x60 and 0 < h[6] < h[0] < 0x400000 and h[4] < h[0]:
            res.append((o, h))
    return res

def name_before(d, o, span=0x200):
    """Closest readable identifier ending before offset o (skipping padding)."""
    p = o
    while p >= 4 and d[p - 4:p] == PAD:
        p -= 4
    best = None
    for m in re.finditer(rb'([A-Za-z_][A-Za-z0-9_:.\-]{3,})\x00', d[max(0, p - span):p]):
        best = m.group(1).decode()
    return best

class GX:
    OPS_DRAW = {0x80: 'quads', 0x90: 'tris', 0x98: 'strip', 0xA0: 'fan', 0xA8: 'lines', 0xB0: 'linestrip', 0xB8: 'points'}

    def __init__(self, d, base):
        self.d = d; self.base = base

    def off(self, phys):
        o = (phys | 0x80000000) - self.base
        return o if 0 <= o < len(self.d) else None

    def parse(self, start, end=None):
        d = self.d; o = start; cp = {}; draws = []
        end = end or len(d)
        while o < end:
            op = d[o]
            if op == 0x00:
                o += 1; continue
            if op == 0x08:
                if o + 6 > end: break
                cp[d[o + 1]] = struct.unpack_from('>I', d, o + 2)[0]; o += 6
            elif op == 0x10:
                hdr = struct.unpack_from('>I', d, o + 1)[0]; n = (hdr >> 16) + 1
                if n > 64: break
                o += 5 + 4 * n
            elif op in (0x20, 0x28, 0x30, 0x38):
                o += 5
            elif op == 0x61:
                o += 5
            elif op & 0x80 and (op & 0xF8) in self.OPS_DRAW:
                prim = op & 0xF8; fmt = op & 7
                n = struct.unpack_from('>H', d, o + 1)[0]; o += 3
                attrs = self.attrs(cp)
                if attrs is None or n == 0 or n > 20000: break
                vsize = sum(s for _, s in attrs)
                if o + n * vsize > end: break
                verts = []
                for _ in range(n):
                    v = {}
                    for nm, s in attrs:
                        v[nm] = d[o] if s == 1 else struct.unpack_from('>H', d, o)[0]
                        o += s
                    verts.append(v)
                draws.append((prim, fmt, verts, dict(cp)))
            else:
                break
        return draws, o

    @staticmethod
    def attrs(cp):
        lo = cp.get(0x50, 0); hi = cp.get(0x60, 0); out = []
        for i in range(9):
            if lo >> i & 1: out.append(('mtx%d' % i, 1))
        for i, nm in enumerate(('pos', 'nrm', 'c0', 'c1')):
            t = (lo >> (9 + 2 * i)) & 3
            if t == 1: return None          # direct data: not supported
            if t: out.append((nm, 1 if t == 2 else 2))
        for i in range(8):
            t = (hi >> (2 * i)) & 3
            if t == 1: return None
            if t: out.append(('t%d' % i, 1 if t == 2 else 2))
        return out

    FMT = {0: ('B', 1), 1: ('b', 1), 2: ('H', 2), 3: ('h', 2), 4: ('f', 4)}

    def read(self, cpreg_base, cpreg_stride, cp, idx, fmt, count, shift):
        o = self.off(cp.get(cpreg_base, 0))
        if o is None: return None
        o += idx * cp.get(cpreg_stride, 0)
        c, s = self.FMT.get(fmt, ('h', 2))
        if o + count * s > len(self.d): return None
        vals = struct.unpack_from('>' + c * count, self.d, o)
        return [v if fmt == 4 else v / (1 << shift) for v in vals]

    def to_mesh(self, draws):
        V = []; N = []; T = []; F = []
        for prim, fmt, verts, cp in draws:
            vat_a = cp.get(0x70 + fmt, 0)
            posfmt = (vat_a >> 1) & 7; posshift = (vat_a >> 4) & 31
            nrmfmt = (vat_a >> 10) & 7; texfmt = (vat_a >> 22) & 7; texshift = (vat_a >> 25) & 31
            nshift = 14 if nrmfmt in (2, 3) else 6
            idx = []
            for v in verts:
                if 'pos' not in v: return None
                p = self.read(0xA0, 0xB0, cp, v['pos'], posfmt, 3, posshift)
                if p is None: return None
                n = self.read(0xA1, 0xB1, cp, v['nrm'], nrmfmt, 3, nshift) if 'nrm' in v else None
                t = self.read(0xA4, 0xB4, cp, v['t0'], texfmt, 2, texshift) if 't0' in v else None
                V.append(p); N.append(n or [0, 0, 1]); T.append(t or [0, 0]); idx.append(len(V) - 1)
                if 'mtx0' in v and v['mtx0'] == 0 and len(verts) == 3:
                    pass
            if prim == 0x90:
                F += [(idx[i], idx[i + 1], idx[i + 2]) for i in range(0, len(idx) - 2, 3)]
            elif prim == 0x98:
                for i in range(len(idx) - 2):
                    a, b, c = idx[i], idx[i + 1], idx[i + 2]
                    F.append((a, b, c) if i % 2 == 0 else (b, a, c))
            elif prim == 0xA0:
                F += [(idx[0], idx[i], idx[i + 1]) for i in range(1, len(idx) - 1)]
            elif prim == 0x80:
                for i in range(0, len(idx) - 3, 4):
                    a, b, c, e = idx[i:i + 4]; F += [(a, b, c), (a, c, e)]
        F = [f for f in F if len(set(f)) == 3]
        if not F: return None
        return np.array(V, float), np.array(N, float), np.array(T, float), np.array(F, int)

def find_streams(d):
    """GX streams: optional array-base CP writes, then the vertex descriptor (CP 0x50/0x60)."""
    starts = []
    for m in re.finditer(rb'\x08\x50....\x08\x60', d, re.S):
        s = m.start()
        while s >= 6 and d[s - 6] == 0x08 and d[s - 5] in (0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xB0, 0xB1, 0xB2, 0xB4):
            s -= 6
        starts.append(s)
    return sorted(set(starts))

# ----------------------------------------------------------------------------- skeletons
def find_skeletons(d):
    """Skinned parts keep two arrays of 4x3 matrices per bone; the first array is the
    inverse bind matrix of each joint. Returns {name: [inverse-bind 4x3 rows,...]}."""
    out = {}
    for m in re.finditer(rb'([A-Za-z0-9_]+_geoA)\x00', d):
        o = m.end(); lim = min(len(d) - 48, o + 0x40000)
        def isrot(p):
            with np.errstate(all='ignore'):
                f = np.frombuffer(d[p:p + 48], '>f4').astype(float)
            if not np.all(np.isfinite(f)) or np.abs(f).max() > 100: return False
            R = f[:9].reshape(3, 3); return np.allclose(R @ R.T, np.eye(3), atol=3e-3)
        p = (o + 3) & ~3
        while p < lim:
            if isrot(p) and isrot(p + 48) and isrot(p + 96):
                n = 0; q = p
                while q < len(d) - 48 and isrot(q): q += 48; n += 1
                nb = n // 2
                mats = [np.frombuffer(d[p + i * 48:p + i * 48 + 48], '>f4').astype(float).reshape(4, 3) for i in range(nb)]
                joints = []
                for M in mats:
                    X = np.eye(4); X[:3, :3] = M[:3]; X[:3, 3] = M[3]
                    W = np.linalg.inv(X); joints.append(dict(inverse_bind=X[:3].tolist(), bind_position=W[:3, 3].tolist()))
                out[m.group(1).decode()] = joints
                break
            p += 4
    return out

# ----------------------------------------------------------------------------- writers
def write_obj(path, groups):
    """groups: list of (group_name, V, N, T, F). Each group becomes an OBJ group."""
    with open(path, 'w') as f:
        f.write('# extracted by sir_decoder.py\n')
        base = 0
        for gname, V, N, T, F in groups:
            f.write(f'g {gname}\n')
            for v in V: f.write('v %.6f %.6f %.6f\n' % tuple(v))
            for t in T: f.write('vt %.6f %.6f\n' % (t[0], 1 - t[1]))
            for n in N:
                l = np.linalg.norm(n) or 1; f.write('vn %.5f %.5f %.5f\n' % tuple(np.asarray(n) / l))
            for a, b, c in F + 1 + base: f.write(f'f {a}/{a}/{a} {b}/{b}/{b} {c}/{c}/{c}\n')
            base += len(V)

# ----------------------------------------------------------------------------- game database
# global_binary.sir: section 0 is a 4 KB block of pointers into section 1, a memory image the Wii
# places at 0x90400000. Header slot 0xDC8 points at the game database object; its word +0xC points
# at 42 table descriptors of 0x428 bytes. Each descriptor describes its own rows:
#   +0x00 two build-time addresses (stale) +0x08 rows pointer   +0x0C row count   +0x10 1
#   +0x14 layout hash   +0x18 row size     +0x1C row count again
#   +0x20 u32 type[64]  +0x120 u32 size[64]  +0x220 u32 column-name hash[64]
#   +0x320 u32 hash of the table an id column refers to, or -1 [64]
#   +0x420 field count  +0x424 u8 present
# Type codes: 3 = s32, 4 = f32, 5 = u32 hash, 6 = s16, 7 = u8 flag, 8 = u8, 11 = s32 (always -1).
# Fields are packed in order, each aligned to its size; rows are padded to 4 bytes. Unused memory
# is filled with 0x7F800001. This reader is the reference the PC loader (src/port/sir_loader.cpp)
# is checked against (tools/loader_check.py), so it reads the big-endian bytes directly.
GB_BASE = 0x90400000
GB_TABLES = 42
GB_DESC = 0x428
GB_FMT = {3: '>i', 4: '>I', 5: '>I', 6: '>h', 7: 'B', 8: 'B', 11: '>i'}
GB_HDR_PTRS = (0x10, 0x418, 0x824, 0x828, 0x830, 0x83C, 0x844, 0xD94, 0xD98, 0xD9C,
               0xDA8, 0xDB0, 0xDB4, 0xDB8, 0xDBC, 0xDC0, 0xDC4, 0xDC8)
GB_HDR_WORDS = (0x8, 0x82C, 0x838, 0x848, 0x85C, 0x860, 0x864, 0x86C, 0x870, 0x874, 0x878, 0x960, 0xD8C)

def gamedb_tables(sec, db):
    """[(descriptor fields dict, rows as lists of raw values)] for the 42 tables."""
    u = lambda o: struct.unpack_from('>I', sec, o)[0]
    data = u(db - GB_BASE + 0xC) - GB_BASE
    out = []
    for t in range(GB_TABLES):
        D = data + t * GB_DESC
        n = u(D + 0x420)
        tb = dict(fn=(u(D), u(D + 4)), rows=u(D + 8), count=u(D + 0xC), layout=u(D + 0x14), rowsize=u(D + 0x18),
                  present=sec[D + 0x424],
                  types=[u(D + 0x20 + i * 4) for i in range(n)], sizes=[u(D + 0x120 + i * 4) for i in range(n)],
                  names=[u(D + 0x220 + i * 4) for i in range(n)], refs=[u(D + 0x320 + i * 4) for i in range(n)])
        rows = []
        for i in range(tb['count']):
            o = tb['rows'] - GB_BASE + i * tb['rowsize']; row = []
            for ty, sz in zip(tb['types'], tb['sizes']):
                o = (o + sz - 1) // sz * sz
                row.append(struct.unpack_from(GB_FMT[ty], sec, o)[0]); o += sz
            rows.append(row)
        out.append((tb, rows))
    return out

def gamedb_dump(path):
    """Text dump of the header slots and every game database value (one line per row)."""
    version, secs = read_archive(path)
    if len(secs) != 2 or secs[0]['size'] != 0x1000:
        raise SystemExit(f'{path}: not global_binary.sir (expected a 4 KB header section and an image)')
    hdr, sec = secs[0]['data'], secs[1]['data']
    u = lambda d, o: struct.unpack_from('>I', d, o)[0]
    def ptr(v):
        if v == 0: return 'null'
        if GB_BASE <= v <= GB_BASE + len(sec): return 'img+0x%x' % (v - GB_BASE)
        return 'wii:%08x' % v
    L = []
    for o in GB_HDR_PTRS: L.append('hdrptr 0x%03x %s' % (o, ptr(u(hdr, o))))
    for o in GB_HDR_WORDS: L.append('hdrword 0x%03x %08x' % (o, u(hdr, o)))
    for o in (0x820, 0x834, 0x840): L.append('hdrbyte 0x%03x %d' % (o, hdr[o]))
    db = u(hdr, 0xDC8)
    L.append('gamedb %s data %s' % (ptr(db), ptr(u(sec, db - GB_BASE + 0xC))))
    tables = gamedb_tables(sec, db)
    for t, (tb, rows) in enumerate(tables):
        n = len(tb['types'])
        line = 'table %d rows %s build %08x %08x count %d rowsize %d fields %d present %d' % (
            t, ptr(tb['rows']), tb['fn'][0], tb['fn'][1], tb['count'], tb['rowsize'] if tb['count'] else 0, n, tb['present'])
        if tb['count']: line += ' layout %08x' % tb['layout']
        L.append(line)
        if not tb['count']: continue
        for i in range(n):
            L.append('field %d %d type %d size %d name %08x ref %08x' % (t, i, tb['types'][i], tb['sizes'][i], tb['names'][i], tb['refs'][i]))
        pre = {4: 'f%08x', 5: 'h%08x'}
        for i, row in enumerate(rows):
            L.append('row %d %d ' % (t, i) + ' '.join(pre.get(ty, '%d') % v for ty, v in zip(tb['types'], row)))
    # Named views, by the offsets the game code uses (include/game/part.h).
    parts, actions, types, tuning = tables[22][1], tables[1][1], tables[0][1], tables[37][1]
    for i, r in enumerate(parts):
        L.append('part %d index %d tiers %08x %08x %08x %08x action1 %d slot %d discipline %d action2 %d' % (
            i, r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[11]))
    for i, r in enumerate(actions):
        L.append('action %d key %d type %d name %08x' % (i, r[0] & 0xFFFFFFFF, r[1], types[r[1]][4]))
    for i, r in enumerate(tuning):
        L.append('tuning %d id %d value %.9g' % (i, r[0], struct.unpack('>f', struct.pack('>I', r[1]))[0]))
    return L

# ----------------------------------------------------------------------------- global_binary image objects
# Hand-typed objects of the global_binary.sir image (docs/loader.md): the input table (header
# slot 0xDC4), the texture list at the start of the image (0xD94), the string table (0xDA8), the
# action table (0x844), the UI root and its objects (0xDB0-0xDBC) and the screen state machines
# (0xD98, 0xD9C). Reference for the PC loader, read straight from the big-endian bytes.
INPUT_STATE = {1: 'f', 2: 'f', 9: 'f', 5: 'ffff', 6: 'ffff', 7: 'ifff', 8: 'ifff'}   # by step type, at +0x50

def fnv(b):
    h = 0x811C9DC5
    for c in b:
        h = ((h ^ c) * 0x01000193) & 0xFFFFFFFF
    return h

def machines_dump(L, sec, base, hud_addr, list_addr, level=False, extra=()):
    """Appends the dump of a HUD and its state machines (global_binary.sir, or a level zone: there
    the HUD carries the screen-list slot at +0x434 and points at it from +0x24)."""
    u = lambda o: struct.unpack_from('>I', sec, o)[0]
    i32 = lambda o: struct.unpack_from('>i', sec, o)[0]
    def ptr(v):
        if v == 0: return 'null'
        if base <= v <= base + len(sec): return 'img+0x%x' % (v - base)
        return 'wii:%08x' % v
    off = lambda v: v - base
    ws = lambda o, n: ' '.join('%08x' % u(o + i * 4) for i in range(n))
    ps = lambda o, n: ' '.join(ptr(u(o + i * 4)) for i in range(n))
    hud = off(hud_addr)
    count = u(hud + 0x428)
    L.append('hud %s depth %d stack %s %s count %d current %s flag %d spare %d' % (
        ptr(hud_addr), i32(hud), ps(hud + 4, 8), ('slot ' + ptr(u(hud + 0x24))) if level else 'w24 %08x' % u(hud + 0x24),
        count, ptr(u(hud + 0x42C)), sec[hud + 0x430], sum(1 for i in range(count, 256) if u(hud + 0x28 + i * 4))) +
        (' list %s' % ptr(list_addr) if level else ''))
    machines = [list_addr] if list_addr else []
    for i in range(count):
        p = u(hud + 0x28 + i * 4)
        L.append('hudmachine %d %s' % (i, ptr(p)))
        if p and p not in machines: machines.append(p)
    for p in extra:
        if p not in machines: machines.append(p)
    defs, trans, shared_trans = [], set(), set()
    for p in machines:
        o = off(p)
        L.append('fsm %s def %s state %d %d %d owner %s words %s floats %s values %s raw %s' % (
            ptr(p), ptr(u(o)), i32(o + 4), i32(o + 8), i32(o + 0xC), ptr(u(o + 0x10)), ws(o + 0x14, 6), ws(o + 0x2C, 10),
            ptr(u(o + 0x54)), sec[o + 0x58:o + 0x70].hex()))
        v = u(o + 0x54)
        if v and base <= v < base + len(sec):       # in a level file the table can lie in another zone
            n = u(off(v))
            L.append('fsmvalues %s count %d sum %08x' % (ptr(v), n, fnv(sec[off(v) + 4:off(v) + 4 + n * 4])))
        if u(o) and u(o) not in defs: defs.append(u(o))
    for d in defs:
        o = off(d)
        ns, nn, nr = i32(o + 4), i32(o + 0x1C), i32(o + 0x2C)
        L.append('fsmdef %s states %s %d strings %s nodes %s %d shared %s %d key %08x start %08x %08x %d %s b44 %d raw %s %s %s' % (
            ptr(d), ptr(u(o)), ns, ptr(u(o + 0xC)), ptr(u(o + 0x10)), nn, ptr(u(o + 0x20)), nr, u(o + 0x30), u(o + 0x34),
            u(o + 0x38), u(o + 0x3C), ptr(u(o + 0x40)), sec[o + 0x44], sec[o + 8:o + 0xC].hex(), sec[o + 0x14:o + 0x1C].hex(),
            sec[o + 0x24:o + 0x2C].hex()))
        trans.add(u(o + 0x40))
        for i in range(ns):
            s = off(u(o)) + i * 0x30
            ne = i32(s + 0x2C)
            L.append('fsmstate %s %d vtable %08x key %08x type %d actions %s timeout %08x flags %08x events %s %d raw %s' % (
                ptr(d), i, u(s), u(s + 4), u(s + 8), ws(s + 0xC, 3), u(s + 0x18), u(s + 0x1C), ptr(u(s + 0x20)), ne,
                sec[s + 0x24:s + 0x2C].hex()))
            for k in range(ne):
                e = off(u(s + 0x20)) + k * 12
                L.append('fsmevent %s %d %d id %08x cond %08x to %s' % (ptr(d), i, k, u(e), u(e + 4), ptr(u(e + 8))))
                trans.add(u(e + 8))
        for i in range(nn):
            np_ = u(off(u(o + 0x10)) + i * 4)
            n = off(np_)
            typ = u(n + 8)
            line = 'fsmnode %s vtable %08x key %08x type %d' % (ptr(np_), u(n), u(n + 4), typ)
            if typ == 2:
                line += ' to %s' % ptr(u(n + 0xC)); trans.add(u(n + 0xC))
            elif typ == 3:
                line += ' action %08x to %s %d %d' % (u(n + 0xC), ptr(u(n + 0x10)), i32(n + 0x14), i32(n + 0x18)); trans.add(u(n + 0x10))
            elif typ == 4:
                c = i32(n + 0x14)
                line += ' action %08x choices %s %d result %d %d to %s' % (u(n + 0xC), ptr(u(n + 0x10)), c, i32(n + 0x1C), i32(n + 0x20),
                                                                         ps(off(u(n + 0x10)), c) if c else '')
                for k in range(c): trans.add(u(off(u(n + 0x10)) + k * 4))
            L.append(line.rstrip())
        for i in range(nr):
            e = off(u(o + 0x20)) + i * 12
            L.append('fsmshared %s %d id %08x to %s cond %08x' % (ptr(d), i, u(e), ptr(u(e + 4)), u(e + 8)))
            shared_trans.add(u(e + 4))
    trans.discard(0); shared_trans.discard(0)
    for p in sorted(trans):
        o = off(p)
        L.append('fsmtrans %s vtable %08x target %s cond %08x action %08x %d %d raw %s' % (
            ptr(p), u(o), ptr(u(o + 4)), u(o + 8), u(o + 0xC), i32(o + 0x10), i32(o + 0x14), sec[o + 0x18:o + 0x20].hex()))
    for p in sorted(shared_trans):
        o = off(p)
        n = i32(o + 0x20)
        L.append(('fsmsharedtrans %s vtable %08x target %s cond %08x action %08x %d %d flags %08x keys %s %d %s' % (
            ptr(p), u(o), ptr(u(o + 4)), u(o + 8), u(o + 0xC), i32(o + 0x10), i32(o + 0x14), u(o + 0x18), ptr(u(o + 0x1C)), n,
            ws(off(u(o + 0x1C)), n) if n else '')).rstrip())


# ----------------------------------------------------------------------------- level files
# A level file: the header block (section type -1) is the world state; section type N is the image
# of zone N. Reference for the PC loader (LevelHeaderConvert, LevelZoneConvert): the fields the
# game code reads, straight from the big-endian bytes. See docs/loader.md, "Level files".
LEVEL_SLOTS = 0x21E0

def level_dump(path):
    version, secs = read_archive(path)
    hdr = [x for x in secs if x['type'] == -1][0]['data']
    zone0 = [x for x in secs if x['type'] == 0][0]['data']
    u = lambda o: struct.unpack_from('>I', hdr, o)[0]
    i32 = lambda o: struct.unpack_from('>i', hdr, o)[0]
    base = u(0x1A4) - LEVEL_SLOTS                    # the block points at its own +0x21E0 (fn_80129CC8)
    count = u(0x1A8)
    first = u(LEVEL_SLOTS + 0x114)
    stride = 0x148
    while count > 1 and u(LEVEL_SLOTS + stride + 0x114) != first + stride: stride += 4
    zbase = u(LEVEL_SLOTS + 0xFC)                    # slot 0 points at the start of zone 0's image (WorldTick)
    def ptr(v):
        if v == 0: return 'null'
        if base <= v <= base + len(hdr): return 'hdr+0x%x' % (v - base)
        if zbase <= v <= zbase + len(zone0): return 'zone0+0x%x' % (v - zbase)
        return 'wii:%08x' % v
    ws = lambda o, n: ' '.join('%08x' % u(o + i * 4) for i in range(n))
    tab = lambda o: '%s %d %d' % (ptr(u(o)), i32(o + 4), i32(o + 8))
    def entries(o):                                  # non-null entries of a pointer table {ptr, capacity, used}
        t, n = u(o) - base, u(o + 4)
        return ' '.join('%d:%s' % (i, ptr(u(t + i * 4))) for i in range(n) if u(t + i * 4)) or '-'
    L = ['levelhdr base %08x size %d zones %d slot %d last %d current %s %s %s %s lists %s %d %s %d results %d %s %d %d %s %d' % (
            base, len(hdr), count, stride, i32(4), ptr(u(8)), ptr(u(0xC)), ptr(u(0x10)), ptr(u(0x14)), ptr(u(0x2C)), i32(0x30),
            ptr(u(0x38)), i32(0x3C), i32(0x7C), ptr(u(0x80)), hdr[0x84], i32(0x88), ptr(u(0x8C)), hdr[0x90]),
         'leveltables %s | %s | %s | %s | %s' % tuple(tab(o) for o in (0x1A4, 0x1B0, 0x1BC, 0x1C8, 0x1D4)),
         'levelstate %s b1ec %d w1f0 %d hud %s floats %s wa20 %d w1a7c %d floats %s' % (
            ws(0x1E0, 3), hdr[0x1EC], i32(0x1F0), ptr(u(0x1F4)), ws(0x1F8, 6), i32(0xA20), i32(0x1A7C), ws(0x1A84, 2))]
    for t in (0x1B0, 0x1BC, 0x1C8):
        L.append('leveltable 0x%x %s' % (t, entries(t)))
    for i in range(count):
        o = LEVEL_SLOTS + i * stride
        L.append('levelslot %d floats %s w18 %d floats %s res %s lists %s %s %d %d words %s %s %s %s bytes %d %d %d actions %s pools %s %d zone %s w100 %08x w10c %08x tables %s | %s | %s e120 %s e12c %s' % (
            i, ws(o, 6), i32(o + 0x18), ws(o + 0x1C, 3), ptr(u(o + 0x50)), ptr(u(o + 0x8C)), ptr(u(o + 0x90)), i32(o + 0x94), i32(o + 0x98),
            ws(o + 0xA0, 1), ws(o + 0xA8, 1), ws(o + 0xBC, 1), ws(o + 0xCC, 2), hdr[o + 0xA4], hdr[o + 0xAC], hdr[o + 0xC0],
            ptr(u(o + 0xD4)), ptr(u(o + 0xE4)), i32(o + 0xE8), ptr(u(o + 0xFC)), u(o + 0x100), u(o + 0x10C), tab(o + 0x114), tab(o + 0x120), tab(o + 0x12C),
            entries(o + 0x120), entries(o + 0x12C)))
    # What the header block names in zone 0: entities (slot +0x50 and the two entity tables), scene
    # objects (block tables +0x1B0, +0x1BC) and pool tables (slot +0xE4, +0xE8).
    inz = lambda v: zbase <= v < zbase + len(zone0)
    roots, scene, pooltabs, records = [], [], [], []
    def table_entries(o):
        t, n = u(o) - base, u(o + 4)
        return [u(t + i * 4) for i in range(n)]
    for t in (0x1B0, 0x1BC):
        scene += [v for v in table_entries(t) if inz(v)]
    for i in range(count):
        o = LEVEL_SLOTS + i * stride
        roots += [v for v in [u(o + 0x50)] + table_entries(o + 0x120) + table_entries(o + 0x12C) if inz(v)]
        if inz(u(o + 0xE4)): pooltabs.append((u(o + 0xE4), u(o + 0xE8)))
        if inz(u(o + 0xFC)): records.append(u(o + 0xFC))
    hud = u(0x1F4)
    zone_dump(L, zone0, zbase, hud if inz(hud) else 0, roots, scene, pooltabs, records)
    return L

FIXUP_ENTITY, FIXUP_PART, FIXUP_POOLED, FIXUP_PROJECTILE, FIXUP_RENDER, FIXUP_FSM = 651, 652, 653, 654, 655, 656

def zone_records_dump(L, sec, base, records):
    """The zone records (zone slot +0xFC) and what WorldTick reaches through their list at +0x10."""
    u = lambda o: struct.unpack_from('>I', sec, o)[0]
    inz = lambda v, n=1: base <= v and v + n <= base + len(sec)
    def ptr(v):
        if v == 0: return 'null'
        if base <= v <= base + len(sec): return 'img+0x%x' % (v - base)
        return 'wii:%08x' % v
    ws = lambda o, n: ' '.join('%08x' % u(o + i * 4) for i in range(n))
    ps = lambda o, n: ' '.join(ptr(u(o + i * 4)) for i in range(n))
    players, tracks = set(), set()
    def player(a):
        if not inz(a, 0x28) or a in players: return
        players.add(a)
        o = a - base
        mode, elems, count, stride = u(o + 4), u(o + 8), u(o + 0x10), u(o + 0x18)
        L.append('animplayer %s mode %d elems 0x%x count %d track %s stride %d flags %d %d time %08x keys %s' % (
            ptr(a), mode, elems, count, ptr(u(o + 0x14)), stride, u(o + 0x1C), u(o + 0x20), u(o + 0x24), ps(o + 0x28, count)))
        if mode != 1: return
        for i in range(count):
            e = o + elems + i * stride
            L.append('animelem %s %08x %08x' % (ptr(base + e), u(e), u(e + 8)))
        t = u(o + 0x14)
        if not inz(t, 0x28) or t in tracks: return
        tracks.add(t)
        to = t - base
        table = to + u(to + 0x24)
        L.append('animtrack %s length %08x table 0x%x' % (ptr(t), u(to + 0x20), u(to + 0x24)))
        for i in range(count):
            first = to + u(table + i * 8)
            k, n, total = first, 0, 0
            while True:
                time, value = struct.unpack_from('>H', sec, k)[0], u(k + 4)
                n += 1; total = (total + time + value) & 0xFFFFFFFF
                if time == 0xFFFF: break
                k += 8
            L.append('animkeys %s rel 0x%x keys %d first %04x %08x last %08x sum %08x' % (
                ptr(base + first), u(table + i * 8), n, struct.unpack_from('>H', sec, first)[0], u(first + 4), u(k + 4), total))
    for rec in records:
        if not inz(rec, 0x14): continue
        node = u(rec - base + 0x10)
        L.append('zonerecord %s list %s' % (ptr(rec), ptr(node)))
        while inz(node - 0x10, 0x80):
            o = node - base - 0x10
            L.append('zonesystem %s w8 %s next %s out %s players %s extra %s' % (
                ptr(node - 0x10), ptr(u(o + 8)), ptr(u(o + 0x10)), ps(o + 0x3C, 8), ps(o + 0x5C, 8), ptr(u(o + 0x7C))))
            for k in range(8):
                pl, out = u(o + 0x5C + k * 4), u(o + 0x3C + k * 4)
                if not pl: continue
                if inz(out, 0x30): L.append('animout %s %s' % (ptr(out), ws(out - base, 12)))
                player(pl)
            player(u(o + 0x7C))
            node = u(o + 0x10)

def zone_dump(L, sec, base, hud, roots, scene_roots, pooltabs, records=()):
    """Dump of what the loader converts in a level zone: pool tables and pools, entities (first
    layer), scene objects, then the HUD and every state machine found on the way."""
    u = lambda o: struct.unpack_from('>I', sec, o)[0]
    i32 = lambda o: struct.unpack_from('>i', sec, o)[0]
    h16 = lambda o: struct.unpack_from('>H', sec, o)[0]
    inz = lambda v, n=1: base <= v and v + n <= base + len(sec)
    off = lambda v: v - base
    def ptr(v):
        if v == 0: return 'null'
        if base <= v <= base + len(sec): return 'img+0x%x' % (v - base)
        return 'wii:%08x' % v
    ws = lambda o, n: ' '.join('%08x' % u(o + i * 4) for i in range(n))
    ents, scene, machines, projectiles = [], [], [], []
    def add(lst, v, n=1):
        if v and inz(v, n) and v not in lst: lst.append(v)
    for v in roots: add(ents, v)
    for v in scene_roots: add(scene, v)
    for addr, n in pooltabs:
        for i in range(n):
            e = off(addr) + i * 0x2C
            pool, fix = u(e + 4), i32(e + 0xC)
            L.append('pooltable %s %d pool %s fixup %d type %d' % (ptr(addr), i, ptr(pool), fix, i32(e + 0x1C)))
            if not inz(pool, 0x2C): continue
            q = off(pool)
            items, size, align, block = u(q), u(q + 4), u(q + 8), u(q + 0x24)
            L.append('levelpool %s items %d size %d align %d used %d free %s inuse %s parent %s blocks %s grow %d block %s alloc %d' % (
                ptr(pool), items, size, align, u(q + 0xC), ptr(u(q + 0x10)), ptr(u(q + 0x14)), ptr(u(q + 0x18)), ptr(u(q + 0x1C)),
                sec[q + 0x20], ptr(block), i32(q + 0x28)))
            first = ((block + 4 + align + 7) & ~(align - 1)) - 8
            links = b''.join(sec[off(first) + k * (size + 8):off(first) + k * (size + 8) + 8] for k in range(items))
            L.append('levelpoolnodes %s first %s sum %08x' % (ptr(pool), ptr(first), fnv(links)))
            for k in range(items):
                item = first + k * (size + 8) + 8
                if fix in (FIXUP_ENTITY, FIXUP_PART, FIXUP_PROJECTILE): add(ents, item)
                if fix == FIXUP_PROJECTILE: projectiles.append(item)
                if fix == FIXUP_POOLED: add(ents, u(off(item)))
                if fix == FIXUP_RENDER: L.append('renderslot %s %s' % (ptr(item), ptr(u(off(item)))))
                if fix == FIXUP_FSM:
                    L.append('fsmslot %s %s' % (ptr(item), ptr(u(off(item)))))
                    add(machines, u(off(item)), 0x58)
    def parts(c):
        if not inz(c, 0x14): return
        o = off(c)
        n = u(o + 0x10)
        L.append('entparts %s items %s %d' % (ptr(c), ptr(u(o + 0xC)), n))
        for i in range(n):
            it = u(off(u(o + 0xC)) + i * 4)
            if not inz(it, 0x2C): continue
            m = u(off(it) + 0x28)
            L.append('entitem %s parts %s %d' % (ptr(it), ptr(u(off(it) + 0x24)), m))
            for k in range(m):
                pp = u(off(u(off(it) + 0x24)) + k * 4)
                if inz(pp, 0x10): L.append('entpart %s %08x %08x kind %d' % (ptr(pp), u(off(pp)), u(off(pp) + 4), u(off(pp) + 0xC)))
    i = 0
    while i < len(ents):
        a = ents[i]; i += 1
        if not inz(a, 0x1B4): continue
        o = off(a)
        L.append('entity %s vtable %08x record %s parts %s scene %s w94 %d %d obj9c %s holder %s children %s f158 %08x more %s %d w1a8 %d flags %08x' % (
            ptr(a), u(o), ptr(u(o + 4)), ptr(u(o + 0x8C)), ptr(u(o + 0x90)), i32(o + 0x94), i32(o + 0x98), ptr(u(o + 0x9C)), ptr(u(o + 0xA4)),
            ptr(u(o + 0xB0)), u(o + 0x158), ptr(u(o + 0x194)), i32(o + 0x1A4), i32(o + 0x1A8), u(o + 0x1B0)))
        r = u(o + 4)
        if inz(r, 0x40):
            q = off(r)
            L.append('entrecord %s floats %s w1c %08x %08x f24 %08x w2c %d h30 %d bytes %s' % (
                ptr(r), ws(q, 7), u(q + 0x1C), u(q + 0x20), u(q + 0x24), i32(q + 0x2C), h16(q + 0x30), sec[q + 0x34:q + 0x3E].hex()))
        L.append('entbase %s id %08x w30 %08x p84 %s wac %08x' % (ptr(a), u(o + 0x2C), u(o + 0x30), ptr(u(o + 0x84)), u(o + 0xAC)))
        prop = inz(r, 0x40) and sec[off(r) + 0x34] == 6
        if prop:
            L.append('entprop %s f14 %08x f28 %08x acts %s %s %08x actor %s fc8 %s wec %08x w15c %08x' % (
                ptr(a), u(o + 0x14), u(o + 0x28), ws(o + 0x38, 3), ws(o + 0x48, 4), u(o + 0x68), ptr(u(o + 0xA0)), ws(o + 0xC8, 3),
                u(o + 0xEC), u(o + 0x15C)))
        parts(u(o + 0x8C))
        add(scene, u(o + 0x90), 8)
        if inz(u(o + 0x9C), 0xC): L.append('entobj9c %s id %08x' % (ptr(u(o + 0x9C)), u(off(u(o + 0x9C)) + 8)))
        if prop and inz(u(o + 0x9C), 0xC0):
            q = off(u(o + 0x9C))
            L.append('entdef %s f24 %08x w2c %08x acts %s %s %08x wbc %08x' % (
                ptr(u(o + 0x9C)), u(q + 0x24), u(q + 0x2C), ws(q + 0x68, 3), ws(q + 0x78, 4), u(q + 0x98), u(q + 0xBC)))
        hl = u(o + 0xA4)
        if inz(hl, 8):
            L.append('entholder %s machine %s other %s' % (ptr(hl), ptr(u(off(hl))), ptr(u(off(hl) + 4))))
            add(machines, u(off(hl)), 0x58)
        n = u(o + 0x1A4)
        if n and inz(u(o + 0x194), n * 4):
            for k in range(n): parts(u(off(u(o + 0x194)) + k * 4))
        node, seen = u(o + 0xB0), set()
        while inz(node, 0xC) and node not in seen:
            seen.add(node)
            L.append('entchild %s entity %s next %s' % (ptr(node), ptr(u(off(node) + 4)), ptr(u(off(node) + 8))))
            add(ents, u(off(node) + 4))
            node = u(off(node) + 8)
    for a in scene:
        o = off(a)
        t = u(o + 4)
        line = 'sceneobj %s type %d' % (ptr(a), t)
        if t == 2: line += ' a %s list %s' % (ptr(u(o + 0x48)), ptr(u(o + 0x4C)))
        if t == 3: line += ' count %d items %s' % (i32(o + 0x54), ptr(u(o + 0x58)))
        L.append(line)
    for a in projectiles:
        L.append('projectile %s part %s' % (ptr(a), ptr(u(off(a) + 0x22C))))
    zone_records_dump(L, sec, base, records)
    if hud:
        machines_dump(L, sec, base, hud, u(off(hud) + 0x434), True, machines)

def image_dump(path):
    """Text dump of the hand-typed image objects of global_binary.sir (one line per record)."""
    version, secs = read_archive(path)
    if len(secs) != 2 or secs[0]['size'] != 0x1000:
        raise SystemExit(f'{path}: not global_binary.sir (expected a 4 KB header section and an image)')
    hdr, sec = secs[0]['data'], secs[1]['data']
    u = lambda o: struct.unpack_from('>I', sec, o)[0]
    i32 = lambda o: struct.unpack_from('>i', sec, o)[0]
    h16 = lambda o: struct.unpack_from('>H', sec, o)[0]
    slot = lambda o: struct.unpack_from('>I', hdr, o)[0]
    def ptr(v):
        if v == 0: return 'null'
        if GB_BASE <= v <= GB_BASE + len(sec): return 'img+0x%x' % (v - GB_BASE)
        return 'wii:%08x' % v
    cstr0 = lambda o: sec[o:sec.index(b'\0', o)]
    L = []

    # Input table: 8 sequences at +0, `count` more at +0x240, s32 index[34] at +0x1E60.
    t = slot(0xDC4) - GB_BASE
    count = i32(t + 0x1EE8)
    L.append('input %s sequences %d' % (ptr(slot(0xDC4)), count))
    L.append('inputindex ' + ' '.join('%d' % i32(t + 0x1E60 + i * 4) for i in range(34)))
    L.append('inputtail ' + ' '.join(ptr(u(t + 0x1EEC + i * 4)) for i in range(6)) +
             ' %d %08x %d %d %08x %08x %d' % (i32(t + 0x1F04), u(t + 0x1F0C), i32(t + 0x1F10), i32(t + 0x1F14),
                                              u(t + 0x1F18), u(t + 0x1F1C), i32(t + 0x1F20)))
    steps = []
    for i in range(8 + count):
        s = t + i * 0x48 if i < 8 else t + 0x240 + (i - 8) * 0x48
        n = i32(s)
        L.append('inputseq %d steps %d current %d value %08x %08x time %08x %08x %08x timing %d ptrs %s' % (
            i, n, i32(s + 4), u(s + 8), u(s + 0xC), u(s + 0x38), u(s + 0x3C), u(s + 0x40), i32(s + 0x44),
            ' '.join(ptr(u(s + 0x10 + k * 4)) for k in range(10))))
        for k in range(n):
            p = u(s + 0x10 + k * 4)
            if p and p not in steps: steps.append(p)
    for p in steps:
        o = p - GB_BASE
        typ, n = u(o + 0x48), i32(o + 8)
        line = 'inputstep %s type %d sources %d raw %08x value %08x mode %d range %08x %08x vtable %08x' % (
            ptr(p), typ, n, u(o), u(o + 4), i32(o + 0x3C), u(o + 0x40), u(o + 0x44), u(o + 0x4C))
        for k in range(n):
            line += ' src %d %08x %d' % (u(o + 0xC + k * 12), u(o + 0x10 + k * 12), u(o + 0x14 + k * 12))
        for k, c in enumerate(INPUT_STATE.get(typ, '')):
            line += (' %d' % i32(o + 0x50 + k * 4)) if c == 'i' else (' %08x' % u(o + 0x50 + k * 4))
        L.append(line)

    # Texture list at the start of the image: u32, then {head, tail, count}; nodes are at texture +0x28.
    t = slot(0xD94) - GB_BASE
    count = u(t + 0xC)
    L.append('texlist %s word %d head %s tail %s count %d' % (ptr(slot(0xD94)), u(t), ptr(u(t + 4)), ptr(u(t + 8)), count))
    node, texs = u(t + 4), []
    for i in range(count):
        o = node - 0x28 - GB_BASE
        texs.append(o)
        size, data, pal = u(o + 0x18), u(o + 0x1C), u(o + 0x20)
        L.append('texture %d %s hdr %d size %dx%d gx %dx%d bytes %s datasize %d data %s palette %s unk24 %d next %s prev %s list %s b34 %d name %s datasum %08x' % (
            i, ptr(node - 0x28), u(o), h16(o + 4), h16(o + 6), h16(o + 8), h16(o + 10), sec[o + 0xC:o + 0x18].hex(),
            size, ptr(data), ptr(pal), u(o + 0x24), ptr(u(o + 0x28)), ptr(u(o + 0x2C)), ptr(u(o + 0x30)), sec[o + 0x34],
            cstr0(o + 0x35).decode('latin1'), fnv(sec[data - GB_BASE:data - GB_BASE + size])))
        node = u(o + 0x28)
    for s in (0x824, 0x828):    # the two textures GameStart hands to the engine
        o = slot(s) - GB_BASE
        L.append('slottexture 0x%03x %d %s' % (s, texs.index(o) if o in texs else -1, cstr0(o + 0x35).decode('latin1')))

    # String table: {strings, ptr, build address, count, text}.
    t = slot(0xDA8) - GB_BASE
    count, arr = u(t + 0xC), u(t) - GB_BASE
    L.append('strings %s array %s unk04 %s build %08x count %d text %s' % (
        ptr(slot(0xDA8)), ptr(u(t)), ptr(u(t + 4)), u(t + 8), count, ptr(u(t + 0x10))))
    for i in range(count):
        p = u(arr + i * 4)
        L.append('string %d %s %08x' % (i, ptr(p), fnv(cstr0(p - GB_BASE)) if p else 0))

    # Action table: {count, records}.
    t = slot(0x844) - GB_BASE
    count, arr = u(t), u(t + 4) - GB_BASE
    L.append('actions %s count %d records %s' % (ptr(slot(0x844)), count, ptr(u(t + 4))))
    for i in range(count):
        L.append('actionrec %d %s' % (i, ptr(u(arr + i * 4))))
    # Action records: programs of 32-bit words for the interpreter fn_801E1BD4. An instruction is
    # {opcode bits 0-7, length in words bits 8-15, function index or jump distance bits 16-27} and
    # length - 1 operand words; opcode 0xFF ends the program, opcodes above 0x1B take one word.
    for i in range(count):
        p = u(arr + i * 4)
        if not p or (u(p - GB_BASE) & 0xFF) != 1:     # fn_801E2368: not run
            continue
        o = p - GB_BASE
        n = calls = jumps = 0
        while True:
            w = u(o + n * 4)
            op, ln = w & 0xFF, (w >> 8) & 0xFF
            if op == 0xFF: break
            calls += op == 0x16
            jumps += 4 <= op <= 0x11
            n += ln if op <= 0x1B else 1
        L.append('actionprog %d %s words %d calls %d jumps %d sum %08x' % (i, ptr(p), n + 1, calls, jumps, fnv(sec[o:o + (n + 1) * 4])))

    # UI root (slot 0xDB0), its fonts and the objects of slots 0xDB4, 0xDB8, 0xDBC. Only the fields
    # the game code reads are decoded (docs/loader.md lists what is left).
    off = lambda v: v - GB_BASE
    ws = lambda o, n: ' '.join('%08x' % u(o + i * 4) for i in range(n))
    ps = lambda o, n: ' '.join(ptr(u(o + i * 4)) for i in range(n))
    fontobjs = []
    def fontobj(p):
        if p and p not in fontobjs: fontobjs.append(p)
    t = off(slot(0xDB0))
    L.append('uiroot %s raw %s w10 %08x fonts %s view %s run %s %s bytes %02x %02x' % (
        ptr(slot(0xDB0)), sec[t:t + 0x10].hex(), u(t + 0x10), ps(t + 0x14, 3), ws(t + 0x20, 19), ws(t + 0x6C, 5),
        ws(t + 0x84, 4), sec[t + 0x80], sec[t + 0x94]))
    for i in range(3):
        f = u(t + 0x14 + i * 4)
        if not f: continue
        o = off(f)
        L.append('uifont %d %s name %s obj %s' % (i, ptr(f), cstr0(o + 4).decode('latin1'), ptr(u(o + 0xCC))))
        fontobj(u(o + 0xCC))
    t = off(slot(0xDB4))
    L.append('uifontlist %s %s' % (ptr(slot(0xDB4)), ps(t + 4, 2)))
    fontobj(u(t + 4)); fontobj(u(t + 8))
    for p in fontobjs:
        o = off(p)
        line = 'uifontobj %s vtable %08x kind %d data %s' % (ptr(p), u(o), u(o + 4), ptr(u(o + 8)))
        if u(o + 4) == 1:
            line += ' pages %d %s' % (u(o + 0x124), ps(o + 0x24, u(o + 0x124)))
        L.append(line.rstrip())
    t = off(slot(0xDB8))
    L.append('uidraw %s raw %s w04 %08x fonts %s raw0c %s w40 %d w404 %d' % (
        ptr(slot(0xDB8)), sec[t:t + 4].hex(), u(t + 4), ptr(u(t + 8)), sec[t + 0xC:t + 0x10].hex(), i32(t + 0x40), i32(t + 0x404)))
    t = off(slot(0xDBC))
    L.append('uipad %s raw %s %s' % (ptr(slot(0xDBC)), sec[t:t + 4].hex(), ws(t + 4, 7)))

    # Screen state machines: the screen list (slot 0xD98) and the HUD's machines (slot 0xD9C).
    machines_dump(L, sec, GB_BASE, slot(0xD9C), slot(0xD98))

    # Effect definitions (header slots 0x82C-0x840): 0x70-byte and 0x80-byte entries, as the effect
    # code reads them through an instance's definition pointer.
    hb = lambda o: struct.unpack_from('>I', hdr, o)[0]
    n, t = hb(0x82C), slot(0x830)
    L.append('fxbolts %s count %d sorted %d' % (ptr(t), n, hdr[0x834]))
    for i in range(n):
        e = off(t) + i * 0x70
        L.append('fxbolt %d hash %08x name %s tex %s words %s %s floats %s %08x %08x owner %08x pool %s end %s raw %s %s %s %s' % (
            i, u(e), cstr0(e + 8).decode('latin1'), ptr(u(e + 0x28)), ws(e + 0x2C, 3), ws(e + 0x3C, 2), ws(e + 0x44, 4),
            u(e + 0x54), u(e + 0x5C), u(e + 0x64), ptr(u(e + 0x68)), ptr(u(e + 0x6C)), sec[e + 4:e + 8].hex(),
            sec[e + 0x38:e + 0x3C].hex(), sec[e + 0x58:e + 0x5C].hex(), sec[e + 0x60:e + 0x64].hex()))
    n, t = hb(0x838), slot(0x83C)
    L.append('fxtrails %s count %d sorted %d' % (ptr(t), n, hdr[0x840]))
    for i in range(n):
        e = off(t) + i * 0x80
        L.append('fxtrail %d hash %08x colour %08x name %s words %s floats %s w4c %d f50 %08x w54 %d floats %s owner %08x tex %s mode %d pools %s raw %s' % (
            i, u(e), u(e + 4), cstr0(e + 8).decode('latin1'), ws(e + 0x28, 2), ws(e + 0x30, 7), i32(e + 0x4C), u(e + 0x50),
            i32(e + 0x54), ws(e + 0x58, 3), u(e + 0x6C), ptr(u(e + 0x70)), i32(e + 0x74), ps(e + 0x78, 2), sec[e + 0x64:e + 0x6C].hex()))

    # Object pools (header slots 0x10 and 0x418) and the nodes of their block, placed as
    # fn_801EBAB8 places them: first node where an item lands on the pool's alignment.
    for s in (0x10, 0x418):
        if not slot(s): continue
        p = off(slot(s))
        count, size, align, block = u(p), u(p + 4), u(p + 8), u(p + 0x24)
        L.append('pool 0x%03x %s items %d size %d align %d used %d free %s inuse %s parent %s blocks %s grow %d block %s alloc %d next %s' % (
            s, ptr(slot(s)), count, size, align, u(p + 0xC), ptr(u(p + 0x10)), ptr(u(p + 0x14)), ptr(u(p + 0x18)), ptr(u(p + 0x1C)),
            sec[p + 0x20], ptr(block), i32(p + 0x28), ptr(u(off(block)))))
        first = ((block + 4 + align + 7) & ~(align - 1)) - 8
        for i in range(count):
            a = first + i * (size + 8)
            L.append('poolnode 0x%03x %d %s prev %s next %s' % (s, i, ptr(a), ptr(u(off(a))), ptr(u(off(a) + 4))))
    return L

# ----------------------------------------------------------------------------- catalog
# catalog.sir: container header (+0x08 entry count, +0x0C file count), then one 0x18-byte entry
# per section of every .sir file (i32 type, u32 offset, u32 size, u32 file-name hash,
# u32 packed size or 0, u32 hash), then one 0x40-byte record per file (u32 name hash, u32, name).
def catalog_read(path):
    d = open(path, 'rb').read()
    magic, version, n, nfiles, h = struct.unpack_from('>4sIIII', d, 0)
    if magic != b'PNP0':
        raise SystemExit(f'{path}: not a PNP0 file')
    entries = [struct.unpack_from('>iIIIII', d, 0x40 + i * 0x18) for i in range(n)]
    base = 0x40 + n * 0x18
    files = []
    for i in range(nfiles):
        a, b = struct.unpack_from('>II', d, base + i * 0x40)
        files.append((a, b, d[base + i * 0x40 + 8:base + i * 0x40 + 0x40].split(b'\0')[0].decode('latin1')))
    return version, h, entries, files

def name_hash(s):
    import zlib
    return zlib.crc32(s.upper().encode('latin1')) & 0xFFFFFFFF

def catalog_dump(path):
    version, h, entries, files = catalog_read(path)
    L = ['catalog version %d entries %d files %d hash %08x' % (version, len(entries), len(files), h)]
    for i, e in enumerate(entries):
        L.append('entry %d type %d offset %d size %d name %08x packed %d hash %08x' % ((i,) + e))
    for i, f in enumerate(files):
        L.append('file %d name %08x unk %08x %s' % ((i,) + f))
    L.append('namehash %d of %d' % (sum(name_hash(f[2]) == f[0] for f in files), len(files)))
    gh = name_hash('global_binary.sir')
    for typ in (-1, 0):
        e = next((e for e in entries if e[0] == typ and e[3] == gh), None)
        fi = next((i for i, f in enumerate(files) if f[0] == gh), -1)
        L.append('lookup global_binary.sir %d missing' % typ if e is None else
                 'lookup global_binary.sir %d file %d sector %d size %d packed %d' % (typ, fi, e[1] >> 11, e[2], e[4]))
    return L

def catalog_crosscheck(path, streams_dir):
    """Compare the catalog with the section table inside each .sir file. Returns (files checked, problems)."""
    version, h, entries, files = catalog_read(path)
    on_disk = {n.lower(): n for n in os.listdir(streams_dir)}
    problems = []; checked = 0
    for nh, _, name in files:
        if name not in on_disk:
            problems.append(f'{name}: not in {streams_dir}'); continue
        with open(os.path.join(streams_dir, on_disk[name]), 'rb') as f:
            head = f.read(0x40)
            n = struct.unpack_from('>I', head, 8)[0]
            tab = f.read(n * 0x18)
        own = [struct.unpack_from('>iIIIII', tab, i * 0x18) for i in range(n)]
        cat = [e for e in entries if e[3] == nh]
        checked += 1
        if [(e[0], e[1], e[2], e[5]) for e in own] != [(e[0], e[1], e[2], e[5]) for e in cat]:
            problems.append(f'{name}: {len(own)} sections in the file, {len(cat)} in the catalog, or fields differ')
    return checked, problems

# ----------------------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser(description='Extract textures, meshes and skeletons from Monster Lab .sir archives')
    ap.add_argument('archive')
    ap.add_argument('-o', '--out', help='output folder (default: <archive>_out next to the archive)')
    ap.add_argument('--list', action='store_true', help='only list contents')
    ap.add_argument('--sections', action='store_true', help='also write raw section dumps')
    ap.add_argument('--only', help='only extract items whose name contains this text')
    ap.add_argument('--no-textures', action='store_true')
    ap.add_argument('--no-meshes', action='store_true')
    ap.add_argument('--gamedb', action='store_true', help='global_binary.sir: print the game database as text')
    ap.add_argument('--image', action='store_true', help='global_binary.sir: print the input table, textures, string and action tables as text')
    ap.add_argument('--catalog', action='store_true', help='catalog.sir: print the file table as text')
    ap.add_argument('--level', action='store_true', help='a level file: print the header block and the HUD of zone 0 as text')
    a = ap.parse_args()
    if a.catalog:
        for line in catalog_dump(a.archive):
            print(line)
        return
    if a.level:
        for line in level_dump(a.archive):
            print(line)
        return
    if a.gamedb or a.image:
        for line in (gamedb_dump(a.archive) if a.gamedb else []) + (image_dump(a.archive) if a.image else []):
            print(line)
        return
    if np is None:
        raise SystemExit('numpy is required: pip install numpy pillow')

    version, secs = read_archive(a.archive)
    stem = os.path.splitext(os.path.basename(a.archive))[0]
    out = a.out or os.path.join(os.path.dirname(os.path.abspath(a.archive)), stem + '_out')
    print(f'{a.archive}: PNP0 v{version}, {len(secs)} sections')
    if not a.list:
        os.makedirs(out, exist_ok=True)
    summary = dict(archive=os.path.basename(a.archive), sections=[])
    want = (lambda n: a.only.lower() in n.lower()) if a.only else (lambda n: True)

    for s in secs:
        d = s['data']; si = s['index']
        texs = find_textures(d)
        base = detect_base(d, texs)
        print(f"\n[section {si}] type {s['type']}  offset 0x{s['offset']:x}  size 0x{s['size']:x}  "
              f"load address {('0x%08x' % base) if base else 'unknown'}  textures {len(texs)}")
        rec = dict(index=si, type=s['type'], offset=s['offset'], size=s['size'],
                   load_address=base, textures=[], meshes=[], skeletons=[])
        summary['sections'].append(rec)
        if a.sections and not a.list:
            open(os.path.join(out, f'section{si}.bin'), 'wb').write(d)
        sec_dir = os.path.join(out, f'section{si}')

        # textures
        tex_files = {}
        for t in texs:
            entry = dict(name=t['name'], width=t['w'], height=t['h'], format=TEX_FMT[t['fmt']], mips=t['mips'])
            rec['textures'].append(entry)
            if a.list or a.no_textures or base is None or not want(t['name']): continue
            o = t['ptr'] - base
            if t['fmt'] not in BLOCK and t['fmt'] != 0xE or Image is None:
                continue
            need, _, _ = tex_size(t['fmt'], t['w'], t['h'])
            if not (0 <= o and o + need <= len(d)): continue
            img = decode_texture(d[o:o + need], t['fmt'], t['w'], t['h'])
            if img is None: continue
            os.makedirs(os.path.join(sec_dir, 'textures'), exist_ok=True)
            fn = safe(t['name']) + '.png'
            Image.fromarray(img, 'RGBA').save(os.path.join(sec_dir, 'textures', fn))
            tex_files[t['name']] = fn; entry['file'] = f'textures/{fn}'
        for t in rec['textures']:
            print(f"  texture  {t['name']:<40} {t['width']}x{t['height']} {t['format']}")

        if base is None:
            continue
        gx = GX(d, base)

        # skinned part meshes (with scale) and their stream ranges
        part_ranges = {}
        for H, h in find_part_headers(d):
            nm = name_before(d, H) or f'part_{H:x}'
            # the part's scale factor is the float stored directly in front of its name
            p = H
            while d[p - 4:p] == PAD: p -= 4
            nstart = d.rfind(nm.encode(), max(0, p - 0x200), p)
            sc = struct.unpack_from('>f', d, nstart - 4)[0] if nstart >= 4 else 1.0
            if not (0.001 < abs(sc) < 1000): sc = 1.0
            part_ranges[H + h[6]] = (nm, H + h[0], sc)

        streams = find_streams(d)
        seen = set(); used_names = collections.Counter(); objs = collections.OrderedDict()
        for st in streams:
            if st in seen: continue
            if st in part_ranges:
                nm, end, sc = part_ranges[st]; kind = 'skinned'
            else:
                nm, end, sc = (name_before(d, st, 0x2000) or f'mesh_{st:x}'), None, 1.0; kind = 'static'
            draws, stop = gx.parse(st, end)
            for s2 in streams:
                if st < s2 < stop: seen.add(s2)
            if not draws: continue
            if not want(nm): continue
            mesh = None if (a.list or a.no_meshes) else gx.to_mesh(draws)
            o = objs.setdefault(nm, dict(name=nm, kind=kind, scale=sc, offsets=[], draws=0, parts=[], errors=0))
            o['offsets'].append(st); o['draws'] += len(draws)
            if mesh is not None:
                V, N, T, F = mesh; o['parts'].append((V * sc, N, T, F))
            elif not (a.list or a.no_meshes):
                o['errors'] += 1
        for nm, o in objs.items():
            info = dict(name=nm, kind=o['kind'], scale=o['scale'], draws=o['draws'], submeshes=len(o['offsets']),
                        offsets=[hex(x) for x in o['offsets']])
            if o['parts']:
                os.makedirs(os.path.join(sec_dir, 'meshes'), exist_ok=True)
                fname = safe(nm) + '.obj'
                write_obj(os.path.join(sec_dir, 'meshes', fname),
                          [(f'{safe(nm)}_{i}', *p) for i, p in enumerate(o['parts'])])
                info.update(file=f'meshes/{fname}', vertices=sum(len(p[0]) for p in o['parts']),
                            triangles=sum(len(p[3]) for p in o['parts']))
            if o['errors']: info['undecodable_submeshes'] = o['errors']
            rec['meshes'].append(info)
            extra = f"{info['triangles']} tris" if 'triangles' in info else ''
            print(f"  mesh     {nm:<40} {o['kind']:<8} {len(o['offsets']):>3} submesh(es) {o['draws']:>4} draws  {extra}")

        # skeletons
        if not a.list:
            sk = {k: v for k, v in find_skeletons(d).items() if want(k)}
            if sk:
                os.makedirs(os.path.join(sec_dir, 'skeletons'), exist_ok=True)
                for k, v in sk.items():
                    json.dump(dict(name=k, joints=v), open(os.path.join(sec_dir, 'skeletons', safe(k) + '.json'), 'w'), indent=1)
                    rec['skeletons'].append(dict(name=k, joints=len(v), file=f'skeletons/{safe(k)}.json'))
                    print(f'  skeleton {k:<40} {len(v)} joints')

    if not a.list:
        json.dump(summary, open(os.path.join(out, 'index.json'), 'w'), indent=1, default=str)
        print(f'\nDone. Output in {out}')

if __name__ == '__main__':
    main()
