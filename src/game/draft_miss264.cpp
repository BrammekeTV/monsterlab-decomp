// Text layout/quad builder (fn_801BA9C4), trail/ribbon update (fn_801BDB6C) and the
// line-vs-line closest-point squared distance (fn_801BFE38).
#include <math.h>

#include "types.h"
#include "engine/math.h"
#include "engine/engine.h"

extern "C" {
void Vec3SetX(Vec3* v, f32 x);                           // 0x80058230
void Vec3SetY(Vec3* v, f32 y);                           // 0x80058238
void Vec3SetZero(Vec3* out);                             // 0x800531A8
void fn_800446BC(void* cam, s32 v);                      // 0x800446BC
void fn_80121434(u8* v, f32 a, f32 b);                   // 0x80121434 (vertex u/v)
void fn_8012145C(u8* v, f32 x, f32 y, f32 z);            // 0x8012145C (vertex position)
void fn_80151E38(s32 quads, void* verts);                // 0x80151E38
void fn_80155724(u8* m);                                 // 0x80155724
void fn_801BA6F4(void* font, u16* out, const u8* str);   // 0x801BA6F4 (decode string into 16-bit chars)
u16  fn_801BA6E4(u8* font, u16 ch);                      // 0x801BA6E4 (glyph index of a char)
void fn_801BA74C(void* font, u16* text, s32 first, s32 last, f32* outW, f32* outH); // 0x801BA74C
s32  fn_801BA9BC(void);                                  // 0x801BA9BC (stub, returns 0; the original is called with a char)
u8*  fn_801BB2F8(s32 size);                              // 0x801BB2F8 (temp vertex buffer)
void fn_801BB338(u8* v, u32 color);                      // 0x801BB338
void fn_801BB374(s32 size);                              // 0x801BB374 (commit the temp buffer)
u8*  fn_801D5850(void);                                  // 0x801D5850
u8*  fn_801D6D78(void);                                  // 0x801D6D78 (camera record: axes at +0/+0xC)
void fn_801BC970(u8* obj, s32 a, s32 b, s32 c, f32 f);  // 0x801BC970
void fn_801BD828(u8* obj);                               // 0x801BD828

// Text scratch block: 0x000 = decode buffer of fn_801BA8C8, 0x800 = four Vec3 (result bounds),
// 0x830 and 0x1030 = 16-bit char buffers of the two layout modes.
extern u16 lbl_80413C40[];   // 0x80413C40
extern u8  lbl_805FB6D8;     // 0x805FB6D8 (result record initialised flag)
extern u8  lbl_8047C640[];   // 0x8047C640 (camera)
}

