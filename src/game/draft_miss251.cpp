// Misc. game helpers: bug splat / goo effects, world pick tests, small struct copies.
#include "types.h"
#include <math.h>

struct Locator;

extern "C" {
void*    ScreenSetFindElement(void* set, u32 hash);       // 0x8018D2B0
void*    ScreenSetsCurrent(void* sets);                   // 0x8018D1E4
u32      StringHash(const char* s);                       // 0x801EB704
Locator* WorldFindLocator(void* world, u32 hash);         // 0x8012AA50
s32      WorldGetEntityZone(void* world, void* entity);
u8*      fn_80138864(u8* p, s32 idx);
void*    UIAnim_FindByName(void* set, u32 hash);                // 0x80198F1C
s32      fn_8019BBA4(u8* arg0, u8* arg1, s32 arg2);
s32      fn_8019BC80(u8* arg0, u32 arg1);
void     fn_801A8654(u8* obj, s32 a1);                    // 0x801A8654
u32      fn_801B5420(u8* a, u8* b, s32 c, f32* v0, f32* v1, s32 d);  // 0x801B5420
s32      fn_801B5510(u8* a, u8* b, s32 c, f32* v, s32 d); // 0x801B5510
void     fn_8002BE88(u8* obj, f32* v, u8* el);            // 0x8002BE88
s32      fn_801D3A60(s32 zone);
u32      fn_801EC880(u8* p, u32 hash);
BOOL     fn_801F5220(void);
extern s32 gScreenHeight;
extern s32 gScreenWidth;
extern u8  gWorldState[];
}
extern "C++" void* gScreenSets;  // 0x805FB654

typedef s32 (*VFn1)(void* obj, s32 a);

// Calls the object's slot at +0x2C of the vtable stored at +0x138.
#define EL_VCALL2C(el, arg) ((*(VFn1*)(*(u8**)((u8*)(el) + 0x138) + 0x2C))((el), (arg)))

