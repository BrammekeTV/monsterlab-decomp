#include "types.h"

// Small functions batch (miss472): catch minigame, world/zone lookup, event sends,
// stick read, and object destructors. Written from the asm; not yet matched.

static inline f32 absf(f32 v) { return v < 0.0f ? -v : v; }

extern "C++" void* gEventMgr;                // 0x805FB6C8
extern "C++" s32 gZoneResourceSize;          // 0x805FB94C

extern "C" {
void  fn_800223D4(void);                                                        // 0x800223D4
void  CatchMinigame_UpdateObjects(void* g, f32 dt);                             // 0x8002FFBC
void  CatchMinigame_UpdateLane(void* g, void* lane, BOOL active, f32 dt);       // 0x8002EF4C
void fn_800D9044(u8* p, f32 delta);                                                        // 0x800D9044
void  fn_8008F7D4(u8* p);                                                       // 0x8008F7D4
u8*   fn_801F94EC(u8* p, s32 del);                                              // 0x801F94EC
u8*   fn_80216B94(u8* p, s32 del);                                              // 0x80216B94
u8*   fn_80091D28(u8* p, s32 del);                                              // 0x80091D28
u8*   fn_800938E0(u8* p, s32 del);                                              // 0x800938E0
u8*   fn_8009215C(u8* p, s32 del);                                              // 0x8009215C
void  fn_8006FAF4(u8* p, s32 a);                                                // 0x8006FAF4
void  __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count);  // 0x8032F2E0
void  operator_delete(void* p);                                                 // 0x801D426C
BOOL  ZoneIsLoaded(void* world, s32 zone);                                      // 0x8012F408
void  StickRead(s32 channel, f32* x, f32* y);                                   // 0x80147704
void  fn_80121C90(s32 action, void* obj, void* other, const f32* pos);          // 0x80121C90
s32   TriggerDoAction(void* action, void* a1, void* a2);                       // 0x801E0054
void  fn_800C0660(u8* p, f32* pos);                                             // 0x800C0660
void  EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);    // 0x801BA2FC
void  MemFreeSmall(void* p);                                                   // 0x801D4340
u8*   fn_80041880(u8* p, s32 del);                                              // 0x80041880
void  fn_80042CC0(u8* p, s32 del);                                              // 0x80042CC0
void  fn_80044FA4(void);                                                        // 0x80044FA4
u8*   fn_800445F4(u8* p, s32 del);                                              // 0x800445F4
u8*   fn_80044C7C(u8* p, s32 del);                                              // 0x80044C7C
u8* fn_8021F3F0(u8* p, s32 del);                                              // 0x8021F3F0
void  fn_801C7E0C(u8* p, s32 del);                                              // 0x801C7E0C

extern u8 gWorldState[];                     // 0x803E6D00 (first word: world block pointer)
extern u8 lbl_8035BC90[];                    // 0x8035BC90 (vtable)
extern u8 lbl_803C4298[];                    // 0x803C4298
extern void* lbl_805FB058;                   // 0x805FB058
extern f32 lbl_80384460[4];                  // 0x80384460 (vec4 constant)
extern f32 lbl_805FD0F8;                     // 0x805FD0F8 (0.0f)
extern f32 lbl_805FD100;                     // 0x805FD100 (stick low threshold)
extern f32 lbl_805FD104;                     // 0x805FD104 (stick high threshold)
extern f32 lbl_805FD800;                     // 0x805FD800
extern f32 lbl_805FE230;                     // 0x805FE230
extern f32 lbl_805FE238;                     // 0x805FE238
}

