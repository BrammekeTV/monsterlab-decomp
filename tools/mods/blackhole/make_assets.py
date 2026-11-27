"""Black hole head: makes every picture and the model of the mod from code. No game data is read or written.

    python tools/mods/blackhole/make_assets.py [--out DIR]

Writes (default DIR: build/mods/BlackHoleHead, never committed):
  textures/bho_h_01.png        256x256 colour sheet (the size the game's heads use)
  textures/bho_h_01spec.png    256x256 gloss sheet (bright = shiny; only the emitter shines)
  icons/p_hd_sha077.png        64x64 part icon      (the game's part icons: 64x64, and 256x256 for "_h")
  icons/p_hd_sha077_h.png      256x256 part icon
  model/blackhole.obj          the model, for a look in any viewer
  model/blackhole.json         positions, normals and uvs (three separate lists) and the triangle strips per group,
                               each strip vertex a triple [position, normal, uv], for build_mod.py
Needs numpy and Pillow.

The design is the user's concept (black_hole_concept: render_black_hole.py), built as real geometry:
  horizon   black sphere
  disk      flat accretion disc around it, leaning towards the front, streaky and hot at the inner edge
  arc_top   the far side of the disc as gravity bends it: a wide arc standing over the top of the hole
  arc_low   the thin arc under it
  photon    a thin bright ring hugging the horizon
  corona    a slightly larger sphere turned inside out (only its far inner wall is drawn): the soft glow behind
  emitter   the stabilizer on the neck, with an amber cap pointing up at the hole
In the concept the arcs and the ring always face the camera and everything glowing is added to the picture; the
game's head material is one opaque, lit material, so here they face the front of the head, are drawn from both
sides, and fade to a dark red where the concept fades to nothing.

The sheet's layout (u right, v down, in pixels of 256):
  rows   0..63    disc: u once around the hole, v from the inner to the outer edge
  rows  64..127   corona
  rows 128..159   photon ring (v across the band)
  rows 160..255   left half black (the horizon), right half the emitter's metal with its amber cap on top
"""
import argparse
import json
import math
import os

import numpy as np
from PIL import Image

MODEL_KEY = "p_hd_sha077"      # "p_" + limb + "_" + animation set + "_" + model number (PartAnimKey, 0x802152FC)


# ---------- textures (after the concept's render_black_hole.py) ----------
def vnoise(h, w, fy, fx, seed):
    r = np.random.default_rng(seed).random((fy + 1, fx))
    r[fy] = r[0]
    y = np.linspace(0, fy, h, endpoint=False)
    x = np.linspace(0, fx, w, endpoint=False)
    y0, x0 = np.floor(y).astype(int), np.floor(x).astype(int)
    ty, tx = (y - y0)[:, None], (x - x0)[None, :]
    ty, tx = ty * ty * (3 - 2 * ty), tx * tx * (3 - 2 * tx)
    x1, y1 = (x0 + 1) % fx, np.minimum(y0 + 1, fy)
    a, b, c, d = r[y0][:, x0], r[y0][:, x1], r[y1][:, x0], r[y1][:, x1]
    return (a * (1 - tx) + b * tx) * (1 - ty) + (c * (1 - tx) + d * tx) * ty


def fbm(h, w, fy, fx, octaves, seed):
    s, amp, tot = 0, 1.0, 0.0
    for o in range(octaves):
        s = s + amp * vnoise(h, w, fy * 2 ** o, fx * 2 ** o, seed + o)
        tot += amp
        amp *= 0.5
    return s / tot


def hexc(h):
    return tuple(int(h[i:i + 2], 16) / 255 for i in (1, 3, 5))


def ramp(v, stops):
    v = np.clip(v, 0, 1)
    out = np.zeros(v.shape + (3,))
    xs = [s[0] for s in stops]
    for c in range(3):
        out[..., c] = np.interp(v, xs, [s[1][c] for s in stops])
    return out


DARK = np.array(hexc("#3a0f1c"))      # what the glow fades to at its edges (the game's material has no see-through)


