// Hand-written functions in 0x8003xxxx - 0x8008xxxx (jump tables, float-heavy, leftover-register args).
#include <math.h>

#include "types.h"

extern "C" {
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
u32   RandomU32(void);                                       // 0x80143460
void* ScreenSetFindElement(void* set, u32 hash);             // 0x80198F18
void* ScreenSetsCurrent(void* sets);                         // 0x8018D1E4
u32   StringHash(const char* s);                             // 0x801EB704
int   SysSprintf(char* buf, const char* fmt, ...);           // 0x80142EF8
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);   // 0x80209E70
void* fn_801D40E0(s32 n);                                    // 0x801D40E0
void* fn_80209814(void* v);                                  // 0x80209814
void* memset(void* p, int c, unsigned long n);
void  operator_delete(void* p);                              // 0x801D426C

void  fn_800325D8(u8* arg0, f32 dt);                         // 0x800325D8
void  fn_80031F84(s32 arg0);                                 // 0x80031F84
void  fn_80032A98(u8* arg0);                                 // 0x80032A98
void  fn_80032CFC(u8* arg0, u8* arg1);                       // 0x80032CFC
void* fn_80040A08(u8* arg0, s32 arg1);                       // 0x80040A08
u8*   fn_8007F900(u8* mgr, s32 type);                        // 0x8007F900
void  fn_800822A8(u8* arg0, u8* arg1);                       // 0x800822A8
u32   fn_8015B82C(u8* hdr, s32 x, s32 y);                    // 0x8015B82C
void* fn_80198F1C(void* set, u32 hash);                      // 0x80198F1C
s32   fn_8019BBA4(u8* arg0, u8* arg1, s32 arg2);             // 0x8019BBA4
s32   fn_8019BC80(u8* arg0, u32 arg1);                       // 0x8019BC80
ScriptValue fn_801A1208(u8* el);                             // 0x801A1208
f64   fn_8033F20C(f64 x, f64 y);                             // 0x8033F20C (pow)

extern const char lbl_80357D50[];   // 0x80357D50 "gem_falling"
extern const char lbl_80357F38[];   // 0x80357F38 "Image_%d"
extern u8   lbl_803584A0[];         // 0x803584A0 (vtable)
extern s32  lbl_803C4E28[];         // 0x803C4E28 (combat part zone-loaded flags)
extern void* lbl_805FB010;          // 0x805FB010
extern u8*  lbl_805FB014;           // 0x805FB014 (battle camera object)
extern u8*  lbl_805FB018;           // 0x805FB018
extern void* lbl_805FB01C;          // 0x805FB01C
extern void* lbl_805FB020;          // 0x805FB020
extern u8   lbl_805FB024;           // 0x805FB024
extern u8   lbl_805FB025;           // 0x805FB025
extern s32  lbl_805FB028;           // 0x805FB028
extern s32  lbl_805FB02C;           // 0x805FB02C
extern u8   lbl_805FB030;           // 0x805FB030
extern s32  lbl_805FB034;           // 0x805FB034
extern s32  lbl_805FB038;           // 0x805FB038 (number of combat part zones)
extern void* lbl_805FBBAC;          // 0x805FBBAC (voice SFX handle)
extern u8   lbl_805FB1C0;           // 0x805FB1C0
extern u8   lbl_805FB1C1;           // 0x805FB1C1
}

extern "C++" u8*   gBattle;         // 0x805FBBB8
extern "C++" void* gEventMgr;       // 0x805FB6C8
extern "C++" void* gScreenSets;     // 0x805FB654
extern "C++" s32   gScreenWidth;    // 0x805F9388
extern "C++" s32   gScreenHeight;   // 0x805F938C

typedef void (*SlotFn)(void* self, s32 a);   // virtual slot taking one int argument
typedef u8* (*GetFn)(void* self);            // virtual slot returning an object pointer

