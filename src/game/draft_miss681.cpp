#include "types.h"

struct Character;
struct StringTable;
struct GameDb;
struct ScopedLock { u8 pad[4]; };  // as in props.cpp

extern "C" {
void CatchMinigame_UpdatePointer(void* g);
void CatchMinigame_UpdateBase(void* g, f32 dt);
s32 PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);
void ScopedLockCtor(ScopedLock* l);
void ScopedLockDtor(ScopedLock* l, s32 flags);
void* ScreenSetsCurrent(void* sets);
u32 StringHash(const char* s);
s32 TextIdFromHash(u32 hash);
void TriggerEndCamera(Character* trigger, int a);
u8* fn_800216B8(u8* p, s32 del);  // 0x800216B8, result unused here
void fn_800223DC(u8* p);
void fn_800225C8(void* a0);
void fn_80028A84(u8* p, f32 v);
void fn_8002B94C(u8* p, f32 dt);   // 0x8002B94C, result unused here
void fn_80031090(u8* p, s32 del);
void fn_8004B938(u8* chr);
void fn_8005DB64(u8* a0, s32 a1);  // 0x8005DB64, tail-calls fn_800535F8 (not decompiled)
void fn_80062DC4(u8* p, s32 a, s32 b);
void* UIAnim_FindByName(void* set, u32 hash);
s32 fn_8019BBA4(u8* obj, void* set, s32 n);  // result unused here
s32 fn_8019BC80(u8* obj, u32 v);             // result unused here
void fn_8032F358(u8* obj, void* cb);
u8* fn_801B3308(void);
void fn_801B3378(u8* p, u8* key);
extern "C++" u8* fn_801B55A8(u8* p);  // 0x801B55A8 (C++ linkage in small_801Bb.cpp); asm compares the result as a signed int
void fn_801B55F4(u8* obj, u8* out, s32 idx);
void* fn_801D40E0(s32 n);
u8* fn_801F6238(u8* p, s32 del);
u8* fn_801F81E0(u8* p, s32 del);
void* fn_80209814(void* v);
s32 fn_8022E430(u8* a3);
s32 fn_8022E438(u8* a3);
void operator_delete(void* p);

extern "C++" GameDb* gGameDb;
extern "C++" void* gScreenSets;
extern "C++" StringTable* gStringTable;
extern "C++" StringTable* gStringTable2;
extern u8 lbl_80355C80[];
extern u8 lbl_805FAFE8[];
extern void* lbl_805FAFB0;
extern void* lbl_805FBBAC;
extern u8 lbl_805FBBE0[];
extern const char lbl_805F8748[];
extern const char lbl_805F8200[];
extern const char lbl_80354800[];
extern const char lbl_8035480C[];
extern const char lbl_80357D38[];
extern const char lbl_80357D44[];
extern f32 lbl_805FCA48;
extern f32 lbl_805FCA58;

// Creates the voice SFX handle lbl_805FBBAC once (fn_801D40E0, then fn_80209814 if that
// returned something). Same helper as in small_801Dc.cpp.
static void EnsureVoiceSfxHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
}

}  // extern "C"

