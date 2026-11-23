// Catch-minigame style screen objects in 0x8002xxxx - 0x8003xxxx (float-heavy).
#include <math.h>

#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);    // 0x80209E70
void* ScreenSetFindElement(void* set, u32 hash);                 // 0x80198F18
void* ScreenSetsCurrent(void* sets);                             // 0x8018D1E4
u32   StringHash(const char* s);                                 // 0x801EB704
int   SysSprintf(char* buf, const char* fmt, ...);               // 0x80142EF8
void  CatchMinigame_UpdateBase(void* g, f32 dt);                 // 0x800224C4
void  fn_800223DC(u8* p);                                        // 0x800223DC
void  fn_8002BE88(u8* obj, f32* v, u8* el);                      // 0x8002BE88
void  fn_80032CFC(u8* arg0, u8* arg1);                           // 0x80032CFC
s32   fn_80032574(u8* p);                                        // 0x80032574
void  fn_80032E44(u8* p, u8* obj);                               // 0x80032E44
void  fn_80032EB0(u8* p, s32 a, u8* b);                          // 0x80032EB0
void  fn_80032FBC(u8* p);                                        // 0x80032FBC
void  fn_800336DC(u8* a, u8* b, u8* c);                          // 0x800336DC
void  fn_80033958(u8* p);                                        // 0x80033958
u8*   fn_80034048(u8* p);                                        // 0x80034048
void* UIAnim_FindByName(void* set, u32 hash);                          // 0x80198F1C
s32   fn_8019BBA4(u8* arg0, u8* arg1, s32 arg2);                 // 0x8019BBA4
s32   fn_8019BC80(u8* arg0, u32 arg1);                           // 0x8019BC80
u8*   fn_801B3308(void);                                         // 0x801B3308
void  fn_801B3310(u8* p, u8* q);                                 // 0x801B3310
void* fn_801D40E0(s32 n);                                        // 0x801D40E0
u8*   fn_801F61AC(u8* arg0, s32 arg1);                           // 0x801F61AC
void  fn_801F85E4(u8* p, const u8* src, s32 count);              // 0x801F85E4
void  fn_801F8F34(u8* p, f32 v);                                 // 0x801F8F34
void  fn_801F8FA0(u8* p, f32 v);                                 // 0x801F8FA0
void  fn_801F916C(f32* out, u8* p, s32 a);                       // 0x801F916C
void  fn_801F94B0(u8* p, s32 v);                                 // 0x801F94B0
void* fn_80209814(void* v);                                      // 0x80209814

u8*   fn_80032208(u8* arg0, u8* arg1, f32* arg2, f32* arg3);     // 0x80032208
}

extern "C++" void* gEventMgr;    // 0x805FB6C8
extern "C++" void* gScreenSets;  // 0x805FB654
extern s32 gScreenWidth;         // 0x805F9388
extern s32 gScreenHeight;        // 0x805F938C
extern u8  lbl_803C4298[];       // 0x803C4298
extern u8  lbl_80347A10[];       // 0x80347A10 ("gem99" scratch name buffer init)
extern f32 lbl_80347548[];       // 0x80347548 (3 floats)
extern s32 lbl_805FAFF4;         // 0x805FAFF4
extern void* lbl_805FBBAC;       // 0x805FBBAC (voice SFX handle)

typedef void (*ElemVFn)(u8*, s32);

