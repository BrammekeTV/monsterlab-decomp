// Game code from the m2c draft batch miss55 (verified against the asm).
// Functions: script registration, base-class constructors, tree lookup, small helpers.
#include "types.h"

struct ScopedLock { u8 pad[4]; };

typedef void (*VCallFn)(u8*, s32);

// C++ linkage, as in the files that define them.
void RobotEvolutionExperiment_RegisterDoorScripts(void);                                          // 0x80091DAC (small_8009c.cpp)
void fn_8015A2E8(u8* p, u32 unused4, u32 flush);                 // 0x8015A2E8 (small_8015.cpp)

extern "C" {
s32  RegisterScriptFunc(const char* name, s32 type, void* fn);  // 0x801DC91C
s32  PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);
s32  RaySceneTest(void* ray);
void ScopedLockCtor(ScopedLock* l);
void ScopedLockDtor(ScopedLock* l, s32 flags);
void fn_800690D4(u8* p, s32 a, void* params, s32 b);            // 0x800690D4 (base ctor)
void* fn_801D40E0(s32 n);                                        // 0x801D40E0
void* fn_80209814(void* v);                                      // 0x80209814
u32  fn_801EC880(u8* p, u32 hash);                               // 0x801EC880
void fn_801EC8C4(u8* p, u32 a, u32 b);                           // 0x801EC8C4
void fn_801ECD1C(u8* p, s32 idx, s32 immediate);                 // 0x801ECD1C
u8*  fn_8015A210(u8* p, s32 a, s32* out, s32 b);                 // 0x8015A210
void fn_800A68F8(u8* a, u8* b, u8* c, f32 d);                    // 0x800A68F8
void fn_800A71F0(u8* a, u8* b);                                  // 0x800A71F0
void RobotEvolutionExperiment_RegisterRobotScripts(void);                                        // 0x800921B0
s32  RobotEvolutionExperiment_RegisterSmasherScripts(void);                                          // 0x80093934
f32  fn_80147800(s32 a);                                         // 0x80147800

s32 PuzzleTimeout(void);
s32  PuzzleSelectPiece(void);
void PuzzleRepositionCursor(void);
void PuzzleMovePiece(void);
s32  PuzzlePiecePlaced(void);
void ShowCompletePuzzle(void);
void PuzzleSetCursor(s32 unused, u32 hash);
s32  CheckBugSound(void);
void ResetCounter(void);
void RobotEvolutionExperiment_IntroSmasherHit(u8* arg0);
void RobotEvolutionExperiment_IntroSmasherDone(void);
void RobotEvolutionExperiment_IntroRobotShowCountdown(u8* p);
void RobotEvolutionExperiment_IntroRobotDone(void);
}

extern u8  lbl_80355D58[];       // puzzle script name table
extern u8  lbl_80357504[];       // script name "CheckBugSound"
extern u8  lbl_80357514[];       // script name "ResetCounter"
extern u8  lbl_8035BBD8[];       // robot evolution script name table
extern u8  lbl_8035D950[];       // object vtable
extern u8  lbl_8035DA50[];       // object vtable
extern u8  lbl_8035DB48[];       // object vtable
extern u8  lbl_8035DC48[];       // object vtable
extern u8  lbl_803C4298[];       // level info block
extern f32 lbl_80346AB8[];       // f32 table indexed by RaySceneTest
extern f32 lbl_80346CB0[];       // f32 table indexed by RaySceneTest
extern f32 lbl_805FB168;         // mutable float global
extern void* lbl_805FBBAC;       // voice SFX handle

