// Small functions from the draft_miss222 batch: slot-array helpers, a controller reset,
// a world-entity lookup, and a few flag and state helpers.
#include "types.h"

typedef s32 (*VCallS32)(u8*);
typedef u8* (*VCallPtr)(u8*);

extern "C" {
void  fn_8020F498(u8* p, s32 a2);               // 0x8020F498 (small_8004.cpp)
void  fn_801F8238(u8* p, s32 a4);               // 0x801F8238 (small_801F.cpp)
void  fn_801F8F34(u8* p, f32 v);                // 0x801F8F34 (small_801F.cpp)
void  fn_801F8FA0(u8* p, f32 v);                // 0x801F8FA0 (small_801F.cpp)
void  fn_801F8628(u8* obj, s8* src, s32 n);     // 0x801F8628
s32   fn_80097064(f32* pos);      // 0x80097064
void  fn_800A63B4(u8* p);                       // 0x800A63B4 (small_800A.cpp)
u8*   fn_80138864(u8* p, s32 idx);              // 0x80138864
s32   fn_801D3A60(s32 zone);                    // 0x801D3A60
u8*   GetGlobalSettings(void);                  // 0x801D2BF8 (world_alloc.cpp)
void* memcpy(void* d, const void* s, u32 n);
}

extern u8 gWorldState[];          // 0x803E6D00 (holds the world pointer)
extern u8 lbl_80358920[];         // 0x80358920 (vtable)
extern u8 lbl_803C4298[];         // 0x803C4298 (level info block)
extern u8 lbl_80346D40[];         // 0x80346D40 (s32 table)
extern f32 lbl_805FD190;          // 0x805FD190 (float constant)
extern f32 lbl_805FD194;          // 0x805FD194 (float constant)
extern f32 lbl_805FD198;          // 0x805FD198 (float constant)
extern f32 lbl_805FD19C;          // 0x805FD19C (float constant)
extern f32 lbl_805FD1A0;          // 0x805FD1A0 (float constant)
extern f32 lbl_805FDE98;          // 0x805FDE98 (float constant)
extern f32 lbl_805FE208;          // 0x805FE208 (float constant)
extern f32 lbl_805FE20C;          // 0x805FE20C (float constant)

