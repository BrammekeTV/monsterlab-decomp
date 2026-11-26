"""Black hole head: makes every picture and the model of the mod from code. No game data is read or written.

    python tools/mods/blackhole/make_assets.py [--out DIR]

Writes (default DIR: build/mods/BlackHoleHead, never committed):
  textures/bho_h_01.png        256x256 colour sheet (the size the game's heads use)
  textures/bho_h_01spec.png    256x256 gloss sheet (bright = shiny; only the emitter shines)
  icons/p_hd_sha077.png        64x64 part icon      (the game's part icons: 64x64, and 256x256 for "_h")
  icons/p_hd_sha077_h.png      256x256 part icon
  model/blackhole.obj          the model, for a look in any viewer
  model/blackhole.json         the same as arrays (positions, normals, uvs, triangles and strips per group) for build_mod.py
Needs numpy and Pillow.

The look: a black ball inside a larger amber ball that is turned inside out (an "inverted hull"), so only the
far inner side of the amber ball is drawn and the black ball gets a glowing, streaky rim from every side.

The sheet's layout (u right, v down, 0..1):
  top half             the hull: u runs around its axis (wraps), v from pole to pole
  bottom left quarter  the core (black)
  bottom right quarter the stabilizer: brushed dark metal with an amber emitter cap
"""
import argparse
import json
import math
import os

import numpy as np
from PIL import Image

MODEL_KEY = "p_hd_sha077"      # "p_" + limb + "_" + animation set + "_" + model number (PartAnimKey, 0x802152FC)


def noise(shape, seed, octaves=4, wrap_x=True):
    """Value noise that tiles in x (the disc sheet wraps around the hole)."""
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


def amber(t):
    """Black -> deep orange -> amber -> pale yellow, for t in 0..1 (arrays)."""
    t = np.clip(t, 0, 1)[..., None]
    stops = np.array([[0.02, 0.01, 0.03], [0.55, 0.16, 0.02], [1.0, 0.55, 0.10], [1.0, 0.86, 0.45], [1.0, 0.97, 0.85]])
    x = t * (len(stops) - 1)
    i = np.clip(x.astype(int), 0, len(stops) - 2)
    f = x - i
    return stops[i[..., 0]] * (1 - f) + stops[i[..., 0] + 1] * f


