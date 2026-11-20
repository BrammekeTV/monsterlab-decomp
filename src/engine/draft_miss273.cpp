// Engine 0x801C: UI quad batcher helpers: back-to-front sort of 16-byte sprite records (0x801CAD80)
// and the two particle/billboard-quad emitters (0x801CC928, 0x801CD044) that walk an entity's list
// of nodes and append 4-vertex quads (0x60 bytes) to the shared vertex buffer.
#include <string.h>
#include "types.h"
#include "engine/math.h"

extern "C" {
u8*  fn_801CC8A4(u8* e);                    // 0x801CC8A4 (leaf: node list of an entity, at +0x60)
u8*  fn_801CC8AC(u8* list);                 // 0x801CC8AC (leaf: first node of the list, or NULL)
u8*  fn_801CC90C(u8* list, u8* node);       // 0x801CC90C (leaf: next node, or NULL)
s32  fn_801CC8C8(u8* node, s32 bit);        // 0x801CC8C8 (node flag bit test, flags at +0x98)
s32  fn_801CC8F4(u8* obj, s32 bit);         // 0x801CC8F4 (flag bit test, flags at +0x1A8)
u8*  fn_801CC8E4(u8* node);                 // 0x801CC8E4 (leaf: node + 0x10, color floats)
u8*  fn_801CC8EC(u8* node);                 // 0x801CC8EC (leaf: node + 0x20, size/scale floats)
u8*  fn_801CBD28(u8* e);                    // 0x801CBD28 (leaf: e + 0x10)
f32  fn_801CBD30(u8* obj);                  // 0x801CBD30 (leaf: float at +0x1B0)
s32  fn_801CBD38(u8* obj);                  // 0x801CBD38 (leaf: word at +0x1D4)
s32  fn_801CCDBC(u8* obj);                  // 0x801CCDBC (leaf: word at +0x1A0)
u8*  fn_80067844(u8* e);                    // 0x80067844 (leaf: pointer at +0x5C)
void fn_801CCDC4(void);                     // 0x801CCDC4 (start a new vertex batch)
void fn_801CCFE0(void);                     // 0x801CCFE0 (commit the vertices written so far)
void fn_801CA940(u8* begin, u8* end);       // 0x801CA940 (draw the quads between begin and end)
void fn_801CA7B0(u32 mode);                 // 0x801CA7B0
u32  fn_801CCE08(const f32* rgba);          // 0x801CCE08 (float color -> packed color)
void fn_801CCF00(f32* angle, f32* a, f32* b);  // 0x801CCF00
void fn_801CCFAC(f32* out, f32* v);         // 0x801CCFAC (leaf: out = v[1])
void fn_801CCFB8(f32* out, f32* v);         // 0x801CCFB8 (leaf: out = v[0])
f32  fn_801CCFC4(u8* vertex, Vec3* pos);    // 0x801CCFC4 (leaf: vertex position at +0xC)
s32  fn_801CD648(f32* a, f32* b);           // 0x801CD648 (|a - b| <= 0.0001)
f32  fn_801CD680(f32* a, f32* b);           // 0x801CD680 (leaf: a += b)
f32  fn_801CD694(f32* a, f32* b);           // 0x801CD694 (leaf: a *= b)
void fn_801CD6A8(f32* angle, f32* cosOut, f32* sinOut);  // 0x801CD6A8
void fn_800446BC(void*, int);               // 0x800446BC
void fn_80052638(Vec3* v);                  // 0x80052638 (empty leaf; the caller passes the vector)
void fn_80054484(Vec3* out, const Vec3* a, const Vec3* b);  // 0x80054484
f32  fn_800544B8(f32* out, f32* a, f32* b); // 0x800544B8 (leaf: out = a - b)
void fn_800544CC(f32* out);                 // 0x800544CC (leaf: out = 1.0f)
f32  fn_80058BD4(f32* out, f32* in);        // 0x80058BD4 (leaf: out = 1.0f / in)
s32  fn_80058BE8(const f32* a, const f32* b);  // 0x80058BE8 (a > b)
s32  fn_80058C00(f32 x);                    // 0x80058C00
void fn_8005C158(Vec3* out, const Vec3* v); // 0x8005C158
void fn_800636A0(void);                     // 0x800636A0 (no-op scalar ctor)
f32  fn_80082F20(u8* out, u8* v);           // 0x80082F20 (copies a Vec3)
void fn_80121434(u8* vertex, f32 u, f32 v); // 0x80121434 (vertex u/v)
f32* fn_80148D70(f32* dst, const f32* src); // 0x80148D70 (copies 4 floats, returns dst)
void fn_8016F6F0(f32* out, f32* v);         // 0x8016F6F0 (leaf: out = v[3])
s32  fn_8016F740(f32* a, f32* b);           // 0x8016F740 (leaf: a >= b)
void fn_801BB338(u8* vertex, u32 color);    // 0x801BB338
void ScalarMul(f32* out, const f32* a, const f32* b);      // 0x80058214
void ScalarSet(f32* out, f32 v);            // 0x80053250
void ScalarZero(f32* out);                  // 0x80055E2C
void Vec3AddTo(Vec3* v, const Vec3* d);     // 0x800544FC
void Vec3Assign(Vec3* out, const Vec3* v);  // 0x80053354
void Vec3Copy(Vec3* out, const Vec3* v);    // 0x80052640
void Vec3Dot(f32* out, const Vec3* a, const Vec3* b);      // 0x800549C0
void Vec3MulScalar(Vec3* out, const Vec3* v, const f32* s);  // 0x80053224
void Vec3SetZero(Vec3* out);                // 0x800531A8
void Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);     // 0x80053370

