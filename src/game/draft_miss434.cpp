// Drafts miss434: player ground-ray shadow/decal probe (0x800558BC) and two ribbon vertex builders
// (0x8010ABA8, 0x8010AED8).
#include <math.h>

#include "types.h"
#include "revolution.h"

extern u8  lbl_803C4298[];   // 0x803C4298 (level info block; +4 = camera controller)
extern u8  gWorldState[];    // 0x803E6D00 (first word: world block pointer)
extern u8* lbl_805FB318;     // 0x805FB318 (nearest overlapping object found by WorldPostUpdate)
extern s32 lbl_805FB4C4;     // 0x805FB4C4 (vertex buffer write offset)
extern s32 lbl_805FB4CC;     // 0x805FB4CC (vertex buffer bytes left)
extern u8* lbl_805FB4D0;     // 0x805FB4D0 (vertex buffer base)
extern s32 lbl_805FB6B8;     // 0x805FB6B8

extern "C++" void fn_801B503C(u8* p);  // 0x801B503C (C++ linkage, defined in small_801Bb.cpp)

extern "C" {
void  fn_80054B48(void* pos, s32 b, f32 strength, f32 blend);                // 0x80054B48
BOOL  RayQueryCast(u8* q, void* world, u32 filter, u8* hit, u8* hit2);       // 0x801B203C
void  RayQueryInit(u8* q);                                                   // 0x801B11DC
s32   TriggerDoAction(void* action, void* a1, void* a2);                     // 0x801E0054
void  fn_8014DB90(u8* verts, s32 count);                                     // 0x8014DB90
void  fn_8014E3EC(s32 n);                                                    // 0x8014E3EC
s32   fn_80151644(s32 a, s32 b, u8* quad);                                   // 0x80151644
void  fn_80152620(s32 a, s32 b, u8* quad);                                   // 0x80152620
void* fn_801ACCA0(u8* arg0, s32 arg1);                                       // 0x801ACCA0
void* fn_801B7198(void* anim, s32 a);                                        // 0x801B7198
}

static const u32 kHashBAse = 0xF62113D5u;  // "BAse"

// 1 / sqrt, refined like the frsqrte + two Newton steps in the original.
static f32 RSqrtRefined(f32 x) { return (f32)(1.0 / sqrt((f64)x)); }

