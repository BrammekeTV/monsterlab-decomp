// Small functions from the 0x80007xxx-0x800BDxxx range: event sends, reaction playback on the
// fighter limbs, sound triggers and a few float helpers.
#include <math.h>

#include "types.h"

extern "C++" void* gEventMgr;           // 0x805FB6C8 (ai_states.cpp)

extern f32 lbl_805FDEB0;                // float constant
extern f32 lbl_805FDEB4;                // float constant
extern f32 lbl_805FE280;                // float constant (zero-like default)
extern f32 lbl_805FE33C;                // float constant (lower clamp)
extern f32 lbl_805FE340;                // float constant (upper clamp)
extern f32 lbl_805FE6FC;                // float constant (sign flip)
extern f32 lbl_805FE700;                // float constant (pi)
extern f32 lbl_805FE704;                // float constant (2 pi)

extern "C" {
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void  LimbPlayReaction(void* limb, u8* reaction, BOOL special);   // 0x8004F398
void  CharacterStartReaction(void* ch);                    // 0x8004E3D0
void  ReactionBuild(u8* out, s32 a, void* fighter, const char* anim);   // 0x80215648
void  ReactionBuildForAction(char* out, s32 limb, void** fighter, s32 action);   // 0x802157A0
const char* ReactionName(s32 id);                          // 0x8020E310
s32   TriggerDoAction(void* action, void* a1, void* a2);  // 0x801E0054
s32   PlaySoundAt(void* sound, int a, const void* pos, int b);   // 0x80209ABC
void  fn_800BF180(u8* a3);                                 // 0x800BF180
f32   fn_800C54A0(u8* p);                                  // 0x800C54A0
u8*   fn_800C2FF0(u8* p);                                  // 0x800C2FF0
u8*   fn_8018A41C(u8* p, s32 a);                           // 0x8018A41C
u8*   fn_801ABF0C(u8* table, u32 hash, s32 key);           // 0x801ABF0C
u8*   fn_801AC024(u8* table, u32 hash, s32 key, u8* prev); // 0x801AC024
u32   fn_801EC880(u8* p, u32 hash);                        // 0x801EC880
void  fn_801ECD1C(u8* p, s32 idx, s32 immediate);          // 0x801ECD1C
void  fn_8020A398(s32 handle);                             // 0x8020A398
void  KPADSetHoriParam(s32 chan, f32 a, f32 b);                 // 0x8027D28C
char* fn_80338BC0(char* s, const char* sub);               // 0x80338BC0
char* strcpy(char* d, const char* s);
extern u8* lbl_803C4000[16];            // 0x803C4000 (pointer table)
extern s32 lbl_803C4040[16];            // 0x803C4040 (flag table)
extern const char lbl_805F8578[];       // 0x805F8578 (string)
extern const char lbl_80358778[];       // 0x80358778 (string)
}

