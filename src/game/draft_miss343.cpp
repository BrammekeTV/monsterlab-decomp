#include "types.h"
#include "revolution.h"

extern "C" {
void*  MemAllocDefault(u32 size);                       // 0x801D4650
void*  MemAllocSmall(u32 size);                         // 0x801D41CC
void   MemFreeSmall(void* p);                           // 0x801D4340
void   BufferFree(void* p);                             // 0x801D4980
void   operator_delete(void* p);                        // 0x801D426C
void*  memset(void* p, int c, unsigned long n);
void*  fn_801D40E0(s32 n);                              // 0x801D40E0
void   fn_80142878(u8* cb);                             // 0x80142878
void   fn_80142898(u8* cb);                             // 0x80142898
void   fn_801B327C(void);                               // 0x801B327C
void   fn_801D7F18(u8* cam);                            // 0x801D7F18
void   fn_801EB748(void* p);                            // 0x801EB748
u8*    fn_801EB7A4(void* p, s32 a4, s32 a5, s32 a6, s32 a7);  // 0x801EB7A4
void   fn_801EBC74(u8* a, void* fn, u8* entry);         // 0x801EBC74
s32    fn_801EBBE0(u8* a, void* fn, u8* entry);         // 0x801EBBE0
void   fn_801CE944(u8* p);                              // 0x801CE944
f32    fn_801A9114(s32 n);                              // 0x801A9114
void fn_801A9A94(f32* arg0, u8* arg1, f32* arg2);   // 0x801A9A94
void   fn_80184604(s32 h);                              // 0x80184604
void   fn_80184AB0(s32 h, u8* a, u8* b, f32 c);         // 0x80184AB0
void   fn_80185A64(s32 h, s32 b);                       // 0x80185A64
u8*    fn_8015ECA0(u8* a);                              // 0x8015ECA0
u8*    fn_80183FA0(u8* a);                              // 0x80183FA0
f32    sinf(f32 x);                                     // MSL
f32    sqrtf(f32 x);                                    // MSL

extern u8  lbl_80478470[];
extern void* lbl_805FB6A0;
extern void* lbl_805FB700;
extern void* lbl_805FB704;
extern s32 lbl_805FB708;
extern u8* lbl_805FB8D8;
extern u8  lbl_805FB50C;
extern void* lbl_805FB5F8;
extern u8  lbl_805F9212;
extern u8  lbl_8047C640[];
extern u8  lbl_8040D500[];
extern u8  lbl_80412B20[];
extern u8  lbl_80559D78[];
extern u8  gScriptFuncs[];
extern f32 lbl_805FFCB0;
extern f32 lbl_805FFCB4;
extern f32 lbl_805FFD54;
extern f32 lbl_805FFDF0;
extern f32 lbl_805FFE00;
extern f32 lbl_80600020;
extern f32 lbl_80600334;
extern f32 lbl_8060034C;
}

// Frees a slot list: zeroes its pointer array (count at +0, array at +4), then frees array and object.
static void FreeSlotList(u8* list) {
    s32 n = *(s32*)(list + 0x0);
    for (s32 i = 0; i < n; i++) {
        *(s32*)(*(u8**)(list + 0x4) + i * 4) = 0;
    }
    *(s32*)(list + 0x0) = 0;
    MemFreeSmall(*(u8**)(list + 0x4));
    operator_delete(list);
}

// Resets the 0xFF-entry record table at base to its default state and records the table as current.
static void InitRecordTable(u8* base) {
    *(s32*)(base + 0x837C) = 0xFE;
    *(s32*)(base + 0x8380) = 1;
    for (s32 i = 0; i < 0xFF; i++) {
        u8* rec = base + i * 0x80;
        memset(rec, 0, 0x64);
        *(s32*)(rec + 0x64) = 0;
        *(s32*)(rec + 0x68) = 0;
        *(s32*)(rec + 0x6C) = 0;
        *(f32*)(rec + 0x70) = lbl_80600020;
        *(u16*)(rec + 0x78) = 0xFFFF;
        *(u16*)(rec + 0x7A) = 0xFFFF;
        *(u16*)(rec + 0x7C) = 0xFFFF;
        *(s32*)(rec + 0x74) = 0;
        *(u8*)(rec + 0x7E) = 0;
        *(u8*)(rec + 0x7F) = 0;
        *(s32*)(base + 0x7F80 + i * 4) = 0xFE - i;
    }
}

