// Small functions from the miss703 draft batch (verified against the asm).
#include "types.h"
#include <math.h>
extern "C" {
#include "game/script_callbacks.h"
}
#include "engine/math.h"

// Virtual-call signatures used by the slot calls below (vtable slot at +OFF).
typedef void (*Fn_Pfv)(void* a, const void* b, f32 f);   // (obj+0x3020, table, float)
typedef void (*Fn_PP)(void* out, void* in);              // slot 0x24: (out, in)
typedef void (*Fn_OPP)(u8* self, void* a, void* b);      // slot 0x2C: (self, a, b)
typedef void (*Fn_OP)(u8* self, void* a);                // slot 0x1C: (self, a)
typedef void (*Fn_Of)(u8* self, f32 f);                  // slot 0x98: (self, f)
typedef s32 (*Fn_Obj38)(u8* self, s32 zero, f64 a, f64 b); // slot 0x38: (self, 0, x, y)

extern "C" {
s32  RaySceneTest(void* ray);
s32  SaveGetQuestLevel(u8* s, u32 quest);
int  SysSprintf(char* buf, const char* fmt, ...);
void TextureRelease(void* tex);
void GameFlowSignal(s32 id, s32 a, s32 b);
void __register_global_object(void* obj, void (*dtor)(void*), void* handle);
void fn_8032F1E8(u8* obj, void (*cb)(void), s32 a, s32 b, s32 c);
f64  fn_8033F1F4(f64 x);
f32  fn_8033F1FC(f32 y, f32 x);
s32  fn_80109A0C(void* h);
s32  fn_801098DC(void* h);
u8*  fn_80109330(const char* name);
u8*  fn_80222704(u8* s, s32 a, s32 b);
s32  fn_80222994(u8* s);
u8*  fn_801483BC(s32 idx);
void fn_800CFDCC(u8* p, f32 x);
void* fn_800CF71C(void);
void fn_80146180(void* quat, const Vec3* axis, f32 angle);
void fn_801455E0(f32* out, u8* quat);
void fn_801ACD08(u8* a, u8* b, u8* out, f32 x, f32 y);
void fn_801F7240(u8* p, f32* v);
s32  fn_801F72F0(u8* arg0, f32* arg1, s8* arg2);
void fn_801F59A4(u8* a3);
u8*  fn_801F59B0(u8* p, s32 del);

extern u8* gGameDb;
extern u8  lbl_803C4298[];
extern u8  lbl_804DD140[];
extern u8  lbl_805FBBE0[];
extern const char* lbl_805F8F90;
extern s32 lbl_805FB960;
extern f32 lbl_80347290[];
extern f32 lbl_805FE558;
extern f32 lbl_805FE594;
extern f32 lbl_805FE598;
extern f32 lbl_805FE5A8;
extern f32 lbl_805FF95C;
extern f32 lbl_805FFA70;
extern f32 lbl_805FFA74;
extern f32 lbl_805FFAC0;
extern f32 lbl_80600130;
extern f32 lbl_80600150;
extern f32 lbl_80600154;
extern f32 lbl_80600158;
extern f32 lbl_8060015C;
extern f32 lbl_80600160;
extern f32 lbl_80600188;
extern f32 lbl_8060018C;
}

