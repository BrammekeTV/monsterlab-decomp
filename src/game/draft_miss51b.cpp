// Small game-logic functions from 0x80053558-0x80074DA0 (effect/anim state resets, zone and
// distance checks, event dispatch, camera hookup). Verified against the asm.
#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C++" u8 gWorldState[];          // 0x803E6D00
extern "C++" void* gEventMgr;           // 0x805FB6C8
extern "C++" s32 gZoneResourceSize;     // 0x805FB94C
extern "C++" u8 lbl_8047C7AC[];         // 0x8047C7AC
extern "C++" u8 lbl_8047C640[];         // 0x8047C640 (camera)
extern "C++" u8 lbl_8035A760[];         // 0x8035A760 (0x70-byte static block)
extern "C++" s32 lbl_805FB120;          // 0x805FB120

extern "C" {
void* memset(void* p, int c, unsigned long n);
void operator_delete(void* p);
void CameraSetFov(void* camera, f32 fov);                                    // 0x801D8ACC
void CharacterResetGroundState(u8* c, s32 a, s32 b);                         // 0x8005BAD4
void EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);   // 0x801BA2FC
s32 TriggerDoAction(void* action, void* a1, void* a2);                      // 0x801E0054
s32  fn_8001AFB4(u8* p, s32 a);                                              // 0x8001AFB4
s32 fn_801C3E94(u8* p);                                                      // 0x801C3E94
void fn_8020A398(s32 handle);                                                // 0x8020A398
void fn_801B4520(u8* p);                                                     // 0x801B4520
s32 fn_801B4C6C(u8* obj, s32 a);                                             // 0x801B4C6C
u32 fn_801EC880(u8* p, u32 hash);                                            // 0x801EC880
void fn_801ECFA0(u8* p, s32 idx);                                            // 0x801ECFA0
void fn_801EC080(u8* a, u8* b);                                              // 0x801EC080
s32 fn_801ED538(u8* p);                                                      // 0x801ED538
void* fn_801B7198(void* anim, s32 a);                                        // 0x801B7198
u8* fn_801C668C(u8* key);                                                    // 0x801C668C
void fn_801ED554(u8* out, u8* color);                                        // 0x801ED554
void* fn_801B50FC(u8* p);                                                    // 0x801B50FC
void* fn_80060A28(u8* p);                                                     // 0x80060A28
void fn_8005FD4C(u8* p, void* entry);                                        // 0x8005FD4C
s32 fn_80062EA8(u8* obj, s32 a);                                             // 0x80062EA8
s32 fn_8006FBB8(void);                                                       // 0x8006FBB8
s32 fn_80021770(void);                                                       // 0x80021770
s32 fn_8012EB74(u8* world, u32 arg, s32 a, s32 b, s32 c, s32 d);             // 0x8012EB74
void fn_80062D38(u8* obj, s32 unused);                                       // 0x80062D38
}

// Virtual method in vtable slot 0xB8 that returns a u8* (the draft's unkB8).
typedef u8* (*VFnB8)(u8* self);

