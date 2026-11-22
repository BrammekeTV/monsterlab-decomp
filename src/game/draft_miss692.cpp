// Small functions batch (miss692): stick/timer update, ring-list lookups, event helpers,
// AI-state reset helpers and the 0x800B59E8 / 0x800B5B54 list builders.
#include "types.h"

struct ScopedLock { u8 pad[4]; };

extern "C++" void* gEventMgr;   // 0x805FB6C8

extern "C" {
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void* CharacterFind(s32 id);                                    // 0x800E876C
s32   AnimSetFindClip(void* set, s32 anim);                     // 0x801AC2FC
void  PropPlayAnim(void* prop, int a, int b, BOOL once);        // 0x80069350
u32   RandomU32(void);                                          // 0x80143460
s32   RaySceneTest(void* ray);                                  // 0x80070458
void  ScopedLockCtor(ScopedLock* l);                           // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                // 0x801F53B0
void  StickRead(s32 channel, f32* x, f32* y);                  // 0x80147704
void* UIWidgetFindChild(void* widget, u32 hash);               // 0x80199AEC
void  fn_8006AADC(u8* p, f32 delta);                           // 0x8006AADC
void  fn_80070460(u8* p);                                      // 0x80070460
void* fn_80070F4C(void);                                       // 0x80070F4C
void  fn_800712BC(u8* p, s32 a, s32 b);                        // 0x800712BC
s32   fn_80097064(f32* pos);                                   // 0x80097064
void  fn_800980C8(u8* obj, s32 mode);                          // 0x800980C8
void  fn_800995AC(u8* p, u8* b);                               // 0x800995AC (asm passes a pointer)
void  fn_80099874(u8* p, u8* effect);                          // 0x80099874 (asm passes a pointer)
void  fn_800998E4(u8* obj, f32 angle);                         // 0x800998E4
void* fn_8009A47C(u8* p);                                      // 0x8009A47C
void  fn_800A7D88(void* obj, u32 hash);                        // 0x800A7D88
s32   fn_800C2628(s32 a, s32 b);                               // 0x800C2628
void  fn_800C5F44(u8* p);                                      // 0x800C5F44
u8*   fn_800E4BFC(u8* p);                                      // 0x800E4BFC
void  fn_800E4CB0(u8* p);                                      // 0x800E4CB0
u8*   fn_800E4D7C(u8* p);                                      // 0x800E4D7C (asm passes a pointer; decl in small_800Ec.cpp says u32)
s32   fn_801A403C(u8* a3, f32 f1);                             // 0x801A403C
s32   fn_801A4050(u8* a3, f32 f1);                             // 0x801A4050
void  fn_801B7020(u8* p);                                      // 0x801B7020
void  fn_800B5CB0(u8* p);                                      // 0x800B5CB0

extern f32 lbl_805FDB28;
extern f32 lbl_805FDB38;
extern f32 lbl_805FDB6C;
extern f32 lbl_805FDB70;
extern f32 lbl_805FDB74;
extern f32 lbl_805FDB90;
extern f32 lbl_805FDC50;
extern f32 lbl_805FDC68;
extern f32 lbl_805FDD7C;
extern f32 lbl_805FDD80;
extern f32 lbl_805FDF18;
extern f32 lbl_805FE03C;
extern const f32 lbl_803462E8[];
extern f32 lbl_803463F8[];
extern f32 lbl_80346418[];
extern f32 lbl_80346428[];
extern s32 lbl_80346248[];
extern s32 lbl_80346A68[];
extern u8 lbl_803C4298[];
}

