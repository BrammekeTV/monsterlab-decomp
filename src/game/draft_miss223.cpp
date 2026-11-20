// Battle setup callbacks (0x8004xxxx), saw-path difficulty switch, tree copy, attachment helpers.
#include "types.h"
#include "game/part.h"
#include "engine/math.h"

typedef u8* (*GetRootFn)(u8* obj);          // vtable slot 0xC: the object's flag block (or NULL)
typedef s32 (*CanUseFn)(u8* obj, u8* arg);  // vtable slot 0xCC

struct StringTable;
extern u8* gBattle;                       // 0x805FBBB8
extern "C++" StringTable* gStringTable;   // 0x805FB910
extern void* gStringTable2;               // 0x805FB90C
extern u8 gWorldState[];                  // 0x803E6D00 (first word: world block pointer)
extern "C++" s32 gZoneResourceSize;       // 0x805FB94C
void MonsterGenerateOpponent(void* builder, s32 tmplIndex);   // 0x8021A1D0 (C++ linkage)

extern "C" {
void BattleReset(u8*);                                        // 0x80212BE4-area battle reset
void EulerToQuat(Quat* out, const Vec3* euler);               // 0x80145B00
void FighterStatsSetOwner(u8* stats, u32 v);                  // 0x80213F84

int SysSprintf(char* buf, const char* fmt, ...);
void PartSetDef(PartInstance* p, s32 def);                    // 0x8021D43C
void fn_8003DCE0(u8* p);                                      // 0x8003DCE0
void* fn_8003DFCC(void* part, s32 idx, s32 def, s32 zero, s32 side);  // 0x8003DFCC
void fn_8004C718(u8* p, s32 idx, void* elem);                 // 0x8004C718
void fn_800A3CAC(u8* obj, u8* tbl, s32 kind);                 // 0x800A3CAC
void fn_801458B0(Quat* q, const Vec3* a, const Vec3* b);      // 0x801458B0
void fn_80146180(void* quat, const Vec3* axis, f32 angle);    // 0x80146180
u8* fn_801B3938(s32 a);                                       // 0x801B3938
void fn_801B4308(u8* p, const char* text);                    // 0x801B4308
s32 fn_801B5510(u8* a, u8* b, s32 c, f32* v, s32 d);          // 0x801B5510
void* fn_801D40E0(s32 n);                                     // 0x801D40E0
u32 fn_801EC880(u8* p, u32 hash);                             // 0x801EC880
u8* fn_801FA7CC(u8* p, s32 a4);                               // 0x801FA7CC
s32 fn_80336BF8(u8* file, const char* fmt, ...);              // 0x80336BF8
void fn_8033B09C(void);                                       // 0x8033B09C
void operator_delete(void* p);                                // 0x801D426C
char* strcpy(char* d, const char* s);

extern u8 __files[];               // 0x803B4978
extern u8 lbl_80386320[];          // 0x80386320 (vtable)
extern s32 lbl_803C51D8[];         // 0x803C51D8
extern const char lbl_805F8200[];  // 0x805F8200 (empty string)
extern const char lbl_80354800[];  // 0x80354800 ("Not found")
extern const char lbl_8035480C[];  // 0x8035480C ("NOT FOUND")
extern u8 lbl_805FB070[];          // 0x805FB070
extern u8* lbl_805FB078[];         // 0x805FB078 (pointer table)
extern u8* lbl_805FB080[];         // 0x805FB080 (pointer table)
extern f32 lbl_805FE208;           // 0x805FE208 (0.0f)
extern f32 lbl_805FE20C;           // 0x805FE20C (1.0f)
extern f32 lbl_805FE228;           // 0x805FE228 (2/pi)
}

