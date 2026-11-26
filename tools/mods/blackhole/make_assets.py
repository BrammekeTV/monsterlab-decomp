"""Black hole head: makes every picture and the model of the mod from code. No game data is read or written.

    python tools/mods/blackhole/make_assets.py [--out DIR]

Writes (default DIR: build/mods/BlackHoleHead, never committed):
  textures/bho_h_01.png        256x256 colour sheet (the size the game's heads use)
  textures/bho_h_01spec.png    256x256 gloss sheet (bright = shiny; only the emitter shines)
  icons/p_hd_sha077.png        64x64 part icon      (the game's part icons: 64x64, and 256x256 for "_h")
  icons/p_hd_sha077_h.png      256x256 part icon
  model/blackhole.obj          the model, for a look in any viewer
  model/blackhole.json         the same as arrays (positions, normals, uvs, triangles per group) for the zone builder
Needs numpy and Pillow.

The sheet's layout (u right, v down, 0..1):
  top half             the accretion disc: u runs once around the hole, v from the inner to the outer edge
  bottom left quarter  the core (black) with the photon ring at its rim
  bottom right quarter the stabilizer: brushed dark metal, an amber emitter cap, the field cone
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


def disc_strip(w, h):
    """The disc unrolled: long streaks along u (sheared, so they swirl), brightest at the inner edge."""
    v = np.linspace(0, 1, h)[:, None]
    u = np.linspace(0, 1, w, endpoint=False)[None, :]
    streak = np.zeros((h, w))
    for k, (rows, seed) in enumerate(((24, 1), (48, 2), (96, 3))):
        n = noise((rows, w // 8), seed, octaves=3)
        img = np.asarray(Image.fromarray((n * 255).astype(np.uint8)).resize((w, h), Image.BICUBIC)) / 255.0
        shift = (v[:, 0] * w * (0.35 + 0.2 * k)).astype(int)            # shear: the outer matter trails behind
        img = np.stack([np.roll(img[y], shift[y]) for y in range(h)])
        streak += img / (k + 1)
    streak /= streak.max()
    clumps = noise((h, w), 7, octaves=3)
    heat = (1 - v) ** 1.3 * 0.75 + 0.25                                  # hot inside, cooler outside
    edge = np.clip(v / 0.06, 0, 1) * np.clip((1 - v) / 0.25, 0, 1)        # soft inner rim, ragged fade outside
    t = (0.25 + 0.95 * streak ** 1.6) * heat * (0.75 + 0.35 * clumps) * edge + 0 * u
    rgb = amber(t)
    alpha = np.clip(edge * (0.35 + streak) * 1.6, 0, 1)
    return rgb, alpha


def sheet(size=256):
    half = size // 2
    rgb = np.zeros((size, size, 3))
    spec = np.zeros((size, size))
    d_rgb, _ = disc_strip(size, half)
    rgb[:half] = d_rgb
    # Core: a disc of darkness, a hair of deep blue towards the rim, the photon ring at the rim.
    y, x = np.mgrid[0:half, 0:half]
    r = np.hypot(x - half / 2 + 0.5, y - half / 2 + 0.5) / (half / 2)
    core = np.zeros((half, half, 3))
    core += np.clip((r - 0.55) / 0.4, 0, 1)[..., None] ** 2 * np.array([0.0, 0.10, 0.16])
    ring = np.exp(-((r - 0.93) / 0.035) ** 2)
    core += amber(ring * 0.95) * ring[..., None]
    rgb[half:, :half] = np.clip(core, 0, 1)
    # Stabilizer: dark gunmetal with a brushed grain, an amber cap (top band), the field cone (right strip).
    grain = noise((half, half), 11, octaves=4, wrap_x=True)
    metal = (0.16 + 0.10 * grain)[..., None] * np.array([0.85, 0.92, 1.0])
    vy = np.linspace(0, 1, half)[:, None] + 0 * grain
    cap = np.clip(1 - vy / 0.22, 0, 1)
    metal = metal * (1 - cap[..., None]) + amber(0.45 + 0.5 * cap) * cap[..., None]
    for band in (0.45, 0.72):                                            # two grooves, no bolts
        metal *= (1 - 0.6 * np.exp(-((vy - band) / 0.02) ** 2))[..., None]
    ux = np.linspace(0, 1, half)[None, :] + 0 * grain
    cone = np.clip((ux - 0.75) / 0.25, 0, 1) > 0
    beam = amber(0.35 + 0.6 * (1 - vy) * (0.6 + 0.4 * grain))
    metal = np.where(cone[..., None], beam, metal)
    rgb[half:, half:] = np.clip(metal, 0, 1)
    spec[half:, half:] = np.where(cone, 0.0, 0.55 + 0.35 * grain)
    return rgb, spec


def icon(size):
    """The hole seen as in the reference: tilted disc in front, its light bent over the top and under the core."""
    ss = 4
    n = size * ss
    y, x = np.mgrid[0:n, 0:n]
    x = (x + 0.5) / n * 2 - 1
    y = (y + 0.5) / n * 2 - 1
    tilt = math.radians(-14)
    xr = x * math.cos(tilt) - y * math.sin(tilt)
    yr = x * math.sin(tilt) + y * math.cos(tilt)
    r = np.hypot(xr, yr)
    ang = np.arctan2(yr, xr)
    swirl = noise((n, n), 5, octaves=4, wrap_x=False)
    streaks = 0.5 + 0.5 * np.sin(ang * 9 + r * 26 + swirl * 5)
    col = np.zeros((n, n, 3))
    a = np.zeros((n, n))
    # The lensed light: a thick halo over the top, a thinner one below.
    for rad, wid, mask in ((0.50, 0.12, yr < 0.05), (0.42, 0.07, yr > -0.05)):
        halo = np.exp(-((r - rad) / wid) ** 2) * mask
        col += amber(halo * (0.55 + 0.45 * streaks)) * halo[..., None]
        a = np.maximum(a, halo)
    core = r < 0.30
    col[core] = 0
    a = np.maximum(a, np.clip((0.31 - r) / 0.02, 0, 1))
    ring = np.exp(-((r - 0.305) / 0.012) ** 2)
    col += amber(ring)[...] * ring[..., None] * 0.9
    # The disc in front: a thin, long ellipse across the middle.
    band = np.exp(-(yr / 0.055) ** 2) * np.clip((0.98 - np.abs(xr)) / 0.25, 0, 1)
    lane = 0.6 + 0.4 * np.sin(xr * 30 + swirl * 6)
    front = amber(band * (0.5 + 0.5 * lane) * 1.05)
    col = col * (1 - band[..., None]) + front * band[..., None]
    a = np.maximum(a, np.clip(band * 1.4, 0, 1))
    rgba = np.dstack([np.clip(col, 0, 1), np.clip(a, 0, 1)])
    img = Image.fromarray((rgba * 255).astype(np.uint8), "RGBA")
    return img.resize((size, size), Image.LANCZOS)


class Mesh:
    def __init__(self):
        self.pos, self.nrm, self.uv, self.groups = [], [], [], {}

    def vert(self, p, n, uv):
        self.pos.append([round(float(c), 5) for c in p])
        l = math.sqrt(sum(c * c for c in n)) or 1.0
        self.nrm.append([round(float(c) / l, 4) for c in n])
        self.uv.append([round(float(c), 5) for c in uv])
        return len(self.pos) - 1

    def quad(self, group, a, b, c, d):
        self.groups.setdefault(group, []).extend([[a, b, c], [a, c, d]])


def build_model():
    """Part space: origin at the neck socket, +y up, +z to the front; units as the game's heads (about 0.5 across).

    core       dark sphere, radius R, centre C
    photon     thin ring hugging the core, facing the front
    disc       flat ring around the core, tilted like the reference
    lens_top / lens_bottom   the disc's light bent over and under the core: half rings standing upright behind it
    emitter    the stabilizer on the neck: a tapered column with an amber cap
    field      an open cone of light from the cap up to the core
    The uvs point into the sheet of make_assets.sheet(). disc, lens_*, photon and field are meant to be drawn
    additive and unlit; core and emitter are ordinary lit surfaces.
    """
    m = Mesh()
    R = 0.13
    C = np.array([0.0, 0.34, 0.0])
    # core: 12 x 8 sphere, planar uv into the bottom left quarter (centre = black)
    seg, rings = 12, 8
    grid = []
    for j in range(rings + 1):
        th = math.pi * j / rings
        row = []
        for i in range(seg + 1):
            ph = 2 * math.pi * i / seg
            n = np.array([math.sin(th) * math.sin(ph), math.cos(th), math.sin(th) * math.cos(ph)])
            row.append(m.vert(C + R * n, n, (0.25 + 0.2 * n[0], 0.75 - 0.2 * n[1])))
        grid.append(row)
    for j in range(rings):
        for i in range(seg):
            m.quad("core", grid[j][i], grid[j + 1][i], grid[j + 1][i + 1], grid[j][i + 1])

    def ring(group, r0, r1, a0, a1, segs, frame, v0, v1, u0=0.0, u1=1.0, lift=0.0):
        """A flat band between radii r0 and r1 from angle a0 to a1 in the plane frame = (x axis, y axis, normal)."""
        ex, ey, en = (np.array(v, float) for v in frame)
        prev = None
        for i in range(segs + 1):
            t = i / segs
            a = a0 + (a1 - a0) * t
            d = ex * math.cos(a) + ey * math.sin(a)
            u = u0 + (u1 - u0) * t
            cur = (m.vert(C + d * r0 + en * lift, en, (u, v0)), m.vert(C + d * r1 + en * lift, en, (u, v1)))
            if prev:
                m.quad(group, prev[0], prev[1], cur[1], cur[0])
            prev = cur

    tilt = math.radians(14)
    dx = (math.cos(tilt), -math.sin(tilt), 0.0)             # the disc dips to the right, as in the reference
    dz = (0.0, 0.0, 1.0)
    dn = (math.sin(tilt), math.cos(tilt), 0.0)
    ring("disc", R * 1.25, R * 3.1, 0, 2 * math.pi, 24, (dx, dz, dn), 0.02, 0.48)
    up = (math.sin(tilt), math.cos(tilt), 0.0)
    front = (0.0, 0.0, 1.0)
    ring("lens_top", R * 1.08, R * 1.95, 0, math.pi, 14, (dx, up, front), 0.02, 0.34, 0.0, 0.5, lift=-0.01)
    ring("lens_bottom", R * 1.08, R * 1.55, math.pi, 2 * math.pi, 14, (dx, up, front), 0.02, 0.22, 0.5, 1.0, lift=-0.01)
    # photon ring: uv along the bright rim of the core picture
    ex, ey = np.array(dx), np.array(up)
    prev = None
    for i in range(25):
        a = 2 * math.pi * i / 24
        d = ex * math.cos(a) + ey * math.sin(a)
        uvc = np.array([0.25 + 0.232 * math.cos(a), 0.75 - 0.232 * math.sin(a)])
        cur = (m.vert(C + d * R * 1.0 + np.array(front) * 0.005, front, uvc * 0.985 + np.array([0.25, 0.75]) * 0.015),
               m.vert(C + d * R * 1.09 + np.array(front) * 0.005, front, uvc))
        if prev:
            m.quad("photon", prev[0], prev[1], cur[1], cur[0])
        prev = cur

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

    lathe("emitter", [(0.060, 0.000, 1.00), (0.070, 0.020, 0.90), (0.050, 0.045, 0.72), (0.050, 0.075, 0.55),
                      (0.066, 0.095, 0.45), (0.066, 0.110, 0.24), (0.040, 0.125, 0.10), (0.0, 0.125, 0.0)], 10, 0.0, 0.72)
    lathe("field", [(0.036, 0.126, 1.0), (0.085, C[1] - R * 0.75, 0.0)], 10, 0.78, 0.98)
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
    additive = ["disc", "lens_top", "lens_bottom", "photon", "field"]
    with open(os.path.join(a.out, "model", "blackhole.json"), "w") as f:
        json.dump({"key": MODEL_KEY, "positions": m.pos, "normals": m.nrm, "uvs": m.uv, "groups": m.groups,
                   "additive": additive, "spin": {"group": "disc", "axis": "normal", "turns_per_second": 0.15}}, f)
    tris = sum(len(t) for t in m.groups.values())
    print("black hole head: %d vertices, %d triangles, groups %s -> %s" % (len(m.pos), tris, ", ".join(m.groups), a.out))


if __name__ == "__main__":
    main()