extern "C" {

// 0x80053558
void fn_80053558(u8* arg0) {
    u8* list = *(u8**)(arg0 + 0x9C);
    u8* p = arg0;
    s32 i = 0;
    while (i < *(s32*)(list + 0x114)) {
        if (*(s32*)(p + 0x81C) != 0) {
            fn_801C3E94(lbl_8047C7AC);
            *(s32*)(p + 0x81C) = 0;
        }
        p += 4;
        i += 1;
    }
    fn_8001AFB4(arg0 + 0x170, 0);
    s32 h = *(s32*)(arg0 + 0x830);
    if (h != -1) {
        fn_8020A398(h);
        *(s32*)(arg0 + 0x830) = -1;
    }
    fn_801B4520(arg0);
}

// 0x800596F0
void fn_800596F0(u8* arg0, s32 arg1) {
    s32 idx = (s32)fn_801EC880(arg0 + 0x94, 0xCC70BCDEu);
    if (idx >= 0) {
        fn_801ECFA0(arg0 + 0x94, idx);
    }
    if (*(u32*)(*(u8**)(arg0 + 0x4) + 0x20) & 1) {
        PlayAttachedParticleEffectAtEmitter(arg0, 0x9AD02496u, 0xCC70BCDEu);
    }
    if (arg1 != 0) {
        *(s32*)(arg0 + 0x9FC) = 1;
    }
}

// 0x8005CB40
void fn_8005CB40(u8* arg0, f32 farg0, s32 arg1) {
    // farg0 (f1) is forwarded to fn_80062D38 by the original; that function does not read it.
    if (*(u32*)(*(u8**)(arg0 + 0x4) + 0x20) & 1) {
        fn_801EC080(arg0 + 0x94, arg0);
        fn_80062D38(arg0, 0);
    }
}

// 0x80060198
void fn_80060198(u8* arg0, s32 arg1, f32 farg0) {
    u8 entry[0x20];
    memset(entry, 0, 0x20);
    *(u8*)(entry + 0x8) = 0xFF;
    *(s32*)(entry + 0x0) = arg1;
    *(s32*)(entry + 0x10) = 0;
    *(f32*)(entry + 0x1C) = farg0;
    fn_8005FD4C(arg0, entry);
}

// 0x8006023C
void fn_8006023C(u8* arg0, s32 arg1, s32 arg2, f32 farg0) {
    u8 entry[0x20];
    memset(entry, 0, 0x20);
    *(u8*)(entry + 0x8) = 0xFF;
    *(s32*)(entry + 0x0) = arg2;
    *(s32*)(entry + 0x10) = arg1;
    *(s32*)(entry + 0x14) = arg1;
    *(f32*)(entry + 0x1C) = farg0;
    fn_8005FD4C(arg0, entry);
}

// 0x800602E8
void fn_800602E8(u8* arg0, u8* color, f32 farg0) {
    u8 entry[0x20];
    u8 tmp[4];
    memset(entry, 0, 0x20);
    *(u8*)(entry + 0x8) = 0xFF;
    *(s32*)(entry + 0x0) = 0x7A6B66E7;
    *(f32*)(entry + 0x1C) = farg0;
    fn_801ED554(tmp, color);
    *(u8*)(entry + 0xC) = tmp[0];
    *(u8*)(entry + 0xD) = tmp[1];
    *(u16*)(entry + 0xE) = *(u16*)(tmp + 2);
    fn_8005FD4C(arg0, entry);
}

// 0x80060920
void fn_80060920(u8* arg0, f32 farg0, f32 farg1) {
    u8 entry[0x20];
    memset(entry, 0, 0x20);
    *(u8*)(entry + 0x8) = 0xFF;
    *(s32*)(entry + 0x0) = (s32)0xCF423616u;
    *(f32*)(entry + 0x18) = farg0;
    *(f32*)(entry + 0x1C) = farg1;
    *(void**)(entry + 0x10) = *(void**)(arg0 + 0xA14);
    if (*(void**)(entry + 0x10) == NULL) {
        *(void**)(entry + 0x10) = fn_80060A28(arg0);
    }
    fn_8005FD4C(arg0, entry);
}

// 0x80061770
s32 fn_80061770(u8* arg0) {
    s32 handle = fn_801ED538(arg0 + 0xBD0);
    u8* anim = (u8*)fn_801B7198(&handle, 0);
    u8* pt = fn_801C668C(arg0 + (*(s32*)(arg0 + 0xBCC)) * 4 + 0xBBC);
    if (anim != NULL && pt != NULL) {
        f32 dx = *(f32*)(anim + 0x18) - *(f32*)(pt + 0x4);
        f32 dy = *(f32*)(anim + 0x1C) - *(f32*)(pt + 0x8);
        f32 dz = *(f32*)(anim + 0x20) - *(f32*)(pt + 0xC);
        f32 distSq = (dx * dx + dy * dy) + dz * dz;
        if (distSq > 9.0f) {
            return 0;
        }
    }
    return 1;
}

// 0x800619A4
void fn_800619A4(u8* arg0, f32 farg0, f32 farg1) {
    f32 oldScale = *(f32*)(arg0 + 0x84C);
    u8* src = (u8*)fn_801B50FC(arg0);
    *(f32*)(arg0 + 0x844) = *(f32*)(src + 0xC) * farg0;
    *(f32*)(arg0 + 0x848) = *(f32*)(src + 0x10) * farg0;
    *(f32*)(arg0 + 0x84C) = *(f32*)(src + 0x14) * farg0;
    if (farg1 > 0.0f) {
        *(f32*)(arg0 + 0x84C) = farg1;
        CharacterResetGroundState(arg0, 1, 0);
    } else {
        *(f32*)(arg0 + 0x84C) = oldScale;
    }
}

// 0x800623D4
s32 fn_800623D4(u8* arg0) {
    u32 flags = *(u32*)(arg0 + 0xA08);
    if (flags & 8) {
        u8* ent = *(u8**)(arg0 + 0x4);
        u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4)
                   + (u32)(*(u16*)(ent + 0x30)) * gZoneResourceSize;
        if (zone[0xC0] == 0 || (flags & 4) || (flags & 1)) {
            return (3.0f + *(f32*)(arg0 + 0xB90)) < *(f32*)(ent + 0x8) ? 1 : 0;
        }
    }
    if (flags & 0x10) {
        return (3.0f + *(f32*)(arg0 + 0xB9C)) < *(f32*)(*(u8**)(arg0 + 0x4) + 0x8) ? 1 : 0;
    }
    return 0;
}