def disk_tex(h, w):
    """Orbital streaks: fast across the radius, slow along the orbit; white-hot inside, red outside."""
    v = np.linspace(0, 1, h)[:, None] * np.ones((1, w))
    streak = fbm(h, w, 10, 6, 4, 3)
    fine = fbm(h, w, 16, 24, 3, 9)
    clump = fbm(h, w, 3, 4, 2, 21)
    col = ramp(v, [(0, hexc("#fff7e6")), (0.18, hexc("#ffd98a")), (0.45, hexc("#ff9a3c")), (0.75, hexc("#d9482a")), (1, hexc("#6e1c3a"))])
    inten = (1.25 * (1 - v) ** 1.3 + 0.18) * (0.45 + 0.9 * streak ** 1.4) * (0.75 + 0.5 * fine) * (0.8 + 0.4 * clump)
    fade = np.clip((1 - v) / 0.3, 0, 1) ** 1.5                           # the outer edge dies away
    rgb = np.clip(col * inten[..., None] * 1.6, 0, 1)
    return rgb * fade[..., None] + DARK * (1 - fade[..., None])


def corona_tex(h, w):
    cn = fbm(h, w, 4, 8, 4, 40)
    return np.clip(ramp(cn, [(0, hexc("#ff9a4d")), (0.5, hexc("#ffc27a")), (1, hexc("#fff0d6"))]) * (0.55 + 0.7 * cn[..., None]), 0, 1)


def ring_tex(h, w):
    pv = np.linspace(-1, 1, h)[:, None] * np.ones((1, w))
    pr = np.exp(-(pv * 1.4) ** 2) * (0.85 + 0.3 * fbm(h, w, 2, 16, 2, 60))
    return np.clip(ramp(1 - np.abs(pv), [(0, hexc("#ff9a4d")), (1, hexc("#fff6e4"))]) * pr[..., None] * 1.6, 0, 1)


ROWS = {"disk": (0, 64), "corona": (64, 128), "photon": (128, 160)}      # pixel rows of the sheet


def sheet(size=256):
    rgb = np.zeros((size, size, 3))
    spec = np.zeros((size, size))
    for name, fn in (("disk", disk_tex), ("corona", corona_tex), ("photon", ring_tex)):
        y0, y1 = ROWS[name]
        rgb[y0:y1] = fn(y1 - y0, size)
    # rows 160..255: left half stays black (the horizon); right half the stabilizer: dark gunmetal with a
    # brushed grain, an amber cap (top band), two grooves, no bolts.
    h, w = size - 160, size // 2
    grain = fbm(h, w, 6, 8, 4, 11)
    metal = (0.16 + 0.10 * grain)[..., None] * np.array([0.85, 0.92, 1.0])
    vy = np.linspace(0, 1, h)[:, None] + 0 * grain
    cap = np.clip(1 - vy / 0.22, 0, 1)
    amber = ramp(0.45 + 0.5 * cap, [(0, hexc("#6e1c3a")), (0.5, hexc("#ff9a3c")), (1, hexc("#fff0d6"))])
    metal = metal * (1 - cap[..., None]) + amber * cap[..., None]
    for band in (0.45, 0.72):
        metal *= (1 - 0.6 * np.exp(-((vy - band) / 0.02) ** 2))[..., None]
    rgb[160:, w:] = np.clip(metal, 0, 1)
    spec[160:, w:] = (0.55 + 0.35 * grain) * (1 - cap)
    return rgb, spec


