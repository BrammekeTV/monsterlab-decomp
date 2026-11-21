#include "types.h"

extern "C" {
void SysInit(void);
void GameInitLanguage(void);
void __register_global_object(void* obj, void (*dtor)(void*), void* handle);
void operator_delete(void* p);
u8* fn_801FB098(u8* p, s32 del);              // 0x801FB098
f32 fn_800B1D90(u8* p);                       // 0x800B1D90
extern u8  lbl_804FB850[];                    // 0x804FB850 (__register_global_object handle)
extern u8  lbl_805FBA10[];                    // 0x805FBA10 (global object)
extern u32 lbl_805F8B10[];                    // 0x805F8B10 (word table of keys)
extern f32 lbl_805FBA80;                      // 0x805FBA80 (screen width scale)
extern f32 lbl_805FDF5C;                      // 0x805FDF5C (float constant)
extern f32 lbl_805FDF64;                      // 0x805FDF64 (float constant)
extern f32 lbl_80600314;                      // 0x80600314 (float constant)
}

// gScreenWidth is declared at file scope with C++ linkage in catch_minigame.cpp.
extern "C++" s32 gScreenWidth;

// MemInit is defined with C++ linkage in engine.cpp.
extern "C++" void MemInit(void);

extern "C" {

// Deleting destructor (no base dtor): frees the block when del > 0, returns p.
// 0x801BC7E8
u8* fn_801BC7E8(u8* p, s32 del) {
    if ((p != NULL) && (del > 0)) {
        operator_delete(p);
    }
    return p;
}

// Deleting destructor (no base dtor): frees the block when del > 0, returns p.
// 0x801BC828
u8* fn_801BC828(u8* p, s32 del) {
    if ((p != NULL) && (del > 0)) {
        operator_delete(p);
    }
    return p;
}

// Resets a small state record at p (fields +0x00, +0x04, +0x20, +0x24, +0x28, +0x2C).
// 0x801C763C
void fn_801C763C(u8* p) {
    *(s32*)(p + 0x24) = 0;
    *(s32*)(p + 0x4) = 0;
    *(s32*)(p + 0x0) = 0;
    *(s32*)(p + 0x20) = 4;
    *(s32*)(p + 0x28) = -1;
    *(s32*)(p + 0x2C) = -1;
    *(s32*)(p + 0x28) = 1;
    *(s32*)(p + 0x24) = 0;
}

// Global init: system, language, memory, then registers the global object's dtor.
// 0x801FB058
void fn_801FB058(void) {
    SysInit();
    GameInitLanguage();
    MemInit();
    __register_global_object(lbl_805FBA10, (void (*)(void*))fn_801FB098, lbl_804FB850);
}

// Deleting destructor (no base dtor): frees the block when del > 0, returns p.
// 0x801FB098
u8* fn_801FB098(u8* p, s32 del) {
    if ((p != NULL) && (del > 0)) {
        operator_delete(p);
    }
    return p;
}

// Deleting destructor (no base dtor): frees the block when del > 0, returns p.
// 0x801FCF74
u8* fn_801FCF74(u8* p, s32 del) {
    if ((p != NULL) && (del > 0)) {
        operator_delete(p);
    }
    return p;
}

// Returns a scaled value: fn_800B1D90 of the record's +0x94 pointer times a constant,
// or a fallback constant when that pointer is NULL.
// 0x800A9B40
f32 fn_800A9B40(u8* p) {
    u8* sub = *(u8**)(p + 0x94);
    if (sub != NULL) {
        f32 scale = fn_800B1D90(sub);
        return lbl_805FDF5C * scale;
    }
    return lbl_805FDF64;
}

// Returns 1 if key appears in the table lbl_805F8B10 (count from record +0x10), else 0.
// 0x8009F6F8
s32 fn_8009F6F8(u8* p, u32 key) {
    s32 count = *(s32*)(p + 0x10);
    s32 i;
    for (i = 0; i < count; i++) {
        if (key == lbl_805F8B10[i]) {
            return 1;
        }
    }
    return 0;
}

// Stores the half-width-style scale: screen width times a float constant.
// 0x801FCD64
void fn_801FCD64(void) {
    lbl_805FBA80 = (f32)gScreenWidth * lbl_80600314;
}

}  // extern "C"
