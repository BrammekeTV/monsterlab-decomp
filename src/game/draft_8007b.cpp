// Experiment sound table and small helpers in 0x8007xxxx.
#include "types.h"

extern "C" {
#include "game/script_callbacks.h"

s32     PlaySoundAt(void* sound, int a, const void* pos, int b);  // 0x80209ABC
struct Locator;
Locator* WorldFindLocator(void* world, u32 hash);                 // 0x8012AA50
void*   fn_801D40E0(s32 n);                                       // 0x801D40E0
void*   fn_80209814(void* v);                                     // 0x80209814
void    fn_8020A398(s32 handle);                                  // 0x8020A398
u32     fn_8020A9A8(s32 handle);                                  // 0x8020A9A8 (signature guessed)
s32     fn_8020B0AC(f32 v);                                       // 0x8020B0AC (signature guessed)

extern u8 gWorldState[];
extern u8 lbl_803C4298[];     // 0x803C4298 (+4 and +8: objects with vtables)
extern u8 lbl_803C5740[];     // 0x803C5740 (experiment sound table, 0x10 bytes per entry)
extern s32 lbl_805FB12C;      // 0x805FB12C (entry count)
extern void* lbl_805FBBAC;    // 0x805FBBAC (voice SFX handle)
extern f32 lbl_805FD5D8;      // 0x805FD5D8

typedef void (*Fn)(void* self, ...);

// Entry layout: +0 sound, +4 owner, +8 id, +0xC handle (-1 = none).

// Creates the voice SFX handle once (fn_801D40E0, then fn_80209814 if non-null).
static inline void EnsureVoiceHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
}

// 0x80070270
// Maps a single-bit mask (0x1..0x8000) to a name hash; 0 for anything else.
u32 fn_80070270(s32 mask) {
    switch (mask) {
    case 0x1:    return 0x634B9C91;
    case 0x2:    return 0x4A0420DE;
    case 0x4:    return 0xEB9E9819;
    case 0x8:    return 0xC4DF2204;
    case 0x10:   return 0xAF5709C6;
    case 0x20:   return 0x8016B3DB;
    case 0x40:   return 0xEB80FD6F;
    case 0x80:   return 0xFA1F900C;
    case 0x100:  return 0xD55E2A11;
    case 0x200:  return 0x2066CC8C;
    case 0x400:  return 0x0F277691;
    case 0x800:  return 0xB7CAF015;
    case 0x1000: return 0x39FFF8CF;
    case 0x2000: return 0x16BE42D2;
    case 0x4000: return 0x4AD84AA2;
    case 0x8000: return 0xF89709F6;
    default:     return 0;
    }
}

// 0x80070884
// Camera setup between two locators (hashes 0x834C7963 / 0xAAE1A370) via the camera object's vtable.
void fn_80070884(void) {
    u8* a = (u8*)WorldFindLocator(gWorldState, 0x834C7963);
    u8* b = (u8*)WorldFindLocator(gWorldState, 0xAAE1A370);
    if (a != NULL && b != NULL) {
        u8* cam = *(u8**)(lbl_803C4298 + 4);
        (*(Fn*)(*(u8**)cam + 0x50))(cam, 0);
        cam = *(u8**)(lbl_803C4298 + 4);
        (*(Fn*)(*(u8**)cam + 0xC))(cam, 6);
        cam = *(u8**)(lbl_803C4298 + 4);
        (*(Fn*)(*(u8**)cam + 0x2C))(cam, a + 8, b + 8);
        cam = *(u8**)(lbl_803C4298 + 4);
        (*(Fn*)(*(u8**)cam + 0x1C))(cam, a + 8);
        u8* o = *(u8**)(lbl_803C4298 + 8);
        typedef void (*FnF)(void* self, f32 v);
        (*(FnF*)(*(u8**)o + 0x98))(o, lbl_805FD5D8);
    }
}

// 0x80070978
// Adds (owner, sound, id) to the experiment sound table if not present yet (max 99 entries).
void RegisterExperimentSound(void* owner, u32 sound, u32 id) {
    s32 n = lbl_805FB12C;
    u8* e = lbl_803C5740;
    for (s32 i = 0; i < n; i++, e += 0x10) {
        if (*(void**)(e + 4) == owner && *(u32*)(e + 8) == id && *(u32*)(e + 0) == sound) return;
    }
    if (n + 1 >= 0x64) return;
    e = lbl_803C5740 + n * 0x10;
    lbl_805FB12C = lbl_805FB12C + 1;
    *(void**)(e + 4) = owner;
    *(u32*)(e + 8) = id;
    *(u32*)(e + 0) = sound;
    *(s32*)(e + 0xC) = -1;
}