def hull_strip(w, h):
    """The glowing hull unrolled: u runs once around its axis, v from pole to pole. Long streaks along u,
    sheared so they swirl, hot pale lanes on deep orange, a little darker towards the poles."""
    v = np.linspace(0, 1, h)[:, None]
    streak = np.zeros((h, w))
    for k, (rows, seed) in enumerate(((20, 1), (40, 2), (80, 3))):
        n = noise((rows, w // 8), seed, octaves=3)
        img = np.asarray(Image.fromarray((n * 255).astype(np.uint8)).resize((w, h), Image.BICUBIC)) / 255.0
        shift = (v[:, 0] * w * (1 + k)).astype(int)                      # shear by whole turns: the streaks wind around the hole and still tile
        img = np.stack([np.roll(img[y], shift[y]) for y in range(h)])
        streak += img / (k + 1)
    streak = (streak - streak.min()) / (streak.max() - streak.min())
    clumps = noise((h, w), 7, octaves=3)
    belt = 0.55 + 0.45 * np.sin(v * math.pi) ** 0.8                       # brightest around the equator
    t = (0.30 + 0.80 * streak ** 1.5) * belt * (0.80 + 0.30 * clumps)
    return amber(t)


def sheet(size=256):
    half = size // 2
    rgb = np.zeros((size, size, 3))
    spec = np.zeros((size, size))
    rgb[:half] = hull_strip(size, half)
    # Core: black. Nothing else: the hull behind it gives the rim.
    # Stabilizer: dark gunmetal with a brushed grain, an amber cap (top band), two grooves, no bolts.
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


def icon(size):
    """The hole as it looks in game: a black ball, the swirling amber hull as a halo around it, the emitter below."""
    ss = 4
    n = size * ss
    y, x = np.mgrid[0:n, 0:n]
    x = (x + 0.5) / n * 2 - 1
    y = (y + 0.5) / n * 2 - 1 + 0.16                                      # the hole sits a little above the middle
    r = np.hypot(x, y)
    ang = np.arctan2(y, x)
    swirl = noise((n, n), 5, octaves=4, wrap_x=False)
    rc, rh = 0.46, 0.70                                                  # core and hull radius
    # Inside of a sphere seen from the front: the streaks crowd together towards the rim.
    depth = np.sqrt(np.clip(1 - (r / rh) ** 2, 0, 1))
    streaks = 0.5 + 0.5 * np.sin(ang * 7 + depth * 9 + swirl * 6)
    t = (0.35 + 0.65 * streaks ** 1.4) * (0.75 + 0.3 * swirl)
    col = amber(t)
    a = np.clip((rh - r) / 0.02, 0, 1)
    core = np.clip((rc - r) / 0.015, 0, 1)
    col *= (1 - core)[..., None]
    # Emitter: a small dark column with an amber cap under the hole.
    ey = y - 0.16
    body = (np.abs(x) < 0.16 - 0.05 * np.clip((ey - 0.70) / 0.2, 0, 1)) & (ey > 0.62) & (ey < 0.98)
    capm = (np.abs(x) < 0.13) & (ey > 0.62) & (ey < 0.70)
    free = (a < 0.5)[..., None]
    metal = np.array([0.15, 0.17, 0.20]) * (0.8 + 0.5 * swirl)[..., None]
    col = np.where(body[..., None] & free, metal, col)
    col = np.where(capm[..., None] & free, amber(0.6 + 0.3 * swirl), col)
    a = np.maximum(a, body.astype(float))
    rgba = np.dstack([np.clip(col, 0, 1), np.clip(a, 0, 1)])
    img = Image.fromarray((rgba * 255).astype(np.uint8), "RGBA")
    return img.resize((size, size), Image.LANCZOS)


class Mesh:
    def __init__(self):
        self.pos, self.nrm, self.uv, self.groups, self.strips = [], [], [], {}, {}

    def vert(self, p, n, uv):
        self.pos.append([round(float(c), 5) for c in p])
        l = math.sqrt(sum(c * c for c in n)) or 1.0
        self.nrm.append([round(float(c) / l, 4) for c in n])
        self.uv.append([round(float(c), 5) for c in uv])
        return len(self.pos) - 1

    def quad(self, group, a, b, c, d):
        self.groups.setdefault(group, []).extend([[a, b, c], [a, c, d]])

    def strip(self, group, xs, ys):
        """The same surface as a triangle strip x0 y0 x1 y1 ...: its first triangle (x0, y0, x1) is wound like the quads."""
        self.strips.setdefault(group, []).append([i for pair in zip(xs, ys) for i in pair])


HULL_GROUP = "hull"


def build_model():
    """Part space: origin at the neck socket, +y up, +z to the front; units as the game's heads (about 0.5 across).

    core       black sphere, radius R, centre C
    hull       a larger sphere around it that carries the amber picture, turned inside out: its triangles are
               wound the other way and its normals point at the centre, so with the usual back-face culling only
               the far inner side is drawn: a glowing rim around the core from every direction
    emitter    the stabilizer on the neck: a tapered column with an amber cap pointing up at the hole
    The triangles of core and emitter are counter-clockwise seen from outside (as in an .obj); the hull's are
    clockwise. The uvs point into the sheet of make_assets.sheet().
    """
    m = Mesh()
    R = 0.13
    C = np.array([0.0, 0.36, 0.0])

    def sphere(group, radius, seg, rings, axis_tilt, uv, inside_out):
        ct, st = math.cos(axis_tilt), math.sin(axis_tilt)
        grid = []
        for j in range(rings + 1):
            th = math.pi * j / rings
            row = []
            for i in range(seg + 1):
                ph = 2 * math.pi * i / seg
                n = np.array([math.sin(th) * math.sin(ph), math.cos(th), math.sin(th) * math.cos(ph)])
                n = np.array([n[0] * ct + n[1] * st, -n[0] * st + n[1] * ct, n[2]])     # lean the axis sideways
                row.append(m.vert(C + radius * n, -n if inside_out else n, uv(i / seg, j / rings)))
            grid.append(row)
        for j in range(rings):
            for i in range(seg):
                q = (grid[j][i], grid[j + 1][i], grid[j + 1][i + 1], grid[j][i + 1])
                m.quad(group, *(q[::-1] if inside_out else q))
            m.strip(group, *((grid[j + 1], grid[j]) if inside_out else (grid[j], grid[j + 1])))

    # core: everything maps to one black spot of the bottom left quarter
    sphere("core", R, 12, 8, 0.0, lambda u, v: (0.25 + 0.1 * (u - 0.5), 0.75 + 0.1 * (v - 0.5)), False)
    # hull: the top half of the sheet, twice around (the streaks repeat on the far side), axis leaning 14 degrees
    sphere(HULL_GROUP, R * 1.6, 16, 10, math.radians(14), lambda u, v: (2.0 * u, 0.008 + 0.484 * v), True)

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

    lathe("emitter", [(0.060, 0.000, 1.00), (0.070, 0.020, 0.90), (0.050, 0.045, 0.72), (0.050, 0.075, 0.55),
                      (0.066, 0.095, 0.45), (0.066, 0.110, 0.24), (0.040, 0.125, 0.10), (0.0, 0.125, 0.0)], 10, 0.0, 1.0)
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
                   "winding": "ccw-outside",
                   "inverted": [HULL_GROUP],       # wound inside out: draw with back-face culling as everything else
                   "unlit": [HULL_GROUP],          # full bright if the material allows
                   "spin": {"group": HULL_GROUP, "by": "texture scroll along u", "turns_per_second": 0.08}}, f)
    tris = sum(len(t) for t in m.groups.values())
    print("black hole head: %d vertices, %d triangles, groups %s -> %s" % (len(m.pos), tris, ", ".join(m.groups), a.out))


if __name__ == "__main__":
    main()
