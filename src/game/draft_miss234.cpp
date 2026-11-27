// Missing functions batch 234: experiment/lab UI refresh (fn_800226F0), world object slot setup
// (fn_800A81A8), palette classification (fn_80079440), axis name lookup (fn_800D2D38) and the
// combat HUD action name label (fn_80048CB4).
#include "types.h"
#include "game/part.h"
#include "engine/math.h"

struct StringTable;
struct ScopedLock { u8 pad[4]; };  // as in props.cpp

typedef void (*VObjFn)(void* obj, s32 a);   // vtable +0x18 of a world object

extern "C" {
#include "game/script_callbacks.h"

void* ChoiceDestroy(void* c, s32 del);                          // 0x8020F394
s32   RoundGetAttacker(void* round);                            // 0x80214F48
void  ScopedLockCtor(ScopedLock* l);                            // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                 // 0x801F53B0
void* ScreenSetFindElement(void* set, u32 hash);                // 0x80198F18
void* ScreenSetsCurrent(void* sets);                            // 0x8018D1E4
int   SysSprintf(char* buf, const char* fmt, ...);              // 0x80142EFC
void  UISetNumber(void* el, s32 n);                             // 0x8019F2A8
void  UISetText(void* el, const char* text, s32 a);             // 0x8019EFC8
s32   UISetTexture(u8* el, void* tex);                          // 0x801A11FC
void  fn_80041810(u8* dst, u8* src);                            // copy a choice (action, limb flag, limb)
u8*   fn_801186FC(u8* rec);                                     // 0x801186FC
void* fn_8011AD88(void* def, s32 flag);                         // 0x8011AD88
u8*   fn_8015A210(u8* p, s32 a, s32* out, s32 b);               // 0x8015A210
u8*   fn_8015A340(u8* p, s32 a);                                // 0x8015A340
s32   fn_80214F50(u8* p);                                       // 0x80214F50
s32   fn_80215274(u8* round, s32 side, s32 i);                  // 0x80215274
s32   fn_8021529C(u8* round, s32 side, s32 i);                  // 0x8021529C
u8*   fn_80218B8C(void* lab, s32 a);                            // 0x80218B8C
void  fn_800AB028(u8* obj);                                     // 0x800AB028
void  fn_800ABFA8(u8* a, u8* b);                                // 0x800ABFA8
void  fn_800ACDE0(u8* obj, u32 h0, u32 h1, u32 h2, s32 idx);    // 0x800ACDE0
s32   fn_800AD130(u8* a, u8* b);                                // 0x800AD130
void  fn_800AD9C4(u8* a, u8* b);                                // 0x800AD9C4
void  fn_800AF6A0(u8* obj, f32* pos);                           // 0x800AF6A0
s32   fn_800AFA54(u8* a, u8* b);                                // 0x800AFA54
void  fn_800B07F0(u8* obj, f32* pos);                           // 0x800B07F0
s32   fn_800B0CB4(u8* p, u8* value);                            // 0x800B0CB4
void  fn_800B154C(u8* p, u8* value);                            // 0x800B154C
void  fn_800B1F80(u8* p, u8* value);                            // 0x800B1F80

extern u32 lbl_805F8628[];      // 0x805F8628 (HUD element hashes, one per side)
extern u8* lbl_805FB134;        // 0x805FB134 (experiment object)
}

void  fn_8015A2E8(u8* p, u32 unused4, u32 flush);               // 0x8015A2E8 (C++ linkage)
void  fn_8015A368(u8* p, u32 flush);                            // 0x8015A368 (C++ linkage)

extern u8 gWorldState[];                   // 0x803E6D00 (first word: pointer to the world block)
extern u8* gBattle;                        // 0x805FBBB8
extern void* gLab;                         // 0x805FBBC8
extern "C++" void* gScreenSets;            // 0x805FB654
extern "C++" StringTable* gStringTable;    // 0x805FB910
extern "C++" void* gStringTable2;          // 0x805FB90C
extern "C++" s32 gZoneResourceSize;        // 0x805FB94C

