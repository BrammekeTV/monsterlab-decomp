// Grid layout update, slab ray test, locator matrix concat, movie packet reader, hit-plane sweeps.
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "types.h"

extern "C" {
u8*   fn_801483BC(s32 idx);                                          // 0x801483BC
u8*    fn_8019552C(u8* list, s32 index);                              // 0x8019552C (leaf; really returns the element pointer)
s32   fn_801A9EC8(u8* obj, f32 x, f32 y, f32 z);                     // 0x801A9EC8
void  fn_801ACD08(u8* a, u8* b, u8* out, f32 x, f32 y);              // 0x801ACD08
void  MtxFromLocatorRotation(f32 out[12], const void* rot);          // 0x80143CA8
void  fn_801443B0(f32* m);                                           // 0x801443B0
f64   fn_8033F20C(f64 x, f64 y);                                     // 0x8033F20C (pow)
u8*   fn_801F59F0(u8* p);                                            // 0x801F59F0
u8*   fn_801F5A14(u8* p, u8* q);                                     // 0x801F5A14
void  fn_801F5E48(void* a, s32 b);                                   // 0x801F5E48
void  fn_801F5E4C(void* a, s32 b);                                   // 0x801F5E4C
s32   fn_801F8764(u8* list, s32 mode, f32* normal, s32 index, f32* outT, u32* outU, f32* start, f32 f1);  // 0x801F8764
s32   fn_801FA0A8(u8* list, s32 mode, f32* plane, s32 index, f32* outT, f32* start, f32 f1);               // 0x801FA0A8

extern f32 lbl_805FFA08;  // 0x805FFA08 (1.0)
extern f32 lbl_805FFA0C;  // 0x805FFA0C (0.0)
extern f32 lbl_805FFAB8;  // 0x805FFAB8 (0.0)
extern f32 lbl_805FFABC;  // 0x805FFABC (1.0)
extern f32 lbl_805FFAC0;  // 0x805FFAC0 (1/3)
extern f32 lbl_805FFAC8;  // 0x805FFAC8 (+epsilon)
extern f32 lbl_805FFAE0;  // 0x805FFAE0 (-epsilon)
extern f32 lbl_805FFAE4;  // 0x805FFAE4 (tiny direction epsilon)
extern f32 lbl_805FFAE8;  // 0x805FFAE8 (rotation scale)
extern f32 lbl_806001DC;  // 0x806001DC (0.0)
extern f32 lbl_806001D4;  // 0x806001D4 (-1.0)
extern f32 lbl_80600208;  // 0x80600208 (5.0)
extern f32 lbl_80600270;  // 0x80600270 (-1.0)
extern f32 lbl_80600258;  // 0x80600258 (0.0)
extern f32 lbl_8060025C;  // 0x8060025C (1.0)
extern f32 lbl_80600274;  // 0x80600274 (0.1)
extern f32 lbl_805F9880;  // 0x805F9880
extern f32 lbl_805FBA00;  // 0x805FBA00

extern u8  lbl_804DD568[];  // 0x804DD568 (packet ring: +8 head, +0xC tail, +0x10 capacity, +0x14 base, 0x400-byte slots)
extern u8  lbl_804EC590[];  // 0x804EC590 (movie packet ring, same layout, +0 flag)
extern u8  lbl_804FB5E0[];  // 0x804FB5E0 (lock object of fn_801F5E4C / fn_801F5E48)
extern s32 lbl_804FB698[];  // 0x804FB698
extern s32 lbl_805F97FC;    // 0x805F97FC
extern u8* lbl_805FB95C;    // 0x805FB95C (current movie slot)
extern void* lbl_805FB964;  // 0x805FB964 (movie player)
extern void* lbl_805FB968;  // 0x805FB968 (current target)
extern s32 lbl_805FB970;    // 0x805FB970
extern s32 lbl_805FB974;    // 0x805FB974
extern s32 lbl_805FB980;    // 0x805FB980
extern s32 lbl_805FB984;    // 0x805FB984
extern s32 lbl_805FB988;    // 0x805FB988
extern s32 lbl_805FB98C;    // 0x805FB98C
extern s32 lbl_805FB990;    // 0x805FB990
extern s32 lbl_805FB998;    // 0x805FB998
}

