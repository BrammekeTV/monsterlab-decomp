// Helpers and small world/zone/record accessors (0x80007C54 .. 0x80083C80). Verified against the asm.
#include "types.h"

struct ScriptFunc { u32 type; void* fn; u32 hash; };   // 0xC bytes (as in script_dispatch.cpp)

extern ScriptFunc gScriptFuncs[];
extern u8 gWorldState[];          // first word is a pointer to the world object (zone table at +0x1A4)
extern "C++" s32 gZoneResourceSize;
extern u8 lbl_805FB140[];         // first word is a pointer to the global block (+0x758, +0x75C, +0x8C, ...)
extern u8 lbl_8040D500[];         // record used by the transform helpers

extern "C" {
void* ObjAlloc(u32 size);
BOOL ZoneIsLoaded(void* world, s32 zone);
s32 RaySceneTest(void* ray);
void fn_801EBA20(void* list, void* item);
s32 fn_8018BDE0(u8* p, s32 key);
u8* fn_8018A3EC(u8* p);
void* fn_80070400(u8* obj);
s32 fn_801B5BD8(u8* self, s32 a, s32 b, u8* c);
void fn_801B468C(u8* self, s32 arg1);
void fn_80122A10(u8* p, u32 handle);
void fn_801B7410(void* self, void* p4, void* p5, s32 p6, f32* pos);
s32 fn_8007BEE4(u8* a, u8** list, s32 n, u8* b);
s32 fn_8007919C(void);
s32 fn_8008EFE0(void);
s32 fn_80024BDC(void);
s32 fn_80025CE0(void);
s32 fn_8002917C(void);
s32 fn_8002D400(void);
s32 fn_8002E370(void);
s32 fn_80038A8C(void);
void fn_80031F1C(void);
extern u8* lbl_803C4000[16];      // per-slot record pointers
extern s32 lbl_803C4040[16];      // per-slot "in use" flags
extern s32 lbl_803C4E28[];        // per-slot flags set by fn_8003DB60
extern f32 lbl_803474D8[];        // per-index float table (ray hit)
extern f32 lbl_803474E8[];        // per-index float table (ray hit)
}

