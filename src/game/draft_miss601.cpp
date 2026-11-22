// Functions 0x80029B28-0x80079ADC (draft_miss601 batch), verified against the asm.
#include "types.h"

// Globals (defined in C++ files).
extern "C++" {
extern u8* gBattle;                 // 0x805FBBB8
extern void* gEventMgr;
extern void* gScreenSets;
extern s32 gSelectedPart;
extern u8 gWorldState[];            // 0x803E6D00 (first word points to the world block)
extern s32 gZoneResourceSize;
extern u8 lbl_805FB0BE;             // 0x805FB0BE
extern u8* lbl_805FB088;            // 0x805FB088
extern u8* lbl_805FB078[];          // 0x805FB078
extern u8* lbl_805FB080[];          // 0x805FB080
extern u8* lbl_803C51D8[];          // 0x803C51D8 (table of object pointers)
extern u8 lbl_803C4298[];           // 0x803C4298
extern u8 lbl_803589D8[];           // 0x803589D8 (screen element hash table)
}

extern "C" {
void* ScreenSetsCurrent(void* sets);
void* ScreenSetFindElement(void* set, u32 hash);
void ExplorationHudSetFighter(void* set, void* fighter, s32 a);
u8** FighterStatsGetPart(u8* stats, s32 slot);
BOOL fn_80212E28(u8** slot);
void BufferFree(void* p);
s32 RaySceneTest(void* ray);
u32 RandomU32(void);
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);
void StickRead(s32 channel, f32* x, f32* y);
void fn_8020A398(s32 handle);
void GameFlowSignal(s32 id, s32 arg1, s32 arg2);
u32 UIScreen_Is_Pushed(u32 hash);
void UIImage_SetTexture(u32 hash, BOOL tex);
ScriptValue UITextureList_GetTexture(u32 hash, s32 index);
s32 fn_8019CC7C(void* el, u32 flag);
void operator_delete(void* p);
s32 rand(void);
f32 cosf(f32 x);
f32 sinf(f32 x);
void* memset(void* p, int c, unsigned long n);

void fn_8002C080(u8* p, u8* a, u8* b);       // 0x8002C080
void fn_8002C444(f32* out, u8* p, f32 v);    // 0x8002C444
u8* fn_8002C614(u8* p);                     // 0x8002C614
u8* fn_801B37C4(u8* p, s32 del);            // 0x801B37C4
void fn_801B1668(void* p);                  // 0x801B1668
void fn_801B4520(u8* p);                    // 0x801B4520
void fn_801B55F4(u8* obj, u8* out, s32 idx); // 0x801B55F4
BOOL fn_801C6634(u8* col);                  // 0x801C6634
u8* fn_801C668C(u8* key);                   // 0x801C668C
s32 fn_801DA780(u8* a3);                    // 0x801DA780
u8* fn_801DA788(u8* a3, s32 i);             // 0x801DA788
void fn_80050164(u8* obj);                  // 0x80050164
u32 fn_8021F7E8(u8* p);                     // 0x8021F7E8
u32 fn_8021F7C0(u8* p);                     // 0x8021F7C0
void fn_8021F94C(u8* p, f32 v);             // 0x8021F94C
void fn_8014474C(u8* src, f32* out);        // 0x8014474C
u8* fn_800D8F70(u8* a, u8* out);            // 0x800D8F70
void fn_800D8CB0(u8* a, f32* o0, f32* o1, f32* o2, f32* o3); // 0x800D8CB0
f32 fn_800D8F90(u8* a);                     // 0x800D8F90
f32 fn_8007CF94(s32 h);                     // 0x8007CF94
s32 fn_8007CDBC(u8* p, s32 h, f32 t);       // 0x8007CDBC
void fn_8007C958(u8* obj, f32* pt);         // 0x8007C958
s32 fn_8007D2C4(s32 h, f32 t);              // 0x8007D2C4
s32 fn_8007D2C8(s32 h);                     // 0x8007D2C8
s32 fn_800771F8(u8* p);                     // 0x800771F8
s32 fn_80076928(u8* a, u8* b);              // 0x80076928
void fn_80037058(u8* obj, f32* to, f32* from); // 0x80037058

