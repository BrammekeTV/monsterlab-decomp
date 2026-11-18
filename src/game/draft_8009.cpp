// Robot evolution experiment (0x8009xxxx): lab object setup/teardown, difficulty-scaled score
// percentage, HUD update, stick-driven slot selection, per-frame update, and end-of-round handling.
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

typedef f32 (*VGetScoreFn)(void* obj);                      // vtable +0x64: current score (<= 0 means lost)
typedef void (*VSetHiddenFn)(void* obj, s32 hidden);        // vtable +0x14 / +0x18
typedef u8* (*VFindFn)(void* obj, f32 a, f32 b);            // vtable +0x10
typedef void (*VWidgetFn)(void* widget, s32 what);          // widget helper table +0x2C

extern "C" {
s32   AnimSetFindClip(void* set, s32 anim);                                       // 0x801AC2FC
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);                    // 0x80209E70
void  PropPlayAnim(void* prop, int a, int b, BOOL once);
u32   RandomU32(void);                                                            // 0x80143460
s32   RaySceneTest(void* ray);                                                    // 0x80070458
void  ScopedLockCtor(ScopedLock* l);                                              // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                                   // 0x801F53B0
void  StickRead(s32 channel, f32* x, f32* y);                                     // 0x80147704
void  TextureRelease(void* tex);                                                  // 0x80184028
void  UISetNumber(void* el, s32 n);                                               // 0x8019F2A8
void  UISetProgress(void* widget, f32 t);
void* UIWidgetFindChild(void* widget, u32 hash);                                  // 0x80199AEC
void* fn_80070F4C(void);                                                          // 0x80070F4C
void  fn_80070FAC(u8* p, f32 v);                                                  // 0x80070FAC
void  fn_800712BC(u8* p, s32 a, s32 b);                                          // 0x800712BC
void  fn_8008F068(u8* p);                                                         // 0x8008F068
void  fn_8008F7D4(u8* p);                                                         // 0x8008F7D4
void  fn_800906EC(u8* p);
void  fn_80090900(u8* p, f32 dt);
void  fn_80090B00(u8* p, f32 dt);
void  fn_800917FC(u8* p, s32 unused);
void  fn_80090D24(u8* p, f32 dt);                                                 // 0x80090D24
void  fn_80091E88(u8* p, f32 dt);                                                 // 0x80091E88
void  fn_80091F28(u8* p, s32 a);                                                  // 0x80091F28
void  fn_8009253C(u8* p, f32 dt);                                                 // 0x8009253C
void  fn_800925C0(u8* p, void* tex, f32* color, s32 flag);                        // 0x800925C0
void  fn_80092678(u8* p);                                                         // 0x80092678
void  fn_800928F4(u8* p);                                                         // 0x800928F4
void  fn_800929D8(u8* p);                                                         // 0x800929D8
void  fn_80092C40(u8* p, s32 a);                                                  // 0x80092C40
void  fn_80093B3C(u8* p, s32 a, f32 dt);                                          // 0x80093B3C
void  fn_80093BF0(u8* p);                                                         // 0x80093BF0
void  fn_80093C20(u8* p, s32 on);                                                 // 0x80093C20
void* fn_801D40E0(s32 n);                                                         // 0x801D40E0
void* fn_80209814(void* v);                                                       // 0x80209814
BOOL  fn_80216C80(u8* lab, s32 level);                                            // 0x80216C80
f32   fn_802170E8(u8* lab, s32 level);                                            // 0x802170E8

extern void* gEventMgr;            // 0x805FB6C8
extern u8    gWorldState[];        // 0x803E6D00
extern s32   gZoneResourceSize;    // 0x805FB94C
extern s32   lbl_803471B0[];       // 0x803471B0 (tables indexed by the RaySceneTest result)
extern s32   lbl_803471D0[];       // 0x803471D0
extern s32   lbl_803471E0[];       // 0x803471E0
extern s32   lbl_803471F0[];       // 0x803471F0
extern s32   lbl_80347220[];       // 0x80347220
extern u8    lbl_803C4298[];       // 0x803C4298 (+0x38 = controller)
extern s32   lbl_805FB124;         // 0x805FB124 (returned by RaySceneTest)
extern void* lbl_805FBBAC;         // 0x805FBBAC (voice SFX handle)
}