extern "C" {

// 0x800328A0
// Per-frame update: projects the world position (+0xDC..+0xE4) to screen space and, if it moved
// 5 or more pixels from the cached screen position (+4/+8), pushes it to fn_80032CFC; then runs
// the two sub-updates and sends slot 0x2C (arg 0x1A) to the object at +0.
void fn_800328A0(u8* p, f32 dt) {
    f32 sp68[3];
    f32 halfH = 0.5f * (f32)gScreenHeight;
    f32 halfW = 0.5f * (f32)gScreenWidth;
    f32 a = *(f32*)(p + 0xFC) * *(f32*)(p + 0xDC);
    f32 b = *(f32*)(p + 0x100) * *(f32*)(p + 0xE0);
    f32 sx = halfW + halfW * a;
    f32 sy = halfH + halfH * b;
    f32 dist = fabsf(sx - *(f32*)(p + 0x4)) + fabsf(sy - *(f32*)(p + 0x8));
    if (!(dist < 5.0f)) {
        sp68[0] = sx;
        sp68[1] = sy;
        sp68[2] = *(f32*)(*(u8**)p + 0x70);
        fn_80032CFC(p, (u8*)sp68);
    }
    fn_80032A98(p);
    fn_800325D8(p, dt);
    u8* obj = *(u8**)p;
    (*(SlotFn*)(*(u8**)(obj + 0x138) + 0x2C))(obj, 0x1A);
}

// 0x800330E0
// State machine of the "gem falling" effect (state at +8, 0..7); dt is the frame time.
//  0: arm the flag at +0x5D, go to 1.
//  1: tick the timer (+0x24 / +0x28), place and scale the gem object (+0x18) along the path,
//     swap the effect object at +0x14, and when the counter (+4) reaches the limit (+0) play
//     the sound and go to 2 (past the timer: go to 5 first).
//  2: start the "gem_falling" animation on the gem object, go to 7.
//  3: go to 7.   4: nothing.
//  5: copy the gem transform to the object at +0x1C and +0x54, play a sound, go to 6.
//  6: count down +0x2C, then park the object at +0x1C off screen and go to 7.
//  7: once the gem fell below 398 or is gone, hide it, add +0x10 to the progress and go to 3.
void fn_800330E0(u8* p, f32 dt) {
    u32 state = *(u32*)(p + 0x8);
    if (state > 7) return;
    switch (state) {
    case 0:
        *(u8*)(p + 0x5D) = 1;
        *(s32*)(p + 0x8) = 1;
        return;
    case 1: {
        f32 t = *(f32*)(p + 0x24) + dt;
        f32 dur = *(f32*)(p + 0x28);
        *(f32*)(p + 0x24) = t;
        if (t > dur) {
            *(s32*)(p + 0x8) = 5;
        } else {
            f32 s = 0.5f + *(f32*)(p + 0x30) * (t / dur);
            f32 vx = *(f32*)(p + 0x3C) * s;
            f32 vy = *(f32*)(p + 0x40) * s;
            f32 vz = *(f32*)(p + 0x44) * s;
            u8* g = *(u8**)(p + 0x18);
            if (g != NULL) {
                *(f32*)(g + 0x68) = *(f32*)(p + 0x48);
                *(f32*)(g + 0x6C) = *(f32*)(p + 0x4C);
                *(f32*)(g + 0x70) = *(f32*)(p + 0x50);
                g = *(u8**)(p + 0x18);
                *(f32*)(g + 0x74) = vx;
                *(f32*)(g + 0x78) = vy;
                *(f32*)(g + 0x7C) = vz;
                g = *(u8**)(p + 0x18);
                (*(SlotFn*)(*(u8**)(g + 0x138) + 0x2C))(g, 0x38);
                g = *(u8**)(p + 0x18);
                f32 zero = 0.0f;
                f32 rx = *(f32*)(g + 0x90) + zero;
                f32 ry = *(f32*)(g + 0x94) + zero;
                f32 rz = *(f32*)(g + 0x98) + 0.017453292f * *(f32*)(p + 0x38);
                *(f32*)(g + 0x90) = rx;
                *(f32*)(g + 0x94) = ry;
                *(f32*)(g + 0x98) = rz;
                if (*(u8*)(p + 0x5D) != 0) {
                    g = *(u8**)(p + 0x18);
                    u8* q = *(u8**)(p + 0x58);
                    *(f32*)(q + 0x68) = *(f32*)(g + 0x68);
                    *(f32*)(q + 0x6C) = *(f32*)(g + 0x6C);
                    *(f32*)(q + 0x70) = *(f32*)(g + 0x70);
                    q = *(u8**)(p + 0x58);
                    *(f32*)(q + 0x70) = 0.01f;
                    q = *(u8**)(p + 0x58);
                    (*(SlotFn*)(*(u8**)(q + 0x138) + 0x2C))(q, 0x1A);
                    *(u8*)(p + 0x5D) = 0;
                }
            }
            u8* old = *(u8**)(p + 0x14);
            u8* cur = *(u8**)(p + 0x18);
            if (old != NULL) (*(SlotFn*)(*(u8**)(old + 0x138) + 0x30))(old, 2);
            *(u8**)(p + 0x14) = cur;
            if (cur != NULL) (*(SlotFn*)(*(u8**)(cur + 0x138) + 0x2C))(cur, 2);
        }
        if (*(s32*)(p + 0x4) < *(s32*)(p + 0x0)) return;
        if (lbl_805FBBAC == NULL) {
            void* v = fn_801D40E0(1);
            if (v != NULL) v = fn_80209814(v);
            lbl_805FBBAC = v;
        }
        PlaySound(0x961CE754u, 0, 0, 0, 0, 0);
        u8* fx = *(u8**)(p + 0x14);
        *(s32*)(p + 0x8) = 2;
        if (fx != NULL) (*(SlotFn*)(*(u8**)(fx + 0x138) + 0x30))(fx, 2);
        *(u8**)(p + 0x14) = NULL;
        return;
    }
    case 2: {
        *(u8*)(p + 0x5C) = 1;
        u32 hash = StringHash(lbl_80357D50);
        u8* g = *(u8**)(p + 0x18);
        if (*(s32*)(g + 0x10) != 0) fn_8019BC80(g, 1);
        void* anim = fn_80198F1C(ScreenSetsCurrent(gScreenSets), hash);
        g = *(u8**)(p + 0x18);
        (*(SlotFn*)(*(u8**)(g + 0x138) + 0x2C))(g, 0x1A);
        fn_8019BBA4(*(u8**)(p + 0x18), (u8*)anim, 10);
        *(s32*)(p + 0x8) = 7;
        return;
    }
    case 3:
        *(s32*)(p + 0x8) = 7;
        return;
    case 4:
        return;
    case 5: {
        u8* q = *(u8**)(p + 0x1C);
        if (q != NULL) {
            u8* g = *(u8**)(p + 0x18);
            f32 one = 1.0f;
            f32 w = *(f32*)(g + 0x8C);
            *(f32*)(q + 0x80) = one;
            *(f32*)(q + 0x84) = one;
            *(f32*)(q + 0x88) = one;
            *(f32*)(q + 0x8C) = w;
            g = *(u8**)(p + 0x18);
            q = *(u8**)(p + 0x1C);
            *(f32*)(q + 0x68) = *(f32*)(g + 0x68);
            *(f32*)(q + 0x6C) = *(f32*)(g + 0x6C);
            *(f32*)(q + 0x70) = *(f32*)(g + 0x70);
            g = *(u8**)(p + 0x18);
            q = *(u8**)(p + 0x1C);
            *(f32*)(q + 0x74) = *(f32*)(g + 0x74);
            *(f32*)(q + 0x78) = *(f32*)(g + 0x78);
            *(f32*)(q + 0x7C) = *(f32*)(g + 0x7C);
            q = *(u8**)(p + 0x1C);
            (*(SlotFn*)(*(u8**)(q + 0x138) + 0x2C))(q, 0x1A);
            g = *(u8**)(p + 0x18);
            u8* r = *(u8**)(p + 0x54);
            *(f32*)(r + 0x68) = *(f32*)(g + 0x68);
            *(f32*)(r + 0x6C) = *(f32*)(g + 0x6C);
            *(f32*)(r + 0x70) = *(f32*)(g + 0x70);
            r = *(u8**)(p + 0x54);
            *(f32*)(r + 0x70) = 0.01f;
            r = *(u8**)(p + 0x54);
            (*(SlotFn*)(*(u8**)(r + 0x138) + 0x2C))(r, 0x1A);
            q = *(u8**)(p + 0x1C);
            if (*(s32*)(q + 0x10) != 0) fn_8019BC80(q, 1);
        }
        if (lbl_805FBBAC == NULL) {
            void* v = fn_801D40E0(1);
            if (v != NULL) v = fn_80209814(v);
            lbl_805FBBAC = v;
        }
        PlaySound(0x22102C0Eu, 0, 0, 0, 0, 0);
        *(s32*)(p + 0x8) = 6;
        return;
    }
    case 6: {
        u8* fx = *(u8**)(p + 0x14);
        if (fx != NULL) (*(SlotFn*)(*(u8**)(fx + 0x138) + 0x30))(fx, 2);
        *(u8**)(p + 0x14) = NULL;
        f32 t = *(f32*)(p + 0x2C);
        if (t <= 0.0f) {
            u8* q = *(u8**)(p + 0x1C);
            *(f32*)(q + 0x68) = -1000.0f;
            *(f32*)(q + 0x6C) = -1000.0f;
            *(f32*)(q + 0x70) = 0.0f;
            q = *(u8**)(p + 0x1C);
            (*(SlotFn*)(*(u8**)(q + 0x138) + 0x2C))(q, 0x1A);
            *(s32*)(p + 0x8) = 7;
            *(f32*)(p + 0x2C) = 1.0f;
        } else {
            *(f32*)(p + 0x2C) = t - dt;
        }
        return;
    }
    case 7: {
        u8* fx = *(u8**)(p + 0x14);
        if (fx != NULL) (*(SlotFn*)(*(u8**)(fx + 0x138) + 0x30))(fx, 2);
        *(u8**)(p + 0x14) = NULL;
        if (*(u8*)(p + 0x5C) == 0) return;
        u8* g = *(u8**)(p + 0x18);
        if (!(*(f32*)(g + 0x6C) > 398.0f) && *(s32*)(g + 0x10) != 0) return;
        (*(SlotFn*)(*(u8**)(g + 0x138) + 0x30))(g, 0x1A);
        *(u8*)(p + 0x5C) = 0;
        fn_80031F84(*(s32*)(p + 0x10));
        *(s32*)(p + 0x8) = 3;
        fn_8019BC80(*(u8**)(p + 0x18), 1);
        return;
    }
    }
}

// 0x800367CC
// Initialises a bit-mask image: clears the 0x800-byte mask at +4 and the counters, then looks up
// the screen-set element "Image_<arg2>" (+0x810), reads its pixel header and sets one bit per
// transparent pixel (row (0x7F - a) * 4, column word (b / 32) * 0x200, bit 0x80000000 >> (b & 31));
// opaque pixels are counted in the float at +0x818. Finally selects "Image_<arg3>" as the element.
void fn_800367CC(u8* p, s32 arg1, s32 arg2, s32 arg3) {
    char name[32];
    *(s32*)(p + 0x820) = arg1;
    *(u8**)(p + 0x810) = NULL;
    *(s32*)(p + 0x814) = 0;
    *(f32*)(p + 0x818) = 0.0f;
    *(f32*)(p + 0x81C) = 0.0f;
    *(f32*)(p + 0x824) = 0.0f;
    *(s32*)(p + 0x828) = 0;
    *(f32*)(p + 0x82C) = 0.0f;
    *(s32*)(p + 0x830) = 0;
    memset(p + 4, 0, 0x800);
    f32 v = *(f32*)p;
    *(f32*)(p + 0x804) = v;
    *(f32*)(p + 0x808) = v;
    *(f32*)(p + 0x80C) = 0.0f;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    SysSprintf(name, lbl_80357F38, arg2);
    u8* el = (u8*)ScreenSetFindElement(set, StringHash(name));
    *(u8**)(p + 0x810) = el;
    if (el != NULL) {
        fn_801A1208(el);
        u8* hdr = (u8*)fn_801A1208(*(u8**)(p + 0x810)) + 4;
        for (s32 a = 0; a < (s32)*(u16*)(hdr + 2); a++) {
            for (s32 b = 0; b < (s32)*(u16*)(hdr + 0); b++) {
                if ((fn_8015B82C(hdr, a, b) >> 24) == 0) {
                    u8* w = p + ((b / 32) << 9) + (0x7F - a) * 4;
                    *(u32*)(w + 4) |= 0x80000000u >> (b & 31);
                } else {
                    *(f32*)(p + 0x818) = *(f32*)(p + 0x818) + 1.0f;
                }
            }
        }
    }
    if (arg2 != arg3) {
        SysSprintf(name, lbl_80357F38, arg3);
        *(u8**)(p + 0x810) = (u8*)ScreenSetFindElement(set, StringHash(name));
    }
}

// 0x8003C888
// Screen-shake decay: while the timer (+0x99C) is >= 0 it computes an amplitude from the time left
// (a damped cosine), offsets the object at +0x9AC by a random +-22 / +-16 scaled shake around the
// base position (+0x9A0/+0x9A4) and refreshes it (slot 0x2C, arg 0x18); once negative it stops.
void fn_8003C888(u8* p, f32 dt) {
    if (*(s32*)(p + 0x998) == 0) return;
    f32 t = *(f32*)(p + 0x99C);
    if (t >= 0.0f) {
        f32 k = 0.6f;
        f32 root = 0.0f;
        f32 sq = 1.0f - k * k;
        if (!(sq <= 0.0f)) root = sqrtf(sq);
        f32 x = 12.566371f * (3.0f - *(f32*)(p + 0x99C));
        f32 c = cosf(1.5707964f * (0.63661975f * (root * x)));
        f32 amp = (f32)fn_8033F20C(2.0, (f64)(-k * x)) * c;
        u8* obj = *(u8**)(p + 0x9AC);
        *(f32*)(p + 0x99C) = *(f32*)(p + 0x99C) - dt;
        f32 dx = 22.0f * amp;
        f32 dy = 16.0f * amp;
        if (RandomU32() & 1) dx = dx * -1.0f;
        if (RandomU32() & 1) dy = dy * -1.0f;
        *(f32*)(obj + 0x68) = dx + *(f32*)(p + 0x9A0);
        *(f32*)(obj + 0x6C) = dy + *(f32*)(p + 0x9A4);
        obj = *(u8**)(p + 0x9AC);
        (*(SlotFn*)(*(u8**)(obj + 0x138) + 0x2C))(obj, 0x18);
    } else {
        *(f32*)(p + 0x99C) = 0.0f;
        *(s32*)(p + 0x998) = 0;
    }
}

// 0x8003E3A4
// Deleting destructor of the battle-setup singleton: sets the vtable, destroys the battle
// (gBattle, virtual slot 8), clears the battle flags and globals, frees the camera object and
// the buffer at lbl_805FB018, zeroes the combat part zone flags and frees the object if del > 0.
u8* fn_8003E3A4(u8* p, s32 del) {
    if (p == NULL) return p;
    *(u8**)p = lbl_803584A0;
    if (gBattle != NULL) {
        (*(SlotFn*)(*(u8**)gBattle + 8))(gBattle, 1);
        gBattle = NULL;
    }
    lbl_805FB1C0 = 0;
    lbl_805FB1C1 = 0;
    if (lbl_805FB014 != NULL) {
        fn_80040A08(lbl_805FB014, 1);
        lbl_805FB014 = NULL;
    }
    if (lbl_805FB018 != NULL) {
        operator_delete(lbl_805FB018);
        lbl_805FB018 = NULL;
    }
    lbl_805FB010 = NULL;
    lbl_805FB01C = NULL;
    lbl_805FB020 = NULL;
    lbl_805FB024 = 0;
    lbl_805FB025 = 0;
    lbl_805FB028 = 0;
    lbl_805FB02C = 0;
    lbl_805FB030 = 0;
    for (s32 i = 0; i < lbl_805FB038; i++) lbl_803C4E28[i] = 0;
    lbl_805FB038 = 0;
    lbl_805FB034 = 0;
    if (del > 0) operator_delete(p);
    return p;
}

// 0x800548F4
// Speed factor from the state in bits 12..15 of the flags at +0x748: 0.8 for 1, 0.9 for 3,
// else 1.0 (always 1.0 when the mode at +0x78C is 1).
f32 fn_800548F4(u8* p) {
    if (*(s32*)(p + 0x78C) == 1) return 1.0f;
    switch ((*(u32*)(p + 0x748) >> 12) & 0xF) {
    case 1: return 0.8f;
    case 3: return 0.9f;
    default: return 1.0f;
    }
}

// 0x8005BE5C
// Tail call of virtual slot 0x98 with (obj, 0); the return type is not known (vtable entry only).
s32 fn_8005BE5C(u8* p) {
    return (*(s32 (**)(void*, s32))(*(u8**)p + 0x98))(p, 0);
}

// 0x8008517C
// Timed spawner: lazily takes the target (+0x18) from the owner's (+4) record (+0x1F4 -> +0x264);
// when that record has no pending target and the timer (+0x20) runs out, counts a spawn (+0x14),
// activates the spawned object (flag +0x268, +8 bits 2) with the target and sends event
// 0x49C2D2C1; re-arms the timer (+0x1C) while count < limit (+0x10), else sets +0xC to -1.
// The countdown at +0xC always advances by dt.
void fn_8008517C(u8* p, f32 dt) {
    if (*(u8**)(p + 0x18) == NULL) {
        *(u8**)(p + 0x18) = *(u8**)(*(u8**)(*(u8**)(p + 0x4) + 0x1F4) + 0x264);
    }
    u8* target = *(u8**)(p + 0x18);
    if (target == NULL) return;
    u8* owner = *(u8**)(p + 0x4);
    if (*(u8**)(*(u8**)(owner + 0x1F4) + 0x264) == NULL) {
        f32 t = *(f32*)(p + 0x20) - dt;
        *(f32*)(p + 0x20) = t;
        if (t <= 0.0f) {
            *(s32*)(p + 0x14) = *(s32*)(p + 0x14) + 1;
            u8* o = fn_8007F900(owner, *(s32*)(target + 0x264));
            if (*(u8*)(o + 0x268) != 1) {
                *(u8*)(o + 0x268) = 1;
                if ((*(GetFn*)(*(u8**)o + 0xC))(o) != NULL) {
                    u8* r = (*(GetFn*)(*(u8**)o + 0xC))(o);
                    *(u32*)(r + 0x8) |= 2;
                }
                u8* q = *(u8**)(o + 0x4);
                *(u32*)(q + 0x20) |= 2;
                *(s32*)(o + 0x2B4) = 0;
            }
            fn_800822A8(o, *(u8**)(p + 0x18));
            EventSend(gEventMgr, o, NULL, 0x49C2D2C1u, 0, 0, 1, 0);
            if (*(s32*)(p + 0x14) < *(s32*)(p + 0x10)) {
                *(f32*)(p + 0x20) = *(f32*)(p + 0x1C);
            } else {
                *(f32*)(p + 0xC) = -1.0f;
            }
        }
    }
    *(f32*)(p + 0xC) = *(f32*)(p + 0xC) - dt;
}

}  // extern "C"
