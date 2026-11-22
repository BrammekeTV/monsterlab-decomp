// Prop and quest script callbacks, small math helpers and debug print helpers (0x800Exxxx, 0x801Dxxxx).
#include "types.h"
#include "game/node_ref.h"
#include "engine/math.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern void* lbl_805FB1C8;          // 0x805FB1C8 (quest counter state record pointer)
extern u8 lbl_80360334[];           // 0x80360334 (level-name log format string)

typedef s32 (*Fn)(void*);

extern "C" {
void  TextureRelease(void* tex);                                    // 0x80184028
void  WaypointParamsInit(void* params, int a);                      // 0x801ED314
void  GameLog(int level, int category, const char* fmt, ...);      // 0x8000B2F0
f32   cosf(f32 x);                                                  // 0x8033EC98 (MSL)
f32   sinf(f32 x);                                                  // 0x8033F0A4 (MSL)
void  fn_8006B808(u8* a0, u32 a1, s32* a2);                         // 0x8006B808
void  fn_800ECCCC(void* a0, u32 a1, void* a2, void* a3, s32 a4, s32 a5, f32 a6);  // 0x800ECCCC
u32   fn_801EC880(u8* p, u32 hash);                                 // 0x801EC880
void  fn_801EE078(void* params, u8* path, s32 a);                   // 0x801EE078
s32   fn_801098DC(s32 a0);                                          // 0x801098DC
s32   fn_80109A0C(s32 id);                                            // 0x80109A0C
u8*   fn_801B6E74(u8* a3);                                          // 0x801B6E74
}

extern "C" {

// 0x801DDEDC
// Debug print helper: runs fn_801B6E74 on each non-null pointer argument.
void Print_v_E_E_i_f(void* a0, void* a1, s32 a2, f32 a3) {
    if (a0 != NULL) {
        fn_801B6E74((u8*)a0);
    }
    if (a1 != NULL) {
        fn_801B6E74((u8*)a1);
    }
}

// 0x800ED1CC
// Attaches a projectile: copies the s32 at a2 into a local and passes it on to fn_8006B808.
void PropAttachProjectile(u32 a0, u32 a1, u32 a2) {
    s32 sp8 = *(s32*)a2;
    fn_8006B808((u8*)a0, a1, &sp8);
}

// 0x800EF448
// Fires lightning at the active target of the prop. Only runs when the prop's target object
// (at +0x18C) exists and its virtual +0x4C check returns 0.
void PropFireLightningAtActiveTarget(u32 a0, u32 a1, u32 a2, u32 a3, f32 a4, f32 a5) {
    u8* self = (u8*)a0;
    u32 wpWord;  // node reference word (kind 0, index 0xFFFF)
    u8* wp = (u8*)&wpWord;
    s32 spC;
    s32 sp8;
    u8* obj = *(u8**)(self + 0x18C);

    if (obj != NULL && (*(Fn*)(*(u8**)obj + 0x4C))(obj) == 0) {
        wpWord = 0x0000FFFFu;
        WaypointParamsInit(wp, 6);
        fn_801EE078(wp, *(u8**)(self + 0x18C), 0);
        sp8 = *(s32*)wp;
        spC = *(s32*)a1;
        FireLightningAtTarget(self, (u32)(uintptr_t)&spC, a2, a3, a4, a5, (u32)(uintptr_t)&sp8);  // lint:ok header takes u32 for these pointer args
    }
}

// 0x800ED200
// Fires a projectile at the active target. Looks up the target handle from the prop's
// sub-record (when the flag word at a2 has bit 0 clear) and stores the result back into a2.
void PropFireProjectileAtActiveTarget(u32 a0, u32 a1, u32 a2, f32 a3) {
    u8* self = (u8*)a0;
    u8* ap = (u8*)a2;
    u8* sub = *(u8**)(self + 0x4);
    u8* arr = *(u8**)(self + 0x1A0);
    u8* temp_r4 = *(u8**)(arr + (*(u8*)(sub + 0x35)) * 4);
    u32 var_r30 = a1;
    s32 var_r0;
    s32 hit;
    s32 sp8;

    if (a1 == 0) {
        var_r30 = *(u32*)(temp_r4 + 0xC4);
    }
    if ((*(u16*)ap & 1) != 0) {
        var_r0 = *(u8*)(ap + 0x2);
    } else {
        var_r0 = -1;
    }
    if (var_r0 < 0) {
        hit = (s32)fn_801EC880(self + 0x94, *(u32*)(temp_r4 + 0xCC));
        *(s32*)ap = 0;
        if (hit >= 0 && hit < 0x100) {
            *(u8*)(ap + 0x2) = (u8)hit;
            *(u16*)ap = (u16)((*(u16*)ap | 2) & 0xFFFE);
        }
    }
    sp8 = *(s32*)ap;
    fn_800ECCCC(self, var_r30, *(void**)(self + 0x18C), &sp8, 0, 0, a3);
}

// 0x80145EE8
// Builds a quaternion from an axis and an angle: w = cos(half angle), xyz = sin(half angle) * axis.
void QuatFromAxisAngle(Quat* arg0, const Vec3* arg1, f32 farg0) {
    f32 temp_f31 = 1.5707964f * (0.5f * farg0);
    f32 temp_f30 = (f32)cosf(temp_f31);
    f32 temp_f0 = (f32)sinf(temp_f31);

    arg0->w = temp_f30;
    arg0->x = temp_f0 * arg1->x;
    arg0->y = temp_f0 * arg1->y;
    arg0->z = temp_f0 * arg1->z;
}

// 0x800E695C
// Clears the quest counter state: resets the flag at +0x1D, releases the texture at +0x8 when its
// refcount hits zero, and frees the handle at +0xC.
void QuestCounterCleanUp(void) {
    u8* st = (u8*)lbl_805FB1C8;
    s32 temp_r0;
    u8* temp_r3;

    st[0x1D] = 0;
    temp_r3 = *(u8**)(st + 0x8);
    if (temp_r3 != NULL) {
        temp_r0 = *(s32*)(temp_r3 + 0x24) - 1;
        *(s32*)(temp_r3 + 0x24) = temp_r0;
        if (temp_r0 == 0) {
            TextureRelease(temp_r3);
        }
        *(void**)(st + 0x8) = NULL;
    }
    if (*(s32*)(st + 0xC) != 0) {
        if (fn_80109A0C(*(s32*)(st + 0xC)) == 0) {
            fn_801098DC(*(s32*)(st + 0xC));
        }
        *(s32*)(st + 0xC) = 0;
    }
}

// 0x800F001C
// Logs the level name for a level index; does nothing for indices up to 0xFF.
void PrintLevelName(u32 a0) {
    if (a0 > 0xFF) {
        GameLog(4, 3, (const char*)lbl_80360334, a0);
    }
}

}  // extern "C"