// Localized string for a text hash (as inlined in the HUD callbacks): "Not found" when the hash is
// unknown, otherwise the entry of the matching string table (empty string when out of range).
static const char* TextFromHash(u32 hash) {
    s32 id = TextIdFromHash(hash);
    if (id == -1) return "Not found";
    if ((id & 0x80000000) && gStringTable2 != NULL) {
        const char** arr = *(const char***)((u8*)gStringTable2 + 0x0);
        if (arr != NULL && ((u32)id & 0x7FFFFFFFu) < *(u32*)((u8*)gStringTable2 + 0xC)) {
            return arr[(u32)id & 0x7FFFFFFFu];
        }
        return "";
    }
    const char** arr = *(const char***)((u8*)gStringTable + 0x0);
    if (arr != NULL && (u32)id < *(u32*)((u8*)gStringTable + 0xC)) return arr[id];
    if (id == -1) return "NOT FOUND";
    return "";
}

struct Rec4C { u32 w[19]; };   // 0x4C-byte db record, copied by value

extern "C" {

// 0x800226F0
// Refreshes the experiment/lab info screen: money, the name and amount of the selected item (an
// item record, or a part when no item is selected) and its icon texture.
void fn_800226F0(void) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    u8* exp = lbl_805FB134;
    if (exp != NULL && *(s32*)exp != 0) exp = NULL;
    Rec4C rec = *(Rec4C*)(*(u8**)(gGameDb->data + 0x80E0) + *(s32*)(exp + 0x7C) * 0x4C);
    if (set == NULL) return;
    u8* elMoney = (u8*)ScreenSetFindElement(set, 0xD74249AFu);
    u8* elTitle = (u8*)ScreenSetFindElement(set, 0x7D652CB9u);
    u8* elCount = (u8*)ScreenSetFindElement(set, 0x52D7C1EFu);
    if (elMoney != NULL) UISetNumber(elMoney, *(s32*)((u8*)gLab + 0x78));
    if (elTitle != NULL) UISetText(elTitle, TextFromHash(rec.w[1]), 1);
    u8* elName = (u8*)ScreenSetFindElement(set, 0xA25EA537u);
    u8* elIcon = (u8*)ScreenSetFindElement(set, 0x99E9F2EAu);
    if (*(s32*)((u8*)gLab + 0x8) != 0) {
        u8* sel = fn_80218B8C(gLab, 0);
        u8 amount = *(u8*)(sel + 0x8);
        if (elName != NULL) {
            const char* text;
            switch (*(s32*)sel) {
            case 0: {
                u8* e = *(u8**)(gGameDb->data + 0x4F00) + *(s32*)(sel + 0x4) * 0x10;
                text = TextFromHash(*(u32*)(e + 0x4));
                break;
            }
            case 1: {
                u8* e = *(u8**)(gGameDb->data + 0x5328) + *(s32*)(sel + 0x4) * 0x18;
                text = TextFromHash(*(u32*)(e + 0x4));
                break;
            }
            default:
                text = NULL;
                break;
            }
            if (text != NULL) UISetText(elName, text, 1);
            if (elCount != NULL) UISetNumber(elCount, amount);
        }
        if (elIcon != NULL) {
            u8* tex = fn_801186FC(sel);
            *(u8**)(exp + 0x84) = tex;
            if (tex != NULL) UISetTexture(elIcon, tex);
        }
    } else {
        s32 idx = *(s32*)((u8*)gLab + 0x58);
        if (idx != -1) {
            u8* e = *(u8**)(gGameDb->data + 0x6C18) + idx * 0x24;
            PartDef tmp;
            PartDbGetDef(&tmp, gGameDb, *(s32*)(e + 0x20));
            PartDef def = tmp;
            if (elName != NULL) UISetText(elName, TextFromHash(def.tierName[0]), 1);
            if (elIcon != NULL) {
                void* tex = fn_8011AD88(&def, 0);
                *(void**)(exp + 0x84) = tex;
                if (tex != NULL) UISetTexture(elIcon, tex);
            }
        }
    }
}

