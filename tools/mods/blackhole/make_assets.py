"""Black hole head: makes every picture and the model of the mod from code. No game data is read or written.

    python tools/mods/blackhole/make_assets.py [--out DIR]

Writes (default DIR: build/mods/BlackHoleHead, never committed):
  textures/bho_h_01.png        256x256 colour sheet (the size the game's heads use)
  textures/bho_h_01spec.png    256x256 gloss sheet (bright = shiny; only the projector shines)
  icons/p_hd_sha077.png        64x64 part icon      (the game's part icons: 64x64, and 256x256 for "_h")
  icons/p_hd_sha077_h.png      256x256 part icon
  model/blackhole.obj          the model, for a look in any viewer
  model/blackhole.json         the same as arrays (positions, normals, uvs, brightness, triangles and strips per group) for build_mod.py
Needs numpy and Pillow.

The look (a black hole as films draw it):
  core       a black ball
  disc       a thin, tilted, glowing ring of streaky amber gas around it, crossing in front of the ball
  halo       the light of the disc's far side bent around the hole: a wide arc over the ball and a thinner one under it,
             and a thin bright "photon ring" right at the ball's edge
             Disc and halo always show the camera the same side (seen from a little above the disc, as on the
             film poster), from wherever the monster is looked at.
  projector  the only machine part: a small column on the neck with an amber lens, and a faint beam up to the hole
Disc, halo and beam are drawn as light (added to the picture, full bright, both sides): black in their picture is nothing.

The sheet's layout (u right, v down, 0..1):
  rows 0-79            the disc unrolled: u runs around the hole (wraps), v from its inner to its outer edge
  rows 80-89           the photon ring: u around, v across the thin band
  rows 90-127          the halo's arcs: u around, v from the ball outwards
  bottom left quarter  the core (black)
  bottom right quarter the projector: brushed dark metal with an amber lens
"""
import argparse
import json
import math
import os

import numpy as np
from PIL import Image

MODEL_KEY = "p_hd_sha077"      # "p_" + limb + "_" + animation set + "_" + model number (PartAnimKey, 0x802152FC)
DISC_ROWS, RING_ROWS, ARC_ROWS = (0, 80), (80, 90), (90, 128)       # rows of the sheet
DISC_RADII = (1.35, 1.8, 2.35, 3.0)                                   # in core radii
ROLL, PITCH = math.radians(13), math.radians(14)                      # the disc leans sideways (left end up), and is seen from this far above its plane
GLOW = 0.37                                                           # brightness of the light groups in the game (1 = the sheet's own, doubled)