extern "C" {

// 0x801DA788
u8* fn_801DA788(u8* arg0, s32 arg1) {
    return *(u8**)(*(u8**)(arg0 + 0x58) + arg1 * 4);
}

// 0x80025CE0
s32 fn_80025CE0(void) {
    RegisterScriptFunc((const char*)(lbl_80355D58 + 0x1588), 0x1D, (void*)PuzzleTimeout);
    RegisterScriptFunc((const char*)(lbl_80355D58 + 0x1598), 0x1D, (void*)PuzzleSelectPiece);
    RegisterScriptFunc((const char*)(lbl_80355D58 + 0x15AC), 0x4E, (void*)PuzzleRepositionCursor);
    RegisterScriptFunc((const char*)(lbl_80355D58 + 0x15C8), 0x4E, (void*)PuzzleMovePiece);
    RegisterScriptFunc((const char*)(lbl_80355D58 + 0x15D8), 0x1D, (void*)PuzzlePiecePlaced);
    RegisterScriptFunc((const char*)(lbl_80355D58 + 0x15EC), 0x4E, (void*)ShowCompletePuzzle);
    return RegisterScriptFunc((const char*)(lbl_80355D58 + 0x1600), 0x51, (void*)PuzzleSetCursor);
}

// 0x8002917C
s32 fn_8002917C(void) {
    RegisterScriptFunc((const char*)lbl_80357504, 0x1D, (void*)CheckBugSound);
    return RegisterScriptFunc((const char*)lbl_80357514, 0x4E, (void*)ResetCounter);
}

// 0x8007B9E4
// Writes the three-float aim vector from the two aim axes (player slot 0x38 of the level
// info block), each clamped to +-0.7 and scaled. arg1/arg2 are not read by the body (kept to
// match the declaration in draft_miss54.cpp).
void fn_8007B9E4(f32* arg0, u8* arg1, u8* arg2) {
    f32 x = fn_80147800((*(s32*)(lbl_803C4298 + 0x38) * 0x10) + 0x1093);
    f32 y = fn_80147800((*(s32*)(lbl_803C4298 + 0x38) * 0x10) + 0x1094);
    if (x < -0.7f) {
        x = -0.7f;
    }
    if (x > 0.7f) {
        x = 0.7f;
    }
    if (y < -0.7f) {
        y = -0.7f;
    }
    if (y > 0.7f) {
        y = 0.7f;
    }
    arg0[0] = x * -(3.2f / 0.7f);
    arg0[1] = 0.0f;
    arg0[2] = y * -(2.4f / 0.7f);
}

// 0x8007C50C
// Clears bits 0xDD of every byte of the cell table at +0x24 (table base from fn_8015A210),
// then flags the table dirty through fn_8015A2E8.
void fn_8007C50C(u8* arg0) {
    s32 stride;
    u8* base = fn_8015A210(*(u8**)(arg0 + 0x24) + 4, 0, &stride, 0);
    for (s32 row = 0; row < (s32)*(u16*)(*(u8**)(arg0 + 0x24) + 6); ++row) {
        u8* p = base + row * stride;
        for (s32 col = 0; col < (s32)(*(u16*)(*(u8**)(arg0 + 0x24) + 4) >> 1); ++col) {
            *p &= 0xDD;
            ++p;
        }
    }
    fn_8015A2E8(*(u8**)(arg0 + 0x24) + 4, 0, 1);
}

// 0x8008D194
s32 fn_8008D194(u8* arg0, u8* arg1, f32 farg0, f32 farg1) {
    ScopedLock lock;

    if (arg1 == NULL) {
        return 0;
    }
    *(s32*)(arg0 + 0x264) = 5;
    ScopedLockCtor(&lock);
    (*(VCallFn*)(*(u8**)arg0 + 0x14))(arg0, 1);
    ScopedLockDtor(&lock, -1);
    *(u8**)(arg0 + 0x280) = arg1;
    *(f32*)(arg0 + 0x298) = farg1;
    *(f32*)(arg0 + 0x290) = farg0;
    return 1;
}

// 0x8008EFE0
s32 RobotEvolutionExperiment_RegisterScripts(void) {
    RegisterScriptFunc((const char*)(lbl_8035BBD8 + 0x00), 0x4E, (void*)RobotEvolutionExperiment_IntroSmasherHit);
    RegisterScriptFunc((const char*)(lbl_8035BBD8 + 0x2C), 0x4E, (void*)RobotEvolutionExperiment_IntroSmasherDone);
    RegisterScriptFunc((const char*)(lbl_8035BBD8 + 0x58), 0x4E, (void*)RobotEvolutionExperiment_IntroRobotShowCountdown);
    RegisterScriptFunc((const char*)(lbl_8035BBD8 + 0x90), 0x4E, (void*)RobotEvolutionExperiment_IntroRobotDone);
    RobotEvolutionExperiment_RegisterDoorScripts();
    RobotEvolutionExperiment_RegisterRobotScripts();
    return RobotEvolutionExperiment_RegisterSmasherScripts();
}

// 0x800A50B4
// Looks up the key (*keyPtr) in the ordered tree rooted at head+4 (node: left +0, right +4,
// key s32 +0xC). Writes the lower bound (first node >= key) to out[0] and the upper bound
// (first node > key) to out[1]; both are the header slot head+4 when there is none.
void fn_800A50B4(u8** out, u8* head, s32* keyPtr) {
    s32 key = *keyPtr;
    u8* node = *(u8**)(head + 0x4);
    u8* after = head + 0x4;

    while (node != NULL) {
        s32 nodeKey = *(s32*)(node + 0xC);
        if (key < nodeKey) {
            after = node;
            node = *(u8**)(node + 0x0);
        } else if (nodeKey < key) {
            node = *(u8**)(node + 0x4);
        } else {
            break;
        }
    }
    if (node == NULL) {
        out[0] = after;
        out[1] = after;
        return;
    }

    u8* first = node;
    u8* lo = *(u8**)(node + 0x0);
    while (lo != NULL) {
        if (*(s32*)(lo + 0xC) >= key) {
            first = lo;
            lo = *(u8**)(lo + 0x0);
        } else {
            lo = *(u8**)(lo + 0x4);
        }
    }

    u8* hi = *(u8**)(node + 0x4);
    while (hi != NULL) {
        if (key < *(s32*)(hi + 0xC)) {
            after = hi;
            hi = *(u8**)(hi + 0x0);
        } else {
            hi = *(u8**)(hi + 0x4);
        }
    }
    out[0] = first;
    out[1] = after;
}

// 0x800A6694
// Creates the voice SFX handle lbl_805FBBAC once, then plays the cue and stores its
// handle at +0x28C.
s32 fn_800A6694(u8* arg0) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) {
            v = fn_80209814(v);
        }
        lbl_805FBBAC = v;
    }
    s32 handle = PlaySound(0xF03E563E, 0, 0, 0, 0, 0);
    *(s32*)(arg0 + 0x28C) = handle;
    return handle;
}