// 0x800709FC
// For each matching entry: stop its handle, set the sound's byte at +0x2E, restart it. With an
// owner the position is read from owner+4.
void PlayExperimentSoundAttached(void* owner, u32 id, s32 vol) {
    u8* e = lbl_803C5740;
    for (s32 i = 0; i < lbl_805FB12C; i++, e += 0x10) {
        if (*(void**)(e + 4) != owner || *(u32*)(e + 8) != id) continue;
        s32 h = *(s32*)(e + 0xC);
        if (h != -1) fn_8020A398(h);
        u8* snd = *(u8**)(e + 0);
        if (snd != NULL) *(u8*)(snd + 0x2E) = (u8)vol;
        EnsureVoiceHandle();
        if (owner != NULL) {
            *(s32*)(e + 0xC) = PlaySoundAt(*(void**)(e + 0), 0, *(void**)(*(u8**)(e + 4) + 4), 0);
        } else {
            *(s32*)(e + 0xC) = PlaySoundAt(*(void**)(e + 0), 0, NULL, 0);
        }
    }
}

// 0x80070B18
// Like PlayExperimentSoundAttached, but a non-null owner value is passed as the second argument
// of PlaySoundAt (prop location) with no position.
void PlayExperimentSoundAtPropLocation(void* owner, u32 id, s32 vol) {
    u8* e = lbl_803C5740;
    for (s32 i = 0; i < lbl_805FB12C; i++, e += 0x10) {
        if (*(void**)(e + 4) != owner || *(u32*)(e + 8) != id) continue;
        s32 h = *(s32*)(e + 0xC);
        if (h != -1) fn_8020A398(h);
        u8* snd = *(u8**)(e + 0);
        if (snd != NULL) *(u8*)(snd + 0x2E) = (u8)vol;
        EnsureVoiceHandle();
        if (owner != NULL) {
            *(s32*)(e + 0xC) = PlaySoundAt(*(void**)(e + 0), (int)(intptr_t)owner, NULL, 0);
        } else {
            *(s32*)(e + 0xC) = PlaySoundAt(*(void**)(e + 0), 0, NULL, 0);
        }
    }
}

// 0x80070C30
// Restarts every matching entry's sound at volume byte vol, no position.
void PlayExperimentSound(void* owner, u32 id, s32 vol) {
    u8* e = lbl_803C5740;
    for (s32 i = 0; i < lbl_805FB12C; i++, e += 0x10) {
        if (*(void**)(e + 4) != owner || *(u32*)(e + 8) != id) continue;
        s32 h = *(s32*)(e + 0xC);
        if (h != -1) fn_8020A398(h);
        u8* snd = *(u8**)(e + 0);
        if (snd != NULL) *(u8*)(snd + 0x2E) = (u8)vol;
        EnsureVoiceHandle();
        *(s32*)(e + 0xC) = PlaySoundAt(*(void**)(e + 0), 0, NULL, 0);
    }
}

// 0x80070D00
// Stops and clears the handle of every matching entry.
void StopExperimentSound(void* owner, u32 id) {
    u8* e = lbl_803C5740;
    for (s32 i = 0; i < lbl_805FB12C; i++, e += 0x10) {
        if (*(void**)(e + 4) == owner && *(u32*)(e + 8) == id) {
            s32 h = *(s32*)(e + 0xC);
            if (h != -1) {
                fn_8020A398(h);
                *(s32*)(e + 0xC) = -1;
            }
        }
    }
}

// 0x80070D88
// First matching entry: if it has a live handle, query it via fn_8020A9A8; otherwise 0.
u32 fn_80070D88(void* owner, u32 id) {
    u8* e = lbl_803C5740;
    for (s32 n = lbl_805FB12C; n > 0; n--, e += 0x10) {
        if (*(void**)(e + 4) == owner && *(u32*)(e + 8) == id) {
            if (*(s32*)(e + 0xC) == -1) return 0;
            EnsureVoiceHandle();
            return fn_8020A9A8(*(s32*)(e + 0xC));
        }
    }
    return 0;
}

// 0x80070E30
// Calls fn_8020B0AC(v) once per matching entry that has a live handle.
void fn_80070E30(void* owner, u32 id, f32 v) {
    u8* e = lbl_803C5740;
    for (s32 i = 0; i < lbl_805FB12C; i++, e += 0x10) {
        if (*(void**)(e + 4) == owner && *(u32*)(e + 8) == id && *(s32*)(e + 0xC) != -1) {
            fn_8020B0AC(v);
        }
    }
}

}  // extern "C"
