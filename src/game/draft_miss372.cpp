// Mixed small functions: collision shape recorder (0x8013), BSP triangle query (0x8016), effect
// update (0x8016), draw list flush (0x8018), object teardown (0x8018), matrix array builder (0x8018).
#include "types.h"
#include "engine/math.h"

extern "C" {
void  ScalarSet(f32* out, f32 v);                              // 0x80053250
void  Vec3Copy(Vec3* out, const Vec3* v);                      // 0x80052640
f32   Vec3GetX(const Vec3* v);
f32   Vec3GetY(const Vec3* v);
f32   Vec3GetZ(const Vec3* v);
void  Vec3MulScalar(Vec3* out, const Vec3* v, const f32* s);   // 0x80053224
void  Vec3SetZero(Vec3* out);                                  // 0x800531A8
void  fn_800359F4(u8* a3, f32 f1, f32 f2, f32 f3);             // 0x800359F4
void  fn_80052638(void);                                    // 0x80052638 (empty leaf)
void  fn_800526D0(u8* out);                                    // 0x800526D0 (matrix identity)
void  fn_8005535C(u8* dst, const u8* src);                     // 0x8005535C
void  fn_800554DC(u8* out, const u8* a, const u8* b);          // 0x800554DC (matrix multiply)
void  fn_8005C158(Vec3* out, const Vec3* v);                   // 0x8005C158
f32   fn_8015EC78(u8* out, u8* in);                            // 0x8015EC78 (copy vec4)
void  fn_8015F224(u8* p, f32 dt);                              // 0x8015F224
void  fn_80160808(u8* p);                                      // 0x80160808
void  fn_8016120C(u8* mgr, s32 a, u8* slot, u8* state, f32* dt, u32 b);  // 0x8016120C (jump table)
void  fn_80161464(u8* p);                                      // 0x80161464
void  fn_8016A2B8(u8* p);                                      // 0x8016A2B8
void  fn_8016C28C(u8* p, s32 v);                               // 0x8016C28C (empty leaf; leaf file declares void)
f32   fn_8016C290(u8* dst, u8* src);                           // 0x8016C290 (copy vec3)
void  fn_8016D814(u8* p);                                      // 0x8016D814 (empty leaf; leaf file declares void)
s32   fn_8016CBC4(u8* ray, u8* a, u8* b);                      // 0x8016CBC4
s32   fn_8016CD60(u8* ray, u8* a, u8* b, u8* c, f32* t, u8* normal);  // 0x8016CD60
void  fn_8017493C(void* p);                                    // 0x8017493C
void  fn_8017514C(s32 a);                                      // 0x8017514C
void fn_80175C48(void);                                      // 0x80175C48
void  fn_80182B64(u8* list, u32 count);                        // 0x80182B64
void  fn_80182FAC(u8* list, u32 count);                        // 0x80182FAC
void  fn_801833B8(u8* self, u8* list, s32 count);              // 0x801833B8
void  fn_80183BD0(u8* self, u8* list, s32 count);              // 0x80183BD0
void  fn_801868D4(u8* m);                                      // 0x801868D4
void  fn_8018A5F4(void* p);                                    // 0x8018A5F4
void  operator_delete(void* p);                                // 0x801D426C

extern u8  lbl_803EA0D0[];   // 0x803EA0D0 (array of 0x48-byte collision shapes)
extern u8  lbl_8040D500[];   // 0x8040D500
extern u8  lbl_8040E260[];   // 0x8040E260 (effect handle pool)
extern u8  lbl_804129E8[];   // 0x804129E8 (draw statistics, 4 counters)
extern s32 lbl_805FB330;     // 0x805FB330 (number of recorded shapes)
extern u8* lbl_805FB608;     // 0x805FB608 (heap-like object, free function at +4)
extern f32 lbl_805F9208;     // 0x805F9208
extern f32 lbl_805FEF68;     // 0x805FEF68 (1.0f)
extern f32 lbl_805FF2C4;     // 0x805FF2C4 (1.0f)
extern f32 lbl_805FF2C8;     // 0x805FF2C8 (0.0f)
extern f32 lbl_805FF350;     // 0x805FF350 (fixed step, 1/30)
}

typedef s32 (*TriCallback)(u8* query, u8* userData);
typedef void (*HeapFreeFn)(void* p, void* heap);