// 0x800A81A8
// Sorts the world object's loaded props into the controller's slots by the hash at +0x9C->+8
// (each one is flagged 0x400000 and sent through its setup call), then wires the groups together.
void fn_800A81A8(u8* p) {
    u8* sub = p;      // +0xB8.. group
    u8* grp2 = p;     // +0xE0.. group
    u8* grp3 = p;     // +0x118.. group
    u8* grp4 = p;     // +0x134.. group
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
    s32 total = *(s32*)(zone + 0x94) + *(s32*)(zone + 0x98);
    s32 recOff = 0;
    s32 ptrOff = 0;
    for (s32 i = 0; i < total; i++) {
        zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
        s32 n0 = *(s32*)(zone + 0x94);
        u8* obj;
        u8* rec;
        if (i < n0) {
            obj = *(u8**)(*(u8**)(zone + 0x120) + ptrOff);
            rec = *(u8**)(zone + 0x8C) + recOff;
        } else {
            s32 j = i - n0;
            obj = *(u8**)(*(u8**)(zone + 0x12C) + j * 4);
            rec = *(u8**)(zone + 0x90) + (j << 6);
        }
        if ((*(s32*)(rec + 0x1C) & 0x40) && obj != NULL) {
            u8* info = *(u8**)(obj + 0x9C);
            s32 key = info != NULL ? *(s32*)(info + 0x8) : 0;
            switch (key) {
            case 0x7EABE956:
                *(u8**)(p + 0x90) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                break;
            case 0x0BE4259C:
                *(u8**)(p + 0x94) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                break;
            case 0x0C46F507:
                *(u8**)(p + 0x98) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                fn_800ACDE0(*(u8**)(p + 0x98), 0x6EA76A13u, 0x18DB6703u, 0x429F3B83u, 1);
                break;
            case 0x276BA6C4:
                *(u8**)(p + 0x9C) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                fn_800ACDE0(*(u8**)(p + 0x9C), 0xE83318BDu, 0x29337D9Eu, 0xDB966A39u, 2);
                break;
            case 0x3E709785:
                *(u8**)(p + 0xA0) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                fn_800ACDE0(*(u8**)(p + 0xA0), 0x236FCB18u, 0x8F44762Au, 0xAC915AAFu, 3);
                break;
            case 0x71310142:
                *(u8**)(p + 0xA4) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                fn_800ACDE0(*(u8**)(p + 0xA4), 0x3E6AFBA0u, 0x4AE348A4u, 0x32F5CF0Cu, 4);
                break;
            case 0x682A3003:
                *(u8**)(p + 0xA8) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                fn_800ACDE0(*(u8**)(p + 0xA8), 0xF5362805u, 0xEC944310u, 0x45F2FF9Au, 5);
                break;
            case 0x430763C0:
                *(u8**)(p + 0xAC) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                fn_800ACDE0(*(u8**)(p + 0xAC), 0x73A25AABu, 0xDD7C598Du, 0xDCFBAE20u, 6);
                break;
            case 0x6D3BDFAA: {
                f32 v[3];
                *(u8**)(p + 0xB0) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                v[0] = *(f32*)(p + 0x6D0);
                v[1] = *(f32*)(p + 0x6D4);
                v[2] = *(f32*)(p + 0x6D8);
                fn_800AF6A0(*(u8**)(p + 0xB0), v);
                break;
            }
            case (s32)0xBAC5C3F2: {
                f32 v[3];
                *(u8**)(p + 0xB4) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                v[0] = *(f32*)(p + 0x6D0);
                v[1] = *(f32*)(p + 0x6D4);
                v[2] = *(f32*)(p + 0x6D8);
                fn_800B07F0(*(u8**)(p + 0xB4), v);
                break;
            }
            case (s32)0x861FD133:
                *(u8**)(sub + 0xB8) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                fn_800AB028(*(u8**)(sub + 0xB8));
                sub += 4;
                break;
            case (s32)0xBA7D0996:
                *(u8**)(grp2 + 0xE0) = obj;
                grp2 += 4;
                *(u32*)(obj + 0x15C) |= 0x400000;
                break;
            case (s32)0xDCF3BBEC:
                *(u8**)(grp3 + 0x118) = obj;
                grp3 += 4;
                *(u32*)(obj + 0x15C) |= 0x400000;
                break;
            case (s32)0xF69C0A19:
                *(u8**)(grp4 + 0x134) = obj;
                grp4 += 4;
                *(u32*)(obj + 0x15C) |= 0x400000;
                break;
            default: {
                ScopedLock lock;
                *(u32*)(obj + 0x15C) |= 0x400000;
                ScopedLockCtor(&lock);
                (*(VObjFn*)(*(u8**)obj + 0x18))(obj, 1);
                ScopedLockDtor(&lock, -1);
                break;
            }
            }
        }
        recOff += 0x40;
        ptrOff += 4;
    }
    fn_800B154C(*(u8**)(p + 0x94), *(u8**)(p + 0x90));
    fn_800B1F80(*(u8**)(p + 0x90), *(u8**)(p + 0x94));
    for (s32 k = 0; k < 6; k++) {
        fn_800AD130(*(u8**)(p + 0x98 + k * 4), *(u8**)(p + 0x94));
    }
    for (s32 k = 0; k < 5; k++) {
        fn_800B0CB4(*(u8**)(p + 0xB4), *(u8**)(p + 0xB8 + k * 4));
    }
    for (s32 k = 5; k < 10; k++) {
        fn_800AFA54(*(u8**)(p + 0xB0), *(u8**)(p + 0x14 + 0xB8 + (k - 5) * 4));
    }
    u8* tmp[7];
    for (s32 k = 0; k < 7; k++) {
        fn_800AD9C4(*(u8**)(p + 0x118 + k * 4), *(u8**)(p + 0x134 + k * 4));
        tmp[k] = *(u8**)(p + 0x118 + k * 4);
    }
    for (s32 k = 0; k < 7; k++) {
        u8* o = tmp[k];
        switch (*(s32*)(o + 0x2C)) {
        case (s32)0xC151C7F3: *(u8**)(p + 0x118) = o; break;
        case 0x58589649:      *(u8**)(p + 0x11C) = o; break;
        case (s32)0xB88D7F57: *(u8**)(p + 0x120) = o; break;
        case 0x283262C6:      *(u8**)(p + 0x124) = o; break;
        case 0x5F355250:      *(u8**)(p + 0x128) = o; break;
        case (s32)0xC63C03EA: *(u8**)(p + 0x12C) = o; break;
        case (s32)0xB13B337C: *(u8**)(p + 0x130) = o; break;
        }
    }
    u8* q = p;
    for (s32 k = 0; k < 14; k++) {
        if (k < 7) {
            fn_800ABFA8(*(u8**)(q + 0xE0), *(u8**)(q + 0x118));
        } else {
            fn_800ABFA8(*(u8**)(q + 0xE0), NULL);
        }
        q += 4;
    }
}

}  // extern "C"