// 0x800A7020
void fn_800A7020(u8* arg0, u8* arg1, u8* arg2, f32 farg0) {
    fn_800A68F8(arg0, arg1, arg2, farg0);
    fn_800A71F0(arg0, arg1);
}

// 0x800A7828
// When the requested state at +0x288 differs from the current one at +0x284, sends the
// matching cue for the new state to the sub-object at +0x94, then adopts the new state.
void fn_800A7828(u8* arg0) {
    if (*(s32*)(arg0 + 0x288) == *(s32*)(arg0 + 0x284)) {
        return;
    }
    u8* sub = arg0 + 0x94;
    u32 hash = fn_801EC880(sub, 0xAEB44737u);
    fn_801ECD1C(sub, (s32)hash, 0);
    switch (*(s32*)(arg0 + 0x288)) {
    case 0:
        fn_801EC8C4(sub, hash, 0x49D0C130u);
        break;
    case 1:
        fn_801EC8C4(sub, hash, 0x074B1D43u);
        break;
    case 2:
        fn_801EC8C4(sub, hash, 0xA42B7C2Cu);
        break;
    default:
        break;
    }
    *(s32*)(arg0 + 0x284) = *(s32*)(arg0 + 0x288);
}

// 0x800A791C
// Returns the value at +0x264 divided by the table entry selected by RaySceneTest.
f32 fn_800A791C(u8* arg0) {
    s32 idx = RaySceneTest(arg0);
    f32 value = *(f32*)(arg0 + 0x264);
    return value / lbl_80346AB8[idx];
}

