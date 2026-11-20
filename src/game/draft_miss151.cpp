#include "types.h"

extern "C++" s32 gZoneResourceSize;   // defined in game_flow.cpp

extern "C" {
void* ObjAlloc(u32 size);                                       // 0x801D4414
u32   StringHash(const char* s);                                // 0x801EB704
void* fn_80070400(u8* obj);                                     // 0x80070400
s32   fn_80079440(u8* p);                  // 0x80079440
void  fn_8010B2D8(u8* p, u8* item);                             // 0x8010B2D8
void  fn_801B7410(void* self, void* p4, void* p5, s32 p6, f32* pos); // 0x801B7410
s32   fn_801B7C58(u8* a, void* b, f32 dt);                    // 0x801B7C58
void* memset(void* p, int c, unsigned long n);
extern u8        gWorldState[];
extern s32       lbl_805FB124;
extern void*     lbl_805FB6A0;
extern u8        lbl_805FB6B0;
extern s32       lbl_805FB6B8;
extern u32       lbl_806010F8;
extern const char lbl_80383E20[];
extern f32       lbl_805FD688;
extern f32       lbl_805FD6C8;
extern f32       lbl_805FFB68;
extern f32       lbl_805FFB6C;
}

extern "C" {

// 0x80070458
s32 RaySceneTest(void* ray) {
    return lbl_805FB124;
}

// 0x801B3308
u8* fn_801B3308(void) {
    return (u8*)lbl_805FB6A0;
}

// 0x801ED538
s32 fn_801ED538(u8* a3) {
    s32 result = lbl_805FB6B8;
    if (a3[0] & 0x80) {
        result = *(s32*)a3;
    }
    return result;
}

// 0x801D8AD4
f32 fn_801D8AD4(u8* p) {
    return *(f32*)(p + 0xAC);
}

// 0x80058BE8
s32 fn_80058BE8(f32* a, f32* b) {
    return *a > *b;
}

// 0x80075E8C
void fn_80075E8C(u8* obj) {
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
    u8* entry = *(u8**)(zone + 0xA0);
    s32 i = 0;

    while (i < (s32)*(u8*)(zone + 0xA4)) {
        u32 id = *(u32*)(entry + 0x2C);
        switch (id) {
        case 0x68702C6Cu:
            fn_8010B2D8(obj + 0x224, entry);
            break;
        case 0xA255EB5Bu:
            fn_8010B2D8(obj + 0x230, entry);
            break;
        case 0x27D89125u:
            fn_8010B2D8(obj + 0x20C, entry);
            break;
        case 0xEDFD5612u:
            fn_8010B2D8(obj + 0x218, entry);
            break;
        }
        i += 1;
        *(s32*)(entry + 0x30) = *(s32*)(entry + 0x30) & ~1;
        entry += 0x4C;
    }
}

// 0x8007A4B4
void fn_8007A4B4(u8* obj, u8* src) {
    u8* tmp;
    u16 sx;
    u16 sy;
    f32 hw;
    f32 hh;
    f32 a;
    f32 b;

    *(void**)(obj + 0x0) = src;
    tmp = (u8*)fn_80070400(src);
    *(void**)(obj + 0x4) = tmp;

    hw = *(f32*)(obj + 0x1C);
    hh = *(f32*)(obj + 0x20);
    a = lbl_805FD6C8 * hw;
    b = lbl_805FD6C8 * hh;

    sx = *(u16*)(tmp + 0x4);
    sy = *(u16*)(tmp + 0x6);
    *(f32*)(obj + 0x28) = (f32)sx / hw;
    *(f32*)(obj + 0x24) = (f32)sy / hh;

    fn_80079440(tmp + 0x4);

    *(f32*)(obj + 0x14) = a * lbl_805FD688;
    *(f32*)(obj + 0xC) = -a * lbl_805FD688;
    *(f32*)(obj + 0x10) = -b * lbl_805FD688;
    *(f32*)(obj + 0x18) = b * lbl_805FD688;
}

// 0x800C6C88
void fn_800C6C88(u8* obj) {
    *(s32*)(obj + 0x0) = 0;
    *(s32*)(obj + 0x4) = 0;
    *(s32*)(obj + 0x8) = 0;
    *(s32*)(obj + 0xC) = 0;
    *(s32*)(obj + 0x10) = 0;
}

// 0x800C82BC
void fn_800C82BC(void) {
}

// 0x8018C4D8
void fn_8018C4D8(u8* obj) {
    u8* p = *(u8**)(obj + 0x4);
    if (p != NULL) {
        u8* q = *(u8**)(p + 0x14);
        *(s32*)(q + 0x1C) = 0;
    }
    *(s32*)(obj + 0x20) = -1;
    *(s32*)(obj + 0x14) = *(s32*)(obj + 0x14) | 0x20000;
}

// 0x801B5510
s32 fn_801B5510(u8* obj, u8* arg1, s32 arg2, f32* arg3, s32 arg4) {
    typedef s32 (*Fn)(u8*, u8*, s32, f32*, s32);
    u8* item;

    if ((*(Fn*)(*(u8**)obj + 0x10))(obj, arg1, arg2, arg3, arg4) == 0) {
        return 0;
    }
    item = (u8*)ObjAlloc(0x2C);
    if (item != NULL) {
        *(u8*)(item + 0x1C) = (u8)arg4;
        fn_801B7410(item, obj, arg1, arg2, arg3);
        return 1;
    }
    return 0;
}

// 0x801B6AA4
void fn_801B6AA4(u8* obj) {
    u8* p = *(u8**)(obj + 0x4);
    f32 x = *(f32*)(p + 0x0);
    f32 y = *(f32*)(p + 0x4);
    f32 z = *(f32*)(p + 0x8);
    (void)x;
    (void)y;
    (void)z;
}

// 0x801B7524
void fn_801B7524(void* p) {
    u8* obj = (u8*)p;
    u8* owner = *(u8**)(obj + 0x0);
    u8* list;
    u8* cur;

    if (owner == NULL) {
        return;
    }
    list = *(u8**)(obj + 0x4);
    if (list == NULL) {
        return;
    }
    if (*(u8**)(list + 0xB4) != obj) {
        return;
    }
    cur = *(u8**)(owner + 0xB0);
    if (cur != NULL) {
        if (cur == obj) {
            *(void**)(owner + 0xB0) = *(void**)(obj + 0x8);
        } else {
            while (cur != NULL && *(u8**)(cur + 0x8) != obj) {
                cur = *(u8**)(cur + 0x8);
            }
            if (cur != NULL) {
                *(void**)(cur + 0x8) = *(void**)(obj + 0x8);
            }
        }
    }
    *(void**)(*(u8**)(obj + 0x4) + 0xB4) = NULL;
    *(void**)(obj + 0x0) = NULL;
    *(void**)(obj + 0x4) = NULL;
}

// 0x801B7B60
void fn_801B7B60(void* anim, s32 mode, f32 rate) {
    u8* obj = (u8*)anim;
    f32 scaled;

    *(u16*)(obj + 0x78) = 0xFFFF;
    *(u16*)(obj + 0x7A) = 0xFFFF;
    *(u16*)(obj + 0x7C) = 0xFFFF;
    memset(obj, 0, 0x64);
    scaled = rate * lbl_805FFB6C;
    *(void**)(obj + 0x6C) = NULL;
    *(s32*)(obj + 0x64) = 0;
    *(s32*)(obj + 0x68) = 0;
    *(f32*)(obj + 0x70) = lbl_805FFB68;
    *(u16*)(obj + 0x78) = 0xFFFF;
    *(u16*)(obj + 0x7A) = 0xFFFF;
    *(u16*)(obj + 0x7C) = 0xFFFF;
    *(s32*)(obj + 0x74) = 0;
    *(u8*)(obj + 0x7E) = 0;
    *(u8*)(obj + 0x7F) = 0;
    *(void**)(obj + 0x6C) = (void*)fn_801B7C58;
    *(f32*)(obj + 0x0) = scaled;
    *(s32*)(obj + 0x10) = mode;
    if (lbl_805FB6B0 == 0) {
        lbl_806010F8 = StringHash(lbl_80383E20);
        lbl_805FB6B0 = 1;
    }
    *(u32*)(obj + 0x64) = lbl_806010F8;
}

}  // extern "C"