// One of the 16-bit palette entries is either RGB5 (bit 15 set) or RGB4A3. Sets / clears one color
// channel group of an entry: mask5 applies in RGB5 form, mask4 in RGB4A3 form.
// The palette is the console's own data (the graphics chip reads it): its entries stay big-endian
// in memory on every machine, so they are read and written a byte at a time.
static inline u16 PalGet(const u16* e) {
    const u8* b = (const u8*)e;
    return (u16)((b[0] << 8) | b[1]);
}
static inline void PalPut(u16* e, u16 v) {
    u8* b = (u8*)e;
    b[0] = (u8)(v >> 8);
    b[1] = (u8)v;
}
static inline void PalSet(u16* e, u16 mask5, u16 mask4) {
    u16 v = PalGet(e);
    if (((v >> 15) & 1) == 1) PalPut(e, (u16)(v | mask5));
    else PalPut(e, (u16)(v | mask4));
}
static inline void PalClear(u16* e, u16 mask5, u16 mask4) {
    u16 v = PalGet(e);
    if (((v >> 15) & 1) == 1) PalPut(e, (u16)(v & ~mask5));
    else PalPut(e, (u16)(v & ~mask4));
}
// Entry k of the corner palette: 'on' bits select which of R, G, B are saturated.
static inline void PalCorner(u16* e, bool r, bool g, bool b) {
    if (r) PalSet(e, 0x7C00, 0xF00); else PalClear(e, 0x7C00, 0xF00);
    if (g) PalSet(e, 0x03E0, 0x00F0); else PalClear(e, 0x03E0, 0x00F0);
    if (b) PalSet(e, 0x001F, 0x000F); else PalClear(e, 0x001F, 0x000F);
}

