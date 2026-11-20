// Small functions from the miss211 batch: object constructors and resets (0x8002xxxx, 0x80032EDC),
// zone slot refresh (0x8004DD30), script-table registration (0x8009xxxx), 0x801Exxxx/0x801Fxxxx helpers.
#include "types.h"

extern "C++" s32 gZoneResourceSize;  // 0x805FB94C
extern u8 gWorldState[];             // 0x803E6D00
void UpdateAllZoneEntities(void);

extern "C" {
// Callees
void    fn_800293C8(u8* p);                                  // 0x800293C8
u8*     fn_80029660(u8* p, s32 flags);                       // 0x80029660
void    operator_delete(void* p);                            // 0x801D426C
void*   fn_801D40E0(s32 n);                                  // 0x801D40E0
void*   fn_801B373C(void* p);                                // 0x801B373C
void*   fn_8004FFB4(u32 id, u8* part);                       // 0x8004FFB4
void    fn_8004C718(u8* p, s32 idx, void* elem);             // 0x8004C718
void    fn_801F8238(u8* p, s32 a4);                          // 0x801F8238
void    fn_801F8F34(u8* p, f32 v);                           // 0x801F8F34
void    fn_801F8FA0(u8* p, f32 v);                           // 0x801F8FA0
void    fn_801F8628(u8* obj, u8* src, s32 n);                // 0x801F8628
void    fn_801F900C(u8* p, const u8* src);                   // 0x801F900C
void    fn_801EE1F8(u8* p, s32 a);                           // 0x801EE1F8
void    fn_801EFA24(void);                                   // 0x801EFA24
void    fn_801EFAA4(void);                                   // 0x801EFAA4
void    fn_801EFB44(void);                                   // 0x801EFB44
void    fn_801EFB48(void);                                   // 0x801EFB48
void    fn_801EFBB8(void);                                   // 0x801EFBB8
void    fn_801EFC28(void);                                   // 0x801EFC28
u64     fn_801425DC(u8* timer);                              // 0x801425DC
void    fn_8027D270(s32 a, f32 b, f32 c);                    // 0x8027D270
u8*     fn_801B3308(void);                                   // 0x801B3308
void    fn_801B3310(u8* p, u8* q);                           // 0x801B3310
s32     RaySceneTest(void* ray);                             // 0x80070458
void    UIElement_PlayUIAnim(u32 element, u32 anim);
void    TextureRelease(void* tex);
s32     AnimSetFindClip(void* set, s32 anim);
u8*     AnimSetGetClip(void* set, s32 index);
void    BufferFree(void* p);

// Globals
extern u8   lbl_80386288[];     // 0x80386288 (vtable)
extern s32  lbl_80349000[];     // 0x80349000 (4-word block)
extern u8   lbl_80357680[];     // 0x80357680 (vtable)
extern u8   lbl_803576D0[];     // 0x803576D0 (vtable)
extern f32  lbl_806001C8;       // 0x806001C8 (float constant)
extern f32  lbl_805FCB00;       // 0x805FCB00 (float constant)
extern f32  lbl_805FCB30;       // 0x805FCB30 (float constant)
extern f32  lbl_805FCD88;       // 0x805FCD88 (float constant)
extern f32  lbl_805FCD8C;       // 0x805FCD8C (float constant)
extern f32  lbl_805FDB08;       // 0x805FDB08 (float constant)
extern f32  lbl_805FDD18;       // 0x805FDD18 (float constant)
extern f32  lbl_805FDD1C;       // 0x805FDD1C (float constant)
extern f32  lbl_805FDD20;       // 0x805FDD20 (float constant)
extern f32  lbl_805FDD24;       // 0x805FDD24 (float constant)
extern f32  lbl_803462F8[];     // 0x803462F8 (f32 table)
extern u8   lbl_803C4298[];     // 0x803C4298 (level info block)
extern s32  lbl_803C51D8[];     // 0x803C51D8
extern u8*  lbl_805FB078[];     // 0x805FB078 (pointer table)
extern u8*  lbl_805FB080[];     // 0x805FB080 (pointer table)
extern u8*  lbl_805FB088;       // 0x805FB088
extern u8*  lbl_803C6A58[3];    // 0x803C6A58 (door table)
extern u8*  lbl_803C6A70[3];    // 0x803C6A70 (robot table)
extern u8*  lbl_803C6A88[3];    // 0x803C6A88 (smasher table)
extern u8   lbl_804DD0F0[];     // 0x804DD0F0
extern u8   lbl_805FB768[];     // 0x805FB768 (static object)
extern s32  lbl_805FB928;       // 0x805FB928
extern u32  lbl_805FB92C;       // 0x805FB92C
extern u8   lbl_805FB930;       // 0x805FB930

// 0x801F8110
u8* fn_801F8110(u8* p, s32 a4) {
    u8 flags[12] = {1, 0, 0, 0xFF, 0, 0, 0, 1, 0, 0, 0xFF, 0};
    s32 blk[4];

    fn_801F8238(p, a4);
    *(void**)p = lbl_80386288;
    for (s32 i = 0; i < 4; i++) {
        blk[i] = lbl_80349000[i];
    }
    fn_801F8F34(p, lbl_806001C8);
    fn_801F8FA0(p, lbl_806001C8);
    fn_801F8628(p, flags, 4);
    fn_801F900C(p, (const u8*)blk);
    return p;
}

// 0x8002AB00
u8* fn_8002AB00(u8* p) {
    fn_800293C8(p);
    *(void**)p = lbl_80357680;
    *(f32*)(p + 0x4) = lbl_805FCB00;
    *(s32*)(p + 0xB4) = 0;
    return p;
}

// 0x8002AB4C
u8* fn_8002AB4C(u8* p, s32 del) {
    if (p != NULL) {
        fn_80029660(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x8002B4D0
u8* fn_8002B4D0(u8* p) {
    fn_800293C8(p);
    *(void**)p = lbl_803576D0;
    *(f32*)(p + 0x4) = lbl_805FCB30;
    return p;
}

// 0x8002B514
u8* fn_8002B514(u8* p, s32 del) {
    if (p != NULL) {
        fn_80029660(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x80032EDC
u8* fn_80032EDC(u8* p) {
    *(s32*)(p + 0x0) = 0;
    *(s32*)(p + 0x4) = 0;
    *(s32*)(p + 0x8) = 7;
    *(s32*)(p + 0xC) = 3;
    *(s32*)(p + 0x10) = 1;
    *(s32*)(p + 0x14) = 0;
    *(s32*)(p + 0x18) = 0;
    *(s32*)(p + 0x1C) = 0;
    *(s32*)(p + 0x20) = 0;
    *(f32*)(p + 0x24) = lbl_805FCD88;
    *(f32*)(p + 0x28) = lbl_805FCD88;
    *(f32*)(p + 0x30) = lbl_805FCD88;
    *(f32*)(p + 0x34) = lbl_805FCD88;
    *(f32*)(p + 0x38) = lbl_805FCD88;
    *(s32*)(p + 0x54) = 0;
    *(s32*)(p + 0x58) = 0;
    *(u8*)(p + 0x5C) = 0;
    *(u8*)(p + 0x5D) = 0;
    *(f32*)(p + 0x3C) = lbl_805FCD88;
    *(f32*)(p + 0x40) = lbl_805FCD88;
    *(f32*)(p + 0x44) = lbl_805FCD88;
    *(f32*)(p + 0x48) = lbl_805FCD88;
    *(f32*)(p + 0x4C) = lbl_805FCD88;
    *(f32*)(p + 0x50) = lbl_805FCD88;
    *(f32*)(p + 0x2C) = lbl_805FCD8C;
    return p;
}

// 0x80042C38
// Clears the table globals and creates the object stored at +0x260.
u8* fn_80042C38(u8* p) {
    void* obj;

    *(void**)(p + 0x260) = NULL;
    for (s32 i = 0; i < 6; i++) {
        lbl_803C51D8[i] = 0;
    }
    lbl_805FB078[0] = NULL;
    lbl_805FB080[0] = NULL;
    lbl_805FB078[1] = NULL;
    lbl_805FB080[1] = NULL;
    lbl_805FB088 = NULL;
    obj = fn_801D40E0(0x70);
    if (obj != NULL) {
        obj = fn_801B373C(obj);
    }
    *(void**)(p + 0x260) = obj;
    return p;
}

// 0x8004DD30
void fn_8004DD30(u8* arg0) {
    for (s32 i = 0; i < 5; i++) {
        u8* slot = arg0 + i * 4;
        u32 zone = *(u32*)(slot + 0xC6C);
        if (zone != 0) {
            if (zone < 0x3E8) {
                u8* world = *(u8**)gWorldState;
                u8* table = *(u8**)(world + 0x1A4);
                *(u32*)(slot + 0xC6C) = *(u32*)(table + (zone - 1) * gZoneResourceSize + 0x10C);
            }
            void* elem = fn_8004FFB4(*(u32*)(slot + 0xC6C), *(u8**)(slot + 0x89C));
            if (elem != NULL) {
                fn_8004C718(arg0, i, elem);
            }
        }
    }
    UpdateAllZoneEntities();
}

// 0x80091E28
// Registers arg0 in the door table at idx and initialises its fields.
void fn_80091E28(u8* arg0, s32 arg1, u8* arg2) {
    lbl_803C6A58[arg1] = arg0;
    *(void**)(arg0 + 0x0) = arg2;
    *(s32*)(arg0 + 0x8) = arg1;
}

// 0x8009231C
// Registers arg0 in the robot table at idx, plays the UI anim and releases its texture.
void fn_8009231C(u8* arg0, s32 arg1, u8* arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6) {
    lbl_803C6A70[arg1] = arg0;
    *(s32*)(arg0 + 0x0) = arg1;
    *(void**)(arg0 + 0x4) = arg2;
    *(s32*)(arg0 + 0x8) = 0;
    if (*(void**)(arg0 + 0xC) != NULL) {
        UIElement_PlayUIAnim(*(u32*)(arg0 + 0x10), 0xB729E3E6u);
        u8* tex = *(u8**)(arg0 + 0xC);
        s32 refs = *(s32*)(tex + 0x24) - 1;
        *(s32*)(tex + 0x24) = refs;
        if (refs == 0) {
            TextureRelease(tex);
        }
        *(void**)(arg0 + 0xC) = NULL;
    }
    *(s32*)(arg0 + 0x10) = arg3;
    *(s32*)(arg0 + 0x18) = arg4;
    *(s32*)(arg0 + 0x1C) = arg5;
    *(s32*)(arg0 + 0x20) = arg6;
    *(u8*)(arg0 + 0x34) = 0;
    *(f32*)(arg0 + 0x38) = lbl_805FDB08;
}

// 0x800939EC
// Registers arg0 in the smasher table at idx and looks up the clip length of the model in arg2.
void fn_800939EC(u8* arg0, s32 arg1, u8* arg2, s32 arg3, s32 arg4) {
    lbl_803C6A88[arg1] = arg0;
    *(s32*)(arg0 + 0x0) = arg1;
    *(void**)(arg0 + 0x4) = arg2;
    *(s32*)(arg0 + 0x8) = 0;
    *(s32*)(arg0 + 0xC) = arg3;
    *(s32*)(arg0 + 0x10) = arg4;
    *(u8*)(arg0 + 0x24) = 0;
    u8* model = *(u8**)(arg2 + 0x9C);
    if (model != NULL) {
        u8* set = *(u8**)(model + 0x48);
        if (set != NULL) {
            s32 clip = AnimSetFindClip(set, 0x51E28E95);  // hash, unresolved
            if (clip >= 0) {
                *(s32*)(arg0 + 0x20) = *(s32*)(AnimSetGetClip(*(void**)(model + 0x48), clip) + 0x28);
            }
        }
    }
}

// 0x8009C414
// Stores the float parameters for the object at p and points its two sub-objects at the
// level info block.
void fn_8009C414(u8* p, u8* arg1) {
    fn_8027D270(*(s32*)(lbl_803C4298 + 0x38), lbl_805FDD18, lbl_805FDD1C);
    *(void**)(p + 0x264) = arg1;
    fn_801B3310(fn_801B3308(), p + 0x268);
    *(s32*)(p + 0x3BC) = 1;
    *(f32*)(p + 0x3B8) = lbl_805FDD20;
    fn_801B3310(fn_801B3308(), p + 0x324);
    u8* ray = *(u8**)(p + 0x4);
    *(f32*)(p + 0x3D8) = *(f32*)(ray + 0xC);
    *(f32*)(p + 0x3DC) = *(f32*)(ray + 0x10);
    *(f32*)(p + 0x3E0) = *(f32*)(ray + 0x14);
    *(f32*)(p + 0x3E4) = *(f32*)(ray + 0x18);
    f32 hit = lbl_803462F8[RaySceneTest(ray)];
    *(f32*)(p + 0x3EC) = lbl_805FDD24;
    *(f32*)(p + 0x3E8) = hit;
    *(u8*)(p + 0x3F0) = 0;
    *(f32*)(p + 0x3F4) = lbl_805FDD1C;
    *(s32*)(p + 0x3F8) = 0;
    *(s32*)(p + 0x3FC) = 0;
    *(s32*)(p + 0x400) = 0;
}

// 0x801ED624
// Decodes a packed 4-byte record at q into the 4-byte record at p (type 3 only).
void fn_801ED624(u8* p, u8* q) {
    *(u8*)(p + 0x0) = 0xFF;
    *(u8*)(p + 0x1) = 0;
    *(u16*)(p + 0x2) = 0;
    u8 b = *(u8*)q;
    u32 kind = 1;
    if (((b >> 3) & 1) == 0) {
        kind = b & 0xF;
    }
    if (kind != 3) {
        return;
    }
    *(u8*)(p + 0x0) = (u8)*(u16*)(q + 0x2);
    *(u8*)(p + 0x1) = *(u8*)(q + 0x1);
    u8 b2 = *(u8*)q;
    u32 nib = (b2 >> 4) & 0xF;
    u32 sign = (b2 >> 7) & 1;
    *(u16*)(p + 0x2) = (u16)((nib & ~(0u - sign)) & 0xFF);
}

// 0x801ED684
// Returns the world-state entry addressed by the record at p, or NULL when out of range.
u8* fn_801ED684(u8* p) {
    u8 b = *(u8*)p;
    u32 kind;
    if (((b >> 7) & 1) == 0) {
        kind = b & 0xF;
    } else {
        kind = 1;
    }
    u32 idx;
    if (kind == 4) {
        idx = *(u16*)(p + 0x2);
    } else {
        idx = 0xFFFFFFFFu;
    }
    u8* ws = *(u8**)gWorldState;
    if (idx < (u32)*(s32*)(ws + 0x24)) {
        return *(u8**)(ws + 0x28) + (idx << 5);
    }
    return NULL;
}

// 0x801EE2C4
// Frees the buffer at +0x4 (if owned) and clears the record.
void fn_801EE2C4(u8* arg0) {
    if (*(u8*)arg0 != 0) {
        u8* buf = *(u8**)(arg0 + 0x4);
        if (buf != NULL && *(u8*)(arg0 + 0x14) != 0) {
            BufferFree(buf);
        }
    }
    *(void**)(arg0 + 0x4) = NULL;
    *(u8*)(arg0 + 0x14) = 0;
    *(s32*)(arg0 + 0x10) = 0;
    *(u8*)(arg0 + 0x0) = 0;
    *(s32*)(arg0 + 0x8) = 0;
    *(s32*)(arg0 + 0xC) = 0;
}

// 0x801EF980
// Initialises the sub-object at lbl_804DD0F0+0x10 with arg0 and fills the two callback tables.
void fn_801EF980(s32 arg0) {
    u8* base = lbl_804DD0F0;
    fn_801EE1F8(base + 0x10, arg0);
    *(void**)(base + 0x28) = (void*)fn_801EFA24;
    *(void**)(base + 0x2C) = (void*)fn_801EFB44;
    *(void**)(base + 0x30) = (void*)fn_801EFB48;
    *(void**)(base + 0x34) = (void*)fn_801EFBB8;
    *(void**)(base + 0x38) = (void*)fn_801EFC28;
    *(void**)(base + 0x3C) = (void*)fn_801EFAA4;
    *(void**)(base + 0x40) = (void*)fn_801EFB44;
    *(void**)(base + 0x44) = (void*)fn_801EFB48;
    *(void**)(base + 0x48) = (void*)fn_801EFBB8;
    *(void**)(base + 0x4C) = (void*)fn_801EFC28;
}

// 0x801F052C
void fn_801F052C(void) {
    u64 ret = fn_801425DC(lbl_805FB768);
    lbl_805FB928 = (s32)(u32)(ret >> 32);
    lbl_805FB92C = (u32)ret;
    lbl_805FB930 = 0;
}

}  // extern "C"