extern u8   lbl_80478570[];  // 0x80478570 (camera data: axes at +0xC / +0x18, position at +0x3C)
extern u8   lbl_8047C640[];  // 0x8047C640 (camera)
extern s32  lbl_805FB720;    // 0x805FB720
extern s32  lbl_805FB724;    // 0x805FB724
extern u8*  lbl_805FB728;    // 0x805FB728 (vertex batch start)
extern u8*  lbl_805FB72C;    // 0x805FB72C (vertex batch write cursor)
extern s8   lbl_805FB730;    // 0x805FB730 (lbl_805FB734 initialized flag)
extern f32  lbl_805FB734;    // 0x805FB734 (fill limit, set to 4.0f on first use)
}

// Sprite records are 16 bytes: key (s32) at +0xC, depth (f32) at +4. Order: larger key first,
// then larger depth first, then lower address (keeps the sort stable).
static inline BOOL RecBefore(const u8* a, const u8* b) {
    s32 ka = *(const s32*)(a + 0xC);
    s32 kb = *(const s32*)(b + 0xC);
    if (ka != kb) return ka > kb;
    f32 fa = *(const f32*)(a + 4);
    f32 fb = *(const f32*)(b + 4);
    if (fa == fb) return a < b;
    return fa > fb;
}

static inline void RecSwap(u8* a, u8* b) {
    u8 t[16];
    memcpy(t, a, 16);
    memcpy(a, b, 16);
    memcpy(b, t, 16);
}

