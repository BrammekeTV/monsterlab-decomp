#include "types.h"

extern "C++" void* gScreenSets;                                 // 0x805FB654
extern "C++" s32 RobotEvolutionExperiment_SmasherCheckImpactFrame(u8* obj); // small_8009c.cpp (C++ linkage)
extern "C++" s32 RobotEvolutionExperiment_SmasherHitSomething(u8* obj);     // small_8009c.cpp (C++ linkage)

extern "C" {
s32   RegisterScriptFunc(const char* name, s32 type, void* fn); // 0x801DC91C
void* ScreenSetsCurrent(void* sets);                            // 0x8018D1E4
void* UIWidgetFindChild(void* widget, u32 hash);                // 0x80199AEC
u8*   fn_8006FBDC(void);
s32   fn_8006FBFC(u8* obj);
u8*   fn_8022014C(u8* list, s32 idx);                           // leaf file declares u32 return; asm returns a pointer
u8*   fn_8021F350(u8* p, s32 idx);
void  fn_80076584(u8* mgr, s32 idx, u8* item);                  // 0x80076584
void  fn_800765D0(u8* mgr, s32 idx);                            // 0x800765D0
u8*   fn_800C73EC(u8* h);                                       // 0x800C73EC (leaf file declares s32 return)
void  fn_801C83AC(u8* mgr, u8* item);                           // 0x801C83AC
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner); // 0x801C80E0; 4th arg is a pointer in the asm
int   SysSprintf(char* buf, const char* fmt, ...);
u8*   fn_801094F4(char* name, s32 flags);                       // 0x801094F4
void  fn_8018D2B8(const char* fmt, ...);                        // 0x8018D2B8
u8*   fn_8019BF64(u8* widget, u32 hash);                        // 0x8019BF64
void  RobotEvolutionExperiment_SmasherIdling(u8* obj);         // small_8009b.cpp
void  RobotEvolutionExperiment_SmasherDropping(u8* obj);       // small_8009b.cpp
void  RobotEvolutionExperiment_SmasherRaising(u8* obj);        // small_8009b.cpp
void  RobotEvolutionExperiment_SmasherHit(u8* obj);            // not defined in repo yet, signature assumed
void  RobotEvolutionExperiment_SmasherMiss(u8* obj);           // not defined in repo yet, signature assumed

extern u8  lbl_8035C0A0[];          // smasher callback names
extern u8  lbl_8047C760[];          // effect manager
extern const char* lbl_805F8F90;    // string prefix pointer
extern const char  lbl_8036D2E4[];  // format string
extern const char  lbl_8036D2F8[];  // format string
extern const char  lbl_80383760[];  // debug log string
extern f32 lbl_805FB168;
extern f32 lbl_805FDEB4;
extern f32 lbl_805FDEB8;
extern f32 lbl_805FE468;
extern f32 lbl_805FE618;
}

