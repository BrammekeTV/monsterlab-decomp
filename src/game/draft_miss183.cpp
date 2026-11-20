// Part-picker (lightning table / Troublescope) updates, combat HUD, turret muzzle and the
// achievement counters kept in the save block.
#include "types.h"
#include "engine/math.h"
#include "game/part.h"
extern "C" {
#include "game/script_callbacks.h"
}

struct Locator;
extern "C++" u8* fn_801B55A8(u8* ent);               // 0x801B55A8: returns a count (as an integer value)

extern "C" {
void* fn_8003F7DC(void);                                    // 0x8003F7DC: (value is an id, not a pointer)
u32   fn_8003F858(void);                                    // 0x8003F858
s32   fn_8003F8D4(void);                                    // 0x8003F8D4
void  fn_8003DCE0(u8* p);                                   // 0x8003DCE0
void  fn_8001D4FC(u8* mgr);                                 // 0x8001D4FC
void  fn_8001D718(u8* mgr, s32 idx, u8* slot);              // 0x8001D718
void  fn_8004C718(u8* p, s32 idx, void* elem);              // 0x8004C718
s32   fn_8004D2C8(u8* obj, const char* name, s32 a, s32 b); // 0x8004D2C8
s32   fn_8004D3F8(void* c, const char* name, s32 a, s32 b); // 0x8004D3F8
void  fn_8004D760(u8* c, u8* monster);                      // 0x8004D760
void* fn_8004DAF8(void* c, s32 a, s32 b);                   // 0x8004DAF8
void  fn_8004DCEC(u8* p, s32 idx);                          // 0x8004DCEC
s32   fn_8004E300(void* a, s32 b);                          // 0x8004E300
u8*   fn_8004EDD0(u8* obj, s32 flag, s32 a2);               // 0x8004EDD0
void* fn_8004FFB4(void* id, void* part);                    // 0x8004FFB4
s32   fn_801A3010(void* obj, void* data);                   // 0x801A3010
void  fn_801B55F4(u8* obj, u8* out, s32 idx);               // 0x801B55F4
void* fn_801B7198(void* anim, s32 a);                       // 0x801B7198
u8*   FighterFromStats(u8* stats);                          // 0x80213F8C
s32   FighterStatsGet(u8* stats, s32 i);                    // 0x80214950
s32   MonsterGetName(u8* m, char* out, s32 size);           // 0x8021C968
void  MtxFromLocatorRotation(f32 out[12], const void* rot); // 0x80143CA8
void  NoticePosted(u8* p, s32 idx);                         // 0x8022DF3C
s32   PartGetBattleEnergy(const PartInstance* torso, u8** limbs);   // 0x8021ED90
void  QuatFromMtx(Quat* out, const f32 m[12]);              // 0x801456E4
void* ScreenSetFindElement(void* set, u32 hash);            // 0x80198F18
void* ScreenSetsCurrent(void* sets);                        // 0x8018D1E4
s32   SysSprintf(char* buf, const char* fmt, ...);          // 0x80142EFC
void  UISetNumber(void* el, s32 n);                         // 0x8019F2A8
void  UISetText(void* el, const char* text, s32 a);         // 0x8019EFC8
Locator* WorldFindLocator(void* world, u32 hash);           // 0x8012AA50
void  WorldPreUpdate(void* world);                          // 0x80130418

extern u8   gWorldState[];       // 0x803E6D00
extern s32  lbl_803C4E28[];      // 0x803C4E28 (combat part zone-loaded flags)
extern u8*  lbl_805FAF78;        // 0x805FAF78 (lightning / operating table manager)
extern u8*  lbl_805FAF80;        // 0x805FAF80 (Troublescope state)
extern u8*  lbl_805FB014;        // 0x805FB014 (battle camera object)
extern s32  lbl_805FB034;        // 0x805FB034 (number of combat part zones)
extern s32  lbl_805FB038;        // 0x805FB038
extern u8   lbl_805FB0BF;        // 0x805FB0BF
extern u8   lbl_805FB0C0;        // 0x805FB0C0
extern u8   lbl_805FB1C0;        // 0x805FB1C0
extern u8   lbl_805FB1C1;        // 0x805FB1C1
extern u32  lbl_805F8638[];      // 0x805F8638 (element hashes: energy numbers)
extern u32  lbl_805F8640[];      // 0x805F8640 (element hashes: names)
extern u32  lbl_805F8648[];      // 0x805F8648 (element hashes: energy bars)
extern const f32 lbl_80346020[]; // 0x80346020 (Vec3 offset)
}
extern "C++" u8* gBattle;            // 0x805FBBB8
extern "C++" u8* gSaveSystem;        // 0x805FBBE0
extern "C++" void* gScreenSets;      // 0x805FB654

