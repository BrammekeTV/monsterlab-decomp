#include "types.h"

// Prototypes copied from their existing definitions / other files.
extern "C" {
void operator_delete(void* p);                                    // 0x801D426C
void BattleHudSetTurn(s32 side, BOOL on);                         // 0x80048BF4
void fn_8004182C(f32 f1);                                         // 0x8004182C
void fn_8020F290(u8* p);                                          // 0x8020F290
u8*  fn_800216B8(u8* p, s32 del);                                 // 0x800216B8
u8*  fn_80032F7C(u8* p, s32 del);                                 // 0x80032F7C
void* fn_800324DC(u8* p, s32 del);                                // 0x800324DC
void fn_8032F358(void* obj, void* cb);                            // 0x8032F358
void __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count);  // 0x8032F2E0
u8*  ObjRefDtor(u8* ref, s32 flags);                              // (prototype from src/game/draft_800F.cpp)
u8*  AnimSetGetClip(void* set, s32 index);                        // 0x801AC164
s32  fn_801AC51C(u8* p, u8* obj, u8* target);                     // 0x801AC51C
void fn_80070FD8(u8* obj);                                        // 0x80070FD8
ScriptValue SetPropState(u8* prop, u32 state, s32 a2);            // include/game/script_callbacks.h:1302
u8*  fn_80062CE0(u8* p, s32 del);                                 // 0x80062CE0 (base dtor)
void fn_8009C4EC(u8* p);                                // 0x8009C4EC
u8*  fn_801F9598(u8* p, s32 del);                                // 0x801F9598
u8*  fn_801F6238(u8* p, s32 del);                                // 0x801F6238
u8*  fn_800D4B74(u8* p, s32 del);                                 // 0x800D4B74
u8*  fn_800D4BE8(u8* p, s32 del);                                 // 0x800D4BE8
void TextureRelease(void* tex);                                   // 0x80184028
void fn_801A904C(u8* p);                                          // 0x801A904C
void fn_801C83AC(u8* mgr, u8* item);                              // 0x801C83AC
void fn_8018D4CC(void* p);                                        // 0x8018D4CC
void fn_8018D2B8(const char* fmt, ...);                           // 0x8018D2B8
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190

extern u8 lbl_80357C60[];          // 0x80357C60 (vtable)
extern u8 lbl_8035B8F0[];          // 0x8035B8F0 (vtable)
extern u8 lbl_803821D0[];          // 0x803821D0 (vtable)
extern u8 lbl_8047C760[];          // 0x8047C760 (effect manager)
extern u8 lbl_80347D88[];          // 0x80347D88 (prop state table)
extern const char lbl_80382314[];  // 0x80382314 (debug string)
extern f32 lbl_805FB0C8;           // 0x805FB0C8
extern f32 lbl_805FD200;           // 0x805FD200
extern f32 lbl_805FD220;           // 0x805FD220
extern f32 lbl_805FD5F0;           // 0x805FD5F0
extern u8 lbl_805FB1C0;            // 0x805FB1C0
extern u8 lbl_805FB1C1;            // 0x805FB1C1
extern s32 lbl_805FB1C4;           // 0x805FB1C4
}

extern "C++" u8 lbl_8035CBA8[];    // 0x8035CBA8 (vtable)
extern "C++" void* gEventMgr;      // 0x805FB6C8