extern "C" {

// 0x800544D8
// Returns 1 if bit `bit` is set in the flags word at +0x1C of the sub-object at +0x4.
u32 fn_800544D8(u8* arg0, s32 arg1) {
    u8* sub = *(u8**)(arg0 + 0x4);
    return (*(s32*)(sub + 0x1C) & (1 << arg1)) != 0;
}

// 0x800BF878
// Sets the value at +0x264, clamped to 0..100; sends the low-value event when it drops below
// zero and the high-value event when it rises above the stored maximum at +0x268.
void fn_800BF878(u8* arg0, f32 farg0) {
    if (farg0 < lbl_805FE33C) {
        EventSend(gEventMgr, arg0, NULL, 0x80E3B472U, 0, 0, 1, 0);
        *(f32*)(arg0 + 0x268) = *(f32*)(arg0 + 0x264);
    }
    f32 t = farg0 + *(f32*)(arg0 + 0x264);
    f32 v = t;
    if (t < lbl_805FE33C) {
        v = lbl_805FE33C;
    } else if (t > lbl_805FE340) {
        v = lbl_805FE340;
    }
    *(f32*)(arg0 + 0x264) = v;
    if (v > *(f32*)(arg0 + 0x268)) {
        EventSend(gEventMgr, arg0, NULL, 0xAE21DFE0U, 0, 0, 1, 0);
    }
}

// 0x800DB844
// Signed shortest angle from farg0 to farg1 (wrapped to -pi..pi).
f32 fn_800DB844(f32 farg0, f32 farg1) {
    f32 d = farg1 - farg0;
    if ((f32)fabs(d) >= lbl_805FE700) {
        d = lbl_805FE704 - (f32)fabs(d);
        if (farg1 > farg0) {
            d *= lbl_805FE6FC;
        }
    }
    return d;
}

// 0x80007CD4
// Finds the entry in the table of 16 slots that equals arg0 with a zero flag, releases its
// handle through fn_8018A41C and clears the flag.
void fn_80007CD4(u8* arg0) {
    for (s32 i = 0; i < 16; i++) {
        if (arg0 == lbl_803C4000[i] && lbl_803C4040[i] == 0) {
            fn_8018A41C(lbl_803C4000[i], -1);
            lbl_803C4040[i] = 0;
            return;
        }
    }
}

// 0x80042364
// Plays the hit reaction on each of the five limbs of the fighter (arg2 holds the five limb
// pointers, the character pointer sits at +0x1C), then starts the character's reaction.
void fn_80042364(u8* arg0, u8* arg1, u8* arg2, s32 arg3) {
    u8 sp8[0x500];
    ReactionBuildForAction((char*)sp8, *(s32*)(arg1 + 0x8), (void**)arg2, *(s32*)(arg1 + 0x0));
    for (s32 i = 0; i < 5; i++) {
        u8* limb = ((u8**)arg2)[i];
        if (limb != NULL) {
            u8* obj = *(u8**)(limb + 0x20);
            if (obj != NULL) {
                obj -= 0x264;
            }
            LimbPlayReaction(obj, sp8 + i * 0x100, arg3);
        }
    }
    u8* ch = *(u8**)(arg2 + 0x1C);
    if (ch != NULL) {
        ch -= 0xC48;
    }
    CharacterStartReaction(ch);
}

// 0x80042408
// Like fn_80042364, but the reaction name comes from the id at *arg1 and action 0x30 uses the
// "idleintro" name. The name buffer lives inside the reaction buffer at +0x100 (element 1).
void fn_80042408(u8* arg0, s32* arg1, s32 arg2, u8* arg3, s32 arg4) {
    u8 sp8[0x500];
    ReactionBuild(sp8, *arg1, arg3, ReactionName(arg2));
    if (arg2 == 0x30) {
        strcpy(fn_80338BC0((char*)(sp8 + 0x100), lbl_805F8578), lbl_80358778);
    }
    for (s32 i = 0; i < 5; i++) {
        u8* limb = ((u8**)arg3)[i];
        if (limb != NULL) {
            u8* obj = *(u8**)(limb + 0x20);
            if (obj != NULL) {
                obj -= 0x264;
            }
            LimbPlayReaction(obj, sp8 + i * 0x100, arg4);
        }
    }
    u8* ch = *(u8**)(arg3 + 0x1C);
    if (ch != NULL) {
        ch -= 0xC48;
    }
    CharacterStartReaction(ch);
}

// 0x8005379C
// Fires the action triggers for slot arg1 (if arg1 != -1): the list at +0x9C and the inline
// list at +0x34, each when its 4-bit field at bits 1..4 is set.
void fn_8005379C(u8* arg0, s32 arg1, void* arg2) {
    if (arg1 != -1) {
        u8* list = *(u8**)(arg0 + 0x9C);
        if (list != NULL) {
            u8* entry = list + (arg1 * 4);
            if (((u32)*(s32*)(entry + 0x64) >> 0x1BU) & 0xF) {
                TriggerDoAction(entry + 0x64, arg0, arg2);
            }
        }
        u8* inl = arg0 + (arg1 * 4);
        if (((u32)*(s32*)(inl + 0x34) >> 0x1BU) & 0xF) {
            TriggerDoAction(inl + 0x34, arg0, arg2);
        }
    }
}

// 0x80054B38
// Clears the bits in arg1 from the word at arg0.
void fn_80054B38(u8* arg0, s32 arg1) {
    *(s32*)arg0 &= ~arg1;
}

// 0x8005A178
// Plays a sound for every entry in the list found under hash 0xB3FD94BD, walking the list
// with fn_801AC024.
void fn_8005A178(u8* arg0) {
    u8* node = fn_801ABF0C(*(u8**)(arg0 + 0x84), 0xB3FD94BDU, *(s32*)(arg0 + 0xA0));
    while (node != NULL) {
        PlaySoundAt(*(u8**)(node + 0x18), (int)(intptr_t)arg0, *(u8**)(arg0 + 0x4), 0);
        node = fn_801AC024(*(u8**)(arg0 + 0x84), 0xB3FD94BDU, *(s32*)(arg0 + 0xA0), node);
    }
}

// 0x8009C1E8
// Calls virtual slot +0xD8 of the object found from arg1, passing arg0, that object and the
// float returned by fn_800C54A0(arg1).
void fn_8009C1E8(u8* arg0, u8* arg1) {
    f32 v = fn_800C54A0(arg1);
    u8* obj = fn_800C2FF0(arg1);
    void (*slot)(u8*, u8*, f32) = (void (*)(u8*, u8*, f32)) * (void**)(*(u8**)obj + 0xD8);
    slot(arg0, obj, v);
}

// 0x800A6630
// Resets the global float state, stops the handle at +0x28C and clears the widget value at +0x94.
void fn_800A6630(u8* arg0) {
    KPADSetHoriParam(0, lbl_805FDEB0, lbl_805FDEB4);
    fn_8020A398(*(s32*)(arg0 + 0x28C));
    u8* p = arg0 + 0x94;
    fn_801ECD1C(p, (s32)fn_801EC880(p, 0xAEB44737U), 0);
}

// 0x800BD3D4
// If arg1 is set, stores it at +0x29C, runs fn_800BF180 on it, zeroes the float at +0x268 and
// sends the 0x367FA727 event.
void fn_800BD3D4(u8* arg0, u8* arg1) {
    if (arg1 != NULL) {
        *(u8**)(arg0 + 0x29C) = arg1;
        fn_800BF180(arg1);
        *(f32*)(arg0 + 0x268) = lbl_805FE280;
        EventSend(gEventMgr, arg0, NULL, 0x367FA727U, 0, 0, 1, 0);
    }
}

// 0x800BD644
// Walks the chain at +0x8C -> +0xC -> +0x24 (each node tagged 1 at +0x10 / +0x28) to a leaf
// whose +0xC is 0; when the byte at +0x298 is set its two floats are reset. Returns the leaf or NULL.
u8* fn_800BD644(u8* arg0) {
    u8* temp = *(u8**)(arg0 + 0x8C);
    if (temp == NULL) {
        return NULL;
    }
    if (*(s32*)(temp + 0x10) != 1) {
        return NULL;
    }
    u8* n = *(u8**)(*(u8**)(temp + 0xC));
    if (n == NULL) {
        return NULL;
    }
    if (*(s32*)(n + 0x28) != 1) {
        return NULL;
    }
    u8* leaf = *(u8**)(*(u8**)(n + 0x24));
    if (*(s32*)(leaf + 0xC) != 0) {
        return NULL;
    }
    if (*(u8*)(arg0 + 0x298) != 0) {
        *(f32*)(leaf + 0x24) = lbl_805FE280;
        *(f32*)(leaf + 0x18) = lbl_805FE280;
    }
    return leaf;
}

}  // extern "C"