// UI element virtual (table pointer at +0x138, slot +0x2C): notify the element of an event.
typedef void (*UIElemNotifyFn)(void*, s32);
typedef u8* (*ObjGetFn)(u8*);

#define HUD_COMBAT 0x1B66AC94u   // "UI_MSA_Combat_HUD"

extern "C" {

// 0x8001E954
// Finishes the part loading of the lightning table: when every zone is loaded, attaches the
// loaded part of each of the five slots (clears flags on the slot object, shows it) and parks the
// pointer entity at the "hanging" locator, then resets the zone flags.
void UpdatePartLoading(void) {
    s32 i;
    for (i = 0; i < 5; i++) {
        fn_8001D718(lbl_805FAF78, i, NULL);
    }
    for (i = 0; i < lbl_805FB034; i++) {
        if (lbl_803C4E28[i] == 0) return;
    }
    for (i = 0; i < 5; i++) {
        s32 off = i * 4;
        s32 soff = i * 0x1A4;
        fn_8004DCEC(*(u8**)lbl_805FAF78, i);
        void* ent = *(void**)(*(u8**)lbl_805FAF78 + off + 0xC6C);
        if (ent != NULL) {
            *(void**)(lbl_805FAF78 + soff + 0x1A4) = fn_8004FFB4(ent, *(void**)(lbl_805FAF78 + off + 0x92C));
            u8* slot = lbl_805FAF78 + soff + 8;
            s32 count = *(s32*)(slot + 0x198);
            s32 sel;
            if (count > 0 && *(s32*)(slot + 0x194) < count) {
                sel = *(s32*)(slot + *(s32*)(slot + 0x194) * 4);
            } else {
                sel = -1;
            }
            if (sel >= 0) {
                u8* body = *(u8**)(*(u8**)lbl_805FAF78 + off + 0xC50);
                if (body != NULL) *(u8*)(body + 0x28C) = 0;
                fn_8004C718(*(u8**)lbl_805FAF78, i, NULL);
                fn_8004C718(*(u8**)lbl_805FAF78, i, *(void**)(lbl_805FAF78 + soff + 0x1A4));
                u8* obj = *(u8**)(lbl_805FAF78 + soff + 0x1A8);
                if ((*(ObjGetFn*)(*(u8**)obj + 0xC))(obj) != NULL) {
                    u8* c = (*(ObjGetFn*)(*(u8**)obj + 0xC))(obj);
                    *(u32*)(c + 8) &= ~2u;
                }
                *(u32*)(*(u8**)(obj + 4) + 0x20) &= ~2u;
                *(u32*)(*(u8**)(*(u8**)(lbl_805FAF78 + soff + 0x1A8) + 4) + 0x20) |= 0x8000;
                fn_8004EDD0(*(u8**)(*(u8**)lbl_805FAF78 + off + 0xC50), 1, 0);
                u8* body2 = *(u8**)(*(u8**)lbl_805FAF78 + off + 0xC50);
                *(u32*)(body2 + 0x15C) |= 0x400000;
            }
            *(void**)(*(u8**)lbl_805FAF78 + off + 0xC6C) = NULL;
            u8* mgr = lbl_805FAF78;
            if (*(u8**)mgr != NULL) {
                u8* loc = (u8*)WorldFindLocator(gWorldState, 0x42BCE9A5u);
                if (loc != NULL) {
                    f32* dst = *(f32**)(*(u8**)mgr + 4);
                    f32 rot[15];
                    f32 m[12];
                    dst[0] = *(f32*)(loc + 0x8);
                    dst[1] = *(f32*)(loc + 0xC);
                    dst[2] = *(f32*)(loc + 0x10);
                    m[0] = 1.0f; m[1] = 0.0f; m[2] = 0.0f;
                    m[3] = 0.0f; m[4] = 1.0f; m[5] = 0.0f;
                    m[6] = 0.0f; m[7] = 0.0f; m[8] = 1.0f;
                    m[9] = 0.0f; m[10] = 0.0f; m[11] = 0.0f;
                    rot[0] = *(f32*)(loc + 0x14) * 0.63661975f;
                    rot[1] = *(f32*)(loc + 0x18) * 0.63661975f;
                    rot[2] = *(f32*)(loc + 0x1C) * 0.63661975f;
                    rot[3] = 0.0f; rot[4] = 0.0f; rot[5] = 0.0f; rot[6] = 0.0f; rot[7] = 0.0f;
                    rot[8] = 1.0f; rot[9] = 0.0f; rot[10] = 1.0f; rot[11] = 0.0f; rot[12] = 1.0f;
                    rot[13] = 0.0f; rot[14] = 0.0f;
                    MtxFromLocatorRotation(m, rot);
                    QuatFromMtx((Quat*)(*(u8**)(*(u8**)mgr + 4) + 0xC), m);
                }
                *(u32*)(*(u8**)(*(u8**)mgr + 4) + 0x20) |= 0x8000;
                *(u8*)(*(u8**)mgr + 0xC83) = 0;
            }
            fn_8004D2C8(*(u8**)lbl_805FAF78, "hanging", 0, 1);
            if (lbl_805FAF78 != NULL) {
                u8* chr = *(u8**)lbl_805FAF78;
                if (chr != NULL) fn_8004D3F8(chr, "hanging", 0, 1);
            }
            *(u8*)(lbl_805FAF78 + 0xA8C) = 1;
            if (i == *(s32*)(lbl_805FAF78 + 0xA80)) fn_8001D4FC(lbl_805FAF78);
        }
    }
    *(u8*)(lbl_805FAF78 + 0xA8C) = 1;
    for (i = 0; i < lbl_805FB034; i++) lbl_803C4E28[i] = 0;
    lbl_805FB034 = 0;
}

// 0x8001EFFC
// Per-frame update of the Troublescope part picker: once every zone is loaded and no part is
// still being created, swaps the five slot entities and shows the selected monster; then, while
// the picker object is up, resolves the freshly loaded part entities and keeps the picker object
// at its own position (offset down while the object's status is 0xD).
void Troublescope_Update(void) {
    if (lbl_805FAF80 == NULL) return;
    if (*(s32*)(lbl_805FAF80 + 0x1C) == 0 && *(s32*)(lbl_805FAF80 + 0x20) == 0) {
        s32 ready = 1;
        s32 i;
        for (i = 0; i < lbl_805FB034; i++) {
            if (lbl_803C4E28[i] == 0) ready = 0;
        }
        if (ready) {
            u8* mgr = *(u8**)(lbl_805FAF80 + 0xC);
            uintptr_t w = (uintptr_t)*(void**)(mgr + 0xC6C);
            if (w != 0 && w < 0x3E8) ready = 0;
            w = (uintptr_t)*(void**)(mgr + 0xC70);
            if (w != 0 && w < 0x3E8) ready = 0;
            w = (uintptr_t)*(void**)(mgr + 0xC74);
            if (w != 0 && w < 0x3E8) ready = 0;
            w = (uintptr_t)*(void**)(mgr + 0xC78);
            if (w != 0 && w < 0x3E8) ready = 0;
            w = (uintptr_t)*(void**)(mgr + 0xC7C);
            if (w != 0 && w < 0x3E8) ready = 0;
        }
        if (ready) {
            for (i = 0; i < 5; i++) {
                u8* m = *(u8**)(lbl_805FAF80 + 0xC);
                u8* body = *(u8**)(m + i * 4 + 0xC50);
                fn_8004C718(m, i, NULL);
                fn_8003DCE0(body);
            }
            *(u8**)(lbl_805FAF80 + 4) = *(u8**)(gSaveSystem + *(s32*)(lbl_805FAF80 + 0x10) * 4 + 0xA30);
            if (*(u8**)(lbl_805FAF80 + 4) != NULL) {
                fn_8004D760(*(u8**)(lbl_805FAF80 + 0xC), *(u8**)(lbl_805FAF80 + 4));
                fn_8004DAF8(*(u8**)(lbl_805FAF80 + 0xC), 1, 0);
                lbl_805FB038 = 5;
            }
            u8* obj = *(u8**)(lbl_805FAF80 + 8);
            if (obj != NULL) {
                fn_801A3010(obj, *(u8**)(lbl_805FAF80 + 0xC));
                obj = *(u8**)(lbl_805FAF80 + 8);
                (*(UIElemNotifyFn*)(*(u8**)(obj + 0x138) + 0x2C))(obj, 0x1A);
            }
            *(s32*)(lbl_805FAF80 + 0x14) = *(s32*)(lbl_805FAF80 + 0x10);
        }
    }
    u8* picker = *(u8**)(lbl_805FAF80 + 8);
    if (picker != NULL && fn_801B7198(picker + 0x140, 0) != NULL) {
        s32 i;
        WorldPreUpdate(gWorldState);
        for (i = 0; i < lbl_805FB034; i++) {
            if (lbl_803C4E28[i] == 0) return;
        }
        for (i = 0; i < 5; i++) {
            s32 off = i * 4;
            fn_8004DCEC(*(u8**)(lbl_805FAF80 + 0xC), i);
            void* ent = *(void**)(*(u8**)(lbl_805FAF80 + 0xC) + off + 0xC6C);
            if (ent != NULL) {
                void* e = fn_8004FFB4(ent, *(void**)(*(u8**)(lbl_805FAF80 + 4) + off + 4));
                if (e != NULL) {
                    fn_8004C718(*(u8**)(lbl_805FAF80 + 0xC), i, NULL);
                    fn_8004C718(*(u8**)(lbl_805FAF80 + 0xC), i, e);
                    u8* body = *(u8**)(*(u8**)(lbl_805FAF80 + 0xC) + off + 0xC50);
                    if (body != NULL) {
                        fn_8004EDD0(body, 1, 0);
                        u8* body2 = *(u8**)(*(u8**)(lbl_805FAF80 + 0xC) + off + 0xC50);
                        *(u32*)(body2 + 0x15C) |= 0x400000;
                    }
                }
                *(void**)(*(u8**)(lbl_805FAF80 + 0xC) + off + 0xC6C) = NULL;
            }
        }
        for (i = 0; i < lbl_805FB034; i++) lbl_803C4E28[i] = 0;
        lbl_805FB034 = 0;
        if (*(s32*)(lbl_805FAF80 + 0x10) == *(s32*)(lbl_805FAF80 + 0x14)) *(s32*)(lbl_805FAF80 + 0x1C) = 1;
    }
    u8* obj = *(u8**)(lbl_805FAF80 + 8);
    if (obj != NULL) {
        f32 x = *(f32*)(obj + 0x14);
        f32 y = *(f32*)(obj + 0x18);
        f32 z = *(f32*)(obj + 0x1C);
        void* anim = fn_801B7198(obj + 0x140, 0);
        if (anim != NULL && fn_8004E300(anim, 1) == 0xD) {
            x += lbl_80346020[0];
            y += lbl_80346020[1];
            z += lbl_80346020[2];
        }
        obj = *(u8**)(lbl_805FAF80 + 8);
        *(f32*)(obj + 0x68) = x;
        *(f32*)(obj + 0x6C) = y;
        *(f32*)(obj + 0x70) = z;
        obj = *(u8**)(lbl_805FAF80 + 8);
        (*(UIElemNotifyFn*)(*(u8**)(obj + 0x138) + 0x2C))(obj, 0x18);
    }
}

// 0x800463D0
// Per-frame update of the combat HUD: round counter, the two "mutant is down" banners in
// multiplayer, the two fighters' names and energy numbers/bars, and the attack-hint flags.
void UpdateCombatHUD(f32 a0) {
    char buf[0x100];
    s32 i;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    if (*(u32*)set != HUD_COMBAT) return;
    void* el = ScreenSetFindElement(set, 0x08A85674u);
    if (el != NULL) {
        SysSprintf(buf, "Round: %d", *(s32*)(gBattle + 0xA10));
        UISetText(el, buf, 1);
        (*(UIElemNotifyFn*)(*(u8**)((u8*)el + 0x138) + 0x2C))(el, 2);
    }
    if (IsMultiPlayer() != 0) {
        u8* a = *(u8**)(gBattle + 0xA00);
        u8* b = *(u8**)(gBattle + 0xA04);
        u8* sa = *(u8**)(a + 0x20);
        if (sa != NULL) {
            u8* sb = *(u8**)(b + 0x20);
            if (sb != NULL) {
                if (*(u8*)(sa + 4) != 0 && *(u8*)(sb + 4) == 0) {
                    if (lbl_805FB0C0 == 0) {
                        lbl_805FB0C0 = 1;
                        UIEvent_SendToElement(0x63CC8B0Bu, 0xA3556852u);
                    }
                } else if (lbl_805FB0C0 != 0 && UIElement_IsUIAnimPlaying(0xA3556852u) == 0) {
                    lbl_805FB0C0 = 0;
                    UIEvent_SendToElement(0xA51E8C39u, 0xA3556852u);
                }
                if (*(u8*)(*(u8**)(a + 0x20) + 4) == 0 && *(u8*)(*(u8**)(b + 0x20) + 4) != 0) {
                    if (lbl_805FB0BF == 0) {
                        lbl_805FB0BF = 1;
                        UIEvent_SendToElement(0x8D64AB0Eu, 0x9F13DBBDu);
                    }
                } else if (lbl_805FB0BF != 0 && UIElement_IsUIAnimPlaying(0x9F13DBBDu) == 0) {
                    lbl_805FB0BF = 0;
                    UIEvent_SendToElement(0x45752889u, 0x9F13DBBDu);
                }
            }
        }
    }
    for (i = 0; i < 2; i++) {
        u8* stats = *(u8**)(gBattle + i * 4 + 0xA00) + 0x20;
        void* nameEl = ScreenSetFindElement(set, lbl_805F8640[i]);
        if (nameEl != NULL) {
            u8* m = *(u8**)(FighterFromStats(stats) + 0x1C);
            if (m != NULL) m -= 0xC48;
            if (MonsterGetName(m + 0x898, buf, 0x100) <= 0) {
                SysSprintf(buf, "Mutant %d", 0);
            }
            UISetText(nameEl, buf, 1);
            (*(UIElemNotifyFn*)(*(u8**)((u8*)nameEl + 0x138) + 0x2C))(nameEl, 2);
        }
        void* barEl = ScreenSetFindElement(set, lbl_805F8648[i]);
        if (barEl != NULL) {
            s32 cur = FighterStatsGet(stats, 3);
            if (cur > 0x3E7) cur = 0x3E7;
            PartInstance* torso = *(PartInstance**)*(u8**)(FighterFromStats(stats) + 0x10);
            s32 maxE = PartGetBattleEnergy(torso, (u8**)FighterFromStats(stats));
            if (maxE != 0) {
                if (maxE < cur) maxE = cur;
                void* numEl = ScreenSetFindElement(set, lbl_805F8638[i]);
                if (numEl != NULL) {
                    UISetNumber(numEl, cur);
                    (*(UIElemNotifyFn*)(*(u8**)((u8*)numEl + 0x138) + 0x2C))(numEl, 2);
                }
                *(f32*)((u8*)barEl + 0x78) = 32.0f * ((f32)cur / (f32)maxE);
                (*(UIElemNotifyFn*)(*(u8**)((u8*)barEl + 0x138) + 0x2C))(barEl, 0x18);
            }
        }
    }
    if (fn_8003F8D4() != 0 && *(s32*)(lbl_805FB014 + 0x4F4) == 1) {
        u8* cam = *(u8**)(lbl_805FB014 + 0x4D4);
        u8* shot = (*(ObjGetFn*)(*(u8**)cam + 0x10))(cam);
        lbl_805FB1C1 = 1;
        if ((uintptr_t) * (u32*)(shot + 0xEC) == (uintptr_t)fn_8003F7DC()) {
            lbl_805FB1C0 = 1;
            return;
        }
        if (*(u32*)(shot + 0xEC) == fn_8003F858()) {
            lbl_805FB1C0 = 0;
            lbl_805FB1C1 = 0;
        }
    }
}

// 0x800EBE74
// Turret: sets the muzzle orientation from a locator (a1: u16 flags, u8 index, u32 value stored at
// +0x178). The locator matrix (from the entity's locator list or the record at +0x98, 0xC8 each)
// is rotated by the entity's quaternion and written back as a quaternion at +0x17C.
void TurretSetMuzzle(void* a0, u32 /*0x30*/ a1) {
    u8* obj = (u8*)a0;
    u8* args = (u8*)(uintptr_t)a1;
    f32 m[12];
    s32 have;
    if (!(*(u32*)(*(u8**)(obj + 4) + 0x1C) & 0x40)) return;
    u16 flags = *(u16*)args;
    s32 idx = (flags & 1) ? *(u8*)(args + 2) : -1;
    if (idx != -1) {
        if ((intptr_t)idx < (intptr_t)fn_801B55A8(obj)) {
            fn_801B55F4(obj, (u8*)m, idx);
            have = 1;
        } else {
            have = 0;
        }
    } else {
        s32 j = (flags & 2) ? *(u8*)(args + 2) : -1;
        if (j != -1) {
            u8* rec = *(u8**)(obj + 0x98) + j * 0xC8;
            have = 1;
            for (s32 k = 0; k < 12; k++) m[k] = *(f32*)(rec + 0x38 + k * 4);
        } else {
            have = 0;
        }
    }
    if (have != 0) {
        u8* ent = *(u8**)(obj + 4);
        f32 qy = *(f32*)(ent + 0x10);
        f32 qz = *(f32*)(ent + 0x14);
        f32 qx = *(f32*)(ent + 0xC);
        f32 qw = *(f32*)(ent + 0x18);
        f32 y2 = 2.0f * qy;
        f32 z2 = 2.0f * qz;
        f32 x2 = 2.0f * qx;
        f32 yy = y2 * qy;
        f32 zz = z2 * qz;
        f32 zw = z2 * qw;
        f32 xy = y2 * qx;
        f32 yw = y2 * qw;
        f32 r10 = xy - zw;
        f32 r00 = 1.0f - (yy + zz);
        f32 r01 = xy + zw;
        f32 zx = z2 * qx;
        f32 xx = x2 * qx;
        f32 r20 = zx + yw;
        f32 r02 = zx - yw;
        f32 xw = x2 * qw;
        f32 yz = z2 * qy;
        f32 r22 = 1.0f - (xx + yy);
        f32 r11 = 1.0f - (xx + zz);
        f32 r12 = yz + xw;
        f32 r21 = yz - xw;
        f32 out[12];
        out[0] = (r20 * m[2]) + ((r00 * m[0]) + (r10 * m[1]));
        out[3] = (r20 * m[5]) + ((r00 * m[3]) + (r10 * m[4]));
        out[9] = 0.0f + ((r20 * m[11]) + ((r00 * m[9]) + (r10 * m[10])));
        out[10] = 0.0f + ((r21 * m[11]) + ((r01 * m[9]) + (r11 * m[10])));
        out[11] = 0.0f + ((r22 * m[11]) + ((r02 * m[9]) + (r12 * m[10])));
        out[1] = (r21 * m[2]) + ((r01 * m[0]) + (r11 * m[1]));
        out[4] = (r21 * m[5]) + ((r01 * m[3]) + (r11 * m[4]));
        out[2] = (r22 * m[2]) + ((r02 * m[0]) + (r12 * m[1]));
        out[7] = (r21 * m[8]) + ((r01 * m[6]) + (r11 * m[7]));
        out[6] = (r20 * m[8]) + ((r00 * m[6]) + (r10 * m[7]));
        out[5] = (r22 * m[5]) + ((r02 * m[3]) + (r12 * m[4]));
        out[8] = (r22 * m[8]) + ((r02 * m[6]) + (r12 * m[7]));
        QuatFromMtx((Quat*)(obj + 0x17C), out);
        *(u32*)(obj + 0x178) = *(u32*)args;
    }
}

}  // extern "C"