// 0x800AA5A0
// Moves the value at +0x724 by the table entry selected by RaySceneTest (sign from arg1),
// clamped to +-50. Also sets the field at +0x15C to 2.
void fn_800AA5A0(u8* arg0, s32 arg1) {
    const f32 limit = 50.0f;
    *(f32*)(arg0 + 0x15C) = 2.0f;
    f32 sign = (arg1 != 0) ? -1.0f : 1.0f;
    s32 idx = RaySceneTest(arg0);
    f32 value = *(f32*)(arg0 + 0x724) + lbl_80346CB0[idx] * sign;
    if (value < -limit) {
        value = -limit;
    } else if (value > limit) {
        value = limit;
    }
    *(f32*)(arg0 + 0x724) = value;
}

// 0x800AAEE8
// Constructor: base ctor, vtable lbl_8035D950, zeroes the float fields 0x264..0x284 and the
// integer fields 0x290/0x294. Returns the object.
void* fn_800AAEE8(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(f32*)(p + 0x264) = 0.0f;
    *(void**)(p + 0x0) = lbl_8035D950;
    *(f32*)(p + 0x268) = 0.0f;
    *(f32*)(p + 0x26C) = 0.0f;
    *(f32*)(p + 0x270) = 0.0f;
    *(f32*)(p + 0x274) = 0.0f;
    *(f32*)(p + 0x278) = 0.0f;
    *(f32*)(p + 0x27C) = 0.0f;
    *(f32*)(p + 0x280) = 0.0f;
    *(f32*)(p + 0x284) = 0.0f;
    *(s32*)(p + 0x290) = 0;
    *(s32*)(p + 0x294) = 0;
    return p;
}

// 0x800ABE10
// Constructor: base ctor, vtable lbl_8035DA50, sets the state words at 0x264/0x268 to -1,
// the float fields 0x26C..0x29C to 0 (0x284 to 1), and 0x2A0 to 0. Returns the object.
void* fn_800ABE10(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(void**)(p + 0x0) = lbl_8035DA50;
    *(s32*)(p + 0x264) = -1;
    *(s32*)(p + 0x268) = -1;
    *(f32*)(p + 0x26C) = 0.0f;
    *(f32*)(p + 0x270) = 0.0f;
    *(f32*)(p + 0x274) = 0.0f;
    *(f32*)(p + 0x278) = 0.0f;
    *(f32*)(p + 0x27C) = 0.0f;
    *(f32*)(p + 0x280) = 0.0f;
    *(f32*)(p + 0x284) = 1.0f;
    *(f32*)(p + 0x288) = 0.0f;
    *(f32*)(p + 0x28C) = 0.0f;
    *(f32*)(p + 0x290) = 0.0f;
    *(f32*)(p + 0x294) = 0.0f;
    *(f32*)(p + 0x298) = 0.0f;
    *(f32*)(p + 0x29C) = 0.0f;
    *(s32*)(p + 0x2A0) = 0;
    return p;
}

// 0x800ACD24
// Constructor: base ctor, vtable lbl_8035DB48, sets 0x264, 0x268, 0x26C to -1 and 0x270..0x278
// to 0. Returns the object.
void* fn_800ACD24(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(s32*)(p + 0x264) = -1;
    *(void**)(p + 0x0) = lbl_8035DB48;
    *(s32*)(p + 0x268) = -1;
    *(s32*)(p + 0x26C) = -1;
    *(s32*)(p + 0x270) = 0;
    *(s32*)(p + 0x274) = 0;
    *(s32*)(p + 0x278) = 0;
    return p;
}

}  // extern "C"
