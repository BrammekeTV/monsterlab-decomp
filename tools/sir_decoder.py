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
# slot 0xDC4), the texture list at the start of the image (0xD94), the string table (0xDA8) and
# the action table (0x844). Reference for the PC loader, read straight from the big-endian bytes.
INPUT_STATE = {1: 'f', 2: 'f', 9: 'f', 5: 'ffff', 6: 'ffff', 7: 'ifff', 8: 'ifff'}   # by step type, at +0x50

def fnv(b):
    h = 0x811C9DC5
    for c in b:
        h = ((h ^ c) * 0x01000193) & 0xFFFFFFFF
    return h

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
    a = ap.parse_args()
    if a.catalog:
        for line in catalog_dump(a.archive):
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
