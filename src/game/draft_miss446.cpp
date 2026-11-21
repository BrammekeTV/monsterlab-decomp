#include "types.h"

extern "C" {
void* MemAllocDefault(u32 size);                                // 0x801D4650
u8*   fn_801EB7A4(void* p, s32 a4, s32 a5, s32 a6, s32 a7);    // 0x801EB7A4
void* fn_801EB94C(void* p);                                     // 0x801EB94C
extern u8* lbl_805FB818;                                        // 0x805FB818 (list of queued quad records)
}

extern "C" {

// 0x801D7270
// Lazily creates the quad record pool at lbl_805FB818, then copies the three vertex positions
// (arg1, arg2, arg3 as f32 triples) and the 32-bit flags word (arg4) into a new record.
void fn_801D7270(u8* arg0, f32* arg1, f32* arg2, f32* arg3, u32 arg4) {
    if (lbl_805FB818 == NULL) {
        u8* pool = (u8*)MemAllocDefault(0x2C);
        lbl_805FB818 = pool;
        if (pool != NULL) {
            fn_801EB7A4(pool, 0x28, 4, 8, 1);
        }
    }
    u8* rec = (u8*)fn_801EB94C(lbl_805FB818);
    if (rec != NULL) {
        *(f32*)(rec + 0x0)  = *(f32*)((u8*)arg1 + 0x0);
        *(f32*)(rec + 0x4)  = *(f32*)((u8*)arg1 + 0x4);
        *(f32*)(rec + 0x8)  = *(f32*)((u8*)arg1 + 0x8);
        *(f32*)(rec + 0xC)  = *(f32*)((u8*)arg2 + 0x0);
        *(f32*)(rec + 0x10) = *(f32*)((u8*)arg2 + 0x4);
        *(f32*)(rec + 0x14) = *(f32*)((u8*)arg2 + 0x8);
        *(f32*)(rec + 0x18) = *(f32*)((u8*)arg3 + 0x0);
        *(f32*)(rec + 0x1C) = *(f32*)((u8*)arg3 + 0x4);
        *(f32*)(rec + 0x20) = *(f32*)((u8*)arg3 + 0x8);
        *(u32*)(rec + 0x24) = arg4;
    }
}

}  // extern "C"