extern "C" {

// Hash of UI widget "astral_rift_meter".
static const u32 kHashAstralRiftMeter = 0xEB3D5086u;   // "astral_rift_meter"
// Hash of character "astral_intro".
static const u32 kHashAstralIntro = 0x6D235EC6u;       // "astral_intro"

// Shared stick/timer step: runs when the lock-flag at +0x270 is set (stick held),
// re-arms the timers and clears the active state when no target is set.
// 0x8009677C
void fn_8009677C(u8* p, f32 dt) {
    u8* sub = *(u8**)(p + 0x90);
    s32 ch;
    f32 stickX;
    f32 stickY;
    u8* o;
    ScopedLock lk;

    (void)dt;
    if (*(s32*)(sub + 0x268) != 0) {
        *(u8*)(p + 0x354) = 0;
        *(f32*)(p + 0x8C) = lbl_805FDB28;
        o = *(u8**)(p + 0x98);
        ScopedLockCtor(&lk);
        (*(void (**)(void*, s32))(*(u8**)o + 0x18))(o, 1);
        ScopedLockDtor(&lk, -1);
        return;
    }
    // Stick values are read but not used by this step.
    stickX = lbl_805FDB28;
    stickY = lbl_805FDB28;
    ch = *(s32*)(lbl_803C4298 + 0x38);
    StickRead(ch * 16 + 0xE, &stickX, &stickY);
    if (*(u8*)(p + 0x270) != 0) {
        f32 v = *(f32*)(p + 0x274);
        if (v > lbl_805FDB28) {
            *(f32*)(p + 0x1E8) = v;
        }
    }
    if (*(u8*)(p + 0x270) != 0 && *(u8**)(p + 0x1D8) != NULL) {
        *(f32*)(p + 0x374) = lbl_805FDB38;
        *(f32*)(*(u8**)(p + 0x90) + 0x274) = lbl_805FDB90;
    }
    if (*(u8**)(p + 0x1D8) == NULL) {
        o = *(u8**)(p + 0x98);
        ScopedLockCtor(&lk);
        (*(void (**)(void*, s32))(*(u8**)o + 0x18))(o, 1);
        ScopedLockDtor(&lk, -1);
        *(u8*)(p + 0x354) = 0;
        *(f32*)(p + 0x8C) = lbl_805FDB28;
    }
}

// Looks up the entry for the index at +0x6C in the table at +0x78 (count +0x74, span +0x70).
// Stores the entry pointer at +0x7C and returns the entry value, or 0 when out of range.
static s32 RingLookupEntry(u8* p) {
    s32 idx = *(s32*)(p + 0x6C);
    s32 val = 0;

    if (idx >= 0 && *(s32*)(p + 0x70) >= 0) {
        s32 len = *(s32*)(p + 0x74);
        s32 hi = *(s32*)(p + 0x70);
        s32 pos = idx;
        s32 inRange = 0;

        if (idx < len) {
            if (hi < idx) {
                if (idx <= hi) {
                    pos = idx + len;
                }
                hi += len;
            }
            inRange = (idx <= pos && pos <= hi);
        }
        if (inRange) {
            u8* entry = *(u8**)(p + 0x78) + idx * 4;
            *(u8**)(p + 0x7C) = entry;
            if (entry != NULL) {
                val = *(s32*)entry;
            }
        } else {
            *(u8**)(p + 0x7C) = NULL;
        }
    }
    return val;
}

// Returns 1 when the span length from +0x6C fits the limit at +0xD0, else 0.
// 0x8009BFE4
s32 fn_8009BFE4(u8* p) {
    s32 val = RingLookupEntry(p);
    s32 last;
    s32 count = 0;

    if (fn_800C2628(*(s32*)(p + 0x60), val) != 0) {
        return 0;
    }
    if (*(s32*)(p + 0x6C) >= 0 && *(s32*)(p + 0x70) >= 0) {
        last = *(s32*)(p + 0x70);
        if (last < *(s32*)(p + 0x6C)) {
            last += *(s32*)(p + 0x74);
        }
        count = (last - *(s32*)(p + 0x6C)) + 1;
    }
    return (count <= *(s32*)(p + 0xD0));
}

// 0x8009D8D8
void fn_8009D8D8(u8* p) {
    u32 r;
    s32 idx1;
    s32 idx2;
    f32 scale;
    u8* t;
    u8* set;
    s32 clip;
    u8* obj;

    fn_800C5F44(p);
    r = RandomU32();
    idx1 = RaySceneTest((void*)(uintptr_t)r);
    scale = (lbl_805FDD7C * (f32)r) * lbl_80346418[idx1];
    idx2 = RaySceneTest((void*)(uintptr_t)idx1);
    *(f32*)(p + 0x274) = lbl_803463F8[idx2] + scale;
    if (RandomU32() & 1) {
        *(f32*)(p + 0x274) = *(f32*)(p + 0x274) * lbl_805FDD80;
    }
    t = *(u8**)(p + 0x9C);
    *(u8*)(p + 0x278) = 0;
    *(s32*)(p + 0x15C) &= ~1;
    if (t != NULL) {
        set = *(u8**)(t + 0x48);
        if (set != NULL) {
            clip = AnimSetFindClip(set, 0x548FB9CB);
            if (clip >= 0) {
                PropPlayAnim(p, clip, 1, FALSE);
                *(u8*)(p + 0x278) = 1;
            }
        }
    }
    obj = (*(u8* (**)(void*))(*(u8**)p + 0x10))(p);
    if (obj != NULL) {
        *(f32*)(obj + 0xB0) = *(f32*)(obj + 0xB0) * lbl_80346428[RaySceneTest(obj)];
    }
}

// 0x8009F730
s32 fn_8009F730(u8* p) {
    s32 val = RingLookupEntry(p);
    s32 last;
    s32 arg = 1;
    s32 cnt = 0;

    if (fn_800C2628(*(s32*)(p + 0x60), val) != 0) {
        return 0;
    }
    if (*(s32*)(p + 0x6C) >= 0 && *(s32*)(p + 0x70) >= 0) {
        last = *(s32*)(p + 0x70);
        if (last < *(s32*)(p + 0x6C)) {
            last += *(s32*)(p + 0x74);
        }
        arg = last - *(s32*)(p + 0x6C);
        cnt = arg + 1;
    }
    if (cnt > lbl_80346A68[RaySceneTest((void*)(intptr_t)arg)]) {
        return 0;
    }
    return 1;
}

// Sends event 0xC608C842 to obj. The guard bit 0 of the block at obj+4 (+0x20)
// is set while the event is sent, then cleared again when this call set it.
static void SendGuardedEvent(u8* obj) {
    u8* sub = *(u8**)(obj + 0x4);
    s32 flags = *(s32*)(sub + 0x20);

    if (!(flags & 1)) {
        *(s32*)(sub + 0x20) = flags | 1;
        EventSend(gEventMgr, obj, NULL, 0xC608C842u, 0, 0, 1, 0);
        sub = *(u8**)(obj + 0x4);
        *(s32*)(sub + 0x20) &= ~1;
    } else {
        EventSend(gEventMgr, obj, NULL, 0xC608C842u, 0, 0, 1, 0);
    }
}

// 0x800A9520
void fn_800A9520(u8* p) {
    *(f32*)(p + 0x8C) = lbl_805FDF18;
    SendGuardedEvent(*(u8**)(p + 0x90));
    SendGuardedEvent(*(u8**)(p + 0x94));
}

// 0x800A9690
void fn_800A9690(u8* p) {
    *(f32*)(p + 0x8C) = lbl_805FDF18;
    SendGuardedEvent(*(u8**)(p + 0x90));
    SendGuardedEvent(*(u8**)(p + 0x94));
}

// 0x800ADC7C
void fn_800ADC7C(u8* p, f32 dt) {
    u8* node = *(u8**)(p + 0xB0);
    u8* a;
    u8* b;

    while (node != NULL) {
        a = *(u8**)(node + 0x4);
        if (a != NULL) {
            b = *(u8**)(a + 0x4);
            *(s32*)(b + 0x20) |= 1;
        }
        node = *(u8**)(node + 0x8);
    }
    fn_8006AADC(p, dt);
    (*(void (**)(void*, f32))(*(u8**)p + 0x74))(p, dt);
    node = *(u8**)(p + 0xB0);
    while (node != NULL) {
        a = *(u8**)(node + 0x4);
        if (a != NULL) {
            b = *(u8**)(a + 0x4);
            *(s32*)(b + 0x20) &= ~1;
        }
        node = *(u8**)(node + 0x8);
    }
    fn_801B7020(p);
    a = *(u8**)(p + 0x26C);
    if (a != NULL) {
        fn_801B7020(a);
    }
    if (*(s32*)(p + 0x264) == 2) {
        *(f32*)(p + 0x360) = *(f32*)(p + 0x360) + dt;
    } else {
        *(f32*)(p + 0x360) = lbl_805FE03C;
    }
    if (*(u8*)(p + 0x364) != 0) {
        f32 rem = *(f32*)(p + 0x36C) - dt;
        *(f32*)(p + 0x36C) = rem;
        if (rem <= lbl_805FE03C) {
            *(u8*)(p + 0x364) = 0;
            fn_800A7D88(p, 0x8EAA2203u);
        }
    }
}

// State 1: clears the code list at +0x4..+0x28 and fills it with random item codes
// (count from a lookup table). State 2: counts +0x2C down by dt.
// 0x800B59E8
void fn_800B59E8(u8* p, f32 dt) {
    s32 kind;
    s32 total;
    s32 i;
    u8* cur;

    fn_800B5CB0(p);
    kind = *(s32*)p;
    if (kind == 2) {
        *(f32*)(p + 0x2C) = *(f32*)(p + 0x2C) - dt;
        return;
    }
    if (kind != 1) {
        return;
    }
    *(s32*)p = 2;
    *(s32*)(p + 0x30) = 0;
    for (i = 0x4; i <= 0x28; i += 4) {
        *(s32*)(p + i) = 0;
    }
    total = lbl_80346248[RaySceneTest((void*)2)];
    cur = p;
    for (i = 0; i < total; i++) {
        s32 r = (s32)(RandomU32() & 3);
        s32 code = 0;
        switch (r) {
        case 0: code = 0x1089; break;
        case 1: code = 0x108A; break;
        case 2: code = 0x109C; break;
        case 3: code = 0x109B; break;
        }
        cur += 4;
        *(s32*)cur = code;
    }
}

// Converts the code list at +0x4.. (after the count at +0x30) to a text string at +0x34.
// Each code writes one letter at +0x34 + 2k followed by 0x3E (or 0 for the last).
// 0x800B5B54
void* fn_800B5B54(u8* p) {
    s32 count;
    s32 off = 0;
    s32 i;

    for (i = 0; i < 0x20; i++) {
        *(u8*)(p + 0x34 + i) = 0x20;
    }
    count = *(s32*)(p + 0x30);
    if (count < 0xA) {
        u8* entry = p + count * 4;
        s32 wOff = count * 2;
        s32 n;
        for (n = 0xA - count; n > 0; n--) {
            s32 code = *(s32*)(entry + 4);
            u8 ch;
            off = wOff;
            if (code == 0x1089) {
                ch = 0x41;
            } else if (code == 0x108A) {
                ch = 0x42;
            } else if (code == 0x109C) {
                ch = 0x43;
            } else if (code == 0x109B) {
                ch = 0x5A;
            } else {
                *(u8*)(p + wOff + 0x35) = 0;
                return p + 0x34;
            }
            *(u8*)(p + wOff + 0x34) = ch;
            *(u8*)(p + wOff + 0x35) = 0x3E;
            wOff += 2;
            entry += 4;
        }
    }
    *(u8*)(p + off + 0x35) = 0;
    return p + 0x34;
}

// Refreshes the HUD widget, then resets the child block at +0x54 and the character state.
// 0x8009A65C
void fn_8009A65C(u8* p) {
    f32 f;
    void* hud;
    u8* w;
    u8* o;
    u8* q;
    u8* o3;
    ScopedLock lk;

    fn_80070460(p);
    f = *(f32*)(p + 0x8C);
    fn_800712BC(p, (s32)f, (s32)(f <= lbl_805FDC68));
    hud = fn_80070F4C();
    if (hud != NULL) {
        w = (u8*)UIWidgetFindChild(hud, kHashAstralRiftMeter);
        fn_801A4050(w, lbl_803462E8[RaySceneTest((void*)(intptr_t)fn_801A403C(w, lbl_805FDC50))]);
    }
    o = (u8*)fn_8009A47C(p);
    *(u8*)(o + 0x54) = 0;
    q = fn_800E4D7C(o);
    *(u8*)(q + 0x54) = 1;
    o = fn_800E4BFC(q);
    (*(void (**)(void*))(*(u8**)o + 0x10))(o);
    fn_800E4CB0(q);
    o = fn_800E4BFC(q);
    *(s32*)(o + 0x15C) &= ~1;
    o3 = (u8*)CharacterFind((s32)kHashAstralIntro);
    ScopedLockCtor(&lk);
    (*(void (**)(void*, s32))(*(u8**)o3 + 0x14))(o3, 1);
    ScopedLockDtor(&lk, -1);
    (*(void (**)(void*, u32))(*(u8**)o3 + 0x84))(o3, 0x3B03C373u);
    (*(void (**)(void*, f32))(*(u8**)o3 + 0x90))(o3, lbl_805FDC50);
}

// 0x80096BDC
void fn_80096BDC(u8* p) {
    f32 pos[3];
    u32 n;
    u32 idx;
    u8* e;
    u8* src;
    u8* cur;
    s32 slot;
    u32 t;
    u32 sgn;
    u32 r0;
    u32 bucket;

    n = *(u32*)(p + 0x1B4);
    do {
        idx = RandomU32() % n;
        e = *(u8**)(p + 0x9C + idx * 4);
    } while (e == NULL || e == *(u8**)(p + 0x1D8) || *(u8*)(e + 0x26E) == 0);

    src = *(u8**)(e + 0x4);
    pos[0] = *(f32*)(src + 0x0);
    pos[1] = *(f32*)(src + 0x4);
    pos[2] = *(f32*)(src + 0x8);
    slot = fn_80097064(pos);
    fn_800998E4(*(u8**)(p + 0x90), lbl_805FDB6C + (lbl_805FDB70 * (f32)slot) * lbl_805FDB74);

    // Table index (slot + 4) mod 8, computed with the sign fixup the compiler emitted.
    t = (u32)(slot + 4);
    sgn = t >> 31;
    r0 = (t << 29) - sgn;
    bucket = ((r0 << 3) | (r0 >> 29)) + sgn;
    fn_80099874(*(u8**)(p + 0x90), *(u8**)(p + 0x194 + bucket * 4));

    fn_800995AC(*(u8**)(p + 0x90), e);
    cur = *(u8**)(p + 0x1D8);
    if (cur != NULL) {
        fn_800980C8(cur, 0);
        *(u8**)(p + 0x1DC) = *(u8**)(p + 0x1D8);
    }
    *(u8**)(p + 0x1D8) = NULL;
    *(u8*)(p + 0x370) = 0;
}

}  // extern "C"
