// Sound and effect script callbacks (monster voices, spray, particles, sound FX dispatch), plus
// a few small UI and battle helpers. Verified against the asm.
#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
// Callees (prototypes copied from their definitions in other files).
void    CharacterStartReaction(void* ch);                                   // 0x8004E3D0
s32     PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);             // 0x80209E70
s32     PlaySoundAt(void* sound, int a, const void* pos, int b);            // 0x80209ABC
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner); // 0x801C80E0
u8*     fn_80021750(void);
s32     fn_8004E2A4(u8* fighters, s32 slot);
void    fn_8004F310(void* slot, s32 idx, s32 a, s32 b);
void*   fn_801D40E0(s32 n);
void*   fn_80209814(void* v);
void    fn_801B6878(void* a0, u32 a1, s32 a2, s32 a3);
void    GameLog(int level, int category, const char* fmt, ...);             // 0x8000B2F0

extern u8    gGame[];
extern u8*   gBattle;
extern u8    lbl_8047C760[];
extern void* lbl_805FB1C8;
extern s32   lbl_805FB430;
extern void* lbl_805FBBAC;
extern u8*   lbl_805FBBE0;
extern const char lbl_805F9648[];
}

// Virtual call signatures used by the sound dispatchers below (vtable slot at 0xA4 / 0xA8 / 0xAC).
typedef void (*SoundFn3)(void* self, u32 a1, s32 a2);
typedef void (*SoundGroupFn4)(void* self, u32 a1, s32 a2, s32 a3);
typedef void (*SoundGroupFn5)(void* self, u32 a1, s32 a2, s32 a3, s32 a4);
typedef u8* (*GetPtrFn)(void* self);
typedef void (*MonsterVoiceFn)(void* self);

extern "C" {

// Lazily creates the shared voice SFX handle (lbl_805FBBAC) on first use.
static void VoiceSfxEnsureHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
}

// 0x8010FBEC
// Sets the byte at +0x2E of both sound objects to a2, then plays sound a0 on channel 6 and
// sound a1 on channel 7.
void PlayBackgroundSFXWithEffect(u32 a0, u32 a1, s32 a2) {
    u8* s0 = (u8*)(uintptr_t)a0;
    u8* s1 = (u8*)(uintptr_t)a1;
    s0[0x2E] = (u8)a2;
    s1[0x2E] = (u8)a2;
    VoiceSfxEnsureHandle();
    PlaySoundAt(s0, 0, NULL, 6);
    VoiceSfxEnsureHandle();
    PlaySoundAt(s1, 0, NULL, 7);
}

// 0x800EFE80
// Calls the failure-voice method (slot 0x64) of the object returned by fn_80021750.
void PlayMonsterFailureVoice(void) {
    u8* voice = fn_80021750();
    (*(MonsterVoiceFn*)(*(u8**)(voice + 0x4) + 0x64))(voice);
}

// 0x800EFE50
// Calls the success-voice method (slot 0x60) of the object returned by fn_80021750.
void PlayMonsterSuccessVoice(void) {
    u8* voice = fn_80021750();
    (*(MonsterVoiceFn*)(*(u8**)(voice + 0x4) + 0x60))(voice);
}

// 0x800F4B08
// Spawns an effect at the position of object a0 (its +0x4 pointer) plus the offset (a2, a3, a4).
void PlayParticleEffectAtOffsetFromFeet(void* a0, u32 a1, f32 a2, f32 a3, f32 a4) {
    if (a0 != NULL) {
        u8* feet = *(u8**)((u8*)a0 + 0x4);
        f32 pos[4];
        pos[0] = *(f32*)(feet + 0x0) + a2;
        pos[1] = *(f32*)(feet + 0x4) + a3;
        pos[2] = *(f32*)(feet + 0x8) + a4;
        pos[3] = 0.0f;
        SpawnEffectAt(lbl_8047C760, a1, pos, 0);
    }
}

// 0x800E936C
// For the player's fighter group, sets each limb's mode (3 unless fn_8004E2A4 returns <= 0, then
// 2) on every present limb except index 1, then starts the reaction.
void PlayPlayerAttackAnimation(void) {
    u8* fighters = *(u8**)(gGame + 0xD614);
    if (fighters != NULL) {
        s32 mode = 3;
        if (fn_8004E2A4(fighters, 3) <= 0) mode = 2;
        for (s32 i = 0; i < 5; i++) {
            u8* limb = *(u8**)(fighters + 0xC50 + i * 4);
            if (limb != NULL && i != 1) fn_8004F310(limb, 1, mode, 0);
        }
        CharacterStartReaction(fighters);
    }
}

// 0x8010FA4C
// Plays sound a0 on the voice channel. Returns 0 without playing if the sound is flagged (bit 0
// of +0x5).
s32 PlaySoundEffect(u32 a0) {
    u8* snd = (u8*)(uintptr_t)a0;
    if (snd != NULL && (snd[0x5] & 1)) return 0;
    VoiceSfxEnsureHandle();
    return PlaySoundAt(snd, 0, NULL, 0);
}