// ---- achievement counters in the save block ----
// Flags: 256-bit set at save + 0x20 (four big-endian u64 words, highest 64 ids first).
static u64 SaveFlagWord(const u8* s, u32 base) {
    return ((u64) * (const u32*)(s + base) << 32) | *(const u32*)(s + base + 4);
}

static u32 SaveFlagBase(u32 i) {
    return i < 0x40 ? 0x38 : i < 0x80 ? 0x30 : i < 0xC0 ? 0x28 : 0x20;
}

static BOOL SaveFlagGet(const u8* s, s32 id) {
    u32 i = (u8)id;
    return (BOOL)((SaveFlagWord(s, SaveFlagBase(i)) >> (i & 0x3F)) & 1);
}

static void SaveFlagSet(u8* s, s32 id) {
    u32 i = (u8)id;
    u32 base = SaveFlagBase(i);
    u64 v = SaveFlagWord(s, base) | ((u64)1 << (i & 0x3F));
    *(u32*)(s + base) = (u32)(v >> 32);
    *(u32*)(s + base + 4) = (u32)v;
}

// Notice queue (save + 0x744, 15 entries of 0x1C; count +0x8E8, sequence +0x910).
static void SavePostNotice(u8* s, s32 type, s32 a) {
    s32 n = *(s32*)(s + 0x8E8);
    if (n >= 15) return;
    s32* rec = (s32*)(s + 0x744 + n * 0x1C);
    rec[0] = type; rec[1] = a; rec[2] = 0; rec[3] = 0; rec[4] = 0; rec[5] = -1;
    rec[6] = ++*(s32*)(s + 0x910);
    *(s32*)(s + 0x8E8) = n + 1;
    NoticePosted(s, n);
}