extern "C" {

// 0x8002F7EC
void fn_8002F7EC(u8* arg0, f32 farg0) {
    u8* obj;
    u8* a;
    u8* src;

    fn_800223D4();
    CatchMinigame_UpdateObjects(arg0, farg0);
    CatchMinigame_UpdateLane(arg0, arg0 + 0x90, (BOOL) * (u8*)(arg0 + 0xF74), farg0);
    CatchMinigame_UpdateLane(arg0, arg0 + 0xC8, *(u8*)(arg0 + 0xF74) == 0, farg0);
    obj = *(u8**)(arg0 + 0x294);
    if (obj != NULL) {
        a = *(u8**)(arg0 + 0xF0C);
        if (a != NULL) {
            src = *(u8**)(a + 0x4);
            *(f32*)(obj + 0x68) = *(f32*)(src + 0x68);
            *(f32*)(obj + 0x6C) = *(f32*)(src + 0x6C);
            // Virtual-style call: fn = *(*(obj + 0x138) + 0x2C), args (obj, 0x1A, src).
            ((void (*)(u8*, s32, u8*))(*(void**)(*(u8**)(obj + 0x138) + 0x2C)))(obj, 0x1A, src);
        }
    }
}

// 0x8007A1D0
// Transforms the point at +0x24 (plus a scaled offset from a table entry) into the output vec3 at +0x4.
void fn_8007A1D0(u8* arg0, f32 dt) {
    u8* e;
    u8* dst;
    f32 t;
    f32 x;
    f32 y;
    f32 z;

    fn_800D9044(arg0, dt);  // f1 passes through untouched (vtable slot 0x98 callers load it)
    e = *(u8**)(arg0 + 0x8) + *(s32*)(arg0 + 0x10) * 0x1C;
    t = *(f32*)(arg0 + 0x14) / *(f32*)(e + 0x18);
    x = *(f32*)(e + 0x0) + *(f32*)(e + 0xC) * t + *(f32*)(arg0 + 0x24);
    y = *(f32*)(e + 0x4) + *(f32*)(e + 0x10) * t + *(f32*)(arg0 + 0x28);
    z = *(f32*)(e + 0x8) + *(f32*)(e + 0x14) * t + *(f32*)(arg0 + 0x2C);
    dst = *(u8**)(arg0 + 0x4);
    *(f32*)(dst + 0x0) = x;
    *(f32*)(dst + 0x4) = y;
    *(f32*)(dst + 0x8) = z;
}

// 0x8008EF14
// Destructor for the big object: releases sub-objects and arrays, then frees if del > 0.
u8* fn_8008EF14(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)(arg0 + 0x4) = (void*)lbl_8035BC90;
        fn_8008F7D4(arg0);
        fn_801F94EC(arg0 + 0x35C, -1);
        fn_80216B94(arg0 + 0x2A8, -1);
        __destroy_arr(arg0 + 0x1F8, (void (*)(void*, s32))fn_80091D28, 0x14, 3);
        __destroy_arr(arg0 + 0x180, (void (*)(void*, s32))fn_800938E0, 0x28, 3);
        __destroy_arr(arg0 + 0xA8, (void (*)(void*, s32))fn_8009215C, 0x48, 3);
        fn_8006FAF4(arg0, 0);
        if (arg1 > 0) operator_delete(arg0);
    }
    return arg0;
}

// 0x800CF71C
// Scans the loaded zones for the first resource entry whose type word is 0x12; returns it or NULL.
void* fn_800CF71C(void) {
    u8* result = NULL;
    s32 i = 0;

    while (i <= *(s32*)(*(u8**)gWorldState + 0x4) && result == NULL) {
        if (ZoneIsLoaded(gWorldState, i) != 0) {
            u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + i * gZoneResourceSize;
            s32 j = 0;
            s32 k = 0;
            while (j < (s32)*(u8*)(zone + 0xAC) && result == NULL) {
                u8* e = *(u8**)(*(u8**)(zone + 0xA8) + k);
                if (*(s32*)(e + 0x8) == 0x12) result = e;
                j += 1;
                k += 4;
            }
        }
        i += 1;
    }
    return result;
}

// 0x801A8DB4
// Clears the given flag bits in +0x64. If bit 2 is set, clears bit 1 in each entry of the
// table at +0x178 and calls fn_801C7E0C on every non-null entry.
void fn_801A8DB4(u8* arg0, s32 arg1) {
    u8* tbl;
    u8* cur;
    s32 i;

    *(s32*)(arg0 + 0x64) = *(s32*)(arg0 + 0x64) & ~arg1;
    if (arg1 & 0x2) {
        tbl = *(u8**)(arg0 + 0x178);
        if (tbl != NULL) {
            cur = tbl;
            for (i = 0; i < *(s32*)(tbl + 0xC); i++) {
                s32* e = *(s32**)(cur + 0x14);
                if (e != NULL) *e &= ~0x2;
                cur += 4;
            }
            for (i = 0; i < *(s32*)(*(u8**)(arg0 + 0x178) + 0xC); i++) {
                u8* e = *(u8**)(tbl + i * 4 + 0x14);
                if (e != NULL) fn_801C7E0C(e, 0);
            }
        }
    }
}