def icon(size):
    """The concept seen from the front: horizon, the disc as a flat ellipse in front, its arcs over and under, the
    photon ring, a soft corona, the emitter below."""
    ss = 4
    n = size * ss
    y, x = np.mgrid[0:n, 0:n]
    x = (x + 0.5) / n * 2 - 1
    y = (y + 0.5) / n * 2 - 1 + 0.14                                      # the hole sits a little above the middle
    r = np.hypot(x, y)
    ang = np.arctan2(y, x)
    rc = 0.30                                                            # horizon radius
    noise = fbm(n, n, 6, 6, 4, 5)
    streaks = 0.55 + 0.45 * np.sin(ang * 5 + r * 30 + noise * 6)
    glow = np.zeros((n, n))
    heat = np.zeros((n, n))                                              # 1 = white-hot, 0 = red

    def add(g, t):
        nonlocal glow, heat
        heat = np.where(g > glow, t, heat)
        glow = np.maximum(glow, g)

    add(0.55 * np.exp(-((r - rc * 1.05) / (rc * 0.35)) ** 2), 0.5)                                   # corona
    top = np.exp(-((np.hypot(x, y / 0.92) - rc * 1.34) / (rc * 0.24)) ** 2) * (y < 0.02)
    add(top * (0.6 + 0.4 * streaks), 0.75 - 0.5 * np.clip((r - rc) / (rc * 0.6), 0, 1))              # arc over the top
    low = np.exp(-((r - rc * 1.12) / (rc * 0.08)) ** 2) * (y > -0.02)
    add(0.7 * low, 0.6)                                                                              # thin arc under
    add(np.exp(-((r - rc * 1.02) / (rc * 0.035)) ** 2), 1.0)                                         # photon ring
    hole = np.clip((rc - r) / 0.01, 0, 1)
    glow *= 1 - hole
    # the disc: an ellipse band across the front, over the lower half of the horizon
    e = np.hypot(x / 3.2, (y - 0.03) / 0.62) / rc
    band = np.clip((e - 0.42) / 0.05, 0, 1) * np.clip((1.0 - e) / 0.3, 0, 1) ** 1.2
    front = (y > 0.03) | (np.abs(x) > rc)                                # the far half is hidden behind the horizon
    band = band * front
    lanes = 0.6 + 0.4 * np.sin(e * 40 + noise * 5)
    dheat = np.clip(1.25 - e * 1.1, 0, 1)
    on = band > 0.02
    glow = np.where(on, np.maximum(glow * (1 - band), band * lanes), glow)
    heat = np.where(on, dheat, heat)
    col = ramp(heat, [(0, hexc("#d9482a")), (0.45, hexc("#ff9a3c")), (0.8, hexc("#ffd98a")), (1, hexc("#fff7e6"))]) * np.clip(glow * 1.5, 0, 1)[..., None]
    a = np.maximum(np.clip(glow * 1.6, 0, 1), hole)
    # emitter: a small dark column with an amber cap under the hole
    ey = y - 0.14
    body = (np.abs(x) < 0.13 - 0.04 * np.clip((ey - 0.72) / 0.2, 0, 1)) & (ey > 0.66) & (ey < 0.98)
    capm = (np.abs(x) < 0.11) & (ey > 0.66) & (ey < 0.72)
    metal = np.array([0.15, 0.17, 0.20]) * (0.8 + 0.5 * noise)[..., None]
    col = np.where(body[..., None], metal, col)
    col = np.where(capm[..., None], np.array(hexc("#ffc27a")) * (0.8 + 0.3 * noise)[..., None], col)
    a = np.maximum(a, body.astype(float))
    rgba = np.dstack([np.clip(col, 0, 1), np.clip(a, 0, 1)])
    return Image.fromarray((rgba * 255).astype(np.uint8), "RGBA").resize((size, size), Image.LANCZOS)