extern "C" {

// 0x80139514
// Records a collision shape (4 corner points, a flag word from the owner, and a mask test) in the
// global 0x48-byte shape array. Returns 0 if the array (0x180 entries) is full, else 1.
s32 fn_80139514(u8* arg0, u8* arg1) {
    s32 count = lbl_805FB330;
    if (count > 0x17F) {
        return 0;
    }
    u8* entry;
    u8* a;
    if (arg1 != NULL && (a = *(u8**)(arg1 + 0x4C)) != NULL &&
        *(s32*)(*(u8**)(*(u8**)(a + 0x4C) + 0x1C) + 0xC) > 0) {
        u32 flags = **(u32**)(arg0 + 0x14);
        *(u32*)(arg1 + 0x48) = flags;
        entry = lbl_803EA0D0 + lbl_805FB330 * 0x48;
        *(u32*)(entry + 0x44) = flags;
    } else {
        entry = lbl_803EA0D0 + count * 0x48;
        *(s32*)(entry + 0x44) = -1;
    }
    u32 mask = (*(u32*)(arg1 + 0x48) >> 16) & 0xF;
    s32 ok = 0;
    if ((*(u32*)(arg1 + 0x50) & ~mask) == 0 && (*(u32*)(arg1 + 0x54) & mask) != 0) {
        ok = 1;
    }
    if (ok == 0) {
        return 1;
    }
    u8* p = *(u8**)(arg0 + 0x18);
    entry = lbl_803EA0D0 + count * 0x48;
    f32 one = lbl_805FEF68;
    s32 idx = lbl_805FB330;
    *(f32*)(entry + 0x24) = *(f32*)(p + 0x0);
    *(f32*)(entry + 0x28) = *(f32*)(p + 0x4);
    *(f32*)(entry + 0x2C) = *(f32*)(p + 0x8);
    u8* e2 = lbl_803EA0D0 + idx * 0x48;
    p = *(u8**)(arg0 + 0x1C);
    *(f32*)(e2 + 0x30) = *(f32*)(p + 0x0);
    *(f32*)(e2 + 0x34) = *(f32*)(p + 0x4);
    *(f32*)(e2 + 0x38) = *(f32*)(p + 0x8);
    p = *(u8**)(arg0 + 0x1C);
    *(f32*)(e2 + 0x18) = *(f32*)(p + 0x0);
    *(f32*)(e2 + 0x1C) = *(f32*)(p + 0x4);
    *(f32*)(e2 + 0x20) = *(f32*)(p + 0x8);
    p = *(u8**)(arg0 + 0x20);
    *(f32*)(e2 + 0xC) = *(f32*)(p + 0x0);
    *(f32*)(e2 + 0x10) = *(f32*)(p + 0x4);
    *(f32*)(e2 + 0x14) = *(f32*)(p + 0x8);
    p = *(u8**)(arg0 + 0x24);
    *(f32*)(e2 + 0x0) = *(f32*)(p + 0x0);
    *(f32*)(e2 + 0x4) = *(f32*)(p + 0x4);
    *(f32*)(e2 + 0x8) = *(f32*)(p + 0x8);
    lbl_805FB330 = idx + 1;
    *(f32*)(arg1 + 0x18) = one;
    return 1;
}

// 0x80161A28
// Per-frame update of the effect manager: steps every active slot (0x40 slots of 0xD8 bytes)
// by dt until it finishes (state 4) or the time runs out, then advances the fixed-step counter.
void fn_80161A28(u8* arg0, f32 dt) {
    f32 rem;
    f32 zero;

    fn_80160808(lbl_8040E260);
    *(s32*)(arg0 + 0x3614) = 0;
    zero = lbl_805FF2C8;
    u8* slot = arg0;
    for (s32 i = 0; i < 0x40; i++, slot += 0xD8) {
        if (*(s32*)(slot + 0xD0) != 0) {
            rem = dt;
            while (*(s32*)(slot + 0xD0) != 4 && rem > zero) {
                fn_8016120C(arg0, *(s32*)(slot + 0xC4), slot, slot + 0xD0, &rem, *(u8*)(slot + 0xD4));
            }
            if (*(s32*)(slot + 0xD0) == 4) {
                s32 gen = *(s32*)(slot + 0xC0) + 0x40;
                *(s32*)(slot + 0xC0) = gen;
                *(s32*)(slot + 0xD0) = 0;
                *(s32*)(slot + 0xC8) = *(s32*)(arg0 + 0x3600);
                *(s32*)(arg0 + 0x3600) = gen & 0x3F;
            }
        }
    }
    if (*(s32*)(arg0 + 0x3614) != 0) {
        *(s32*)(arg0 + 0x40BC) = 0;
    }
    if (*(s32*)(arg0 + 0x40BC) == 0) {
        f32 step = lbl_805FF2C4;
        if (step >= dt) {
            step = dt;
        }
        f32 fixed = lbl_805FF350;
        *(f32*)(arg0 + 0x40C0) = *(f32*)(arg0 + 0x40C0) + step;
        while (*(f32*)(arg0 + 0x40C0) >= fixed) {
            fn_80161464(arg0);
        }
    } else {
        *(f32*)(arg0 + 0x40C0) = lbl_805FF2C8;
    }
    fn_8015F224(lbl_8040D500, dt);
}

// 0x8016CA24
// Walks a BSP-like tree (arg0: +8 node count, +0x18 vertices, +0x1C per-triangle data, +0x20 nodes)
// with the ray arg1; for every triangle hit it fills a query record and calls the callback arg2
// (query, arg3). Stops as soon as the callback returns 0.
void fn_8016CA24(u8* arg0, u8* arg1, TriCallback arg2, u8* arg3) {
    u8 query[0x40];       // sp30 (0x28 bytes on GC; padded for 8-byte pointers): ray copy, hit t, data/normal/vertex ptrs
    Vec3 normal;          // sp20
    Vec3 boxMin;          // sp14
    Vec3 boxMax;          // sp8
    s32 stack[44];        // sp58: pending node offsets

    fn_8016C28C(arg0, 0x43);
    if (*(s32*)(arg0 + 8) > 0) {
        fn_8016D814(query);
        fn_8015EC78(query, arg1);
        fn_80052638();
        s32* sp = stack;
        *(u8**)(query + 0x18) = (u8*)&normal;
        s32 off = 0;
        for (;;) {
            u8* node = *(u8**)(arg0 + 0x20) + off;
            fn_8016C290((u8*)&boxMin, node);
            fn_8016C290((u8*)&boxMax, node + 0x10);
            if (fn_8016CBC4(arg1, (u8*)&boxMin, (u8*)&boxMax) != 0) {
                u32 link = *(u32*)(node + 0xC);
                if ((link & 0x80000000u) != 0) {
                    // Leaf: list of triangles (8 bytes each) starting at +link.
                    s32 n = *(s32*)(node + 0x1C);
                    u8* tri = *(u8**)(arg0 + 0x20) + (link & 0x7FFFFFFF);
                    do {
                        u8* verts = *(u8**)(arg0 + 0x18);
                        u8* v0 = verts + *(u16*)(tri + 0) * 0xC;
                        u8* v1 = verts + *(u16*)(tri + 2) * 0xC;
                        u8* v2 = verts + *(u16*)(tri + 4) * 0xC;
                        if (fn_8016CD60(arg1, v0, v1, v2, (f32*)(query + 0x10), (u8*)&normal) != 0) {
                            *(u8**)(query + 0x14) = *(u8**)(arg0 + 0x1C) + *(u16*)(tri + 6) * 4;
                            *(u8**)(query + 0x1C) = v0;
                            *(u8**)(query + 0x20) = v1;
                            *(u8**)(query + 0x24) = v2;
                            if (arg2(query, arg3) == 0) {
                                return;
                            }
                        }
                        n--;
                        tri += 8;
                    } while (n > 0);
                    if (sp <= stack) {
                        return;
                    }
                    off = *--sp;
                } else {
                    // Inner node: remember the second child, descend into the first.
                    *sp++ = *(s32*)(node + 0x1C);
                    off = *(s32*)(node + 0xC);
                }
            } else {
                if (sp <= stack) {
                    return;
                }
                off = *--sp;
            }
        }
    }
}

// 0x80183CFC
// Flushes the four pending draw lists (counts at +0x18..+0x24, lists at +8..+0x14): updates the
// draw statistics, sorts and draws each list, then clears its count.
void fn_80183CFC(u8* arg0) {
    if (*(s32*)(arg0 + 0x18) > 0) {
        fn_8017514C(0);
        *(s32*)(lbl_804129E8 + 0x0) = *(s32*)(lbl_804129E8 + 0x0) + *(s32*)(arg0 + 0x18);
        fn_80182B64(*(u8**)(arg0 + 0x8), *(s32*)(arg0 + 0x18));
        fn_80183BD0(arg0, *(u8**)(arg0 + 0x8), *(s32*)(arg0 + 0x18));
        *(s32*)(arg0 + 0x18) = 0;
        fn_80175C48();
    }
    if (*(s32*)(arg0 + 0x1C) > 0) {
        fn_8017514C(1);
        *(s32*)(lbl_804129E8 + 0x4) = *(s32*)(lbl_804129E8 + 0x4) + *(s32*)(arg0 + 0x1C);
        fn_80182B64(*(u8**)(arg0 + 0xC), *(s32*)(arg0 + 0x1C));
        fn_801833B8(arg0, *(u8**)(arg0 + 0xC), *(s32*)(arg0 + 0x1C));
        *(s32*)(arg0 + 0x1C) = 0;
        fn_80175C48();
    }
    if (*(s32*)(arg0 + 0x20) > 0) {
        fn_8017514C(2);
        *(s32*)(lbl_804129E8 + 0x8) = *(s32*)(lbl_804129E8 + 0x8) + *(s32*)(arg0 + 0x20);
        fn_80182B64(*(u8**)(arg0 + 0x10), *(s32*)(arg0 + 0x20));
        fn_801833B8(arg0, *(u8**)(arg0 + 0x10), *(s32*)(arg0 + 0x20));
        *(s32*)(arg0 + 0x20) = 0;
        fn_80175C48();
    }
    if (*(s32*)(arg0 + 0x24) > 0) {
        fn_8017514C(3);
        *(s32*)(lbl_804129E8 + 0xC) = *(s32*)(lbl_804129E8 + 0xC) + *(s32*)(arg0 + 0x24);
        fn_80182FAC(*(u8**)(arg0 + 0x14), *(s32*)(arg0 + 0x24));
        fn_801833B8(arg0, *(u8**)(arg0 + 0x14), *(s32*)(arg0 + 0x24));
        *(s32*)(arg0 + 0x24) = 0;
        fn_80175C48();
    }
    *(s32*)(arg0 + 0x4) = 0;
}

// Releases a counted sub-object at +0x14 of o (calls fn_8016A2B8 when its count at +0x10 reaches
// zero), clears o+0 and frees o through the heap object.
static void ReleaseHeapObject(u8* o) {
    u8* sub = *(u8**)(o + 0x14);
    if (sub != NULL) {
        s32 n = *(s32*)(sub + 0x10) - 1;
        *(s32*)(sub + 0x10) = n;
        if (n == 0) {
            fn_8016A2B8(sub);
        }
        *(u8**)(o + 0x14) = NULL;
    }
    *(s32*)(o + 0x0) = 0;
    (*(HeapFreeFn*)(lbl_805FB608 + 0x4))(o, lbl_805FB608);
}

// 0x8018A41C
// Deleting destructor: frees the three 0x24-byte slots' payloads and heap objects, then the three
// heap objects at +0x74/+0x78/+0x7C, runs the base destructor and frees p if del > 0.
u8* fn_8018A41C(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        u8* slot = arg0;
        for (s32 i = 0; i < 3; i++, slot += 0x24) {
            if (*(u8**)(slot + 0x8) != NULL) {
                fn_8017493C(*(u8**)(slot + 0x8));
                *(u8**)(slot + 0x8) = NULL;
                u8* o = *(u8**)(slot + 0x1C);
                ReleaseHeapObject(o);
                *(u8**)(slot + 0x1C) = NULL;
            }
        }
        u8* o = *(u8**)(arg0 + 0x74);
        if (o != NULL) {
            ReleaseHeapObject(o);
            *(u8**)(arg0 + 0x74) = NULL;
        }
        o = *(u8**)(arg0 + 0x78);
        if (o != NULL) {
            ReleaseHeapObject(o);
            *(u8**)(arg0 + 0x78) = NULL;
        }
        o = *(u8**)(arg0 + 0x7C);
        if (o != NULL) {
            ReleaseHeapObject(o);
            *(u8**)(arg0 + 0x7C) = NULL;
        }
        fn_8018A5F4(arg0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x80186E1C
// Builds `count` 3x4 transforms into out (first 9 floats: scaled axes, then 12 floats of the
// product with a fixed matrix) from 3x3 float inputs, stepping both arrays by stride floats.
void fn_80186E1C(f32* arg0, u8* arg1, s32 arg2, s32 arg3) {
    u8 fixedM[0x30];   // spE8
    u8 m[0x30];        // spB8 (+0xC / +0x18 / +0x24 hold the other columns)
    u8 prod[0x30];     // sp88
    Vec3 scaled0, scaled1, scaled2;  // sp78 / sp6C / sp60
    Vec3 t0, t1, t2;   // sp54 / sp48 / sp3C
    Vec3 n0, n1, n2;   // sp30 / sp24 / sp18
    Vec3 zero;         // spC
    f32 scale;         // sp8
    Vec3* c0 = (Vec3*)(m + 0x0);
    Vec3* c1 = (Vec3*)(m + 0xC);
    Vec3* c2 = (Vec3*)(m + 0x18);
    Vec3* c3 = (Vec3*)(m + 0x24);
    u8* in = arg1;
    f32* out = arg0;
    s32 n = arg2;

    fn_800526D0(fixedM);
    fn_801868D4(fixedM);
    ScalarSet(&scale, lbl_805F9208);
    fn_800526D0(m);
    s32 stride = arg3 * 4;
    while (n-- > 0) {
        fn_800359F4((u8*)&t0, *(f32*)(in + 0x0), *(f32*)(in + 0x10), *(f32*)(in + 0x20));
        Vec3Copy(c0, &t0);
        fn_800359F4((u8*)&t1, *(f32*)(in + 0x4), *(f32*)(in + 0x14), *(f32*)(in + 0x24));
        Vec3Copy(c1, &t1);
        fn_800359F4((u8*)&t2, *(f32*)(in + 0x8), *(f32*)(in + 0x18), *(f32*)(in + 0x28));
        Vec3Copy(c2, &t2);
        fn_8005C158(&n0, c0);
        Vec3Copy(c0, &n0);
        fn_8005C158(&n1, c1);
        Vec3Copy(c1, &n1);
        fn_8005C158(&n2, c2);
        Vec3Copy(c2, &n2);
        Vec3MulScalar(&scaled0, c0, &scale);
        Vec3MulScalar(&scaled1, c1, &scale);
        Vec3MulScalar(&scaled2, c2, &scale);
        out[0x30 / 4] = Vec3GetX(&scaled0);
        out[0x34 / 4] = Vec3GetX(&scaled1);
        out[0x38 / 4] = Vec3GetX(&scaled2);
        out[0x3C / 4] = Vec3GetY(&scaled0);
        out[0x40 / 4] = Vec3GetY(&scaled1);
        out[0x44 / 4] = Vec3GetY(&scaled2);
        out[0x48 / 4] = Vec3GetZ(&scaled0);
        out[0x4C / 4] = Vec3GetZ(&scaled1);
        out[0x50 / 4] = Vec3GetZ(&scaled2);
        Vec3SetZero(&zero);
        Vec3Copy(c3, &zero);
        fn_800554DC(prod, m, fixedM);
        fn_8005535C(m, prod);
        out[0x00 / 4] = Vec3GetX(c0);
        out[0x04 / 4] = Vec3GetX(c1);
        out[0x08 / 4] = Vec3GetX(c2);
        out[0x0C / 4] = Vec3GetX(c3);
        out[0x10 / 4] = Vec3GetY(c0);
        out[0x14 / 4] = Vec3GetY(c1);
        out[0x18 / 4] = Vec3GetY(c2);
        out[0x1C / 4] = Vec3GetY(c3);
        out[0x20 / 4] = Vec3GetZ(c0);
        out[0x24 / 4] = Vec3GetZ(c1);
        out[0x28 / 4] = Vec3GetZ(c2);
        out[0x2C / 4] = Vec3GetZ(c3);
        in += stride;
        out += arg3;
    }
}

}  // extern "C"