// 0x801A9F5C
// Clamps the move from the current position (+0x140/+0x144/+0x148) toward the target
// (+0x14C/+0x150/+0x154) to at most farg0 per axis, stores the result at +0x68..+0x70,
// then calls the virtual at +0x138 + 0x2C with the new y and x.
void fn_801A9F5C(u8* arg0, f32 farg0) {
    s32 flag = 0;
    f32 x0 = *(f32*)(arg0 + 0x140);
    f32 y0 = *(f32*)(arg0 + 0x144);
    f32 z0 = *(f32*)(arg0 + 0x148);
    f32 x = *(f32*)(arg0 + 0x14C);
    f32 y = *(f32*)(arg0 + 0x150);
    f32 z = *(f32*)(arg0 + 0x154);
    f32 dx = x - x0;
    f32 dy = y - y0;
    f32 dz = z - z0;

    if (absf(dx) > farg0) {
        flag = 0x10;
        x = x0 + dx * farg0;
    }
    if (absf(dy) > farg0) {
        flag = 0x10;
        y = y0 + dy * farg0;
    }
    if (absf(dz) > farg0) {
        flag = 0x10;
        z = z0 + dz * farg0;
    }
    *(f32*)(arg0 + 0x68) = x;
    *(f32*)(arg0 + 0x6C) = y;
    *(f32*)(arg0 + 0x70) = z;
    ((void (*)(u8*, s32, f32, f32))(*(void**)(*(u8**)(arg0 + 0x138) + 0x2C)))(arg0, flag, y, x);
}

// 0x801D957C
// Copies the vec4 constant at lbl_80384460 into +0x48, +0x58 (and the +0x48 slot again),
// clears the flag bits at +0x6C bit 0x80 and +0x74 bits 0xE0, and zeroes the int at +0x70.
void fn_801D957C(u8* arg0) {
    f32 v[4];

    v[0] = lbl_80384460[0];
    v[1] = lbl_80384460[1];
    v[2] = lbl_80384460[2];
    v[3] = lbl_80384460[3];
    *(s32*)(arg0 + 0x70) = 0;
    *(u8*)(arg0 + 0x6C) = (u8)(*(u8*)(arg0 + 0x6C) & 0x7F);
    *(f32*)(arg0 + 0x58) = v[0];
    *(f32*)(arg0 + 0x5C) = v[1];
    *(f32*)(arg0 + 0x60) = v[2];
    *(f32*)(arg0 + 0x64) = v[3];
    if (!(*(u8*)(arg0 + 0x74) & 0x80)) {
        *(f32*)(arg0 + 0x48) = v[0];
        *(f32*)(arg0 + 0x4C) = v[1];
        *(f32*)(arg0 + 0x50) = v[2];
        *(f32*)(arg0 + 0x54) = v[3];
    }
    *(f32*)(arg0 + 0x48) = *(f32*)(arg0 + 0x58);
    *(f32*)(arg0 + 0x4C) = *(f32*)(arg0 + 0x5C);
    *(f32*)(arg0 + 0x50) = *(f32*)(arg0 + 0x60);
    *(f32*)(arg0 + 0x54) = *(f32*)(arg0 + 0x64);
    *(u8*)(arg0 + 0x74) = (u8)(*(u8*)(arg0 + 0x74) & 0x1F);
}

// 0x80040550
// Reads stick channel (table value * 16 + 14) and returns a direction bitmask:
// 4 left, 8 right, 2 down, 1 up, using the dominant axis and +/-0.1 dead zones.
s32 fn_80040550(void) {
    f32 x = lbl_805FD0F8;
    f32 y = lbl_805FD0F8;
    s32 result = 0;
    s32 chan = *(s32*)(lbl_803C4298 + 0x38);

    StickRead(chan * 16 + 14, &x, &y);
    if (absf(x) >= absf(y)) {
        if (x < lbl_805FD100) {
            result |= 4;
        } else if (x > lbl_805FD104) {
            result |= 8;
        }
    } else {
        if (y < lbl_805FD100) {
            result |= 2;
        } else if (y > lbl_805FD104) {
            result |= 1;
        }
    }
    return result;
}

// 0x80058C24
// Stores the target's +0x60 into arg0+0x14C, fires the action at arg0+0x88 with arg2 as the
// position, then triggers the actions stored at arg1+0x9C+0x74 and arg1+0x44 when their flags are set.
void fn_80058C24(u8* arg0, u8* arg1, u8* arg2) {
    f32 pos[3];
    u8* sub;

    if (arg1 != NULL) {
        *(s32*)(arg0 + 0x14C) = *(s32*)(*(u8**)(arg1 + 0x9C) + 0x60);
    }
    if (*(s32*)(arg0 + 0x88) != 0) {
        pos[0] = *(f32*)(arg2 + 0x0);
        pos[1] = *(f32*)(arg2 + 0x4);
        pos[2] = *(f32*)(arg2 + 0x8);
        fn_80121C90(*(s32*)(arg0 + 0x88), arg0, arg1, pos);
    }
    if (arg1 != NULL) {
        sub = *(u8**)(arg1 + 0x9C);
        if (sub != NULL && ((*(u32*)(sub + 0x74) >> 27) & 0xF) != 0) {
            TriggerDoAction(sub + 0x74, arg1, arg0);
        }
        if ((*(u32*)(arg1 + 0x44) >> 27) & 0xF) {
            TriggerDoAction(arg1 + 0x44, arg1, arg0);
        }
    }
}