// Unlocks achievement `id` once the counter reaches the threshold in the table at
// gGameDb->data + 0x99D0 (8 bytes each, f32 limit at +4): bumps the unlock count, sets the flag,
// posts a notice, adds the achievement's score (table at +0x10A8, 0x18 each, f32 at +0x10) to the
// save's total (capped at 99999999) and posts a rank-up notice when the total passes the next
// rank limit (table at +0x63C8, 0x10 each, f32 at +8; rank at save + 0x128).
static void SaveAchievementCheck(u8* s, s32 id, s32 thr, s32 total) {
    if (SaveFlagGet(s, id)) return;
    const u8* t = *(u8**)(gGameDb->data + 0x99D0) + thr * 8;
    if (total < (s32)*(const f32*)(t + 4)) return;
    if (id == -1) return;
    *(s32*)(s + 0x120) += 1;
    SaveFlagSet(s, id);
    SavePostNotice(s, 4, id);
    const u8* e = *(u8**)(gGameDb->data + 0x10A8) + id * 0x18;
    u32 score = *(u32*)(gSaveSystem + 0x12C) + (u32)*(const f32*)(e + 0x10);
    *(u32*)(gSaveSystem + 0x12C) = score;
    if (score > 0x05F5E0FFu) *(u32*)(gSaveSystem + 0x12C) = 0x05F5E0FFu;
    s32 rank = *(s32*)(gSaveSystem + 0x128);
    if (rank < 0x31) {
        u32 next = (u32)rank + 1;
        BOOL changed = FALSE;
        f32 limit = *(f32*)(*(u8**)(gGameDb->data + 0x63C8) + next * 0x10 + 8);
        while (limit < (f32)*(u32*)(gSaveSystem + 0x12C) && next <= 0x31) {
            *(s32*)(gSaveSystem + 0x128) = (s32)next;
            next += 1;
            changed = TRUE;
            if (next <= 0x31) limit = *(f32*)(*(u8**)(gGameDb->data + 0x63C8) + next * 0x10 + 8);
        }
        if (changed) SavePostNotice(gSaveSystem, 1, *(s32*)(gSaveSystem + 0x128));
    }
}