extern "C" {

// 0x801B313C
void fn_801B313C(void) {
    u8* list = (u8*)lbl_805FB6A0;
    if (list != NULL) {
        FreeSlotList(list);
    }
    list = (u8*)fn_801D40E0(8);
    if (list != NULL) {
        *(s32*)(list + 0x0) = 0;
        *(void**)(list + 0x4) = MemAllocSmall(0x80);
    }
    lbl_805FB6A0 = list;
    fn_80142878((u8*)fn_801B327C);
}

// 0x801B31F0
void fn_801B31F0(void) {
    fn_80142898((u8*)fn_801B327C);
    u8* list = (u8*)lbl_805FB6A0;
    if (list != NULL) {
        FreeSlotList(list);
        lbl_805FB6A0 = NULL;
    }
}

// 0x801BC870
void fn_801BC870(void) {
    memset(lbl_80478470, 0, 0x100);
}

// 0x801BD97C
void fn_801BD97C(u8* arg0) {
    memset(arg0 + 0xC, 0, 0xC);
    if (*(u8**)(arg0 + 0x4) != NULL) {
        fn_801EB748(*(u8**)(arg0 + 0x4));
        BufferFree(*(u8**)(arg0 + 0x4));
        *(u8**)(arg0 + 0x4) = NULL;
    }
    if (*(u8**)(arg0 + 0x4) == NULL) {
        u8* p = (u8*)MemAllocDefault(0x2C);
        *(u8**)(arg0 + 0x4) = p;
        if (p != NULL) {
            fn_801EB7A4(p, 0x200, 1, 0x80, 0);
        }
    }
}

// Looks up key in the sorted table (flag at +0x20 set) or scans it linearly; returns the entry or NULL.
// 0x801C3DB8
u8* fn_801C3DB8(u8* arg0, u32 arg1) {
    u8* base = *(u8**)(arg0 + 0x1C);
    if (*(u8*)(arg0 + 0x20) != 0) {
        s32 lo = -1;
        s32 hi = *(s32*)(arg0 + 0x18);
        while (hi - lo > 1) {
            s32 sum = hi + lo;
            s32 mid = (sum + (s32)((u32)sum >> 31)) >> 1;
            if (arg1 <= *(u32*)(base + (mid << 7))) {
                hi = mid;
            } else {
                lo = mid;
            }
        }
        if (arg1 == *(u32*)(base + (hi << 7))) {
            return base + (hi << 7);
        }
        return NULL;
    }
    s32 count = *(s32*)(arg0 + 0x18);
    for (s32 i = 0; i < count; i++) {
        if (arg1 == *(u32*)(base + (i << 7))) {
            return base + (i << 7);
        }
    }
    return NULL;
}

// 0x801C42A8
void fn_801C42A8(u8* arg0) {
    if (!(*(s32*)(arg0 + 0x4) & 1)) {
        memset(arg0 + 0xC, 0, 0x300);
        memset(arg0 + 0x30C, 0, 0x80);
        *(s32*)(arg0 + 0x8) = 0;
        *(s32*)(arg0 + 0x4) = *(s32*)(arg0 + 0x4) | 0x80;
    }
    *(f32*)(arg0 + 0x390) = lbl_805FFCB0;
    *(s32*)(arg0 + 0x4) = (*(s32*)(arg0 + 0x4) | 1) & ~2;
    *(f32*)(arg0 + 0x394) = lbl_805FFCB4;
    *(s32*)(arg0 + 0x39C) = 0;
}

// 0x801C64E8
void fn_801C64E8(void) {
    BufferFree(lbl_805FB700);
    lbl_805FB700 = NULL;
    for (s32 i = 0; i < lbl_805FB708; i++) {
        BufferFree(*(u8**)((u8*)lbl_805FB704 + i * 0x10 + 0x8));
    }
    BufferFree(lbl_805FB704);
    lbl_805FB704 = NULL;
}

// 0x801C767C
void fn_801C767C(u8* arg0) {
    memset(arg0, 0, 0xC);
    memset(arg0 + 0xC, 0, 0xC);
    s32 off = 0;
    for (s32 i = 0; i < *(s32*)arg0; i++) {
        fn_801CE944(*(u8**)(arg0 + 0x4) + off);
        off += 0x1F4;
    }
}

// 0x801C908C
void fn_801C908C(u8* arg0, u8* arg1, f32 farg0) {
    *(f32*)(arg1 + 0x0) = *(f32*)(arg0 + 0x30) - *(f32*)(arg0 + 0x20);
    *(f32*)(arg1 + 0x4) = *(f32*)(arg0 + 0x34) - *(f32*)(arg0 + 0x24);
    *(f32*)(arg1 + 0x8) = *(f32*)(arg0 + 0x38) - *(f32*)(arg0 + 0x28);
    *(f32*)(arg1 + 0xC) = *(f32*)(arg0 + 0x3C) - *(f32*)(arg0 + 0x2C);
    f32 s = lbl_805FFD54 / (farg0 * (lbl_805FFD54 - *(f32*)(arg0 + 0x0)));
    *(f32*)(arg1 + 0x0) = *(f32*)(arg1 + 0x0) * s;
    *(f32*)(arg1 + 0x4) = *(f32*)(arg1 + 0x4) * s;
    *(f32*)(arg1 + 0x8) = *(f32*)(arg1 + 0x8) * s;
    *(f32*)(arg1 + 0xC) = *(f32*)(arg1 + 0xC) * s;
}

// 0x801CE330
void fn_801CE330(u8* arg0, u8* arg1) {
    s32 i = 0;
    for (u8* slot = arg0; i < *(s32*)(arg0 + 0xC); slot += 4, i++) {
        u8* dst = *(u8**)(slot + 0x14);
        if (dst != NULL) {
            *(f32*)(dst + 0x10) = *(f32*)(arg1 + 0x0);
            *(f32*)(dst + 0x14) = *(f32*)(arg1 + 0x4);
            *(f32*)(dst + 0x18) = *(f32*)(arg1 + 0x8);
            *(f32*)(dst + 0x1C) = *(f32*)(arg1 + 0xC);
        }
    }
}

// 0x801CE3B0
void fn_801CE3B0(u8* arg0, s32 arg1) {
    *(s32*)(arg0 + 0x98) = *(s32*)(arg0 + 0x98) | (s32)(1u << arg1);
}

// 0x801CE3D0
void fn_801CE3D0(u8* arg0, u8* arg1) {
    *(f32*)(arg0 + 0x0) = *(f32*)(arg0 + 0x0) * *(f32*)(arg1 + 0x0);
    *(f32*)(arg0 + 0x4) = *(f32*)(arg0 + 0x4) * *(f32*)(arg1 + 0x4);
    *(f32*)(arg0 + 0x8) = *(f32*)(arg0 + 0x8) * *(f32*)(arg1 + 0x8);
    *(f32*)(arg0 + 0xC) = *(f32*)(arg0 + 0xC) * *(f32*)(arg1 + 0xC);
}

// 0x801CE424
f32 fn_801CE424(u8* arg0) {
    s32 t = *(s32*)(arg0 + 0x74);
    if (t != 0) {
        f32 scale = fn_801A9114(t);
        return *(f32*)(*(u8**)(arg0 + 0x70) + 0x14) * scale;
    }
    return *(f32*)(*(u8**)(arg0 + 0x70) + 0x14);
}

// 0x801CE4A4
void fn_801CE4A4(u8* arg0, u8* arg1) {
    f32 out[4];
    f32* src;
    if (*(s32*)(arg1 + 0x74) != 0) {
        u8* obj = *(u8**)(arg1 + 0x5C);
        f32 in[4];
        in[0] = *(f32*)(obj + 0xC);
        in[1] = *(f32*)(obj + 0x10);
        in[2] = *(f32*)(obj + 0x14);
        in[3] = *(f32*)(obj + 0x18);
        fn_801A9A94((f32*)out, *(u8**)(arg1 + 0x74), in);
        src = out;
    } else {
        src = (f32*)(*(u8**)(arg1 + 0x5C) + 0xC);
    }
    *(f32*)(arg0 + 0x0) = src[0];
    *(f32*)(arg0 + 0x4) = src[1];
    *(f32*)(arg0 + 0x8) = src[2];
    *(f32*)(arg0 + 0xC) = src[3];
}

// 0x801CE6D8
void fn_801CE6D8(f32* arg0, f32* arg1) {
    *arg0 = sinf(lbl_805FFE00 * *arg1);
}

// 0x801CE7A0
void fn_801CE7A0(f32* arg0, f32* arg1) {
    f32 x = *arg1;
    f32 r = lbl_805FFDF0;
    if (x > lbl_805FFDF0) {
        r = sqrtf(x);
    }
    *arg0 = r;
}

// 0x801DAFEC
void fn_801DAFEC(u8* arg0, f32* arg1, f32* arg2, u8* arg3) {
    if (*(s32*)(arg0 + 0x4C) != 0) {
        u8* p = fn_80183FA0(lbl_80412B20);
        if (p != NULL) {
            *(f32*)(p + 0xC) = arg2[0];
            *(f32*)(p + 0x10) = arg2[1];
            *(f32*)(p + 0x14) = arg2[2];
            *(f32*)(p + 0x18) = arg2[3];
            *(f32*)(p + 0x0) = arg1[0];
            f32 f1 = arg1[1];
            *(f32*)(p + 0x4) = f1;
            *(f32*)(p + 0x8) = arg1[2];
            fn_80184AB0(*(s32*)(arg0 + 0x4C), arg3, p, f1);
        }
    }
}

// 0x801DB0A0
void fn_801DB0A0(u8* arg0, s32 arg1, s32 arg2) {
    if (*(s32*)(arg0 + 0x4C) != 0 && lbl_805FB5F8 != NULL) {
        if (arg2 != 0) {
            fn_801D7F18(lbl_8047C640);
        }
        lbl_805FB50C |= 8;
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = 0xFE00C000u;
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = (u32)lbl_805F9212 << 14;
        if (arg1 != 0) {
            fn_80184604(*(s32*)(arg0 + 0x4C));
        }
        fn_80185A64(*(s32*)(arg0 + 0x4C), *fn_8015ECA0(lbl_8040D500));
    }
}

// 0x801DB248
void fn_801DB248(u8* arg0) {
    u8* arr = *(u8**)(arg0 + 0x0);
    u32 count = *(u32*)(arg0 + 0x4);
    for (u32 i = 0; i < count; i++) {
        u8* e = arr + i * 0x2C;
        s32 idx = *(s32*)(e + 0x10);
        if (idx != -1) {
            void* fn = *(void**)(gScriptFuncs + idx * 0xC + 0x4);
            fn_801EBC74(*(u8**)(e + 0x4), fn, e);
            fn_801EBBE0(*(u8**)(e + 0x4), fn, e);
        }
    }
}

// 0x801E74AC
void fn_801E74AC(u8* arg0) {
    InitRecordTable(arg0);
    lbl_805FB8D8 = arg0;
}

// 0x801E7574
void fn_801E7574(u8* arg0) {
    InitRecordTable(arg0);
    lbl_805FB8D8 = arg0;
}

// One step of a small state machine: state word at +0x8 selects a swap of the values at +0x64, +0x70, +0x108, +0x10C.
// 0x802019C4
void fn_802019C4(u8* arg0) {
    switch (*(s32*)(arg0 + 0x8)) {
    case 1: {
        *(s32*)(arg0 + 0x10C) = *(s32*)(arg0 + 0x70);
        s32 r = (s32)(lbl_8060034C * lbl_80600334);
        s32 v;
        if (r < -0x8000) {
            v = -0x8000;
        } else if (r <= 0x7FFF) {
            v = r;
        } else {
            v = 0x7FFF;
        }
        *(s32*)(arg0 + 0x70) = (s32)((u32)v << 16);
        *(s32*)(arg0 + 0x8) = 2;
        return;
    }
    case 2:
        *(s32*)(arg0 + 0x108) = *(s32*)(arg0 + 0x64);
        *(s32*)(arg0 + 0x64) = (s32)0xD1200000u;
        *(s32*)(arg0 + 0x8) = 3;
        return;
    case 4:
        *(s32*)(arg0 + 0x8) = 5;
        *(s32*)(arg0 + 0x64) = *(s32*)(arg0 + 0x108);
        return;
    case 5:
        *(s32*)(arg0 + 0x8) = 0;
        *(s32*)(arg0 + 0x70) = *(s32*)(arg0 + 0x10C);
        return;
    default:
        return;
    }
}

// Comparator over two indices into the 0x28-byte voice records at lbl_80559D78: orders by the
// record's flag, then by the float at +0x18; returns -1, 0 or 1.
// 0x8020B548
s32 fn_8020B548(u8* pa, u8* pb) {
    u8* recA = lbl_80559D78 + *(s32*)pa * 0x28;
    u8* recB = lbl_80559D78 + *(s32*)pb * 0x28;
    s32 flagA = *(s32*)(recA + 0x0);
    if (flagA != 0 && *(s32*)(recB + 0x0) == 0) {
        return -1;
    }
    if (flagA == 0 && *(s32*)(recB + 0x0) != 0) {
        return 1;
    }
    f32 fa = *(f32*)(recA + 0x18);
    f32 fb = *(f32*)(recB + 0x18);
    if (fa > fb) {
        return -1;
    }
    if (fa < fb) {
        return 1;
    }
    if (recA == recB) {
        return 1;
    }
    return 0;
}

// Copies five words from arg3 into the entry at arg0 + arg1 * 0x3C + arg2 * 0x14 (+0x5B8..+0x5C8);
// indices outside 0..3 (arg1) and 0..2 (arg2) are ignored.
// 0x80222744
void fn_80222744(u8* arg0, s32 arg1, s32 arg2, u8* arg3) {
    if (arg1 <= -1 || arg1 >= 4 || arg2 < 0 || arg2 >= 3) {
        return;
    }
    u8* dst = arg0 + arg1 * 0x3C + arg2 * 0x14;
    *(s32*)(dst + 0x5B8) = *(s32*)(arg3 + 0x0);
    *(s32*)(dst + 0x5BC) = *(s32*)(arg3 + 0x4);
    *(s32*)(dst + 0x5C0) = *(s32*)(arg3 + 0x8);
    *(s32*)(dst + 0x5C4) = *(s32*)(arg3 + 0xC);
    *(s32*)(dst + 0x5C8) = *(s32*)(arg3 + 0x10);
}

}  // extern "C"