extern "C" {

// 0x8002AA8C
// True when the two objects (center +0x2C/+0x30, radius +0x38) overlap on both axes.
s32 fn_8002AA8C(u8* arg0, u8* arg1) {
    f32 r = *(f32*)(arg0 + 0x38) + *(f32*)(arg1 + 0x38);
    if (fabsf(*(f32*)(arg1 + 0x2C) - *(f32*)(arg0 + 0x2C)) < r &&
        fabsf(*(f32*)(arg1 + 0x30) - *(f32*)(arg0 + 0x30)) < r) {
        return 1;
    }
    return 0;
}

// 0x8002A9E0
// Plays effect 0x1A on the element at +0x118 and moves it to the position of the element at +0x104 (z = 20).
void fn_8002A9E0(u8* arg0) {
    u8* el = *(u8**)(arg0 + 0x118);
    EL_VCALL2C(el, 0x1A);
    u8* src = *(u8**)(arg0 + 0x104);
    f32 x = *(f32*)(src + 0x68);
    f32 y = *(f32*)(src + 0x6C);
    el = *(u8**)(arg0 + 0x118);
    *(f32*)(el + 0x68) = x;
    *(f32*)(el + 0x6C) = y;
    *(f32*)(el + 0x70) = 20.0f;
    fn_801A8654(*(u8**)(arg0 + 0x118), 1);
}

// 0x8002BDD4
// Places the "bug_smash_pfx_dust" element at the smash position and starts it.
void fn_8002BDD4(u8* arg0) {
    void* sets = gScreenSets;
    u32 h = StringHash("bug_smash_pfx_dust");
    u8* el = (u8*)ScreenSetFindElement(ScreenSetsCurrent(sets), h);
    EL_VCALL2C(el, 0x1A);
    u8* src = *(u8**)arg0;
    f32 x = *(f32*)(src + 0x68);
    f32 y = *(f32*)(src + 0x6C);
    *(f32*)(el + 0x68) = x;
    *(f32*)(el + 0x6C) = y;
    *(f32*)(el + 0x70) = 0.05f;
    fn_801A8654(el, 1);
}

// 0x80132F04
// Copies three 0x30-byte records (looked up per zone) into the slots at +0x20.
void fn_80132F04(u8* arg0) {
    s32 i;
    u8* pp = arg0;
    u8* dst = arg0;
    for (i = 0; i < 3; i++) {
        u8* z = *(u8**)(pp + 0x14);
        if (z != NULL) {
            u8* s = fn_80138864(arg0, fn_801D3A60(*(s32*)(z + 0x18)));
            *(s32*)(dst + 0x20) = *(s32*)(s + 0x0);
            *(s32*)(dst + 0x24) = *(s32*)(s + 0x4);
            *(s32*)(dst + 0x28) = *(s32*)(s + 0x8);
            *(s32*)(dst + 0x2C) = *(s32*)(s + 0xC);
            *(s32*)(dst + 0x30) = *(s32*)(s + 0x10);
            *(f32*)(dst + 0x34) = *(f32*)(s + 0x14);
            *(u16*)(dst + 0x38) = *(u16*)(s + 0x18);
            *(s32*)(dst + 0x3C) = *(s32*)(s + 0x1C);
            *(s32*)(dst + 0x40) = *(s32*)(s + 0x20);
            *(s32*)(dst + 0x44) = *(s32*)(s + 0x24);
            *(s32*)(dst + 0x48) = *(s32*)(s + 0x28);
            *(s32*)(dst + 0x4C) = *(s32*)(s + 0x2C);
        }
        dst += 0x30;
        pp += 4;
    }
}

// 0x800B8624
// Sets the target at +0x28C and tests it from the entity's position relative to the current record's origin;
// on a hit clears bit 0 of the entity flags.
s32 fn_800B8624(u8* arg0, u8* arg1) {
    f32 a[3];
    f32 b[3];
    *(u8**)(arg0 + 0x28C) = arg1;
    u8* ent = *(u8**)(arg0 + 0x4);
    u8* rec = *(u8**)(arg0 + 0x98) + *(s32*)(arg0 + 0x290) * 0xC8;
    f32 dx = *(f32*)(ent + 0x0) - *(f32*)(rec + 0x5C);
    f32 dy = *(f32*)(ent + 0x4) - *(f32*)(rec + 0x60);
    f32 dz = *(f32*)(ent + 0x8) - *(f32*)(rec + 0x64);
    a[0] = 0.0f; a[1] = 0.0f; a[2] = 0.0f;
    b[0] = dx; b[1] = dy; b[2] = dz;
    if (fn_801B5420(*(u8**)(arg0 + 0x28C), arg0, *(s32*)(arg0 + 0x298), a, b, 1) == 1) {
        u8* e = *(u8**)(arg0 + 0x4);
        *(u32*)(e + 0x20) &= 0xFFFFFFFE;
        return 1;
    }
    return 0;
}

// 0x8002A8D8
// Shows the "bug_goo" element above the owner's element (+0x104, z + 2) and starts "bug_goo_anim" on it.
void fn_8002A8D8(u8* arg0) {
    void* sets = gScreenSets;
    u32 h0 = StringHash("bug_goo_anim");
    u8* anim = (u8*)UIAnim_FindByName(ScreenSetsCurrent(sets), h0);
    sets = gScreenSets;
    u32 h1 = StringHash("bug_goo");
    u8* el = (u8*)ScreenSetFindElement(ScreenSetsCurrent(sets), h1);
    if (*(s32*)(el + 0x10) != 0) {
        fn_8019BC80(el, 1);
    }
    EL_VCALL2C(el, 0x1A);
    u8* src = *(u8**)(arg0 + 0x104);
    f32 z = (f32)(2.0 + (f64)*(f32*)(src + 0x70));
    f32 x = *(f32*)(src + 0x68);
    f32 y = *(f32*)(src + 0x6C);
    *(f32*)(el + 0x68) = x;
    *(f32*)(el + 0x6C) = y;
    *(f32*)(el + 0x70) = z;
    fn_8019BBA4(el, anim, 10);
}

// 0x80130A38
// Picks the current target at +0xC of the object at +0x0: the one at +0x8 in a flag mode, else the first of
// the two candidates (+0x10/+0x14) in the zone at +0x1A8, falling back to +0x8.
void fn_80130A38(u8* arg0) {
    if (fn_801F5220() == 0) {
        u8* o = *(u8**)arg0;
        *(u8**)(o + 0xC) = NULL;
        if (*(u8*)(arg0 + 0xF4) != 0) {
            o = *(u8**)arg0;
            *(u8**)(o + 0xC) = *(u8**)(o + 0x8);
            return;
        }
        s32 zone = *(s32*)(arg0 + 0x1A8);
        if (zone > 0) {
            u8* o2 = *(u8**)arg0;
            u8* cur = *(u8**)(o2 + 0x8);
            u8* inner;
            if (cur != NULL && (inner = *(u8**)(cur + 0x4)) != NULL && *(u16*)(inner + 0x30) == zone) {
                *(u8**)(o2 + 0xC) = cur;
            } else {
                s32 i = 0;
                s32 off = 0;
                do {
                    u8* c = *(u8**)(*(u8**)arg0 + off + 0x10);
                    if (c != NULL && WorldGetEntityZone(arg0, c) == *(s32*)(arg0 + 0x1A8)) {
                        u8* oo = *(u8**)arg0;
                        *(u8**)(oo + 0xC) = *(u8**)(oo + i * 4 + 0x10);
                        break;
                    }
                    i++;
                    off += 4;
                } while (i < 2);
            }
            u8* o3 = *(u8**)arg0;
            if (*(u8**)(o3 + 0xC) == NULL) {
                *(u8**)(o3 + 0xC) = *(u8**)(o3 + 0x8);
            }
        }
    }
}

// 0x800B8708
// Tests the record named by hash arg2 (position = its translation row at +0x2C..0x34) against arg1; on a hit
// clears bit 0 of arg1's entity flags.
s32 fn_800B8708(u8* arg0, u8* arg1, u32 arg2) {
    f32 v[3];
    u32 idx = fn_801EC880(arg0 + 0x94, arg2);
    u8* rec = *(u8**)(arg0 + 0x98) + idx * 0xC8;
    f32 z = 0.0f;
    v[0] = *(f32*)(rec + 0x2C) + (*(f32*)(rec + 0x20) * z + (*(f32*)(rec + 0x8) * z + *(f32*)(rec + 0x14) * z));
    v[1] = *(f32*)(rec + 0x30) + (*(f32*)(rec + 0x24) * z + (*(f32*)(rec + 0xC) * z + *(f32*)(rec + 0x18) * z));
    v[2] = *(f32*)(rec + 0x34) + (*(f32*)(rec + 0x28) * z + (*(f32*)(rec + 0x10) * z + *(f32*)(rec + 0x1C) * z));
    if (fn_801B5510(arg0, arg1, *(s32*)(rec + 0x4), v, 1) != 0) {
        u8* e = *(u8**)(arg1 + 0x4);
        *(u32*)(e + 0x20) &= 0xFFFFFFFE;
        return 1;
    }
    return 0;
}

// 0x8002A028
// Moves every child element (+0x104.., count from vtable slot +0x14) and the one at +0x114 to (x, y),
// keeping z, and triggers effect 0x18 on each.
void fn_8002A028(u8* arg0, f32 x, f32 y) {
    s32 i = 0;
    u8* pp = arg0;
    while (i < (*(s32 (**)(void*))(*(u8**)arg0 + 0x14))(arg0)) {
        u8* el = *(u8**)(pp + 0x104);
        f32 z = *(f32*)(el + 0x70);
        *(f32*)(el + 0x68) = x;
        *(f32*)(el + 0x6C) = y;
        *(f32*)(el + 0x70) = z;
        el = *(u8**)(pp + 0x104);
        EL_VCALL2C(el, 0x18);
        pp += 4;
        i++;
    }
    u8* el = *(u8**)(arg0 + 0x114);
    f32 z = *(f32*)(el + 0x70);
    *(f32*)(el + 0x68) = x;
    *(f32*)(el + 0x6C) = y;
    *(f32*)(el + 0x70) = z;
    el = *(u8**)(arg0 + 0x114);
    EL_VCALL2C(el, 0x18);
}

// 0x800B8D5C
// Resets the follow state and caches the distance between two world locators at +0x274.
void fn_800B8D5C(u8* arg0) {
    *(f32*)(arg0 + 0x28C) = 0.0f;
    *(f32*)(arg0 + 0x290) = 1.0f;
    *(f32*)(arg0 + 0x294) = 0.0f;
    *(f32*)(arg0 + 0x298) = 1.0f;
    *(u8*)(arg0 + 0x2A8) = 1;
    *(u8*)(arg0 + 0x2A9) = 1;
    *(f32*)(arg0 + 0x2AC) = 1.0f;
    *(Locator**)(arg0 + 0x270) = WorldFindLocator(gWorldState, 0x372667E6u);
    u8* b = (u8*)WorldFindLocator(gWorldState, 0xB0381BDCu);
    u8* a = *(u8**)(arg0 + 0x270);
    f32 dx = *(f32*)(a + 0x8) - *(f32*)(b + 0x8);
    f32 dy = *(f32*)(a + 0xC) - *(f32*)(b + 0xC);
    f32 dz = *(f32*)(a + 0x10) - *(f32*)(b + 0x10);
    f32 sq = dz * dz + (dx * dx + dy * dy);
    f32 len = 0.0f;
    if (!(sq <= 0.0f)) len = sqrtf(sq);
    *(f32*)(arg0 + 0x274) = len;
}

// 0x8002BC20
// Projects the point at +0x568 (scaled by +0x588) onto the screen, hands it with the current element to
// fn_8002BE88, clears +0x8 and fires effect 2 on the object at +0x0.
s32 fn_8002BC20(u8* arg0) {
    f32 px = *(f32*)(arg0 + 0x568);
    f32 py = *(f32*)(arg0 + 0x56C);
    f32 pz = *(f32*)(arg0 + 0x570);
    f32 w = 0.5f * (f32)gScreenWidth;
    f32 h = 0.5f * (f32)gScreenHeight;
    f32 sx = *(f32*)(arg0 + 0x588) * px;
    f32 sy = *(f32*)(arg0 + 0x58C) * py;
    u8* el = *(u8**)(arg0 + *(s32*)(arg0 + 0x8) * 4);
    f32 ez = *(f32*)(el + 0x70);
    f32 X = w + w * sx;
    f32 Y = h + h * sy;
    f32 v[12];
    v[0] = X; v[1] = Y; v[2] = ez;
    v[3] = X; v[4] = Y; v[5] = 0.0f;
    v[6] = px; v[7] = py; v[8] = pz;
    v[9] = X; v[10] = Y; v[11] = 0.0f;
    fn_8002BE88(arg0, v, el);
    *(s32*)(arg0 + 0x8) = 0;
    u8* o = *(u8**)arg0;
    return EL_VCALL2C(o, 2);
}

}  // extern "C"