extern "C" {

// 0x80023C18
// Destructor of the catch minigame object: clears its state, releases its sub-objects and
// the owned buffer, then frees the object itself when del > 0.
u8* fn_80023C18(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)(arg0 + 0x4) = (void*)lbl_80355C80;
        fn_801B3378(fn_801B3308(), arg0 + 0xA1C);
        fn_801B3378(fn_801B3308(), arg0 + 0x114);
        fn_801B3378(fn_801B3308(), arg0 + 0x598);
        lbl_805FAFB0 = NULL;
        *(s32*)(arg0 + 0x90) = 0;
        *(s32*)(arg0 + 0x94) = 0;
        *(s32*)(arg0 + 0x98) = 0;
        *(s32*)(arg0 + 0x9C) = 0;
        *(s32*)(arg0 + 0xA0) = 0;
        *(s32*)(arg0 + 0xA4) = 0;
        *(s32*)(arg0 + 0xA8) = 0;
        fn_8032F358(*(u8**)(arg0 + 0xADC), (void*)fn_80031090);
        // Owned buffer at +0xAF4 (its size slot at +0xAF8 is cleared before the free).
        if (*(void**)(arg0 + 0xAF4) != NULL) {
            u8* buf = *(u8**)(arg0 + 0xAF4);
            *(s32*)(arg0 + 0xAF8) = 0;
            operator_delete(buf);
        }
        fn_80031090(arg0 + 0xAE0, -1);
        fn_801F6238(arg0 + 0xA1C, -1);
        fn_801F81E0(arg0 + 0x598, -1);
        fn_801F81E0(arg0 + 0x114, -1);
        fn_800216B8(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x80028974
// Per-frame update of the catch minigame: updates the base and pointer state, arms the
// almost-caught flag when the ratio of +0xB4 / +0xCC drops below one half at stage 10,
// then updates the timer-driven part while the object at +0xAC is not active.
void fn_80028974(u8* arg0, f32 farg0) {
    CatchMinigame_UpdatePointer(arg0);
    CatchMinigame_UpdateBase(arg0, farg0);
    if (*(u8*)(arg0 + 0xFC) == 0 && (s32)*(f32*)(arg0 + 0x6C) == 10) {
        f32 ratio = (f32)*(s32*)(arg0 + 0xB4) / (f32)*(s32*)(arg0 + 0xCC);
        if (ratio < lbl_805FCA48) {
            *(u8*)(arg0 + 0xFC) = 1;
            fn_800225C8(arg0);
        }
    }
    *(s32*)(arg0 + 0x120) = 1;
    if (*(u8*)(arg0 + 0xAC) == 0) {
        fn_8002B94C(*(u8**)(arg0 + 0xA0), farg0);
        if (*(u8*)(arg0 + 0xFD) != 0) {
            *(u8*)(arg0 + 0xFD) = 0;
            *(f32*)(arg0 + 0x100) = lbl_805FCA58;
        }
    }
    fn_80028A84(arg0, farg0);
}

// 0x80031F84
// Adds delta to the global progress counter (+0xD0), recomputes the clamped fraction (+0xB0 scaled
// by progress over +0x9C) into the object at +0xAC, pushes its position (+0xB4..+0xBC) there,
// calls its show slot, bumps the event counter and plays the chime.
void fn_80031F84(s32 arg0) {
    u8* g = *(u8**)lbl_805FAFE8;
    *(s32*)(g + 0xD0) += arg0;
    f32 b0 = *(f32*)(g + 0xB0);
    f32 frac = (b0 * (f32)*(s32*)(g + 0xD0)) / (f32)*(s32*)(g + 0x9C);
    if (!(frac < b0)) frac = b0;
    *(f32*)(*(u8**)(g + 0xAC) + 0x74) = frac;
    f32 x = *(f32*)(g + 0xB4);
    f32 y = *(f32*)(g + 0xB8);
    f32 z = *(f32*)(g + 0xBC);
    u8* obj = *(u8**)(g + 0xAC);
    *(f32*)(obj + 0x68) = x;
    *(f32*)(obj + 0x6C) = y;
    *(f32*)(obj + 0x70) = z;
    obj = *(u8**)(g + 0xAC);
    (*(void (**)(void*, s32, f32, f32))(*(u8**)(obj + 0x138) + 0x2C))(obj, 0x1A, y, x);
    *(s32*)(g + 0xE4) += 1;
    EnsureVoiceSfxHandle();
    PlaySound(0x11550A70u, 0, 0, 0, 0, 0);
}

// 0x80032FBC
// Advances the sequence counter at +4. While the mode at +8 is 1, re-triggers the gem shake
// animation on the object at +0x18: the first step uses clip A, later steps clip B.
void fn_80032FBC(u8* arg0) {
    s32 count = *(s32*)(arg0 + 0x4) + 1;
    *(s32*)(arg0 + 0x4) = count;
    if (*(s32*)(arg0 + 0x8) == 1) {
        const char* clip = (count < 2) ? lbl_80357D38 : lbl_80357D44;
        u32 hash = StringHash(clip);
        u8* obj = *(u8**)(arg0 + 0x18);
        if (*(s32*)(obj + 0x10) != 0) fn_8019BC80(obj, 1);
        void* set = UIAnim_FindByName(ScreenSetsCurrent(gScreenSets), hash);
        obj = *(u8**)(arg0 + 0x18);
        (*(void (**)(void*, s32))(*(u8**)(obj + 0x138) + 0x2C))(obj, 0x1A);
        fn_8019BBA4(*(u8**)(arg0 + 0x18), set, 0xA);
    }
}

// 0x8003A0A0
// Picks and plays the hit sound for the current value at +0x3B8 against the thresholds at
// +0x3BC (high) and +0x3C0 (mid), storing the sound handle at +0x3C4.
void fn_8003A0A0(u8* arg0) {
    fn_800223DC(arg0);
    s32 v = *(s32*)(arg0 + 0x3B8);
    if (v > *(s32*)(arg0 + 0x3BC)) {
        EnsureVoiceSfxHandle();
        *(s32*)(arg0 + 0x3C4) = PlaySound(0xC6055425u, 0, 0, 0, 1, 0);
        return;
    }
    if (v > *(s32*)(arg0 + 0x3C0)) {
        EnsureVoiceSfxHandle();
        *(s32*)(arg0 + 0x3C4) = PlaySound(0x9A5F41CEu, 0, 0, 0, 1, 0);
        return;
    }
    EnsureVoiceSfxHandle();
    *(s32*)(arg0 + 0x3C4) = PlaySound(0xEA2B9788u, 0, 0, 0, 1, 0);
}

// 0x8004CB1C
// Updates the five attached child objects (slots at +0xC50): each one that is not flagged
// gets its 0x30 hook run, then the arg1 callback under a scoped lock. Afterwards, if the
// owner flag at +0xC87 is set, the two dirty bits are cleared, then the owner is refreshed.
void fn_8004CB1C(u8* arg0, s32 arg1) {
    fn_8005DB64(arg0, arg1);
    for (s32 i = 0; i < 5; i++) {
        u8* obj = *(u8**)(arg0 + 0xC50 + i * 4);
        if (obj != NULL) {
            u8* hdr = *(u8**)(obj + 0x4);
            if (!(*(s32*)(hdr + 0x20) & 0x200000) && !(*(s32*)(obj + 0xB8) & 1)) {
                (*(void (**)(void*, s32, s32))(*(u8**)obj + 0x30))(obj, 0, 0);
            }
            u8* cur = *(u8**)(arg0 + 0xC50 + i * 4);
            ScopedLock lock;
            ScopedLockCtor(&lock);
            (*(void (**)(void*, s32))(*(u8**)cur + 0x14))(cur, arg1);
            ScopedLockDtor(&lock, -1);
        }
    }
    if (*(u8*)(arg0 + 0xC87) != 0) {
        if ((*(u8* (**)(void*))(*(u8**)arg0 + 0xC))(arg0) != NULL) {
            u8* p = (*(u8* (**)(void*))(*(u8**)arg0 + 0xC))(arg0);
            *(s32*)(p + 0x8) &= ~2;
        }
        u8* hdr = *(u8**)(arg0 + 0x4);
        *(s32*)(hdr + 0x20) &= ~2;
    }
    fn_8004B938(arg0);
}

// 0x80051F44
// Looks up the display string of the entry selected by fn_8022E438 in the game database
// (16-byte entries at table +0x4F00): text id from the entry hash, then the string table
// (high bit set = the second table). Falls back to a placeholder when nothing matches.
const char* fn_80051F44(void) {
    const char* str = lbl_805F8748;
    s32 idx = fn_8022E438(*(u8**)lbl_805FBBE0);
    if (idx != -1) {
        u8* table = *(u8**)(*(u8**)((u8*)gGameDb + 0xC) + 0x4F00);
        u8* entry = table + idx * 0x10;
        u32 hash = *(u32*)(entry + 0x4);
        s32 id = TextIdFromHash(hash);
        if (id == -1) {
            str = lbl_80354800;
        } else if ((id & 0x80000000) && (u8*)gStringTable2 != NULL) {
            u8* st2 = (u8*)gStringTable2;
            u8* strs = *(u8**)st2;
            u32 k = (u32)id & 0x7FFFFFFF;
            if (strs != NULL && k < *(u32*)(st2 + 0xC)) {
                str = (const char*)*(u8**)(strs + k * 4);
            } else {
                str = lbl_805F8200;
            }
        } else {
            u8* st = (u8*)gStringTable;
            u8* strs = *(u8**)st;
            if (strs != NULL && (u32)id < *(u32*)(st + 0xC)) {
                str = (const char*)*(u8**)(strs + (u32)id * 4);
            } else {
                str = lbl_805F8200;
                if (id == -1) str = lbl_8035480C;
            }
        }
    }
    return str;
}

// 0x800520F0
// Same lookup as fn_80051F44, using the selector fn_8022E430.
const char* fn_800520F0(void) {
    const char* str = lbl_805F8748;
    s32 idx = fn_8022E430(*(u8**)lbl_805FBBE0);
    if (idx != -1) {
        u8* table = *(u8**)(*(u8**)((u8*)gGameDb + 0xC) + 0x4F00);
        u8* entry = table + idx * 0x10;
        u32 hash = *(u32*)(entry + 0x4);
        s32 id = TextIdFromHash(hash);
        if (id == -1) {
            str = lbl_80354800;
        } else if ((id & 0x80000000) && (u8*)gStringTable2 != NULL) {
            u8* st2 = (u8*)gStringTable2;
            u8* strs = *(u8**)st2;
            u32 k = (u32)id & 0x7FFFFFFF;
            if (strs != NULL && k < *(u32*)(st2 + 0xC)) {
                str = (const char*)*(u8**)(strs + k * 4);
            } else {
                str = lbl_805F8200;
            }
        } else {
            u8* st = (u8*)gStringTable;
            u8* strs = *(u8**)st;
            if (strs != NULL && (u32)id < *(u32*)(st + 0xC)) {
                str = (const char*)*(u8**)(strs + (u32)id * 4);
            } else {
                str = lbl_805F8200;
                if (id == -1) str = lbl_8035480C;
            }
        }
    }
    return str;
}

// 0x8006B6E0
// Returns the position (Vec3) to use for the object: either the one selected by index at +0x17A
// (via fn_801B55F4 when the index is in range, or the entry at +0x98 when the second flag is set),
// or the object own position at +0x18. The selected position is copied into a scratch buffer
// (the original returns a pointer into its stack frame; the caller reads it immediately).
f32* fn_8006B6E0(u8* arg0) {
    static f32 sBuf[12];
    u16 flags = *(u16*)(arg0 + 0x178);
    s32 idx = (flags & 1) ? *(u8*)(arg0 + 0x17A) : -1;
    s32 found;
    if (idx != -1) {
        if (idx < (s32)(intptr_t)fn_801B55A8(arg0)) { // lint:ok (asm compares as signed int)
            fn_801B55F4(arg0, (u8*)sBuf, idx);
            found = 1;
        } else {
            found = 0;
        }
    } else {
        s32 idx2 = (flags & 2) ? *(u8*)(arg0 + 0x17A) : -1;
        if (idx2 != -1) {
            u8* t = *(u8**)(arg0 + 0x98) + idx2 * 0xC8;
            for (s32 k = 0; k < 12; k++) {
                sBuf[k] = *(f32*)(t + 0x38 + k * 4);
            }
            found = 1;
        } else {
            found = 0;
        }
    }
    if (found != 0) {
        return &sBuf[9];
    }
    return (f32*)(arg0 + 0x18);
}

// 0x8006D574
// Sets up the camera trigger state of the object at arg0 for the given stage: the timer
// reset, a per-kind start value, the end-camera trigger and the final update call.
void fn_8006D574(u8* arg0, s32 arg1, s32 arg2) {
    *(s32*)(arg0 + 0x1AC) = 0;
    *(s32*)(arg0 + 0x20C) = 2;
    s32 kind = *(s32*)(*(u8**)(arg0 + 0x9C) + 0x8);
    if (kind == (s32)0xBF9D7EB0 || kind == (s32)0xBC2C83D0) {
        *(f32*)(arg0 + 0x15C) = (f32)*(u8*)(*(u8**)(arg0 + 0x4) + 0x3C);
    } else if (kind == 0x6DB80004) {
        *(s32*)(arg0 + 0x158) = 0;
    }
    if (arg2 != 0) {
        if (*(s32*)(*(u8**)(arg0 + 0x4) + 0x2C) & 1) {
            TriggerEndCamera((Character*)arg0, 0);
        } else {
            TriggerEndCamera((Character*)arg0, 1);
        }
    } else {
        TriggerEndCamera((Character*)arg0, *(s32*)(arg0 + 0x1B0) & 1);
    }
    fn_80062DC4(arg0, arg1, arg2);
}

}  // extern "C"
