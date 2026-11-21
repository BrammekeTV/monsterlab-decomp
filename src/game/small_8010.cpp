// Small functions in 0x8010xxxx (entity/level script callbacks, debug prints).
#include "types.h"
#include "revolution.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern u8 gGame[];                 // 0x803B53A0
extern u8 gWorldState[];           // 0x803E6D00
extern u8 lbl_803C2A20[];          // global block; tuning table pointer at +0x860 (used by fn_801070B0)
extern u8 lbl_8047C760[];          // effect manager
extern f32 lbl_805FEBB8;           // float constant, fourth element of the effect position
extern const char lbl_805F8E90[];  // table name, printed as %s by the PrintBit/PrintInt/PrintFloat helpers
extern const char lbl_803698C8[];  // format string: "f32 %s[%d] = %5.2f"
extern const char lbl_803698DC[];  // format string: int print
extern const char lbl_803698F0[];  // format string: "bit %s[%d] = %d"

extern "C" {
void     GameLog(int level, int category, const char* fmt, ...);  // 0x8000B2F0
void     operator_delete(void* p);                                 // 0x801D426C
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner); // 0x801C80E0
void*    CreateEntityByType(void* mem, u32 typeHash, void* params); // entity_factory.cpp
void     fn_8005F204(u8* obj);                                     // 0x8005F204
void     fn_80061A64(void* a0, void* a1, void* a2);                // 0x80061A64 (signature not confirmed)
s32      fn_801DFF00(u8* base, s32 idx);                           // 0x801DFF00 (get bit)
s32      fn_801DFF54(u8* base, s32 idx);                           // 0x801DFF54 (get int)
f32      fn_801DFF9C(u8* base, s32 idx);                           // 0x801DFF9C (get float)
void     fn_801DFE94(u8* base, u8* table);                         // 0x801DFE94
void     fn_801DC574(s32 fn);                                      // 0x801DC574 (callback slot setter)
void     fn_801DC57C(s32 fn);                                      // 0x801DC57C (callback slot setter)
void     fn_801DC584(s32 fn);                                      // 0x801DC584 (callback slot setter)
void     fn_801DC58C(s32 fn);                                      // 0x801DC58C (callback slot setter)
void     fn_801DC594(s32 fn);                                      // 0x801DC594 (callback slot setter)
s32      fn_80106900(s32 a0);                                      // 0x80106900
u32      fn_80106988(s32 a0);                                      // 0x80106988
s32      fn_80106920(s32 code);                                    // 0x80106920
s32      fn_801069B0(s32 code);                                    // 0x801069B0
s32      fn_80106A2C(s32 code);                                    // 0x80106A2C
}

