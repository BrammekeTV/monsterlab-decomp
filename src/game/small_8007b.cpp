// More small functions in 0x8007xxxx (experiment sounds, dance, welding).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  fn_8020A398(s32 handle);                                          // 0x8020A398
void  fn_80062CE0(u8* p, s32 del);                                      // 0x80062CE0 (base dtor)
void  fn_800690D4(u8* p, s32 a, void* params, s32 b);                   // 0x800690D4 (base ctor)
void  operator_delete(void* p);                                         // 0x801D426C
void  fn_8006FA44(u8* a3, s32 a4, s32 a5, s32 a6, s32 a7, s32 a8);      // 0x8006FA44
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void  PropPlayAnim(void* prop, int a, int b, BOOL once);                // 0x80069350
void  fn_80070460(u8* p);                                               // 0x80070460
s32   fn_8021F9B8(u8* p);                                               // 0x8021F9B8
u8    fn_8022015C(u8* p);                                               // 0x8022015C
void  fn_800773CC(u8* p, f32 v);                                        // 0x800773CC
u8*   fn_800D8F70(u8* a3, u8* a4);                                      // 0x800D8F70
void  fn_800D9180(u8* a3, f32 f1);                                      // 0x800D9180
s32   fn_8007B318(u8* a3);                                              // 0x8007B318
void  fn_8007B320(u8* a3, u8* a4);                                      // 0x8007B320
void  fn_8007BED8(u8* a3);                                              // 0x8007BED8
s32   fn_8007C11C(u8* a3);                                              // 0x8007C11C
f32   fn_8007C138(u8* a3);                                              // 0x8007C138
void  fn_80079CBC(u8* chr);                                             // 0x80079CBC
void  fn_80079C78(u8* a);                                               // 0x80079C78 (a is overwritten before use)
}

extern u8  lbl_803C5740[];         // experiment sound table (16-byte entries, handle at +0xC)
extern s32 lbl_805FB12C;           // experiment sound table count
extern u8  lbl_8035A89C[];         // vtable
extern u8  lbl_8035A9F0[];         // vtable
extern u8  lbl_803C68B0[];         // block passed to fn_8006FA44
extern u8  lbl_803C68C4[];         // float triple
extern f32 lbl_805FD650;           // float
extern f32 lbl_805FD658;           // float
extern f32 lbl_805FD668;           // float
extern f32 lbl_805FD680;           // float
extern f32 lbl_805FD68C;           // float
extern u8  lbl_805FB124[];         // index into lbl_803474B8 (s32 value)
extern u8  lbl_805FB140[];         // welding state pointer
extern u8  lbl_803474B8[];         // float table
extern void* gEventMgr;            // 0x805FB6C8