extern "C" {

// 0x801CAD80
// Sorts `count` 16-byte sprite records (quicksort with median-of-three while intervals are
// larger than 4 records, then a final insertion sort pass).
void fn_801CAD80(void* recs, s32 count) {
    u8* base = (u8*)recs;
    u8* last = base + (s32)count * 16 - 16;

    if ((u32)count > 4) {
        u8* stackLo[32];
        u8* stackHi[32];
        s32 sp = 0;
        u8* lo = base;
        u8* hi = last;
        for (;;) {
            s32 n = (s32)(hi - lo) / 16;
            u8* piv = lo + ((n * 8) & ~0xF);  // middle record
            if (RecBefore(piv, lo)) RecSwap(piv, lo);
            if (RecBefore(hi, piv)) {
                RecSwap(piv, hi);
                if (RecBefore(piv, lo)) RecSwap(piv, lo);
            }
            u8* i = lo + 0x10;
            u8* j = hi - 0x10;
            for (;;) {
                while (RecBefore(i, piv)) i += 0x10;
                while (RecBefore(piv, j)) j -= 0x10;
                if (i < j) {
                    RecSwap(i, j);
                    if (piv == i) piv = j;
                    else if (piv == j) piv = i;
                    i += 0x10;
                    j -= 0x10;
                    if (i <= j) continue;
                    break;
                }
                if (i == j) {
                    i += 0x10;
                    j -= 0x10;
                }
                break;
            }
            s32 nLeft = (s32)(j - lo) / 16;
            s32 nRight = (s32)(hi - i) / 16;
            if (nLeft <= 4) {
                if (nRight <= 4) {
                    if (sp == 0) break;
                    sp--;
                    lo = stackLo[sp];
                    hi = stackHi[sp];
                } else {
                    lo = i;
                }
            } else if (nRight <= 4) {
                hi = j;
            } else if (nLeft > nRight) {
                stackLo[sp] = lo;
                stackHi[sp] = j;
                sp++;
                lo = i;
            } else {
                stackLo[sp] = i;
                stackHi[sp] = hi;
                sp++;
                hi = j;
            }
        }
    }

    // Move the smallest of the first five records to the front (acts as a sentinel).
    u8* lim = base + 0x40;
    if (lim > last) lim = last;
    u8* minp = base;
    for (u8* p = base + 0x10; p <= lim; p += 0x10) {
        if (RecBefore(p, minp)) minp = p;
    }
    if (minp != base) RecSwap(minp, base);

    // Insertion sort of the rest.
    for (u8* p = base + 0x20; p <= last; p += 0x10) {
        u8* q = p - 0x10;
        while (RecBefore(p, q)) q -= 0x10;
        q += 0x10;
        if (q != p) {
            u8 tmp[16];
            memcpy(tmp, p, 16);
            memmove(q + 0x10, q, (size_t)(p - q));
            memcpy(q, tmp, 16);
        }
    }
}

// 0x801CC928
// Emits a billboard quad for every visible node of the entity's node list (particle-style
// sprites: position/angle in a 4-float record, size/scale in a second one), then draws the batch.
s32 fn_801CC928(u8* e) {
    u8* list = fn_801CC8A4(e);
    u8* node = fn_801CC8AC(list);
    if (node == 0) {
        lbl_805FB728 = 0;
        lbl_805FB72C = 0;
        return 0;
    }
    u8* owner = *(u8**)(e + 0x5C);
    s32 a = fn_801CBD38(owner);
    if (lbl_805FB720 != a) {
        lbl_805FB720 = a;
        fn_800446BC(lbl_8047C640, a);
    }
    s32 b = fn_801CCDBC(owner);
    if (lbl_805FB724 != b) {
        lbl_805FB724 = b;
        fn_801CA7B0(fn_801CCDBC(owner));
    }
    fn_801CCDC4();
    u8* batchStart = lbl_805FB72C;
    u8* batchLimit = lbl_805FB728 + 0x390;

    Vec3 origin;
    fn_80052638(&origin);
    if (fn_801CC8F4(fn_80067844(e), 0) != 0) {
        Vec3 t;
        fn_80082F20((u8*)&t, fn_801CBD28(e));
        Vec3Copy(&origin, &t);
    } else {
        Vec3 z;
        Vec3SetZero(&z);
        Vec3Copy(&origin, &z);
    }
    if (fn_80058C00(fn_801CBD30(fn_80067844(e))) == 0) {
        Vec3 t, d, nrm, m;
        f32 s;
        fn_80082F20((u8*)&t, fn_801CBD28(e));
        Vec3Sub(&d, &t, (Vec3*)(lbl_80478570 + 0x3C));
        fn_8005C158(&nrm, &d);
        Vec3Copy(&d, &nrm);
        ScalarSet(&s, fn_801CBD30(fn_80067844(e)));
        Vec3MulScalar(&m, &d, &s);
        Vec3AddTo(&origin, &m);
    }
    Vec3 axisA;  // camera axis 0
    Vec3 axisB;  // camera axis 1
    Vec3Assign(&axisA, (Vec3*)(lbl_80478570 + 0xC));
    Vec3Assign(&axisB, (Vec3*)(lbl_80478570 + 0x18));

    while (node != 0) {
        if (fn_801CC8C8(node, 3) == 0) {
            f32 rec[4];    // position (xyz) and rotation angle (w)
            f32 size[4];   // scale values: [0] horizontal, [1] vertical
            Vec3 pos, tmp;
            f32 angle, cs, sn, sy, sx;
            // fn_801CC8E0 is an identity leaf (returns the node)
            fn_80148D70(rec, (f32*)node);
            fn_80148D70(size, (f32*)fn_801CC8EC(node));
            u32 color = fn_801CCE08((f32*)fn_801CC8E4(node));
            fn_80082F20((u8*)&tmp, (u8*)rec);
            fn_80054484(&pos, &tmp, &origin);
            fn_800636A0();
            fn_800636A0();
            fn_8016F6F0(&angle, rec);
            fn_801CCF00(&angle, &cs, &sn);
            fn_801CCFAC(&sy, size);
            Vec3 t74, t80, t8c, offA;
            Vec3MulScalar(&t74, &axisB, &cs);
            Vec3MulScalar(&t80, &axisA, &sn);
            Vec3Sub(&t8c, &t80, &t74);
            Vec3MulScalar(&offA, &t8c, &sy);
            fn_801CCFB8(&sx, size);
            Vec3 t50, t5c, t68, offB;
            Vec3MulScalar(&t50, &axisB, &sn);
            Vec3MulScalar(&t5c, &axisA, &cs);
            fn_80054484(&t68, &t5c, &t50);
            Vec3MulScalar(&offB, &t68, &sx);
            Vec3 plus, minus;
            fn_80054484(&plus, &pos, &offB);
            Vec3Sub(&minus, &pos, &offB);
            Vec3 v;
            u8* cur;

            Vec3Sub(&v, &plus, &offA);
            cur = lbl_805FB72C;
            fn_801CCFC4(cur, &v);
            fn_80121434(cur, 0.0f, 0.0f);
            fn_801BB338(cur, color);
            fn_80054484(&v, &plus, &offA);
            cur = lbl_805FB72C + 0x18;
            fn_801CCFC4(cur, &v);
            fn_80121434(cur, 0.0f, 1.0f);
            fn_801BB338(cur, color);
            Vec3Sub(&v, &minus, &offA);
            cur = lbl_805FB72C + 0x30;
            fn_801CCFC4(cur, &v);
            fn_80121434(cur, 1.0f, 0.0f);
            fn_801BB338(cur, color);
            fn_80054484(&v, &minus, &offA);
            cur = lbl_805FB72C + 0x48;
            fn_801CCFC4(cur, &v);
            fn_80121434(cur, 1.0f, 1.0f);
            fn_801BB338(cur, color);

            lbl_805FB72C = lbl_805FB72C + 0x60;
            if (lbl_805FB72C >= batchLimit) {
                fn_801CCFE0();
                fn_801CA940(batchStart, lbl_805FB72C);
                lbl_805FB728 = 0;
                lbl_805FB72C = 0;
                fn_801CCDC4();
                batchStart = lbl_805FB72C;
                batchLimit = lbl_805FB728 + 0x390;
            }
        }
        node = fn_801CC90C(list, node);
    }
    if (lbl_805FB72C != 0 && lbl_805FB72C != batchStart) {
        fn_801CCFE0();
        fn_801CA940(batchStart, lbl_805FB72C);
    }
    lbl_805FB728 = 0;
    lbl_805FB72C = 0;
    return 0;
}

// 0x801CD044
// Like fn_801CC928 but only for nodes in front of the plane `ref` (reference record: normal xyz
// and distance w); when `spread` > 0 it also accumulates a coverage value per node and stops
// emitting once that exceeds spread * the fill limit (4.0f).
s32 fn_801CD044(u8* e, u8* ref, f32 spread) {
    u8* list = fn_801CC8A4(e);
    u8* node = fn_801CC8AC(list);
    if (node == 0) {
        lbl_805FB728 = 0;
        lbl_805FB72C = 0;
        return 0;
    }
    u8* owner = *(u8**)(e + 0x5C);
    s32 a = fn_801CBD38(owner);
    if (lbl_805FB720 != a) {
        lbl_805FB720 = a;
        fn_800446BC(lbl_8047C640, a);
    }
    s32 b = fn_801CCDBC(owner);
    if (lbl_805FB724 != b) {
        lbl_805FB724 = b;
        fn_801CA7B0(fn_801CCDBC(owner));
    }
    fn_801CCDC4();
    u8* batchStart = lbl_805FB72C;
    u8* batchLimit = lbl_805FB728 + 0x390;

    Vec3 origin;
    fn_80052638(&origin);
    if (fn_801CC8F4(fn_80067844(e), 0) != 0) {
        Vec3 t;
        fn_80082F20((u8*)&t, fn_801CBD28(e));
        Vec3Copy(&origin, &t);
    } else {
        Vec3 z;
        Vec3SetZero(&z);
        Vec3Copy(&origin, &z);
    }
    if (fn_80058C00(fn_801CBD30(fn_80067844(e))) == 0) {
        Vec3 t, d, nrm, m;
        f32 s;
        fn_80082F20((u8*)&t, fn_801CBD28(e));
        Vec3Sub(&d, &t, (Vec3*)(lbl_80478570 + 0x3C));
        fn_8005C158(&nrm, &d);
        Vec3Copy(&d, &nrm);
        ScalarSet(&s, fn_801CBD30(fn_80067844(e)));
        Vec3MulScalar(&m, &d, &s);
        Vec3AddTo(&origin, &m);
    }
    Vec3 axisA;  // camera axis 0
    Vec3 axisB;  // camera axis 1
    Vec3Assign(&axisA, (Vec3*)(lbl_80478570 + 0xC));
    Vec3Assign(&axisB, (Vec3*)(lbl_80478570 + 0x18));
    f32 coverage;
    ScalarZero(&coverage);

    while (node != 0) {
        if (fn_801CC8C8(node, 3) == 0) {
            f32 rec[4];    // position (xyz) and rotation angle (w)
            f32 size[4];   // scale values: [0] horizontal, [1] vertical
            Vec3 pos, tmp, refDir;
            f32 dist, refW, depth, zero;
            // fn_801CC8E0 is an identity leaf (returns the node)
            fn_80148D70(rec, (f32*)node);
            fn_80082F20((u8*)&tmp, (u8*)rec);
            fn_80054484(&pos, &tmp, &origin);
            fn_80082F20((u8*)&refDir, ref);
            Vec3Dot(&dist, &pos, &refDir);
            fn_8016F6F0(&refW, (f32*)ref);
            fn_800544B8(&depth, &refW, &dist);
            ScalarZero(&zero);
            if (fn_8016F740(&depth, &zero) != 0) {
                fn_80148D70(size, (f32*)fn_801CC8EC(node));
                u32 color = fn_801CCE08((f32*)fn_801CC8E4(node));
                if (spread > 0.0f) {
                    f32 minusOne;
                    ScalarSet(&minusOne, -1.0f);
                    if (fn_801CD648(&depth, &minusOne) == 0) {
                        f32 one, inv, sy, sx, prod, prod2, lim, limScaled;
                        if (lbl_805FB730 == 0) {
                            ScalarSet(&lbl_805FB734, 4.0f);
                            lbl_805FB730 = 1;
                        }
                        fn_800544CC(&one);
                        fn_801CD680(&depth, &one);
                        fn_801CD694(&depth, &depth);
                        fn_80058BD4(&inv, &depth);
                        fn_801CCFAC(&sy, size);
                        fn_801CCFB8(&sx, size);
                        ScalarMul(&prod, &sx, &sy);
                        ScalarMul(&prod2, &prod, &inv);
                        fn_801CD680(&coverage, &prod2);
                        ScalarSet(&lim, spread);
                        ScalarMul(&limScaled, &lim, &lbl_805FB734);
                        if (fn_80058BE8(&coverage, &limScaled) != 0) {
                            // coverage exceeded the limit: stop emitting
                            break;
                        }
                    }
                }
                f32 angle, cs, sn, sy, sx;
                fn_800636A0();
                fn_800636A0();
                fn_8016F6F0(&angle, rec);
                fn_801CD6A8(&angle, &cs, &sn);
                fn_801CCFAC(&sy, size);
                Vec3 t74, t80, t8c, offA;
                Vec3MulScalar(&t74, &axisB, &cs);
                Vec3MulScalar(&t80, &axisA, &sn);
                Vec3Sub(&t8c, &t80, &t74);
                Vec3MulScalar(&offA, &t8c, &sy);
                fn_801CCFB8(&sx, size);
                Vec3 t50, t5c, t68, offB;
                Vec3MulScalar(&t50, &axisB, &sn);
                Vec3MulScalar(&t5c, &axisA, &cs);
                fn_80054484(&t68, &t5c, &t50);
                Vec3MulScalar(&offB, &t68, &sx);
                Vec3 plus, minus;
                fn_80054484(&plus, &pos, &offB);
                Vec3Sub(&minus, &pos, &offB);
                Vec3 v;
                u8* cur;

                Vec3Sub(&v, &plus, &offA);
                cur = lbl_805FB72C;
                fn_801CCFC4(cur, &v);
                fn_80121434(cur, 0.0f, 0.0f);
                fn_801BB338(cur, color);
                fn_80054484(&v, &plus, &offA);
                cur = lbl_805FB72C + 0x18;
                fn_801CCFC4(cur, &v);
                fn_80121434(cur, 0.0f, 1.0f);
                fn_801BB338(cur, color);
                Vec3Sub(&v, &minus, &offA);
                cur = lbl_805FB72C + 0x30;
                fn_801CCFC4(cur, &v);
                fn_80121434(cur, 1.0f, 0.0f);
                fn_801BB338(cur, color);
                fn_80054484(&v, &minus, &offA);
                cur = lbl_805FB72C + 0x48;
                fn_801CCFC4(cur, &v);
                fn_80121434(cur, 1.0f, 1.0f);
                fn_801BB338(cur, color);

                lbl_805FB72C = lbl_805FB72C + 0x60;
                if (lbl_805FB72C >= batchLimit) {
                    fn_801CCFE0();
                    fn_801CA940(batchStart, lbl_805FB72C);
                    lbl_805FB728 = 0;
                    lbl_805FB72C = 0;
                    fn_801CCDC4();
                    batchStart = lbl_805FB72C;
                    batchLimit = lbl_805FB728 + 0x390;
                }
            }
        }
        node = fn_801CC90C(list, node);
    }
    if (lbl_805FB72C != 0 && lbl_805FB72C != batchStart) {
        fn_801CCFE0();
        fn_801CA940(batchStart, lbl_805FB72C);
    }
    lbl_805FB728 = 0;
    lbl_805FB72C = 0;
    return 0;
}

}  // extern "C"