extern "C" {

// 0x80103624
// Asks the object whether it is dead (vtable slot 0x4C); if so, calls fn_8005F204 on it.
void ExplodeIfDead(u32 a0) {
    u8* obj = (u8*)(uintptr_t)a0;
    s32 dead = (*(s32 (**)(u8*))(*(u8**)obj + 0x4C))(obj);
    if (dead) fn_8005F204(obj);
}

// 0x80103710
// Copies the word at a1 and the word at a2, then passes the object a0 and both copies to fn_80061A64.
void JumpToPlatform(u32 a0, u32 a1, u32 a2) {
    u32 fromWord = *(u32*)(uintptr_t)a1;
    u32 toWord = *(u32*)(uintptr_t)a2;
    fn_80061A64((void*)(uintptr_t)a0, &fromWord, &toWord);
}

// 0x80103840
// Spawns the effect a1 at the object's position (+0x18/+0x1C/+0x20). Does nothing for a null object.
void PlayEffectTransient(void* a0, u32 a1) {
    if (a0 == 0) return;
    u8* obj = (u8*)a0;
    f32 pos[4];
    pos[0] = *(f32*)(obj + 0x18);
    pos[1] = *(f32*)(obj + 0x1C);
    pos[2] = *(f32*)(obj + 0x20);
    pos[3] = lbl_805FEBB8;
    SpawnEffectAt(lbl_8047C760, (u32)(uintptr_t)a1, pos, 0);  // lint:ok
}

// 0x801068A8
// Registers five callbacks (fn_80106900, fn_80106920, fn_80106988, fn_801069B0, fn_80106A2C)
// with the 0x801DC57x/58x setters.
void fn_801068A8(void) {
    fn_801DC574((s32)(uintptr_t)fn_80106900);
    fn_801DC58C((s32)(uintptr_t)fn_80106920);
    fn_801DC57C((s32)(uintptr_t)fn_80106988);
    fn_801DC584((s32)(uintptr_t)fn_801069B0);
    fn_801DC594((s32)(uintptr_t)fn_80106A2C);
}

// 0x80106A2C
// Returns 1 for codes 0x2B, 0x33, 0x35-0x37, 0x3F-0x54 and 0x5B, otherwise 0.
s32 fn_80106A2C(s32 code) {
    if (code == 0x2B || code == 0x33) return 1;
    if (code >= 0x35 && code <= 0x37) return 1;
    if (code >= 0x3F && code <= 0x54) return 1;
    if (code == 0x5B) return 1;
    return 0;
}

// 0x80106B54
// Debug log of one global bit: "bit NAME[idx] = value".
void PrintBit(u32 a0) {
    s32 idx = (s32)a0;
    s32 value = fn_801DFF00(gGame + 0x5C8, idx);
    GameLog(1, 3, lbl_803698F0, lbl_805F8E90, idx, value);
}

// 0x80106BB4
// Debug log of one global int: idx and value.
void PrintInt(u32 a0) {
    s32 idx = (s32)a0;
    s32 value = fn_801DFF54(gGame + 0x5C8, idx);
    GameLog(1, 3, lbl_803698DC, lbl_805F8E90, idx, value);
}

// 0x80106C14
// Debug log of one global float: idx, then the value (returned in f1 and passed to the log).
void PrintFloat(u32 a0) {
    s32 idx = (s32)a0;
    f32 value = fn_801DFF9C(gGame + 0x5C8, idx);
    GameLog(1, 3, lbl_803698C8, lbl_805F8E90, idx, value);
}

// 0x80106D1C
// Same as PrintBit, for the level table (gGame + 0x6C8).
void Level_PrintBit(u32 a0) {
    s32 idx = (s32)a0;
    s32 value = fn_801DFF00(gGame + 0x6C8, idx);
    GameLog(1, 3, lbl_803698F0, lbl_805F8E90, idx, value);
}

// 0x80106D7C
// Same as PrintInt, for the level table (gGame + 0x6C8).
void Level_PrintInt(u32 a0) {
    s32 idx = (s32)a0;
    s32 value = fn_801DFF54(gGame + 0x6C8, idx);
    GameLog(1, 3, lbl_803698DC, lbl_805F8E90, idx, value);
}

// 0x80106DDC
// Same as PrintFloat, for the level table (gGame + 0x6C8).
void Level_PrintFloat(u32 a0) {
    s32 idx = (s32)a0;
    f32 value = fn_801DFF9C(gGame + 0x6C8, idx);
    GameLog(1, 3, lbl_803698C8, lbl_805F8E90, idx, value);
}

// 0x801070B0
// Code 0x1FF: fn_801DFE94 on the global table (gGame + 0x5C8) with the block at lbl_803C2A20 + 0x860.
// Code 0: fn_801DFE94 on the level table (gGame + 0x6C8) with gWorldState's pointer + 0xA4.
// Any other code does nothing. (Return value of fn_801DFE94 is not used.)
void fn_801070B0(s32 a0) {
    if (a0 == 0x1FF) {
        fn_801DFE94(gGame + 0x5C8, lbl_803C2A20 + 0x860);
        return;
    }
    if (a0 != 0) return;
    u8* ws = *(u8**)gWorldState;
    fn_801DFE94(gGame + 0x6C8, ws + 0xA4);
}

// 0x801088C0
// Creates an entity from the given arguments; returns the result of CreateEntityByType.
void* fn_801088C0(void* a0, u32 a1, void* a2) {
    return CreateEntityByType(a0, a1, a2);
}

// 0x80108FB4
// Deleting-destructor tail: frees p when del > 0, then returns p.
void* fn_80108FB4(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x80108FF4
// Deleting-destructor tail: frees p when del > 0, then returns p.
void* fn_80108FF4(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x80109034
// Deleting-destructor tail: frees p when del > 0, then returns p.
void* fn_80109034(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x80109074
// Deleting-destructor tail: frees p when del > 0, then returns p.
void* fn_80109074(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x801090B4
// Deleting-destructor tail: frees p when del > 0, then returns p.
void* fn_801090B4(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x801090F4
// Deleting-destructor tail: frees p when del > 0, then returns p.
void* fn_801090F4(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x80109134
// Deleting-destructor tail: frees p when del > 0, then returns p.
void* fn_80109134(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}
}

// ---- Batch: PlayEffect, FSM debug prints, net/hello callbacks, GX helpers, misc deleting dtors ----

extern u8 lbl_803C7270[];          // default FSM table when the context has none
extern u8 lbl_803C75C8[];          // object passed to fn_8032F1E8 by fn_80109BA4
extern u8 lbl_803E69C8[];          // three-float table written by fn_80109BA4
extern u8 lbl_803C4298[];          // level info block (flags at +0x3C, byte at +0x1F)
extern u8 gIdPool[];               // 0x8048A8F0
extern const char lbl_805F8EDC[];  // format string for the id-name buffer (SysSprintf, no args)
extern void* lbl_805FB964;         // movie player object
extern u8 lbl_805FB50C;            // GX shadow flags byte
extern u8 lbl_805F9212;            // GX copy filter value
extern f32 lbl_805FEBF8;           // float constant (first table entry)
extern f32 lbl_805FEBFC;           // float constant (second table entry)
extern f32 lbl_805FEC50;           // float constant (reset value for the +8 field)

void     GameSetOption3(u8* game, s32 value);  // session.cpp (C++ linkage there)

extern "C" {
int      SysSprintf(char* buf, const char* fmt, ...);      // engine.cpp
void*    IdPoolNext(void* pool);                           // 0x801E6EB0
void     BufferFree(void* p);                              // 0x801D4980
u8*      fn_801F5408(void);                               // 0x801F5408
void     fn_80132094(void);                                // 0x80132094 (bare blr in this build)
void     fn_8012AAA4(u8* world);                           // 0x8012AAA4
void     fn_8032F1E8(u8* obj, void (*cb)(void), s32 a, s32 b, s32 c); // 0x8032F1E8
void*    fn_80109BE0(void* obj);                           // 0x80109BE0
}

extern "C" {

// 0x801037DC
// Spawns effect a1 at the object's position (+0x18/+0x1C/+0x20) when the object is non-null and its
// +0xA8 field is zero. The address of +0xA8 is passed to SpawnEffectAt as its extra argument.
void PlayEffect(void* a0, u32 a1) {
    if (a0 == 0) return;
    u8* obj = (u8*)a0;
    if (*(s32*)(obj + 0xA8) != 0) return;
    f32 pos[4];
    pos[0] = *(f32*)(obj + 0x18);
    pos[1] = *(f32*)(obj + 0x1C);
    pos[2] = *(f32*)(obj + 0x20);
    pos[3] = lbl_805FEBB8;
    SpawnEffectAt(lbl_8047C760, (u32)(uintptr_t)a1, pos, (u8**)(obj + 0xA8));  // lint:ok
}

// 0x80106920
// Code test: returns 1 for codes 0x0F, 0x2E, 0x2F, 0x31, 0x32, 0x34, 0x3E, 0x55-0x5A; otherwise 0.
s32 fn_80106920(s32 code) {
    if (code == 0xF) return 1;
    if (code < 0x2E) return 0;
    if (code < 0x33) return code != 0x30;
    if (code == 0x33) return 0;
    if (code < 0x35) return 1;
    if (code < 0x3E) return 0;
    if (code == 0x3E) return 1;
    if (code < 0x55) return 0;
    if (code < 0x5B) return 1;
    return 0;
}

// 0x801069B0
// Same code ranges as fn_80106920 (without 0x0F): returns the constant 0xC87B6F93 for those codes, else 0.
s32 fn_801069B0(s32 code) {
    if (code < 0x2E) return 0;
    if (code < 0x33) return code != 0x30 ? (s32)0xC87B6F93u : 0;
    if (code == 0x33) return 0;
    if (code < 0x35) return (s32)0xC87B6F93u;
    if (code < 0x3E) return 0;
    if (code == 0x3E) return (s32)0xC87B6F93u;
    if (code < 0x55) return 0;
    if (code < 0x5B) return (s32)0xC87B6F93u;
    return 0;
}

// Table base for the FSM debug prints: the context's +0x58 table, or lbl_803C7270 when absent.
static u8* FsmPrintBase(ScriptCtx* ctx) {
    if (ctx != 0) {
        u8* t = *(u8**)((u8*)ctx + 0x58);
        if (t != 0) return t;
    }
    return lbl_803C7270;
}

// 0x80106F58
// Debug log of one FSM bit: "bit NAME[idx] = value".
void FSM_PrintBit(ScriptCtx* a0, u32 a1) {
    s32 idx = (s32)a1;
    s32 value = fn_801DFF00(FsmPrintBase(a0), idx);
    GameLog(1, 3, lbl_803698F0, lbl_805F8E90, idx, value);
}

// 0x80106FCC
// Debug log of one FSM int: "NAME[idx] = value" (format lbl_803698DC).
void FSM_PrintInt(ScriptCtx* a0, u32 a1) {
    s32 idx = (s32)a1;
    s32 value = fn_801DFF54(FsmPrintBase(a0), idx);
    GameLog(1, 3, lbl_803698DC, lbl_805F8E90, idx, value);
}

// 0x80107040
// Debug log of one FSM float: "f32 NAME[idx] = value" (format lbl_803698C8).
void FSM_PrintFloat(ScriptCtx* a0, u32 a1) {
    s32 idx = (s32)a1;
    f32 value = fn_801DFF9C(FsmPrintBase(a0), idx);
    GameLog(1, 3, lbl_803698C8, lbl_805F8E90, idx, value);
}

// 0x80109174
// Deleting-destructor tail: frees p when del > 0, then returns p.
void* fn_80109174(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x80109BA4
// Stores three floats (A, B, A) in lbl_803E69C8 and calls fn_8032F1E8 on lbl_803C75C8 with
// callback fn_80109BE0, tags 0x40 / 0x7D0 and t = A.
void fn_80109BA4(void) {
    f32 a = lbl_805FEBF8;
    f32 b = lbl_805FEBFC;
    *(f32*)(lbl_803E69C8 + 0x0) = a;
    *(f32*)(lbl_803E69C8 + 0x4) = b;
    *(f32*)(lbl_803E69C8 + 0x8) = a;
    fn_8032F1E8(lbl_803C75C8, (void (*)(void))fn_80109BE0, 0, 0x40, 0x7D0);
}

// 0x80109BE0
// Initializes a new object: id from gIdPool at +0, clears +4, +0x38 and +0x3C, and writes the
// (argument-less) format string lbl_805F8EDC into the name buffer at +8.
void* fn_80109BE0(void* obj) {
    u8* o = (u8*)obj;
    *(void**)o = IdPoolNext(gIdPool);
    *(s32*)(o + 0x4) = 0;
    *(s32*)(o + 0x38) = 0;
    SysSprintf((char*)(o + 0x8), lbl_805F8EDC);
    *(s32*)(o + 0x3C) = 0;
    return obj;
}

// 0x80109C44
// Chunk observer: for chunk type 0x06 or 0x15, or type 0x67 with the word at +4 equal to 5, calls
// fn_8012AAA4 on gWorldState. Type is bits 5..11 (MSB numbering) of the chunk's first word.
void GameNetObserveChunkCallback(u32 a0) {
    u8* chunk = (u8*)(uintptr_t)a0;
    u32 type = (*(u32*)chunk >> 20) & 0x7F;
    if (type == 0x15 || type == 0x6) {
        fn_8012AAA4(gWorldState);
        return;
    }
    if (type == 0x67 && *(s32*)(chunk + 0x4) == 5) fn_8012AAA4(gWorldState);
}

// 0x8010A1C8
// Builds the hello-payload word: bit 31 = flag at gGame+0xC464 (bit 0), bits 0-1 = index at gGame+0xC45C
// (1 when outside 0..3), bits 30-31 = low two bits of the byte at lbl_803C4298+0x1F.
ScriptValue GetHelloPayloadCallback(void) {
    u32 flag = *(u32*)(gGame + 0xC464) & 1;
    s32 idx = *(s32*)(gGame + 0xC45C);
    if (idx < 0 || idx > 3) idx = 1;
    u32 packed = (flag << 31) | (u32)idx;
    u32 b = *(u8*)(lbl_803C4298 + 0x1F);
    return (ScriptValue)(s32)(packed | (b << 30));
}

// 0x8010A21C
// Applies a hello payload: stores bit 31 at gGame+0xC464 and its inverse at +0xC468, passes the low two
// bits to GameSetOption3 on gGame+0x5C8, and stores bit 30 in the byte at lbl_803C4298+0x1F.
void ProcessHelloPayloadCallback(BOOL a0) {
    u32 payload = (u32)a0;
    u32 flag = payload >> 31;
    *(u32*)(gGame + 0xC464) = flag;
    *(s32*)(gGame + 0xC468) = (flag == 0);
    GameSetOption3(gGame + 0x5C8, (s32)(payload & 3));
    lbl_803C4298[0x1F] = (u8)((payload >> 30) & 1);
}

// 0x8010A3C0
// Starts hosting: sets flag 0x20 in lbl_803C4298+0x3C when the movie player's vslot 0x3C returns bit 30,
// then signals the game flow with event 0xA6DC3EC0.
void GameNetStartHostGame(void) {
    fn_80132094();
    u8* movie = (u8*)lbl_805FB964;
    u32 r = (*(u32 (**)(u8*))(*(u8**)movie + 0x3C))(movie);
    if ((r >> 30) & 1) *(u32*)(lbl_803C4298 + 0x3C) |= 0x20;
    GameFlowSignal((s32)0xA6DC3EC0u, 0, 0);
}

// 0x8010A434
// Starts joining: if the object from fn_801F5408 exists, sets flag 0x40 in lbl_803C4298+0x3C when its
// +0x98 word has bit 30 set, then signals the game flow with event 0xA6DC3EC0.
void GameNetStartClientGame(void) {
    u8* net = fn_801F5408();
    if (net == 0) return;
    if ((*(u32*)(net + 0x98) >> 30) & 1) *(u32*)(lbl_803C4298 + 0x3C) |= 0x40;
    fn_80132094();
    GameFlowSignal((s32)0xA6DC3EC0u, 0, 0);
}

// 0x8010AB3C
// Two-phase GX BP write: clears bit 0x08 of the shadow byte lbl_805FB50C and sends BP word 0xFE00C000
// then 0 (no filter value).
void fn_8010AB3C(void) {
    lbl_805FB50C = (u8)(lbl_805FB50C & ~0x08);
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE00C000;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0;
}

// 0x8010AB70
// Two-phase GX BP write: sets bit 0x08 of the shadow byte lbl_805FB50C and sends BP word 0xFE00C000
// then (lbl_805F9212 << 14) as the filter value.
void fn_8010AB70(void) {
    lbl_805FB50C = (u8)(lbl_805FB50C | 0x08);
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE00C000;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = (u32)lbl_805F9212 << 14;
}

// 0x8010B184
// Resets a buffer holder: frees the buffer at +0 if set, clears +0 and +4, sets the float at +8 to
// lbl_805FEC50, and frees the holder when del > 0. Returns p (no-op for null).
void* fn_8010B184(void* p, s32 del) {
    if (p != 0) {
        u8* o = (u8*)p;
        if (*(void**)o != 0) BufferFree(*(void**)o);
        *(void**)o = 0;
        *(s32*)(o + 0x4) = 0;
        *(f32*)(o + 0x8) = lbl_805FEC50;
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x8010B4C0
// Deleting-destructor helper: sets the two words at +0xB04 and +0xB00 to -1 and frees obj when del > 0.
// Returns nothing (matches the existing prototype used by callers in small_800Cb.cpp).
void fn_8010B4C0(u8* obj, s32 del) {
    if (obj == 0) return;
    *(s32*)(obj + 0xB04) = -1;
    *(s32*)(obj + 0xB00) = -1;
    if (del > 0) operator_delete(obj);
}

// 0x8010BF6C
// Advances a ring index at +0xB00 (modulus at +0xB08) count times. When the index equals the value at
// +0xB04 it resets both to -1 instead.
void fn_8010BF6C(u8* obj, s32 count) {
    for (s32 i = 0; i < count; ++i) {
        s32 cur = *(s32*)(obj + 0xB00);
        if (*(s32*)(obj + 0xB04) == cur) {
            *(s32*)(obj + 0xB04) = -1;
            *(s32*)(obj + 0xB00) = -1;
        } else {
            s32 mod = *(s32*)(obj + 0xB08);
            *(s32*)(obj + 0xB00) = (cur + 1) % mod;
        }
    }
}

// 0x8010C784
// Unload callback: if the object pointed to by a0 is set, calls its vslot 0x10. Always returns 1.
u32 RenderObjectZoneUnloadCallback(u32 a0, u32 a1) {
    (void)a1;
    u8* obj = *(u8**)(uintptr_t)a0;
    if (obj != 0) (*(void (**)(u8*))(*(u8**)obj + 0x10))(obj);
    return 1;
}

}