extern "C" {

typedef void (*GridWidgetFn)(u8* obj, s32 arg);
typedef s32 (*PlayerReadFn)(void* player, u8* buf, s32* size);

// Slot of the ring buffer at index i if it is inside the live range [head, tail] (wrapping),
// otherwise NULL. Inlined by the original wherever a ring slot is looked up.
static u8* RingSlot(u8* q, s32 i) {
    s32 head = *(s32*)(q + 0x8);
    s32 tail = *(s32*)(q + 0xC);
    s32 cap = *(s32*)(q + 0x10);
    if (i < 0 || i >= cap) return NULL;
    s32 j = i;
    s32 hi = tail;
    if (tail < head) {
        if (i < head) j = i + cap;
        hi = tail + cap;
    }
    if (head <= j && j <= hi) return *(u8**)(q + 0x14) + (i << 10);
    return NULL;
}

// Reads the five floats of a path point (+0xC.. or the alternate copy at +0x68..).
static void LoadPathPoint(const u8* e, s32 alt, f32* out) {
    const f32* s = (const f32*)(e + (alt != 0 ? 0x68 : 0xC));
    out[0] = s[0];
    out[1] = s[1];
    out[2] = s[2];
    out[3] = s[3];
    out[4] = s[4];
}

// 0x801A6D7C
// Lays out the grid of child objects of a container: places each cell's object at its
// (column, row) position, and when the container is scaled rescales the object's children.
s32 fn_801A6D7C(u8* obj, s32 force) {
    f32 z = *(f32*)(obj + 0x70);
    f32 cellW = *(f32*)(obj + 0x180);
    f32 cellH = *(f32*)(obj + 0x184);
    // Divide the cell size by the scale of every container up the chain.
    for (u8* p = obj; p != NULL; p = *(u8**)(p + 0x128)) {
        cellW /= *(f32*)(p + 0x74);
        cellH /= *(f32*)(p + 0x78);
    }
    s32 rowLo = 0;
    s32 colLo = 0;
    s32 rowHi = *(s32*)(obj + 0x144);
    s32 colHi = *(s32*)(obj + 0x148);
    s32 rowEdge = 0;
    s32 colEdge = 0;
    s32 scaled = 0;
    if (*(f32*)(obj + 0x1A8) != lbl_805FFA08 || *(f32*)(obj + 0x1AC) != lbl_805FFA08 ||
        *(f32*)(obj + 0x1B0) != lbl_805FFA08) {
        scaled = 1;
    }
    if (*(u32*)(obj + 0x64) & 0x800000) {
        s32 rows = *(s32*)(obj + 0x13C);
        if (rowHi < rows && rows - rowHi >= 1) {
            rowEdge = 1;
            if (*(u8*)(obj + 0x17B) != 0) {
                rowLo = -1;
            } else if (*(u8*)(obj + 0x17A) != 0) {
                rowHi += 1;
            }
        }
        s32 cols = *(s32*)(obj + 0x140);
        if (colHi < cols && cols - colHi >= 1) {
            colHi += 1;
            colEdge = 1;
            colLo = -1;
        }
    } else {
        force = 1;
    }
    for (s32 row = rowLo; row < rowHi; row++) {
        for (s32 col = colLo; col < colHi; col++) {
            s32 edgeMoved = 0;
            s32 rows = *(s32*)(obj + 0x13C);
            s32 gr = row + *(s32*)(obj + 0x15C);
            s32 gc = col + *(s32*)(obj + 0x160);
            if (gr >= rows) {
                gr -= rows;
            } else if (gr < 0) {
                gr += rows;
            }
            s32 cols = *(s32*)(obj + 0x140);
            if (gc >= cols) {
                gc -= cols;
            } else if (gc < 0) {
                gc += cols;
            }
            u8** slot = (u8**)fn_8019552C(*(u8**)(obj + 0x1BC), gc + gr * cols);
            u8* o = (slot != NULL) ? *slot : NULL;
            if (o == NULL) continue;
            // virtual call, slot 0x2C of the vtable at +0x138 (arg 0x18; float regs are leftovers)
            (*(GridWidgetFn*)(*(u8**)(o + 0x138) + 0x2C))(o, 0x18);
            f32 baseX = *(f32*)fn_8019552C(*(u8**)(obj + 0x1B8), gr);
            f32 x = baseX + cellW * (f32)col;
            if (colEdge != 0) {
                if (*(u8*)(obj + 0x178) != 0 && col == colLo) {
                    if (*(s32*)(obj + 0x140) - *(s32*)(obj + 0x148) >= 2) edgeMoved = 1;
                    x = baseX - cellW;
                } else if (*(u8*)(obj + 0x179) != 0 && col == colHi - 1) {
                    s32 n = *(s32*)(obj + 0x148);
                    if (*(s32*)(obj + 0x140) - n >= 2) edgeMoved = 1;
                    x = baseX + (f32)n * cellW;
                }
            }
            f32 y = lbl_805FFA0C + cellH * (f32)row;
            if (rowEdge != 0) {
                if (*(u8*)(obj + 0x17A) != 0 && row == rowLo) {
                    if (*(s32*)(obj + 0x13C) - *(s32*)(obj + 0x144) >= 2) edgeMoved = 1;
                    y = -cellH;
                } else if (*(u8*)(obj + 0x17B) != 0 && row == rowHi - 1) {
                    s32 n = *(s32*)(obj + 0x144);
                    if (*(s32*)(obj + 0x13C) - n >= 2) edgeMoved = 1;
                    y = (f32)n * cellH;
                }
            }
            fn_801A9EC8(o, x, y, z);
            if (force != 0 || edgeMoved != 0) {
                *(f32*)(o + 0x68) = x;
                *(f32*)(o + 0x6C) = y;
                *(f32*)(o + 0x70) = z;
            }
            if (scaled != 0) {
                s32 dr = *(s32*)(obj + 0x150) - col;
                s32 dc = *(s32*)(obj + 0x14C) - row;
                s32 dist = (dc < 0 ? -dc : dc) + (dr < 0 ? -dr : dr);
                f32 scaleX = (f32)fn_8033F20C((f64) * (f32*)(obj + 0x1A8), (f64)(f32)dist);
                f32 scaleY = (f32)fn_8033F20C((f64) * (f32*)(obj + 0x1AC), (f64)(f32)dist);
                for (u8* c = *(u8**)(o + 0x120); c != NULL; c = *(u8**)(c + 0x124)) {
                    f32 sx = scaleX * *(f32*)(c + 0x20);
                    f32 sy = scaleY * *(f32*)(c + 0x24);
                    f32 sz = *(f32*)(c + 0x28);
                    *(f32*)(c + 0x74) = sx;
                    *(f32*)(c + 0x78) = sy;
                    *(f32*)(c + 0x7C) = sz;
                    (*(GridWidgetFn*)(*(u8**)(c + 0x138) + 0x2C))(c, 0x18);
                }
            }
        }
    }
    return 0;
}

// 0x801AE9AC
// Slab test of the segment p0 -> p1 against the box (center +/- half) grown by margin. On a hit
// fills out: +0 hit point, +0xC reversed direction, +0x18 remaining distance. Returns 1 on a hit.
s32 fn_801AE9AC(u8* p0, u8* p1, u8* center, u8* half, u8* out, f32 margin) {
    f32 o[3], d[3], mn[3], mx[3], dir[3];
    for (s32 i = 0; i < 3; i++) {
        o[i] = *(f32*)(p0 + i * 4);
        d[i] = *(f32*)(p1 + i * 4) - o[i];
        f32 c = *(f32*)(center + i * 4);
        f32 h = *(f32*)(half + i * 4);
        mn[i] = c - h;
        mx[i] = c + h;
    }
    f32 len2 = d[2] * d[2] + (d[0] * d[0] + d[1] * d[1]);
    f32 len = lbl_805FFAB8;
    if (len2 == lbl_805FFAB8) {
        dir[0] = lbl_805FFAB8;
        dir[1] = lbl_805FFAB8;
        dir[2] = lbl_805FFAB8;
    } else {
        if (len2 > lbl_805FFAB8) len = sqrtf(len2);
        f32 inv = lbl_805FFABC / len;
        dir[0] = d[0] * inv;
        dir[1] = d[1] * inv;
        dir[2] = d[2] * inv;
    }
    for (s32 i = 0; i < 3; i++) {
        mn[i] -= margin;
        mx[i] += margin;
    }
    s32 neg[3];
    for (s32 i = 0; i < 3; i++) neg[i] = dir[i] < lbl_805FFAB8 ? 1 : 0;
    for (s32 i = 0; i < 3; i++) {
        if (lbl_805FFAB8 == dir[i]) dir[i] = lbl_805FFAE4;
    }
    f32 inv[3] = { lbl_805FFABC / dir[0], lbl_805FFABC / dir[1], lbl_805FFABC / dir[2] };
    f32 b[6] = { mn[0], mn[1], mn[2], mx[0], mx[1], mx[2] };
    f32 tminX = (b[3 * neg[0] + 0] - o[0]) * inv[0];
    f32 tmaxX = (b[3 * (1 - neg[0]) + 0] - o[0]) * inv[0];
    f32 tmaxY = (b[3 * (1 - neg[1]) + 1] - o[1]) * inv[1];
    f32 tminY = (b[3 * neg[1] + 1] - o[1]) * inv[1];
    if (tminX > tmaxY || tminY > tmaxX) return 0;
    f32 tmin = tminX;
    if (tminY > tmin) tmin = tminY;
    f32 tmax = tmaxX;
    if (tmaxY < tmax) tmax = tmaxY;
    f32 tmaxZ = (b[3 * (1 - neg[2]) + 2] - o[2]) * inv[2];
    f32 tminZ = (b[3 * neg[2] + 2] - o[2]) * inv[2];
    if (tmin > tmaxZ || tminZ > tmax) return 0;
    if (tminZ > tmin) tmin = tminZ;
    f32 t = tmin;
    *(f32*)(out + 0x0) = o[0] + dir[0] * t;
    *(f32*)(out + 0x4) = o[1] + dir[1] * t;
    *(f32*)(out + 0x8) = o[2] + dir[2] * t;
    *(f32*)(out + 0xC) = -dir[0];
    *(f32*)(out + 0x10) = -dir[1];
    *(f32*)(out + 0x14) = -dir[2];
    *(f32*)(out + 0x18) = (lbl_805FFABC - t) * len;
    if (t > -margin && t < len + margin) return 1;
    return 0;
}

// 0x801B01A8
// Recomputes the object's matrix (+0x3C, 4x3): the locator rotation of its definition (obj+8)
// concatenated with the matrix from fn_801ACD08, new position at +0x14, scale at +0x6C.
// If snap is set the previous position (+0x20) is also set to the new one.
void fn_801B01A8(u8* obj, s32 snap) {
    *(f32*)(obj + 0x20) = *(f32*)(obj + 0x14);
    *(f32*)(obj + 0x24) = *(f32*)(obj + 0x18);
    *(f32*)(obj + 0x28) = *(f32*)(obj + 0x1C);
    f32 B[12];
    // f1/f2 are leftover caller values here: pass 0
    fn_801ACD08(obj, *(u8**)(obj + 0x8) + 0xC, (u8*)B, 0.0f, 0.0f);
    u8* def = *(u8**)(obj + 0x8);
    *(f32*)(obj + 0x14) = B[9];
    *(f32*)(obj + 0x18) = B[10];
    *(f32*)(obj + 0x1C) = B[11];
    f32 rot[3];
    rot[0] = *(f32*)(def + 0x34) * lbl_805FFAE8;
    rot[1] = *(f32*)(def + 0x38) * lbl_805FFAE8;
    rot[2] = *(f32*)(def + 0x3C) * lbl_805FFAE8;
    MtxFromLocatorRotation((f32*)(obj + 0x3C), rot);
    f32* M = (f32*)(obj + 0x3C);
    f32 A[12];
    for (s32 i = 0; i < 12; i++) A[i] = M[i];
    // M = A * B (3x3 part); translation row = B.t + A.t * B
    for (s32 i = 0; i < 3; i++) {
        for (s32 j = 0; j < 3; j++) {
            M[i * 3 + j] = (B[6 + j] * A[i * 3 + 2]) + ((B[j] * A[i * 3 + 0]) + (B[3 + j] * A[i * 3 + 1]));
        }
    }
    for (s32 j = 0; j < 3; j++) {
        M[9 + j] = B[9 + j] + ((B[6 + j] * A[11]) + ((B[j] * A[9]) + (B[3 + j] * A[10])));
    }
    u8* sc = *(u8**)(obj + 0x10);
    f32 avg = lbl_805FFAC0 * (*(f32*)(sc + 0x10) + (*(f32*)(sc + 0x8) + *(f32*)(sc + 0xC)));
    s32 unit = 0;
    if (avg > lbl_805FFAE0 && avg < lbl_805FFAC8) unit = 1;
    if (unit == 0) {
        unit = 0;
        f32 dev = lbl_805FFAC0 * (*(f32*)(sc + 0x10) + (*(f32*)(sc + 0x8) + *(f32*)(sc + 0xC))) - lbl_805FFABC;
        if (dev > lbl_805FFAE0 && dev < lbl_805FFAC8) unit = 1;
        if (unit == 0) fn_801443B0(M);
    }
    u8* sc2 = *(u8**)(obj + 0x10);
    u8* def2 = *(u8**)(obj + 0x8);
    *(f32*)(obj + 0x6C) = *(f32*)(def2 + 0x28) * *(f32*)(sc2 + 0x8);
    *(f32*)(obj + 0x70) = *(f32*)(def2 + 0x2C) * *(f32*)(sc2 + 0xC);
    *(f32*)(obj + 0x74) = *(f32*)(def2 + 0x30) * *(f32*)(sc2 + 0x10);
    if (snap != 0) {
        *(f32*)(obj + 0x20) = *(f32*)(obj + 0x14);
        *(f32*)(obj + 0x24) = *(f32*)(obj + 0x18);
        *(f32*)(obj + 0x28) = *(f32*)(obj + 0x1C);
    }
}

// 0x801F43B0
// Movie packet reader loop: reads packets from the player (virtual slot 0x1C), drops ring
// entries the packet list has passed, tracks the sequence numbers (lbl_804FB698) and copies the
// packet for the current sequence into the movie packet ring. Runs while lbl_805FB998 is set.
s32 fn_801F43B0(void) {
    u8 pkt[0x400];
    s32 got;
    lbl_805FB998 = 1;
    while (lbl_805FB998 != 0) {
        lbl_805FB998 = 0;
        if (lbl_805FB970 == 0 || lbl_805FB974 == 0 || (u32)lbl_805FB990 == 0x20 || lbl_805FB968 == NULL) continue;
        fn_801F5E4C(lbl_804FB5E0, 4);
        s32 tries = 2;
        while (tries > 0) {
            got = 0x400;
            s32 ended = 0;
            s32 rc = (*(PlayerReadFn*)(*(u8**)lbl_805FB964 + 0x1C))(lbl_805FB964, pkt, &got);
            if (rc == -1) {
                tries -= 1;
                continue;
            }
            if (rc == -2) ended = 1;
            if (got == 0 && ended == 0) continue;
            tries = 2;
            if (ended != 0) {
                lbl_805FB984 += 1;
                lbl_805FB98C += got;
                continue;
            }
            u32 seq = *(u32*)(pkt + 4);
            if (seq == 0) {
                u8* e = fn_801F59F0(pkt);
                while (e != NULL) {
                    u8* cur = lbl_805FB95C;
                    for (;;) {
                        s32 head = *(s32*)(lbl_804DD568 + 0x8);
                        u8* s = RingSlot(lbl_804DD568, head);
                        if (s == NULL) break;
                        if (*(u32*)(s + 4) > *(u32*)(e + 4)) break;
                        if (s == cur) break;
                        // drop the head entry
                        if (*(s32*)(lbl_804DD568 + 0xC) == *(s32*)(lbl_804DD568 + 0x8)) {
                            *(s32*)(lbl_804DD568 + 0xC) = -1;
                            *(s32*)(lbl_804DD568 + 0x8) = -1;
                        } else {
                            *(s32*)(lbl_804DD568 + 0x8) = (*(s32*)(lbl_804DD568 + 0x8) + 1) % *(s32*)(lbl_804DD568 + 0x10);
                        }
                    }
                    e = fn_801F5A14(pkt, e);
                }
            } else if (seq < (u32)lbl_805F97FC) {
                u32 idx = (u32)lbl_805FB990;
                s32 added;
                if (idx < 8u) {
                    lbl_804FB698[idx] = (s32)seq;
                    added = 1;
                    lbl_805FB990 += 1;
                } else {
                    if (seq > (u32)lbl_804FB698[idx]) lbl_804FB698[idx] = (s32)seq;
                    added = 0;
                }
                if (added == 0) break;
                continue;
            }
            if (seq == (u32)lbl_805F97FC) {
                if (lbl_805FB968 != NULL) {
                    u8* q = lbl_804EC590;
                    s32 next = (*(s32*)(q + 0xC) + 1) % *(s32*)(q + 0x10);
                    s32 head = *(s32*)(q + 0x8);
                    u8* slot;
                    if (head == next) {
                        slot = NULL;
                    } else {
                        *(s32*)(q + 0xC) = next;
                        if (head < 0) {
                            *(s32*)(q + 0x8) = next;
                            *(s32*)(q + 0x0) = 1;
                        }
                        slot = RingSlot(q, *(s32*)(q + 0xC));
                    }
                    if (slot != NULL) {
                        u32 idx = (u32)lbl_805FB990;
                        if (idx < 8u) {
                            lbl_804FB698[idx] = (s32)seq;
                            lbl_805FB990 += 1;
                        } else if (seq > (u32)lbl_804FB698[idx]) {
                            lbl_804FB698[idx] = (s32)seq;
                        }
                        memcpy(slot, pkt, 0x400);
                        lbl_805F97FC = (s32)(seq + 1);
                    }
                }
                lbl_805FB980 += 1;
                lbl_805FB988 += got;
            }
        }
        fn_801F5E48(lbl_804FB5E0, 4);
    }
    return 0;
}

// 0x801F8A00
// Per-frame update of a hit tracker: counts the timers down and sweeps the path segments
// (newest first) against the six planes of the object, recording the best crossing in +0x474.
void fn_801F8A00(u8* obj, f32 dt) {
    f32 t0 = *(f32*)(obj + 0x3F0);
    if (t0 >= lbl_806001DC) {
        *(f32*)(obj + 0x3F0) = t0 + dt;
    } else {
        *(f32*)(obj + 0x474) = lbl_806001D4;
        for (s32 i = 0; i < *(s32*)(obj + 0x1E0); i++) *(f32*)(obj + 0x40C + i * 4) = lbl_806001D4;
    }
    f32 span = *(f32*)(obj + 0x3F4);
    if (span > lbl_806001DC) {
        f32 left = *(f32*)(obj + 0x3F8) - dt;
        *(f32*)(obj + 0x3F8) = left;
        f32 v = *(f32*)(obj + 0x3FC) * (left / span);
        *(f32*)(obj + 0x400) = v;
        if (v <= lbl_806001DC) *(f32*)(obj + 0x400) = lbl_806001DC;
    } else {
        *(f32*)(obj + 0x400) = lbl_806001DC;
    }
    for (s32 i = 0; i < *(s32*)(obj + 0x1E0); i++) {
        u8* p = obj + i * 4;
        f32 cd = *(f32*)(p + 0x384);
        if (cd > lbl_806001DC) {
            if (cd - dt <= lbl_806001DC) *(f32*)(p + 0x31C) = lbl_806001DC;
        }
        *(f32*)(p + 0x384) = *(f32*)(p + 0x384) - dt;
    }
    u8* path = fn_801483BC(*(s32*)(obj + 0x4));
    f32 start[3];
    start[0] = *(f32*)(obj + 0x9C) * lbl_806001D4;
    start[1] = *(f32*)(obj + 0xA0) * lbl_806001D4;
    start[2] = *(f32*)(obj + 0xA4) * lbl_806001D4;
    for (s32 seg = *(s32*)(path + 0x14) - 1; seg >= 0; seg--) {
        s32 base = *(s32*)(path + 0x10);
        s32 count = *(s32*)(path + 0xC);
        s32 alt = *(s32*)(obj + 0x98);
        u8* ents = *(u8**)(path + 0x4);
        f32 P1[5], P2[5];
        LoadPathPoint(ents + ((base + seg) % count) * 0x84, alt, P1);
        LoadPathPoint(ents + ((base + seg + 1) % count) * 0x84, alt, P2);
        f32 ox = *(f32*)(obj + 0x9C);
        f32 oy = *(f32*)(obj + 0xA0);
        f32 oz = *(f32*)(obj + 0xA4);
        P1[0] -= ox;
        P1[1] -= oy;
        P1[2] -= oz;
        P2[0] -= ox;
        P2[1] -= oy;
        P2[2] -= oz;
        f32 key = P1[4];
        for (s32 i = 0; i < *(s32*)(obj + 0x1E0); i++) {
            u8* sl = obj + i * 4;
            u8* pl = obj + i * 0xC;
            if (key >= *(f32*)(sl + 0x1E4)) {
                f32 n0 = *(f32*)(pl + 0xA8);
                f32 n1 = *(f32*)(pl + 0xAC);
                f32 n2 = *(f32*)(pl + 0xB0);
                f32 d1 = (n2 * P2[2]) + ((n0 * P2[0]) + (n1 * P2[1]));
                f32 d0 = (n2 * P1[2]) + ((n0 * P1[0]) + (n1 * P1[1]));
                s32 side;
                if (d1 > lbl_806001DC && d0 <= lbl_806001DC) {
                    side = -1;
                } else if (d1 < lbl_806001DC && d0 >= lbl_806001DC) {
                    side = 1;
                } else {
                    side = 0;
                }
                if (side > 0) {
                    f32 hitT = lbl_806001DC;
                    u32 hitU = (u32) * (f32*)(sl + 0x31C);
                    f32 from[3] = { start[0], start[1], start[2] };
                    f32 nrm[3] = { -*(f32*)(pl + 0xA8), -*(f32*)(pl + 0xAC), -*(f32*)(pl + 0xB0) };
                    fn_801F8764(path, *(s32*)(obj + 0x98), nrm, seg + 1, &hitT, &hitU, from, *(f32*)(sl + 0x24C));
                    f32 frac = hitT / *(f32*)(sl + 0x2B4);
                    if (frac > *(f32*)(obj + 0x404) + *(f32*)(obj + 0x400)) {
                        *(f32*)(sl + 0x40C) = hitT;
                        *(f32*)(sl + 0x31C) = (f32)hitU;
                        *(f32*)(sl + 0x384) = lbl_80600208;
                        if (frac > *(f32*)(obj + 0x474)) {
                            f32 cur = *(f32*)(obj + 0x3F0);
                            *(f32*)(obj + 0x474) = frac;
                            if (cur < lbl_806001DC) *(f32*)(obj + 0x3F0) = lbl_806001DC;
                        }
                    }
                }
            }
        }
    }
    *(f32*)(obj + 0x408) = *(f32*)(obj + 0x408) + dt;
    if (*(f32*)(obj + 0x3F0) >= *(f32*)(obj + 0x3EC)) {
        f32 best = *(f32*)(obj + 0x474);
        if (best > lbl_806001DC) {
            *(u8*)(obj + 0x90) = 1;
            *(f32*)(obj + 0x3F8) = *(f32*)(obj + 0x3F4);
            *(f32*)(obj + 0x3FC) = best - *(f32*)(obj + 0x404);
            *(f32*)(obj + 0x408) = lbl_806001DC;
        }
        *(f32*)(obj + 0x3F0) = lbl_806001D4;
    }
}

// 0x801FA304
// Per-frame update of the four-direction hit detector: advances the timers (+0xD8..+0xE4, +0xFC),
// sweeps the path segments against the four planes (+0xB4, negated copies) and records which
// direction was crossed last (+0xD4), its repeat count (+0xF4) and the result (+0xCC).
void fn_801FA304(u8* obj, f32 dt) {
    f32 planes[12];
    for (s32 i = 0; i < 6; i++) planes[i] = *(f32*)(obj + 0xB4 + i * 4);
    for (s32 i = 0; i < 6; i++) planes[6 + i] = -*(f32*)(obj + 0xB4 + i * 4);
    f32 age = *(f32*)(obj + 0xFC) + dt;
    *(f32*)(obj + 0xD8) = *(f32*)(obj + 0xD8) + dt;
    *(f32*)(obj + 0xDC) = *(f32*)(obj + 0xDC) + dt;
    *(f32*)(obj + 0xE0) = *(f32*)(obj + 0xE0) + dt;
    *(f32*)(obj + 0xE4) = *(f32*)(obj + 0xE4) + dt;
    *(f32*)(obj + 0xFC) = age;
    if (age > *(f32*)(obj + 0xF8) && *(s32*)(obj + 0xCC) != 0) *(s32*)(obj + 0xCC) = 0;
    lbl_805FBA00 = lbl_805FBA00 + dt;
    u8* path = fn_801483BC(*(s32*)(obj + 0x4));
    f32 start[3];
    start[0] = *(f32*)(obj + 0x9C) * lbl_80600270;
    start[1] = *(f32*)(obj + 0xA0) * lbl_80600270;
    start[2] = *(f32*)(obj + 0xA4) * lbl_80600270;
    for (s32 seg = *(s32*)(path + 0x14) - 1; seg >= 0; seg--) {
        s32 base = *(s32*)(path + 0x10);
        s32 count = *(s32*)(path + 0xC);
        s32 alt = *(s32*)(obj + 0x98);
        u8* ents = *(u8**)(path + 0x4);
        f32 P1[5], P2[5];
        LoadPathPoint(ents + ((base + seg) % count) * 0x84, alt, P1);
        LoadPathPoint(ents + ((base + seg + 1) % count) * 0x84, alt, P2);
        f32 ox = *(f32*)(obj + 0x9C);
        f32 oy = *(f32*)(obj + 0xA0);
        f32 oz = *(f32*)(obj + 0xA4);
        P1[0] -= ox;
        P1[1] -= oy;
        P1[2] -= oz;
        P2[0] -= ox;
        P2[1] -= oy;
        P2[2] -= oz;
        for (s32 i = 0; i < 4; i++) {
            if (lbl_80600258 == *(f32*)(obj + 0xD8 + i * 4)) continue;
            f32* pl = &planes[i * 3];
            f32 n0 = pl[0];
            f32 n1 = pl[1];
            f32 n2 = pl[2];
            f32 d1 = (n2 * P2[2]) + ((n0 * P2[0]) + (n1 * P2[1]));
            f32 d0 = (n2 * P1[2]) + ((n0 * P1[0]) + (n1 * P1[1]));
            s32 side;
            if (d1 > lbl_80600258 && d0 <= lbl_80600258) {
                side = -1;
            } else if (d1 < lbl_80600258 && d0 >= lbl_80600258) {
                side = 1;
            } else {
                side = 0;
            }
            if (side > 0) {
                f32 hitT = lbl_80600258;
                f32 from[3] = { start[0], start[1], start[2] };
                lbl_805FBA00 = lbl_80600258;
                fn_801FA0A8(path, *(s32*)(obj + 0x98), pl, seg + 1, &hitT, from, lbl_80600274);
                if (hitT > lbl_805F9880) {
                    s32 count2 = *(s32*)(obj + 0xF4) + 1;
                    s32 last = *(s32*)(obj + 0xD4);
                    *(f32*)(obj + 0xD0) = lbl_8060025C / *(f32*)(obj + 0xD8 + i * 4);
                    *(f32*)(obj + 0xFC) = lbl_80600258;
                    *(s32*)(obj + 0xF4) = count2;
                    s32 dir;
                    if (last == ((i + 3) & 3)) {
                        dir = 1;
                    } else {
                        dir = 0;
                        if (last == ((i + 1) & 3)) dir = 2;
                    }
                    if (*(s32*)(obj + 0xEC) != dir) *(s32*)(obj + 0xF4) = 0;
                    *(s32*)(obj + 0xEC) = dir;
                    if (*(s32*)(obj + 0xF4) >= *(s32*)(obj + 0xF0)) *(s32*)(obj + 0xCC) = dir;
                    *(f32*)(obj + 0xD8 + i * 4) = lbl_80600258;
                    *(s32*)(obj + 0xD4) = i;
                }
            }
        }
    }
    if (*(s32*)(obj + 0xCC) == 0) *(f32*)(obj + 0xD0) = lbl_80600258;
}

}  // extern "C"