# ---------- model ----------
class Mesh:
    """Positions, normals and uvs are three lists of their own (a vertex names one of each), as the game's meshes."""

    def __init__(self):
        self.pos, self.nrm, self.uv, self.strips = [], [], [], {}
        self._n, self._t = {}, {}

    def p(self, v):
        self.pos.append([round(float(c), 5) for c in v])
        return len(self.pos) - 1

    def n(self, v):
        l = math.sqrt(sum(float(c) * float(c) for c in v)) or 1.0
        key = tuple(round(float(c) / l, 3) for c in v)
        if key not in self._n:
            self._n[key] = len(self.nrm)
            self.nrm.append(list(key))
        return self._n[key]

    def t(self, u, v):
        key = (round(float(u), 4), round(float(v), 4))
        if key not in self._t:
            self._t[key] = len(self.uv)
            self.uv.append(list(key))
        return self._t[key]

    def strip(self, group, xs, ys):
        """Triangle strip x0 y0 x1 y1 ... of vertices (position, normal, uv)."""
        self.strips.setdefault(group, []).append([list(v) for pair in zip(xs, ys) for v in pair])

    def facing(self, xs, ys, n):
        """True when the strip x0 y0 x1 ... is counter-clockwise seen from the side the normal n points to."""
        a, b, c = (np.array(self.pos[v[0]]) for v in (xs[0], ys[0], xs[1]))
        return float(np.dot(np.cross(b - a, c - a), n)) > 0

    def band(self, group, row_a, row_b, n):
        """A flat band between two rows of (position, uv), drawn from both sides (the game culls back faces)."""
        for side in (np.array(n, float), -np.array(n, float)):
            ni = self.n(side)
            xs = [(p, ni, t) for p, t in row_a]
            ys = [(p, ni, t) for p, t in row_b]
            if not self.facing(xs, ys, side):
                xs, ys = ys, xs
            self.strip(group, xs, ys)


def build_model():
    """Part space: origin at the neck socket, +y up, +z to the front. R is the horizon's radius; build_mod.py scales
    and places the whole for the template's skeleton. Triangles are counter-clockwise seen from outside, as the
    template's; the corona's are the other way round (and its normals point inward)."""
    m = Mesh()
    R = 0.13
    C = np.array([0.0, 0.36, 0.0])
    row = lambda name, v: (ROWS[name][0] + 0.5 + v * (ROWS[name][1] - ROWS[name][0] - 1)) / 256.0

    def sphere(group, radius, seg, rings, uv, inside_out):
        grid = []
        for j in range(rings + 1):
            th = math.pi * j / rings
            r = []
            for i in range(seg + 1):
                ph = 2 * math.pi * i / seg
                nv = np.array([math.sin(th) * math.sin(ph), math.cos(th), math.sin(th) * math.cos(ph)])
                r.append((m.p(C + radius * nv), m.n(-nv if inside_out else nv), m.t(*uv(i / seg, j / rings))))
            grid.append(r)
        for j in range(rings):
            m.strip(group, *((grid[j + 1], grid[j]) if inside_out else (grid[j], grid[j + 1])))

    # horizon: one black spot of the sheet
    sphere("horizon", R, 12, 8, lambda u, v: (0.25, 0.85), False)
    # corona: the inverted hull, 1.28 R as in the concept
    sphere("corona", R * 1.28, 12, 8, lambda u, v: (0.004 + 0.992 * u, row("corona", v)), True)

    # accretion disc: 24 segments, two bands, in a plane that leans 13 degrees towards the front (the concept's
    # camera looks down on it by that much)
    lean = math.radians(13)
    cl, sl = math.cos(lean), math.sin(lean)
    tilt = lambda v: np.array([v[0], v[1] * cl - v[2] * sl, v[1] * sl + v[2] * cl])
    radii, segs = (1.35, 2.2, 3.3), 24
    rings = []
    for k, rad in enumerate(radii):
        rings.append([(m.p(C + tilt(np.array([rad * R * math.cos(2 * math.pi * i / segs), 0.0, rad * R * math.sin(2 * math.pi * i / segs)]))),
                       m.t(0.004 + 0.992 * i / segs, row("disk", k / (len(radii) - 1)))) for i in range(segs + 1)])
    for k in range(len(radii) - 1):
        m.band("disk", rings[k], rings[k + 1], tilt(np.array([0.0, 1.0, 0.0])))

    # the lensed arcs and the photon ring stand upright, facing the front, just in front of the hole's middle
    def arc(group, rin, rout, a0, a1, n, squash, tex, zoff, u0=0.0, u1=1.0):
        inner, outer = [], []
        for i in range(n + 1):
            a = a0 + (a1 - a0) * i / n
            u = 0.004 + 0.992 * (u0 + (u1 - u0) * i / n)
            for rad, rw, v in ((rin, inner, 0.0), (rout, outer, 1.0)):
                rw.append((m.p(C + np.array([rad * R * math.cos(a), rad * R * squash * math.sin(a), zoff * R])), m.t(u, row(tex, v))))
        m.band(group, inner, outer, (0.0, 0.0, 1.0))

    arc("arc_top", 1.06, 1.62, 0.0, math.pi, 12, 0.92, "disk", 0.2, 0.0, 0.5)             # over the top
    arc("arc_low", 1.03, 1.22, math.pi, 2 * math.pi, 12, 0.95, "disk", 0.2, 0.5, 1.0)     # the thin arc underneath
    arc("photon", 0.99, 1.07, 0.0, 2 * math.pi, 20, 1.0, "photon", 0.05)

    # emitter: a lathed column, its uvs in the bottom right part of the sheet (amber cap at the top)
    profile = [(0.060, 0.000, 1.00), (0.070, 0.020, 0.90), (0.050, 0.045, 0.72), (0.050, 0.075, 0.55),
               (0.066, 0.095, 0.45), (0.066, 0.110, 0.24), (0.040, 0.125, 0.10), (0.0, 0.125, 0.0)]
    segs, cols = 8, []
    for i in range(segs + 1):
        a = 2 * math.pi * i / segs
        col = []
        for k, (r, y, v) in enumerate(profile):
            r2, y2 = profile[min(k + 1, len(profile) - 1)][:2]
            r1, y1 = profile[max(k - 1, 0)][:2]
            slope = (-(r2 - r1), (y2 - y1))                               # outward normal of the profile, turned
            nv = (math.sin(a) * slope[1], slope[0], math.cos(a) * slope[1])
            col.append((m.p((r * math.sin(a), y, r * math.cos(a))), m.n(nv), m.t(0.504 + 0.492 * i / segs, (160.5 + 95.0 * v) / 256.0)))
        cols.append(col)
    for i in range(segs):
        m.strip("emitter", cols[i + 1], cols[i])
    return m


