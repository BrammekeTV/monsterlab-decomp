#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
// Callees. Prototypes match the existing definitions where those exist; the
// rest are taken from the asm at each call site.
void* ScreenSetsCurrent(void* sets);
void* ScreenSetFindElement(void* set, u32 hash);
int SysSprintf(char* buf, const char* fmt, ...);
void __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count);
void __register_global_object(void* obj, void (*dtor)(void*), void* handle);
void fn_80121494(u8* p, u8* v, f32 f);                    // 0x80121494 (3 args per asm)
void fn_80021ECC(void);                                   // 0x80021ECC
void fn_800223F0(u8* p, f32 v);                           // 0x800223F0
void fn_80023160(void);                                   // 0x80023160
void fn_8003A2B4(u8* p);                                  // 0x8003A2B4 (r3 = arg0 passed through)
u8* fn_80062CE0(u8* p, s32 del);                          // 0x80062CE0
void fn_8006FAF4(u8* p, s32 a);                           // 0x8006FAF4
void fn_80070FAC(u8* p, f32 v);                           // 0x80070FAC
void fn_800773CC(u8* p, f32 v);                           // 0x800773CC
s32 fn_80077A90(u8* p);                                   // 0x80077A90
void fn_80077AEC(u8* p);                                  // 0x80077AEC
void fn_8007617C(u8* p);                                  // 0x8007617C
void fn_800A7D88(void* obj, u32 hash);                    // 0x800A7D88
void fn_800AC5EC(u8* p);                                  // 0x800AC5EC
f32 fn_800C3E84(void);                                    // 0x800C3E84
BOOL fn_801B50B0(void* unused, u8* obj);                  // 0x801B50B0
s32 fn_801B5338(u8* obj, u8* target, u32 hash, f32* vec, u8 flag);  // 0x801B5338
void fn_801F5E50(u8* out, f32 dz);                        // 0x801F5E50
u8* fn_801F69B8(u8* p, s32 del);                          // 0x801F69B8
u8* fn_801F9598(u8* p, s32 del);                          // 0x801F9598
u32 fn_8019552C(u8* list, s32 index);                     // 0x8019552C
s32 fn_8022E438(u8* a);                                   // 0x8022E438
void fn_80094C0C(u8* p, u8* tbl);                         // 0x80094C0C
u8* fn_80097590(u8* p, s32 del);                          // 0x80097590
void* memset(void* p, int c, unsigned long n);
void operator_delete(void* p);

extern u8 lbl_803C4258[];
extern u8 lbl_803C2A00[];
extern u8 lbl_8035C208[];
extern u8 lbl_8035E568[];
extern u8 lbl_803C53E8[];
extern u8 lbl_805F874C[];
extern u8 lbl_805FBBE0[];
extern u8* gGameDb;
extern u8 lbl_805FB1A1;
extern f32 lbl_805FC630;
extern f32 lbl_805FC660;
extern f32 lbl_805FCCC8;
extern f32 lbl_805FCFC4;
extern f32 lbl_805FD304;
extern f32 lbl_805FD318;
extern f32 lbl_805FD374;
extern f32 lbl_805FD650;
extern f32 lbl_805FD660;
extern f32 lbl_805FE018;
extern f32 lbl_805FE3E0;
extern f32 lbl_805FE820;

typedef s32 (*FnStep57548)(u8*, u8*, s32, s32, s32, f32);
typedef void (*FnVoidPP)(u8*, u8*);
}

extern "C++" void* gScreenSets;
extern "C++" void* fn_8000C41C(void* self, s32 flags);  // 0x8000C41C (C++ linkage in game_spawn.cpp)

