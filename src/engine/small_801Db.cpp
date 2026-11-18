// More small engine functions in 0x801Dxxxx (camera, lights, hash table, frame effects).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  operator_delete(void* p);                                  // 0x801D426C
void  fn_801D55F4(void* p);                                      // 0x801D55F4 (no-op)
void  fn_801D55F0(void* p);                                      // 0x801D55F0 (no-op)
u8*   fn_801D55F8(u8* p, s32 del);                               // 0x801D55F8 (deleting dtor)
u8*   fn_801D2BB8(u8* p, s32 del);                               // 0x801D2BB8 (deleting dtor)
u8*   fn_801D9038(u8* p, s32 del);                               // 0x801D9038 (base deleting dtor, defined below)
void  fn_801D9164(u8* p, void* v);                               // 0x801D9164
u8*  fn_80142500(void* p);                                      // 0x80142500
void  fn_80172E84(u8* obj);                                      // 0x80172E84
void  fn_80184414(void* a, void* b);                             // 0x80184414
void* fn_801DA758(void* v);                                      // 0x801DA758
void* fn_801EB94C(void* p);                                      // 0x801EB94C
void  fn_8016195C(void* p, s32 a, s32 b, void* c);               // 0x8016195C
void* ObjAlloc(u32 size);                                        // 0x801D4414
int   SysSprintf(char* buf, const char* fmt, ...);
u32   RandomU32(void);                                           // 0x80143460
u8    SCGetAspectRatio(void);                                    // 0x8026A9D8: 1 = 16:9
void  __register_global_object(void* obj, void (*dtor)(void*), void* handle);  // 0x8032EFC8
}

extern f32 lbl_805FFE40;            // 0x805FFE40 (float constant, RandomU32 scale)
extern f32 lbl_805FFEF8;            // 0x805FFEF8 (camera aspect value, 4:3)
extern f32 lbl_805FFEFC;            // 0x805FFEFC (camera aspect value, 16:9)
extern u8  lbl_8040E5F0[];          // 0x8040E5F0 (frame effects object)
extern u8  lbl_8047D2E8[];          // 0x8047D2E8 (12-float matrix, copied by fn_801D679C)
extern u8  lbl_805FB7E1[];          // 0x805FB7E1 (debug logging suppressed when nonzero)
extern u8  lbl_805F9600[];          // 0x805F9600 (debug format string)
extern u8  lbl_805FB760[];          // 0x805FB760 (static object, sbss)
extern u8  lbl_805FB768[];          // 0x805FB768 (static object, sbss)
extern u8  lbl_805FB770[];          // 0x805FB770 (static object, sbss)
extern u8  lbl_8047C5E8[];          // 0x8047C5E8 (__register_global_object handle)
extern u8  lbl_8047C5F4[];          // 0x8047C5F4 (__register_global_object handle)
extern u8  lbl_80384430[];          // 0x80384430 (vtable, base class)
extern u8  lbl_80384470[];          // 0x80384470 (vtable)
extern u8  lbl_803844A8[];          // 0x803844A8 (vtable)
extern u8  lbl_803844E0[];          // 0x803844E0 (vtable)
extern u8  lbl_80384518[];          // 0x80384518 (vtable)
extern u32 lbl_805FB838;            // 0x805FB838 (64-bit frame counter, high word)
extern u32 lbl_805FB83C;            // 0x805FB83C (64-bit frame counter, low word)
extern u32 lbl_805FB854;            // 0x805FB854 (32-bit frame counter)
extern u32 lbl_805F9614;            // 0x805F9614 (cleared at frame end)