extern "C" {

// 0x8004421C
// Resets the controller sub-object at +0x48: sets the vtable, the two timers at +0x4CC/+0x4D0,
// the control bytes {-1, 0, 0}, a zero at +0xE0 and the float at +0xDC. Returns the record.
u8* fn_8004421C(u8* arg0, s32 arg1) {
    s8 ctl[3];

    fn_8020F498(arg0, arg1);
    *(void**)arg0 = (void*)lbl_80358920;
    fn_801F8238(arg0 + 0x48, *(s32*)(lbl_803C4298 + 0x38));
    *(f32*)(arg0 + 0x4CC) = lbl_805FD190;
    *(f32*)(arg0 + 0x4D0) = lbl_805FD194;
    ctl[0] = -1;
    ctl[1] = 0;
    ctl[2] = 0;
    fn_801F8F34(arg0 + 0x48, lbl_805FD198);
    fn_801F8FA0(arg0 + 0x48, lbl_805FD19C);
    *(s32*)(arg0 + 0xE0) = 0;
    fn_801F8628(arg0 + 0x48, ctl, 1);
    *(f32*)(arg0 + 0xDC) = lbl_805FD1A0;
    return arg0;
}

// 0x80096F6C
// Takes the eight entries at +0x194 out and clears them, then puts each one back at the slot
// index that fn_80097064 returns for its record position.
void fn_80096F6C(u8* arg0) {
    u8* saved[8];
    s32 i;

    for (i = 0; i < 8; i++) {
        saved[i] = *(u8**)(arg0 + 0x194 + i * 4);
        *(void**)(arg0 + 0x194 + i * 4) = 0;
    }
    for (i = 0; i < 8; i++) {
        u8* p = saved[i];
        u8* rec = *(u8**)(p + 0x4);
        f32 pos[3];
        s32 idx;

        pos[0] = *(f32*)(rec + 0x0);
        pos[1] = *(f32*)(rec + 0x4);
        pos[2] = *(f32*)(rec + 0x8);
        idx = fn_80097064(pos);
        *(void**)(arg0 + 0x194 + idx * 4) = p;
    }
}

// 0x800A4320
// Checks the current entry in the list at +0x338 against the value at arg1; when the
// test passes it calls fn_800A63B4 on that entry and advances the index at +0x360.
s32 fn_800A4320(u8* arg0, f32* arg1) {
    s32 idx = *(s32*)(arg0 + 0x360);

    if (idx < *(s32*)(arg0 + 0x26C)) {
        u8* elem = *(u8**)(arg0 + 0x338 + idx * 4);
        u8* rec = *(u8**)(elem + 0x4);

        if ((*arg1 - lbl_805FDE98) > *(f32*)(rec + 0x0)) {
            fn_800A63B4(elem);
            *(s32*)(arg0 + 0x360) = *(s32*)(arg0 + 0x360) + 1;
        }
    }
    return 0;
}

// 0x800AEF48
// Appends an entry (three values) to the 15-slot list whose count is at +0xB4.
// Returns 1 when added, 0 when the list is full.
s32 fn_800AEF48(u8* arg0, s32 arg1, s32 arg2, s32 arg3) {
    s32 idx = *(s32*)(arg0 + 0xB4);

    if (idx >= 0xF) return 0;
    *(s32*)(arg0 + idx * 4) = arg1;
    *(s32*)(arg0 + idx * 4 + 0x3C) = arg2;
    *(s32*)(arg0 + idx * 4 + 0x78) = arg3;
    *(s32*)(arg0 + 0xB4) = idx + 1;
    return 1;
}

// 0x800AEF98
// Same as fn_800AEF48 but the middle value is always 0 and the last one is arg2.
s32 fn_800AEF98(u8* arg0, s32 arg1, s32 arg2) {
    s32 idx = *(s32*)(arg0 + 0xB4);

    if (idx >= 0xF) return 0;
    *(s32*)(arg0 + idx * 4) = arg1;
    *(s32*)(arg0 + idx * 4 + 0x3C) = 0;
    *(s32*)(arg0 + idx * 4 + 0x78) = arg2;
    *(s32*)(arg0 + 0xB4) = idx + 1;
    return 1;
}

// 0x800B999C
// Stores the flag byte at +0x2A9 (cleared when the byte at +0x2A8 is 0). When arg2 is set and
// a current entry exists, writes the chosen float (1.0 or 0.0 by the flag) to +0x2AC and to the
// entry's sub-record at +0x44.
void fn_800B999C(u8* arg0, s32 arg1, s32 arg2) {
    f32 v;
    s32 idx;
    u8* rec;
    u8* sub;

    *(u8*)(arg0 + 0x2A9) = (u8)arg1;
    if (*(u8*)(arg0 + 0x2A8) == 0) {
        *(u8*)(arg0 + 0x2A9) = 0;
    }
    if (arg2 == 0) return;

    if (*(u8*)(arg0 + 0x2A9) != 0) {
        v = lbl_805FE20C;
    } else {
        v = lbl_805FE208;
    }
    idx = *(s32*)(arg0 + 0x278);
    if (idx == -1) return;

    *(f32*)(arg0 + 0x2AC) = v;
    rec = *(u8**)(arg0 + 0x264 + idx * 4);
    sub = *(u8**)(rec + 0x90);
    *(f32*)(sub + 0x44) = v;
}

// 0x800BD948
// Stores the state id at +0x268, clears the flag at +0x270, and sets it to 1 when
// the state's entry in the lbl_80346D40 table equals 2.
void fn_800BD948(u8* arg0, s32 arg1) {
    *(s32*)(arg0 + 0x268) = arg1;
    *(s32*)(arg0 + 0x270) = 0;
    if (*(s32*)(lbl_80346D40 + arg1 * 4) != 2) return;
    *(s32*)(arg0 + 0x270) = 1;
}

// 0x80107120
// Reads the world's +0x24 count; the entity sync loop that follows it is unreachable in the
// original (the branch skips it), so nothing else happens.
void fn_80107120(void) {
    u8* world = *(u8**)gWorldState;
    s32 count = *(s32*)(world + 0x24);

    (void)count;
}

// 0x8012847C
// Detaches the object from its owner: when bit 2 of arg1 is set and the owner's vtable call
// at +0xC succeeds, clears bit 1 in the returned sub-object's +0x8. Then clears arg1 bits
// in the flags at +0x20 of the object at +0x4.
void fn_8012847C(u8* arg0, u32 arg1) {
    if (arg1 & 2) {
        if ((*(VCallS32*)(*(u8**)arg0 + 0xC))(arg0) != 0) {
            u8* sub = (*(VCallPtr*)(*(u8**)arg0 + 0xC))(arg0);
            *(u32*)(sub + 0x8) &= ~2u;
        }
    }
    {
        u8* obj = *(u8**)(arg0 + 0x4);
        *(u32*)(obj + 0x20) &= ~arg1;
    }
}

// 0x8012A75C
// Clears the bits in arg1 from the flag word at +0x3C.
void fn_8012A75C(u8* arg0, u32 arg1) {
    *(u32*)(arg0 + 0x3C) &= ~arg1;
}

// 0x8012A780
// Returns the sub-object at +0x1E0 of the world (arg0 is not read).
u8* fn_8012A780(u8* arg0) {
    (void)arg0;
    return *(u8**)gWorldState + 0x1E0;
}

// 0x80132FE4
// For each of the three slots: when a source entry is present, creates an entry from the
// matching record (fn_80138864), copies its fields, and copies the source data block into the
// buffer the entry points to, sized by the global settings.
void fn_80132FE4(u8* arg0) {
    s32 i;

    for (i = 0; i < 3; i++) {
        u8* src = *(u8**)(arg0 + i * 4 + 0x14);
        u8* rec = arg0 + i * 0x30;

        if (src != 0) {
            u8* dst = fn_80138864(arg0, fn_801D3A60(*(s32*)(src + 0x18)));
            u8* gs;

            *(s32*)(dst + 0x0) = *(s32*)(rec + 0x20);
            *(s32*)(dst + 0x4) = *(s32*)(rec + 0x24);
            *(s32*)(dst + 0x8) = *(s32*)(rec + 0x28);
            *(s32*)(dst + 0xC) = *(s32*)(rec + 0x2C);
            *(s32*)(dst + 0x10) = *(s32*)(rec + 0x30);
            *(f32*)(dst + 0x14) = *(f32*)(rec + 0x34);
            *(u16*)(dst + 0x18) = *(u16*)(rec + 0x38);
            *(s32*)(dst + 0x1C) = *(s32*)(rec + 0x3C);
            *(s32*)(dst + 0x20) = *(s32*)(rec + 0x40);
            *(s32*)(dst + 0x24) = *(s32*)(rec + 0x44);
            *(void**)(dst + 0x28) = *(void**)(rec + 0x48);
            *(s32*)(dst + 0x2C) = *(s32*)(rec + 0x4C);
            gs = GetGlobalSettings();
            memcpy(*(void**)(dst + 0x28), src, *(s32*)(gs + 0x34) * 0x54 + 0x148);
        }
    }
}

}  // extern "C"