extern "C" {

// 0x801C668C
u8* fn_801C668C(u8* arg0) {
    s32 loaded = 0;
    if (arg0[0] != 0xFF && ZoneIsLoaded(gWorldState, (s32)arg0[1]) != 0) {
        loaded = 1;
    }
    if (loaded != 0) {
        u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + (s32)arg0[1] * gZoneResourceSize;
        return *(u8**)(zone + 0xBC) + (s32)arg0[0] * 0x3C;
    }
    return NULL;
}

// 0x801B5338
s32 fn_801B5338(u8* arg0, u8* arg1, u32 arg2, f32* arg3, u8 arg4) {
    // Slot 0x10 of the object's vtable, called on arg0 (result is the key's owner handle).
    if ((*(u8* (**)(u8*))(*(u8**)arg0 + 0x10))(arg0) == NULL) {
        return 0;
    }
    if (fn_8018BDE0((*(u8* (**)(u8*))(*(u8**)arg0 + 0x10))(arg0), (s32)arg2) < 0) {
        return 0;
    }
    u8* obj = (u8*)ObjAlloc(0x2C);
    if (obj != NULL) {
        s32 idx = fn_8018BDE0((*(u8* (**)(u8*))(*(u8**)arg0 + 0x10))(arg0), (s32)arg2);
        fn_801B7410(obj, arg0, arg1, idx, arg3);
        *(u8*)(obj + 0x1C) = arg4;
        return 1;
    }
    return 0;
}

// 0x801DB398
void fn_801DB398(u8* arg0, u32 arg1, u8* arg2) {
    u32 count = *(u32*)(arg0 + 0x4);
    u8* table = *(u8**)arg0;
    u8* hit = NULL;
    for (u32 i = 0; i < count; i++) {
        if (*(u32*)(table + i * 0x2C) == arg1) {
            hit = table + i * 0x2C;
            break;
        }
    }
    if (hit != NULL && *(u8**)(hit + 0x4) != NULL) {
        s32 script = *(s32*)(hit + 0x14);
        if (script != -1) {
            ScriptFunc* e = &gScriptFuncs[script];
            ((void (*)(u8*, u8*, ScriptFunc*))e->fn)(arg2, hit, e);
        }
        fn_801EBA20(*(u8**)(hit + 0x4), arg2);
    }
}

// 0x80007C54
u8* fn_80007C54(void) {
    for (s32 i = 0; i < 16; i++) {
        if (lbl_803C4040[i] != 0) {
            if (lbl_803C4000[i] != NULL) {
                fn_8018A3EC(lbl_803C4000[i]);
            }
            return lbl_803C4000[i];
        }
    }
    return NULL;
}

// 0x80021770
s32 fn_80021770(void) {
    fn_80024BDC();
    fn_80025CE0();
    fn_8002917C();
    fn_80031F1C();
    fn_80038A8C();
    fn_8002D400();
    return fn_8002E370();
}

// arg0 is never read (r3 is overwritten before use); arg1 is the slot index.
// 0x8003DB60
void fn_8003DB60(u8* arg0, s32 arg1) {
    lbl_803C4E28[arg1] = 1;
}

// 0x8004EDA0
s32 fn_8004EDA0(u8* arg0, u32 arg1) {
    u8* node = *(u8**)(arg0 + 0xB0);
    while (node != NULL) {
        if (*(u32*)(node + 0x4) == arg1) {
            return 1;
        }
        node = *(u8**)(node + 0x8);
    }
    return 0;
}

// Clears the 0x42-byte per-object record at p (used by 80062DC4 and 80062EA8).
static void ClearRecord(u8* p) {
    for (s32 off = 0; off <= 0x3C; off += 4) {
        *(s32*)(p + off) = 0;
    }
    *(u8*)(p + 0x40) = 0xFF;
    *(u8*)(p + 0x41) = 0;
}

// 0x80062DC4
void fn_80062DC4(u8* arg0, s32 arg1, s32 arg2) {
    u8* rec = (*(u8* (**)(u8*))(*(u8**)arg0 + 0xB8))(arg0);
    if (rec != NULL) {
        ClearRecord(rec);
    }
    *(u32*)(arg0 + 0x14C) = 0xDC1BFEC5u;
    u8* owner = *(u8**)(arg0 + 0x4);
    *(u8*)(arg0 + 0x150) = 1;
    *(s32*)(arg0 + 0x154) = -1;
    *(s32*)(owner + 0x20) |= 0x200000;
    fn_801B5BD8(arg0, arg1, arg2, owner);
}

// 0x80062EA8
s32 fn_80062EA8(u8* arg0, s32 arg1) {
    u8* rec = (*(u8* (**)(u8*))(*(u8**)arg0 + 0xB8))(arg0);
    if (rec != NULL) {
        ClearRecord(rec);
        fn_80122A10(rec, (u32)(uintptr_t)arg0);  // lint:ok 32-bit slot handle compare; revisit with the x64 layout
    }
    fn_801B468C(arg0, arg1);
    return 0;
}

// 0x8006FBB8
s32 fn_8006FBB8(void) {
    fn_8007919C();
    return fn_8008EFE0();
}

// arg0 is never read by the asm; kept to match the prototype in small_8007b.cpp.
// 0x80079CBC
void fn_80079CBC(u8* arg0) {
    u8* blk = *(u8**)lbl_805FB140;
    s32 n = *(s32*)(blk + 0x8C);
    u8* sp[24];   // original frame holds about 21 words; n is small in practice
    u8** table = (u8**)(blk + 0x90);
    sp[0] = blk + 0x1D0;
    sp[n + 1] = blk + 0x210;
    for (s32 k = 0; k < n; k++) {
        sp[k + 1] = table[k];
    }
    fn_8007BEE4(*(u8**)(blk + 0x75C), sp, n + 2, *(u8**)(blk + 0x758));
}

// 0x8007A598
void fn_8007A598(u8* arg0) {
    *(f32*)(arg0 + 0x1C) = 80.0f;
    *(f32*)(arg0 + 0x20) = 80.0f;
    u8* tex = (u8*)fn_80070400(*(u8**)arg0);
    *(u8**)(arg0 + 0x4) = tex;
    f32 w = *(f32*)(arg0 + 0x1C);
    f32 h = *(f32*)(arg0 + 0x20);
    f32 halfW = 0.5f * w;
    f32 halfH = 0.5f * h;
    *(f32*)(arg0 + 0x28) = (f32)*(u16*)(tex + 0x4) / w;
    *(f32*)(arg0 + 0xC) = -halfW * 0.01f;
    *(f32*)(arg0 + 0x10) = -halfH * 0.01f;
    *(f32*)(arg0 + 0x14) = halfW * 0.01f;
    *(f32*)(arg0 + 0x18) = halfH * 0.01f;
    *(f32*)(arg0 + 0x24) = (f32)*(u16*)(tex + 0x6) / h;
}

// fn_8015EC9C is a bare blr (returns its first argument, the record table), so the record
// is taken directly. The float arguments of the original are never read.
// 0x8007C3B0
void fn_8007C3B0(u8* arg0) {
    u8* rec = lbl_8040D500;
    *(f32*)(rec + 0x60) = *(f32*)(arg0 + 0x358);
    *(f32*)(rec + 0x64) = *(f32*)(arg0 + 0x35C);
    *(f32*)(rec + 0x68) = *(f32*)(arg0 + 0x360);
    *(f32*)(rec + 0x6C) = *(f32*)(arg0 + 0x364);
    *(f32*)(rec + 0x70) = *(f32*)(arg0 + 0x368);
    *(f32*)(rec + 0x74) = *(f32*)(arg0 + 0x36C);
    *(f32*)(rec + 0x78) = *(f32*)(arg0 + 0x370);
    *(f32*)(rec + 0x7C) = *(f32*)(arg0 + 0x374);
    *(f32*)(rec + 0x80) = *(f32*)(arg0 + 0x378);
    *(f32*)(rec + 0x84) = *(f32*)(arg0 + 0x37C);
    *(f32*)(rec + 0x88) = *(f32*)(arg0 + 0x380);
    *(f32*)(rec + 0x8C) = *(f32*)(arg0 + 0x384);
    *(f32*)(rec + 0x24) = *(f32*)(arg0 + 0x388);
    *(f32*)(rec + 0x1C) = *(f32*)(arg0 + 0x38C);
}

// 0x8007CFAC
f32 fn_8007CFAC(u8* arg0) {
    return (f32)*(s32*)(arg0 + 0x10);
}

// 0x8007CFD8
f32 fn_8007CFD8(u8* arg0) {
    return (f32)*(s32*)(arg0 + 0xC);
}

// 0x8007D24C
void fn_8007D24C(u8* arg0) {
    *(s32*)(arg0 + 0x4) = 0;
    *(s32*)(arg0 + 0x8) = 0;
    *(s32*)(arg0 + 0xC) = 0;
    *(s32*)(arg0 + 0x10) = 0;
    *(s32*)(arg0 + 0x20) = 0;
    *(f32*)(arg0 + 0x14) = 50.0f;
    s32 idx = RaySceneTest(arg0);
    *(f32*)(arg0 + 0x18) = lbl_803474D8[idx];
    *(f32*)(arg0 + 0x1C) = lbl_803474E8[idx];
}

// 0x80083C80
void fn_80083C80(u8* arg0) {
    u8* world = *(u8**)gWorldState;
    s32 total = *(s32*)(*(u8**)(world + 0x1A4) + gZoneResourceSize + 0x94)
              + *(s32*)(*(u8**)(world + 0x1A4) + gZoneResourceSize + 0x98);
    for (s32 i = 0; i < total; i++) {
        u8* zone = *(u8**)(world + 0x1A4) + gZoneResourceSize;
        s32 countA = *(s32*)(zone + 0x94);
        u8* item;
        u8* ent;
        if (i < countA) {
            item = (*(u8***)(zone + 0x120))[i];
            ent = *(u8**)(zone + 0x8C) + i * 0x40;
        } else {
            item = (*(u8***)(zone + 0x12C))[i - countA];
            ent = *(u8**)(zone + 0x90) + (i - countA) * 0x40;
        }
        if ((*(u32*)(ent + 0x1C) & 0x40) && item != NULL) {
            u8* p9c = *(u8**)(item + 0x9C);
            u32 id = (p9c != NULL) ? *(u32*)(p9c + 0x8) : 0u;
            if ((u32)(id + 0x03CB0000u) == 0x0000E14Cu) {
                u8* pos = *(u8**)(item + 0x4);
                *(f32*)(arg0 + 0x274) = *(f32*)(pos + 0x0);
                *(f32*)(arg0 + 0x278) = *(f32*)(pos + 0x4);
                *(f32*)(arg0 + 0x27C) = 0.0f;
                return;
            }
        }
    }
}

}  // extern "C"