// 0x800BB420
// Sends two events to the event manager, then sets the position at +0x240 from the
// vector at *(+0x238)+4 offset by two constants, and clears +0x8C to lbl_805FE230.
void fn_800BB420(u8* arg0) {
    f32 pos[3];
    u8* src;

    EventSendTo(gEventMgr, 0x83B4950AU, NULL, 0x9B51AA64U, 0, 0);
    EventSendTo(gEventMgr, 0x2A75F102U, NULL, 0x9B51AA64U, 0, 0);
    src = *(u8**)(*(u8**)(arg0 + 0x238) + 0x4);
    pos[0] = *(f32*)(src + 0x0) + lbl_805FE230;
    pos[1] = *(f32*)(src + 0x4) + lbl_805FE230;
    pos[2] = *(f32*)(src + 0x8) + lbl_805FE238;
    fn_800C0660(arg0 + 0x240, pos);
    *(f32*)(arg0 + 0x8C) = lbl_805FE230;
}

// 0x800BB520
// Same body as fn_800BB420 (duplicate of the same routine at a second address).
void fn_800BB520(u8* arg0) {
    f32 pos[3];
    u8* src;

    EventSendTo(gEventMgr, 0x83B4950AU, NULL, 0x9B51AA64U, 0, 0);
    EventSendTo(gEventMgr, 0x2A75F102U, NULL, 0x9B51AA64U, 0, 0);
    src = *(u8**)(*(u8**)(arg0 + 0x238) + 0x4);
    pos[0] = *(f32*)(src + 0x0) + lbl_805FE230;
    pos[1] = *(f32*)(src + 0x4) + lbl_805FE230;
    pos[2] = *(f32*)(src + 0x8) + lbl_805FE238;
    fn_800C0660(arg0 + 0x240, pos);
    *(f32*)(arg0 + 0x8C) = lbl_805FE230;
}

// 0x80040A08
// Destructor for the object at arg0: releases the two held pointers, tears down two arrays,
// then frees arg0 if arg1 > 0.
void* fn_80040A08(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        if (*(void**)(arg0 + 0x408) != NULL) {
            fn_80041880(*(u8**)(arg0 + 0x408), 1);
            *(void**)(arg0 + 0x408) = NULL;
        }
        if (lbl_805FB058 != NULL) {
            fn_80042CC0((u8*)lbl_805FB058, 1);
            lbl_805FB058 = NULL;
        }
        fn_80044FA4();
        *(s32*)(arg0 + 0x4D4) = 0;
        __destroy_arr(arg0 + 0x290, (void (*)(void*, s32))fn_800445F4, 0xBC, 2);
        __destroy_arr(arg0, (void (*)(void*, s32))fn_80044C7C, 0x148, 2);
        if (arg1 > 0) operator_delete(arg0);
    }
    return arg0;
}

// 0x80076FCC
// Calls the virtual at +0x1C +0x8 (arg 1) on each of the four pointers at +0x14..+0x20, then
// frees the four buffers at +0x1228..+0x1234 and the one at +0x1238; deletes arg0 if arg1 > 0.
void* fn_80076FCC(u8* arg0, s32 arg1) {
    s32 i;

    if (arg0 != NULL) {
        for (i = 0; i < 4; i++) {
            u8* obj = *(u8**)(arg0 + 0x14 + i * 4);
            if (obj != NULL) {
                u8* sub = *(u8**)(obj + 0x1C);
                ((void (*)(u8*, s32))(*(void**)(sub + 0x8)))(obj, 1);
            }
        }
        fn_8021F3F0(*(u8**)(arg0 + 0x1238), 1);
        MemFreeSmall(*(void**)(arg0 + 0x1228));
        MemFreeSmall(*(void**)(arg0 + 0x122C));
        MemFreeSmall(*(void**)(arg0 + 0x1230));
        MemFreeSmall(*(void**)(arg0 + 0x1234));
        if (arg1 > 0) operator_delete(arg0);
    }
    return arg0;
}

// 0x800822A8
// Resets arg0's timers (+0x274, +0x2B8), sets +0x270 from the world global, and copies
// the fifteen floats at +0x278..+0x2B0 from arg1.
void fn_800822A8(u8* arg0, u8* arg1) {
    s32 off;

    *(f32*)(arg0 + 0x274) = lbl_805FD800;
    *(f32*)(arg0 + 0x270) = *(f32*)(gWorldState + 0x8);
    *(s32*)(arg0 + 0x2B8) = 0;
    for (off = 0x278; off <= 0x2B0; off += 4) {
        *(f32*)(arg0 + off) = *(f32*)(arg1 + off);
    }
}

}  // extern "C"