extern "C" {

// Calls the cleanup routine stored at vtable slot 0x10 with p as the argument.
static void CallVtblCleanup(u8* vtbl, u8* p) {
    ((void (*)(u8*))(*(void**)(vtbl + 0x10)))(p);
}

// Shared body of the derived deleting destructors: set vtable, run its cleanup,
// run the base cleanup (fn_801D9038 with del 0), then free when del > 0.
static u8* DerivedDelete(u8* p, s32 del, u8* vtbl) {
    if (p) {
        *(void**)p = (void*)vtbl;
        CallVtblCleanup(vtbl, p);
        fn_801D9038(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801D2B54
// Static initializer: registers the global objects at lbl_805FB760 and lbl_805FB770 with their destructors.
void fn_801D2B54(void) {
    fn_801D55F4(lbl_805FB760);
    __register_global_object(lbl_805FB760, (void (*)(void*))fn_801D55F8, lbl_8047C5E8);
    fn_80142500(lbl_805FB768);
    fn_801D55F0(lbl_805FB770);
    __register_global_object(lbl_805FB770, (void (*)(void*))fn_801D2BB8, lbl_8047C5F4);
}

// 0x801D5584
// Allocates an object of the requested size. When the debug flag byte is clear, formats a
// debug name from the two trailing arguments (the result is not kept).
void* fn_801D5584(u32 size, s32, s32, s32, s32 a7, s32 a8) {
    void* obj = ObjAlloc(size);
    if (lbl_805FB7E1[0] == 0) {
        char buf[0x40C];
        SysSprintf(buf, (const char*)lbl_805F9600, a7, a8);
    }
    return obj;
}

// 0x801D679C
// Copies the 12 floats at src into the global matrix at lbl_8047D2E8.
void fn_801D679C(void* /*unused*/, f32* src) {
    f32* dst = (f32*)lbl_8047D2E8;
    for (s32 i = 0; i < 12; i++) {
        dst[i] = src[i];
    }
}

// 0x801D7E9C
// Scales each row of the 3x3 float matrix at m by the matching component of the vector v (in place).
void fn_801D7E9C(f32* m, f32* v) {
    for (s32 row = 0; row < 3; row++) {
        for (s32 col = 0; col < 3; col++) {
            m[row * 3 + col] = m[row * 3 + col] * v[row];
        }
    }
}

// 0x801D8ADC
// Returns a pointer just past the last '\' or '/' in s, or s itself when there is none.
char* fn_801D8ADC(char* s) {
    s32 n = 0;
    while (s[n] != '\0') n++;
    for (s32 i = n; i >= 0; i--) {
        if (s[i] == '\\' || s[i] == '/') return s + i + 1;
    }
    return s;
}

// 0x801D8B50
// Resets the camera: picks the default aspect value from the screen aspect ratio (16:9 or 4:3)
// and clears the flags and counter at +0x8C, +0xA4, +0xE0 and +0xE1.
void CameraInit(void* camera) {
    u8* c = (u8*)camera;
    *(f32*)(c + 0xa8) = SCGetAspectRatio() ? lbl_805FFEFC : lbl_805FFEF8;
    *(u8*)(c + 0xa4) = 1;
    *(u8*)(c + 0xe0) = 0;
    *(u8*)(c + 0xe1) = 1;
    *(u32*)(c + 0x8c) = 0;
}

// 0x801D8D48
// End-of-frame camera bookkeeping: bumps the 64-bit frame counter and the 32-bit frame counter,
// and clears the flag at lbl_805F9614.
void CameraEndFrame(void* /*camera*/) {
    u32 lo = lbl_805FB83C;
    u32 hi = lbl_805FB838;
    u32 newLo = lo + 1;
    u32 carry = (newLo < lo) ? 1 : 0;
    lbl_805FB83C = newLo;
    lbl_805FB838 = hi + carry;
    lbl_805FB854 = lbl_805FB854 + 1;
    lbl_805F9614 = 0;
}

// 0x801D08DC
// Returns the float at p+4 plus a random offset in [-r, r) (r = first float argument).
f32 fn_801D08DC(u8* p, f32 r) {
    u32 u = RandomU32();
    f32 t = (f32)((f64)u * (f64)lbl_805FFE40);
    return ((-r) + (2.0f * r) * t) + *(f32*)(p + 0x4);
}

// 0x801D9038
// Base deleting destructor: sets the base vtable, runs its cleanup slot, frees when del > 0. Returns p.
u8* fn_801D9038(u8* p, s32 del) {
    if (p) {
        *(void**)p = (void*)lbl_80384430;
        CallVtblCleanup(lbl_80384430, p);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801D9508
// Deleting destructor for the class with vtable 0x80384470 (derives from the base at 0x801D9038).
u8* fn_801D9508(u8* p, s32 del) {
    return DerivedDelete(p, del, lbl_80384470);
}

// 0x801D99BC
// Sets the 3-float vector at +0x20 of the attached object from v, only when the attached object
// exists and the flag at +0x50 is set. Calls the base helper 0x801D9164 first.
void fn_801D99BC(u8* p, f32* v) {
    fn_801D9164(p, v);
    u8* target = *(u8**)(p + 0x48);
    if (target && *(u8*)(p + 0x50)) {
        *(f32*)(target + 0x20) = v[0];
        *(f32*)(target + 0x24) = v[1];
        *(f32*)(target + 0x28) = v[2];
    }
}

// 0x801D9A60
// Deleting destructor for the class with vtable 0x803844A8 (derives from the base at 0x801D9038).
u8* fn_801D9A60(u8* p, s32 del) {
    return DerivedDelete(p, del, lbl_803844A8);
}

// 0x801DA20C
// Deleting destructor for the class with vtable 0x803844E0 (derives from the base at 0x801D9038).
u8* fn_801DA20C(u8* p, s32 del) {
    return DerivedDelete(p, del, lbl_803844E0);
}

// 0x801DA858
// Deleting destructor for the class with vtable 0x80384518 (derives from the base at 0x801D9038).
u8* fn_801DA858(u8* p, s32 del) {
    return DerivedDelete(p, del, lbl_80384518);
}

// 0x801DA9C8
// If the object's pointer at +0x4C is set and other passes fn_801DA758, passes both to fn_80184414.
void fn_801DA9C8(u8* obj, void* other) {
    if (*(void**)(obj + 0x4c) == NULL || other == NULL) return;
    if (fn_801DA758(other) == NULL) return;
    void* r = fn_801DA758(other);
    void* cur = *(void**)(obj + 0x4c);
    if (cur == NULL) return;
    fn_80184414(cur, r);
}

// 0x801DB32C
// Looks up hash in a table of 0x2C-byte entries (base at +0, count at +4; key at entry+0).
// Returns fn_801EB94C(value) for the matching entry's value at entry+4, or NULL.
void* HashTableFind(void* table, u32 hash) {
    u8* t = (u8*)table;
    u32 count = *(u32*)(t + 0x4);
    u8* base = *(u8**)(t + 0x0);
    u8* found = NULL;
    for (u32 i = 0; i < count; i++) {
        if (hash == *(u32*)(base + i * 0x2c)) {
            found = base + i * 0x2c;
            break;
        }
    }
    if (found == NULL) return NULL;
    void* value = *(void**)(found + 0x4);
    if (value == NULL) return NULL;
    return fn_801EB94C(value);
}

// 0x801DB91C
// Frame effect: tints the screen with seven float parameters (sent to the effect object as type 4).
void FrameFxTint(f32 a0, f32 a1, f32 a2, f32 a3, f32 a4, f32 a5, f32 a6) {
    f32 v[7] = {a0, a1, a2, a3, a4, a5, a6};
    fn_8016195C(lbl_8040E5F0, 4, 0, v);
}

// 0x801DB98C
// Frame effect: egg-crate pattern with three float parameters (effect type 8).
void FrameFxEggcrate(f32 a0, f32 a1, f32 a2) {
    f32 v[3] = {a0, a1, a2};
    fn_8016195C(lbl_8040E5F0, 8, 0, v);
}

// 0x801DB9CC
// Frame effect: blur and tint with five float parameters (effect type 11).
void FrameFxBlurAndTint(f32 a0, f32 a1, f32 a2, f32 a3, f32 a4) {
    f32 v[6] = {a0, a1, a2, a3, a4, 0.0f};
    fn_8016195C(lbl_8040E5F0, 11, 0, v);
}

}  // extern "C"
