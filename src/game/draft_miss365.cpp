// Functions from the draft_miss365 batch (0x80184604, 0x80184AB0, 0x801873C0, 0x80208D00,
// 0x8020DEA0): light colour accumulation, a display-list draw record, the listener-relative
// camera angles and the part-action target filter.
#include "types.h"
#include "revolution.h"
#include "game/part.h"
#include <math.h>

extern "C++" u8 sRendDLs[];   // 0x80401108 (display-list records, 0x94 bytes each)
extern u8* gBattle;           // 0x805FBBB8 (+0xA00 fighters[2])

extern "C" {
void  TargetListInit(void* t);                                     // 0x802161DC
void  TargetListDecode(void* t, const u32* mask, s32 side);        // 0x80216240
void* TargetListDestroy(void* t, s32 del);                         // 0x80216200
void  fn_8014DE68(void);                                           // 0x8014DE68
s32   fn_8015BA30(void* p);                                        // 0x8015BA30
u8*   fn_80183FA0(u8* a);                                          // 0x80183FA0
void  fn_80185C98(f32* out, u8* src, s32 n, s32 idx);              // 0x80185C98
void  fn_80185F1C(f32* out, u8* src, s32 n, s32 idx);              // 0x80185F1C
void  fn_80186E1C(f32* out, u8* src, s32 n, s32 idx);              // 0x80186E1C
void  fn_801870AC(s32 n, u8* p, s32 arg, s32 flags);               // 0x801870AC
void  fn_80200494(u8* obj, f32 a, f32 b, f32 c, f32 d);            // 0x80200494 (args in f1..f4, none in r4)
f64   fn_8033F1F4(f64 x);                                          // 0x8033F1F4 (acos)
f64   fn_8033F204(f64 x);                                          // 0x8033F204 (natural log)

extern u8* lbl_805FB5D8;      // 0x805FB5D8 (single light override, or NULL)
extern u8  lbl_80412B20[];    // 0x80412B20 (static record)
extern u8  lbl_804129E8[];    // 0x804129E8 (draw statistics)
extern u8  lbl_80401000[];    // 0x80401000
extern u8  lbl_8055C978[];    // 0x8055C978 (Vec3 listener positions, 12 bytes per id)
extern u8  lbl_8055C9A8[];    // 0x8055C9A8 (Vec3 listener forward vectors, 12 bytes per id)
extern u8  lbl_8055C9D8[];    // 0x8055C9D8 (Vec3 listener up vectors, 12 bytes per id)
extern u8  lbl_805FBBB4[];    // 0x805FBBB4 (listener-position valid flag per id)
extern s32 lbl_805FB4BC;      // 0x805FB4BC (current viewport index)
extern s32 lbl_805FB4C0;      // 0x805FB4C0 (display-list recording flag)
extern s32 lbl_805FB4C8;      // 0x805FB4C8 (render budget)
}

// Clamps the RGBA floats at obj + 0x48..0x54 to [0, 1] (the original clamps low, then high).
static void ClampColor(u8* obj) {
    for (s32 i = 0; i < 4; i++) {
        f32 v = *(f32*)(obj + 0x48 + i * 4);
        if (!(v > 0.0f)) v = 0.0f;
        *(f32*)(obj + 0x48 + i * 4) = v;
    }
    for (s32 i = 0; i < 4; i++) {
        f32 v = *(f32*)(obj + 0x48 + i * 4);
        if (!(v < 1.0f)) v = 1.0f;
        *(f32*)(obj + 0x48 + i * 4) = v;
    }
}

// Normalizes in[0..2] into out[0..2]; a zero-length vector gives zeros (original: frsqrte plus two
// Newton steps).
static void Normalize3(const f32* in, f32* out) {
    f32 l2 = (in[0] * in[0] + in[1] * in[1]) + in[2] * in[2];
    if (l2 == 0.0f) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(l2);
        out[0] = in[0] * inv;
        out[1] = in[1] * inv;
        out[2] = in[2] * inv;
    }
}

// Clamped arccos converted to the original's angle units (acos * 2/pi * pi/2 = radians).
static f32 AngleFromDot(f32 d) {
    f32 c = d;
    if (d < -1.0f) c = -1.0f;
    else if (d > 1.0f) c = 1.0f;
    return 1.5707964f * (0.63661975f * (f32)fn_8033F1F4((f64)c));
}