extern "C" {

// 0x80070ED0
// Stops every experiment sound still playing: each table entry with a handle other than -1 is
// stopped and its handle reset to -1.
void StopAllExperimentSounds(void) {
    u8* e = lbl_803C5740;
    for (s32 i = 0; i < lbl_805FB12C; i++, e += 0x10) {
        if (*(s32*)(e + 0xC) != -1) {
            fn_8020A398(*(s32*)(e + 0xC));
            *(s32*)(e + 0xC) = -1;
        }
    }
}

// 0x80074CC8
// Initialiser: stores a at +0, the vtable at +4, and sets ten slots at +0x08 to 0x10B2 and
// ten slots at +0x30 to 0. Then +0x58 = 0x10B2, +0x5C = -1, +0x60 = 0 and +0x64 = 0.
u8* fn_80074CC8(u8* p, void* a) {
    *(void**)(p + 0x0) = a;
    *(void**)(p + 0x4) = (void*)lbl_8035A89C;
    for (s32 i = 0; i < 10; i++) {
        *(s32*)(p + 0x8 + i * 4) = 0x10B2;
        *(s32*)(p + 0x30 + i * 4) = 0;
    }
    *(s32*)(p + 0x58) = 0x10B2;
    *(s32*)(p + 0x5C) = -1;
    *(s32*)(p + 0x60) = 0;
    *(s32*)(p + 0x64) = 0;
    return p;
}

// 0x800768B8
// Plays the dance animation on the prop if it has an attached animation (+0x9C, flag +0x48):
// starts the prop animation, then sets a flag (+0x20) on the sub-object reached through the
// virtual call at slot 0x10.
void PlayDanceAnimation(void* a0, u32 a1) {
    u8* self = (u8*)a0;
    u8* obj = *(u8**)(self + 0x9C);
    if (obj != NULL && *(s32*)(obj + 0x48) != 0) {
        PropPlayAnim(self, (int)a1, 0, TRUE);
        typedef u8* (*GetSubFn)(u8*);
        GetSubFn get = *(GetSubFn*)(*(u8**)self + 0x10);
        u8* ret = get(self);
        u8* x = *(u8**)ret;
        u8* y = *(u8**)(x + 0x14);
        *(s32*)(y + 0x20) = 1;
    }
}

// 0x80076A6C
// Sets the three floats at lbl_803C68C4 ({a, b, a}) and tail-calls fn_8006FA44 on lbl_803C68B0
// with fixed hashes.
void fn_80076A6C(void) {
    *(f32*)(lbl_803C68C4 + 0x0) = lbl_805FD650;
    *(f32*)(lbl_803C68C4 + 0x4) = lbl_805FD658;
    *(f32*)(lbl_803C68C4 + 0x8) = lbl_805FD650;
    fn_8006FA44(lbl_803C68B0, 0, 0x8001, (s32)0xF7B53F, (s32)0x85AB5092u, (s32)0xE76CC814u);
}

// 0x80076AB8
// Constructor: base ctor fn_800690D4, clears the byte at +0x26C, sets vtable lbl_8035A9F0.
void* fn_80076AB8(void* obj, s32 a, void* params, s32 b) {
    u8* p = (u8*)obj;
    fn_800690D4(p, a, params, b);
    *(u8*)(p + 0x26C) = 0;
    *(void**)p = (void*)lbl_8035A9F0;
    return p;
}

// 0x80076AFC
// Deleting destructor: if non-null, base dtor fn_80062CE0 (with del 0), then free if del > 0.
void* fn_80076AFC(void* obj, s32 del) {
    u8* p = (u8*)obj;
    if (p != NULL) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80076B5C
// Stores the arguments in +0x270 (a) and +0x264 (f), clears +0x268 to lbl_805FD668 and sets
// the flag byte at +0x26C, then sends event 0x4608BB44 to the event manager with p as target.
void fn_80076B5C(u8* p, void* a, f32 f) {
    *(void**)(p + 0x270) = a;
    *(f32*)(p + 0x264) = f;
    *(f32*)(p + 0x268) = lbl_805FD668;
    *(u8*)(p + 0x26C) = 1;
    EventSend(gEventMgr, p, NULL, 0x4608BB44u, 0, 0, 0, 0);
}

// 0x80077544
// Sets the flag at +0x123C, then sends the value of the object at +0x14 (as an unsigned int,
// converted to float) divided by the float at +0x4 to fn_800773CC.
void fn_80077544(u8* p) {
    *(u8*)(p + 0x123C) = 1;
    s32 v = fn_8021F9B8(*(u8**)(p + 0x14));
    f32 q = (f32)(u32)v / *(f32*)(p + 0x4);
    fn_800773CC(p, q);
}

// 0x80077A90
// Returns 1 if both the object at +0x14 and the object at +0x20 pass fn_8022015C, else 0.
s32 fn_80077A90(u8* p) {
    s32 r = 0;
    if (fn_8022015C(*(u8**)(p + 0x14)) != 0 && fn_8022015C(*(u8**)(p + 0x20)) != 0) r = 1;
    return r;
}

// 0x80077AEC
// Stops the two sound handles at +0x12E4 and +0x12E8 and resets both to -1.
void fn_80077AEC(u8* p) {
    fn_8020A398(*(s32*)(p + 0x12E4));
    fn_8020A398(*(s32*)(p + 0x12E8));
    *(s32*)(p + 0x12E4) = -1;
    *(s32*)(p + 0x12E8) = -1;
}

// 0x80077B50
// Welding gun off: runs fn_80079C78 on the entity, then sets the state at +0x290 to 3.
void WeldingGunOffUpdate(ScriptCtx* a0) {
    u8* chr = *(u8**)((u8*)a0 + 0x10);
    fn_80079C78(chr);
    *(s32*)(chr + 0x290) = 3;
}

// 0x80077B88
// Welding gun on: runs fn_80079C78 and then fn_80079CBC on the entity.
void WeldingGunOnUpdate(ScriptCtx* a0) {
    u8* chr = *(u8**)((u8*)a0 + 0x10);
    fn_80079C78(chr);
    fn_80079CBC(chr);
}

// 0x80077BF4
// True when the entity's welding state (fn_8007B318) is 1 or 2.
s32 WeldingOverTrench(ScriptCtx* a0) {
    u8* chr = *(u8**)((u8*)a0 + 0x10);
    if (fn_8007B318(chr) == 1) return 1;
    return fn_8007B318(chr) == 2;
}

// 0x80077C50
// True when the weld quality (fn_8007C138) is at least lbl_805FD680.
s32 WeldingBadWeld(ScriptCtx* a0) {
    u8* chr = *(u8**)((u8*)a0 + 0x10);
    f32 q = fn_8007C138(chr);
    return q >= lbl_805FD680;
}

// 0x80077C88
// Bit trick on the shake result (fn_8007C11C): returns 1 when the result is 1 or 2 (threshold
// beaten), computed as the original does with xor, and, and subtract.
s32 WeldingShakeThresholdBeaten(ScriptCtx* a0) {
    u8* chr = *(u8**)((u8*)a0 + 0x10);
    s32 v = fn_8007C11C(chr);
    s32 x = v ^ 3;
    s32 r4 = x >> 1;
    s32 r0 = x & v;
    r0 = r4 - r0;
    return (s32)((u32)r0 >> 31);
}

// 0x80079C78
// Looks up the object at +0x758 and +0x75C of the global block at lbl_805FB140 and forwards
// both to fn_800D8F70 and fn_8007B320 with a scratch buffer.
void fn_80079C78(u8* a) {
    (void)a;
    u8 tmp[0x10];
    u8* obj = *(u8**)lbl_805FB140;
    if (obj != NULL) {
        fn_800D8F70(*(u8**)(obj + 0x758), tmp);
        fn_8007B320(*(u8**)(*(u8**)lbl_805FB140 + 0x75C), tmp);
    }
}

// 0x80079D40
// Sets the rate at +0x758 to 1.0 via fn_800D9180, sends event 0xCA90298E to the object at
// +0x75C, then calls fn_8007BED8 on it.
void fn_80079D40(u8* p) {
    fn_80070460(p);
    fn_800D9180(*(u8**)(p + 0x758), lbl_805FD68C);
    EventSend(gEventMgr, *(u8**)(p + 0x75C), NULL, 0xCA90298Eu, 0, 0, 1, 0);
    fn_8007BED8(*(u8**)(p + 0x75C));
}

// 0x80079DA8
// Sets the rate at +0x758 from the table lbl_803474B8 indexed by lbl_805FB124, then sends
// event 0xF15918A6 to the object at +0x75C.
void fn_80079DA8(u8* p) {
    s32 idx = *(s32*)lbl_805FB124;
    f32 rate = ((f32*)lbl_803474B8)[idx];
    fn_800D9180(*(u8**)(p + 0x758), rate);
    EventSend(gEventMgr, *(u8**)(p + 0x75C), NULL, 0xF15918A6u, 0, 0, 1, 0);
}

// 0x80079E20
// Sets the rate at +0x758 to lbl_805FD68C, sends event 0xC608C842 to the object at +0x75C,
// stores lbl_805FD68C at +0x110C, then calls fn_8007BED8 on the object.
void fn_80079E20(u8* p) {
    fn_800D9180(*(u8**)(p + 0x758), lbl_805FD68C);
    EventSend(gEventMgr, *(u8**)(p + 0x75C), NULL, 0xC608C842u, 0, 0, 1, 0);
    *(f32*)(p + 0x110C) = lbl_805FD68C;
    fn_8007BED8(*(u8**)(p + 0x75C));
}

}  // extern "C"