extern "C" {

// 0x800558BC
// Per-frame ground contact for the world's player object (gWorldState block +8) when it is not
// hidden (+0x838 bit 0x80000) and an overlap object exists. The object it stands on is the handle
// at +0x7B0; it is kept when +0x7B4 and its +0x8C are set, otherwise the handle is released (the
// old object's triggers fire, +0x790 is set when it had no +0x8C) and reset. With a valid object
// calls fn_80054B48 at the position (+4) at full blend; without one, casts a ray straight down
// from the position and, when it hits within 5 units, calls fn_80054B48 with the hit and a blend
// of (5 - distance) / 5. The asm also fills several dead stack vectors, not reproduced here.
void fn_800558BC(u8* arg0) {
    f32 strength;
    void* obj;
    s32 handle;

    fn_801B503C(arg0);
    if (!((*(u32*)(arg0 + 0x838) >> 0x13) & 1) && arg0 == *(u8**)(*(u8**)gWorldState + 0x8) && lbl_805FB318 != NULL) {
        strength = *(f32*)(arg0 + 0x14);
        if (*(s32*)(arg0 + 0x8C) != 0) {
            strength *= 0.6f;
        }
        handle = *(s32*)(arg0 + 0x7B0);
        obj = fn_801B7198(&handle, 0);
        if (obj != NULL && *(s32*)(arg0 + 0x7B4) != 0 && *(s32*)((u8*)obj + 0x8C) != 0) {
            // keep the current standing object
        } else {
            u8* old;
            handle = *(s32*)(arg0 + 0x7B0);
            old = (u8*)fn_801B7198(&handle, 0);
            if (old != NULL) {
                u8* sub;
                if (*(s32*)(old + 0x8C) == 0) {
                    *(s32*)(arg0 + 0x790) = 1;
                    *(u32*)(arg0 + 0x838) |= 0x02000000;
                }
                sub = *(u8**)(old + 0x9C);
                if (sub != NULL) {
                    if ((*(u32*)(sub + 0xA4) >> 0x1B) & 0xF) {
                        TriggerDoAction(sub + 0xA4, arg0, old);
                    }
                }
                if ((*(u32*)(old + 0x74) >> 0x1B) & 0xF) {
                    TriggerDoAction(old + 0x74, arg0, old);
                }
            }
            obj = NULL;
            *(s32*)(arg0 + 0x7B4) = 0;
            *(s32*)(arg0 + 0x7B0) = lbl_805FB6B8;
            *(f32*)(arg0 + 0x7B8) = 0.0f;
            *(f32*)(arg0 + 0x7BC) = 0.0f;
            *(f32*)(arg0 + 0x7C0) = 0.0f;
            *(f32*)(arg0 + 0x7C4) = 0.0f;
            *(f32*)(arg0 + 0x7C8) = 0.0f;
            *(f32*)(arg0 + 0x7CC) = 0.0f;
            *(f32*)(arg0 + 0x7D0) = 1.0f;
        }
        if (obj != NULL) {
            fn_80054B48(*(void**)(arg0 + 0x4), *(s32*)(arg0 + 0x7B4), strength, 1.0f);
            return;
        }
        if (lbl_805FB318 != NULL) {
            u8 query[0x6C];    // asm: sp 0x14C
            u8 hit[0x6C];      // asm: sp 0xE0
            u8 hit2[0x6C];     // asm: sp 0x74
            f32* start;
            s32 i;

            RayQueryInit(query);
            *(s32*)(query + 0xC) = 6;
            for (i = 0; i < 0x6C; i++) {
                hit[i] = 0;
                hit2[i] = 0;
            }
            *(s32*)(hit + 0x54) = 7;
            *(s32*)(hit2 + 0x54) = 7;
            start = *(f32**)(arg0 + 0x4);
            *(f32*)(query + 0x3C) = start[0];
            *(f32*)(query + 0x40) = start[1];
            *(f32*)(query + 0x44) = start[2];
            *(f32*)(query + 0x48) = -0.0f;     // ray direction: straight down
            *(f32*)(query + 0x4C) = -0.0f;
            *(f32*)(query + 0x50) = -1.0f;
            if (RayQueryCast(query, *(void**)(lbl_805FB318 + 0x8C), kHashBAse, hit, hit2) != 0 && *(f32*)(hit + 0x18) < 5.0f) {
                fn_80054B48(hit, *(s32*)(hit + 0x68), strength, (5.0f - *(f32*)(hit + 0x18)) / 5.0f);
            }
            fn_801ACCA0(query, 0);
        }
    }
}

// 0x8010ABA8
// Builds a camera-facing ribbon strip from the path points in arg0 (0x28-byte records at +0,
// count at +0x24, half-width at +0x38): takes each segment direction, crosses it with the vector
// to the camera and offsets the point +/- width along the normalised result. Writes two
// 0x18-byte vertices per point (position, colour word, u, 0/1) into the temp vertex buffer and
// submits them with fn_8014DB90. arg1 is unused.
void fn_8010ABA8(u8* arg0, s32 arg1) {
    s32 size = ((*(s32*)(arg0 + 0x24) * 2 * 0x18) + 0x1F) & ~0x1F;
    u8* base;
    u8* out;
    s32 i;
    s32 rec;   // byte offset into the 0x28-byte point records
    s32 uv;    // byte offset into the 0xC-byte uv/colour records

    if (lbl_805FB4CC < size) {
        fn_8014E3EC(size);
    }
    base = lbl_805FB4D0 + lbl_805FB4C4;
    lbl_805FB4C4 += size;
    lbl_805FB4CC -= size;
    out = base;
    rec = 0;
    uv = 0;
    for (i = 0; i < *(s32*)(arg0 + 0x24); i++) {
        f32 dir[3];
        f32 n[3];
        f32 off[3];
        f32 plus[3];
        f32 minus[3];
        f32 to[3];
        f32 cx, cy, cz, len2;
        u8* p = *(u8**)arg0 + rec;
        u8* cur = p + 0xC;
        u8* cam;
        s32 color;

        if (i > 0) {
            dir[0] = *(f32*)(p + 0xC) - *(f32*)(p - 0x1C);
            dir[1] = *(f32*)(p + 0x10) - *(f32*)(p - 0x18);
            dir[2] = *(f32*)(p + 0x14) - *(f32*)(p - 0x14);
        } else {
            dir[0] = *(f32*)(p + 0x34) - *(f32*)(p + 0xC);
            dir[1] = *(f32*)(p + 0x38) - *(f32*)(p + 0x10);
            dir[2] = *(f32*)(p + 0x3C) - *(f32*)(p + 0x14);
        }
        cam = *(u8**)(*(u8**)(lbl_803C4298 + 0x4) + 0x4);
        to[0] = *(f32*)(cam + 0x0) - *(f32*)(cur + 0x0);
        to[1] = *(f32*)(cam + 0x4) - *(f32*)(cur + 0x4);
        to[2] = *(f32*)(cam + 0x8) - *(f32*)(cur + 0x8);
        cx = to[1] * dir[2] - to[2] * dir[1];
        cy = to[2] * dir[0] - to[0] * dir[2];
        cz = to[0] * dir[1] - to[1] * dir[0];
        len2 = cz * cz + (cx * cx + cy * cy);
        if (len2 == 0.0f) {
            n[0] = 0.0f;
            n[1] = 0.0f;
            n[2] = 0.0f;
        } else {
            f32 r = RSqrtRefined(len2);
            n[0] = cx * r;
            n[1] = cy * r;
            n[2] = cz * r;
        }
        off[0] = n[0] * *(f32*)(arg0 + 0x38);
        off[1] = n[1] * *(f32*)(arg0 + 0x38);
        off[2] = n[2] * *(f32*)(arg0 + 0x38);
        plus[0] = *(f32*)(p + 0xC) + off[0];
        plus[1] = *(f32*)(p + 0x10) + off[1];
        plus[2] = *(f32*)(p + 0x14) + off[2];
        minus[0] = *(f32*)(p + 0xC) - off[0];
        minus[1] = *(f32*)(p + 0x10) - off[1];
        minus[2] = *(f32*)(p + 0x14) - off[2];
        if (*(u8*)(arg0 + 0x49) != 0) {
            color = *(s32*)(*(u8**)(arg0 + 0x20) + uv + 0x4);
        } else {
            u8* c = *(u8**)(arg0 + 0x34);
            color = (c != NULL) ? *(s32*)(c + 0x4) : -1;
        }
        rec += 0x28;
        *(f32*)(out + 0x0) = plus[0];
        *(f32*)(out + 0x4) = plus[1];
        *(f32*)(out + 0x8) = plus[2];
        *(s32*)(out + 0xC) = color;
        *(f32*)(out + 0x10) = *(f32*)(*(u8**)(arg0 + 0x20) + uv);
        *(f32*)(out + 0x14) = 0.0f;
        *(f32*)(out + 0x18) = minus[0];
        *(f32*)(out + 0x1C) = minus[1];
        *(f32*)(out + 0x20) = minus[2];
        *(s32*)(out + 0x24) = color;
        *(f32*)(out + 0x28) = *(f32*)(*(u8**)(arg0 + 0x20) + uv);
        *(f32*)(out + 0x2C) = 1.0f;
        uv += 0xC;
        out += 0x30;
    }
    fn_8014DB90(base, *(s32*)(arg0 + 0x24));
}

// 0x8010AED8
// Builds a vertex list from the same ribbon record as fn_8010ABA8 (no camera facing): per point
// a vertex of 0x10 bytes (colour word, position) or, when arg1 is set, 0x18 bytes (u, 0.0, colour
// word, position), and submits it with opcode 0xB0 through fn_80151644 (arg1 != 0) or
// fn_80152620.
void fn_8010AED8(u8* arg0, s32 arg1) {
    u32 vsize = 0x10;
    s32 size;
    u8* base;
    u8* out;
    s32 i;
    s32 uv;
    s32 rec;

    if (arg1 != 0) {
        vsize = 0x18;
    }
    size = ((vsize * (*(s32*)(arg0 + 0x24) * 4) * 4) + 0x1F) & ~0x1F;
    if (lbl_805FB4CC < size) {
        fn_8014E3EC(size);
    }
    base = lbl_805FB4D0 + lbl_805FB4C4;
    lbl_805FB4C4 += size;
    lbl_805FB4CC -= size;
    out = base;
    uv = 0;
    rec = 0;
    for (i = 0; i < *(s32*)(arg0 + 0x24); i++) {
        u8* p;
        if (vsize == 0x18) {
            *(f32*)(out + 0x0) = *(f32*)(*(u8**)(arg0 + 0x20) + uv);
            *(f32*)(out + 0x4) = 0.0f;
            if (*(u8*)(arg0 + 0x49) != 0) {
                *(s32*)(out + 0x8) = *(s32*)(*(u8**)(arg0 + 0x20) + uv + 0x4);
            } else {
                *(s32*)(out + 0x8) = *(s32*)(*(u8**)(arg0 + 0x34) + 0x4);
            }
            out += 0xC;
        } else {
            *(s32*)(out + 0x0) = *(s32*)(*(u8**)(arg0 + 0x20) + uv + 0x4);
            out += 0x4;
        }
        uv += 0xC;
        p = *(u8**)arg0 + rec;
        *(f32*)(out + 0x0) = *(f32*)(p + 0xC);
        *(f32*)(out + 0x4) = *(f32*)(p + 0x10);
        rec += 0x28;
        *(f32*)(out + 0x8) = *(f32*)(p + 0x14);
        out += 0xC;
    }
    if (arg1 != 0) {
        fn_80151644(0xB0, *(s32*)(arg0 + 0x24), base);
        return;
    }
    fn_80152620(0xB0, *(s32*)(arg0 + 0x24), base);
}

}  // extern "C"