extern "C" {

// 0x80184604
// Accumulates the lights of the scene list (type 3 = ambient add, 4 = ambient multiply, 0 =
// directional) into arg0: direction record at +0x58, clamped colour at +0x48.
void fn_80184604(u8* arg0) {
    f32 dirSum[3] = {0.0f, 0.0f, 0.0f};
    f32 colSum[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    f32 add[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    f32 mul[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    s32 count = 0;
    u8* single = lbl_805FB5D8;
    u8* link = *(u8**)(arg0 + 0x10);

    for (;;) {
        u8* node;
        if (single == NULL) {
            if (link == NULL) break;
            node = link - 4;
        } else {
            node = single;
        }
        s16 type = *(s16*)(node + 0x10);
        switch (type) {
        case 3:
            add[0] += *(f32*)(node + 0x44);
            add[1] += *(f32*)(node + 0x48);
            add[2] += *(f32*)(node + 0x4C);
            add[3] += *(f32*)(node + 0x50);
            break;
        case 4:
            mul[0] *= *(f32*)(node + 0x44);
            mul[1] *= *(f32*)(node + 0x48);
            mul[2] *= *(f32*)(node + 0x4C);
            mul[3] *= *(f32*)(node + 0x50);
            break;
        case 0:
            count++;
            dirSum[0] += *(f32*)(node + 0x38);
            dirSum[1] += *(f32*)(node + 0x3C);
            dirSum[2] += *(f32*)(node + 0x40);
            colSum[0] += *(f32*)(node + 0x44);
            colSum[1] += *(f32*)(node + 0x48);
            colSum[2] += *(f32*)(node + 0x4C);
            colSum[3] += *(f32*)(node + 0x50);
            break;
        }
        if (single != NULL) break;
        link = *(u8**)link;
    }

    if (count > 0) {
        *(u8**)(arg0 + 0x58) = fn_80183FA0(lbl_80412B20);
        f32 dir[3];
        Normalize3(dirSum, dir);
        u8* rec = *(u8**)(arg0 + 0x58);
        *(f32*)(rec + 0x0) = dir[0];
        *(f32*)(rec + 0x4) = dir[1];
        *(f32*)(rec + 0x8) = dir[2];
        f32 inv = 1.0f / (f32)count;
        rec = *(u8**)(arg0 + 0x58);
        *(f32*)(rec + 0xC) = colSum[0] * inv;
        *(f32*)(rec + 0x10) = colSum[1] * inv;
        *(f32*)(rec + 0x14) = colSum[2] * inv;
        *(f32*)(rec + 0x18) = colSum[3] * inv;
    } else {
        *(u8**)(arg0 + 0x58) = NULL;
    }

    *(f32*)(arg0 + 0x48) = add[0] * mul[0];
    *(f32*)(arg0 + 0x4C) = add[1] * mul[1];
    *(f32*)(arg0 + 0x50) = add[2] * mul[2];
    *(f32*)(arg0 + 0x54) = add[3] * mul[3];
    ClampColor(arg0);
}

// 0x80184AB0
// Sets the light colour (RGBA from arg1, clamped to [0, 1]) and the direction record pointer.
void fn_80184AB0(u8* arg0, u8* arg1, u8* arg2, f32 farg0) {
    *(f32*)(arg0 + 0x48) = *(f32*)(arg1 + 0x0);
    *(f32*)(arg0 + 0x4C) = *(f32*)(arg1 + 0x4);
    *(f32*)(arg0 + 0x50) = *(f32*)(arg1 + 0x8);
    *(f32*)(arg0 + 0x54) = *(f32*)(arg1 + 0xC);
    ClampColor(arg0);
    *(u8**)(arg0 + 0x58) = arg2;
}

// 0x801873C0
// Draws a recorded mesh: flushes if over the render budget, loads the XF matrices for the
// vertex format (flags 0x20 / 0x40 / 0x60), calls the display list and updates the statistics.
void fn_801873C0(u8* arg0, s32 arg1, s32 arg2) {
    f32 m[21];

    if (lbl_805FB4C0 != 0 && (fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x10C0) > lbl_805FB4C8) {
        fn_8014DE68();
    }
    s32 flags = *(s32*)(arg0 + 0xC) & 0x60;
    if (arg1 != 0) {
        fn_801870AC(*(s32*)(arg0 + 0x1C), arg0 + 0x28, arg1, flags);
    } else if (flags == 0x20) {
        fn_80185C98(m, lbl_80401000, 1, 0);
        GXWGFifo._u8 = 0x10;
        GXWGFifo._u16 = 8;
        GXWGFifo._u16 = 0x400;
        for (s32 i = 0; i < 9; i++) GXWGFifo._f32 = m[i];
    } else if (flags == 0x40) {
        fn_80185F1C(m, lbl_80401000, 1, 0);
        GXWGFifo._u8 = 0x10;
        GXWGFifo._u16 = 0xB;
        GXWGFifo._u16 = 0x78;
        for (s32 i = 0; i < 12; i++) GXWGFifo._f32 = m[i];
    } else if (flags == 0x60) {
        fn_80186E1C(m, lbl_80401000, 1, 0);
        GXWGFifo._u8 = 0x10;
        GXWGFifo._u16 = 0xB;
        GXWGFifo._u16 = 0x78;
        for (s32 i = 0; i < 12; i++) GXWGFifo._f32 = m[i];
        GXWGFifo._u8 = 0x10;
        GXWGFifo._u16 = 8;
        GXWGFifo._u16 = 0x400;
        for (s32 i = 12; i < 21; i++) GXWGFifo._f32 = m[i];
    }
    GXWGFifo._u8 = 0x40;
    GXWGFifo._u32 = (u32)(size_t)(arg0 + *(s32*)(arg0 + 0x18));   // display-list address
    GXWGFifo._u32 = *(u32*)(arg0 + 0x24);                         // display-list size
    if (arg1 != 0) {
        *(s32*)(lbl_804129E8 + 0x14) += 1;
        *(s32*)(lbl_804129E8 + 0x18) += *(s32*)(arg0 + 0x4);
        *(s32*)(lbl_804129E8 + 0x1C) += *(s32*)(arg0 + 0x8);
        return;
    }
    *(s32*)(lbl_804129E8 + 0x24) += 1;
    *(s32*)(lbl_804129E8 + 0x28) += *(s32*)(arg0 + 0x4);
    *(s32*)(lbl_804129E8 + 0x2C) += *(s32*)(arg0 + 0x8);
}

// 0x80208D00
// Orients the object (arg0) towards the nearest valid listener: picks the listener closest to
// the object, builds yaw / pitch angles from its forward and up vectors plus a distance-based
// strength (log scale), then calls fn_80200494 with them.
void fn_80208D00(u8* arg0, void* arg1, void* arg2) {
    if (arg0 == NULL || arg1 == NULL || arg2 == NULL) {
        return;
    }
    f32* pos = (f32*)lbl_8055C978;
    f32* fwdTab = (f32*)lbl_8055C9A8;
    f32 ox = *(f32*)(arg0 + 0x18);
    f32 oy = *(f32*)(arg0 + 0x1C);
    f32 oz = *(f32*)(arg0 + 0x20);

    f32 fwd[3] = {fwdTab[0], fwdTab[1], fwdTab[2]};
    f32 dir[3] = {ox - pos[0], oy - pos[1], oz - pos[2]};
    s32 best = 0;
    for (s32 i = 1; i < 4; i++) {
        if (lbl_805FBBB4[i] != 0) {
            f32* p = pos + i * 3;
            f32 d[3] = {ox - p[0], oy - p[1], oz - p[2]};
            f32 bestL2 = (dir[0] * dir[0] + dir[1] * dir[1]) + dir[2] * dir[2];
            f32 newL2 = (d[0] * d[0] + d[1] * d[1]) + d[2] * d[2];
            if (newL2 < bestL2) {
                best = i;
                fwd[0] = fwdTab[i * 3 + 0];
                fwd[1] = fwdTab[i * 3 + 1];
                fwd[2] = fwdTab[i * 3 + 2];
                dir[0] = d[0];
                dir[1] = d[1];
                dir[2] = d[2];
            }
        }
    }

    // Distance and unit direction to the listener (negative squared length leaves the length at 0).
    f32 l2 = (dir[0] * dir[0] + dir[1] * dir[1]) + dir[2] * dir[2];
    f32 dist = 0.0f;
    if (l2 == 0.0f) {
        dir[0] = 0.0f;
        dir[1] = 0.0f;
        dir[2] = 0.0f;
    } else {
        if (l2 > 0.0f) {
            dist = sqrtf(l2);
        }
        f32 inv = 1.0f / dist;
        dir[0] *= inv;
        dir[1] *= inv;
        dir[2] *= inv;
    }

    f32 fn_[3];
    Normalize3(fwd, fn_);
    f32 up[3];
    Normalize3((f32*)(lbl_8055C9D8 + best * 12), up);

    // right = fwd x up, up2 = right x fwd
    f32 right[3] = {fn_[1] * up[2] - fn_[2] * up[1], fn_[2] * up[0] - fn_[0] * up[2],
                    fn_[0] * up[1] - fn_[1] * up[0]};
    f32 up2[3] = {right[1] * fn_[2] - right[2] * fn_[1], right[2] * fn_[0] - right[0] * fn_[2],
                  right[0] * fn_[1] - right[1] * fn_[0]};

    // Yaw: direction with its up2 component removed, angle to forward.
    f32 dotUp = dir[2] * up2[2] + (dir[0] * up2[0] + dir[1] * up2[1]);
    f32 flat[3] = {dir[0] - up2[0] * dotUp, dir[1] - up2[1] * dotUp, dir[2] - up2[2] * dotUp};
    f32 n1[3];
    Normalize3(flat, n1);
    f32 yaw = AngleFromDot(fn_[2] * n1[2] + (fn_[0] * n1[0] + fn_[1] * n1[1]));
    if (fn_[0] * n1[1] - fn_[1] * n1[0] > 0.0f) {
        yaw = -yaw;
    }

    // Pitch: direction with its right component removed, angle to forward.
    f32 dotRight = dir[2] * right[2] + (dir[0] * right[0] + dir[1] * right[1]);
    f32 flat2[3] = {dir[0] - right[0] * dotRight, dir[1] - right[1] * dotRight,
                    dir[2] - right[2] * dotRight};
    f32 n2[3];
    Normalize3(flat2, n2);
    f32 pitch = AngleFromDot(fn_[2] * n2[2] + (fn_[0] * n2[0] + fn_[1] * n2[1]));
    if (fn_[0] * n2[1] - fn_[1] * n2[0] > 0.0f) {
        pitch = -pitch;
    }

    // Strength from the distance between the object's range limits (+0x3C near, +0x40 far).
    f32 nearD = *(f32*)(arg0 + 0x3C);
    f32 t = 1.0f + (-1.0f * ((dist - nearD) / (*(f32*)(arg0 + 0x40) - nearD)));
    f32 s = 0.0f;
    if (t >= 0.0f) {
        s = (t <= 1.0f) ? t : 1.0f;
    }
    f32 strength;
    if (s > -0.0001f && s < 0.0001f) {
        strength = 400.0f;
    } else {
        f32 ln2a = (f32)fn_8033F204(2.0);
        f32 base = (f32)fn_8033F204((f64)2.718281f) / ln2a;
        f32 ln2b = (f32)fn_8033F204(2.0);
        f32 lg = ((f32)fn_8033F204((f64)s) / ln2b) / base;
        strength = 40.0f * ((10.0f * lg) / -6.0f);
        if (strength > 400.0f) strength = 400.0f;
    }
    fn_80200494(arg0, yaw, pitch, dist, strength);
}

// 0x8020DEA0
// Filters the 8-byte action-target records of arg0 (count at +0x84) down to those whose decoded
// target list hits fighter slot arg1 (or, for arg1 == 4, any empty / dead slot). Returns 1 and
// the new count when any record survived.
s32 fn_8020DEA0(u8* arg0, s32 arg1) {
    if (arg1 == -1) {
        return 0;
    }
    u8* rec = arg0;
    s32 kept = 0;
    s32 outOff = 0;
    for (s32 i = 0; i < *(s32*)(arg0 + 0x84); i++, rec += 8) {
        PartAction* actions = *(PartAction**)(gGameDb->data + 0x430);
        PartAction* a = &actions[*(s32*)(rec + 0x38)];
        s32 list[6];
        u32 act[5] = {a->key, (u32)a->type, a->unk08, a->unk0C, a->unk10};
        TargetListInit(list);
        u32 mask = act[2];
        TargetListDecode(list, &mask, *(s32*)(*(u8**)(rec + 0x34) + 0x4));
        s32 count = list[5];
        s16 hits = (s16)act[3];
        for (s32 k = 0; k < count; k++) {
            s32 id = list[k];
            if (hits > 0) {
                u8** fighters = (u8**)(gBattle + 0xA00);
                u8** limbs = (u8**)fighters[1 - *(s32*)(arg0 + 0x4)];
                u8* slot = limbs[id];
                s32 empty = 1;
                if (slot != NULL) {
                    if (**(s32**)(slot + 0x28) > 0) {
                        empty = 0;
                    }
                }
                if (id == arg1 || (empty != 0 && arg1 == 4)) {
                    u8* dst = arg0 + outOff;
                    kept++;
                    outOff += 8;
                    *(u8**)(dst + 0x34) = *(u8**)(rec + 0x34);
                    *(s32*)(dst + 0x38) = *(s32*)(rec + 0x38);
                    break;
                }
            }
        }
        TargetListDestroy(list, -1);
    }
    if (kept != 0) {
        *(s32*)(arg0 + 0x84) = kept;
        return 1;
    }
    return 0;
}

}  // extern "C"
