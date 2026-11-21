// Static initialiser of the game's main translation unit: constructs the game object at
// 0x803C4298, the frame timer, the first ten words of the global settings (the sizes of the
// memory areas reserved for .sir images) and a small list object.
// Original: 0x80007B38 (auto_03_80007A8C_text.s), 0x800109C4 (auto_fn_800109C4_text.s)
#include <string.h>
#include "types.h"

extern "C" {
void  fn_80122B5C(u8* p);                       // 0x80122B5C
u8*   fn_801E763C(u8* arg0);                    // 0x801E763C
u8*   fn_80142500(u8* p);                       // 0x80142500: timer constructor
void  __register_global_object(void* obj, void (*dtor)(void*), void* handle);   // 0x8032EFC8

extern u8  lbl_803C4288[];                      // 0x803C4288: this file's .bss (the game object is at +0x10)
extern f32 lbl_803C4998[3];                     // 0x803C4998
extern u8  lbl_805FAEC0[];                      // 0x805FAEC0: a timer
extern u8  lbl_805FB618;                        // 0x805FB618
}

void* fn_80010AC0(void* self, s32 flags);       // 0x80010AC0: destructor of the game object
void* fn_80010B1C(void* self, s32 flags);       // 0x80010B1C: destructor of the list object

// 0x80007B38
// Constructor of the game object. The second argument (a vector) and the float are not used.
extern "C" u8* fn_80007B38(u8* self) {
    fn_80122B5C(self + 0x20);
    fn_801E763C(self + 0x40);
    for (s32 i = 0; i < 8; i++) {
        memset(self + 0x6C + i * 0x18, 0, 0x18);
    }
    *(u8**)(self + 0x68) = self + 0x6C;
    *(u32*)(self + 0x64) = 8;
    *(s32*)(self + 0x60) = -1;
    *(s32*)(self + 0x5C) = -1;
    *(u32*)(self + 0x58) = 0;
    *(u32*)(self + 0x138) = 0;
    *(u32*)(self + 0x13C) = 0;
    *(u32*)(self + 0x3C) = 0;
    self[0x1C] = 0;
    self[0x1D] = 0;
    self[0x1E] = 0;
    self[0x1F] = 0;
    *(f32*)(self + 0x140) = 0.0f;
    *(u32*)(self + 0x148) = 0;
    *(u32*)(self + 0x14C) = 0;
    *(u32*)(self + 0x150) = 0;
    *(u32*)(self + 0x154) = 0;
    *(u32*)(self + 0x158) = 0;
    *(u32*)(self + 0x15C) = 0;
    *(f32*)(self + 0x160) = -1.0f;
    *(f32*)(self + 0x164) = 0.0f;
    lbl_805FB618 = 1;
    return self;
}

// 0x800109C4
extern "C" void fn_800109C4(void) {
    lbl_803C4998[0] = 0.0f;
    lbl_803C4998[1] = 1.0f;
    lbl_803C4998[2] = 0.0f;
    fn_80007B38(lbl_803C4288 + 0x10);
    __register_global_object(lbl_803C4288 + 0x10, (void (*)(void*))fn_80010AC0, lbl_803C4288);
    fn_80142500(lbl_805FAEC0);

    // 0x803C44A0: sizes in bytes, copied into the global settings by fn_8000E488. The first is
    // the room for global_binary.sir at 0x90400000.
    u32* sizes = (u32*)(lbl_803C4288 + 0x218);
    sizes[0] = 0x140000;
    sizes[1] = 0x440000;
    sizes[2] = 0x01200000;
    sizes[3] = 0x100000;
    sizes[4] = 0x300000;
    sizes[5] = 0x1A0000;
    sizes[6] = 0x1A0000;
    sizes[7] = 0x200000;
    sizes[8] = 0x380000;
    sizes[9] = 0x480000;

    u8* list = lbl_803C4288 + 0x258;
    *(u8**)(list + 0x14) = list + 0x18;
    *(u32*)(list + 0x10) = 8;
    *(s32*)(list + 0xC) = -1;
    *(s32*)(list + 0x8) = -1;
    *(u32*)(list + 0x4) = 0;
    __register_global_object(list, (void (*)(void*))fn_80010B1C, lbl_803C4288 + 0x24C);
}
