// Missing functions batch 122: pause counter, widget focus/highlight, dirt fling, part
// matchup preview, 4x3 matrix multiply, contact flagging, effect-slot removal, id pool pop,
// save error report.
#include "types.h"
#include "engine/math.h"
#include "game/part.h"

extern "C" {
#include "game/script_callbacks.h"
}

typedef void (*VFnI)(u8*, s32);
typedef u8* (*VFnP)(u8*);
typedef void (*VFnPP)(u8*, u8*);

extern "C" {
void  AudioSetPaused(void* audio, int paused);                  // 0x801FF708
u8**  FighterGetSlot(u8* f, s32 i);                             // 0x8020E794
void  OSReport(const char* fmt, ...);
s32   PartMatchup(s32 defA, s32 defB);                          // 0x80213988
void  QuatGetForward(Vec3* out, const Quat* q);                 // 0x801454CC
void* ScreenSetFindWidget(void* set, u32 hash);                 // 0x80198E34
void* ScreenSetsCurrent(void* sets);                            // 0x8018D1E4
u32   StringHash(const char* s);                                // 0x801EB704
void  TargetListDecode(void* t, const u32* mask, s32 side);     // 0x80216240
void* TargetListDestroy(void* t, s32 del);                      // 0x80216200
void  TargetListInit(void* t);                                  // 0x802161DC
void* UIWidgetFindChild(void* widget, u32 hash);                // 0x80199AEC
void  fn_801455E0(f32* out, u8* quat);                          // 0x801455E0
void  fn_8018DBB8(void* sets, s32 a);                           // 0x8018DBB8
u8*    fn_8019552C(u8* list, s32 index);                         // 0x8019552C
s32   fn_8019555C(u8* list);                                    // 0x8019555C
s32 fn_801987BC(u8* arg0, u8* arg1);                     // 0x801987BC
s32   fn_80198A8C(void* set, void* widget);                     // 0x80198A8C
BOOL  fn_80198C34(void* set, void* w);                          // 0x80198C34
u8*   fn_80198E20(u8* set);                                     // 0x80198E20
void* fn_80198F1C(void* set, u32 hash);                         // 0x80198F1C
s32   fn_8019EE40(u8* widget, u32 id, s32 flag);                // 0x8019EE40
s32   fn_801AA730(u8* dst, u8* src);                      // 0x801AA730
void  fn_801AA760(f32* out, u32 id);                            // 0x801AA760
s32 fn_801B19C4(u8* a, u32 hashA, u8* b, u32 hashB, u8* outA, u8* outB, s32 c, s32 d, u8 e);  // 0x801B19C4
s32   fn_801D5F6C(void);                                        // 0x801D5F6C
void  fn_801D65CC(f32* out);                                    // 0x801D65CC
s32   fn_801E6FAC(u8* ent, s32 flag, f32 t);                    // 0x801E6FAC
u32   fn_801EC880(u8* p, u32 hash);                             // 0x801EC880
void  fn_801EC8C4(u8* p, u32 a, u32 b);                         // 0x801EC8C4
f32   fn_801F0650(void);                                        // 0x801F0650
void  fn_801F0658(f32 t);                                       // 0x801F0658
BOOL  fn_80212E28(u8** slot);                                   // 0x80212E28

extern u8   gAudio[];                                           // 0x8048A8F0 region (audio state)
extern u8*  gBattle;                                            // 0x805FBBB8
extern u8   gIdPool[];                                          // 0x8048A8F0
extern "C++" s32 gPartSlotState[10];
extern "C++" s32 gSelectedPart;
extern u32  lbl_8035916C[];                                     // 0x8035916C
extern Vec3 lbl_803475D0;                                       // 0x803475D0
extern u8   lbl_80386C10[];                                     // 0x80386C10 (save error messages)
extern u8   lbl_8047C790[];                                     // 0x8047C790
extern const char lbl_805F9914[];                               // 0x805F9914
extern u8   lbl_805FB09C[];                                     // 0x805FB09C (per action ready flags)
extern f32  lbl_805FB80C;                                       // 0x805FB80C
extern void (*lbl_805FBB24)(s32);                               // 0x805FBB24 (error callback)
extern u8*  lbl_805FBBE0;                                       // 0x805FBBE0 (save block pointer)
}
extern "C++" void* gScreenSets;                                 // 0x805FB654