extern "C" {

typedef void (*VoidFn1)(u8*);
typedef u8* (*PtrFn1)(u8*);
typedef f32 (*F32Fn1)(u8*);
typedef void (*VFn8)(u8*, s32, s32, s32, s32, s32, s32, f32);

// 0x800D44F4
// Deleting destructor: destroys the two embedded arrays, base dtor ObjRefDtor(-1), frees if del > 0.
u8* fn_800D44F4(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        u8* p = arg0 + 0x575C;
        if (p != NULL) {
            __destroy_arr(p, (void (*)(void*, s32))fn_800D4B74, 0xDC, 0xFF);
        }
        p = arg0 + 0x428;
        if (p != NULL) {
            __destroy_arr(p, (void (*)(void*, s32))fn_800D4BE8, 0x428, 0x14);
        }
        ObjRefDtor(arg0, -1);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x80196F1C
// Deleting destructor: sets the font vtable, drops the texture reference held at +0x8, frees if del > 0.
void* fn_80196F1C(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        u8* ref = *(u8**)(arg0 + 0x8);
        *(void**)arg0 = lbl_803821D0;
        if (ref != NULL) {
            u8* tex = *(u8**)(ref + 0x4);
            s32 rc = *(s32*)(tex + 0x24) - 1;
            *(s32*)(tex + 0x24) = rc;
            if (rc == 0) {
                TextureRelease(tex);
            }
            *(void**)(ref + 0x4) = NULL;
        }
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x80197D04
// Prints a debug string, then calls virtual slot 0x8 of arg0 with the clip-less argument set
// (0x7FFFFFFF in the r5 slot, the other arguments forwarded from the caller).
void UIFont_DrawClippedString(u8* arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, f32 farg0) {
    (void)arg2;  // r5 is overwritten with 0x7FFFFFFF before the virtual call
    fn_8018D2B8(lbl_80382314);
    (*(VFn8*)(*(u8**)arg0 + 0x8))(arg0, arg1, 0x7FFFFFFF, arg3, arg4, arg5, arg6, farg0);
}

// 0x801A85C0
// Clears the per-item back pointer (+0x74) of every entry in the list at +0x178, frees the list
// through fn_801C83AC, then calls fn_8018D4CC on the object itself.
s32 fn_801A85C0(u8* arg0) {
    fn_801A904C(arg0);
    if (*(u8**)(arg0 + 0x178) != NULL) {
        for (s32 i = 0; i < *(s32*)(*(u8**)(arg0 + 0x178) + 0xC); i++) {
            u8* item = *(u8**)(*(u8**)(arg0 + 0x178) + i * 4 + 0x14);
            if (item != NULL) {
                *(s32*)(item + 0x74) = 0;
            }
        }
    }
    fn_801C83AC(lbl_8047C760, *(u8**)(arg0 + 0x178));
    *(void**)(arg0 + 0x178) = NULL;
    fn_8018D4CC(arg0);
    return 0;
}

// 0x801A8910
// Same list walk as fn_801A85C0, then frees the list and sets the status word at +0x140 to -1.
s32 fn_801A8910(u8* arg0) {
    fn_801A904C(arg0);
    if (*(u8**)(arg0 + 0x178) != NULL) {
        for (s32 i = 0; i < *(s32*)(*(u8**)(arg0 + 0x178) + 0xC); i++) {
            u8* item = *(u8**)(*(u8**)(arg0 + 0x178) + i * 4 + 0x14);
            if (item != NULL) {
                *(s32*)(item + 0x74) = 0;
            }
        }
    }
    fn_801C83AC(lbl_8047C760, *(u8**)(arg0 + 0x178));
    *(void**)(arg0 + 0x178) = NULL;
    *(s32*)(arg0 + 0x140) = -1;
    return 0;
}

// 0x800316BC
// Deleting destructor for the sub-object: sets its vtable at +4, clears two counters, detaches
// the callbacks, then base dtor fn_800216B8 and frees if del > 0.
u8* fn_800316BC(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)(arg0 + 0x4) = lbl_80357C60;
        *(s32*)(arg0 + 0x90) = 0;
        *(s32*)(arg0 + 0xAC) = 0;
        fn_8032F358(*(u8**)(arg0 + 0xC0), (void*)fn_80032F7C);
        fn_800324DC(*(u8**)(arg0 + 0xCC), 1);
        fn_800216B8(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8004A94C
// Resets the battle HUD turn state: keeps the larger of the stored timer and the global maximum,
// zeroes the timers and flags, then calls fn_8020F290.
void fn_8004A94C(u8* arg0) {
    f32 t = *(f32*)(arg0 + 0x3C);
    fn_8004182C(t);
    BattleHudSetTurn(0, 1);
    BattleHudSetTurn(1, 1);
    t = *(f32*)(arg0 + 0x3C);
    if ((lbl_805FB0C8 < t) && (t < lbl_805FD220)) {
        lbl_805FB0C8 = t;
    }
    *(u8*)(arg0 + 0x44) = 0;
    *(f32*)(arg0 + 0x3C) = lbl_805FD200;
    *(f32*)(arg0 + 0x40) = lbl_805FD200;
    lbl_805FB1C0 = 0;
    lbl_805FB1C1 = 0;
    lbl_805FB1C4 = 0;
    fn_8020F290(arg0);
}

// 0x800694A8
// Sets the animation state byte, clears bit 0 of the flags at +0x15C, and if the clip at the
// current index exists, updates it and marks its flag bit 1 when the clip has flag 8.
void fn_800694A8(u8* arg0, s8 arg1) {
    u8* set = *(u8**)(*(u8**)(arg0 + 0x9C) + 0x48);
    if (set != NULL) {
        *(s8*)(*(u8**)(arg0 + 0x4) + 0x39) = arg1;
        *(s32*)(arg0 + 0x15C) = *(s32*)(arg0 + 0x15C) & ~1;
        u8* clip = AnimSetGetClip(set, (s32)arg1);
        if (clip != NULL) {
            fn_801AC51C(clip, *(u8**)(arg0 + 0xA0), arg0);
            if (*(s32*)(clip + 0xC) & 8) {
                u8* obj = *(u8**)(arg0 + 0x4);
                *(s32*)(obj + 0x2C) = *(s32*)(obj + 0x2C) | 2;
            }
        }
    }
}

// 0x8006FC88
// Advances the two animation timers at +0x74 and +0x78 through the vtable at +4, runs fn_80070FD8,
// then adds the frame delta to the accumulator at +0x88.
void fn_8006FC88(u8* arg0, f32 farg0) {
    (*(VoidFn1*)(*(u8**)(arg0 + 0x4) + 0x58))(arg0);
    f32 d = (*(F32Fn1*)(*(u8**)(arg0 + 0x4) + 0x64))(arg0);
    *(f32*)(arg0 + 0x74) = *(f32*)(arg0 + 0x74) + d;
    *(f32*)(arg0 + 0x78) = *(f32*)(arg0 + 0x78) + lbl_805FD5F0;
    fn_80070FD8(arg0);
    *(f32*)(arg0 + 0x88) = *(f32*)(arg0 + 0x88) + farg0;
}

// 0x80084F14
// Decreases the timer at +0xC by farg0; if the linked prop's state is not already 3, sets it to 3,
// applies the prop state from the table, flags bit 0x400000 and copies the value at +0x10.
void fn_80084F14(u8* arg0, f32 farg0) {
    *(f32*)(arg0 + 0xC) = *(f32*)(arg0 + 0xC) - farg0;
    u8* obj = *(u8**)(arg0 + 0x4);
    u8* temp_r31 = *(u8**)(*(u8**)(obj + 0x1F4) + 0x264);
    if ((temp_r31 != NULL) && (*(s32*)(temp_r31 + 0x264) != 3)) {
        *(s32*)(temp_r31 + 0x264) = 3;
        SetPropState(temp_r31, *(u32*)(lbl_80347D88 + 0xC), 0);
        *(s32*)(temp_r31 + 0x15C) = *(s32*)(temp_r31 + 0x15C) | 0x400000;
        *(s32*)(temp_r31 + 0x26C) = *(s32*)(arg0 + 0x10);
    }
}

// 0x8008B7C0
// Deleting destructor: sets its vtable and resets the state fields, then base dtor fn_80062CE0
// and frees if del > 0.
u8* fn_8008B7C0(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(s32*)(arg0 + 0x370) = 0;
        *(void**)arg0 = lbl_8035B8F0;
        *(s32*)(arg0 + 0x264) = -1;
        *(s32*)(arg0 + 0x374) = 0;
        *(s32*)(arg0 + 0x35C) = 0;
        *(s32*)(arg0 + 0x360) = -1;
        *(s32*)(arg0 + 0x36C) = -1;
        *(s32*)(arg0 + 0x4CC) = 0;
        fn_80062CE0(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8009C384
// Deleting destructor: sets its vtable, runs the helper fn_8009C4EC on it, destroys the two
// embedded sub-objects at +0x324 and +0x268, base dtor fn_80062CE0 and frees if del > 0.
u8* fn_8009C384(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)arg0 = lbl_8035CBA8;
        fn_8009C4EC(arg0);
        fn_801F9598(arg0 + 0x324, -1);
        fn_801F6238(arg0 + 0x268, -1);
        fn_80062CE0(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x800A5D2C
// Sets the two flag bytes at +0x264/+0x265, lets the vtable slot 0xC return a sub-object and flags
// bit 2 on it, flags bit 2 on the object at +4, sets +0x266 and flags 0x600000 in +0x15C.
void fn_800A5D2C(u8* arg0) {
    *(u8*)(arg0 + 0x264) = 1;
    *(u8*)(arg0 + 0x265) = 1;
    if ((*(PtrFn1*)(*(u8**)arg0 + 0xC))(arg0) != NULL) {
        u8* sub = (*(PtrFn1*)(*(u8**)arg0 + 0xC))(arg0);
        *(s32*)(sub + 0x8) = *(s32*)(sub + 0x8) | 2;
    }
    u8* obj = *(u8**)(arg0 + 0x4);
    *(s32*)(obj + 0x20) = *(s32*)(obj + 0x20) | 2;
    *(u8*)(arg0 + 0x266) = 1;
    *(s32*)(arg0 + 0x15C) = *(s32*)(arg0 + 0x15C) | 0x600000;
}

// 0x800C694C
// Copies the 3-float vector arg1 into the transform block of the object at +4 (the y component
// comes from +0xCC), sets four flag bits in its +0x20 word, clears +0x268 and sends event 0xBF755822.
void fn_800C694C(u8* arg0, u8* arg1) {
    f32 y = *(f32*)(arg0 + 0xCC);
    u8* obj = *(u8**)(arg0 + 0x4);
    *(f32*)(obj + 0x0) = *(f32*)(arg1 + 0x0);
    *(f32*)(obj + 0x4) = y;
    *(f32*)(obj + 0x8) = *(f32*)(arg1 + 0x8);
    *(f32*)(arg1 + 0x4) = y;
    *(s32*)(obj + 0x20) = *(s32*)(obj + 0x20) | 0x8000;
    *(s32*)(obj + 0x20) = *(s32*)(obj + 0x20) | 0x10000000;
    *(s32*)(obj + 0x20) = *(s32*)(obj + 0x20) | 0x4000;
    *(s32*)(obj + 0x20) = *(s32*)(obj + 0x20) | 0x10000000;
    *(s32*)(arg0 + 0x268) = 0;
    EventSend(gEventMgr, arg0, NULL, 0xBF755822u, 0, 0, 0, 0);
}

}  // extern "C"
