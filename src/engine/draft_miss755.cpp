// Misc 0x8017xxxx helpers: id remap, heap free thunks, record alloc, light-direction GX setup.
#include "types.h"
#include "revolution.h"
#include <math.h>

extern "C" {
void  fn_8017A618(void* p);                                              // 0x8017A618
s32   fn_8017B9BC(void* a, void* b, void* c, void* d);                  // 0x8017B9BC

extern u8  lbl_80412B10[];   // 0x80412B10 (4 record pointers)
extern s32 lbl_805FB598;     // 0x805FB598
extern u8* lbl_805FB608;     // 0x805FB608 (heap object: slot +0 = alloc, slot +4 = free, called as (p, heap))
extern u8  lbl_805FB618;     // 0x805FB618 (enable flag)

typedef void* (*HeapAllocFn)(u32, u32, u32, void*, u32, void*);
typedef void  (*HeapFreeFn)(void*, void*);

// 0x80178E9C
// Maps an id (0, 1, 0x300..0x308, 0x8001..0x8004) to a dense index 0..14; 0 for unknown ids.
s32 fn_80178E9C(s32 id) {
    s32 r = 0;
    switch (id) {
    case 0x0:    r = 0;   break;
    case 0x1:    r = 1;   break;
    case 0x300:  r = 2;   break;
    case 0x301:  r = 3;   break;
    case 0x302:  r = 4;   break;
    case 0x303:  r = 5;   break;
    case 0x304:  r = 6;   break;
    case 0x305:  r = 7;   break;
    case 0x306:  r = 8;   break;
    case 0x307:  r = 9;   break;
    case 0x308:  r = 0xA; break;
    case 0x8001: r = 0xB; break;
    case 0x8002: r = 0xC; break;
    case 0x8003: r = 0xD; break;
    case 0x8004: r = 0xE; break;
    }
    return r;
}

// 0x80178FB0
// Tail-calls the heap object's free slot (+4) with (p, heap).
void fn_80178FB0(void* p) {
    (*(HeapFreeFn*)(lbl_805FB608 + 4))(p, lbl_805FB608);
}

// 0x8017A090
// Sets byte +0xD of the sub-record at +0x50 from byte +0x136 (2 -> 2, 3 -> 3, else 1), and +0xE = 1.
void fn_8017A090(u8* p) {
    switch (*(u8*)(p + 0x136)) {
    case 2:  *(u8*)(*(u8**)(p + 0x50) + 0xD) = 2; break;
    case 3:  *(u8*)(*(u8**)(p + 0x50) + 0xD) = 3; break;
    default: *(u8*)(*(u8**)(p + 0x50) + 0xD) = 1; break;
    }
    *(u8*)(*(u8**)(p + 0x50) + 0xE) = 1;
}

// 0x8017ACCC
// Allocates a 0xD20-byte record (owner field points back at p), initialises it, returns it.
void* fn_8017ACCC(u8* p) {
    void* rec = (*(HeapAllocFn*)lbl_805FB608)(0xD20, 0x10, 0x12, p ? (void*)(p + 0x80) : (void*)0, 0, lbl_805FB608);
    *(u8**)rec = p;
    fn_8017A618(rec);
    return rec;
}

// 0x8017D844
// Same free thunk as fn_80178FB0.
void fn_8017D844(void* p) {
    (*(HeapFreeFn*)(lbl_805FB608 + 4))(p, lbl_805FB608);
}

// 0x80181DE0
// If lighting is enabled: normalises the vector at arg1+0x68 and writes it (scaled by 1e19) into
// four XF light/matrix register loads; then draws via fn_8017B9BC with the record for the type.
void fn_80181DE0(u8** arg0, u8* arg1) {
    f32 d[3];
    u8* rec_p = *(u8**)(lbl_80412B10 + (*(u8*)(*arg0 + 0xD)) * 4);
    if (lbl_805FB618 != 0) {
        f32 x = *(f32*)(arg1 + 0x68);
        f32 y = *(f32*)(arg1 + 0x6C);
        f32 z = *(f32*)(arg1 + 0x70);
        f32 m = z * z + (x * x + y * y);
        if (m == 0.0f) {
            d[0] = 0.0f; d[1] = 0.0f; d[2] = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(m);
            d[0] = x * inv; d[1] = y * inv; d[2] = z * inv;
        }
        f32 a = d[0] * 1e19f;
        f32 b = d[1] * 1e19f;
        f32 c = d[2] * 1e19f;
        GXWGFifo._u8  = 0x10;
        GXWGFifo._u16 = 2;
        GXWGFifo._u16 = 0x64A;
        GXWGFifo._f32 = a;
        GXWGFifo._f32 = b;
        GXWGFifo._f32 = c;
        GXWGFifo._u8  = 0x10;
        GXWGFifo._u16 = 2;
        GXWGFifo._u16 = 0x64D;
        GXWGFifo._f32 = 0.0f;
        GXWGFifo._f32 = 0.0f;
        GXWGFifo._f32 = 1.0f;
        GXWGFifo._u8  = 0x10;
        GXWGFifo._u16 = 2;
        GXWGFifo._u16 = 0x644;
        GXWGFifo._f32 = 1.0f;
        GXWGFifo._f32 = 0.0f;
        GXWGFifo._f32 = 0.0f;
        GXWGFifo._u8  = 0x10;
        GXWGFifo._u16 = 2;
        GXWGFifo._u16 = 0x647;
        GXWGFifo._f32 = 1.0f;
        GXWGFifo._f32 = 0.0f;
        GXWGFifo._f32 = 0.0f;
    }
    fn_8017B9BC(arg0, &lbl_805FB598, rec_p, arg1);
}

// 0x80181F80
// Tail call: fn_8017B9BC(record for type at (*p)+0xD, &lbl_805FB598, p, 0).
s32 fn_80181F80(u8** p) {
    return fn_8017B9BC(*(u8**)(lbl_80412B10 + (*(u8*)(*p + 0xD)) * 4), &lbl_805FB598, p, 0);
}

}  // extern "C"