// Adds `n` to the counter at `counterOff` and checks each (id, threshold) pair, last first.
static void SaveCountAchievements(u8* s, s32 counterOff, const s32* ids, const s32* thr, s32 count, s32 total) {
    for (s32 i = count - 1; i >= 0; i--) SaveAchievementCheck(s, ids[i], thr[i], total);
}

extern "C" {

// 0x8022AE70
// Counts blocked attacks (save + 0x9E8) and unlocks the block achievements.
void SaveCountBlocks(u8* s, s32 n) {
    static const s32 ids[2] = { 0x5B, 0x5D };
    static const s32 thr[2] = { 0x75, 0x77 };
    s32 total = *(s32*)(s + 0x9E8) + n;
    *(s32*)(s + 0x9E8) = total;
    SaveCountAchievements(s, 0x9E8, ids, thr, 2, total);
}

// 0x8022B38C
// Counts dodged attacks (save + 0x9EC) and unlocks the dodge achievements.
void SaveCountDodges(u8* s, s32 n) {
    static const s32 ids[2] = { 0x5C, 0x5E };
    static const s32 thr[2] = { 0x76, 0x78 };
    s32 total = *(s32*)(s + 0x9EC) + n;
    *(s32*)(s + 0x9EC) = total;
    SaveCountAchievements(s, 0x9EC, ids, thr, 2, total);
}

// 0x8022A3D8
// Counts limbs lost (save + 0x9E0) and unlocks the limbs-lost achievements.
void SaveCountLimbsLost(u8* s, s32 n) {
    static const s32 ids[3] = { 0x90, 0x91, 0x92 };
    static const s32 thr[3] = { 0x99, 0x9A, 0x9B };
    s32 total = *(s32*)(s + 0x9E0) + n;
    *(s32*)(s + 0x9E0) = total;
    SaveCountAchievements(s, 0x9E0, ids, thr, 3, total);
}

// 0x8022A90C
// Counts limbs broken (save + 0x9E4) and unlocks the limbs-broken achievements.
void SaveCountLimbsBroken(u8* s, s32 n) {
    static const s32 ids[6] = { 0x53, 0x54, 0x55, 0x56, 0x57, 0x58 };
    static const s32 thr[6] = { 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74 };
    s32 total = *(s32*)(s + 0x9E4) + n;
    *(s32*)(s + 0x9E4) = total;
    SaveCountAchievements(s, 0x9E4, ids, thr, 6, total);
}

// 0x8022B8A8
// Counts battles fought (save + 0x9F0) and unlocks the battle-count achievements.
void SaveCountBattleResult(u8* s, s32 n) {
    static const s32 ids[3] = { 0x93, 0x94, 0x95 };
    static const s32 thr[3] = { 0x9C, 0x9D, 0x9E };
    s32 total = *(s32*)(s + 0x9F0) + n;
    *(s32*)(s + 0x9F0) = total;
    SaveCountAchievements(s, 0x9F0, ids, thr, 3, total);
}

// 0x80226C4C
// Records battle wins: resets the loss streak (save + 0x8F8), adds to the win counter
// (+0x8F0) and unlocks the win achievements.
void SaveRecordBattleWin(u8* s, s32 n) {
    static const s32 ids[7] = { 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52 };
    static const s32 thr[7] = { 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E };
    s32 total = *(s32*)(s + 0x8F0) + n;
    *(s32*)(s + 0x8F8) = 0;
    *(s32*)(s + 0x8F0) = total;
    SaveCountAchievements(s, 0x8F0, ids, thr, 7, total);
}

// 0x802271BC
// Records battle losses: adds to the loss counter (save + 0x8F4) and the loss streak (+0x8F8)
// and unlocks the loss achievements.
void SaveRecordBattleLoss(u8* s, s32 n) {
    static const s32 ids[3] = { 0x8D, 0x8E, 0x8F };
    static const s32 thr[3] = { 0x96, 0x97, 0x98 };
    s32 total = *(s32*)(s + 0x8F4) + n;
    s32 streak = *(s32*)(s + 0x8F8) + n;
    *(s32*)(s + 0x8F4) = total;
    *(s32*)(s + 0x8F8) = streak;
    SaveCountAchievements(s, 0x8F4, ids, thr, 3, total);
}

// 0x8022BDE4
// Stores the best single hit (save + 0x9F4) and unlocks the best-hit achievement.
void SaveSetBestHit(u8* s, s32 hit) {
    *(s32*)(s + 0x9F4) = hit;
    SaveAchievementCheck(s, 0x5F, 0x79, hit);
}

}  // extern "C"