static const u32 kHashMissingWiimote  = 0xB1E1B80Bu;  // "MissingWiimote"
static const u32 kHashMissingNunchuck = 0x0F9FE8CEu;  // "MissingNunchuck"
static const u32 kHashPause           = 0x20CE505Du;  // "PAUSE"

// Calls vtable-like slot `off` of the sub-object at child+0x138 with value 2, if the child exists.
static void WidgetSlot(void* root, u32 hash, s32 off) {
    if (root != NULL) {
        u8* child = (u8*)UIWidgetFindChild(root, hash);
        if (child != NULL) {
            (*(VFnI*)(*(u8**)(child + 0x138) + off))(child, 2);
        }
    }
}

// Entry `i` of the ring buffer at p (head +0x10, tail +0x14, capacity +0x18, items +0x1C, stride
// 0x88), or NULL when the index is not inside the live range.
static u8* RingEntry(u8* p, s32 i) {
    s32 head = *(s32*)(p + 0x10);
    s32 cap = *(s32*)(p + 0x18);
    if (i < 0 || i >= cap) return NULL;
    s32 lo = i;
    s32 hi = i;
    if (i < head) {
        lo = i + cap;
        hi = i + cap;
    }
    if (head <= lo && lo <= hi) return *(u8**)(p + 0x1C) + i * 0x88;
    return NULL;
}

// Moves the tail index of the ring buffer back by one (or empties it when head == tail).
static void RingPopTail(u8* p) {
    s32 tail = *(s32*)(p + 0x14);
    if (tail == *(s32*)(p + 0x10)) {
        *(s32*)(p + 0x14) = -1;
        *(s32*)(p + 0x10) = -1;
    } else {
        s32 n = tail - 1;
        if (n < 0) n = *(s32*)(p + 0x18) - 1;
        *(s32*)(p + 0x14) = n;
    }
}

// 256-bit mask stored as four 64-bit values (high word first), highest 64 bits first.
static void MaskSet(u8* m, u32 bit) {
    u32 base;
    u32 off;
    if (bit < 0x40) { base = 0; off = 0x18; }
    else if (bit < 0x80) { base = 0x40; off = 0x10; }
    else if (bit < 0xC0) { base = 0x80; off = 0x8; }
    else { base = 0xC0; off = 0x0; }
    u64 v = (u64)1 << (bit - base);
    *(u32*)(m + off + 4) |= (u32)v;
    *(u32*)(m + off) |= (u32)(v >> 32);
}

static BOOL MaskTest(u8* m, u32 bit) {
    u32 base;
    u32 off;
    if (bit < 0x40) { base = 0; off = 0x18; }
    else if (bit < 0x80) { base = 0x40; off = 0x10; }
    else if (bit < 0xC0) { base = 0x80; off = 0x8; }
    else { base = 0xC0; off = 0x0; }
    u64 v = ((u64) * (u32*)(m + off) << 32) | *(u32*)(m + off + 4);
    return (BOOL)((v >> (bit - base)) & 1);
}

// Zeroes a 0x6C-byte contact record (the callee fills it in).
static void HitInit(u8* h) {
    for (s32 i = 0; i < 16; i++) *(f32*)(h + i * 4) = 0.0f;
    *(s32*)(h + 0x50) = 0;
    *(s32*)(h + 0x54) = 7;
    *(s32*)(h + 0x58) = 0;
    *(s32*)(h + 0x5C) = 0;
    *(s32*)(h + 0x60) = 0;
    *(s32*)(h + 0x64) = 0;
    *(u8**)(h + 0x68) = NULL;
}

// Copies one 0x20-byte slot, field by field, in the original's order.
static void SlotCopy(u8* d, const u8* s) {
    *(u32*)(d + 0x00) = *(const u32*)(s + 0x00);
    *(u32*)(d + 0x04) = *(const u32*)(s + 0x04);
    *(u8*)(d + 0x08) = *(const u8*)(s + 0x08);
    *(u8*)(d + 0x09) = *(const u8*)(s + 0x09);
    *(u16*)(d + 0x0A) = *(const u16*)(s + 0x0A);
    *(u16*)(d + 0x0E) = *(const u16*)(s + 0x0E);
    *(u8*)(d + 0x0D) = *(const u8*)(s + 0x0D);
    *(u8*)(d + 0x0C) = *(const u8*)(s + 0x0C);
    *(u32*)(d + 0x10) = *(const u32*)(s + 0x10);
    *(u32*)(d + 0x14) = *(const u32*)(s + 0x14);
    *(u32*)(d + 0x18) = *(const u32*)(s + 0x18);
    *(f32*)(d + 0x1C) = *(const f32*)(s + 0x1C);
}