extern "C" {

// 0x800ACE48
void fn_800ACE48(u8* arg0) {
    u8* h;
    u8* o;

    h = *(u8**)(arg0 + 0x274);
    *(s32*)(arg0 + 0x264) = -1;
    *(s32*)(arg0 + 0x268) = -1;
    *(s32*)(arg0 + 0x26C) = -1;
    *(s32*)(arg0 + 0x270) = 0;
    if (h != NULL) {
        fn_801B50B0(h, arg0);
        o = *(u8**)(arg0 + 0x4);
        *(s32*)(o + 0x20) |= 1;
        *(void**)(arg0 + 0x274) = NULL;
    }
    h = *(u8**)(arg0 + 0x278);
    if (h != NULL) {
        fn_801B50B0(arg0, h);
        o = *(u8**)(h + 0x4);
        *(s32*)(o + 0x20) |= 1;
        *(void**)(arg0 + 0x278) = NULL;
    }
}

// 0x800AD29C
void fn_800AD29C(u8* arg0, u8* arg1) {
    f32 vec[3];
    u8* o;

    fn_800AC5EC(arg1);
    *(void**)(arg0 + 0x278) = arg1;
    vec[0] = lbl_805FE018;
    vec[1] = lbl_805FE018;
    vec[2] = lbl_805FE018;
    if (fn_801B5338(arg0, *(u8**)(arg0 + 0x278), 0xF5343D32U, vec, 0U) == 1) {
        o = *(u8**)(*(u8**)(arg0 + 0x278) + 0x4);
        *(s32*)(o + 0x20) &= 0xFFFFFFFE;
    }
    fn_800A7D88(arg0, 0xA61CF023U);
    fn_800A7D88(*(u8**)(arg0 + 0x278), 0xA61CF023U);
}

// 0x800B7A64
u8* fn_800B7A64(u8* arg0, s32 arg1) {
    u8* h;
    u8* o;

    if (arg0 != NULL) {
        h = *(u8**)(arg0 + 0x268);
        *(u8**)(arg0 + 0x0) = lbl_8035E568;
        if (h != NULL) {
            fn_801B50B0(h, arg0);
            o = *(u8**)(arg0 + 0x4);
            *(s32*)(o + 0x20) |= 1;
            *(void**)(arg0 + 0x268) = NULL;
        }
        fn_80062CE0(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x801A51FC
s32 fn_801A51FC(u8* arg0) {
    s32 n;
    s32 i;
    s32 count;
    u8* ent;
    s32 v;

    n = 0;
    count = *(s32*)(arg0 + 0x13C) * *(s32*)(arg0 + 0x140);
    for (i = 0; i < count; i++) {
        ent = (u8*)(uintptr_t)fn_8019552C(*(u8**)(arg0 + 0x1BC), i);
        v = (ent != NULL) ? *(s32*)ent : 0;
        if (v != 0) {
            n += 2;
        }
    }
    if (*(s32*)(arg0 + 0x1B4) != 0) {
        n += 1;
    }
    return n;
}

// 0x80025918
void fn_80025918(u8* arg0) {
    s32 mode;

    mode = (s32)*(f32*)(arg0 + 0x6C);
    if ((mode == 0xF && *(s32*)(arg0 + 0xE8) > 2 && *(u8*)(arg0 + 0xB10) == 0) ||
        (mode == 0x7 && *(s32*)(arg0 + 0xE8) > 1 && *(u8*)(arg0 + 0xB11) == 0)) {
        if (*(u8*)(arg0 + 0xB10) == 0) {
            *(u8*)(arg0 + 0xB10) = 1;
        } else {
            *(u8*)(arg0 + 0xB11) = 1;
        }
        fn_80023160();
    }
}

// 0x80057548
s32 fn_80057548(u8* arg0, u8* arg1) {
    u8* obj6;
    u8* r9c;
    u8* r8;
    f32 f2;
    f32 f0;
    f32 fm;

    obj6 = *(u8**)(arg1 + 0x5C);
    if (obj6 != NULL && (*(s32*)(*(u8**)(obj6 + 0x4) + 0x1C) & 0x40)) {
        f2 = lbl_805FD304;
        f0 = lbl_805FD318 + *(f32*)(arg0 + 0x6A0);
        r9c = *(u8**)(obj6 + 0x9C);
        *(s32*)(arg0 + 0x14C) = *(s32*)(r9c + 0x60);
        if (f0 < *(f32*)(arg0 + 0x28)) {
            f2 = lbl_805FD374;
        }
        r8 = *(u8**)(arg0 + 0x88);
        if (r8 != NULL) {
            fm = f2 * *(f32*)(r8 + 0x48);
            (*(FnStep57548*)(*(u8**)obj6 + 0x50))(obj6, arg0, *(s32*)(r8 + 0x4C),
                                                   (s32)*(s16*)(r8 + 0x54), 0, fm);
        }
    }
    return 1;
}

// 0x80075FB0
void fn_80075FB0(u8* arg0, f32 farg0) {
    fn_800773CC(*(u8**)(arg0 + 0x6CC), farg0);
    fn_8007617C(arg0);
    fn_80070FAC(arg0, lbl_805FD660 * *(f32*)(arg0 + 0x6BC));
    if (fn_80077A90(*(u8**)(arg0 + 0x6CC)) != 0) {
        GameFlowSignal(-0x39F737BE, 0, 0);
    }
    if (*(f32*)(arg0 + 0x6BC) <= lbl_805FD650) {
        GameFlowSignal(0x117741C3, 0, 0);
        fn_80077AEC(*(u8**)(arg0 + 0x6CC));
    }
}

// 0x800C53B0
void fn_800C53B0(u8* arg0, f32 farg0) {
    f32 m;
    f32 c;
    f32 t;
    f32 r;

    m = fn_800C3E84();
    c = lbl_805FE3E0;
    t = *(f32*)(arg0 + 0xD8) + farg0;
    if (m > c) {
        if (t < c) {
            r = c;
        } else if (t > m) {
            r = m;
        } else {
            r = t;
        }
    } else {
        if (t < m) {
            r = m;
        } else if (t > c) {
            r = c;
        } else {
            r = t;
        }
    }
    *(f32*)(arg0 + 0xD8) = r;
}

// 0x800DF570
s32 fn_800DF570(u8* arg0, u8* arg1, u8* arg2) {
    f32 vec[3];
    f32 dx;
    f32 dy;
    f32 dz;

    lbl_805FB1A1 = 0;
    *(f32*)(arg1 + 0x0) = *(f32*)(arg0 + 0xC);
    *(f32*)(arg1 + 0x4) = *(f32*)(arg0 + 0x10);
    *(f32*)(arg1 + 0x8) = *(f32*)(arg0 + 0x14);
    fn_801F5E50((u8*)vec, lbl_805FE820);
    dx = vec[0] - *(f32*)(arg1 + 0x0);
    dy = vec[1] - *(f32*)(arg1 + 0x4);
    dz = vec[2] - *(f32*)(arg1 + 0x8);
    *(f32*)(arg2 + 0x0) = dx;
    *(f32*)(arg2 + 0x4) = dy;
    *(f32*)(arg2 + 0x8) = dz;
    return 1;
}

// 0x8000C380
void fn_8000C380(void) {
    u8* obj;
    u8* p;
    u8* v5;
    u8* v4;

    obj = lbl_803C2A00 + 0x20;
    *(f32*)(lbl_803C4258 + 0x0) = lbl_805FC630;
    *(f32*)(lbl_803C4258 + 0x4) = lbl_805FC660;
    *(f32*)(lbl_803C4258 + 0x8) = lbl_805FC630;
    fn_80121494(obj + 0x10, lbl_803C4258, lbl_805FC630);
    __register_global_object(obj, (void (*)(void*))fn_8000C41C, lbl_803C2A00);
    p = lbl_803C2A00 + 0x1680;
    v5 = p + 0xC0;
    v4 = p + 0x120;
    do {
        v5 += 0x10;
    } while (v5 < v4);
    do {
        v4 += 0xC;
    } while (v4 < lbl_803C2A00 + 0x1680 + 0x180);
}

// 0x80030AAC
void fn_80030AAC(u8* arg0) {
    s32 idx;
    u8* obj;
    ScriptValue tex;

    idx = *(s32*)(arg0 + 0x24) + 1;
    *(s32*)(arg0 + 0x24) = idx;
    if (idx >= 7) {
        if (*(s32*)(arg0 + 0x38) != 0) {
            *(s32*)(arg0 + 0x24) = 0;
        } else {
            obj = *(u8**)(arg0 + 0x4);
            *(f32*)(obj + 0x70) = lbl_805FCCC8;
            (*(FnVoidPP*)(*(u8**)arg0 + 0x14))(arg0, obj);
            return;
        }
    }
    tex = UITextureList_GetTexture(0x77A5B362U, *(s32*)(arg0 + 0x24));
    if (tex != 0) {
        obj = *(u8**)(arg0 + 0x4);
        UIImage_SetTexture(*(u32*)obj, (BOOL)tex);
    }
}

// 0x8003A1CC
void fn_8003A1CC(u8* arg0) {
    u8* w;

    fn_8003A2B4(arg0);
    fn_80021ECC();
    fn_800223F0(arg0, lbl_805FCFC4);
    UIElement_SetVisibleFlag(0x0CAF9C6FU, 1U);
    *(void**)(arg0 + 0x9AC) = ScreenSetFindElement(ScreenSetsCurrent(gScreenSets), 0xB7F7A96FU);
    *(void**)(arg0 + 0x9B0) = ScreenSetFindElement(ScreenSetsCurrent(gScreenSets), 0xCA7FD799U);
    w = *(u8**)(arg0 + 0x9AC);
    *(f32*)(arg0 + 0x9A0) = *(f32*)(w + 0x68);
    *(f32*)(arg0 + 0x9A4) = *(f32*)(w + 0x6C);
    *(f32*)(arg0 + 0x9A8) = *(f32*)(w + 0x70);
}

// 0x80052054
s8* fn_80052054(void) {
    s32 idx;
    u8* tbl;
    u8* e;
    s32 v0;
    s32 v1;
    s32 v2;
    s32 v3;
    s32 c;

    memset(lbl_803C53E8, 0, 0x80U);
    idx = fn_8022E438(*(u8**)lbl_805FBBE0);
    if (idx != -1) {
        tbl = *(u8**)(*(u8**)(gGameDb + 0xC) + 0x4F00);
        e = tbl + idx * 0x10;
        v0 = *(s32*)(e + 0x0);
        v1 = *(s32*)(e + 0x4);
        v2 = *(s32*)(e + 0x8);
        v3 = *(s32*)(e + 0xC);
        c = (s8)(v2 & 0xFF);
        SysSprintf((char*)lbl_803C53E8, (const char*)lbl_805F874C, c, v0, v1, v2, v3);
    }
    return (s8*)lbl_803C53E8;
}

// 0x80094330
u8* fn_80094330(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(u8**)(arg0 + 0x4) = lbl_8035C208;
        fn_80094C0C(arg0, lbl_8035C208);
        __destroy_arr(arg0 + 0x384, (void (*)(void*, s32))fn_80097590, 0x20, 0x1B);
        fn_801F69B8(arg0 + 0x294, -1);
        fn_801F9598(arg0 + 0x1E0, -1);
        fn_8006FAF4(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

}  // extern "C"