static const u32 kHashRobotKid     = 0x0C898A89u;  // "robotkidA"
static const u32 kHashEvolutionDoor = 0xB4CA5278u; // "robot_evolution_door1A"
static const u32 kHashSmasher      = 0x5DB90DD2u;  // "robot_evolution_smasherA"
static const u32 kEventStart       = 0xC608C842u;
static const u32 kEventLose        = 0x117741C3u;

extern "C" {

// 0x80090040
// Per-frame update: counts the round timer down, ends the round on timeout (or when both timers
// are out in the alternate mode), then updates the slots and the score handling.
void fn_80090040(u8* p, f32 dt) {
    *(f32*)(p + 0x7E8) = *(f32*)(p + 0x7E8) - dt;
    fn_800906EC(p);
    if (lbl_803471F0[RaySceneTest(p)] == 0) {
        if (*(f32*)(p + 0x7E8) <= 0.0f) fn_800917FC(p, 1);
    } else {
        if (*(f32*)(p + 0x7E8) <= 0.0f && *(f32*)(p + 0x7EC) <= 0.0f) fn_800917FC(p, 1);
    }
    if ((*(VGetScoreFn*)(*(u8**)(p + 0x4) + 0x64))(p) <= 0.0f) fn_800917FC(p, 0);
    fn_80090900(p, dt);
    fn_80090B00(p, dt);
    fn_80090D24(p, dt);
}

// 0x80090154
// Resets the three slots and event targets (mode 1), then plays the intro clip on the prop.
void fn_80090154(u8* p) {
    u8* a = p + 0xA8;
    u8* b = p + 0x1F8;
    *(f32*)(p + 0x8C) = 0.0f;
    for (s32 i = 0; i < 3; i++) {
        fn_80092C40(a, 1);
        fn_80091F28(b, 1);
        a += 0x48;
        b += 0x14;
    }
    u8* prop = *(u8**)(p + 0xA4);
    u8* x = *(u8**)(prop + 0x9C);
    if (x != NULL) {
        u8* set = *(u8**)(x + 0x48);
        if (set != NULL) PropPlayAnim(prop, AnimSetFindClip(set, (s32)0xCD668510u), 0, TRUE);
    }
}

// 0x80090248
// Same as fn_80090154 but with mode 0.
void fn_80090248(u8* p) {
    u8* a = p + 0xA8;
    u8* b = p + 0x1F8;
    *(f32*)(p + 0x8C) = 0.0f;
    for (s32 i = 0; i < 3; i++) {
        fn_80092C40(a, 0);
        fn_80091F28(b, 0);
        a += 0x48;
        b += 0x14;
    }
    u8* prop = *(u8**)(p + 0xA4);
    u8* x = *(u8**)(prop + 0x9C);
    if (x != NULL) {
        u8* set = *(u8**)(x + 0x48);
        if (set != NULL) PropPlayAnim(prop, AnimSetFindClip(set, (s32)0xCD668510u), 0, TRUE);
    }
}

// 0x8009033C
// Teardown: sends the start event to every flagged robot/door/smasher object in the zone, resets
// the three slots, releases the texture at +0x358 and restores the base state.
void fn_8009033C(u8* p) {
    s32 n1 = *(s32*)(*(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize + 0x94);
    s32 n2 = *(s32*)(*(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize + 0x98);
    s32 total = n1 + n2;
    for (s32 i = 0; i < total; i++) {
        u8* res = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
        s32 first = *(s32*)(res + 0x94);
        u8* obj;
        u8* rec;
        if (i < first) {
            obj = ((u8**)*(u8**)(res + 0x120))[i];
            rec = *(u8**)(res + 0x8C) + i * 0x40;
        } else {
            s32 j = i - first;
            obj = ((u8**)*(u8**)(res + 0x12C))[j];
            rec = *(u8**)(res + 0x90) + j * 0x40;
        }
        if ((*(u32*)(rec + 0x1C) & 0x40) && obj != NULL) {
            u8* info = *(u8**)(obj + 0x9C);
            s32 type = (info != NULL) ? *(s32*)(info + 0x8) : 0;
            if (type == (s32)kHashRobotKid || type == (s32)kHashEvolutionDoor || type == (s32)kHashSmasher) {
                EventSend(gEventMgr, obj, NULL, kEventStart, 0, 0, 1, 0);
            }
        }
    }
    u8* slot = p + 0xA8;
    for (s32 i = 0; i < 3; i++) {
        fn_800929D8(slot);
        slot += 0x48;
    }
    u8* tex = *(u8**)(p + 0x358);
    if (tex != NULL) {
        s32 rc = *(s32*)(tex + 0x24) - 1;
        *(s32*)(tex + 0x24) = rc;
        if (rc == 0) TextureRelease(tex);
        *(u8**)(p + 0x358) = NULL;
    }
    fn_8008F7D4(p);
    *(s32*)(p + 0x2AC) = lbl_805FB124;
    fn_8008F068(p);
}

// 0x800904EC
// Score as a percentage (clamped to 0..100) of the points the current difficulty allows.
f32 fn_800904EC(u8* p) {
    s32 a = lbl_803471B0[RaySceneTest(p)] * 2;
    s32 lim = *(s32*)(p + 0x2A8);
    s32 cap;
    if (lim < lbl_80347220[RaySceneTest(p)]) {
        cap = *(s32*)(p + 0x2A8);
    } else {
        cap = lbl_80347220[RaySceneTest(p)];
    }
    s32 total = a * cap;
    s32 sub = a * (*(s32*)(p + 0x354) - 1);
    s32 num = total / 2 + (*(s32*)(p + 0x7F4) - sub / 2);
    f32 pct = 100.0f * ((f32)num / (f32)total);
    if (pct < 0.0f) return 0.0f;
    if (pct > 100.0f) return 100.0f;
    return pct;
}

// 0x800906EC
// Updates the HUD widgets (score counters, progress bar, four gauges) and then the timer display.
void fn_800906EC(u8* p) {
    u8* hud = (u8*)fn_80070F4C();
    if (hud != NULL) {
        s32 n = *(s32*)(p + 0x7F8) + 1;
        UISetNumber(UIWidgetFindChild(hud, 0x57EFBC28u), n / 2);
        s32 m = *(s32*)(p + 0x7F8) + 1;
        UISetNumber(UIWidgetFindChild(hud, 0xB9E1DD04u), (m % 2) * 5);
        UISetProgress(UIWidgetFindChild(hud, 0x5A8F51E9u), *(f32*)(p + 0x7EC));
        static const u32 kGauges[4] = { 0x90252014u, 0xA1CD3A89u, 0x07BA313Du, 0x7DBA57FFu };
        for (s32 i = 0; i < 4; i++) {
            u8* w = (u8*)UIWidgetFindChild(hud, kGauges[i]);
            f32 v = *(f32*)(p + 0x804);
            *(f32*)(w + 0x90) = 0.0f;
            *(f32*)(w + 0x94) = 0.0f;
            *(f32*)(w + 0x98) = v;
            (*(VWidgetFn*)(*(u8**)(w + 0x138) + 0x2C))(w, 0x18);
        }
    }
    f32 t = *(f32*)(p + 0x7E8);
    f32 clamped;
    if (t < 0.0f) clamped = 0.0f;
    else if (t > 600.0f) clamped = 600.0f;
    else clamped = t;
    f32 t2 = *(f32*)(p + 0x7E8);
    fn_800712BC(p, (s32)clamped, t2 <= 10.0f);
}

// 0x80090900
// Reads the stick: picks slot 0/1/2 from the horizontal axis, moves the highlight (animating the
// old and new slot objects, playing a sound), and starts the selected slot when the button is held.
// The second argument is unused.
void fn_80090900(u8* p, f32 /*unused dt*/) {
    f32 y = 0.0f;
    f32 x = 0.0f;
    StickRead(*(s32*)(lbl_803C4298 + 0x38) * 0x10 + 0xE, &x, &y);
    s32 dir;
    if (x < -0.7f) dir = 0;
    else if (x > 0.7f) dir = 2;
    else dir = 1;
    if (*(u8*)(p + 0x7E4) == 0) return;
    s32 cur = *(s32*)(p + 0x7E0);
    if (cur != dir) {
        if (cur != -1) {
            fn_80093C20(p + cur * 0x28 + 0x180, 0);
            u8* obj = *(u8**)(p + *(s32*)(p + 0x7E0) * 4 + 0x98);
            ScopedLock lock;
            ScopedLockCtor(&lock);
            (*(VSetHiddenFn*)(*(u8**)obj + 0x18))(obj, 1);
            ScopedLockDtor(&lock, -1);
        }
        *(s32*)(p + 0x7E0) = dir;
        fn_80093C20(p + dir * 0x28 + 0x180, 1);
        u8* obj = *(u8**)(p + *(s32*)(p + 0x7E0) * 4 + 0x98);
        ScopedLock lock;
        ScopedLockCtor(&lock);
        (*(VSetHiddenFn*)(*(u8**)obj + 0x14))(obj, 1);
        ScopedLockDtor(&lock, -1);
        if (lbl_805FBBAC == NULL) {
            void* v = fn_801D40E0(1);
            if (v != NULL) v = fn_80209814(v);
            lbl_805FBBAC = v;
        }
        PlaySound(0x01C16BFEu, 0, 0, 0, 0, 0);
    }
    if (*(u8*)(p + 0x3EC) != 0) {
        f32 t = *(f32*)(p + 0x3F0);
        if (t > 0.0f) *(f32*)(p + 0x364) = t;
    }
    if (*(u8*)(p + 0x3EC) != 0) {
        u8* rec = p + dir * 0x28;
        s32 st = *(s32*)(rec + 0x188);
        bool active = (st == 1 || st == 2);
        if (!active) fn_80093BF0(rec + 0x180);
    }
}

// 0x80090B00
// Per-frame slot update: scales the prop's speed with the remaining time, ticks the three slots,
// and awards the difficulty-table points for slots that finished.
void fn_80090B00(u8* p, f32 dt) {
    f32 ratio = 1.0f - (*(f32*)(p + 0x7E8) / 70.0f);
    f32 scale = 1.0f + 4.0f * ratio;
    u8* obj = *(u8**)(p + 0x90);
    u8* r = (*(VFindFn*)(*(u8**)obj + 0x10))(obj, 1.0f, ratio);
    if (r != NULL) *(f32*)(r + 0xB0) = *(f32*)(p + 0x94) * scale;
    for (s32 i = 0; i < 3; i++) {
        fn_8009253C(p + 0xA8 + i * 0x48, dt);
        fn_80093B3C(p + 0x180 + i * 0x28, *(s32*)(p + i * 0x48 + 0xB0) == 2, dt);
        fn_80091E88(p + 0x1F8 + i * 0x14, dt);
        s32 st = *(s32*)(p + i * 0x28 + 0x188);
        u32 active = (st == 1 || st == 2) ? 1u : 0u;
        if (active == 1u && *(u8*)(p + i * 0x28 + 0x19C) == 1) {
            if (*(s32*)(p + i * 0x48 + 0xB0) == 1) {
                s32 pts;
                if (*(s32*)(p + 0x324) != i) pts = lbl_803471B0[RaySceneTest(p)];
                else                         pts = lbl_803471E0[RaySceneTest(p)];
                *(s32*)(p + 0x7F4) = *(s32*)(p + 0x7F4) + pts;
                fn_80070FAC(p, (*(VGetScoreFn*)(*(u8**)(p + 0x4) + 0x64))(p) / 100.0f);
                fn_80092678(p + 0xA8 + i * 0x48);
            } else {
                s32 pts = lbl_803471D0[RaySceneTest(p)];
                *(s32*)(p + 0x7F4) = *(s32*)(p + 0x7F4) + pts;
                fn_80070FAC(p, (*(VGetScoreFn*)(*(u8**)(p + 0x4) + 0x64))(p) / 100.0f);
            }
        }
    }
}

// 0x800917FC
// Ends the round: hides the gauge, releases the texture, stops the slots, deactivates the
// selected slot object and signals win (score > 0) or lose. The second argument is unused.
void fn_800917FC(u8* p, s32 unused) {
    u8* tex = *(u8**)(p + 0x358);
    if (tex != NULL) {
        UIElement_PlayUIAnim(0x7DBA57FFu, 0x3D242667u);
        tex = *(u8**)(p + 0x358);
        s32 rc = *(s32*)(tex + 0x24) - 1;
        *(s32*)(tex + 0x24) = rc;
        if (rc == 0) TextureRelease(tex);
        *(u8**)(p + 0x358) = NULL;
    }
    u8* slot = p + 0xA8;
    for (s32 i = 0; i < 3; i++) {
        fn_800928F4(slot);
        slot += 0x48;
    }
    s32 cur = *(s32*)(p + 0x7E0);
    *(u8*)(p + 0x7E4) = 0;
    if (cur != -1) {
        fn_80093C20(p + cur * 0x28 + 0x180, 0);
        u8* obj = *(u8**)(p + *(s32*)(p + 0x7E0) * 4 + 0x98);
        ScopedLock lock;
        ScopedLockCtor(&lock);
        (*(VSetHiddenFn*)(*(u8**)obj + 0x18))(obj, 1);
        ScopedLockDtor(&lock, -1);
        *(s32*)(p + 0x7E0) = -1;
    }
    if ((*(VGetScoreFn*)(*(u8**)(p + 0x4) + 0x64))(p) > 0.0f) {
        GameFlowSignal((s32)kEventStart, 0, 0);
    } else {
        GameFlowSignal((s32)kEventLose, 0, 0);
    }
}

// 0x80091950
// Starts the round: ends it at once if the level exceeds the lab's limit, picks the round time and
// a random wobble direction, sets up the gauge texture/colour and the three slot textures.
void fn_80091950(u8* p) {
    if (*(s32*)(p + 0x354) > *(s32*)(p + 0x2A8)) {
        if ((*(VGetScoreFn*)(*(u8**)(p + 0x4) + 0x64))(p) > 0.0f) {
            GameFlowSignal((s32)kEventStart, 0, 0);
        } else {
            GameFlowSignal((s32)kEventLose, 0, 0);
        }
    }
    fn_80216C80(p + 0x2A8, *(s32*)(p + 0x354));
    *(f32*)(p + 0x800) = 0.011111111f * fn_802170E8(p + 0x2A8, *(s32*)(p + 0x354));
    if (RandomU32() & 1) *(f32*)(p + 0x800) = *(f32*)(p + 0x800) * -1.0f;
    s32 colorIdx = *(s32*)(p + 0x314);
    *(f32*)(p + 0x804) = 0.0f;
    u8* tex = (u8*)UITextureList_GetTexture(0xB0B94AB0u, *(s32*)(p + 0x318));
    *(u8**)(p + 0x358) = tex;
    *(s32*)(tex + 0x24) = *(s32*)(tex + 0x24) + 1;
    UIImage_SetTexture(0x7DBA57FFu, (BOOL)(intptr_t)*(u8**)(p + 0x358));
    UIElement_SetColor(0x7DBA57FFu, (BOOL)(intptr_t)(p + colorIdx * 0x10 + 0x234));
    for (s32 i = 0; i < 3; i++) {
        s32 idx = *(s32*)(p + i * 0x1C + 0x2C0);
        u8* t = (u8*)UITextureList_GetTexture(0xB0B94AB0u, *(s32*)(p + i * 0x1C + 0x2C4));
        u8* col = p + idx * 0x10;
        f32 c[4];
        c[0] = *(f32*)(col + 0x234);
        c[1] = *(f32*)(col + 0x238);
        c[2] = *(f32*)(col + 0x23C);
        c[3] = *(f32*)(col + 0x240);
        fn_800925C0(p + 0xA8 + i * 0x48, t, c, *(s32*)(p + 0x324) == i);
    }
}

}  // extern "C"