extern "C" {

// 0x80077948
// For each of the 4 groups under p+0x1C, pushes every entry through fn_80076584 when the manager is ready.
void fn_80077948(u8* p) {
    u8* mgr = fn_8006FBDC();
    if (mgr == NULL || fn_8006FBFC(mgr) != 1) {
        return;
    }
    for (u32 i = 0; i < 4U; i++) {
        u8* list = fn_8022014C(*(u8**)(p + 0x1C), (s32)i);
        s32 n = *(s32*)(list + 0x1000);
        for (u32 j = 0; j < (u32)n; j++) {
            fn_80076584(mgr, (s32)i, fn_8021F350(list, (s32)j));
        }
    }
}

// 0x800779F4
// Same walk as fn_80077948 over the group list at p+0x20, calling fn_800765D0 per entry.
void fn_800779F4(u8* p) {
    u8* mgr = fn_8006FBDC();
    if (mgr == NULL || fn_8006FBFC(mgr) != 1) {
        return;
    }
    for (u32 i = 0; i < 4U; i++) {
        u8* list = fn_8022014C(*(u8**)(p + 0x20), (s32)i);
        s32 n = *(s32*)(list + 0x1000);
        for (u32 j = 0; j < (u32)n; j++) {
            fn_800765D0(mgr, (s32)i);
        }
    }
}

// 0x80093934
// Registers the smasher script callbacks from the name table lbl_8035C0A0.
s32 fn_80093934(void) {
    RegisterScriptFunc((const char*)(lbl_8035C0A0 + 0x00), 0x4E, (void*)RobotEvolutionExperiment_SmasherIdling);
    RegisterScriptFunc((const char*)(lbl_8035C0A0 + 0x28), 0x4E, (void*)RobotEvolutionExperiment_SmasherDropping);
    RegisterScriptFunc((const char*)(lbl_8035C0A0 + 0x58), 0x4E, (void*)RobotEvolutionExperiment_SmasherRaising);
    RegisterScriptFunc((const char*)(lbl_8035C0A0 + 0x80), 0x4E, (void*)RobotEvolutionExperiment_SmasherHit);
    RegisterScriptFunc((const char*)(lbl_8035C0A0 + 0xA4), 0x4E, (void*)RobotEvolutionExperiment_SmasherMiss);
    RegisterScriptFunc((const char*)(lbl_8035C0A0 + 0xCC), 0x1D, (void*)RobotEvolutionExperiment_SmasherCheckImpactFrame);
    return RegisterScriptFunc((const char*)(lbl_8035C0A0 + 0x100), 0x1D, (void*)RobotEvolutionExperiment_SmasherHitSomething);
}

// 0x800A7960
// Writes lbl_805FDEB4 + lbl_805FDEB8 * lbl_805FB168 to the float at the fourth argument.
s32 fn_800A7960(void* a0, void* a1, void* a2, f32* out) {
    *out = lbl_805FDEB4 + lbl_805FDEB8 * lbl_805FB168;
    return 0;
}

// 0x800C79C0
// Finds the first entry (stride 0x14) whose state at +0xFC4 is outside 2..5, sets its sub-object's four floats to lbl_805FE468, and returns it.
u8* fn_800C79C0(u8* arg0) {
    s32 count = *(s32*)(arg0 + 0x178C);
    for (s32 i = 0; i < count; i++) {
        if ((u32)(*(s32*)(arg0 + i * 0x14 + 0xFC4) - 2) > 3U) {
            u8* slot = arg0 + i * 0x14 + 0xFB4;
            u8* obj = fn_800C73EC(slot);
            u8* t = *(u8**)(obj + 0x90);
            *(f32*)(t + 0x38) = lbl_805FE468;
            *(f32*)(t + 0x3C) = lbl_805FE468;
            *(f32*)(t + 0x40) = lbl_805FE468;
            *(f32*)(t + 0x44) = lbl_805FE468;
            return slot;
        }
    }
    return NULL;
}

// 0x800D5B1C
// Clears the effect slot at sub+0x50 (releasing it through fn_801C83AC) and spawns an effect at the given position.
void fn_800D5B1C(u8* arg0, s32 arg1, f32* arg2) {
    u8* sub = *(u8**)(*(u8**)arg0 + 0x14);
    if (sub != NULL) {
        if (*(u8**)(sub + 0x50) != NULL) {
            fn_801C83AC(lbl_8047C760, *(u8**)(sub + 0x50));
            *(s32*)(*(u8**)(*(u8**)arg0 + 0x14) + 0x50) = 0;
        }
        f32 vec[4];
        vec[0] = arg2[0];
        vec[1] = arg2[1];
        vec[2] = arg2[2];
        vec[3] = lbl_805FE618;
        SpawnEffectAt(lbl_8047C760, (u32)arg1, vec, (u8**)(*(u8**)(*(u8**)arg0 + 0x14) + 0x50));
    }
}

// 0x8011ED38
// Builds "quest_icon_<n>" (falls back to the placeholder name) and looks it up.
u8* fn_8011ED38(s32 arg0) {
    char name[0x100];
    SysSprintf(name, lbl_8036D2E4, lbl_805F8F90, (s32)(s8)arg0);
    u8* r = fn_801094F4(name, 0);
    if (r == 0) {
        SysSprintf(name, lbl_8036D2F8, lbl_805F8F90);
        r = fn_801094F4(name, 0);
    }
    return r;
}

// 0x80121440
// Packs four byte channels into the word at +8: a in the top byte, then b, c, d.
void fn_80121440(u8* obj, s32 a, s32 b, s32 c, s32 d) {
    *(u32*)(obj + 0x8) = ((u32)(u8)a << 24) | ((u32)(u8)b << 16) | ((u32)(u8)c << 8) | (u32)(u8)d;
}

// 0x80198E68
// Finds a widget by hash in the current screen set's list (+0x40, count +0x60), else checks the set's root widget at +0x7C.
void* fn_80198E68(void* set, u32 hash) {
    u8* sets = (u8*)ScreenSetsCurrent(gScreenSets);
    u8* found = NULL;
    s32 count = *(s32*)(sets + 0x60);
    for (s32 i = 0; i < count; i++) {
        found = (u8*)UIWidgetFindChild(*(void**)(sets + 0x40 + i * 4), hash);
        if (found != NULL) {
            break;
        }
    }
    if (found == NULL) {
        u8* w = *(u8**)((u8*)gScreenSets + 0x7C);
        if (w != NULL) {
            if (hash != *(u32*)w) {
                w = fn_8019BF64(w, hash);
            }
            found = w;
        }
    }
    return found;
}

// 0x801A9EFC
// Walks the linked list at +0x120 (next at +0x124, type at +0x60) for the given type; logs and returns NULL if not found.
u8* fn_801A9EFC(u8* arg0, s32 arg1) {
    u8* p = *(u8**)(arg0 + 0x120);
    u8* next;
    while ((next = *(u8**)(p + 0x124)) != NULL && arg1 != *(s32*)(p + 0x60)) {
        p = next;
    }
    if (arg1 != *(s32*)(p + 0x60)) {
        fn_8018D2B8(lbl_80383760);
        return NULL;
    }
    return p;
}

// 0x80222704
// Returns the slot at (a, b) of the 4x3 table at +0x5B8 (stride 0x3C by row, 0x14 by column), or NULL if out of range.
u8* fn_80222704(u8* p, s32 a, s32 b) {
    if (a <= -1 || a >= 4 || b < 0 || b >= 3) {
        return NULL;
    }
    return p + a * 0x3C + b * 0x14 + 0x5B8;
}

// 0x8022E2F0
// Scans rows from the last to the first (rows at +0xB4, stride 0x3C, flag at +0x5C8) and returns the first slot whose flag is positive.
u8* fn_8022E2F0(u8* p) {
    for (s32 i = 0; i < 4; i++) {
        u8* e = p + 0xB4 - i * 0x3C;
        if (*(s32*)(e + 0x5C8) > 0) {
            return p + (3 - i) * 0x3C + 0x5B8;
        }
    }
    return NULL;
}

}  // extern "C"