extern "C" {

// 0x8002B94C
// Per-frame update of a two-state slider object: on state 1 it fires an event and rumbles when the
// element is idle; otherwise it re-centres the element on screen, tracks the pointer and sends a
// left/right event depending on the sign of the pointer's y.
void fn_8002B94C(u8* arg0, f32 dt) {
    (void)dt;
    s32 state = *(s32*)(arg0 + 0x4C);
    if (state == 1) {
        if (*(s32*)(*(u8**)(arg0 + 0x48) + 0x10) == 0) {
            *(s32*)(arg0 + 0x4C) = 4;
            EventSend(gEventMgr, *(void**)(arg0 + 0x50), NULL, 0x93A9FBA8u, 0, 0, 1, 0);
            TimedRumble(*(s32*)(lbl_803C4298 + 0x38), 0.1f, 4);
            u8* p = *(u8**)(arg0 + 0x48);
            u8* el = *(u8**)(arg0 + *(s32*)(arg0 + 0x8) * 4);
            f32 v[3];
            v[0] = *(f32*)(p + 0x68);
            v[1] = *(f32*)(p + 0x6C);
            v[2] = *(f32*)(el + 0x70);
            fn_8002BE88(arg0, v, el);
        }
        return;
    }
    if (state == 4) {
        *(s32*)(arg0 + 0x4C) = 0;
    } else if (*(s32*)(*(u8**)(arg0 + 0x48) + 0x10) == 0) {
        f32 px = *(f32*)(arg0 + 0x568);
        f32 py = *(f32*)(arg0 + 0x56C);
        f32 halfW = 0.5f * (f32)gScreenWidth;
        f32 halfH = 0.5f * (f32)gScreenHeight;
        f32 sy = *(f32*)(arg0 + 0x58C) * py;
        f32 sx = *(f32*)(arg0 + 0x588) * px;
        u8* el = *(u8**)(arg0 + *(s32*)(arg0 + 0x8) * 4);
        f32 v[3];
        v[0] = halfW + halfW * sx;
        v[1] = halfH + halfH * sy;
        v[2] = *(f32*)(el + 0x70);
        fn_8002BE88(arg0, v, el);
        *(s32*)(arg0 + 0x8) = 0;
        u8* o = *(u8**)arg0;
        (*(ElemVFn*)(*(u8**)(o + 0x138) + 0x2C))(o, 2);
    }
    if (*(u8*)(arg0 + 0xE4) != 0) {
        f32 t = *(f32*)(arg0 + 0xE8);
        if (t > 0.0f) *(f32*)(arg0 + 0x5C) = t;
    }
    if (*(u8*)(arg0 + 0xE4) != 0) {
        f32 v[3];
        fn_801F916C(v, arg0 + 0x54, 0);
        fn_8019BC80(*(u8**)(arg0 + 0x48), 1);
        fn_8019BC80(*(u8**)arg0, 1);
        if (v[1] < 0.0f) {
            EventSend(gEventMgr, *(void**)(arg0 + 0x50), NULL, 0xC8FF2C8Cu, 0, 0, 1, 0);
            *(s32*)(arg0 + 0x4C) = 1;
        }
        if (v[1] > 0.0f) {
            EventSend(gEventMgr, *(void**)(arg0 + 0x50), NULL, 0xC486AEE5u, 0, 0, 1, 0);
            *(s32*)(arg0 + 0x4C) = 0;
        }
    }
}

// 0x800317A8
// Init of the dig minigame: looks up the "rock", "pickaxe", "spray", gem and progress-meter screen
// elements, builds the pickaxe object (0x5C0 bytes), pairs each gem with its shatter element and
// sends the start event.
void fn_800317A8(u8* arg0) {
    fn_800223DC(arg0);
    void* set = ScreenSetsCurrent(gScreenSets);
    u8* rock = (u8*)ScreenSetFindElement(set, StringHash("rock"));
    (*(ElemVFn*)(*(u8**)(rock + 0x138) + 0x2C))(rock, 0x1A);
    f32 pos[2];
    f32 end[2];
    f32 x = *(f32*)(rock + 0x68);
    f32 y = *(f32*)(rock + 0x6C);
    pos[0] = x;
    pos[1] = y;
    end[0] = x + *(f32*)(rock + 0x74);
    end[1] = y + *(f32*)(rock + 0x78);
    u8* pick = (u8*)ScreenSetFindElement(set, StringHash("pickaxe"));
    (*(ElemVFn*)(*(u8**)(pick + 0x138) + 0x2C))(pick, 0x1A);
    u8* obj = (u8*)fn_801D40E0(0x5C0);
    if (obj != NULL) obj = fn_80032208(obj, pick, pos, end);
    *(u8**)(arg0 + 0xCC) = obj;
    fn_80032E44(*(u8**)(arg0 + 0xCC), (u8*)ScreenSetFindElement(set, StringHash("spray")));
    fn_80032EB0(*(u8**)(arg0 + 0xCC), *(s32*)(arg0 + 0xA8), *(u8**)(arg0 + 0xC0));
    s32 i = 0;
    s32 off = 0;
    char name[32];
    for (s32 k = 0; k < 10; k++) name[k] = (char)lbl_80347A10[k];
    while (i < *(s32*)(arg0 + 0xA8)) {
        SysSprintf(name, "gem%02d", i + 1);
        u8* gem = (u8*)ScreenSetFindElement(set, StringHash(name));
        SysSprintf(name, "gemshatterB%02d", i + 1);
        fn_800336DC(*(u8**)(arg0 + 0xC0) + off, gem, (u8*)ScreenSetFindElement(set, StringHash(name)));
        fn_80033958(*(u8**)(arg0 + 0xC0) + off);
        off += 0x60;
        i += 1;
    }
    *(f32*)(arg0 + 0xD4) = 2.0f;
    void* set2 = ScreenSetsCurrent(gScreenSets);
    *(u8**)(arg0 + 0xAC) = (u8*)ScreenSetFindElement(set2, StringHash("ProgressMeter"));
    if (*(f32*)(arg0 + 0xB4) == 0.0f && *(f32*)(arg0 + 0xB8) == 0.0f && *(f32*)(arg0 + 0xBC) == 0.0f) {
        u8* m = *(u8**)(arg0 + 0xAC);
        *(f32*)(arg0 + 0xB4) = *(f32*)(m + 0x68);
        *(f32*)(arg0 + 0xB8) = *(f32*)(m + 0x6C);
        *(f32*)(arg0 + 0xBC) = *(f32*)(m + 0x70);
        *(f32*)(arg0 + 0xB0) = *(f32*)(m + 0x74);
    }
    *(f32*)(*(u8**)(arg0 + 0xAC) + 0x74) = 0.0f;
    *(f32*)(*(u8**)(arg0 + 0xAC) + 0x68) = 700.0f;
    u8* m = *(u8**)(arg0 + 0xAC);
    (*(ElemVFn*)(*(u8**)(m + 0x138) + 0x2C))(m, 0x18);
    CatchMinigame_UpdateBase(arg0, 0.0f);
    EventSend(gEventMgr, *(void**)(arg0 + 0x90), NULL, 0xF15918A6u, 0, 0, 1, 0);
}

// 0x80032208
// Constructor of the pickaxe object: stores its element and bounds (arg2/arg3 are the two corner
// points), sets up two sub-objects at +0x4C and +0x108 with an up-axis (0,-1,0), registers them,
// insets the bounds by 2, clamps the position and looks up the two hit/miss effect elements.
u8* fn_80032208(u8* arg0, u8* arg1, f32* arg2, f32* arg3) {
    *(u8**)(arg0 + 0x0) = arg1;
    *(void**)(arg0 + 0xC) = NULL;
    *(void**)(arg0 + 0x10) = NULL;
    *(void**)(arg0 + 0x14) = NULL;
    *(void**)(arg0 + 0x18) = NULL;
    *(void**)(arg0 + 0x1C) = NULL;
    *(f32*)(arg0 + 0x20) = arg2[0];
    *(f32*)(arg0 + 0x24) = arg2[1];
    *(f32*)(arg0 + 0x28) = arg3[0];
    *(f32*)(arg0 + 0x2C) = arg3[1];
    *(u8*)(arg0 + 0x40) = 0;
    *(u8*)(arg0 + 0x41) = 1;
    *(s32*)(arg0 + 0x44) = 0;
    *(s32*)(arg0 + 0x48) = 1;
    s32 rumble = *(s32*)(lbl_803C4298 + 0x38);
    fn_801F61AC(arg0 + 0x4C, rumble);
    fn_801F94B0(arg0 + 0x108, *(s32*)(lbl_803C4298 + 0x38));
    // (the original has an empty loop over +0x58C..+0x598 here)
    *(void**)(arg0 + 0x598) = NULL;
    *(void**)(arg0 + 0x59C) = NULL;
    *(f32*)(arg0 + 0x5BC) = 0.0f;
    f32 ax = 0.0f;
    f32 ay = -1.0f;
    f32 az = 0.0f;
    f32 lenSq = az * az + (ax * ax + ay * ay);
    f32 nx, ny, nz;
    if (lenSq == 0.0f) {
        nx = 0.0f;
        ny = 0.0f;
        nz = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(lenSq);
        nx = ax * inv;
        ny = ay * inv;
        nz = az * inv;
    }
    *(f32*)(arg0 + 0x58C) = nx;
    *(f32*)(arg0 + 0x590) = ny;
    *(f32*)(arg0 + 0x594) = nz;
    *(s32*)(arg0 + 0x10C) = *(s32*)(lbl_803C4298 + 0x38);
    *(s32*)(arg0 + 0x1A0) = 1;
    fn_801F85E4(arg0 + 0x108, arg0 + 0x58C, 1);
    *(f32*)(arg0 + 0x19C) = 0.00001f;
    fn_801F8FA0(arg0 + 0x108, 0.0000001f);
    fn_801F8F34(arg0 + 0x108, 0.000001f);
    fn_801B3310(fn_801B3308(), arg0 + 0x4C);
    fn_801B3310(fn_801B3308(), arg0 + 0x108);
    *(f32*)(arg0 + 0x20) = *(f32*)(arg0 + 0x20) + 2.0f;
    *(f32*)(arg0 + 0x24) = *(f32*)(arg0 + 0x24) + 2.0f;
    *(f32*)(arg0 + 0x28) = *(f32*)(arg0 + 0x28) - 2.0f;
    *(f32*)(arg0 + 0x2C) = *(f32*)(arg0 + 0x2C) - 2.0f;
    fn_80032CFC(arg0, *(u8**)arg0 + 0x68);
    u8* o = *(u8**)arg0;
    *(f32*)(o + 0x90) = lbl_80347548[0];
    *(f32*)(o + 0x94) = lbl_80347548[1];
    *(f32*)(o + 0x98) = lbl_80347548[2];
    o = *(u8**)arg0;
    *(f32*)(arg0 + 0x4) = *(f32*)(o + 0x68);
    *(f32*)(arg0 + 0x8) = *(f32*)(o + 0x6C);
    void* set = ScreenSetsCurrent(gScreenSets);
    *(void**)(arg0 + 0x598) = ScreenSetFindElement(set, 0x80F90F3Du);
    *(void**)(arg0 + 0x59C) = ScreenSetFindElement(set, 0x9F37CD7Eu);
    return arg0;
}

// 0x800325D8
// Per-frame update of the pickaxe: when fn_80032574 reports state 5 it plays the "axe_updown"
// animation on first entry, spawns the hit effect at the target (or the miss effect at the cursor
// if there is no hit) with its sound and rumble, and moves the pickaxe element to the cursor.
void fn_800325D8(u8* arg0, f32 dt) {
    *(f32*)(arg0 + 0x5BC) = *(f32*)(arg0 + 0x5BC) + dt;
    if (*(u8*)(arg0 + 0x41) != 0) {
        lbl_805FAFF4 = 0;
        *(u8*)(arg0 + 0x40) = 0;
    }
    s32 st = fn_80032574(arg0);
    *(s32*)(arg0 + 0x44) = st;
    if (st == 5) {
        *(f32*)(arg0 + 0x5BC) = 0.0f;
        if (lbl_805FAFF4 != st) {
            *(u8*)(arg0 + 0x40) = 0;
            u32 hash = StringHash("axe_updown");
            u8* o = *(u8**)arg0;
            if (*(s32*)(o + 0x10) != 0) fn_8019BC80(o, 1);
            fn_8019BBA4(*(u8**)arg0, (u8*)UIAnim_FindByName(ScreenSetsCurrent(gScreenSets), hash), 0xA);
        }
        u8* target = *(u8**)(arg0 + 0x10);
        if (target != NULL && *(s32*)(target + 0x8) == 1) {
            *(u8**)(arg0 + 0x14) = target;
            if (lbl_805FBBAC == NULL) {
                void* v = fn_801D40E0(1);
                if (v != NULL) v = fn_80209814(v);
                lbl_805FBBAC = v;
            }
            PlaySound(0x7336F883u, 0, 0, 0, 0, 0);
            TimedRumble(*(s32*)(lbl_803C4298 + 0x38), 0.2f, 4);
            fn_80032FBC(*(u8**)(arg0 + 0x10));
            u8* t = *(u8**)(arg0 + 0x10);
            u8* fx = *(u8**)(arg0 + 0x598);
            f32 x = *(f32*)(t + 0x48);
            f32 y = *(f32*)(t + 0x4C);
            f32 z = *(f32*)(t + 0x50);
            *(f32*)(fx + 0x68) = x;
            *(f32*)(fx + 0x6C) = y;
            *(f32*)(fx + 0x70) = z;
            *(f32*)(*(u8**)(arg0 + 0x598) + 0x70) = 0.01f;
            fx = *(u8**)(arg0 + 0x598);
            (*(ElemVFn*)(*(u8**)(fx + 0x138) + 0x2C))(fx, 0x1A);
        }
        u8* anim = NULL;
        if (*(u8**)(arg0 + 0x10) != NULL) anim = fn_80034048(*(u8**)(arg0 + 0x10));
        if (anim == NULL) {
            anim = (u8*)UIAnim_FindByName(ScreenSetsCurrent(gScreenSets), 0xBB143141u);
            if (lbl_805FBBAC == NULL) {
                void* v = fn_801D40E0(1);
                if (v != NULL) v = fn_80209814(v);
                lbl_805FBBAC = v;
            }
            PlaySound(0xB31D9C02u, 0, 0, 0, 0, 0);
            f32 cy = *(f32*)(arg0 + 0x8);
            f32 cx = *(f32*)(arg0 + 0x4);
            u8* fx = *(u8**)(arg0 + 0x59C);
            *(f32*)(fx + 0x68) = cx;
            *(f32*)(fx + 0x6C) = cy;
            *(f32*)(fx + 0x70) = 0.01f;
            fx = *(u8**)(arg0 + 0x59C);
            (*(ElemVFn*)(*(u8**)(fx + 0x138) + 0x2C))(fx, 0x1A);
        }
        u8* pick = *(u8**)(arg0 + 0xC);
        f32 py = *(f32*)(arg0 + 0x8);
        f32 pz = *(f32*)(pick + 0x70);
        f32 px = *(f32*)(arg0 + 0x4);
        *(f32*)(pick + 0x68) = px;
        *(f32*)(pick + 0x6C) = py;
        *(f32*)(pick + 0x70) = pz;
        pick = *(u8**)(arg0 + 0xC);
        if (*(s32*)(pick + 0x10) != 0) fn_8019BC80(pick, 1);
        fn_8019BBA4(*(u8**)(arg0 + 0xC), anim, 0xA);
        *(u8*)(arg0 + 0x41) = 1;
        *(s32*)(arg0 + 0x44) = 4;
    }
    lbl_805FAFF4 = *(s32*)(arg0 + 0x44);
}

// 0x80032A98
// Picks, among the entries (0x60 bytes each, count at +0x1C, array at +0x18), the one whose
// ground distance to the cursor (+4/+8) is smallest and stores it at +0x10; clears +0x10 again
// if that distance is not within 50 * (entry +0x24 / +0x28).
void fn_80032A98(u8* arg0) {
    *(void**)(arg0 + 0x10) = NULL;
    u8* e = *(u8**)(arg0 + 0x18);
    f32 x = *(f32*)(arg0 + 0x4);
    f32 z = *(f32*)(arg0 + 0x8);
    f32 dx = *(f32*)(e + 0x48) - x;
    f32 dz = *(f32*)(e + 0x4C) - z;
    f32 dy = *(f32*)(e + 0x50) - *(f32*)(e + 0x50);
    f32 lenSq = dy * dy + (dx * dx + dz * dz);
    f32 best;
    if (lenSq <= 0.0f) {
        best = 0.0f;
    } else {
        best = lenSq * (1.0f / sqrtf(lenSq));
    }
    *(u8**)(arg0 + 0x10) = e;
    s32 i = 1;
    s32 off = 0x60;
    while (i < *(s32*)(arg0 + 0x1C)) {
        u8* c = *(u8**)(arg0 + 0x18) + off;
        f32 cx = *(f32*)(arg0 + 0x4);
        f32 cz = *(f32*)(arg0 + 0x8);
        f32 ex = *(f32*)(c + 0x48) - cx;
        f32 ez = *(f32*)(c + 0x4C) - cz;
        f32 ey = *(f32*)(c + 0x50) - *(f32*)(c + 0x50);
        f32 lsq = ey * ey + (ex * ex + ez * ez);
        f32 len;
        if (lsq <= 0.0f) {
            len = 0.0f;
        } else {
            len = lsq * (1.0f / sqrtf(lsq));
        }
        if (best > len) {
            // the original recomputes the same distance here
            best = len;
            *(u8**)(arg0 + 0x10) = c;
        }
        off += 0x60;
        i += 1;
    }
    u8* sel = *(u8**)(arg0 + 0x10);
    f32 ratio = 0.0f;
    f32 den = *(f32*)(sel + 0x28);
    if (den != 0.0f) ratio = *(f32*)(sel + 0x24) / den;
    f32 limit = 5.0f * 10.0f * ratio;
    if (!(best <= limit)) *(void**)(arg0 + 0x10) = NULL;
}

// 0x8003B994
// Called when a gem is hit: moves the slot element at +0x948 to the position of the one at +0x924,
// rumbles, and for the current kind (+0x3DC) marks entry +0x3E0 as flying off (speed to the
// off-screen x of 768 or -128 depending on arg1), then plays the hit sound; plays the miss sound
// when nothing was selected.
void fn_8003B994(u8* arg0, s32 arg1) {
    f32 target = 768.0f;
    if (arg1 == 4) target = -128.0f;
    if (*(s32*)(arg0 + 0x3D8) != 0 && *(s32*)(arg0 + 0x3E0) >= 0) {
        u8* src = *(u8**)(arg0 + 0x924);
        *(s32*)(arg0 + 0x950) = 1;
        f32 y = *(f32*)(src + 0x6C);
        f32 x = *(f32*)(src + 0x68);
        // (the original dispatches through a compiler-folded switch whose 0x944/0x94C arms are
        // unreachable: only the +0x948 element is ever used)
        u8* el = *(u8**)(arg0 + 0x948);
        *(f32*)(el + 0x68) = x;
        *(f32*)(el + 0x6C) = y;
        *(f32*)(el + 0x70) = 0.0f;
        el = *(u8**)(arg0 + 0x948);
        (*(ElemVFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x1A);
        TimedRumble(*(s32*)(lbl_803C4298 + 0x38), 0.2f, 4);
        s32 kind = *(s32*)(arg0 + 0x3DC);
        if (kind == 0) {
            *(u8*)(arg0 + *(s32*)(arg0 + 0x3E0) + 0x29A) = 1;
            s32 idx = *(s32*)(arg0 + 0x3E0);
            f32 spd = *(f32*)(arg0 + 0x390);
            u8* slot = *(u8**)(arg0 + idx * 4 + 0x244);
            *(f32*)(arg0 + idx * 0xC + 0x2E8) = (target - *(f32*)(slot + 0x68)) / spd;
            *(f32*)(arg0 + *(s32*)(arg0 + 0x3E0) * 0xC + 0x2F0) = *(f32*)(arg0 + 0x390);
            if (lbl_805FBBAC == NULL) {
                void* v = fn_801D40E0(1);
                if (v != NULL) v = fn_80209814(v);
                lbl_805FBBAC = v;
            }
            PlaySound(0xBC8DC241u, 0, 0, 0, 0, 0);
        } else if (kind == 1) {
            *(u8*)(arg0 + *(s32*)(arg0 + 0x3E0) + 0x294) = 1;
            s32 idx = *(s32*)(arg0 + 0x3E0);
            f32 spd = *(f32*)(arg0 + 0x390);
            u8* slot = *(u8**)(arg0 + idx * 4 + 0x1FC);
            *(f32*)(arg0 + idx * 0xC + 0x2A0) = (target - *(f32*)(slot + 0x68)) / spd;
            *(f32*)(arg0 + *(s32*)(arg0 + 0x3E0) * 0xC + 0x2A8) = *(f32*)(arg0 + 0x390);
            if (lbl_805FBBAC == NULL) {
                void* v = fn_801D40E0(1);
                if (v != NULL) v = fn_80209814(v);
                lbl_805FBBAC = v;
            }
            PlaySound(0xB36206DBu, 0, 0, 0, 0, 0);
        }
    } else {
        if (lbl_805FBBAC == NULL) {
            void* v = fn_801D40E0(1);
            if (v != NULL) v = fn_80209814(v);
            lbl_805FBBAC = v;
        }
        PlaySound(0x16CB5F04u, 0, 0, 0, 0, 0);
    }
}

}  // extern "C"
