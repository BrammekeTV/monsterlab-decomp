// Small functions from the miss503 draft batch (hand-verified against the asm).
#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

// Virtual slot call shapes used below (the slot reads the vtable stored at the object's +0 or +0x138).
typedef s32 (*VFnS32)(u8*);
typedef void (*VFnVoid)(u8*);
typedef void (*VFnVoidS32)(u8*, s32);

extern "C" {
void    fn_8003DCE0(u8* p);
void    fn_8004C718(u8* p, s32 idx, void* elem);
void    fn_8019AA3C(u8* obj, s32 a, s32 b);
s32     UIElement_PlayUIAnimByHash(u8* arg0, u32* arg1);
void    OSUnlockMutex(u8* p);
void    DWC_ClearError(void);
s32     fn_802E925C(void);
void    fn_802E9CBC(s32 counter);
void    fn_802E4A90(void);
void    SOCleanup(void);
void    SOFinish(void);
u8* fn_80160A28(u8* arg0);
void    fn_80162A70(u8* p);
void    SessionBegin(void* session);
u32     StringHash(const char* s);
void*   ScreenSetsCurrent(void* sets);
void*   ScreenSetFindElement(void* set, u32 hash);
void    fn_801A2524(u8* el);
void    fn_801A8654(u8* obj, s32 a1);
BOOL    PropIncrementState(void* prop);
s32     RaySceneTest(void* ray);
u8*     fn_8006FBDC(void);
u8*     fn_800C253C(u8** obj, s32 kind);
u8*      fn_8019552C(u8* list, s32 index);   // returns the element address (leaf def is u32)
void    fn_8018D4CC(void* p);
s32     UISetTexture(u8* el, s32 tex);

extern u8 gGame[];
extern u8 gWorldState[];
extern "C++" void* gScreenSets;
extern "C++" u8 gFlag805FB948;
extern u8 lbl_803C4298[];
extern u8 lbl_8040E260[];
extern u8 lbl_8040E5F0[];
extern u8 lbl_80556AB0[];
extern u8 lbl_80383A88[];
extern u8 lbl_80383B60[];
extern s32 lbl_805F95F8;
extern f32 lbl_805FFAB8;
extern f32 lbl_80600480;
extern f32 lbl_803469A8[];
extern f32 lbl_80346268[];
extern f32 lbl_805FD928;
extern f32 lbl_805FD944;
extern f32 lbl_805FE108;
extern f32 lbl_805FE118;
extern f32 lbl_805FDE40;
extern s32 lbl_805FB368;
extern s32 lbl_805FB36C;
extern s32 lbl_805FB370;
extern s32 lbl_805FB374;
extern s32 lbl_805FB380;
extern s32 lbl_805FB384;
extern s32 lbl_805FBB30;
extern f32 lbl_805FBB3C;
extern s32 lbl_805FBB6C;
}