extern "C" {

// Horizontal alignment: 1 = centre, 2 = right, otherwise left.
static f32 AlignX(s32 mode, f32 x, f32 w) {
    if (mode == 1) return x - 0.5f * w;
    if (mode == 2) return x - w;
    return x;
}

// Index of the last char of the line starting at `start` (a '\n' ends it, one char before it).
static u32 LineEnd(const u16* base, u32 start, u32 end) {
    u32 last = end;
    u32 found = 0;
    const u16* p = base + start;
    for (u32 i = start; i < end && found == 0; i++, p++) {
        if (*p == 0xA && i > start) {
            found = 1;
            last = i - 1;
        }
    }
    return last;
}

// One quad corner: texture coordinates, colour, position.
static void PutVert(u8* v, f32 u, f32 w, u32 color, f32 x, f32 y, f32 z) {
    fn_80121434(v, u, w);
    fn_801BB338(v, color);
    fn_8012145C(v, x, y, z);
}

// 0x801BA9C4
// Lays out chars [first, last] of a string as textured quads and draws them. Returns the bounds
// record (four Vec3: top-left, top-right, bottom-left, bottom-right). hAlign/vAlign: 0 start,
// 1 centre, 2 end. Two modes depending on the first byte of the game settings: line-by-line
// layout with newlines (settings != 0) or a single scaled run (settings == 0).
void* fn_801BA9C4(u8* font, s32 flag, u8* str, u32 first, u32 last, u32* colors, u32 colorCount,
                  s32 hAlign, f32 x0, f32 y0, f32 z0, f32 scale, s32 vAlign) {
    u8* rec = (u8*)lbl_80413C40 + 0x800;
    f32 y = y0;

    if ((s8)lbl_805FB6D8 == 0) {
        Vec3SetZero((Vec3*)rec);
        Vec3SetZero((Vec3*)(rec + 0xC));
        Vec3SetZero((Vec3*)(rec + 0x18));
        Vec3SetZero((Vec3*)(rec + 0x24));
        lbl_805FB6D8 = 1;
    }

    if (EngineGetGameSettings()->skipDebugOverlay != 0) {
        if (str == NULL) return rec;

        u16* text = (u16*)((u8*)lbl_80413C40 + 0x830);
        u32 idx = first;
        u32 colorsUsed = 1;
        f32 w0, h0, wTotal, hTotal;

        fn_801BA6F4(font, text, str);
        f32 maxX = 0.0f;
        f32 minX = 0.0f;
        u32 lineLast = LineEnd(text, idx, last);
        fn_801BA74C(font, text, idx, lineLast, &w0, &h0);
        fn_801BA74C(font, text, idx, last, &wTotal, &hTotal);

        f32 x = AlignX(hAlign, x0, w0);
        if (vAlign == 1) y -= 0.5f * hTotal;
        else if (vAlign == 2) y -= hTotal;

        Vec3SetX((Vec3*)rec, x);
        Vec3SetY((Vec3*)rec, y);
        Vec3SetY((Vec3*)(rec + 0xC), y);
        Vec3SetX((Vec3*)(rec + 0x18), x);
        if (maxX > x) minX = x;

        f32 vScale = *(f32*)(font + 0x10) / *(f32*)(font + 0x14);
        u8* quads = fn_801BB2F8((s32)(((last - first) + 1) * 4 * 0x18));
        u8* out = quads;
        u16* p = text + idx;
        s32 verts = 0;

        while (idx <= last && *p != 0) {
            u16 ch = *p;
            u32 color = *colors;
            if (colorsUsed < colorCount) {
                colorsUsed++;
                colors++;
            }
            if (ch == 0xA) {
                if (!(maxX >= x)) maxX = x;
                idx++;
                lineLast = LineEnd(text, idx, last);
                fn_801BA74C(font, text, idx, lineLast, &w0, &h0);
                x = AlignX(hAlign, x0, w0);
                if (minX > x) minX = x;
                y += 1.0f;
                p++;
                str++;
            } else {
                u16 glyph = fn_801BA6E4(font, ch);
                u32 off = (u32)glyph * 0xC;
                u8* tbl = *(u8**)(font + 0xC);
                f32 adv = (*(f32*)(font + 0x18) * *(f32*)(tbl + off + 8)) / *(f32*)(font + 0x10);
                if (glyph < *(u32*)(font + 0x1C) && *p != 1) {
                    if (fn_801BA9BC() != 0) color = (color & 0xFF000000u) | 0x00FF0000u | 0xFFFFu;
                    tbl = *(u8**)(font + 0xC);
                    f32 u0 = *(f32*)(tbl + off);
                    f32 u1 = u0 + *(f32*)(tbl + off + 8);
                    f32 v0 = *(f32*)(tbl + off + 4);
                    PutVert(out, u0, v0, color, x, y, z0);
                    PutVert(out + 0x18, u1, v0, color, x + adv, y, z0);
                    PutVert(out + 0x30, u0, vScale + v0, color, x, 1.0f + y, z0);
                    PutVert(out + 0x48, u1, vScale + v0, color, x + adv, 1.0f + y, z0);
                    x += adv;
                    verts += 4;
                    out += 0x60;
                }
                p++;
                str++;
                idx++;
            }
        }

        if (!(maxX >= x)) maxX = x;
        if (idx > last || *p == 0) {
            Vec3SetX((Vec3*)rec, minX);
            Vec3SetX((Vec3*)(rec + 0x18), minX);
            Vec3SetX((Vec3*)(rec + 0x24), maxX);
            Vec3SetY((Vec3*)(rec + 0x24), 1.0f + y);
            Vec3SetY((Vec3*)(rec + 0x18), 1.0f + y);
            Vec3SetX((Vec3*)(rec + 0xC), maxX);
        }

        s32 nQuads = verts >> 2;
        if (nQuads > 0) {
            fn_801BB374(verts * 0x18);
            fn_800446BC(lbl_8047C640, *(s32*)(font + 0x4));
            if (flag != 0) fn_80155724(fn_801D5850());
            fn_80151E38(nQuads, quads);
        }
        return rec;
    }

    // Single scaled run, no line breaks handling beyond restarting the x position.
    if (str == NULL) return rec;

    u16* text = (u16*)((u8*)lbl_80413C40 + 0x1030);
    u32 idx = first;
    u32 colorsUsed = 1;
    u16* p = text + idx;
    f32 w, h;

    fn_801BA6F4(font, text, str);
    fn_801BA74C(font, text, idx, last, &w, &h);
    w = w * scale;
    h = h * scale;

    f32 x = AlignX(hAlign, x0, w);
    if (vAlign == 1) y -= h * 0.5f;
    else if (vAlign == 2) y -= h;

    f32 scaledH = *(f32*)(font + 0x10) * scale;
    f32 vScale = *(f32*)(font + 0x10) / *(f32*)(font + 0x14);
    u8* quads = fn_801BB2F8((s32)(((last - first) + 1) * 4 * 0x18));
    u8* out = quads;
    s32 verts = 0;

    while (idx <= last && *p != 0) {
        u16 ch = *p;
        u32 color = *colors;
        if (colorsUsed < colorCount) {
            colorsUsed++;
            colors++;
        }
        if (ch == 0xA) {
            x = AlignX(hAlign, x0, w);
            p++;
            idx++;
            y -= h;
        } else {
            u16 glyph = fn_801BA6E4(font, ch);
            u32 off = (u32)glyph * 0xC;
            u8* tbl = *(u8**)(font + 0xC);
            f32 adv = scale * (*(f32*)(font + 0x18) * *(f32*)(tbl + off + 8));
            if (glyph < *(u32*)(font + 0x1C)) {
                if (glyph > 0xC2) color = (color & 0xFF000000u) | 0x00C40000u | 0xC6C3u;
                f32 u0 = *(f32*)(tbl + off);
                f32 u1 = u0 + *(f32*)(tbl + off + 8);
                f32 v0 = *(f32*)(tbl + off + 4);
                PutVert(out, u0, vScale + v0, color, x, y, z0);
                PutVert(out + 0x18, u1, vScale + v0, color, x + adv, y, z0);
                PutVert(out + 0x30, u0, v0, color, x, y + scaledH, z0);
                PutVert(out + 0x48, u1, v0, color, x + adv, y + scaledH, z0);
                x += adv;
                verts += 4;
                out += 0x60;
            }
            p++;
            idx++;
        }
    }

    s32 nQuads = verts >> 2;
    if (nQuads > 0) {
        fn_801BB374(verts * 0x18);
        fn_800446BC(lbl_8047C640, *(s32*)(font + 0x4));
        if (flag != 0) fn_80155724(fn_801D5850());
        fn_80151E38(nQuads, quads);
    }
    return rec;
}

// Normalises (x,y,z); a zero vector stays zero.
static void NormalizeOrZero(f32 x, f32 y, f32 z, f32* out) {
    f32 lenSq = z * z + (x * x + y * y);
    if (lenSq == 0.0f) {
        out[0] = out[1] = out[2] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(lenSq);
        out[0] = x * inv;
        out[1] = y * inv;
        out[2] = z * inv;
    }
}

// 0x801BDB6C
// Per-frame update of a trail/ribbon object. obj+0x0 = definition, obj+0x34 = length of the
// polyline, obj+0x38 = point count, obj+0x48 = vertex array (0x30 per point), obj+0x4C = points
// (Vec3, 0xC apart), obj+0x4 + 4*i = attached child trails (copied from point obj+0x1C+4*i).
// Skipped entirely when the definition's flag byte (+0x64 -> +2) is set.
void fn_801BDB6C(u8* obj, f32 dt) {
    u8* def = *(u8**)obj;
    if (*(u8*)(*(u8**)(def + 0x64) + 2) != 0) return;

    // Length of the polyline = distance between first and last point.
    s32 n = *(s32*)(obj + 0x38);
    u8* lastPt = obj + (n - 1) * 0xC;
    f32 dx = *(f32*)(obj + 0x4C) - *(f32*)(lastPt + 0x4C);
    f32 dy = *(f32*)(obj + 0x50) - *(f32*)(lastPt + 0x50);
    f32 dz = *(f32*)(obj + 0x54) - *(f32*)(lastPt + 0x54);
    f32 lenSq = dz * dz + (dx * dx + dy * dy);
    f32 len = 0.0f;
    if (!(lenSq <= 0.0f)) len = sqrtf(lenSq);
    *(f32*)(obj + 0x34) = len;

    fn_801BC970(obj, 0, n - 1, *(s32*)(obj + 0x38), *(f32*)(def + 0x48));
    if (fabsf(*(f32*)(*(u8**)obj + 0x5C)) > 0.01f) fn_801BD828(obj);

    // Camera right/up axes, normalised.
    u8* cam = fn_801D6D78();
    NormalizeOrZero(*(f32*)(cam + 0x0), *(f32*)(cam + 0x4), *(f32*)(cam + 0x8), (f32*)(obj + 0x1D8));
    NormalizeOrZero(*(f32*)(cam + 0xC), *(f32*)(cam + 0x10), *(f32*)(cam + 0x14), (f32*)(obj + 0x1E4));

    const u32 kColorA = 0xFFFFFF00u;   // bytes FF FF FF 00 (lbl_805FFC0C)
    const u32 kColorB = 0xFFFFFFFFu;   // bytes FF FF FF FF (lbl_805FFC10)

    def = *(u8**)obj;
    f32 fade = *(f32*)(obj + 0x1F8);
    u32 sh = (u32)*(s32*)(def + 0x30) & 0x3F;   // slw: shift count is 6 bits
    s32 total = (sh < 32) ? (s32)(1u << sh) : 0;           // number of vertex slots
    s32 period = *(s32*)(def + 0x3C) + *(s32*)(def + 0x40);
    if (fade != -1.0f) {
        f32 v = fade + dt * *(f32*)(def + 0x44);
        *(f32*)(obj + 0x1F8) = v;
        if (v > (f32)total) *(f32*)(obj + 0x1F8) = -1.0f;
    }

    if (period > 4) {
        f32 pos = *(f32*)(obj + 0x1F4) + dt * *(f32*)(*(u8**)obj + 0x44);
        *(f32*)(obj + 0x1F4) = pos;
        if (pos >= (f32)period) *(f32*)(obj + 0x1F4) = 0.0f;

        def = *(u8**)obj;
        typedef void (*TrailCb)(u8*);
        TrailCb cb = *(TrailCb*)(def + 0x6C);
        s32 i = (s32)*(f32*)(obj + 0x1F4) - period;
        s32 segEnd = i + *(s32*)(def + 0x40);
        if (cb != NULL && *(u8*)(obj + 0x45) == 0 && segEnd >= total) {
            cb(obj);
            *(u8*)(obj + 0x45) = 1;
        }
        if (i <= 0 && (segEnd + i) >= total && *(void**)(*(u8**)obj + 0x6C) != NULL) {
            *(u8*)(obj + 0x44) = 1;
        }

        s32 flag = 0;
        s32 off = i * 0x30;
        if (i < total) {
            do {
                if (i == segEnd) {
                    flag = (flag == 0);
                    s32 step = (flag != 0) ? *(s32*)(*(u8**)obj + 0x3C) : *(s32*)(*(u8**)obj + 0x40);
                    segEnd += step;
                }
                if (i >= 0) {
                    f32 f = *(f32*)(obj + 0x1F8);
                    s32 sel = flag;
                    if (f != -1.0f && f < (f32)i) sel = 0;
                    u8* node = *(u8**)(obj + 0x48) + off;
                    f32 tpos = (f32)((f64)i / (f64)(u32)(*(s32*)(obj + 0x38) - 1));
                    u32 color = (sel != 0) ? kColorB : kColorA;
                    *(u32*)(node + 0x8) = color;
                    *(u32*)(node + 0x20) = color;
                    *(f32*)(node + 0x0) = tpos;
                    *(f32*)(node + 0x4) = 0.0f;
                    *(f32*)(node + 0x18) = tpos;
                    *(f32*)(node + 0x1C) = 1.0f;
                }
                off += 0x30;
                i++;
            } while (i < total);
        }
    }

    // Attached child trails: pin each child's first point to our point, and blend the last
    // point of the child toward (our point + quarter of the last segment).
    def = *(u8**)obj;
    f32 blend = *(f32*)(def + 0x50);
    u8* slot = obj;
    for (u32 k = 0; k < *(u32*)(*(u8**)obj + 0x34); k++, slot += 4) {
        u8* child = *(u8**)(slot + 0x4);
        if (child == NULL) continue;

        s32 idx = *(s32*)(slot + 0x1C);
        u8* pt = obj + idx * 0xC;
        *(f32*)(child + 0x4C) = *(f32*)(pt + 0x4C);
        *(f32*)(child + 0x50) = *(f32*)(pt + 0x50);
        *(f32*)(child + 0x54) = *(f32*)(pt + 0x54);

        idx = *(s32*)(slot + 0x1C);
        u8* prev = obj + (idx - 1) * 0xC;
        u8* cur = obj + idx * 0xC;
        f32 ex = *(f32*)(cur + 0x4C) - *(f32*)(prev + 0x4C);
        f32 ey = *(f32*)(cur + 0x50) - *(f32*)(prev + 0x50);
        f32 ez = *(f32*)(cur + 0x54) - *(f32*)(prev + 0x54);
        f32 dir[3];
        NormalizeOrZero(ex, ey, ez, dir);

        f32 inv = 1.0f - blend;
        u8* c2 = *(u8**)(slot + 0x4);
        f32 q = 0.25f * *(f32*)(obj + 0x34);
        f32 ox = dir[0] * q;
        f32 oy = dir[1] * q;
        f32 oz = dir[2] * q;
        f32 tx = ox + *(f32*)(cur + 0x4C);
        u8* tail = c2 + (*(s32*)(c2 + 0x38) - 1) * 0xC;
        f32 ty = oy + *(f32*)(cur + 0x50);
        f32 tz = oz + *(f32*)(cur + 0x54);
        f32 ax = *(f32*)(tail + 0x4C) * blend;
        f32 ay = *(f32*)(tail + 0x50) * blend;
        f32 az = *(f32*)(tail + 0x54) * blend;
        *(f32*)(tail + 0x4C) = tx * inv + ax;
        *(f32*)(tail + 0x50) = ty * inv + ay;
        *(f32*)(tail + 0x54) = tz * inv + az;
    }
}

// 0x801BFE38
// Squared distance between the lines P0 + s*D0 (P0 = arg0, D0 = arg1) and P1 + t*D1 (P1 = arg2,
// D1 = arg3), clamped to s,t in [0,1] (segments; Eberly's closest-points case analysis). The
// segment parameters of the closest points go to *arg4 (s) and *arg5 (t) when non-NULL.
f32 fn_801BFE38(u8* arg0, f32* arg1, u8* arg2, u8* arg3, f32* arg4, f32* arg5) {
    const f32 kTol = 0.000001f;   // lbl_805F9598
    f32 D0x = arg1[0], D0y = arg1[1], D0z = arg1[2];
    f32 D1x = *(f32*)(arg3 + 0), D1y = *(f32*)(arg3 + 4), D1z = *(f32*)(arg3 + 8);
    f32 dx = *(f32*)(arg0 + 0) - *(f32*)(arg2 + 0);
    f32 dy = *(f32*)(arg0 + 4) - *(f32*)(arg2 + 4);
    f32 dz = *(f32*)(arg0 + 8) - *(f32*)(arg2 + 8);

    f32 a = D0z * D0z + (D0x * D0x + D0y * D0y);          // D0.D0
    f32 b = -(D0z * D1z + (D0x * D1x + D0y * D1y));       // -D0.D1
    f32 c = D1z * D1z + (D1x * D1x + D1y * D1y);          // D1.D1
    f32 d = dz * D0z + (dx * D0x + dy * D0y);             // diff.D0
    f32 e = -(dz * D1z + (dx * D1x + dy * D1y));          // -diff.D1
    f32 f = dz * dz + (dx * dx + dy * dy);                // diff.diff
    f32 det = fabsf(a * c - b * b);
    f32 s = 0.0f;
    f32 t = 0.0f;
    f32 q, nq;

    if (det >= kTol) {
        f32 sn = b * e - c * d;
        f32 tn = b * d - a * e;
        if (sn >= 0.0f) {
            if (sn <= det) {
                if (tn >= 0.0f) {
                    if (tn <= det) {
                        // interior
                        f32 inv = 1.0f / det;
                        s = sn * inv;
                        t = tn * inv;
                        f += (s * ((2.0f * d) + ((a * s) + (b * t)))) + (t * ((2.0f * e) + ((b * s) + (c * t))));
                    } else {
                        // t > 1 edge
                        q = b + d;
                        t = 1.0f;
                        if (q >= 0.0f) {
                            s = 0.0f;
                            f += c + (2.0f * e);
                        } else {
                            nq = -q;
                            if (nq >= a) {
                                s = 1.0f;
                                f = f + (a + c) + (2.0f * (e + q));
                            } else {
                                s = nq / a;
                                f += (2.0f * e) + (c + (q * s));
                            }
                        }
                    }
                } else {
                    // t < 0 edge
                    t = 0.0f;
                    if (d >= 0.0f) {
                        s = 0.0f;
                    } else {
                        nq = -d;
                        if (nq >= a) {
                            s = 1.0f;
                            f += a + (2.0f * d);
                        } else {
                            s = nq / a;
                            f += d * s;
                        }
                    }
                }
            } else {
                // s > 1
                if (tn >= 0.0f) {
                    if (tn <= det) {
                        q = b + e;
                        s = 1.0f;
                        if (q >= 0.0f) {
                            t = 0.0f;
                            f += a + (2.0f * d);
                        } else {
                            nq = -q;
                            if (nq >= c) {
                                t = 1.0f;
                                f = f + (a + c) + (2.0f * (d + q));
                            } else {
                                t = nq / c;
                                f += (2.0f * d) + (a + (q * t));
                            }
                        }
                    } else {
                        q = b + d;
                        nq = -q;
                        if (nq <= a) {
                            t = 1.0f;
                            if (q >= 0.0f) {
                                s = 0.0f;
                                f += c + (2.0f * e);
                            } else {
                                s = nq / a;
                                f += (2.0f * e) + (c + (q * s));
                            }
                        } else {
                            q = b + e;
                            s = 1.0f;
                            if (q >= 0.0f) {
                                t = 0.0f;
                                f += a + (2.0f * d);
                            } else {
                                nq = -q;
                                if (nq >= c) {
                                    t = 1.0f;
                                    f = f + (a + c) + (2.0f * (d + q));
                                } else {
                                    t = nq / c;
                                    f += (2.0f * d) + (a + (q * t));
                                }
                            }
                        }
                    }
                } else {
                    nq = -d;
                    if (nq < a) {
                        t = 0.0f;
                        if (d >= 0.0f) {
                            s = 0.0f;
                        } else {
                            s = nq / a;
                            f += d * s;
                        }
                    } else {
                        q = b + e;
                        s = 1.0f;
                        if (q >= 0.0f) {
                            t = 0.0f;
                            f += a + (2.0f * d);
                        } else {
                            nq = -q;
                            if (nq >= c) {
                                t = 1.0f;
                                f = f + (a + c) + (2.0f * (d + q));
                            } else {
                                t = nq / c;
                                f += (2.0f * d) + (a + (q * t));
                            }
                        }
                    }
                }
            }
        } else {
            // s < 0
            if (tn >= 0.0f) {
                if (tn <= det) {
                    s = 0.0f;
                    if (e >= 0.0f) {
                        t = 0.0f;
                    } else {
                        nq = -e;
                        if (nq >= c) {
                            t = 1.0f;
                            f += c + (2.0f * e);
                        } else {
                            t = nq / c;
                            f += e * t;
                        }
                    }
                } else {
                    q = b + d;
                    if (q < 0.0f) {
                        nq = -q;
                        t = 1.0f;
                        if (nq >= a) {
                            s = 1.0f;
                            f = f + (a + c) + (2.0f * (e + q));
                        } else {
                            s = nq / a;
                            f += (2.0f * e) + (c + (q * s));
                        }
                    } else {
                        s = 0.0f;
                        if (e >= 0.0f) {
                            t = 0.0f;
                        } else {
                            nq = -e;
                            if (nq >= c) {
                                t = 1.0f;
                                f += c + (2.0f * e);
                            } else {
                                t = nq / c;
                                f += e * t;
                            }
                        }
                    }
                }
            } else {
                if (d < 0.0f) {
                    nq = -d;
                    t = 0.0f;
                    if (nq >= a) {
                        s = 1.0f;
                        f += a + (2.0f * d);
                    } else {
                        s = nq / a;
                        f += d * s;
                    }
                } else {
                    s = 0.0f;
                    if (e >= 0.0f) {
                        t = 0.0f;
                    } else {
                        nq = -e;
                        if (nq >= c) {
                            t = 1.0f;
                            f += c + (2.0f * e);
                        } else {
                            t = nq / c;
                            f += e * t;
                        }
                    }
                }
            }
        }
    } else {
        // (nearly) parallel lines
        t = 0.0f;
        if (b > 0.0f) {
            if (d >= 0.0f) {
                s = 0.0f;
            } else {
                nq = -d;
                if (nq <= a) {
                    s = nq / a;
                    f += d * s;
                } else {
                    s = 1.0f;
                    f32 qa = a + d;
                    nq = -qa;
                    if (nq >= b) {
                        t = 1.0f;
                        f = f + (a + c) + (2.0f * (e + (b + d)));
                    } else {
                        t = nq / b;
                        f = (f + (a + (2.0f * d))) + (t * ((c * t) + (2.0f * (b + e))));
                    }
                }
            }
        } else {
            nq = -d;
            if (nq >= a) {
                s = 1.0f;
                f += a + (2.0f * d);
            } else if (d <= 0.0f) {
                s = nq / a;
                f += d * s;
            } else {
                s = 0.0f;
                if (d >= -b) {
                    t = 1.0f;
                    f += c + (2.0f * e);
                } else {
                    t = nq / b;
                    f += t * ((2.0f * e) + (c * t));
                }
            }
        }
    }

    if (arg4 != NULL) *arg4 = s;
    if (arg5 != NULL) *arg5 = t;
    return fabsf(f);
}

}  // extern "C"