extern "C" {

// 0x80079440
// Classifies the colors of a 4-bit palette texture (0 = transparent/dark, 1 / 4 = red- or
// blue-dominant, 2 / 3 = green-dominant), overwrites the palette with the eight RGB cube corners,
// remaps every pixel's two nibbles through the class table and returns how many nibbles were class 0
// or 4.
s32 fn_80079440(u8* p) {
    u8 table[256];
    u16* pal = (u16*)fn_8015A340(p, 0);
    u32 v = (u32)*(u16*)p ^ 0x80;
    u32 neg = (u32)(((s32)v >> 1) - (s32)(v & 0x80)) >> 31;
    s32 n = *(u8*)(p + 0xD) * 8;
    for (s32 i = 0; i < n; i++) {
        u8 cls;
        bool done = false;
        u16 e = pal != NULL ? PalGet(&pal[i]) : 0;
        if (pal != NULL) {
            u8 alpha = ((e >> 15) & 1) ? 7 : (u8)((e >> 12) & 7);
            if (alpha == 0) {
                table[i] = 0;
                continue;
            }
            u8 g = ((e >> 15) & 1) == 1 ? (u8)((e >> 5) & 0x1F) : (u8)((e >> 4) & 0xF);
            if (g > 4) {
                u8 r = ((e >> 15) & 1) == 1 ? (u8)((e >> 10) & 0x1F) : (u8)((e >> 8) & 0xF);
                cls = r > 4 ? 3 : 2;
                done = true;
            }
        }
        if (!done) {
            // The original reads these from a NULL base (low memory) when there is no palette;
            // use 0 there.
            u8 r = 0;
            u8 b = 0;
            if (pal != NULL) {
                r = ((e >> 15) & 1) == 1 ? (u8)((e >> 10) & 0x1F) : (u8)((e >> 8) & 0xF);
                b = ((e >> 15) & 1) == 1 ? (u8)(e & 0x1F) : (u8)(e & 0xF);
            }
            if (r > 4 || b > 4) cls = r > b ? 1 : 4;
            else cls = 0;
        }
        table[i] = cls;
    }
    if (pal != NULL) {
        if (neg == 0) PalPut(&pal[0], (u16)(PalGet(&pal[0]) | 0x8000));
        else PalPut(&pal[0], (u16)(PalGet(&pal[0]) & 0x0FFF));
        PalClear(&pal[0], 0x7C00, 0xF00);
        PalClear(&pal[0], 0x03E0, 0x00F0);
        PalClear(&pal[0], 0x001F, 0x000F);
        for (s32 k = 1; k < 8; k++) {
            PalPut(&pal[k], (u16)(PalGet(&pal[k]) | 0x8000));
            PalCorner(&pal[k], (k & 1) != 0, (k & 2) != 0, (k & 4) != 0);
        }
    }
    fn_8015A368(p, 1);
    s32 count = 0;
    s32 stride;
    u8* pix = fn_8015A210(p, 0, &stride, 0);
    for (s32 y = 0; y < (s32)*(u16*)(p + 2); y++) {
        u8* row = pix + y * stride;
        for (s32 x = 0; x < (s32)(*(u16*)p >> 1); x++) {
            u8 b = *row;
            u8 hi = table[b >> 4];
            u8 lo = table[b & 0xF];
            if (hi == 0 || hi == 4) count++;
            if (lo == 0 || lo == 4) count++;
            *row = (u8)(((hi & 0xFF) << 4) | lo);
            row++;
        }
    }
    fn_8015A2E8(p, 0, 1);
    return count;
}

}  // extern "C"

// True when the three floats equal the normalized axis vector.
static bool DirIs(const f32* d, f32 x, f32 y, f32 z) {
    Vec3 n = Vec3Normalized(Vec3{ x, y, z });
    return d[0] == n.x && d[1] == n.y && d[2] == n.z;
}