// 0x8010FAC8
// Like PlaySoundEffect, but first stores a1 (low byte) at +0x2E of the sound.
s32 PlaySoundEffectWithEffect(u32 a0, s32 a1) {
    u8* snd = (u8*)(uintptr_t)a0;
    if (snd != NULL) {
        if (snd[0x5] & 1) return 0;
        snd[0x2E] = (u8)a1;
    }
    VoiceSfxEnsureHandle();
    return PlaySoundAt(snd, 0, NULL, 0);
}

// 0x800EFF54
// Plays the spray sound.
void PlaySpraySound(void) {
    VoiceSfxEnsureHandle();
    PlaySound(0xFC1935D9u, 0, 0, 0, 0, 0);
}

// 0x800F9378
// Sets the byte at +0xC85 of the object reached through a0's method at slot 0xCC and its
// +0x24 / +0x1C pointers (minus 0xC48), unless gGame flag +0x24 is set or the battle's slot at
// +0xA0C holds 3.
void PlaySwitchBackAnimation(void* a0) {
    if (gGame[0x24] != 0) return;
    u8* st = NULL;
    if (gBattle != NULL) st = *(u8**)(gBattle + 0xA0C);
    if (st == NULL || *(s32*)st != 3) {
        u8* x = (*(GetPtrFn*)(*(u8**)a0 + 0xCC))(a0);
        u8* p = *(u8**)(*(u8**)(x + 0x24) + 0x1C);
        if (p != NULL) p -= 0xC48;
        *(u8*)(p + 0xC85) = 1;
    }
}

// 0x8013FAAC
// Pops the pushed matchup screen (hash lbl_805FB430), if any.
void PopOldScreen(void) {
    if (lbl_805FB430 != 0) {
        UIScreen_PopScreen((u32)lbl_805FB430);
        lbl_805FB430 = 0;
    }
}

// 0x800E748C
// Returns 1 if the last matched level-up record (by id at +0x75C of the save records, 0x1C each,
// count at +0x8E8) has state 1 at +0x744, else 0.
u32 /*0x06*/ Popup_IsPlayingLevelUpEvent(void) {
    u8* save = lbl_805FBBE0;
    s32 count = *(s32*)(save + 0x8E8);
    s32 found = -1;
    for (s32 i = 0; i < count; i++) {
        u32 id = *(u32*)(save + i * 0x1C + 0x75C);
        if (*(u32*)((u8*)lbl_805FB1C8 + 0x24) == id) found = i;
    }
    if (found > -1) {
        s32 state = *(s32*)(save + found * 0x1C + 0x744);
        return state == 1;
    }
    return 0;
}

// 0x801DB498
// Logs a0 as a string at level 1, category 3. Does nothing if a0 is NULL.
void Print(u32 a0) {
    if (a0 == 0) return;
    GameLog(1, 3, lbl_805F9648, (const char*)(uintptr_t)a0);
}

// 0x801DDDDC
// Sets the byte at +0x2E of sound a1 to a3, then tail-calls fn_801B6878.
void PlayLoopingSoundFXWithEffect(void* a0, u32 a1, s32 a2, s32 a3) {
    if (a0 == NULL) return;
    *(u8*)((u8*)(uintptr_t)a1 + 0x2E) = (u8)a3;
    fn_801B6878(a0, a1, a2, a3);
}

// 0x801DDDF0
void PlayLoopingSoundGroupFX(void* a0, u32 a1, s32 a2) {
    if (a0 == NULL) return;
    (*(SoundGroupFn4*)(*(u8**)a0 + 0xA8))(a0, a1, 1, a2);
}

// 0x801DDE14
void PlayLoopingSoundGroupFXWithEffect(void* a0, u32 a1, s32 a2, s32 a3) {
    if (a0 == NULL) return;
    (*(SoundGroupFn5*)(*(u8**)a0 + 0xAC))(a0, a1, 1, a2, a3);
}

// 0x801DDE3C
void PlaySoundFX(void* a0, u32 a1, s32 a2) {
    (void)a2;
    if (a0 == NULL) return;
    (*(SoundFn3*)(*(u8**)a0 + 0xA4))(a0, a1, 0);
}

// 0x801DDE5C
void PlaySoundFXWithEffect(void* a0, u32 a1, s32 a2) {
    if (a0 == NULL) return;
    *(u8*)((u8*)(uintptr_t)a1 + 0x2E) = (u8)a2;
    (*(SoundFn3*)(*(u8**)a0 + 0xA4))(a0, a1, 0);
}

// 0x801DDE80
void PlaySoundGroupFX(void* a0, u32 a1, s32 a2) {
    if (a0 == NULL) return;
    (*(SoundGroupFn4*)(*(u8**)a0 + 0xA8))(a0, a1, 0, a2);
}

// 0x801DDEA4
void PlaySoundGroupFXWithEffect(void* a0, u32 a1, s32 a2, s32 a3) {
    if (a0 == NULL) return;
    (*(SoundGroupFn5*)(*(u8**)a0 + 0xAC))(a0, a1, 0, a2, a3);
}

// 0x800E959C
// No NULL check in the asm: dispatches through a0's vtable at 0xA8 with flag (a2 != 0).
void PlayEntitySoundGroupFX(void* a0, u32 a1, u32 /*0x06*/ a2) {
    (*(SoundGroupFn4*)(*(u8**)a0 + 0xA8))(a0, a1, a2 != 0, 0);
}

}  // extern "C"