def noise(shape, seed, octaves=4, wrap_x=True):
    """Value noise that tiles in x."""
    rng = np.random.default_rng(seed)
    h, w = shape
    out = np.zeros(shape)
    for o in range(octaves):
        n = 4 << o
        g = rng.random((n + 1, n + 1))
        if wrap_x:
            g[:, -1] = g[:, 0]
        ys, xs = np.linspace(0, n, h, endpoint=False), np.linspace(0, n, w, endpoint=False)
        y0, x0 = ys.astype(int), xs.astype(int)
        fy, fx = (ys - y0)[:, None], (xs - x0)[None, :]
        fy, fx = fy * fy * (3 - 2 * fy), fx * fx * (3 - 2 * fx)
        a, b = g[y0][:, x0], g[y0][:, x0 + 1]
        c, d = g[y0 + 1][:, x0], g[y0 + 1][:, x0 + 1]
        out += ((a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy) / (1 << o)
    return out / out.max()


def fbm(h, w, fy, fx, octaves, seed):
    """Noise with its own grain in each direction (fy cells down, fx across; tiles in x): fy > fx gives streaks along x."""
    total, amp, norm = 0, 1.0, 0.0
    for o in range(octaves):
        ny, nx = fy << o, fx << o
        r = np.random.default_rng(seed + o).random((ny + 1, nx))
        y, x = np.linspace(0, ny, h, endpoint=False), np.linspace(0, nx, w, endpoint=False)
        y0, x0 = y.astype(int), x.astype(int)
        ty, tx = (y - y0)[:, None], (x - x0)[None, :]
        ty, tx = ty * ty * (3 - 2 * ty), tx * tx * (3 - 2 * tx)
        x1 = (x0 + 1) % nx
        total = total + amp * ((r[y0][:, x0] * (1 - tx) + r[y0][:, x1] * tx) * (1 - ty) + (r[y0 + 1][:, x0] * (1 - tx) + r[y0 + 1][:, x1] * tx) * ty)
        norm += amp
        amp *= 0.5
    return total / norm


def ramp(t, stops):
    """Colour for t in 0..1 (arrays) from [(t, "#rrggbb")]."""
    t = np.clip(t, 0, 1)
    xs = [s[0] for s in stops]
    cols = [[int(s[1][i:i + 2], 16) / 255 for i in (1, 3, 5)] for s in stops]
    return np.dstack([np.interp(t, xs, [c[k] for c in cols]) for k in range(3)])


HEAT = [(0, "#ffe6a8"), (0.16, "#ffc866"), (0.42, "#ff9a34"), (0.72, "#dc4a1c"), (1, "#5e1810")]    # hot inside, red outside
WHITE_HOT = [(0, "#fff8e8"), (1, "#ff9a4d")]


def amber(t):
    """Black -> deep orange -> amber -> pale yellow, for t in 0..1 (arrays)."""
    t = np.clip(t, 0, 1)[..., None]
    stops = np.array([[0.02, 0.01, 0.03], [0.55, 0.16, 0.02], [1.0, 0.55, 0.10], [1.0, 0.86, 0.45], [1.0, 0.97, 0.85]])
    x = t * (len(stops) - 1)
    i = np.clip(x.astype(int), 0, len(stops) - 2)
    f = x - i
    return stops[i[..., 0]] * (1 - f) + stops[i[..., 0] + 1] * f


def shear(img, turns):
    """Rolls each row further than the one above (in all `turns` of the width): streaks become a slow spiral and still tile."""
    h, w = img.shape[:2]
    return np.stack([np.roll(img[y], int(round(turns * w * y / h)), axis=0) for y in range(h)])


def disc_strip(w, h, seed=3):
    """The disc unrolled, as light: orbital streaks, hot and pale at the inner edge, red and ragged at the outer one."""
    v = np.linspace(0, 1, h)[:, None] * np.ones((1, w))
    streak = shear(fbm(h, w, 12, 5, 4, seed), 0.30)
    fine = shear(fbm(h, w, 20, 20, 3, seed + 6), 0.30)
    clump = fbm(h, w, 3, 4, 2, seed + 18)
    inten = (0.90 * (1 - v) ** 0.7 + 0.25) * (0.35 + 1.0 * streak ** 1.5) * (0.72 + 0.56 * fine) * (0.75 + 0.5 * clump)
    ragged = np.clip((1 - v - 0.16 * clump) / 0.22, 0, 1) ** 1.2
    edge = np.clip((v - 0.02) / 0.07, 0, 1) * ragged
    return np.clip(ramp(v * 0.85, HEAT) * (inten * edge)[..., None] * 1.5, 0, 1)


def ring_strip(w, h):
    """The photon ring: a thin white-hot line, v across it."""
    pv = np.linspace(-1, 1, h)[:, None] * np.ones((1, w))
    line = np.exp(-(pv * 2.3) ** 2) * (0.85 + 0.3 * fbm(h, w, 2, 16, 2, 60))
    return np.clip(ramp(np.abs(pv), WHITE_HOT) * line[..., None], 0, 1)


def arc_strip(w, h):
    """The bent light over and under the hole: fine rings of streaks, brightest at the ball, fading outwards."""
    v = np.linspace(0, 1, h)[:, None] * np.ones((1, w))
    streak = fbm(h, w, 10, 4, 4, 31)
    fine = fbm(h, w, 16, 24, 3, 37)
    clump = fbm(h, w, 2, 5, 2, 43)
    inten = (1.0 * (1 - v) ** 0.8 + 0.10) * (0.30 + 1.1 * streak ** 1.6) * (0.70 + 0.6 * fine)
    edge = np.clip(v / 0.10, 0, 1) * np.clip((1 - v - 0.18 * clump) / 0.35, 0, 1) ** 1.3
    return np.clip(ramp(0.18 + 0.72 * v, HEAT) * (inten * edge)[..., None] * 1.3, 0, 1)


def sheet(size=256):
    half = size // 2
    rgb = np.zeros((size, size, 3))
    spec = np.zeros((size, size))
    k = size / 256
    for r, make in ((DISC_ROWS, disc_strip), (RING_ROWS, ring_strip), (ARC_ROWS, arc_strip)):
        a, b = int(r[0] * k), int(r[1] * k)
        rgb[a:b] = make(size, b - a)
    # Core: black.
    # Projector: dark gunmetal with a brushed grain, an amber lens (top band), two grooves, no bolts.
    grain = noise((half, half), 11, octaves=4, wrap_x=True)
    metal = (0.16 + 0.10 * grain)[..., None] * np.array([0.85, 0.92, 1.0])
    vy = np.linspace(0, 1, half)[:, None] + 0 * grain
    cap = np.clip(1 - vy / 0.22, 0, 1)
    metal = metal * (1 - cap[..., None]) + amber(0.45 + 0.5 * cap) * cap[..., None]
    for band in (0.45, 0.72):
        metal *= (1 - 0.6 * np.exp(-((vy - band) / 0.02) ** 2))[..., None]
    rgb[half:, half:] = np.clip(metal, 0, 1)
    spec[half:, half:] = (0.55 + 0.35 * grain) * (1 - cap)
    return rgb, spec


def halo_shape(s, c):
    """Outer radius (core radii) and brightness of the halo's arcs at an angle with sine s and cosine c (0 right, 90 degrees up).
    Works on numbers and arrays."""
    up, down = np.maximum(s, 0.0) ** 0.8, np.maximum(-s, 0.0) ** 0.8
    r_out = 1.35 + 0.65 * up + 0.20 * down
    gain = (0.35 + 1.15 * up + 0.60 * down) * (1 - 0.25 * c)      # the side that turns towards the viewer is brighter
    return r_out, gain


def disc_gain(c, s=1.0):
    """Brightness around the disc (c, s: cosine and sine of the angle; c = 1 on the right, s > 0 the near half): brighter on
    the left, where the gas comes towards the viewer, and dim on the far half, whose light is what the halo shows."""
    return (1.0 - 0.25 * c) * (1.0 - 0.7 * np.maximum(-s, 0.0) ** 0.7)


def icon(size):
    """The hole as it looks in a fight, seen from the front: ball, photon ring, the arcs, the tilted disc across it, the projector below."""
    ss = 4
    n = size * ss
    yy, xx = np.mgrid[0:n, 0:n]
    x = (xx + 0.5) / n * 2 - 1
    y0 = (yy + 0.5) / n * 2 - 1
    y = y0 + 0.16                                                         # the hole sits above the middle
    rc = 0.30                                                           # core radius
    r = np.hypot(x, y) / rc
    light = np.zeros((n, n, 3))
    # the arcs
    ct, st = math.cos(ROLL), math.sin(ROLL)
    xr, yr = x * ct + y * st, -x * st + y * ct                            # along and across the disc's long axis
    rs = np.maximum(r, 1e-6) * rc
    r_out, gain = halo_shape(-yr / rs, xr / rs)
    v = (r - 1.03) / (r_out - 1.03)
    vc = np.clip(v, 0, 1)
    rings = 0.55 + 0.45 * np.sin(r * 34 + 2.5 * noise((n, n), 5, 3, False))
    arc = np.where((v > 0) & (v < 1), (1 - vc) ** 1.1 * np.clip(vc / 0.08, 0, 1) * np.clip((1 - vc) / 0.3, 0, 1) * rings, 0) * gain
    light += ramp(0.10 + 0.75 * vc, HEAT) * arc[..., None] * 2.2
    # the photon ring
    light += ramp(np.zeros((n, n)), WHITE_HOT) * np.exp(-((r - 1.02) / 0.022) ** 2)[..., None]
    # the disc: a flat ring seen from a little above, leaning
    depth = yr / math.sin(PITCH)                                         # > 0: the near half, in front of the ball
    rho = np.hypot(xr, depth) / rc
    dv = (rho - DISC_RADII[0]) / (DISC_RADII[-1] - DISC_RADII[0])
    streak = 0.5 + 0.5 * np.sin(rho * 21 + np.arctan2(depth, xr) * 2 + 3 * noise((n, n), 8, 3, False))
    din = (dv > 0) & (dv < 1) & ((depth > 0) | (r > 1.0))                 # the far half is hidden behind the ball
    dvc = np.clip(dv, 0, 1)
    disc = np.where(din, (1.15 * (1 - dvc) ** 1.25 + 0.16) * (0.45 + 0.75 * streak) * np.clip(dvc / 0.06, 0, 1) * np.clip((1 - dvc) / 0.3, 0, 1) ** 1.4, 0)
    disc = disc * disc_gain(xr / np.maximum(rho * rc, 1e-6), depth / np.maximum(rho * rc, 1e-6))
    light += ramp(dvc, HEAT) * disc[..., None] * 1.5
    front = din & (depth > 0)
    # the projector under the hole, and its beam
    ey = y0
    top = 0.70
    half = 0.21 - 0.07 * np.clip((ey - top - 0.07) / 0.2, 0, 1)            # a wide lens rim, tapering to the neck
    body = (np.abs(x) < half) & (ey > top) & (ey < 0.98)
    lens = (np.abs(x) < 0.17) & (ey > top) & (ey < top + 0.07)
    grain = noise((n, n), 12, 4, False)
    shade = 1.25 - 0.9 * (np.abs(x) / 0.21) ** 2                           # round: lighter down the middle
    solid = np.array([0.30, 0.34, 0.40]) * ((0.75 + 0.5 * grain) * shade)[..., None]
    solid *= (1 - 0.55 * np.exp(-((ey - top - 0.15) / 0.012) ** 2))[..., None]      # a groove
    solid = np.where(lens[..., None], amber(0.62 + 0.3 * grain), solid)
    bt = np.clip((top - ey) / (top - (rc * 0.8 - 0.16)), 0, 1)             # 0 at the lens, 1 at the ball
    bw = 0.06 + 0.07 * bt
    beam = (np.abs(x) < bw) & (ey < top) & (r > 1.0) & (y > 0)
    light += amber(np.full((n, n), 0.7)) * (np.where(beam, (0.30 * (1 - bt) ** 1.2 + 0.05) * (1 - (np.abs(x) / bw) ** 2), 0))[..., None]
    light = 1 - np.exp(-light * 1.25)                                     # soft clip, as light adds up
    peak = light.max(axis=2, keepdims=True)
    a_light = np.clip(peak[..., 0] * 1.6, 0, 1)
    col = light / np.maximum(peak, 1e-4) * np.minimum(peak * 1.6, 1.0) ** 0.5
    core = np.clip((1.0 - r) / 0.03, 0, 1) * (1 - front)                 # the black ball, where the disc is not in front of it
    alpha = np.maximum(a_light, core)
    col = np.where((core > a_light)[..., None], col * a_light[..., None], col)
    col = np.where(body[..., None], solid, col)
    alpha = np.where(body, 1.0, alpha)
    rgba = np.dstack([np.clip(col, 0, 1), np.clip(alpha, 0, 1)])
    img = Image.fromarray((rgba * 255).astype(np.uint8), "RGBA")
    return img.resize((size, size), Image.LANCZOS)


class Mesh:
    def __init__(self):
        self.pos, self.nrm, self.uv, self.col, self.groups, self.strips = [], [], [], [], {}, {}

    def vert(self, p, n, uv, col=None):
        """col: brightness of a vertex of a light group (0..2), None for the lit groups."""
        self.pos.append([round(float(c), 5) for c in p])
        l = math.sqrt(sum(c * c for c in n)) or 1.0
        self.nrm.append([round(float(c) / l, 4) for c in n])
        self.uv.append([round(float(c), 5) for c in uv])
        self.col.append(None if col is None else round(float(col), 3))
        return len(self.pos) - 1

    def quad(self, group, a, b, c, d):
        self.groups.setdefault(group, []).extend([[a, b, c], [a, c, d]])

    def strip(self, group, xs, ys):
        """The same surface as a triangle strip x0 y0 x1 y1 ...: its first triangle (x0, y0, x1) is wound like the quads."""
        self.strips.setdefault(group, []).append([i for pair in zip(xs, ys) for i in pair])

    def bands(self, group, rows):
        """Quads and strips between consecutive rows of vertices."""
        for a, b in zip(rows, rows[1:]):
            for i in range(len(a) - 1):
                self.quad(group, a[i], b[i], b[i + 1], a[i + 1])
            self.strip(group, a, b)


R = 0.13                                   # core radius
C = (0.0, 0.36, 0.0)                       # core centre


def on_screen(x, y, z=0.0):
    """Billboard space (x right, y up, z towards the viewer; in core radii), leaning by ROLL: the left end goes up."""
    return (R * (x * math.cos(ROLL) + y * math.sin(ROLL)), R * (-x * math.sin(ROLL) + y * math.cos(ROLL)), R * z)


def build_model():
    """Part space: origin at the neck socket, +y up, +z to the front; units as the game's heads (about 0.5 across).

    core, projector   lit by the game as any part; triangles counter-clockwise seen from outside (as in an .obj)
    beam              light, in part space: a cone from the projector's lens to the core
    disc, halo        light, in "billboard space": x right, y up on the screen and z towards the viewer, around the core's
                      centre. The disc is a flat ring seen from PITCH
                      above its plane, so its near half crosses the ball below the middle; photon ring and arcs lie in
                      the plane of the screen.
    The uvs point into the sheet of make_assets.sheet(). The light groups have a brightness per vertex.
    """
    m = Mesh()
    c = np.array(C)

    # core: everything maps to one black spot of the bottom left quarter
    seg, rings = 12, 8
    grid = []
    for j in range(rings + 1):
        th = math.pi * j / rings
        row = []
        for i in range(seg + 1):
            ph = 2 * math.pi * i / seg
            n = np.array([math.sin(th) * math.sin(ph), math.cos(th), math.sin(th) * math.cos(ph)])
            row.append(m.vert(c + R * n, n, (0.25 + 0.1 * (i / seg - 0.5), 0.75 + 0.1 * (j / rings - 0.5))))
        grid.append(row)
    m.bands("core", grid)

    def lathe(group, profile, segs, u0, u1):
        """Turns a profile [(radius, y, v)] around the y axis; u runs across [u0, u1] of the right quarter."""
        cols = []
        for i in range(segs + 1):
            a = 2 * math.pi * i / segs
            col = []
            for k, (r, y, v) in enumerate(profile):
                r2, y2 = profile[min(k + 1, len(profile) - 1)][:2]
                r1, y1 = profile[max(k - 1, 0)][:2]
                slope = (-(r2 - r1), (y2 - y1))                       # outward normal of the profile, turned
                n = (math.sin(a) * slope[1], slope[0], math.cos(a) * slope[1])
                col.append(m.vert((r * math.sin(a), y, r * math.cos(a)), n, (0.5 + 0.5 * (u0 + (u1 - u0) * i / segs), 0.5 + 0.5 * v)))
            cols.append(col)
        for i in range(segs):
            for k in range(len(profile) - 1):
                m.quad(group, cols[i][k], cols[i][k + 1], cols[i + 1][k + 1], cols[i + 1][k])
            m.strip(group, cols[i + 1], cols[i])

    lathe("projector", [(0.060, 0.000, 1.00), (0.070, 0.020, 0.90), (0.050, 0.045, 0.72), (0.050, 0.075, 0.55),
                        (0.066, 0.095, 0.45), (0.066, 0.110, 0.24), (0.040, 0.125, 0.10), (0.0, 0.125, 0.0)], 10, 0.0, 1.0)
    lit = len(m.pos)

    def row_v(rows, t):
        return (rows[0] + 0.6 + t * (rows[1] - rows[0] - 1.2)) / 256.0      # stays half a texel inside its rows

    # beam: a faint cone of the disc's streaks from the lens up to the core
    seg = 10
    rows = []
    for rad, y, t, gain in ((0.030, 0.125, 0.10, 0.50), (0.050, C[1] - 0.9 * R, 0.30, 0.0)):
        row = []
        for i in range(seg + 1):
            a = 2 * math.pi * i / seg
            row.append(m.vert((rad * math.sin(a), y, rad * math.cos(a)), (0, 1, 0), (1.0 * i / seg, row_v(DISC_ROWS, t)), GLOW * gain))
        rows.append(row)
    m.bands("beam", rows)

    # disc (billboard space): u twice around, v across DISC_RADII; the half with z > 0 is the near one
    seg = 32
    rows = []
    for rad in DISC_RADII:
        t = (rad - DISC_RADII[0]) / (DISC_RADII[-1] - DISC_RADII[0])
        row = []
        for i in range(seg + 1):
            a = 2 * math.pi * i / seg
            p = on_screen(rad * math.cos(a), -rad * math.sin(a) * math.sin(PITCH), rad * math.sin(a) * math.cos(PITCH))
            row.append(m.vert(p, (0, 0, 1), (2.0 * i / seg, row_v(DISC_ROWS, t)), GLOW * 1.1 * float(disc_gain(math.cos(a), math.sin(a)))))
        rows.append(row)
    m.bands("disc", rows)

    # halo (billboard space): photon ring, then the arcs
    seg = 40
    ang = [2 * math.pi * i / seg for i in range(seg + 1)]

    def bb(rad, a):
        return on_screen(rad * math.cos(a), rad * math.sin(a))

    ring = [[m.vert(bb(rad, a), (0, 0, 1), (3.0 * i / seg, row_v(RING_ROWS, t)), GLOW * 1.1) for i, a in enumerate(ang)] for rad, t in ((0.955, 0.0), (1.065, 1.0))]
    m.bands("halo", ring)
    arcs = []
    for t in (0.0, 1.0):
        row = []
        for i, a in enumerate(ang):
            r_out, gain = halo_shape(math.sin(a), math.cos(a))
            row.append(m.vert(bb(r_out if t else 1.03, a), (0, 0, 1), (2.0 * i / seg, row_v(ARC_ROWS, t)), GLOW * 0.85 * float(gain)))
        arcs.append(row)
    m.bands("halo", arcs)
    m.lit = lit
    m.light = [{"groups": ["beam"], "billboard": False, "scroll": [0.0, -0.25]},
               {"groups": ["disc", "halo"], "billboard": True, "centre": list(C), "axis": [0.0, 0.0, 0.0], "scroll": [0.04, 0.0]}]     # no axis: y is the screen's up
    return m


def write_obj(m, path):
    with open(path, "w") as f:
        f.write("# Black hole head, made by tools/mods/blackhole/make_assets.py\n")
        for p in m.pos:
            f.write("v %g %g %g\n" % tuple(p))
        for t in m.uv:
            f.write("vt %g %g\n" % (t[0], 1 - t[1]))
        for n in m.nrm:
            f.write("vn %g %g %g\n" % tuple(n))
        for g, tris in m.groups.items():
            f.write("g %s\n" % g)
            for t in tris:
                f.write("f " + " ".join("%d/%d/%d" % (i + 1, i + 1, i + 1) for i in t) + "\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
    ap.add_argument("--out", default=os.path.join(root, "build", "mods", "BlackHoleHead"))
    a = ap.parse_args()
    for d in ("textures", "icons", "model"):
        os.makedirs(os.path.join(a.out, d), exist_ok=True)
    rgb, spec = sheet()
    Image.fromarray((rgb * 255).astype(np.uint8), "RGB").save(os.path.join(a.out, "textures", "bho_h_01.png"))
    Image.fromarray((spec * 255).astype(np.uint8), "L").save(os.path.join(a.out, "textures", "bho_h_01spec.png"))
    icon(64).save(os.path.join(a.out, "icons", MODEL_KEY + ".png"))
    icon(256).save(os.path.join(a.out, "icons", MODEL_KEY + "_h.png"))
    m = build_model()
    write_obj(m, os.path.join(a.out, "model", "blackhole.obj"))
    with open(os.path.join(a.out, "model", "blackhole.json"), "w") as f:
        json.dump({"key": MODEL_KEY, "positions": m.pos, "normals": m.nrm, "uvs": m.uv, "groups": m.groups, "strips": m.strips,
                   "colours": m.col,               # brightness per vertex of the light groups (null: a lit vertex)
                   "lit": m.lit,                   # the first vertices are those of the lit groups (the only ones with a normal)
                   "winding": "ccw-outside",
                   # groups drawn as light: added to the picture, full bright, both sides, the sheet scrolling
                   # along u and v (turns per second). billboard: positions are offsets from `centre` on the
                   # screen, with y along `axis` as the camera sees it.
                   "light": m.light}, f)
    tris = sum(len(t) for t in m.groups.values())
    print("black hole head: %d vertices, %d triangles, groups %s -> %s" % (len(m.pos), tris, ", ".join(m.groups), a.out))


if __name__ == "__main__":
    main()
