// PC platform layer: the pieces of the Metrowerks C/C++ runtime the game code calls by name.
// The standard C functions (memcpy, strlen, sprintf, ...) come from the Windows C runtime.
// No original code. Design: docs/port_runtime.md.
#include <math.h>

#include "types.h"

extern "C" {

// Registers a global object's destructor to run at exit. The PC build never unloads: nothing to do.
void __register_global_object(void* obj, void (*dtor)(void*), void* handle) {}

// Compiler helper: unsigned 64-bit integer to float.
f32 __cvt_ull_flt(u64 v) { return (f32)v; }

f64 ceil_(f64 x) { return ceil(x); }

// __construct_array: runs a constructor over an array in place. The original passes 1 as the
// constructor's second argument ("most derived object").
typedef void (*ArrayCtor)(void* obj, s32 flag);
void fn_8032F1E8(u8* arr, void (*ctor)(void), void (*dtor)(void*, s32), s32 size, s32 count) {
    for (s32 i = 0; i < count; i++) ((ArrayCtor)ctor)(arr + i * size, 1);
}

// __construct_new_array: the same for `new T[n]`. The block starts with a 16-byte header holding
// the element size and count; the array follows it.
void* fn_8032F028(void* mem, void* ctor, void* dtor, s32 size, s32 count) {
    if (!mem) return NULL;
    u8* arr = (u8*)mem + 0x10;
    *(s32*)mem = size;
    *(s32*)((u8*)mem + 4) = count;
    if (ctor) {
        for (s32 i = 0; i < count; i++) ((ArrayCtor)ctor)(arr + i * size, 1);
    }
    return arr;
}

}  // extern "C"