// float constants (asm labels; values not recovered)
extern f32 lbl_805FCAA8;   // 0x805FCAA8
extern f32 lbl_805FCBB0;   // 0x805FCBB0
extern f64 lbl_805FCC10;   // 0x805FCC10 (double, stick threshold low)
extern f64 lbl_805FCC18;   // 0x805FCC18 (double, stick threshold high)
extern f32 lbl_805FCD88;   // 0x805FCD88
extern f32 lbl_805FCD9C;   // 0x805FCD9C
extern f32 lbl_805FCDBC;   // 0x805FCDBC
extern f32 lbl_805FCEBC;   // 0x805FCEBC
extern f32 lbl_805FCED0;   // 0x805FCED0
extern f32 lbl_805FCEE0;   // 0x805FCEE0
extern f32 lbl_805FCEE4;   // 0x805FCEE4
extern f32 lbl_805FD3FC;   // 0x805FD3FC
extern f32 lbl_805FD650;   // 0x805FD650
extern f32 lbl_805FD664;   // 0x805FD664
extern f32 lbl_805FD670;   // 0x805FD670
extern f32 lbl_805FD674;   // 0x805FD674
extern f32 lbl_805FD68C;   // 0x805FD68C
extern f32 lbl_805FD6B8;   // 0x805FD6B8
extern f32 lbl_80347230[]; // 0x80347230
extern f32 lbl_80347240[]; // 0x80347240
extern f32 lbl_80347250[]; // 0x80347250
extern f32 lbl_80347260[]; // 0x80347260
extern f32 lbl_80347270[]; // 0x80347270
extern f32 lbl_80347670[]; // 0x80347670
}

// Helper: object's vtable slot 0x14 (returns an item count).
static s32 ObjCount14(u8* obj) {
    return (*(s32 (**)(void*))(*(u8**)obj + 0x14))(obj);
}