// 0x800629E8
s32 fn_800629E8(u8* arg0) {
    if (!(*(u32*)(arg0 + 0xA08) & 8)) {
        return 0;
    }
    u8* ent = *(u8**)(arg0 + 0x4);
    f32 dx = *(f32*)(arg0 + 0xB88) - *(f32*)(ent + 0x0);
    f32 dz = *(f32*)(arg0 + 0xB8C) - *(f32*)(ent + 0x4);
    f32 dy = 0.0f;
    u8* n = (u8*)fn_801B50FC(arg0);
    f32 dot = ((*(f32*)(n + 0x0) * dx) + (*(f32*)(n + 0x4) * dz)) + (*(f32*)(n + 0x8) * dy);
    return !(dot > 0.0f) ? 1 : 0;
}

// 0x80062D38
void fn_80062D38(u8* obj, s32 unused) {
    fn_801B4C6C(obj, 0);
    u8* t = (*(VFnB8*)(*(u8**)obj + 0xB8))(obj);
    if (t != NULL) {
        *(u32*)(t + 0x20) = *(u32*)(t + 0x00);
        *(u32*)(t + 0x24) = *(u32*)(t + 0x04);
        *(u32*)(t + 0x28) = *(u32*)(t + 0x08);
        *(u32*)(t + 0x2C) = *(u32*)(t + 0x0C);
        *(u32*)(t + 0x30) = *(u32*)(t + 0x10);
        *(u32*)(t + 0x34) = *(u32*)(t + 0x14);
        *(u32*)(t + 0x38) = *(u32*)(t + 0x18);
        *(u32*)(t + 0x3C) = *(u32*)(t + 0x1C);
    }
}

// 0x80069638
void fn_80069638(u8* arg0, s32 arg1) {
    // arg1 (r4) is not read here; it is forwarded to fn_80062EA8 unchanged.
    fn_80062EA8(arg0, arg1);
    u8* p = *(u8**)(arg0 + 0x4);
    *(u32*)(p + 0x20) |= 0x8000;
    p = *(u8**)(arg0 + 0x4);
    *(u32*)(p + 0x20) |= 0x4000;
    u8* a = *(u8**)(arg0 + 0x9C);
    if (a != NULL && ((*(u32*)(a + 0x80) >> 27) & 0xF) != 0) {
        TriggerDoAction(a + 0x80, arg0, NULL);
    }
    if (((*(u32*)(arg0 + 0x50) >> 27) & 0xF) != 0) {
        TriggerDoAction(arg0 + 0x50, arg0, NULL);
    }
}

// 0x8006E99C
s32 fn_8006E99C(u8* arg0, s32 arg1, s32 hash) {
    // arg1 (r4) is not read. Hashes 0x7B196480 and 0x16D9A0E9 select bit 5 of +0x1B0.
    s32 var = 0;
    if (hash == 0x7B196480 || hash == 0x16D9A0E9) {
        var = (*(u32*)(arg0 + 0x1B0) >> 5) & 1;
    }
    return var;
}

// 0x8006EC6C
void fn_8006EC6C(u8* arg0, s32 hash) {
    s32 val = 0;
    switch ((u32)hash) {
    case 0x76AB8424u:
        val = *(s32*)(arg0 + 0x168);
        break;
    case 0xD80BD679u:
        val = *(s32*)(arg0 + 0x16C);
        break;
    case 0xCCFAC817u:
        val = *(s32*)(arg0 + 0x168);
        break;
    }
    if (val == 0) {
        return;
    }
    for (s32 i = 0; i < 4; i++) {
        u32 ev = *(u32*)(arg0 + 0x184 + i * 4);
        if (ev != 0) {
            if (ev + 0x69820000u == 0x2DB3u) {
                fn_8012EB74(gWorldState, (u32)val, 0, 0, 0, 0);
            } else {
                EventSendTo(gEventMgr, ev, arg0, (u32)val, 0, 0);
            }
        }
    }
}

// 0x8006F5B8
s32 fn_8006F5B8(u8* arg0) {
    if (arg0 == NULL || !(*(u32*)(*(u8**)(arg0 + 0x4) + 0x1C) & 0x100)) {
        return 0;
    }
    return *(f32*)(arg0 + 0x164) >= *(f32*)(arg0 + 0x158);
}

// 0x8006FAF4
void fn_8006FAF4(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)(arg0 + 0x4) = lbl_8035A760;
        CameraSetFov(lbl_8047C640, *(f32*)(arg0 + 0x70));
        lbl_805FB120 = 0;
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
}

// 0x80074D80
void fn_80074D80(void) {
    fn_80021770();
    fn_8006FBB8();
}

}  // extern "C"