// Sets or clears flag bits in the object's root block (vtable slot 0xC, +8) and in its child (+4, +0x20).
static void ObjSetFlags(u8* o, u32 rootBits, u32 childBits, bool set) {
    if ((*(GetRootFn*)(*(u8**)o + 0xC))(o) != NULL) {
        u8* r = (*(GetRootFn*)(*(u8**)o + 0xC))(o);
        if (set) *(u32*)(r + 0x8) |= rootBits;
        else *(u32*)(r + 0x8) &= ~rootBits;
    }
    u8* c = *(u8**)(o + 0x4);
    if (set) *(u32*)(c + 0x20) |= childBits;
    else *(u32*)(c + 0x20) &= ~childBits;
}

// String-table lookup by text id: -1 gives the not-found text, the high bit selects the second table.
static const char* LookupText(s32 id) {
    if (id == -1) return lbl_80354800;
    if (((u32)id & 0x80000000u) && gStringTable2 != NULL) {
        u8* t = (u8*)gStringTable2;
        const char** arr = *(const char***)(t + 0x0);
        u32 idx = (u32)id & 0x7FFFFFFFu;
        if (arr != NULL && idx < *(u32*)(t + 0xC)) return arr[idx];
        return lbl_805F8200;
    }
    u8* t = (u8*)gStringTable;
    const char** arr = *(const char***)(t + 0x0);
    if (arr != NULL && (u32)id < *(u32*)(t + 0xC)) return arr[id];
    if (id == -1) return lbl_8035480C;
    return lbl_805F8200;
}