extern "C" {

// 0x80029B28
// Copies the position from arg1 into obj+0x48, then reseats the position of every sub-object.
void fn_80029B28(u8* obj, u8* arg1) {
    u8* p = fn_8002C614(obj + 0xB8);
    f32 x = *(f32*)(p + 0x0);
    f32 y = *(f32*)(p + 0x4);
    f32 z = *(f32*)(p + 0x8);
    (*(void (**)(void*, f32, f32))(*(u8**)obj + 0x18))(obj, y, x);
    *(f32*)(obj + 0x3C) = x;
    *(f32*)(obj + 0x40) = y;
    *(f32*)(obj + 0x44) = z;
    *(f32*)(obj + 0x48) = *(f32*)(arg1 + 0x0);
    *(f32*)(obj + 0x4C) = *(f32*)(arg1 + 0x4);
    *(f32*)(obj + 0x50) = *(f32*)(arg1 + 0x8);
    fn_8002C080(obj + 0xB8, obj + 0x3C, obj + 0x48);
    *(s32*)(obj + 0xB4) = 2;
    *(f32*)(obj + 0x2C) = *(f32*)(obj + 0x3C);
    *(f32*)(obj + 0x30) = *(f32*)(obj + 0x40);
    for (s32 i = 0; i < ObjCount14(obj); i++) {
        u8* sub = *(u8**)(obj + i * 4 + 0x104);
        *(f32*)(sub + 0x68) = *(f32*)(obj + 0x2C);
        *(f32*)(sub + 0x6C) = *(f32*)(obj + 0x30);
        (*(void (**)(void*, s32, f32, f32))(*(u8**)(sub + 0x138) + 0x2C))(sub, 0x18, *(f32*)(obj + 0x30), *(f32*)(sub + 0x70));
    }
    u8* t = *(u8**)(obj + 0x114);
    *(f32*)(t + 0x68) = *(f32*)(obj + 0x2C);
    *(f32*)(t + 0x6C) = *(f32*)(obj + 0x30);
    (*(void (**)(void*, s32, f32, f32))(*(u8**)(t + 0x138) + 0x2C))(t, 0x18, *(f32*)(obj + 0x30), *(f32*)(t + 0x70));
}

// 0x8002A180
// Refreshes the sub-object positions, advances the random offset at +0x8, and returns 1.
s32 fn_8002A180(u8* arg0) {
    f32 out[2];
    (*(void (**)(void*))(*(u8**)arg0 + 0x18))(arg0);
    fn_8002C444(out, arg0 + 0xB8, lbl_805FCAA8);
    *(f32*)(arg0 + 0x2C) = out[0];
    *(f32*)(arg0 + 0x30) = out[1];
    for (s32 i = 0; i < ObjCount14(arg0); i++) {
        u8* sub = *(u8**)(arg0 + i * 4 + 0x104);
        *(f32*)(sub + 0x68) = *(f32*)(arg0 + 0x2C);
        *(f32*)(sub + 0x6C) = *(f32*)(arg0 + 0x30);
        (*(void (**)(void*, s32, f32, f32))(*(u8**)(sub + 0x138) + 0x2C))(sub, 0x18, *(f32*)(arg0 + 0x30), *(f32*)(sub + 0x70));
    }
    u8* t = *(u8**)(arg0 + 0x114);
    *(f32*)(t + 0x68) = *(f32*)(arg0 + 0x2C);
    *(f32*)(t + 0x6C) = *(f32*)(arg0 + 0x30);
    (*(void (**)(void*, s32, f32, f32))(*(u8**)(t + 0x138) + 0x2C))(t, 0x18, *(f32*)(arg0 + 0x30), *(f32*)(t + 0x70));
    *(s32*)(arg0 + 0x18) = 1;
    s32 r = (s32)(RandomU32() & 3) + *(s32*)(arg0 + 0x8);
    *(s32*)(arg0 + 0x8) = r - 2;
    u8* t2 = *(u8**)(arg0 + 0x114);
    (*(void (**)(void*, s32, s32))(*(u8**)(t2 + 0x138) + 0x30))(t2, 2, r);
    for (s32 i = 0; i < ObjCount14(arg0); i++) {
        u8* sub = *(u8**)(arg0 + i * 4 + 0x104);
        (*(void (**)(void*, s32))(*(u8**)(sub + 0x138) + 0x2C))(sub, 0x18);
    }
    return 1;
}

// 0x8002DC04
// Reads the stick direction into the active slot index at +0xA8 and tells the slot's sub-object.
void fn_8002DC04(u8* arg0) {
    u8* e = arg0 + *(s32*)(arg0 + 0xA8) * 0x28;
    u8* sub = *(u8**)(e + 0x118);
    (*(void (**)(void*, s32))(*(u8**)(sub + 0x138) + 0x30))(sub, 2);
    f32 sx = lbl_805FCBB0;
    f32 sy = lbl_805FCBB0;
    StickRead((*(s32*)(lbl_803C4298 + 0x38) << 4) + 0x109D, &sx, &sy);
    if (sx < lbl_805FCC10) {
        *(s32*)(arg0 + 0xA8) = 3;
    } else if (sx > lbl_805FCC18) {
        *(s32*)(arg0 + 0xA8) = 2;
    } else if (sy < lbl_805FCC10) {
        *(s32*)(arg0 + 0xA8) = 1;
    } else if (sy > lbl_805FCC18) {
        *(s32*)(arg0 + 0xA8) = 0;
    } else {
        *(s32*)(arg0 + 0xA8) = 4;
    }
    u8* e2 = arg0 + *(s32*)(arg0 + 0xA8) * 0x28;
    if (*(u8*)(e2 + 0x111) != 0) {
        u8* s2 = *(u8**)(e2 + 0x118);
        (*(void (**)(void*, s32))(*(u8**)(s2 + 0x138) + 0x2C))(s2, 2);
    }
}

// 0x800336DC
// Sets up the two HUD element pairs from arg1/arg2 and re-sends their positions.
void fn_800336DC(u8* arg0, u8* arg1, u8* arg2) {
    *(u8**)(arg0 + 0x18) = arg1;
    *(s32*)(arg0 + 0x14) = 0;
    *(u8**)(arg0 + 0x1C) = arg2;
    *(void**)(arg0 + 0x58) = ScreenSetFindElement(ScreenSetsCurrent(gScreenSets), 0x0A23635FU);
    if (arg1 != NULL) {
        *(s32*)(arg0 + 0x8) = 0;
        *(f32*)(arg0 + 0x3C) = *(f32*)(arg1 + 0x74);
        *(f32*)(arg0 + 0x40) = *(f32*)(arg1 + 0x78);
        *(f32*)(arg0 + 0x44) = *(f32*)(arg1 + 0x7C);
        *(f32*)(arg0 + 0x48) = *(f32*)(arg1 + 0x68);
        *(f32*)(arg0 + 0x4C) = *(f32*)(arg1 + 0x6C);
        *(f32*)(arg0 + 0x50) = *(f32*)(arg1 + 0x70);
        *(f32*)(arg2 + 0x68) = *(f32*)(arg1 + 0x68);
        *(f32*)(arg2 + 0x6C) = *(f32*)(arg1 + 0x6C);
        *(f32*)(arg2 + 0x70) = *(f32*)(arg1 + 0x70);
        (*(void (**)(void*, s32))(*(u8**)(arg1 + 0x138) + 0x30))(arg1, 2);
        (*(void (**)(void*, s32))(*(u8**)(arg2 + 0x138) + 0x30))(arg2, 2);
        *(f32*)(arg1 + 0x68) = lbl_805FCD9C;
        *(f32*)(arg1 + 0x6C) = lbl_805FCD9C;
        *(f32*)(arg1 + 0x70) = lbl_805FCD88;
        (*(void (**)(void*, s32))(*(u8**)(arg1 + 0x138) + 0x2C))(arg1, 0x38);
        if (arg2 != NULL) {
            *(f32*)(arg2 + 0x68) = lbl_805FCD9C;
            *(f32*)(arg2 + 0x6C) = lbl_805FCD9C;
            *(f32*)(arg2 + 0x70) = lbl_805FCD88;
            (*(void (**)(void*, s32))(*(u8**)(arg2 + 0x138) + 0x2C))(arg2, 0x38);
        }
        (*(void (**)(void*, s32))(*(u8**)(arg1 + 0x138) + 0x2C))(arg1, 0x38);
        (*(void (**)(void*, s32))(*(u8**)(arg2 + 0x138) + 0x2C))(arg2, 0x38);
    }
}

// 0x80036618
// Ticks the timers at +0x640 and +0x258 and swaps textures for the state at +0x280.
void fn_80036618(u8* arg0, f32 farg0) {
    if (*(f32*)(arg0 + 0x640) > lbl_805FCDBC) {
        *(f32*)(arg0 + 0x640) = *(f32*)(arg0 + 0x640) - farg0;
    } else {
        ScriptValue tex = UITextureList_GetTexture(0x6C375453U, 0);
        u8* img = *(u8**)(arg0 + 0x59C);
        UIImage_SetTexture(*(u32*)img, tex);
        u8* o = *(u8**)(arg0 + 0x59C);
        (*(void (**)(void*, s32))(*(u8**)(o + 0x138) + 0x2C))(o, 0x1A);
    }
    for (s32 i = 0; i < 10; i++) {
        u8* p = arg0 + i * 4;
        if (*(f32*)(p + 0x258) > lbl_805FCDBC) {
            *(f32*)(p + 0x258) = *(f32*)(p + 0x258) - farg0;
        } else {
            s32 k = *(s32*)(p + 0x280);
            if (k != 0) {
                u8* sub = NULL;
                u32 hash = 0;
                if (k == 1) {
                    sub = *(u8**)(p + 0x198);
                    hash = 0xACCED4C1U;
                } else if (k == 2) {
                    sub = *(u8**)(p + 0x2A8);
                    hash = 0xD2D850FCU;
                } else if (k == 4) {
                    sub = *(u8**)(p + 0x368);
                    hash = 0x3095961FU;
                } else if (k == 8) {
                    sub = *(u8**)(p + 0x428);
                    hash = 0x2EFDE767U;
                }
                if (sub != NULL) {
                    UIImage_SetTexture(*(u32*)sub, UITextureList_GetTexture(hash, 0));
                    (*(void (**)(void*, s32))(*(u8**)(sub + 0x138) + 0x2C))(sub, 0x1A);
                    *(s32*)(p + 0x280) = 0;
                }
            }
        }
    }
}

// 0x80036EE0
// Projects the point arg1 into the camera frame at +0x810, fills the 3 vector slots at +0x804 and sets the flag at +0x814.
void fn_80036EE0(u8* arg0, u8* arg1) {
    u8* cam = *(u8**)(arg0 + 0x810);
    f32 dx = *(f32*)(arg1 + 0x0) - *(f32*)(cam + 0x68);
    f32 dy = *(f32*)(arg1 + 0x4) - *(f32*)(cam + 0x6C);
    f32 dz = *(f32*)(arg1 + 0x8) - lbl_805FCEBC;
    f32 c = cosf(lbl_805FCEE0);
    f32 s = sinf(lbl_805FCEE0);
    f32 ns = -s;
    f32 dxc = dx * c;
    f32 dys = dy * ns;
    f32 A = lbl_805FCED0 + (dy * c + dx * s);   // y component before halving
    f32 B = lbl_805FCED0 + (dxc + dys);          // x component before halving
    f32 pt[3];
    pt[0] = B * lbl_805FCEE4;
    pt[1] = A * lbl_805FCEE4;
    pt[2] = dz * lbl_805FCEE4;
    fn_80037058(arg0, (f32*)(arg0 + 0x804), pt);
    *(f32*)(arg0 + 0x804) = pt[0];
    *(f32*)(arg0 + 0x808) = pt[1];
    *(f32*)(arg0 + 0x80C) = pt[2];
    f32 ratio = *(f32*)(arg0 + 0x81C) / *(f32*)(arg0 + 0x818);
    *(s32*)(arg0 + 0x814) = (ratio > lbl_80347670[*(s32*)(arg0 + 0x820)]) ? 1 : 0;
}

// 0x80042CC0
// Releases the cached object slots, the table at +0x260 and optionally frees the object (del > 0).
void fn_80042CC0(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        for (s32 i = 0; i < 6; i++) {
            u8* p = lbl_803C51D8[i];
            if (p != NULL) {
                (*(void (**)(void*, s32))(*(u8**)p + 8))(p, 1);
            }
            lbl_803C51D8[i] = NULL;
        }
        for (s32 i = 0; i < 2; i++) {
            u8* p0 = lbl_805FB078[i];
            if (p0 != NULL) {
                (*(void (**)(void*, s32))(*(u8**)p0 + 8))(p0, 1);
            }
            lbl_805FB078[i] = NULL;
            u8* p1 = lbl_805FB080[i];
            if (p1 != NULL) {
                (*(void (**)(void*, s32))(*(u8**)p1 + 8))(p1, 1);
            }
            lbl_805FB080[i] = NULL;
            u8* p2 = lbl_805FB088;
            if (p2 != NULL) {
                (*(void (**)(void*, s32))(*(u8**)p2 + 8))(p2, 1);
            }
            lbl_805FB088 = NULL;
        }
        fn_801B37C4(*(u8**)(arg0 + 0x260), 1);
        *(void**)(arg0 + 0x260) = NULL;
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
}

// 0x8004784C
// Clears the fighter HUD elements of the current screen when that screen is the expected type.
void fn_8004784C(void) {
    void* screen = ScreenSetsCurrent(gScreenSets);
    if (screen == NULL) {
        return;
    }
    if (*(u32*)screen != 0x1B66AC94U) {
        return;
    }
    for (s32 i = 0; i < 5; i++) {
        u8* fset = *(u8**)(gBattle + 0xA00);
        u8** part = FighterStatsGetPart(fset + 0x20, i);
        if (fn_80212E28(part) != 0 || *(s32*)(fset + i * 4) == 0) {
            void* scr = ScreenSetsCurrent(gScreenSets);
            if (scr != NULL && *(u32*)scr == 0x1B66AC94U) {
                void* el = ScreenSetFindElement(scr, *(u32*)(lbl_803589D8 + i * 4));
                if (el != NULL) {
                    fn_8019CC7C(el, 0U);
                }
            }
        }
    }
    u8* e1 = (u8*)ScreenSetFindElement(screen, 0x382C6918U);
    if (e1 != NULL) {
        (*(void (**)(void*, s32))(*(u8**)(e1 + 0x138) + 0x30))(e1, 2);
    }
    u8* e2 = (u8*)ScreenSetFindElement(screen, 0x7A1060A7U);
    if (e2 != NULL) {
        (*(void (**)(void*, s32))(*(u8**)(e2 + 0x138) + 0x30))(e2, 2);
    }
    if (UIScreen_Is_Pushed(0x3AB51700U) != 0) {
        for (s32 i = 0; i < 2; i++) {
            ExplorationHudSetFighter(ScreenSetsCurrent(gScreenSets), *(void**)(gBattle + 0xA00 + i * 4), i);
        }
    }
    gSelectedPart = -1;
    lbl_805FB0BE = 0;
}

// 0x8005E514
// Picks a random entry from the table at +0xBA0 into arg1 while the flags at +0xA08 ask for it.
s32 fn_8005E514(u8* arg0, u8* arg1) {
    u8* ws = *(u8**)gWorldState;
    u8* zone = *(u8**)(ws + 0x1A4) + (s32)(*(u16*)(*(u8**)(arg0 + 0x4) + 0x30)) * gZoneResourceSize;
    if (*(u8*)(zone + 0xC0) != 0 && fn_801C6634(arg0 + 0xBA0)) {
        s32 a08 = *(s32*)(arg0 + 0xA08);
        s32 var29 = 0;
        if (!(a08 & 0x10) || !(a08 & 8) || fn_801C6634(arg1 + 8) == 0 || (*(s32*)(arg0 + 0xA08) & 2)) {
            var29 = 1;
        }
        if (var29 != 0) {
            for (;;) {
                if (!(fn_801C6634(arg1 + 8) == 0 || (*(s32*)(arg0 + 0xA08) & 2))) {
                    break;
                }
                u8* e = fn_801C668C(arg0 + 0xBA0);
                if (e == NULL || *(u8*)(e + 0x14) == 0) {
                    break;
                }
                u8* ent = e + (rand() % *(u8*)(e + 0x14)) * 4;
                *(u8*)(arg1 + 0x8) = *(u8*)(ent + 0x16);
                *(u8*)(arg1 + 0x9) = *(u8*)(ent + 0x17);
                *(u16*)(arg1 + 0xA) = *(u16*)(ent + 0x18);
                u8* e2 = fn_801C668C(arg0 + 0xBA0);
                if (e2 != NULL) {
                    *(f32*)(arg0 + 0xB94) = *(f32*)(e2 + 0x4);
                    *(f32*)(arg0 + 0xB98) = *(f32*)(e2 + 0x8);
                    *(f32*)(arg0 + 0xB9C) = *(f32*)(e2 + 0xC);
                    *(f32*)(arg0 + 0x9E8) = *(f32*)(e2 + 0x4);
                    *(f32*)(arg0 + 0x9EC) = *(f32*)(e2 + 0x8);
                    *(f32*)(arg0 + 0x9F0) = *(f32*)(e2 + 0xC);
                }
                *(s32*)(arg0 + 0xA08) = (*(s32*)(arg0 + 0xA08) | 8) & ~2;
            }
        }
    }
    return 1;
}

// 0x800698DC
// Frees the buffer table at +0x194 and the object at +0x8C, then releases the slot at +0x160.
void fn_800698DC(u8* arg0) {
    if (*(u8**)(arg0 + 0x194) != NULL && *(s32*)(arg0 + 0x1A4) > 1) {
        s32 off = 0;
        for (s32 i = 0; i < *(s32*)(arg0 + 0x1A4); i++) {
            u8* e = *(u8**)(*(u8**)(arg0 + 0x194) + off);
            if (e != NULL) {
                fn_801B1668(e);
                BufferFree(*(void**)(*(u8**)(arg0 + 0x194) + off));
                *(void**)(*(u8**)(arg0 + 0x194) + off) = NULL;
            }
            off += 4;
        }
        BufferFree(*(void**)(arg0 + 0x194));
        *(void**)(arg0 + 0x194) = NULL;
        *(void**)(arg0 + 0x8C) = NULL;
    } else {
        u8* m = *(u8**)(arg0 + 0x8C);
        if (m != NULL) {
            fn_801B1668(m);
            *(void**)(arg0 + 0x8C) = NULL;
        }
    }
    fn_801B4520(arg0);
    u8* o = *(u8**)(arg0 + 0x160);
    if (o != NULL) {
        (*(void (**)(void*))(*(u8**)o + 0x58))(o);
    }
    *(void**)(arg0 + 0x160) = NULL;
    *(s32*)(arg0 + 0x164) = -1;
}

// 0x80076654
// Adjusts the meter at +0x6BC by a lookup table and sends three events for slot arg1.
void fn_80076654(u8* arg0, s32 arg1) {
    s32 r30 = fn_80076928(arg0 + arg1 * 0x38 + 0x5D8, arg0);
    if (fn_800771F8(*(u8**)(arg0 + 0x6CC)) != 0) {
        r30 = 0;
    }
    u32 r29 = (r30 != 0) ? 0x633B1840U : 0x4E0DA72DU;
    if (r30 != 0) {
        s32 t = *(s32*)(arg0 + 0x6C0);
        *(s32*)(arg0 + 0x6C0) = t + 1;
        s32 k = RaySceneTest((void*)(s64)t);
        f32 v = *(f32*)(arg0 + 0x6BC) + lbl_80347260[k];
        *(f32*)(arg0 + 0x6BC) = v;
        if (v > lbl_805FD664) {
            *(f32*)(arg0 + 0x6BC) = lbl_805FD664;
        }
    } else {
        s32 k = RaySceneTest((void*)0x4E0E0000U);
        f32 v = *(f32*)(arg0 + 0x6BC) + lbl_80347270[k];
        *(f32*)(arg0 + 0x6BC) = v;
        if (v < lbl_805FD650) {
            *(f32*)(arg0 + 0x6BC) = lbl_805FD650;
        }
    }
    s32 off = arg1 * 4;
    s32 sel = (*(s32*)(arg0 + 0x6C8) * 0x10) & 0x10;
    EventSend(gEventMgr, *(void**)(arg0 + sel + off + 0x5B4), NULL, r29, 0, 0, 1, 0);
    EventSend(gEventMgr, *(void**)(arg0 + off + 0x584), NULL, r29, 0, 0, 1, 0);
    EventSend(gEventMgr, *(void**)(arg0 + off + 0x5A4), NULL, r29, 0, 0, 1, 0);
    *(s32*)(arg0 + 0x6C8) = *(s32*)(arg0 + 0x6C8) + 1;
}

// 0x80077218
// Walks the four child slots, then sets the three layer offsets from lookup tables and resets the matrix state.
void fn_80077218(u8* arg0) {
    void* cur = arg0;
    for (s32 i = 0; i < 4; i++) {
        s32 k = RaySceneTest(cur);
        u8* e = *(u8**)(arg0 + i * 4 + 0x14);
        cur = (*(void* (**)(void*, s32))(*(u8**)(e + 0x1C) + 0xC))(e, k);
    }
    s32 a = RaySceneTest(cur);
    f32 f31v = lbl_80347250[a];
    s32 b = RaySceneTest((void*)(s64)a);
    f32 f30v = f31v + lbl_80347230[b];
    s32 c = RaySceneTest((void*)(s64)b);
    f32 f31n = f31v + lbl_80347240[c];
    fn_8021F94C(*(u8**)(arg0 + 0x14), f31v);
    fn_8021F94C(*(u8**)(arg0 + 0x1C), f30v);
    fn_8021F94C(*(u8**)(arg0 + 0x20), f31n);
    memset(arg0 + 0x1028, 0xFF, 0x200);
    *(f32*)(arg0 + 0x8) = lbl_805FD670;
    *(f32*)(arg0 + 0x0) = lbl_805FD670;
    fn_8020A398(*(s32*)(arg0 + 0x12E4));
    fn_8020A398(*(s32*)(arg0 + 0x12E8));
    *(s32*)(arg0 + 0x12E4) = -1;
    *(s32*)(arg0 + 0x12E8) = -1;
    f32 v4 = (f32)fn_8021F7E8(*(u8**)(arg0 + 0x14));
    *(f32*)(arg0 + 0x4) = v4;
    *(f32*)(arg0 + 0xC) = v4;
    f32 d = (f32)fn_8021F7C0(*(u8**)(arg0 + 0x14));
    *(f32*)(arg0 + 0xC) = *(f32*)(arg0 + 0xC) * (lbl_805FD674 / d);
}

// 0x80079ADC
// Updates the object list at +0x90 against the current frame value and signals the game flow when done.
void fn_80079ADC(u8* arg0, f32 farg0) {
    f32 sp14, spC, sp10, sp8;
    u8 sp18[16];
    fn_800D8F70(*(u8**)(arg0 + 0x758), sp18);
    fn_800D8CB0(*(u8**)(arg0 + 0x758), &sp14, &spC, &sp10, &sp8);
    for (s32 i = *(s32*)(arg0 + 0x250); i < *(s32*)(arg0 + 0x8C); i++) {
        u8* p = *(u8**)(arg0 + i * 4 + 0x90);
        if (fn_8007CDBC(p, *(s32*)(arg0 + 0x760), sp14) != 0) {
            s32 n = *(s32*)(arg0 + 0x8C);
            for (s32 j = i + 1; j < n; j++) {
                if (*(u8**)(arg0 + i * 4 + 0x90) == *(u8**)(arg0 + j * 4 + 0x90)) {
                    fn_8007C958(*(u8**)(arg0 + j * 4 + 0x90), (f32*)(arg0 + j * 0xC + 0xE0));
                    break;
                }
            }
            *(s32*)(arg0 + 0x250) = *(s32*)(arg0 + 0x250) + 1;
        }
    }
    fn_8007CDBC(arg0 + 0x1D0, *(s32*)(arg0 + 0x760), sp14);
    fn_8007CDBC(arg0 + 0x210, *(s32*)(arg0 + 0x760), sp14);
    fn_8007D2C4(*(s32*)(arg0 + 0x760), farg0);
    fn_8007D2C8(*(s32*)(arg0 + 0x760));
    if (fn_800D8F90(*(u8**)(arg0 + 0x758)) == lbl_805FD6B8) {
        GameFlowSignal(-0x39F737BE, 0, 0);
    }
    if (fn_8007CF94(*(s32*)(arg0 + 0x760)) <= lbl_805FD68C) {
        GameFlowSignal(0x117741C3, 0, 0);
    }
}

}  // extern "C"