extern "C" {

// 0x801E5C3C
// Pops finished entries off the id pool's ring buffer (tail first); when the first unfinished
// entry accepts the update it is cleared and one more slot is popped from the global pool.
void fn_801E5C3C(u8* pool, s32 flag, f32 t) {
    s32 empty = 1;
    s32 head = *(s32*)(pool + 0x10);
    if (head >= 0 && *(s32*)(pool + 0x14) >= 0) empty = 0;
    if (empty != 0) return;
    u8* e = RingEntry(pool, *(s32*)(pool + 0x14));
    while (e != NULL && *(s32*)(e + 0x70) != 0) {
        RingPopTail(pool);
        e = RingEntry(pool, *(s32*)(pool + 0x14));
    }
    if (e != NULL && fn_801E6FAC(e, flag, t) == 1) {
        *(s32*)(e + 0x4) = 0;
        RingPopTail(gIdPool);
    }
}

// 0x80206B6C
// Reports a save/IO error code through OSReport (message chosen by code), then calls the
// registered error callback if there is one.
void fn_80206B6C(s32 code, const u8* msg) {
    const char* m = (const char*)msg;
    if (msg == NULL) m = lbl_805F9914;
    s32 off;
    switch (code) {
    case -1:   off = 0xA0;  break;
    case -2:   off = 0xB8;  break;
    case -3:   off = 0xD8;  break;
    case -4:   off = 0xF0;  break;
    case -5:   off = 0x10C; break;
    case -6:   off = 0x128; break;
    case -8:   off = 0x140; break;
    case -9:   off = 0x15C; break;
    case -10:  off = 0x178; break;
    case -11:  off = 0x190; break;
    case -12:  off = 0x1AC; break;
    case -13:  off = 0x1C8; break;
    case -14:  off = 0x1E8; break;
    case -15:  off = 0x200; break;
    case -64:  off = 0x220; break;
    case -128: off = 0x23C; break;
    default:
        OSReport((const char*)(lbl_80386C10 + 0x260), code);
        goto done;
    }
    OSReport((const char*)(lbl_80386C10 + off), m);
done:
    if (lbl_805FBB24 != NULL) lbl_805FBB24(code);
}

// 0x8000AF80
// Adjusts one of the two pause counters (+0x13C when `which`, else +0x138) up or down, updates
// the paused flag (bit 1 of +0x3C), pauses/resumes audio and the screen sets, and saves/restores
// the time scale. Returns the previous paused flag.
s32 fn_8000AF80(u8* p, s32 up, s32 unused, s32 which) {
    s32 was = (*(u32*)(p + 0x3C) >> 1) & 1;
    if (which != 0) {
        if (up != 0) {
            *(s32*)(p + 0x13C) = *(s32*)(p + 0x13C) + 1;
        } else {
            s32 c = *(s32*)(p + 0x13C);
            if (c > 0) *(s32*)(p + 0x13C) = c - 1;
        }
    } else if (up != 0) {
        *(s32*)(p + 0x138) = *(s32*)(p + 0x138) + 1;
    } else {
        s32 c = *(s32*)(p + 0x138);
        if (c != 0) *(s32*)(p + 0x138) = c - 1;
    }
    if ((*(s32*)(p + 0x138) + *(s32*)(p + 0x13C)) != 0) {
        *(u32*)(p + 0x3C) |= 2;
    } else {
        *(u32*)(p + 0x3C) &= ~2u;
    }
    if (*(u32*)(p + 0x3C) & 2) {
        AudioSetPaused(gAudio, 1);
        *(u8*)((u8*)gScreenSets + 0x94) = 1;
        lbl_8047C790[2] = 1;
        lbl_805FB80C = 0.0f;
    } else {
        AudioSetPaused(gAudio, 0);
        *(u8*)((u8*)gScreenSets + 0x94) = 0;
        lbl_8047C790[2] = 0;
    }
    if (up != 0) {
        *(f32*)(p + 0x144) = fn_801F0650();
        fn_801F0658(1.0f);
    } else {
        fn_801F0658(*(f32*)(p + 0x144));
    }
    return was;
}

// 0x8000BD88
// Controller-state prompt: shows/hides the pad icons for the current mode (+0x158 / +0x154),
// swaps the displayed text id (+0x15C) and, when it changed, either arms the PAUSE signal or
// raises the game-flow signal.
void fn_8000BD88(u8* self) {
    u32 target = 0;
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    u8* root = (u8*)ScreenSetFindWidget(set, 0x1B6732BBu);
    if (root == NULL) return;
    u8* label = (u8*)UIWidgetFindChild(root, 0x7E654778u);
    if (label == NULL) return;
    s32 mode = *(s32*)(self + 0x158);
    if (mode != 0) {
        WidgetSlot(root, 0x8DC6BEC1u, 0x2C);
        WidgetSlot(root, 0x73660FB9u, 0x30);
        target = (mode == 2) ? 0x106A : 0x1029;
    } else if (*(s32*)(self + 0x154) == 1) {
        WidgetSlot(root, 0x8DC6BEC1u, 0x30);
        WidgetSlot(root, 0x73660FB9u, 0x2C);
        target = 0x101F;
    } else {
        WidgetSlot(root, 0x8DC6BEC1u, 0x30);
        WidgetSlot(root, 0x73660FB9u, 0x30);
    }
    if (target != 0 && *(u32*)(self + 0x15C) != target) {
        fn_8019EE40(label, target, 1);
        *(u32*)(self + 0x15C) = target;
        switch (target) {
        case 0x1029:
            WidgetSlot(root, kHashMissingWiimote, 0x2C);
            WidgetSlot(root, kHashMissingNunchuck, 0x30);
            *(f32*)(self + 0x160) = -1.0f;
            break;
        case 0x106A:
            WidgetSlot(root, kHashMissingWiimote, 0x30);
            WidgetSlot(root, kHashMissingNunchuck, 0x2C);
            *(f32*)(self + 0x160) = -1.0f;
            break;
        case 0x101F:
            WidgetSlot(root, kHashMissingWiimote, 0x30);
            WidgetSlot(root, kHashMissingNunchuck, 0x30);
            *(f32*)(self + 0x160) = 0.0f;
            break;
        }
        if (*(f32*)(self + 0x160) < 0.0f) {
            *(u32*)(self + 0x150) = kHashPause;
            *(u32*)(self + 0x14C) |= 8;
            *(u32*)(self + 0x148) = (*(u32*)(self + 0x148) | 1) & ~2u;
            return;
        }
        if (*(s32*)(self + 0x18) != 5) {
            *(u32*)(self + 0x148) = ((*(u32*)(self + 0x148) & ~8u) | 2) & ~1u;
            GameFlowSignal(0x0FCE227A, 0, 0);
        }
    }
}

// 0x8000C17C
// Re-applies the widget's rect (+0x2C..0x38, with the bottom clamped to the safe area) to its
// current rect fields (+0x80..0x8C) and drops focus if the widget is no longer in the set.
void fn_8000C17C(u8* self) {
    void* gs;
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    u8* w = (u8*)ScreenSetFindWidget(set, 0x1B6732BBu);
    if (w != NULL) {
        u8* c = (u8*)UIWidgetFindChild(w, 0xA15C894Bu);
        if (c != NULL) {
            f32 r[4];
            f32 safe[4];
            r[0] = *(f32*)(c + 0x2C);
            r[1] = *(f32*)(c + 0x30);
            r[2] = *(f32*)(c + 0x34);
            r[3] = *(f32*)(c + 0x38);
            if (fn_801D5F6C() == 0) {
                fn_801D65CC(safe);
                if (safe[3] > r[3]) r[3] = safe[3];
            }
            *(f32*)(c + 0x80) = r[0];
            *(f32*)(c + 0x84) = r[1];
            *(f32*)(c + 0x88) = r[2];
            *(f32*)(c + 0x8C) = r[3];
        }
    }
    if (w != NULL && fn_80198C34(set, w) == 0) {
        fn_801987BC(set, w);
        *(s32*)(self + 0x15C) = 0;
        gs = gScreenSets;
        fn_8018DBB8(gs, 0);
        return;
    }
    if (w != NULL && fn_80198E20(set) != NULL && fn_80198E20(set) != w) {
        fn_80198A8C(set, w);
        fn_801987BC(set, w);
        *(s32*)(self + 0x15C) = 0;
        gs = gScreenSets;
        fn_8018DBB8(gs, 0);
    }
}

// 0x80025524
// Spawns the "dirt_fling_anim" particles: one per entry of the animation's list, thrown from
// `pos` along the (normalized) velocity direction `vel` with a speed from the table at +0xAF4
// and a small upward drift that grows with the distance factor.
void fn_80025524(u8* self, f32* pos, f32* vel) {
    void* sets = gScreenSets;
    u32 hash = StringHash("dirt_fling_anim");
    u8* list = *(u8**)(*(u8**)((u8*)fn_80198F1C(ScreenSetsCurrent(sets), hash) + 4) + 4);
    s32 count = fn_8019555C(list);

    f32 len2 = vel[2] * vel[2] + (vel[0] * vel[0] + vel[1] * vel[1]);
    f32 len = (len2 <= 0.0f) ? 0.0f : sqrtf(len2);
    f32 scale = 10.0f;
    if (!(len < 10.0f)) {
        scale = 100.0f;
        if (!(len > 100.0f)) scale = len;
    }

    Vec3 n = Vec3Normalized(Vec3{vel[0], vel[1], vel[2]});
    f32 nlen2 = n.z * n.z + (n.x * n.x + n.y * n.y);

    for (s32 i = 0; i < count; i++) {
        f32 t = 0.0f;
        f32 nlen = (nlen2 <= 0.0f) ? 0.0f : sqrtf(nlen2);
        if (nlen > 0.0f) t = (*(f32**)(self + 0xAF4))[i] / scale;
        f32 w = (*(f32**)(self + 0xAF4))[i];
        f32 v[4];
        f32 out[4];
        v[0] = pos[0] + n.x * w;
        v[1] = pos[1] + ((t * (0.098f * t)) * 0.5f + n.y * w);
        v[2] = pos[2] + n.z * w;
        v[3] = pos[3] + 1.0f;
        u32 id = (u32)(uintptr_t)fn_8019552C(list, i);
        fn_801AA760(out, id);
        v[2] = out[2];
        v[3] = out[3];
        fn_801AA730((u8*)(intptr_t)id, (u8*)v);  // lint:ok 32-bit handle
    }
}

// 0x8004739C
// Part matchup preview: marks every enemy limb targeted by the selected part's action
// (index 0 = first action, else second) in gPartSlotState (2 = strong/neutral/weak result),
// and fires the one-time tutorial NIS signals the first time a matchup is shown.
void fn_8004739C(s32 which) {
    if (lbl_805FB09C[which] == 0) return;

    u8** arr = *(u8***)(gBattle + 0xA00);
    u8* part = *(u8**)arr[gSelectedPart];
    PartDef def;
    PartDbGetDef(&def, gGameDb, *(s32*)part);
    s32 actionIdx = (which == 0) ? def.action1 : def.action2;
    PartAction* action = (PartAction*)*(u8**)(gGameDb->data + 0x430) + actionIdx;
    u32 mask = action->unk08;

    u8* fighter = *(u8**)(gBattle + 0xA04);
    for (s32 i = 0; i < 5; i++) {
        fn_80212E28(FighterGetSlot(fighter, i));
        gPartSlotState[i + 5] = -1;
    }

    struct { s32 limbs[5]; s32 count; } t;
    TargetListInit(&t);
    TargetListDecode(&t, &mask, gSelectedPart);
    for (s32 k = 0; k < t.count; k++) {
        s32 limb = t.limbs[k];
        if (fn_80212E28(FighterGetSlot(fighter, limb))) {
            limb = 4;
            if (CheckGameProgressBitFlag(0xE) == 0) {
                s32 inNis = (gBattle != NULL) ? (*(s32*)(gBattle + 0xAD0) == 4) : 0;
                if (inNis == 0 && IsMultiPlayer() == 0) {
                    CombatHUDDisableInput(0xADBF3357u);
                    SendNISSignal(0x2163BF35u);
                }
            }
        }
        if (*(u32*)(lbl_805FBBE0 + 0x118) > 0x12C && CheckGameProgressBitFlag(0x15) == 0) {
            s32 inNis = (gBattle != NULL) ? (*(s32*)(gBattle + 0xAD0) == 4) : 0;
            if (inNis == 0 && IsMultiPlayer() == 0) {
                CombatHUDDisableInput(0xADBF3357u);
                SendNISSignal(0xB9596C7Au);
            }
        }
        s32 off = limb * 4;
        s32 r = PartMatchup(*(s32*)part, **(s32**)(*(u8**)(fighter + off)));
        switch (r) {
        case 0:
        case 1:
        case 2:
            gPartSlotState[limb + 5] = 2;
            break;
        }
    }
    TargetListDestroy(&t, -1);
}

// 0x8004F3E4
// Starts a thrown/pulled effect for the owner: stores the flag, then computes the target
// position (+0x398) from the owner's up axis, a forward-scaled offset and the distance to the
// target, stores the normalized reference direction (+0x3A4) and kicks the emitter effect.
void fn_8004F3E4(u8* self, s32 flag) {
    *(u8*)(self + 0x394) = (u8)flag;
    *(f32*)(self + 0x390) = 0.0f;
    *(u32*)(self + 0x15C) |= 1;
    if (flag == 0) return;

    u8* owner = (*(VFnP*)(*(u8**)self + 0xCC))(self);
    u8* node = *(u8**)(*(u8**)(owner + 0x24) + 0x1C);
    if (node != NULL) node -= 0xC48;
    u8* xf = *(u8**)(node + 4);
    f32 x0 = *(f32*)(xf + 0x0);
    f32 y0 = *(f32*)(xf + 0x4);
    f32 z0 = *(f32*)(xf + 0x8);
    Vec3 up = QuatGetAxisY((const Quat*)(xf + 0xC));
    f32 ux = -up.x * 2.0f;
    f32 uy = -up.y * 2.0f;
    f32 uz = -up.z * 2.0f;
    f32 dz0 = *(f32*)(*(u8**)(self + 0x4) + 0x8) - z0;
    f32 climb = 0.0f;
    if (dz0 >= climb) climb = dz0;
    f32 k = 3.0f + 0.2f * climb;
    f32 q[3];
    fn_801455E0(q, *(u8**)(node + 4) + 0xC);
    f32 qx = q[0] * k;
    f32 qy = q[1] * k;
    f32 qz = q[2] * k;
    u8* tgt = *(u8**)(self + 0x4);
    f32 dx = *(f32*)(tgt + 0x0) - x0;
    f32 dy = *(f32*)(tgt + 0x4) - y0;
    f32 dz = *(f32*)(tgt + 0x8) - z0;
    Vec3 fwd;
    QuatGetForward(&fwd, (const Quat*)(*(u8**)(node + 4) + 0xC));
    f32 along = ((fwd.z * dz) + ((fwd.x * dx) + (fwd.y * dy))) * 5.0f;
    Vec3 fwd2;
    QuatGetForward(&fwd2, (const Quat*)(*(u8**)(node + 4) + 0xC));
    f32 fy = fwd2.y * along;
    f32 fz = fwd2.z * along;
    f32 fx = fwd2.x * along;
    f32 rx = ux + qx;
    f32 ry = uy + qy;
    f32 rz = uz + qz;
    *(f32*)(self + 0x398) = rx + fx;
    *(f32*)(self + 0x39C) = ry + fy;
    *(f32*)(self + 0x3A0) = rz + fz;
    *(f32*)(self + 0x3A4) = lbl_803475D0.x;
    *(f32*)(self + 0x3A8) = lbl_803475D0.y;
    *(f32*)(self + 0x3AC) = lbl_803475D0.z;
    Vec3 dir = Vec3Normalized(lbl_803475D0);
    u8* fxs = self + 0x94;
    *(f32*)(self + 0x3A4) = dir.x;
    *(f32*)(self + 0x3A8) = dir.y;
    *(f32*)(self + 0x3AC) = dir.z;
    u8* owner2 = (*(VFnP*)(*(u8**)self + 0xCC))(self);
    s32 h = (s32)fn_801EC880(fxs, lbl_8035916C[*(s32*)(owner2 + 4)]);
    if (h >= 0) fn_801EC8C4(fxs, (u32)h, 0x5F73A0E4u);
}

// 0x800554DC
// 4x3 matrix product out = lhs * rhs (each stored as four columns of three floats; the fourth
// column is the translation).
void fn_800554DC(u8* out, u8* rhs, u8* lhs) {
    const f32* a = (const f32*)lhs;
    const f32* b = (const f32*)rhs;
    f32 a0 = a[0], a1 = a[1], a2 = a[2], a3 = a[3], a4 = a[4], a5 = a[5];
    f32 a6 = a[6], a7 = a[7], a8 = a[8], a9 = a[9], a10 = a[10], a11 = a[11];
    f32 b0 = b[0], b1 = b[1], b2 = b[2], b3 = b[3], b4 = b[4], b5 = b[5];
    f32 b6 = b[6], b7 = b[7], b8 = b[8], b9 = b[9], b10 = b[10], b11 = b[11];
    f32* o = (f32*)out;
    o[9] = a9 + (a6 * b11 + (a0 * b9 + a3 * b10));
    o[10] = a10 + (a7 * b11 + (a1 * b9 + a4 * b10));
    o[4] = a7 * b5 + (a1 * b3 + a4 * b4);
    o[11] = a11 + (a8 * b11 + (a2 * b9 + a5 * b10));
    o[7] = a7 * b8 + (a1 * b6 + a4 * b7);
    o[3] = a6 * b5 + (a0 * b3 + a3 * b4);
    o[0] = a6 * b2 + (a0 * b0 + a3 * b1);
    o[6] = a6 * b8 + (a0 * b6 + a3 * b7);
    o[1] = a7 * b2 + (a1 * b0 + a4 * b1);
    o[2] = a8 * b2 + (a2 * b0 + a5 * b1);
    o[5] = a8 * b5 + (a2 * b3 + a5 * b4);
    o[8] = a8 * b8 + (a2 * b6 + a5 * b7);
}

// 0x80055E38
// Collides two bodies ("attack" shape of a against "BAse" shape of b); on a hit, optionally flips
// b's contact normal, marks the hit bit on a's collision record, and otherwise notifies both
// bodies of the contact through their vtable slots 0x64 / 0x68.
void fn_80055E38(u8* a, u8* b, s32 flip) {
    u8 hitA[0x70];
    u8 hitB[0x70];
    HitInit(hitA);
    HitInit(hitB);
    if (fn_801B19C4(*(u8**)(a + 0x8C), 0xB738EABDu, *(u8**)(b + 0x8C), 0xF62113D5u, hitA, hitB, 0, 0, 0 /* leftover stack arg */) == 0) return;

    if (flip != 0) {
        u8* obj = *(u8**)(hitB + 0x68);
        if (obj != NULL && *(s32*)(obj + 0xC) == 0) {
            *(f32*)(hitB + 0x0C) = -*(f32*)(hitB + 0x0C);
            *(f32*)(hitB + 0x10) = -*(f32*)(hitB + 0x10);
            *(f32*)(hitB + 0x14) = -*(f32*)(hitB + 0x14);
        }
    }

    if ((*(u32*)(*(u8**)(b + 0x4) + 0x1C) & 0x10) != 0) {
        u8* recB = (*(VFnP*)(*(u8**)b + 0xB8))(b);
        if (recB != NULL) {
            u8 bit = *(u8*)(recB + 0x40);
            if ((*(u32*)(*(u8**)(a + 0x4) + 0x1C) & 2) != 0) {
                u8* recA = (*(VFnP*)(*(u8**)a + 0xB8))(a);
                if (recA != NULL) {
                    MaskSet(recA, bit);
                    if (MaskTest(recA + 0x20, bit)) return;
                }
            }
        }
    }
    (*(VFnPP*)(*(u8**)a + 0x64))(a, hitA);
    (*(VFnPP*)(*(u8**)b + 0x68))(b, hitB);
}

// 0x8005F4F8
// Removes the entry with id `id` from the 4-slot list at +0xB04 (shifting later slots down and
// clearing the vacated slot's float at +0xB80), then does the same for the 8-slot list at
// +0xA04 (clearing +0xB00).
void fn_8005F4F8(u8* self, u32 id) {
    for (s32 i = 0; i < 4; i++) {
        if (*(u32*)(self + 0xB04 + i * 0x20) != id) continue;
        for (s32 j = i; j < 3; j++) SlotCopy(self + 0xB04 + j * 0x20, self + 0xB24 + j * 0x20);
        *(f32*)(self + 0xB80) = 0.0f;
        for (s32 k = 0; k < 8; k++) {
            if (*(u32*)(self + 0xA04 + k * 0x20) != id) continue;
            for (s32 j = k; j < 7; j++) SlotCopy(self + 0xA04 + j * 0x20, self + 0xA24 + j * 0x20);
            *(f32*)(self + 0xB00) = 0.0f;
            return;
        }
        return;
    }
}

}  // extern "C"