extern "C" {

// 0x801F8238
// Object init: base init, vtable, tuning floats at +0x3EC.., 26 slot floats cleared at +0x31C and
// set to -1 at +0x384, slot timers at +0x2B4 set to 15.0 and decay floats at +0x1E4 set to 0.2.
void fn_801F8238(u8* arg0, s32 arg1) {
    fn_801FA7CC(arg0, arg1);
    *(void**)arg0 = lbl_80386320;
    *(s32*)(arg0 + 0x1E0) = 0;
    *(f32*)(arg0 + 0x3EC) = 0.001f;
    *(f32*)(arg0 + 0x3F0) = -1.0f;
    *(f32*)(arg0 + 0x3F4) = 0.05f;
    *(f32*)(arg0 + 0x3F8) = 0.0f;
    *(f32*)(arg0 + 0x3FC) = 0.0f;
    *(f32*)(arg0 + 0x400) = 0.0f;
    *(f32*)(arg0 + 0x404) = 1.0f;
    *(f32*)(arg0 + 0x408) = 0.0f;
    *(f32*)(arg0 + 0x474) = -1.0f;
    for (s32 i = 0; i < 26; i++) {
        *(f32*)(arg0 + 0x31C + i * 4) = 0.0f;
        *(f32*)(arg0 + 0x384 + i * 4) = -1.0f;
    }
    f32 zx = 0.0f, zy = 0.0f, zz = 0.0f;  // sp20..sp28
    f32 ox = 0.0f, oy = 0.0f, oz = 0.0f;  // sp8..sp10
    s32 allZero = 0;
    if (ox == ox && oy == oy && oz == oz) allZero = 1;
    if (allZero != 0) {
        for (s32 i = 0; i < 26; i++) *(f32*)(arg0 + 0x2B4 + i * 4) = 15.0f;
    } else {
        s32 n = *(s32*)(arg0 + 0x1E0);
        for (s32 i = 0; i < n; i++) {
            u8* e = arg0 + i * 0xC;
            if (zx == *(f32*)(e + 0xA8) && zy == *(f32*)(e + 0xAC) && zz == *(f32*)(e + 0xB0)) {
                *(f32*)(arg0 + 0x2B4 + i * 4) = 15.0f;
            }
        }
    }
    for (s32 i = 0; i < 52; i++) *(f32*)(arg0 + 0x1E4 + i * 4) = 0.2f;
    *(f32*)(arg0 + 0x9C) = 0.0f;
    *(f32*)(arg0 + 0xA0) = -1.0f;
    *(f32*)(arg0 + 0xA4) = 0.0f;
}

// 0x80043538
// Battle callback: swaps the part in a fighter's slot (record at +0xD4: side +4, slot +8, def +0xC)
// for the given definition, rebuilds its battle object, flags it and resets the battle.
void fn_80043538(u8* arg0) {
    u8* rec = *(u8**)(arg0 + 0xD4);
    s32 idx = *(s32*)(rec + 0x8);
    u8* fighter = *(u8**)(gBattle + *(s32*)(rec + 0x4) * 4 + 0xA00);
    u8* ctx = *(u8**)(fighter + 0x1C);
    PartInstance* part = **(PartInstance***)(fighter + idx * 4);
    if (ctx != NULL) ctx -= 0xC48;
    u8* slot = *(u8**)(fighter + idx * 4);
    u8* obj;
    if (slot != NULL) obj = *(u8**)(slot + 0x20);
    else obj = NULL;
    if (obj != NULL) obj -= 0x264;
    fn_8004C718(ctx, idx, NULL);
    fn_8003DCE0(obj);
    PartSetDef(part, *(s32*)(rec + 0xC));
    fn_8004C718(ctx, *(s32*)(rec + 0x8),
                fn_8003DFCC(part, *(s32*)(rec + 0x8), *(s32*)(rec + 0xC), 0, *(s32*)(rec + 0x4)));
    u8* o = *(u8**)(ctx + *(s32*)(rec + 0x8) * 4 + 0xC50);
    ObjSetFlags(o, 2, 2, true);
    u8* c = *(u8**)(*(u8**)(ctx + *(s32*)(rec + 0x8) * 4 + 0xC50) + 0x4);
    *(u32*)(c + 0x20) |= 0x8000;
    BattleReset(gBattle);
}

// 0x800436A0
// Battle callback: toggles lbl_805FB070[side], sets the fighter's owner from lbl_803C51D8
// (entry side*2+1 when set, side*2 when cleared), points the +0xA54 entry at the matching pointer
// table and flags every effect record of kind 2.
void fn_800436A0(u8* arg0) {
    u8* rec = *(u8**)(arg0 + 0xD4);
    s32 side = *(s32*)(rec + 0x4);
    u8* fighter = *(u8**)(gBattle + side * 4 + 0xA00);
    lbl_805FB070[side] = (lbl_805FB070[side] == 0);
    side = *(s32*)(rec + 0x4);
    if (lbl_805FB070[side] != 0) {
        FighterStatsSetOwner(fighter + 0x20, (u32)lbl_803C51D8[side * 2 + 1]);
        *(s32*)(gBattle + *(s32*)(rec + 0x4) * 4 + 0xA54) = 0;
        s32 s = *(s32*)(rec + 0x4);
        *(u8**)(gBattle + s * 4 + 0xA54) = lbl_805FB080[s];
    } else {
        FighterStatsSetOwner(fighter + 0x20, (u32)lbl_803C51D8[side * 2]);
        *(s32*)(gBattle + *(s32*)(rec + 0x4) * 4 + 0xA54) = 0;
        s32 s = *(s32*)(rec + 0x4);
        *(u8**)(gBattle + s * 4 + 0xA54) = lbl_805FB078[s];
    }
    s32 count = *(s32*)(gBattle + 0xA30);
    for (s32 i = 0; i < count; i++) {
        u8* e;
        if (i >= *(s32*)(gBattle + 0xA30)) e = NULL;
        else e = *(u8**)(gBattle + i * 4 + 0xA1C);
        if (*(s32*)(e + 0x8) == 2) *(u8*)(e + 0xC) = 1;
    }
}

// 0x80043964
// Battle callback: shows the name of the selected entry. The record (+0xD4) holds the kind at +0x18
// (1 = part action table +0x3610, -1 = table +0x31E8, 0 = none) and the indices at +0x10/+0x14;
// when the object at +0 is not the current one the selection is refreshed from the fighter status
// (+0x28 -> +0x20/+0x24). Formats " %s" with the name ("None" when nothing selected).
void fn_80043964(u8* arg0) {
    char name[0x34];
    char text[0x90];
    s32 sel = -1;
    u8* rec = *(u8**)(arg0 + 0xD4);
    switch (*(s32*)(rec + 0x18)) {
    case -1:
        sel = *(s32*)(rec + 0x14);
        break;
    case 0:
        sel = -1;
        break;
    case 1:
        sel = *(s32*)(rec + 0x10);
        break;
    }
    if (fn_801B3938(*(s32*)(*(u8**)(rec + 0x0) + 0x260)) != arg0) {
        u8* fighter = *(u8**)(*(u8**)(gBattle + *(s32*)(rec + 0x4) * 4 + 0xA00) + *(s32*)(rec + 0x8) * 4);
        if (fighter != NULL) {
            u8* status = *(u8**)(fighter + 0x28);
            sel = *(s32*)(status + 0x20);
            s32 other = *(s32*)(status + 0x24);
            if (sel != -1) {
                *(s32*)(rec + 0x10) = sel;
                *(s32*)(rec + 0x14) = -1;
                *(s32*)(rec + 0x18) = 1;
            } else if (other != -1) {
                sel = other;
                *(s32*)(rec + 0x10) = -1;
                *(s32*)(rec + 0x14) = other;
                *(s32*)(rec + 0x18) = -1;
            } else {
                *(s32*)(rec + 0x10) = -1;
                sel = -1;
                *(s32*)(rec + 0x14) = -1;
                *(s32*)(rec + 0x18) = 0;
            }
        }
    }
    name[0] = 0;
    if (sel > -1) {
        s32 kind = *(s32*)(rec + 0x18);
        if (kind == 1) {
            u8* e = *(u8**)(gGameDb->data + 0x3610) + sel * 0x24;
            SysSprintf(name, "%s", LookupText(TextIdFromHash(*(u32*)(e + 0x4))));
        } else if (kind == -1) {
            u8* e = *(u8**)(gGameDb->data + 0x31E8) + (sel << 5);
            SysSprintf(name, "%s", LookupText(TextIdFromHash(*(u32*)(e + 0x4))));
        }
    } else {
        strcpy(name, "None");
    }
    SysSprintf(text, " %s", name);
    fn_801B4308(arg0, text);
}

// 0x80043E94
// Battle callback: generates an opponent into the battle context (+0x898), then for each of the five
// slots whose object accepts the generated entry (vtable slot 0xCC) rebuilds the slot object.
void fn_80043E94(u8* arg0) {
    u8* rec = *(u8**)(arg0 + 0xD4);
    u8* fighter = *(u8**)(gBattle + *(s32*)(rec + 0x0) * 4 + 0xA00);
    u8* ctx = *(u8**)(fighter + 0x1C);
    if (ctx != NULL) ctx -= 0xC48;
    MonsterGenerateOpponent(ctx + 0x898, *(s32*)(rec + 0x4));
    for (s32 i = 0; i < 5; i++) {
        u8* slot = *(u8**)(fighter + i * 4);
        u8* obj;
        if (slot != NULL) obj = *(u8**)(slot + 0x20);
        else obj = NULL;
        if (obj != NULL) obj -= 0x264;
        if (obj != NULL) {
            u8* gen = *(u8**)(ctx + i * 4 + 0x89C);
            s32 def = *(s32*)gen;
            if ((*(CanUseFn*)(*(u8**)obj + 0xCC))(obj, gen) != 0) {
                u8* part = *(u8**)(ctx + i * 4 + 0x89C);
                fn_8004C718(ctx, i, NULL);
                fn_8003DCE0(obj);
                fn_8004C718(ctx, i, fn_8003DFCC(part, i, def, 0, *(s32*)(rec + 0x0)));
                u8* o = *(u8**)(ctx + i * 4 + 0xC50);
                ObjSetFlags(o, 2, 2, true);
                u8* c = *(u8**)(*(u8**)(ctx + i * 4 + 0xC50) + 0x4);
                *(u32*)(c + 0x20) |= 0x8000;
            }
        }
    }
    BattleReset(gBattle);
}

// 0x800A11F4
// Scans the zone's saw-path objects (count +0x98, list +0x12C) and tells fn_800A3CAC the difficulty
// of each recognised path name: 0 = easy ("saw_path_NN_eA"), 1 = medium (mA), 2 = hard (hA).
void fn_800A11F4(u8* arg0) {
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
    for (s32 i = 0; i < *(s32*)(zone + 0x98); i++) {
        u8* obj = *(u8**)(*(u8**)(zone + 0x12C) + i * 4);
        u8* info = *(u8**)(obj + 0x9C);
        if (info != NULL) {
            u32 hash = *(u32*)(info + 0x8);
            switch (hash) {
            case 0x60412C4Bu:  // "saw_path_01_eA"
            case 0x72F483A5u:  // "saw_path_02_eA"
            case 0xCA48E4C0u:  // "saw_path_03_eA"
            case 0x579FDC79u:  // "saw_path_04_eA"
            case 0xEF23BB1Cu:  // "saw_path_05_eA"
            case 0xFD9614F2u:  // "saw_path_06_eA"
            case 0x452A7397u:  // "saw_path_07_eA"
            case 0x1D4963C1u:  // "saw_path_08_eA"
            case 0xA5F504A4u:  // "saw_path_09_eA"
            case 0xE59D629Eu:  // "saw_path_10_eA"
            case 0x5D2105FBu:  // "saw_path_11_eA"
            case 0x4F94AA15u:  // "saw_path_12_eA"
            case 0xF728CD70u:  // "saw_path_13_eA"
            case 0x6AFFF5C9u:  // "saw_path_14_eA"
            case 0xD24392ACu:  // "saw_path_15_eA"
            case 0xC0F63D42u:  // "saw_path_16_eA"
            case 0x784A5A27u:  // "saw_path_17_eA"
            case 0x20294A71u:  // "saw_path_18_eA"
            case 0x98952D14u:  // "saw_path_19_eA"
            case 0xA23D184Eu:  // "saw_path_20_eA"
                fn_800A3CAC(obj, arg0 + 0xCD8, 0);
                break;
            case 0xA898A643u:  // "saw_path_01_mA"
            case 0xBA2D09ADu:  // "saw_path_02_mA"
            case 0x02916EC8u:  // "saw_path_03_mA"
            case 0x9F465671u:  // "saw_path_04_mA"
            case 0x27FA3114u:  // "saw_path_05_mA"
            case 0x354F9EFAu:  // "saw_path_06_mA"
            case 0x8DF3F99Fu:  // "saw_path_07_mA"
            case 0xD590E9C9u:  // "saw_path_08_mA"
            case 0x6D2C8EACu:  // "saw_path_09_mA"
            case 0x2D44E896u:  // "saw_path_10_mA"
            case 0x95F88FF3u:  // "saw_path_11_mA"
            case 0x874D201Du:  // "saw_path_12_mA"
            case 0x3FF14778u:  // "saw_path_13_mA"
            case 0xA2267FC1u:  // "saw_path_14_mA"
            case 0x1A9A18A4u:  // "saw_path_15_mA"
            case 0x082FB74Au:  // "saw_path_16_mA"
            case 0xB093D02Fu:  // "saw_path_17_mA"
            case 0xE8F0C079u:  // "saw_path_18_mA"
            case 0x504CA71Cu:  // "saw_path_19_mA"
            case 0x6AE49246u:  // "saw_path_20_mA"
                fn_800A3CAC(obj, arg0 + 0xCD8, 1);
                break;
            case 0xD5EF5206u:  // "saw_path_01_hA"
            case 0xC75AFDE8u:  // "saw_path_02_hA"
            case 0x7FE69A8Du:  // "saw_path_03_hA"
            case 0xE231A234u:  // "saw_path_04_hA"
            case 0x5A8DC551u:  // "saw_path_05_hA"
            case 0x48386ABFu:  // "saw_path_06_hA"
            case 0xF0840DDAu:  // "saw_path_07_hA"
            case 0xA8E71D8Cu:  // "saw_path_08_hA"
            case 0x105B7AE9u:  // "saw_path_09_hA"
            case 0x50331CD3u:  // "saw_path_10_hA"
            case 0xE88F7BB6u:  // "saw_path_11_hA"
            case 0xFA3AD458u:  // "saw_path_12_hA"
            case 0x4286B33Du:  // "saw_path_13_hA"
            case 0xDF518B84u:  // "saw_path_14_hA"
            case 0x67EDECE1u:  // "saw_path_15_hA"
            case 0x7558430Fu:  // "saw_path_16_hA"
            case 0xCDE4246Au:  // "saw_path_17_hA"
            case 0x9587343Cu:  // "saw_path_18_hA"
            case 0x2D3B5359u:  // "saw_path_19_hA"
            case 0x17936603u:  // "saw_path_20_hA"
                fn_800A3CAC(obj, arg0 + 0xCD8, 2);
                break;
            }
        }
    }
}

// 0x800A5828
// Recursive tree copy (std::map-style): clones the child of src on the given side (1 = +0, 0 = +4)
// under dst, copying the 8-byte value (+0xC) and the colour bit (+8 bit 0), then recurses into
// both children of the source node.
void fn_800A5828(u8* arg0, u8* arg1, u8* arg2, s32 arg3) {
    u8* node = (u8*)fn_801D40E0(0x14);
    if (node == NULL) {
        fn_80336BF8(__files + 0xA0, "Memory allocation failure");
        fn_8033B09C();
    }
    *(s32*)(node + 0x4) = 0;
    *(s32*)(node + 0x0) = 0;
    u8* guard = node;
    *(u8**)(node + 0x8) = (u8*)((uintptr_t)arg1 | (*(u32*)(node + 0x8) & 1));
    u8* parent;
    u8* src;
    if (arg3 != 0) {
        u8* from = *(u8**)(arg2 + 0x0);
        *(s32*)(node + 0xC) = *(s32*)(from + 0xC);
        *(s32*)(node + 0x10) = *(s32*)(from + 0x10);
        *(u8**)(arg0 + 0x8) = guard;
        *(u8**)(arg1 + 0x0) = guard;
        parent = guard;
        guard = NULL;
        src = *(u8**)(arg2 + 0x0);
    } else {
        u8* from = *(u8**)(arg2 + 0x4);
        *(s32*)(node + 0xC) = *(s32*)(from + 0xC);
        *(s32*)(node + 0x10) = *(s32*)(from + 0x10);
        *(u8**)(arg1 + 0x4) = guard;
        parent = guard;
        guard = NULL;
        src = *(u8**)(arg2 + 0x4);
    }
    if (*(u32*)(src + 0x8) & 1) *(u32*)(parent + 0x8) |= 1;
    else *(u32*)(parent + 0x8) &= ~1u;
    *(s32*)(arg0 + 0x0) = *(s32*)(arg0 + 0x0) + 1;
    if (*(s32*)(src + 0x4) != 0) fn_800A5828(arg0, parent, src, 0);
    if (*(s32*)(src + 0x0) != 0) fn_800A5828(arg0, parent, src, 1);
    if (guard != NULL) operator_delete(guard);
}

// 0x800B8E8C
// Attaches an object to slot arg1 (+0x264 table): looks up the bone-like entry (hash 0x2974C997,
// entries of 0xC8 bytes at +0x98), transforms the zero vector by its matrix (+8..+0x34), hides the
// object and hands the position to fn_801B5510.
void fn_800B8E8C(u8* arg0, s32 arg1, u8* arg2) {
    u8* entry = *(u8**)(arg0 + 0x98) + fn_801EC880(arg0 + 0x94, 0x2974C997u) * 0xC8;
    f32 z = lbl_805FE208;
    s32 key = *(s32*)(entry + 0x4);
    *(u8**)(arg0 + arg1 * 4 + 0x264) = arg2;
    f32 px = *(f32*)(entry + 0x2C) + (*(f32*)(entry + 0x20) * z + (*(f32*)(entry + 0x8) * z + *(f32*)(entry + 0x14) * z));
    f32 py = *(f32*)(entry + 0x30) + (*(f32*)(entry + 0x24) * z + (*(f32*)(entry + 0xC) * z + *(f32*)(entry + 0x18) * z));
    f32 pz = *(f32*)(entry + 0x34) + (*(f32*)(entry + 0x28) * z + (*(f32*)(entry + 0x10) * z + *(f32*)(entry + 0x1C) * z));
    ObjSetFlags(arg2, 2, 2, false);
    f32 pos[3];
    pos[0] = px;
    pos[1] = py;
    pos[2] = pz;
    fn_801B5510(arg0, arg2, key, pos, 1);
}

// 0x800B9548
// Switches the active slot (+0x278): hides the old slot's object (table at +0x264) and shows the new one.
void fn_800B9548(u8* arg0, s32 arg1) {
    s32 old = *(s32*)(arg0 + 0x278);
    if (old != -1) {
        u8* o = *(u8**)(arg0 + old * 4 + 0x264);
        ObjSetFlags(o, 2, 2, false);
    }
    *(s32*)(arg0 + 0x278) = arg1;
    if (arg1 != -1) {
        u8* o = *(u8**)(arg0 + arg1 * 4 + 0x264);
        ObjSetFlags(o, 2, 2, true);
    }
}

// 0x800B9654
// Orbit-camera style attachment: builds the rotation quat at +0x27C from the euler angles of the
// target block (+0x270, scaled by 2/pi), spins it about the axis of the quat at +0x28C, places the
// child (+4) at target position + axis * distance (+0x274), then re-aims the quat and copies it into the child.
void fn_800B9654(u8* arg0, f32 farg0) {
    f32 z = lbl_805FE208;
    f32 one = lbl_805FE20C;
    f32 k = lbl_805FE228;
    *(f32*)(arg0 + 0x2A0) = farg0;
    u8* target = *(u8**)(arg0 + 0x270);
    *(f32*)(arg0 + 0x29C) = farg0;
    *(f32*)(arg0 + 0x27C) = z;
    *(f32*)(arg0 + 0x280) = z;
    *(f32*)(arg0 + 0x284) = z;
    *(f32*)(arg0 + 0x288) = one;
    Vec3 euler;
    euler.x = *(f32*)(target + 0x14) * k;
    euler.y = *(f32*)(target + 0x18) * k;
    euler.z = *(f32*)(target + 0x1C) * k;
    EulerToQuat((Quat*)(arg0 + 0x27C), &euler);
    Vec3 spinAxis = QuatGetAxisY((Quat*)(arg0 + 0x28C));
    fn_80146180(arg0 + 0x27C, &spinAxis, *(f32*)(arg0 + 0x29C));
    u8* base = *(u8**)(arg0 + 0x270);
    Vec3 dir = QuatGetAxisY((Quat*)(arg0 + 0x27C));
    f32 dist = *(f32*)(arg0 + 0x274);
    f32 dx = dir.x * dist;
    f32 dy = dir.y * dist;
    f32 dz = dir.z * dist;
    u8* child = *(u8**)(arg0 + 0x4);
    *(f32*)(child + 0x0) = dx + *(f32*)(base + 0x8);
    *(f32*)(child + 0x4) = dy + *(f32*)(base + 0xC);
    *(f32*)(child + 0x8) = dz + *(f32*)(base + 0x10);
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    Vec3 n = Vec3Normalized(QuatGetAxisY((Quat*)(arg0 + 0x27C)));
    Vec3 c;
    c.x = n.y * z - n.z * z;
    c.y = n.z * one - n.x * z;
    c.z = n.x * z - n.y * one;
    fn_801458B0((Quat*)(arg0 + 0x27C), &c, &n);
    u8* t = *(u8**)(arg0 + 0x4);
    *(f32*)(t + 0xC) = *(f32*)(arg0 + 0x27C);
    *(f32*)(t + 0x10) = *(f32*)(arg0 + 0x280);
    *(f32*)(t + 0x14) = *(f32*)(arg0 + 0x284);
    *(f32*)(t + 0x18) = *(f32*)(arg0 + 0x288);
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
}

}  // extern "C"