def triangles(strip):
    for i in range(len(strip) - 2):
        a, b, c = strip[i], strip[i + 1], strip[i + 2]
        if a[0] != b[0] and b[0] != c[0] and a[0] != c[0]:
            yield (a, b, c) if i % 2 == 0 else (b, a, c)


def write_obj(m, path):
    with open(path, "w") as f:
        f.write("# Black hole head, made by tools/mods/blackhole/make_assets.py\n")
        for p in m.pos:
            f.write("v %g %g %g\n" % tuple(p))
        for t in m.uv:
            f.write("vt %g %g\n" % (t[0], 1 - t[1]))
        for n in m.nrm:
            f.write("vn %g %g %g\n" % tuple(n))
        for g, strips in m.strips.items():
            f.write("g %s\n" % g)
            for s in strips:
                for tri in triangles(s):
                    f.write("f " + " ".join("%d/%d/%d" % (v[0] + 1, v[2] + 1, v[1] + 1) for v in tri) + "\n")


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
        json.dump({"key": MODEL_KEY, "positions": m.pos, "normals": m.nrm, "uvs": m.uv, "strips": m.strips,
                   "winding": "ccw-outside", "inverted": ["corona"], "two_sided": ["disk", "arc_top", "arc_low", "photon"],
                   # wishes the port cannot grant yet (one opaque, lit material):
                   "additive": ["disk", "arc_top", "arc_low", "photon", "corona"],
                   "spin": {"group": "disk", "by": "texture scroll along u", "turns_per_second": 0.08}}, f)
    tris = sum(1 for ss in m.strips.values() for s in ss for _ in triangles(s))
    print("black hole head: %d positions, %d normals, %d uvs, %d triangles, groups %s -> %s"
          % (len(m.pos), len(m.nrm), len(m.uv), tris, ", ".join(m.strips), a.out))


if __name__ == "__main__":
    main()
