// More small functions in 0x8020xxxx (audio listener/music, discipline letter).
#include "types.h"
#include "engine/math.h"   // Vec3

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {

// Callee prototypes (C linkage, matching their definitions elsewhere or the asm call shape).
void  operator_delete(void* p);                                  // 0x801D426C
void* fn_801D40E0(s32 n);                                        // 0x801D40E0
void  fn_8020F3D4(u8* p, s32 a2);                                // 0x8020F3D4
void  fn_8020F42C(u8* p, s32 del);                               // 0x8020F42C
void  fn_8020F46C(u8* p, u8* choice);                            // 0x8020F46C
void* ChoiceDestroy(void* c, s32 del);                           // 0x8020F394
void* fn_8020BE20(void* obj, s32 a2);                            // 0x8020BE20 (return type assumed: the object)
void  fn_8020BFBC(u8* out, u8* src);                             // 0x8020BFBC (argument types assumed)
void  StrCopy(char* dst, const char* src);                       // 0x8022127C
void  fn_801FD8EC(u8* audio);                                    // 0x801FD8EC
void  fn_801FD904(u8* audio);                                    // 0x801FD904
s32   fn_801FF620(u8* audio, s32 id);                            // 0x801FF620 (return type assumed)
void  fn_801FF6A4(u8* audio, s32 id, u8* cb);                    // 0x801FF6A4
void  fn_801FFC04(u8* audio, f32 v);                             // 0x801FFC04
void  fn_801FFDB0(u8* audio, f32 v);                             // 0x801FFDB0
void  fn_801FFF24(u8* audio, f32 v);                             // 0x801FFF24
void  fn_802097E4(f32 f1);                                       // 0x802097E4 (defined in the leaf file)

extern u8  gAudio[];                 // audio manager object
extern u8  lbl_80559D78[];           // 0x80559D78 (0x28-byte audio voice records, indexed by handle & 0xFFFFF)
extern u8  lbl_8055C978[];           // 0x8055C978 (Vec3 listener positions, 12 bytes per id)
extern u8  lbl_805FBBB4[];           // 0x805FBBB4 (listener-position valid flag per id)
extern u8  lbl_805FBBB0;             // 0x805FBBB0 (music paused flag)
extern u8  lbl_805FBBB3;             // 0x805FBBB3 (music enabled flag)
extern s32 lbl_805FB6B8;             // 0x805FB6B8
extern f32 lbl_805F9978;             // 0x805F9978 (last clamped level)
extern f32 lbl_806004F0;             // 0x806004F0 (lower bound for the clamped levels)
extern f32 lbl_806004F4;             // 0x806004F4 (upper bound for the clamped levels)
extern u8  lbl_80387660[];           // 0x80387660 (vtable of the child object)
extern const char lbl_80387640[];    // 0x80387640 (string copied into the object)
extern const char lbl_80387D40[];    // 0x80387D40 (fallback string for out-of-range lookups)
extern u8  lbl_80349314[];           // 0x80349314 (5 string pointers)
extern u8  lbl_80349328[];           // 0x80349328 (186 string pointers)
extern u8  lbl_80349610[];           // 0x80349610 (3 string pointers, discipline letters)
extern u8  lbl_80388D00[];           // 0x80388D00 (3 StringHash values)

// Clamps v to [lbl_806004F0, lbl_806004F4] with the same NaN behaviour as the original compares.
static f32 ClampToRange(f32 v) {
    f32 lo = lbl_806004F0;
    f32 hi = lbl_806004F4;
    if (!(v >= lo)) return lo;
    if (!(v <= hi)) return hi;
    return v;
}

// 0x8020A348
// Voice handles tagged 0x0AB (top 12 bits) store cb in their record's +0x20 slot.
// Any other non-negative id is forwarded to fn_801FF6A4 on the audio manager.
void fn_8020A348(s32 id, u8* cb) {
    if (((u32)id & 0xFFF00000u) == 0x0AB00000u) {
        u8* rec = lbl_80559D78 + ((u32)id & 0xFFFFFu) * 0x28;
        *(void**)(rec + 0x20) = cb;
        return;
    }
    if (id < 0) return;
    fn_801FF6A4(gAudio, id, cb);
}

// 0x8020A958
// Resets a 0x28-byte voice record: ids to -1, a level float to its default, counters to 0.
void fn_8020A958(u8* p) {
    f32 dflt = lbl_806004F0;
    *(s32*)(p + 0x00) = 0;
    *(s32*)(p + 0x04) = lbl_805FB6B8;
    *(f32*)(p + 0x08) = dflt;
    *(f32*)(p + 0x0C) = dflt;
    *(f32*)(p + 0x10) = dflt;
    *(s32*)(p + 0x14) = -1;
    *(f32*)(p + 0x18) = dflt;
    *(s32*)(p + 0x1C) = -1;
    *(s32*)(p + 0x20) = 0;
    *(s32*)(p + 0x24) = 0;
}

// 0x8020A9A8
// For a 0x0AB-tagged voice handle: returns 1 if its record's +0x14 id is set (not -1), else 0.
// Other non-negative ids go to fn_801FF620 on the audio manager; negative ids return 0.
s32 fn_8020A9A8(s32 id) {
    if (((u32)id & 0xFFF00000u) == 0x0AB00000u) {
        s32 st = *(s32*)(lbl_80559D78 + ((u32)id & 0xFFFFFu) * 0x28 + 0x14);
        return st != -1;
    }
    if (id < 0) return 0;
    return fn_801FF620(gAudio, id);
}

// 0x8020AA04
// Stores the position for listener id (3 floats) and marks that id's listener-position flag as set.
void AudioSetListenerPos(int id, const Vec3* v) {
    u8* dst = lbl_8055C978 + id * 12;
    *(f32*)(dst + 0) = v->x;
    *(f32*)(dst + 4) = v->y;
    *(f32*)(dst + 8) = v->z;
    lbl_805FBBB4[id] = 1;
}

// 0x8020AFB0
// Pauses or resumes the music (script callback). Does nothing unless the music-enabled flag is set.
// The original also returns the previous paused flag in r3; the script header declares void.
void PauseMusic(u32 a0) {
    if (lbl_805FBBB3 == 0) return;
    if (a0 != 0) {
        fn_801FD8EC(gAudio);
    } else {
        fn_801FD904(gAudio);
    }
    lbl_805FBBB0 = (u8)a0;
}

// 0x8020B024
// Clamps the level to [lbl_806004F0, lbl_806004F4], stores it in lbl_805F9978 and sets it on the audio manager.
void fn_8020B024(f32 v) {
    f32 c = ClampToRange(v);
    lbl_805F9978 = c;
    fn_801FFC04(gAudio, c);
}

// 0x8020B078
// Clamps the level to the same range and sets it through fn_801FFDB0 on the audio manager.
void fn_8020B078(f32 v) {
    f32 c = ClampToRange(v);
    fn_801FFDB0(gAudio, c);
}

// 0x8020B160
// Clamps the level to the same range and sets it through fn_801FFF24 on the audio manager.
void fn_8020B160(f32 v) {
    f32 c = ClampToRange(v);
    fn_801FFF24(gAudio, c);
}

// 0x8020B194
// Clamps the level to the same range and passes it to fn_802097E4 (which clamps again and stores it).
void fn_8020B194(f32 v) {
    f32 c = ClampToRange(v);
    fn_802097E4(c);
}

// 0x8020BF7C
// Deleting helper: frees p when del > 0. Returns p.
u8* fn_8020BF7C(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x8020BCEC
// Constructor: sets the vtable, allocates a 0x134-byte child at +0x4C (initialised through fn_8020BE20
// when the allocation succeeds), and copies a name string into +0x0C. Returns p.
u8* fn_8020BCEC(u8* p, s32 a2) {
    fn_8020F3D4(p, a2);
    *(void**)p = lbl_80387660;
    void* child = fn_801D40E0(0x134);
    if (child) child = fn_8020BE20(child, a2);
    *(void**)(p + 0x4C) = child;
    StrCopy((char*)(p + 0x0C), lbl_80387640);
    return p;
}

// 0x8020BD60
// Deleting destructor: deletes the child at +0x4C, runs the base destructor, and frees p when del > 0.
// Returns p.
u8* fn_8020BD60(u8* p, s32 del) {
    if (p) {
        *(void**)p = lbl_80387660;
        fn_8020BF7C(*(u8**)(p + 0x4C), 1);
        fn_8020F42C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x8020BDD4
// Reads the choice held by the child at +0x4C and commits it into p (then releases the temporary).
void fn_8020BDD4(u8* p) {
    u8 choice[12];   // size inferred from the ChoiceDestroy use elsewhere
    fn_8020BFBC(choice, *(u8**)(p + 0x4C));
    fn_8020F46C(p, choice);
    ChoiceDestroy(choice, -1);
}

// 0x8020E3D8
// Returns the string for index idx in the 5-entry table, or the fallback string when idx is out of range.
const char* fn_8020E3D8(s32 idx) {
    if (idx >= 0 && idx < 5) return *(const char**)(lbl_80349314 + idx * 4);
    return lbl_80387D40;
}

// 0x8020E438
// Returns the string for index idx in the 186-entry table, or the fallback string when idx is out of range.
// The original copies the table to the stack first; the table is constant, so it is read directly here.
const char* fn_8020E438(s32 idx) {
    if (idx >= 0 && idx < 0xBA) return *(const char**)(lbl_80349328 + idx * 4);
    return lbl_80387D40;
}

// 0x8020E498
// Discipline index (0..2) to its one-letter string; anything else gives the fallback string.
const char* DisciplineLetter(s32 discipline) {
    if (discipline >= 0 && discipline < 3) return *(const char**)(lbl_80349610 + discipline * 4);
    return lbl_80387D40;
}

// 0x8020E4E8
// Maps one of the three StringHash values in lbl_80388D00 to 0, 1 or 2. Returns -1 when none matches.
s32 fn_8020E4E8(s32 hash) {
    if ((u32)hash == *(u32*)(lbl_80388D00 + 0)) return 0;
    if ((u32)hash == *(u32*)(lbl_80388D00 + 4)) return 1;
    if ((u32)hash == *(u32*)(lbl_80388D00 + 8)) return 2;
    return -1;
}

}