extern "C" {

// 0x800D2D38
// Name of the axis direction a vector points along ("Down", "Up", "Left", "Right", "Forward",
// "Back"); anything else is formatted into the buffer at +0x200 as "x y z" and returned.
const char* fn_800D2D38(u8* p, f32* v) {
    if (DirIs(v, 0.0f, -1.0f, 0.0f)) return "Down";
    if (DirIs(v, 0.0f, 1.0f, 0.0f)) return "Up";
    if (DirIs(v, 1.0f, 0.0f, 0.0f)) return "Left";
    if (DirIs(v, -1.0f, 0.0f, 0.0f)) return "Right";
    if (DirIs(v, 0.0f, 0.0f, 1.0f)) return "Forward";
    if (DirIs(v, 0.0f, 0.0f, -1.0f)) return "Back";
    SysSprintf((char*)(p + 0x200), "%0.02f %0.02f %0.02f", v[0], v[1], v[2]);
    return (const char*)(p + 0x200);
}

// 0x80048CB4
// Fills the combat HUD action label of one side: the name of the selected action's item when one of
// its slots is usable (kind 5 or 7), otherwise a generic "no action" text; returns 1 when set.
s32 fn_80048CB4(s32 side) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL || *(u32*)set != 0x1B66AC94u) return 0;   // "UI_MSA_Combat_HUD"
    u8* el = (u8*)ScreenSetFindElement(set, lbl_805F8628[side]);
    if (el == NULL) return 0;
    u8* round = *(u8**)(gBattle + 0xA0C);
    u8 choice[12];
    fn_80041810(choice, *(u8**)(gBattle + 0xA00 + side * 4) + 0x20);
    s32 actionIdx = *(s32*)choice;
    ChoiceDestroy(choice, -1);
    u8* data = gGameDb->data;
    u8* selected = *(u8**)(gBattle + 0xA4C);
    u8* act = *(u8**)(data + 0x430) + actionIdx * 0x14;
    u8* type = *(u8**)(data + 0x8) + *(s32*)(act + 0x4) * 0x24;
    s32 statKey = *(s32*)(type + 0x8);
    u32 nameHash = *(u32*)(type + 0x10);
    if (selected != NULL && selected != *(u8**)(gBattle + 0xA00 + side * 4) && statKey != 5) return 0;
    for (s32 i = 0; i < 5; i++) {
        if (fn_80215274(round, side, i) != 0) {
            u8* rec = *(u8**)(data + 0x3610) + fn_8021529C(round, side, i) * 0x24;
            s32 kind = *(s32*)rec;
            if (kind == 7 || kind == 5) {
                UISetText(el, TextFromHash(*(u32*)(rec + 0x4)), 1);
                return 1;
            }
        }
    }
    s32 mode = *(s32*)round;
    if (((u32)(mode - 6) <= 1u && statKey != 3 && statKey != 5) || side == fn_80214F50(round)) {
        if (mode == 7) {
            if (statKey == 2) mode = 4;
            else if (statKey == 1) mode = 5;
        }
        s32 ok = 0;
        u8* ch = *(u8**)(gBattle + 0xA00 + RoundGetAttacker(round) * 4) + 0x20;
        u8 c2[12];
        fn_80041810(c2, ch);
        s32 has = fn_80215274(round, RoundGetAttacker(round), *(s32*)(c2 + 8));
        ChoiceDestroy(c2, -1);
        if (has != 0) {
            u8 c3[12];
            fn_80041810(c3, ch);
            u8* rec = *(u8**)(data + 0x3610) + fn_8021529C(round, RoundGetAttacker(round), *(s32*)(c3 + 8)) * 0x24;
            s32 kind = *(s32*)rec;
            ChoiceDestroy(c3, -1);
            if (kind != 5) ok = 1;
        }
        if (ok != 0) {
            u8 c4[12];
            fn_80041810(c4, ch);
            u8* rec = *(u8**)(data + 0x3610) + fn_8021529C(round, RoundGetAttacker(round), *(s32*)(c4 + 8)) * 0x24;
            u32 hash = *(u32*)(rec + 0x4);
            ChoiceDestroy(c4, -1);
            UISetText(el, TextFromHash(hash), 1);
            return 1;
        }
        switch (mode) {
        case 4: UIText_SetString_TextID(lbl_805F8628[side], 0x570u); return 1;
        case 5: UIText_SetString_TextID(lbl_805F8628[side], 0x56Fu); return 1;
        case 3: UIText_SetString_TextID(lbl_805F8628[side], 0x572u); return 1;
        default: return 0;
        }
    }
    if (side == RoundGetAttacker(round) && *(u8*)(round + RoundGetAttacker(round) + 0x74) == 0) {
        UISetText(el, TextFromHash(nameHash), 1);
        return 1;
    }
    return 0;
}

}  // extern "C"