extern "C" {

// Small helper: drop one reference on a texture (+0x24) and release it at zero.
static void DraftTexUnref(u8* tex) {
    s32 rc = *(s32*)(tex + 0x24) - 1;
    *(s32*)(tex + 0x24) = rc;
    if (rc == 0) TextureRelease(tex);
}

// 0x800CF50C
// Updates the scene node's transform from a ray-selected offset and posts a flow signal when the
// accumulated value at +0x8C drops below 0.5.
void fn_800CF50C(u8* arg0, f32 farg0) {
    f32 v[3];     // sp8
    f32 d[3];     // sp14
    f32 q[4];     // sp20
    f32 ax[3];    // sp30
    u8* obj;
    u8* obj2;
    u8* st;

    s32 idx = RaySceneTest(arg0);
    u8* vt = *(u8**)(arg0 + 0x30F4);
    (*(Fn_Pfv*)(vt + 0x1C))(arg0 + 0x3020, lbl_80347290, farg0 * lbl_80347290[idx]);
    v[0] = lbl_805FE558;
    v[1] = lbl_805FE594;
    v[2] = lbl_805FE558;
    // Second slot call reuses the leftover table pointer (r4) and f1 = 0.
    (*(Fn_Pfv*)(vt + 0x20))(arg0 + 0x3020, lbl_80347290, lbl_805FE558);
    (*(Fn_PP*)(vt + 0x24))(d, arg0 + 0x3020);
    v[0] = v[0] + d[0];
    v[1] = v[1] + d[1];
    v[2] = v[2] + d[2];

    obj = *(u8**)(lbl_803C4298 + 0x4);
    (*(Fn_OPP*)(*(u8**)obj + 0x2C))(obj, v, d);
    (*(Fn_OP*)(*(u8**)obj + 0x1C))(obj, v);
    obj2 = *(u8**)(lbl_803C4298 + 0x8);
    (*(Fn_Of*)(*(u8**)obj2 + 0x98))(obj2, lbl_805FE558);

    fn_800CFDCC(arg0 + 0x90, farg0);
    st = (u8*)fn_800CF71C(); /* r3 (arg0) is unused by the callee */

    q[0] = lbl_805FE558;
    q[1] = lbl_805FE558;
    q[2] = lbl_805FE558;
    q[3] = lbl_805FE598;
    ax[0] = lbl_805FE558;
    ax[1] = lbl_805FE598;
    ax[2] = lbl_805FE558;
    fn_80146180(q, (const Vec3*)ax, *(f32*)(arg0 + 0xA0));

    *(f32*)(st + 0x58) = v[0];
    *(f32*)(st + 0x5C) = v[1];
    *(f32*)(st + 0x60) = v[2];
    *(f32*)(st + 0x64) = q[0];
    *(f32*)(st + 0x68) = q[1];
    *(f32*)(st + 0x6C) = q[2];
    *(f32*)(st + 0x70) = q[3];

    f32 acc = *(f32*)(arg0 + 0x8C) + farg0;
    *(f32*)(arg0 + 0x8C) = acc;
    if (acc < lbl_805FE5A8) {
        GameFlowSignal((s32)0x88C6E941u, 0, 0);
    }
}

// 0x8011EAC4
// Fills the quest-icon texture slot (+0x30) from the current quest record and counts the
// completed quests in the 3-entry list (+0x2C); +0x2B is the "has quest" flag.
void fn_8011EAC4(u8* arg0) {
    char name[64];
    u8* save;

    *(u8*)(arg0 + 0x2B) = 0;
    *(s32*)(arg0 + 0x2C) = 0;
    save = *(u8**)lbl_805FBBE0;
    fn_80222704(save, 1, 0);
    if (SaveGetQuestLevel(save, 0x16U) > 0) {
        *(u8*)(arg0 + 0x2B) = 1;
    }
    for (s32 i = 0; i < 3; i++) {
        if (*(u8*)(arg0 + 0x2B) != 0) {
            if (*(s32*)(fn_80222704(save, i + 1, 0) + 0x8) != 0) {
                *(s32*)(arg0 + 0x2C) = *(s32*)(arg0 + 0x2C) + 1;
            }
        } else if (*(s32*)(fn_80222704(save, 0, i) + 0x8) != 0) {
            *(s32*)(arg0 + 0x2C) = *(s32*)(arg0 + 0x2C) + 1;
        }
    }
    *(void**)(arg0 + 0x30) = NULL;
    if (fn_80222994(save) != -1) {
        s32 idx = fn_80222994(save);
        u8* table = *(u8**)(*(u8**)(gGameDb + 0xC) + 0x8508);
        u8* rec = table + idx * 0x1C;
        s8 icon = *(s8*)(rec + 0x18);
        if (icon != 0 && icon != -1) {
            SysSprintf(name, "%squest_icon_%03d", lbl_805F8F90, (s32)icon);
            u8* tex = fn_80109330(name);
            if (tex == NULL) {
                SysSprintf(name, "%squest_icon_placeholder", lbl_805F8F90);
                tex = fn_80109330(name);
            }
            *(u8**)(arg0 + 0x30) = tex;
        }
    }
}

// 0x8011FC60
// Releases the per-entry texture and handle slots of the (count at +0x9260) 0x3C-byte entries.
void fn_8011FC60(u8* arg0) {
    s32 count = *(s32*)(arg0 + 0x9260);
    u8* p = arg0;
    for (s32 i = 0; i < count; i++, p += 0x3C) {
        u8* t = *(u8**)(p + 0x1C);
        if (t != NULL) {
            DraftTexUnref(t);
            *(u8**)(p + 0x1C) = NULL;
        }
        void* h = *(void**)(p + 0x20);
        if (h != NULL) {
            if (fn_80109A0C(h) == 0) {
                fn_801098DC(h);
            } else {
                DraftTexUnref((u8*)h);
            }
            *(void**)(p + 0x20) = NULL;
        }
        u8* t2 = *(u8**)(p + 0x38);
        if (t2 != NULL) {
            DraftTexUnref(t2);
            *(u8**)(p + 0x38) = NULL;
        }
        void* h2 = *(void**)(p + 0x3C);
        if (h2 != NULL) {
            if (fn_80109A0C(h2) == 0) {
                fn_801098DC(h2);
            }
            *(void**)(p + 0x3C) = NULL;
        }
        u8* t3 = *(u8**)(p + 0x44);
        if (t3 != NULL) {
            DraftTexUnref(t3);
            *(u8**)(p + 0x44) = NULL;
        }
        void* h3 = *(void**)(p + 0x48);
        if (h3 != NULL) {
            if (fn_80109A0C(h3) == 0) {
                fn_801098DC(h3);
            }
            *(void**)(p + 0x48) = NULL;
        }
    }
}

// 0x8019F810
// Returns 1 when the transformed point (sum of the matrix at +0xC0..+0xE8) lies in the
// x range [+0x1B0, +0x1D4), z >= +0x1B4 and z < 1.0 (lbl_805FF95C); arg1 == 0 also needs flag bit 0.
s32 fn_8019F810(u8* arg0, s32 arg1, f64 farg0, f64 farg1) {
    if (arg1 == 0 && !(*(s32*)(arg0 + 0x64) & 1)) {
        return 0;
    }
    f32 lo = *(f32*)(arg0 + 0x1B0);
    f32 hi = *(f32*)(arg0 + 0x1D4);
    if (lo == hi) {
        return 0;
    }
    f32 x = (f32)farg0;
    f32 y = (f32)farg1;
    f32 w = lbl_805FF95C;
    f32 px = *(f32*)(arg0 + 0xE4) + ((*(f32*)(arg0 + 0xD8) * w) + ((*(f32*)(arg0 + 0xC0) * x) + (*(f32*)(arg0 + 0xCC) * y)));
    f32 pz = *(f32*)(arg0 + 0xE8) + ((*(f32*)(arg0 + 0xDC) * w) + ((*(f32*)(arg0 + 0xC4) * x) + (*(f32*)(arg0 + 0xD0) * y)));
    if (px < lo || px >= hi) {
        return 0;
    }
    f32 lo2 = *(f32*)(arg0 + 0x1B4);
    if (pz < lo2) {
        return 0;
    }
    if (pz >= w) {
        return 0;
    }
    return 1;
}

// 0x801AA028
// Tests whether the point (farg0, farg1) hits the attached object at +0x128 (flag 0x02000000)
// or any object in the +0x120 chain (flag 2, +0x8C > 0) through its slot 0x38.
s32 fn_801AA028(u8* arg0, f64 farg0, f64 farg1) {
    u8* t = *(u8**)(arg0 + 0x128);
    u8* cur = *(u8**)(arg0 + 0x120);
    if (t != NULL && (*(s32*)(t + 0x64) & 0x02000000)) {
        f32 x = (f32)farg0;
        f32 y = (f32)farg1;
        f32 X = *(f32*)(arg0 + 0xE4) + ((*(f32*)(arg0 + 0xD8) * lbl_805FFA74) + ((*(f32*)(arg0 + 0xC0) * x) + (*(f32*)(arg0 + 0xCC) * y)));
        f32 Y = *(f32*)(arg0 + 0xE8) + ((*(f32*)(arg0 + 0xDC) * lbl_805FFA74) + ((*(f32*)(arg0 + 0xC4) * x) + (*(f32*)(arg0 + 0xD0) * y)));
        if (X >= lbl_805FFA74) return 0;
        if (X >= lbl_805FFA70) return 0;
        if (Y >= lbl_805FFA74) return 0;
        if (Y <= lbl_805FFA70) return 0;
        return 1;
    }
    while (cur != NULL) {
        if ((*(s32*)(cur + 0x64) & 2) && (*(f32*)(cur + 0x8C) > lbl_805FFA74)) {
            u8* vt = *(u8**)(cur + 0x138);
            if ((*(Fn_Obj38*)(vt + 0x38))(cur, 0, farg0, farg1) != 0) {
                return 1;
            }
        }
        cur = *(u8**)(cur + 0x124);
    }
    return 0;
}

// 0x801AD050
// Per-frame update of the object's position/velocity block (+0x14..+0x28) and its attached
// rigid-body transform (+0x10); the tag word at +0x2C selects which correction runs.
void fn_801AD050(u8* arg0, s32 arg1) {
    u8 outBuf[0x30];
    f32 nrm[3];
    u8* r4;

    *(f32*)(arg0 + 0x20) = *(f32*)(arg0 + 0x14);
    *(f32*)(arg0 + 0x24) = *(f32*)(arg0 + 0x18);
    *(f32*)(arg0 + 0x28) = *(f32*)(arg0 + 0x1C);
    u8* obj = *(u8**)(arg0 + 0x8);
    fn_801ACD08(arg0, obj + 0xC, outBuf, *(f32*)(arg0 + 0x18), *(f32*)(arg0 + 0x14));
    *(f32*)(arg0 + 0x14) = *(f32*)(outBuf + 0x24);
    r4 = *(u8**)(arg0 + 0x10);
    *(f32*)(arg0 + 0x18) = *(f32*)(outBuf + 0x28);
    *(f32*)(arg0 + 0x1C) = *(f32*)(outBuf + 0x2C);
    *(f32*)(arg0 + 0x3C) = *(f32*)(obj + 0x24) * (lbl_805FFAC0 * (*(f32*)(r4 + 0x8) + *(f32*)(r4 + 0xC) + *(f32*)(r4 + 0x10)));

    if (*(s32*)(arg0 + 0x2C) == 0x7F20C9C6) {
        *(f32*)(r4 + 0x18) = *(f32*)(arg0 + 0x14);
        *(f32*)(r4 + 0x1C) = *(f32*)(arg0 + 0x18);
        *(f32*)(r4 + 0x20) = *(f32*)(arg0 + 0x1C);
        f32 k = *(f32*)(arg0 + 0x3C);
        u8* b = *(u8**)(*(u8**)(arg0 + 0x10) + 0x4);
        f32 bv[3];
        bv[0] = *(f32*)(b + 0x0);
        bv[1] = *(f32*)(b + 0x4);
        bv[2] = *(f32*)(b + 0x8);
        fn_801455E0(nrm, b + 0xC);
        f32 px = *(f32*)(arg0 + 0x14);
        f32 py = *(f32*)(arg0 + 0x18);
        f32 pz = *(f32*)(arg0 + 0x1C);
        f32 dotP = (nrm[0] * px) + ((nrm[1] * py) + (nrm[2] * pz));
        f32 dotB = (nrm[0] * bv[0]) + ((nrm[1] * bv[1]) + (nrm[2] * bv[2]));
        f32 lam = -((dotP - dotB) - k);
        *(f32*)(arg0 + 0x14) = px + nrm[0] * lam;
        *(f32*)(arg0 + 0x18) = py + nrm[1] * lam;
        *(f32*)(arg0 + 0x1C) = pz + nrm[2] * lam;
    }
    if (*(s32*)(arg0 + 0x2C) == 0x0D5F8BC9) {
        u8* r3 = *(u8**)(arg0 + 0x10);
        *(f32*)(r3 + 0x18) = *(f32*)(arg0 + 0x14);
        *(f32*)(r3 + 0x1C) = *(f32*)(arg0 + 0x18);
        *(f32*)(r3 + 0x20) = *(f32*)(arg0 + 0x1C);
    }
    if (arg1 != 0) {
        *(f32*)(arg0 + 0x20) = *(f32*)(arg0 + 0x14);
        *(f32*)(arg0 + 0x24) = *(f32*)(arg0 + 0x18);
        *(f32*)(arg0 + 0x28) = *(f32*)(arg0 + 0x1C);
    }
}

// 0x801F5830
// Clears the global state blocks at lbl_804DD140 and registers their destructors.
void fn_801F5830(void) {
    u8* b = lbl_804DD140;

    fn_8032F1E8(b, (void (*)(void))fn_801F59A4, 0, 4, 0xA);

    *(s32*)(b + 0x28) = -1;
    *(s32*)(b + 0x30) = 0;
    *(u8*)(b + 0x34) = 0;
    *(s32*)(b + 0x38) = 0;
    *(u8*)(b + 0x40 + 0x94) = 0;
    *(s32*)(b + 0x40 + 0x98) = 0;
    for (u8* p = b + 0xE0; p < b + 0x400; p += 0xA0) {
        *(u8*)(p + 0x94) = 0;
        *(s32*)(p + 0x98) = 0;
    }

    u8* r6 = b + 0x40;
    *(s32*)(r6 + 0x3C4) = -1;
    *(s32*)(r6 + 0x3CC) = 0;
    *(u8*)(r6 + 0x3D0) = 0;
    *(s32*)(r6 + 0x3D4) = 0;
    *(s32*)(r6 + 0x3C0) = 0;

    u8* t1 = b + 0x428;
    *(void**)(t1 + 0x14) = t1 + 0x18;
    *(s32*)(t1 + 0x10) = 0x3C;
    *(s32*)(t1 + 0xC) = -1;
    *(s32*)(t1 + 0x8) = -1;
    *(s32*)(t1 + 0x4) = 0;
    __register_global_object(t1, (void (*)(void*))fn_801F59B0, b + 0x418);

    u8* t2 = b + 0xF450;
    *(s32*)(t2 + 0x10) = 0x3C;
    *(void**)(t2 + 0x14) = t2 + 0x18;
    *(s32*)(t2 + 0xC) = -1;
    *(s32*)(t2 + 0x8) = -1;
    *(s32*)(t2 + 0x4) = 0;
    __register_global_object(t2, (void (*)(void*))fn_801F59B0, b + 0xF440);

    *(s32*)(b + 0x1E468) = 0;
    *(s32*)(b + 0x1E46C) = 0;
    *(s32*)(b + 0x1E478) = 0;
    *(s32*)(b + 0x1E484) = 0;
    *(s32*)(b + 0x1E488) = 0;
    *(s32*)(b + 0x1E494) = 0;
    *(s32*)(b + 0x1E4A0) = -1;
    *(s32*)(b + 0x1E4A8) = 0;
    *(u8*)(b + 0x1E4AC) = 0;
    *(s32*)(b + 0x1E4B0) = 0;
    lbl_805FB960 = -1;
    *(u8*)(b + 0x1E4B8 + 0x94) = 0;
    *(s32*)(b + 0x1E4B8 + 0x98) = 0;
}

// 0x801F6A10
// Normalises the vector at arg1 and, if it is nearly perpendicular to the axis at +0xA8..+0xB0,
// stores the cross product of the (rotated) axis with it at +0xB4..+0xBC.
void fn_801F6A10(u8* arg0, u8* arg1) {
    f32 vx = *(f32*)(arg1 + 0x0);
    f32 vy = *(f32*)(arg1 + 0x4);
    f32 vz = *(f32*)(arg1 + 0x8);
    f32 len2 = (vz * vz) + ((vx * vx) + (vy * vy));
    f32 n[3];
    if (lbl_80600130 == len2) {
        n[0] = lbl_80600130;
        n[1] = lbl_80600130;
        n[2] = lbl_80600130;
    } else {
        f32 r = 1.0f / sqrtf(len2);
        n[0] = vx * r;
        n[1] = vy * r;
        n[2] = vz * r;
    }
    f32 dot = (n[2] * *(f32*)(arg0 + 0xB0)) + ((n[0] * *(f32*)(arg0 + 0xA8)) + (n[1] * *(f32*)(arg0 + 0xAC)));
    if (dot > lbl_80600150 && dot < lbl_80600154) {
        fn_801F7240(arg0, n);
        f32 ac = *(f32*)(arg0 + 0xAC);
        f32 a98 = *(f32*)(arg0 + 0x98);
        f32 a90 = *(f32*)(arg0 + 0x90);
        f32 b0 = *(f32*)(arg0 + 0xB0);
        f32 a94 = *(f32*)(arg0 + 0x94);
        f32 a8 = *(f32*)(arg0 + 0xA8);
        *(f32*)(arg0 + 0xB4) = (ac * a98) - (b0 * a94);
        *(f32*)(arg0 + 0xB8) = (b0 * a90) - (a8 * a98);
        *(f32*)(arg0 + 0xBC) = (a8 * a94) - (ac * a90);
    }
}

// 0x801F6EC4
// Projects the axis at +0xA8..+0xB0 onto the surface normal from fn_801F72F0 and, when it is
// not too steep, stores the resulting angle at +0xA0 (flag at +0xA4).
void fn_801F6EC4(u8* arg0) {
    f32 v[3];
    u8 flag = 0;
    v[0] = lbl_80600130;
    v[1] = lbl_80600130;
    v[2] = lbl_80600130;
    if (fn_801F72F0(arg0, v, (s8*)&flag) != 0) {
        if (flag != 0) {
            *(u8*)(arg0 + 0xA4) = 0;
            return;
        }
        f32 a8 = *(f32*)(arg0 + 0xA8);
        f32 ac = *(f32*)(arg0 + 0xAC);
        f32 b0 = *(f32*)(arg0 + 0xB0);
        f32 d = (b0 * v[2]) + ((a8 * v[0]) + (ac * v[1]));
        if (d > lbl_80600158) {
            *(u8*)(arg0 + 0xA4) = 0;
            return;
        }
        f32 w0 = v[0] - (a8 * d);
        f32 w1 = v[1] - (ac * d);
        f32 w2 = v[2] - (b0 * d);
        f32 l2 = (w2 * w2) + ((w0 * w0) + (w1 * w1));
        if (lbl_80600130 == l2) {
            v[0] = lbl_80600130;
            v[1] = lbl_80600130;
            v[2] = lbl_80600130;
        } else {
            f32 r = 1.0f / sqrtf(l2);
            v[0] = w0 * r;
            v[1] = w1 * r;
            v[2] = w2 * r;
        }
        *(u8*)(arg0 + 0xA4) = 1;
        f32 y = (v[2] * *(f32*)(arg0 + 0xBC)) + ((v[0] * *(f32*)(arg0 + 0xB4)) + (v[1] * *(f32*)(arg0 + 0xB8)));
        f32 x = (v[2] * *(f32*)(arg0 + 0x98)) + ((v[0] * *(f32*)(arg0 + 0x90)) + (v[1] * *(f32*)(arg0 + 0x94)));
        f32 ang = fn_8033F1FC(y, x);
        *(f32*)(arg0 + 0xA0) = lbl_80600160 * (lbl_8060015C * ang);
    }
}

// 0x801F72F0
// Walks the segment list of the object at +0x4 (count at +0x14), sums the offsets of entries
// (ring buffer starting at +0x10) into arg1, averages them, and flags arg2 when the sum or any
// entry's weight exceeds 0.5. Returns 1 when there are segments.
s32 fn_801F72F0(u8* arg0, f32* arg1, s8* arg2) {
    u8* obj = fn_801483BC(*(s32*)(arg0 + 0x4));
    *arg2 = 0;
    s32 cnt = *(s32*)(obj + 0x14);
    if (cnt <= 0) {
        return 0;
    }
    for (s32 k = cnt - 1; k >= 0; k--) {
        u8* base = *(u8**)(obj + 0x4);
        s32 idx = (*(s32*)(obj + 0x10) + k) % *(s32*)(obj + 0xC);
        u8* e = base + idx * 0x84;
        f32 ax;
        f32 ay;
        f32 az;
        f32 wt;
        if (*(s32*)(arg0 + 0x9C) == 0) {
            ax = *(f32*)(e + 0xC);
            ay = *(f32*)(e + 0x10);
            az = *(f32*)(e + 0x14);
            wt = *(f32*)(e + 0x1C);
        } else {
            ax = *(f32*)(e + 0x68);
            ay = *(f32*)(e + 0x6C);
            az = *(f32*)(e + 0x70);
            wt = *(f32*)(e + 0x78);
        }
        if (wt > lbl_80600188) {
            *arg2 = 1;
        }
        arg1[0] = arg1[0] + ax;
        arg1[1] = arg1[1] + ay;
        arg1[2] = arg1[2] + az;
    }
    f32 fc = (f32)cnt;
    arg1[0] = arg1[0] / fc;
    arg1[1] = arg1[1] / fc;
    arg1[2] = arg1[2] / fc;
    f32 l2 = (arg1[0] * arg1[0]) + ((arg1[1] * arg1[1]) + (arg1[2] * arg1[2]));
    if (l2 > lbl_8060018C) {
        *arg2 = 1;
    }
    return 1;
}

}  // extern "C"