extern "C" {

// 0x801171D4
// Releases the five slot objects at +0x28 (each detached and freed) and clears the list pointer.
void fn_801171D4(u8* arg0) {
    if (*(u8**)(arg0 + 0x28) != NULL) {
        s32 i = 0;
        s32 off = 0;
        do {
            u8* list = *(u8**)(arg0 + 0x28);
            u8* el = *(u8**)(list + off + 0xC50);
            fn_8004C718(list, i, NULL);
            fn_8003DCE0(el);
            i += 1;
            off += 4;
        } while (i < 5);
    }
    *(s32*)(arg0 + 0x20) = 0;
    *(void**)(arg0 + 0x28) = NULL;
}

// 0x8019CBEC
// Per-frame update of a flagged UI element: plays the pending anim, else sends the event.
void fn_8019CBEC(u8* arg0) {
    if (*(s32*)(arg0 + 0x64) & 1) {
        u8* obj = *(u8**)(arg0 + 0xBC);
        if (obj != NULL) {
            fn_8019AA3C(obj, (s8)*(u8*)(arg0 + 0xB8), (s8)*(u8*)(arg0 + 0xB9));
            return;
        }
        u32 sp8 = 0x705598E1u;
        UIElement_PlayUIAnimByHash(arg0, &sp8);
        if (*(s32*)(arg0 + 0x60) == 4 && *(u8*)(arg0 + 0x16C) != 0) {
            *(u8*)(arg0 + 0x16C) = 1;
        }
    }
}

// 0x8020852C
// Resets the global state of the 0x8020 subsystem (counters, timers and the pending flag).
void fn_8020852C(u8* arg0) {
    lbl_805FBB6C = 0;
    OSUnlockMutex(lbl_80556AB0);
    DWC_ClearError();
    fn_802E9CBC(fn_802E925C());
    fn_802E4A90();
    SOCleanup();
    SOFinish();
    lbl_805FB384 = 0;
    lbl_805FB380 = 0;
    lbl_805FB36C = 0;
    lbl_805FB370 = 0;
    lbl_805FB368 = 0;
    lbl_805FB374 = 0;
    lbl_805FBB3C = lbl_80600480;
    if (lbl_805FBB30 != 0) {
        lbl_805FBB30 = 0;
        *(s32*)(arg0 + 0x4) = 0;
    }
}

// 0x800084B8
// Clears the camera rotate flag, resets two effect objects and starts the session (or copies the world value).
void fn_800084B8(void) {
    *(s32*)(gGame + 0x5C4) = *(s32*)(gGame + 0x5C4) & ~0x2;
    ForceCameraRotateMode(0);
    fn_80160A28(lbl_8040E260);
    fn_80162A70(lbl_8040E5F0);
    if (gFlag805FB948 != 0) {
        u8* world = *(u8**)gWorldState;
        s32 v = *(s32*)(world + 0x8);
        u8* dst = *(u8**)(lbl_803C4298 + 0x8);
        *(s32*)(dst + 0x2C0) = v;
        return;
    }
    SessionBegin(gGame + 0x5C8);
}

// 0x80037F28
// Stops the AnimatedBackground element and tells the object at +0x154 to change to mode 2, then to mode 0.
void fn_80037F28(u8* arg0) {
    void* sets = gScreenSets;
    u32 hash = StringHash("AnimatedBackground");
    u8* el = (u8*)ScreenSetFindElement(ScreenSetsCurrent(sets), hash);
    if (el != NULL) {
        fn_801A2524(el);
    }
    u8* obj = *(u8**)(arg0 + 0x154);
    (*(VFnVoidS32*)(*(u8**)(obj + 0x138) + 0x2C))(obj, 2);
    fn_801A8654(*(u8**)(arg0 + 0x154), 0);
}

// 0x8006BA90
// Calls the object's virtual check; if it fails, up to 10 times advances the prop state while it is not busy.
void fn_8006BA90(u8* arg0) {
    s32 n = 10;
    if ((*(VFnS32*)(*(u8**)arg0 + 0x4C))(arg0) == 0) {
        if (!(*(s32*)(*(u8**)(arg0 + 0x4) + 0x20) & 3)) {
            return;
        }
        for (;;) {
            if (*(s32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x20) {
                break;
            }
            if (PropIncrementState(arg0) == 0) {
                break;
            }
            n -= 1;
            if (n == 0) {
                break;
            }
        }
    }
}

// 0x80087F68
// Ray-scene distance ratio for the record at +0x484, as a percentage clamped to 0..100.
f32 fn_80087F68(u8* arg0) {
    s32 idx = RaySceneTest(arg0);
    if (idx < 0) {
        idx = 0;
    } else if (idx > 3) {
        idx = 3;
    }
    f32 v = lbl_805FD944 * (*(f32*)(arg0 + 0x484) / lbl_803469A8[idx]);
    if (v < lbl_805FD928) {
        return lbl_805FD928;
    }
    if (v > lbl_805FD944) {
        return lbl_805FD944;
    }
    return v;
}

// 0x8009F66C
// Picks the next state for the record: state 0 when the target flag is set or the timer is positive, else 2.
void fn_8009F66C(u8* arg0) {
    if (fn_8006FBDC() == NULL) {
        s32 k = *(s32*)(arg0 + 0x14);
        *(s32*)(arg0 + 0x14) = k + 1;
        fn_800C253C((u8**)arg0, 0);
        return;
    }
    if (*(f32*)(fn_8006FBDC() + 0xCD0) > lbl_805FDE40) {
        s32 k2 = *(s32*)(arg0 + 0x14);
        *(s32*)(arg0 + 0x14) = k2 + 1;
        fn_800C253C((u8**)arg0, 0);
        return;
    }
    fn_800C253C((u8**)arg0, 2);
}

// 0x800B58B8
// Same as fn_80087F68 but for the record at +0x67C with the second ratio table.
f32 fn_800B58B8(u8* arg0) {
    s32 idx = RaySceneTest(arg0);
    if (idx < 0) {
        idx = 0;
    } else if (idx > 3) {
        idx = 3;
    }
    f32 v = lbl_805FE118 * (*(f32*)(arg0 + 0x67C) / lbl_80346268[idx]);
    if (v < lbl_805FE108) {
        return lbl_805FE108;
    }
    if (v > lbl_805FE118) {
        return lbl_805FE118;
    }
    return v;
}

// 0x800D7F24
// Resets the record: the two ids to -1, the pointer at +0x44 to NULL and the other fields to zero.
void fn_800D7F24(u8* arg0) {
    *(s32*)(arg0 + 0x44) = 0;
    *(s32*)(arg0 + 0x3C) = -1;
    *(s32*)(arg0 + 0x40) = -1;
    *(s32*)(arg0 + 0x4) = 0;
    *(s32*)(arg0 + 0x8) = 0;
    *(s32*)(arg0 + 0xC) = 0;
    *(s32*)(arg0 + 0x10) = 0;
    *(s32*)(arg0 + 0x14) = 0;
    *(s32*)(arg0 + 0x18) = 0;
    *(s32*)(arg0 + 0x1C) = 0;
    *(s32*)(arg0 + 0x20) = 0;
    *(s32*)(arg0 + 0x28) = 0;
    *(s32*)(arg0 + 0x2C) = 0;
    *(s32*)(arg0 + 0x24) = 0;
    *(s32*)(arg0 + 0x30) = 0;
    *(s32*)(arg0 + 0x0) = 0;
    *(s32*)(arg0 + 0x34) = 0;
    *(s32*)(arg0 + 0x38) = 0;
}

// 0x801A4798
// Runs the virtual update (slot 0x14) on every element of the grid at +0x1BC, then frees the grid.
s32 fn_801A4798(u8* arg0) {
    s32 n = *(s32*)(arg0 + 0x140) * *(s32*)(arg0 + 0x13C);
    for (s32 i = 0; i < n; i++) {
        u8* elem = (u8*)fn_8019552C(*(u8**)(arg0 + 0x1BC), i);
        u8* obj = *(u8**)elem;
        if (obj != NULL) {
            (*(VFnVoid*)(*(u8**)(obj + 0x138) + 0x14))(obj);
        }
    }
    fn_8018D4CC(arg0);
    return 0;
}

// 0x801A9C54
// Walks the linked list at +0x120: unlinks each node, clears its texture if needed, runs its update, then frees the list.
s32 fn_801A9C54(u8* arg0) {
    u8* el = *(u8**)(arg0 + 0x120);
    while (el != NULL) {
        *(void**)(arg0 + 0x120) = *(u8**)(el + 0x124);
        if (*(s32*)(el + 0x60) == 2 && *(u8*)(arg0 + 0x13C) != 0) {
            UISetTexture(el, 0);
        }
        (*(VFnVoid*)(*(u8**)(el + 0x138) + 0x14))(el);
        el = *(u8**)(arg0 + 0x120);
    }
    fn_8018D4CC(arg0);
    return 0;
}

// 0x801ACFC4
// Constructor-style init: sets the vtable, runs the virtual reset when the global is set, then sets the four float fields.
void* fn_801ACFC4(u8* arg0) {
    *(void**)arg0 = lbl_80383B60;
    if (lbl_805F95F8 != -1) {
        (*(VFnVoid*)(*(u8**)arg0 + 0xC))(arg0);
    }
    *(void**)arg0 = lbl_80383A88;
    if (lbl_805F95F8 != -1) {
        *(f32*)(arg0 + 0x20) = lbl_805FFAB8;
        *(f32*)(arg0 + 0x24) = lbl_805FFAB8;
        *(f32*)(arg0 + 0x28) = lbl_805FFAB8;
        *(f32*)(arg0 + 0x3C) = lbl_805FFAB8;
    }
    return arg0;
}

}  // extern "C"
