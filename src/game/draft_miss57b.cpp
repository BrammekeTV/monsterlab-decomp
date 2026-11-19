// Misc. game helpers (shovel/puzzle/welding state, object setup, zone/effect utilities).
// Verified against the asm at 0x80023840-0x800E4FC4 (see each function).

#include "types.h"

// Global defined in game_flow.cpp, declared with its defining type.
extern "C++" s32 gZoneResourceSize;   // 0x805FB94C

extern "C" {

// Callees (addresses as in symbols.txt; signatures from the defining file where one exists).
void BufferFree(void* p);                                  // 0x801D4980
void operator_delete(void* p);                             // 0x801D426C
u8* fn_80021750(void);                                     // 0x80021750
s32 fn_80024EB0(u8* arg0);                                     // 0x80024EB0
void fn_8019CAD4(void* a, void* b);                        // 0x8019CAD4
void UIElement_SetVisibleFlag(u32 hash, BOOL on);
void UIWidgetPlayAnimLoop(void* w, u32 anim, s32 n);       // 0x8019BB1C
u32 StringHash(const char* s);                             // 0x801EB704
s32 RaySceneTest(void* ray);                               // 0x80070458
u32 RandomU32(void);                                       // 0x80143460
s32 WorldGetPhase(void* world);                            // 0x800D83A8
BOOL ZoneIsLoaded(void* world, s32 zone);                  // 0x8012F408
BOOL fn_800E6BD4(void);
void fn_800DE5B4(u8* p);                                   // 0x800DE5B4
s32 fn_8007B318(u8* a3);                                   // 0x8007B318
u8 fn_80076B9C(u8* a3);                                    // 0x80076B9C
void fn_800D6E90(u8* p, f32 farg0);                        // 0x800D6E90
void fn_800D8D00(u8* obj, void* a1, s32 a2);               // 0x800D8D00
void HudShowConversation(void* hud, s32 conversation);     // 0x8001B0B8

// Globals (addresses as in symbols.txt).
extern u8 gGame[];
extern u8 gWorldState[];
extern void* lbl_805FAFB8;                                 // global object pointer
extern u8* lbl_805FAFD8;                                   // global object pointer
extern u8* lbl_805FB188;                                   // global object pointer
extern void* lbl_805FBBE0;                                 // save block pointer
extern u8 lbl_803C4298[];                                  // level info block
extern u8 lbl_8035FC48[];                                  // vtable (address only)
extern u8 lbl_8035FDF8[];                                  // vtable (address only)
extern u8 lbl_805F8C88[];                                  // word compared by fn_800CD744
extern u8 lbl_80357368[];                                  // string
extern f32 lbl_80347370[];                                 // float table
extern f32 lbl_80347360[];                                 // float table
extern f32 lbl_80347290[];                                 // float table
extern f32 lbl_805FC9C0;
extern f32 lbl_805FCBB0;
extern f32 lbl_805FE400;
extern f32 lbl_805FE404;
extern f64 lbl_805FE408;
extern f32 lbl_805FE440;
extern f32 lbl_805FE444;
extern f32 lbl_805FE7B0;
extern f32 lbl_805FEA50;
extern f32 lbl_805FEA54;

// 0x80023840
s32 ShovelDirection(void) {

    return fn_80024EB0(fn_80021750());  // r3 (the pointer) passes straight through
}

// 0x80026658
void ShowCompletePuzzle(void) {
    u8* obj = (u8*)lbl_805FAFB8;
    u8* arr;
    u8* elem;
    s32 i;
    u32 off;

    fn_8019CAD4(*(void**)(obj + 0xC8), *(void**)(obj + 0xC0));
    *(f32*)(*(u8**)(obj + 0xC8) + 0x70) = lbl_805FC9C0;
    off = 0;
    for (i = 0; i < 8; i++) {
        arr = *(u8**)((u8*)lbl_805FAFB8 + 0xB0);
        elem = *(u8**)(arr + off);
        UIElement_SetVisibleFlag(*(u32*)elem, 0);
        off += 0x48;
    }
    UIElement_SetVisibleFlag(*(u32*)(*(u8**)(obj + 0xC0)), 1);
    UIElement_SetVisibleFlag(*(u32*)(*(u8**)(obj + 0xC8)), 1);
    UIWidgetPlayAnimLoop(*(void**)(obj + 0xC8), StringHash((const char*)lbl_80357368), 0xA);
}

// 0x8002DDC0
s32 StoppedSpinning(void) {
    u8* obj = lbl_805FAFD8;
    f32 v;

    if (obj == NULL) {
        return 0;
    }
    v = *(f32*)(obj + 0xAC);
    if (v < 0.0f) {
        v = -v;
    }
    if (v > lbl_805FCBB0) {
        *(s32*)(obj + 0x90) = 1;
        return 0;
    }
    if (*(s32*)(obj + 0x90) != 0) {
        *(s32*)(obj + 0x90) = 0;
        return 1;
    }
    *(s32*)(obj + 0x90) = 0;
    return 0;
}

// 0x80077BC8
s32 WeldingOverPlate(u8* arg0) {
    return fn_8007B318(*(u8**)(arg0 + 0x10)) == 0;
}

// 0x80075964
u8* fn_80075964(u8* arg0, s32 arg1) {
    u8* base = arg0 + arg1 * 0x50;
    u8* p = base;
    s32 i;

    for (i = 0; i < *(s32*)(arg0 + arg1 * 4 + 0x570); i++) {
        if (fn_80076B9C(*(u8**)(p + 0x3E0)) == 0) {
            return *(u8**)(base + i * 4 + 0x3E0);
        }
        p += 4;
    }
    return NULL;
}

// 0x8007C8CC
void fn_8007C8CC(f32* arg0, u8* arg1, f32* arg2) {
    u8* a = *(u8**)(arg1 + 0x8);
    u8* b = *(u8**)a;
    s32 n = *(s32*)(a + 0x24);
    u8* e = b + (n - 1) * 0x28;
    f32 x = arg2[0] - *(f32*)(b + 0xC);
    f32 y = arg2[1] - *(f32*)(b + 0x10);
    f32 z = arg2[2] - *(f32*)(b + 0x14);

    arg0[0] = *(f32*)(e + 0xC) + x;
    arg0[1] = *(f32*)(e + 0x10) + y;
    arg0[2] = *(f32*)(e + 0x14) + z;
}

// 0x800C5FC8
void fn_800C5FC8(u8* arg0) {
    s32 idx1;
    s32 idx2;
    s32 idx3;
    u32 rnd;
    f32 hi;
    f32 lo;
    f32 s;
    f32 t;
    f32 v;
    f32 tab;
    union {
        u64 u;
        f64 d;
    } cvt;

    *(u32*)(arg0 + 0x15C) = *(u32*)(arg0 + 0x15C) | 0x600000;
    idx1 = RaySceneTest(arg0);
    hi = lbl_80347370[idx1];
    idx2 = RaySceneTest((void*)(uintptr_t)idx1);
    lo = lbl_80347360[idx2];
    rnd = RandomU32();
    cvt.u = 0x4330000000000000ULL | (u64)rnd;
    t = (f32)(cvt.d - lbl_805FE408);
    s = lbl_805FE404 * t;
    *(f32*)(arg0 + 0x264) = lo + (hi - lo) * s;

    idx3 = RaySceneTest((void*)(uintptr_t)rnd);
    tab = lbl_80347290[idx3];
    v = *(f32*)(arg0 + 0x264);
    if (!(v > tab)) {
        v = tab;
    }
    *(f32*)(arg0 + 0x264) = v;
    *(f32*)(arg0 + 0x26C) = lbl_805FE400;
    *(f32*)(arg0 + 0x270) = lbl_805FE400;
    *(f32*)(arg0 + 0x274) = lbl_805FE400;
}

// 0x800C6CE4
void fn_800C6CE4(u8* arg0, u8* arg1, u8* arg2) {
    u8* t;

    *(u8**)(arg0 + 0x0) = arg1;
    *(u8**)(arg0 + 0x4) = arg2;
    *(s32*)(arg0 + 0x8) = 0;
    *(s32*)(arg0 + 0xC) = 2;
    *(s32*)(arg0 + 0x10) = 1;

    t = *(u8**)(arg2 + 0x4);
    *(f32*)(t + 0x0) = *(f32*)(arg2 + 0xC8) + lbl_805FE440;
    *(f32*)(t + 0x4) = *(f32*)(arg2 + 0xCC) + lbl_805FE444;
    *(f32*)(t + 0x8) = *(f32*)(arg2 + 0xD0) + lbl_805FE440;

    *(u32*)(t + 0x20) = *(u32*)(t + 0x20) | 0x8000;
    *(u32*)(t + 0x20) = *(u32*)(t + 0x20) | 0x10000000;
    *(u32*)(t + 0x20) = *(u32*)(t + 0x20) | 0x4000;
    *(u32*)(t + 0x20) = *(u32*)(t + 0x20) | 0x10000000;
}

// 0x800C6D98
void fn_800C6D98(u8* arg0, u8* arg1, u8* arg2) {
    u8* t;

    *(u8**)(arg0 + 0x0) = arg1;
    *(u8**)(arg0 + 0x4) = arg2;
    *(s32*)(arg0 + 0x8) = 0;
    *(s32*)(arg0 + 0xC) = 1;
    *(s32*)(arg0 + 0x10) = 1;

    t = *(u8**)(arg2 + 0x4);
    *(f32*)(t + 0x0) = *(f32*)(arg2 + 0xC8) + lbl_805FE440;
    *(f32*)(t + 0x4) = *(f32*)(arg2 + 0xCC) + lbl_805FE444;
    *(f32*)(t + 0x8) = *(f32*)(arg2 + 0xD0) + lbl_805FE440;

    *(u32*)(t + 0x20) = *(u32*)(t + 0x20) | 0x8000;
    *(u32*)(t + 0x20) = *(u32*)(t + 0x20) | 0x10000000;
    *(u32*)(t + 0x20) = *(u32*)(t + 0x20) | 0x4000;
    *(u32*)(t + 0x20) = *(u32*)(t + 0x20) | 0x10000000;
}

// 0x800C6E48
void fn_800C6E48(u8* arg0, u8* arg1, u8* arg2) {
    u8* t;

    *(u8**)(arg0 + 0x0) = arg1;
    *(u8**)(arg0 + 0x4) = arg2;
    *(s32*)(arg0 + 0x8) = 0;
    *(s32*)(arg0 + 0xC) = 3;
    *(s32*)(arg0 + 0x10) = 1;

    t = *(u8**)(arg2 + 0x4);
    *(f32*)(t + 0x0) = *(f32*)(arg2 + 0xC8) + lbl_805FE440;
    *(f32*)(t + 0x4) = *(f32*)(arg2 + 0xCC) + lbl_805FE444;
    *(f32*)(t + 0x8) = *(f32*)(arg2 + 0xD0) + lbl_805FE440;

    *(u32*)(t + 0x20) = *(u32*)(t + 0x20) | 0x8000;
    *(u32*)(t + 0x20) = *(u32*)(t + 0x20) | 0x10000000;
    *(u32*)(t + 0x20) = *(u32*)(t + 0x20) | 0x4000;
    *(u32*)(t + 0x20) = *(u32*)(t + 0x20) | 0x10000000;
}

// 0x800CD744
s32 fn_800CD744(u8* arg0) {
    u8* temp = *(u8**)(arg0 + 0x9C);
    u32 v = 0;

    if (temp != NULL) {
        v = *(u32*)(temp + 0x8);
    }
    return (*(u32*)lbl_805F8C88 == v) ? 1 : 0;
}

// 0x800D5008
void fn_800D5008(u8* arg0, u8* arg1) {
    u8* src = arg1;
    s32 i;

    *(s32*)(arg0 + 0xD8) = 0;
    for (i = 0; i < *(s32*)(arg1 + 0xD8); i++) {
        s32 n = *(s32*)(arg0 + 0xD8);
        if (n < 0xC) {
            u8* dst = arg0 + n * 0x10;
            *(f32*)(dst + 0x18) = *(f32*)(src + 0x18);
            *(f32*)(dst + 0x1C) = *(f32*)(src + 0x1C);
            *(f32*)(dst + 0x20) = *(f32*)(src + 0x20);
            *(u8*)(dst + 0x24) = *(u8*)(src + 0x24);
            *(s32*)(arg0 + 0xD8) = n + 1;
        }
        src += 0x10;
    }
    *(s32*)(arg0 + 0x0) = *(s32*)(arg1 + 0x0);
    *(s32*)(arg0 + 0x4) = *(s32*)(arg1 + 0x4);
    *(s32*)(arg0 + 0x8) = *(s32*)(arg1 + 0x8);
    *(s32*)(arg0 + 0xC) = *(s32*)(arg1 + 0xC);
    *(s32*)(arg0 + 0x10) = *(s32*)(arg1 + 0x10);
    *(s32*)(arg0 + 0x14) = *(s32*)(arg1 + 0x14);
}

// 0x800D6A24
void fn_800D6A24(u8** arg0) {
    u8* s = *arg0;
    s32 st = *(s32*)(s + 0xC);
    u8 flag = *(u8*)(s + 0x4);

    if (st == *(s32*)(*(u8**)(s + 0x10) + 0xD8) - 1 && flag == 1) {
        *(s32*)(s + 0xC) = -4;
        return;
    }
    if (st == 0 && flag == 0) {
        *(s32*)(s + 0xC) = -3;
        return;
    }
    if (st == -4) {
        *(s32*)(s + 0xC) = -2;
        *(s32*)(s + 0x14) = *(s32*)(*(u8**)(s + 0x10) + 0x8);
        return;
    }
    if (st == -3) {
        *(s32*)(s + 0xC) = -1;
        *(s32*)(s + 0x14) = *(s32*)(*(u8**)(s + 0x10) + 0x4);
        return;
    }
    if (flag != 0) {
        *(s32*)(s + 0xC) = st + 1;
    } else {
        *(s32*)(s + 0xC) = st - 1;
    }
}

// 0x800D6DDC
void fn_800D6DDC(void) {
    if (lbl_805FB188 != NULL) {
        operator_delete(lbl_805FB188);
    }
    lbl_805FB188 = NULL;
}

// 0x800D6E14
void fn_800D6E14(f32 farg0) {
    if (lbl_805FB188 != NULL && WorldGetPhase(gGame + 0xD5F0) == 2 &&
        *(s32*)(lbl_803C4298 + 0x18) == 5 && fn_800E6BD4() == 0) {
        fn_800D6E90(lbl_805FB188, farg0);
    }
}

// 0x800D816C
void fn_800D816C(u8* arg0, s32 arg1) {
    u8* hud = *(u8**)(arg0 + 0x44);

    if (*(s32*)(hud + 0x42C) == 0 && *(s32*)((u8*)lbl_805FBBE0 + 0xA1C) == 0 &&
        *(s32*)(arg0 + 0x28) != 4) {
        HudShowConversation(hud, arg1);
        return;
    }
    *(s32*)(arg0 + 0x38) = arg1;
}

// 0x800D85CC
u8* fn_800D85CC(u8* arg0, s32 arg1) {
    if (arg1 != 0) {
        *(s32*)(arg0 + 0x4) = 0;
        *(s32*)(arg0 + 0x8) = 0;
        *(s32*)(arg0 + 0xC) = 0;
        *(s32*)(arg0 + 0x10) = 0;
        *(s32*)(arg0 + 0x14) = 0;
        *(s32*)(arg0 + 0x18) = 0;
        *(s32*)(arg0 + 0x1C) = 0;
        *(s32*)(arg0 + 0x20) = 0;
    }
    *(s32*)(arg0 + 0x24) = 0;
    return arg0;
}

// 0x800D8A40
void fn_800D8A40(u8* arg0, s32 arg1) {
    u8* temp;

    if (arg0 != NULL) {
        temp = *(u8**)(arg0 + 0x8);
        *(void**)(arg0 + 0x0) = (void*)lbl_8035FC48;
        if (temp != NULL) {
            BufferFree(temp);
            *(void**)(arg0 + 0x8) = NULL;
        }
        temp = *(u8**)(arg0 + 0x4);
        if (temp != NULL) {
            BufferFree(temp);
            *(void**)(arg0 + 0x4) = NULL;
        }
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
}

// 0x800D8FC4
s32 fn_800D8FC4(u8* arg0, void* arg1, s32 arg2) {
    u8* temp = *(u8**)(arg0 + 0x8);
    s32 (*vfn)(u8*);

    if (temp != NULL) {
        BufferFree(temp);
        *(void**)(arg0 + 0x8) = NULL;
    }
    fn_800D8D00(arg0, arg1, arg2);
    vfn = *(s32 (**)(u8*))(*(u8**)arg0 + 0x94);
    return vfn(arg0);
}

// 0x800DEC28
void fn_800DEC28(u8* arg0, void* arg1) {
    *(void**)(arg0 + 0x0) = (void*)lbl_8035FDF8;
    fn_800DE5B4(arg0 + 0xC);
    *(f32*)(arg0 + 0x50) = lbl_805FE7B0;
    *(f32*)(arg0 + 0x34) = lbl_805FE7B0;
    *(f32*)(arg0 + 0x3C) = lbl_805FE7B0;
}

// 0x800E4F84
void fn_800E4F84(u8* arg0) {
    u8* world;
    s32 zi;
    s32 k;
    s32 off;
    u8* zone;
    u8* e;

    *(void**)(arg0 + 0x78) = NULL;
    for (zi = 0; zi <= *(s32*)(*(u8**)gWorldState + 0x4); zi++) {
        if (ZoneIsLoaded(gWorldState, zi) != 0) {
            world = *(u8**)gWorldState;
            zone = *(u8**)(world + 0x1A4) + zi * gZoneResourceSize;
            off = 0;
            for (k = 0; k < *(u8*)(zone + 0xAC); k++) {
                e = *(u8**)(*(u8**)(zone + 0xA8) + off);
                if (e != arg0 && *(u8*)(e + 0x54) != 0) {
                    *(void**)(arg0 + 0x78) = e;
                    *(f32*)(e + 0x44) = lbl_805FEA50;
                    *(f32*)(arg0 + 0x44) = lbl_805FEA54;
                }
                off += 4;
            }
        }
    }
}

}  // extern "C"
