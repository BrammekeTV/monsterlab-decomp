// Missing functions batch 254: fullscreen texture quad, entity attach/reset (fn_80063D6C), Wii remote
// polling (fn_801479D0), combat rewards screen setup and the lab part info model loader.
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "revolution.h"
#include "game/part.h"
#include "engine/math.h"
#include "engine/rend.h"

struct StringTable;

extern "C" {
#include "game/script_callbacks.h"
}

extern "C++" {
extern void*        gLab;            // 0x805FBBC8
extern u8*          gBattle;         // 0x805FBBB8
extern void*        gScreenSets;     // 0x805FB654
extern StringTable* gStringTable;    // 0x805FB910
extern void*        gStringTable2;   // 0x805FB90C
extern s32          gScreenHeight;   // 0x805F938C
extern s32          gViewportIndex;  // 0x805FB4BC
extern s32          gFrameBudget;    // 0x805FB4C8
extern u8           sRendDLs[];      // 0x80401108 (display-list records, 0x94 bytes each)
}

extern "C" {
void*  ScreenSetFindElement(void* set, u32 hash);                    // 0x80198F18
void*  ScreenSetsCurrent(void* sets);                                // 0x8018D1E4
void   ScreenSetsActivate(void* sets, u32 hash);                     // 0x8018CD8C
int    SysSprintf(char* buf, const char* fmt, ...);                  // 0x80142EFC
void   UISetNumber(void* el, s32 n);                                 // 0x8019F2A8
void   UISetText(void* el, const char* text, s32 a);                 // 0x8019EFC8
s32    UISetTexture(u8* el, void* tex);                              // 0x801A11FC
u8**   FighterGetSlot(u8* f, s32 i);                                 // battle_core.cpp
BOOL   MonsterIsFullyHealed(u8* m);                                  // part_instance.cpp
BOOL   PartAllowedAtRank(void* coll, s32 defIndex, s32 tier);        // 0x802191CC
void   PartComputeRequiredRank(void* lab, s32 defIndex, s32 tierIndex);
BOOL   SavePartTierOwned(u8* s, u32 part, s32 tier);                 // 0x802241F0
void   SpawnEffectAt(void* fxMgr, u32 effect, const f32 posAndScale[4], u8** owner);   // 0x801C80E0
void*  HashTableFind(void* table, u32 hash);                         
BOOL   ZoneIsLoaded(void* world, s32 zone);
u32    AlertGetLastError(void);                                      // 0x80140B58
void   QuatGetForward(Vec3* out, Quat* q);
void*  DefectGetTexture(s32 defect);                                 // 0x80116A20
f32    fn_8033F1FC(f32 y, f32 x);                                    // 0x8033F1FC (atan2)
ScriptValue fn_801A1208(u8* el);                                     // 0x801A1208 (image element -> texture)

s32    VideoGetHeight(RendVideo* video);                             // 0x8015DE50
s32    VideoGetWidth(RendVideo* video);                              // 0x8015DE48

// entity attach helpers
void   fn_800493A8(s32 on);                                          // 0x800493A8
u8*    fn_8005CBC4(void);                                            // 0x8005CBC4
u8*    fn_80063388(u8* p);                                           // 0x80063388 (returns the lookup result)
void   fn_80063530(u8* a, const f32* v);                             // 0x80063530
void   fn_8006359C(u8* attachment, f32 duration);                    // 0x8006359C
u8*    fn_800633D4(u8* attachment);                                  // 0x800633D4
s32    fn_800637AC(u8* ent, u8* arg);                                // 0x800637AC
void   fn_80068B50(u8* p, s32 notify);                               // 0x80068B50
s32    fn_8012E420(u8* world, u8* obj, s32 a, s32 b);                // 0x8012E420
void   fn_801458B0(f32* quat, f32* a, f32* b);                       // 0x801458B0
void   fn_801B4EA4(u8* obj, u8* flag);                               // 0x801B4EA4
s32    fn_801C3E88(u8* tbl, u8* handle);                             // 0x801C3E88
s32    fn_801C3E94(u8* tbl);                                          // 0x801C3E94
void   fn_801C849C(u8* mgr, u8* item);                               // 0x801C849C
void   fn_801DA344(u8* obj, u8* prop);                               // 0x801DA344
void   fn_8020A398(s32 handle);                                      // 0x8020A398

// lab / part info
void*  fn_80109330(const char* name);                                // 0x80109330
u8*    fn_801186FC(u8* rec);                                         // 0x801186FC
void*  fn_8011AD88(void* def, s32 flag);                             // 0x8011AD88
s32    fn_8011C214(u8* p);                                           // 0x8011C214
s32    fn_80120390(u8* recipes);                                     // 0x80120390
u32    fn_801203C0(u8* p);                                           // 0x801203C0
s32    fn_80120504(u8* p);                                           // 0x80120504
u8*    fn_80218B8C(void* lab, s32 a);                                // 0x80218B8C

// gamepad polling
s32    fn_8027F504(s32 chan, u8* buf, s32 count);                    // 0x8027F504
s32    fn_802728A4(s32 chan, u32* out);                              // 0x802728A4
u8*    fn_801483BC(s32 idx);                                         // 0x801483BC

// GX / display list
void   fn_8014DE68(void);                                            // 0x8014DE68
void   fn_8014E3EC(s32 n);                                           // 0x8014E3EC
void   fn_801514F0(void);                                            // 0x801514F0
s32    fn_80151644(s32 a, s32 b, u8* quad);                          // 0x80151644
void   fn_80154290(s32 a);                                           // 0x80154290
void   fn_801542DC(void);                                            // 0x801542DC
void   fn_80154974(void);                                            // 0x80154974
void   fn_80155D70(void);                                            // 0x80155D70
void   fn_801566E8(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);        // 0x801566E8
s32    fn_8015BA30(void* p);                                         // 0x8015BA30

extern u8  gWorldState[];           // 0x803E6D00
extern u8  lbl_8047C760[];          // 0x8047C760 (particle effect manager)
extern u8  lbl_8047C7AC[];          // 0x8047C7AC (handle table)
extern u8  lbl_803589D8[];          // 0x803589D8 (combat rewards screen element hash table)
extern u8* lbl_803C5308[];          // 0x803C5308 (reward icon textures)
extern s32 lbl_80400FE0[4];         // 0x80400FE0 (per-channel controller state)
extern const char lbl_8036CF70[];   // 0x8036CF70 (texture path pieces)
extern const char lbl_805F866C[];   // 0x805F866C ("%d")
extern u8* lbl_805FB248;            // 0x805FB248 (lab books section table)
extern u8  lbl_805FB24D;            // 0x805FB24D
extern u8* lbl_805FB250;            // 0x805FB250 (model texture)
extern u8* lbl_805FB254;            // 0x805FB254 (enhancement / defect texture)
extern const char* lbl_805FB258;    // 0x805FB258 (enhancement / defect text)
extern u8* lbl_805FBBE0;            // 0x805FBBE0 (save block pointer)
extern u8  lbl_805F865D;            // 0x805F865D
extern u32 lbl_805FB0A0;            // 0x805FB0A0 (last disabled HUD input hash)
extern s32 lbl_805FB0B8;            // 0x805FB0B8
extern u8* lbl_805FB490;            // 0x805FB490 (current register table)
extern u8* lbl_805FB494;            // 0x805FB494 (spare register table)
extern s32 lbl_805FB4C4;            // 0x805FB4C4 (display-list capacity)
extern s32 lbl_805FB4CC;            // 0x805FB4CC
extern u8* lbl_805FB4D0;            // 0x805FB4D0 (display-list write cursor)
extern u8  lbl_805FB50C;            // 0x805FB50C (GX shadow flags byte)
extern s32 lbl_805FB514;            // 0x805FB514 (GX shadow word)
extern s32 lbl_805FB6AC;            // 0x805FB6AC (entity serial counter)
}

typedef u8* (*HashMethod)(u8* self, u32 hash);
typedef void (*ElementFn)(u8* el, s32 a);

static const u32 kHashDefault = 0xDC1BFEC5u;   // "default"
static const u32 kHashParts   = 0x9E14654Eu;   // "parts"
static const u32 kHashRecipes = 0x9C2C1CAFu;   // "recipes"
static const u32 kHashCombatHud = 0x1B66AC94u; // "UI_MSA_Combat_HUD"

// One BP register write (opcode 0x61 then the 32-bit word) through the write-gather pipe.
static void GxBpWrite(u32 value) {
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = value;
}

// One XF/CP load (opcode 0x10, two halfwords, one data word).
static void GxXfWrite(u16 count, u16 addr, u32 value) {
    GXWGFifo._u8  = 0x10;
    GXWGFifo._u16 = count;
    GXWGFifo._u16 = addr;
    GXWGFifo._u32 = value;
}

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

static inline BOOL NearZero3(f32 x, f32 y, f32 z) {
    return x <= 0.0001f && x >= -0.0001f &&
           y <= 0.0001f && y >= -0.0001f &&
           z <= 0.0001f && z >= -0.0001f;
}

static inline s32 ClampByte(s32 v) {
    if (v < 0) return 0;
    if (v > 0xFF) return 0xFF;
    return v;
}

// Raw bit pattern of a float.
static inline u32 FloatBits(f32 v) {
    u32 u;
    memcpy(&u, &v, sizeof(u));
    return u;
}

// Calls the virtual slot at byte offset `off` of obj with a hash argument.
static u8* CallVirtHash(u8* obj, s32 off, u32 hash) {
    HashMethod fn = *(HashMethod*)(*(u8**)obj + off);
    return fn(obj, hash);
}

// True when the lab books screen is on page `state` and that page answers to `hash` (vfunc 0x24).
static s32 PageActive(s32 state, u32 hash) {
    u8* top = lbl_805FB248;
    if (top == NULL) return 0;
    if (*(s32*)top != state) return 0;
    return CallVirtHash(*(u8**)(top + 4 + state * 4), 0x24, hash) != NULL;
}

// Sub-object at +off of the lab books table, queried for `hash` (vfunc 0x28), or NULL.
static u8* SubQuery(s32 off, u32 hash) {
    u8* top = lbl_805FB248;
    u8* sub = (top != NULL) ? *(u8**)(top + off) : NULL;
    if (sub == NULL) return NULL;
    return CallVirtHash(sub, 0x28, hash);
}

// Texture for the enhancement record `e` (0x24 bytes): builds "<prefix><formatted record>", falls back
// to "<prefix><default suffix>", and stores the enhancement's display text in lbl_805FB258.
static u8* EnhancementTexture(u8* e) {
    char name[0x100];
    char path[0x100];
    SysSprintf(name, lbl_8036CF70 + 0x18, (s32)*(s8*)(e + 0x20), *(s32*)(e + 0x18), *(s32*)(e + 0x14),
               *(s32*)(e + 0x10), *(s32*)(e + 0xC), *(s32*)(e + 0x8));
    strcpy(path, lbl_8036CF70);
    strcat(path, name);
    u8* tex = (u8*)fn_80109330(path);
    lbl_805FB258 = TextFromHash(*(u32*)(e + 0x8));
    if (tex == NULL) {
        strcpy(path, lbl_8036CF70);
        strcat(path, lbl_8036CF70 + 0x30);
        tex = (u8*)fn_80109330(path);
    }
    return tex;
}

extern "C" {

// 0x80011C90
// Draws one fullscreen textured quad: sets up the GX state, programs the BP registers for texture
// `tex` (or switches the texture stage off when NULL), reserves 0x60 bytes in the display list for
// the four vertices (corners -1..1, aspect-ratio U), and submits them.
void DrawFullscreenTexture(void* tex) {
    f32 col[4];

    fn_801542DC();
    fn_80155D70();
    fn_80154974();
    fn_801566E8(-1.0f, 1.0f, 1.0f, -1.0f, 0.0f, -1.0f);
    col[0] = 1.0f;
    col[1] = 1.0f;
    col[2] = 1.0f;
    col[3] = 1.0f;
    if (fn_8015BA30(sRendDLs + gViewportIndex * 0x94) + 0x1107 > gFrameBudget) {
        fn_8014DE68();
        fn_8015BA30(sRendDLs + gViewportIndex * 0x94);
    }
    if (lbl_805FB50C & 1) {
        u8 old = lbl_805FB50C;
        lbl_805FB50C = old & 0xFE;
        if (old & 2) {
            GxBpWrite(0xFE00000F);
            GxBpWrite(1);
            GxXfWrite(0, 0x103F, 1);
            GxBpWrite(0xFE003C00);
            GxBpWrite(0);
            GxXfWrite(0, 0x100E, 0x442);
            GxXfWrite(0, 0x1010, 0x442);
            GxBpWrite(0x28380040);
            GxBpWrite(0xC008F8AF);
            GxBpWrite(0xC108F2F0);
        } else {
            GxBpWrite(0xFE00000F);
            GxBpWrite(0);
            GxXfWrite(0, 0x103F, 0);
            GxBpWrite(0xFE003C00);
            GxBpWrite(0);
            GxXfWrite(0, 0x100E, 0x442);
            GxXfWrite(0, 0x1010, 0x442);
            GxBpWrite(0x28380000);
            GxBpWrite(0xC008FFFA);
            GxBpWrite(0xC108FFD0);
        }
    }
    // Constant color register (XF 0x100C) from the 0..1 color, clamped to bytes.
    s32 r = ClampByte((s32)(255.0f * col[0]));
    s32 g = ClampByte((s32)(255.0f * col[1]));
    s32 b = ClampByte((s32)(255.0f * col[2]));
    s32 a = ClampByte((s32)(255.0f * col[3]));
    GxXfWrite(0, 0x100C, (u32)((r << 24) | (g << 16) | (b << 8) | a));
    if (lbl_805FB50C & 4) {
        lbl_805FB50C = lbl_805FB50C & ~4;
        s32 v = lbl_805FB514 | 0x40;
        lbl_805FB514 = v;
        GxBpWrite((u32)v | 0x43000000u);
        GxBpWrite(0xF33F0000);
    }
    fn_801514F0();
    lbl_805FB50C = lbl_805FB50C & ~8;
    GxBpWrite(0xFE00C000);
    GxBpWrite(0);
    GxBpWrite(0xFE000001);
    GxBpWrite(0x41000000);
    fn_80154290(0);
    if (tex != NULL) {
        if (!(lbl_805FB50C & 2)) {
            u8 flags = lbl_805FB50C | 2;
            lbl_805FB50C = flags;
            if (flags & 1) {
                GxBpWrite(0xFE00000F);
                GxBpWrite(1);
                GxXfWrite(0, 0x103F, 1);
                GxBpWrite(0xFE003C00);
                GxBpWrite(0x400);
                GxXfWrite(0, 0x1010, 0x441);
                GxBpWrite(0x283C0000);
                GxBpWrite(0xC008F2A4);
                GxBpWrite(0xC108E6F0);
                GxBpWrite(0xC208F80F);
                GxBpWrite(0xC308F070);
            } else {
                GxBpWrite(0xFE00000F);
                GxBpWrite(1);
                GxXfWrite(0, 0x103F, 1);
                GxBpWrite(0xFE003C00);
                GxBpWrite(0);
                GxXfWrite(0, 0x100E, 0x442);
                GxXfWrite(0, 0x1010, 0x442);
                GxBpWrite(0x28380040);
                GxBpWrite(0xC008F8AF);
                GxBpWrite(0xC108F2F0);
            }
        }
    } else if (lbl_805FB50C & 2) {
        u8 old = lbl_805FB50C;
        lbl_805FB50C = old & ~2;
        if (old & 1) {
            GxBpWrite(0xFE00000F);
            GxBpWrite(0);
            GxXfWrite(0, 0x103F, 0);
            GxBpWrite(0xFE003C00);
            GxBpWrite(0);
            GxXfWrite(0, 0x1010, 0x441);
            GxBpWrite(0x28380000);
            GxBpWrite(0xC008F2A4);
            GxBpWrite(0xC108E6F0);
        } else {
            GxBpWrite(0xFE00000F);
            GxBpWrite(0);
            GxXfWrite(0, 0x103F, 0);
            GxBpWrite(0xFE003C00);
            GxBpWrite(0);
            GxXfWrite(0, 0x100E, 0x442);
            GxXfWrite(0, 0x1010, 0x442);
            GxBpWrite(0x28380000);
            GxBpWrite(0xC008FFFA);
            GxBpWrite(0xC108FFD0);
        }
    }
    if (lbl_805FB4CC < 0x60) {
        fn_8014E3EC(0x60);
    }
    s32 cap = lbl_805FB4C4;
    s32 left = lbl_805FB4CC;
    u8* base = lbl_805FB4D0;
    lbl_805FB4C4 = cap + 0x60;
    u8* p = base + cap;
    lbl_805FB4CC = left - 0x60;

    // Vertex 0: u, v, color, x, y, z
    *(f32*)(p + 0x0)  = 0.0f;
    *(f32*)(p + 0x4)  = 1.0f;
    *(u32*)(p + 0x8)  = 0xFFFFFFFFu;
    *(f32*)(p + 0xC)  = -1.0f;
    *(f32*)(p + 0x10) = 1.0f;
    *(f32*)(p + 0x14) = -1.0f;
    // Vertex 1
    s32 h = VideoGetHeight(&gVideo);
    s32 ratio = VideoGetWidth(&gVideo) / h;
    *(f32*)(p + 0x18) = (f32)ratio;
    *(f32*)(p + 0x1C) = 1.0f;
    *(u32*)(p + 0x20) = 0xFFFFFFFFu;
    *(f32*)(p + 0x24) = 1.0f;
    *(f32*)(p + 0x28) = 1.0f;
    *(f32*)(p + 0x2C) = -1.0f;
    // Vertex 2
    *(f32*)(p + 0x30) = 0.0f;
    *(f32*)(p + 0x34) = 0.0f;
    *(u32*)(p + 0x38) = 0xFFFFFFFFu;
    *(f32*)(p + 0x3C) = -1.0f;
    *(f32*)(p + 0x40) = -1.0f;
    *(f32*)(p + 0x44) = -1.0f;
    // Vertex 3
    s32 h2 = VideoGetHeight(&gVideo);
    s32 ratio2 = VideoGetWidth(&gVideo) / h2;
    *(f32*)(p + 0x48) = (f32)ratio2;
    *(f32*)(p + 0x4C) = 0.0f;
    *(u32*)(p + 0x50) = 0xFFFFFFFFu;
    *(f32*)(p + 0x54) = 1.0f;
    *(f32*)(p + 0x58) = -1.0f;
    *(f32*)(p + 0x5C) = -1.0f;
    fn_80151644(0x98, 4, p);
}

// 0x80063D6C
// (Re)initialises a world entity from its definition (arg0+0x9C): releases its effect/handles, copies
// the spawn position into the entity and its child objects, spawns the entrance effect, registers it
// with the world, attaches the optional camera link and hash-table entry, and sets the initial
// orientation from its facing vector. Returns 1 on success, 0 when the entity was torn down.
s32 fn_80063D6C(u8* arg0, u8* arg1, u8* arg2, u8* arg3, s32 arg4, s32 arg5, s32 arg6) {
    u8* fx = *(u8**)(arg0 + 0xA8);
    if (fx != NULL) {
        fn_801C849C(lbl_8047C760, fx);
        *(u8**)(arg0 + 0xA8) = NULL;
    }
    s32 handle = *(s32*)(arg0 + 0xAC);
    if (handle != -1) {
        fn_8020A398(handle);
        *(s32*)(arg0 + 0xAC) = -1;
    }
    u8* h234 = *(u8**)(arg0 + 0x234);
    if (h234 != NULL) {
        fn_801C3E94(lbl_8047C7AC);
        *(u8**)(arg0 + 0x234) = NULL;
    }
    u8* h238 = *(u8**)(arg0 + 0x238);
    if (h238 != NULL) {
        fn_801C3E88(lbl_8047C7AC, h238);
        *(u8**)(arg0 + 0x238) = NULL;
    }
    u8* def = *(u8**)(arg0 + 0x9C);
    *(s32*)(arg0 + 0x24C) = 0;
    *(u32*)(arg0 + 0x244) = *(u32*)(arg0 + 0x244) & 0xFBFFFFFFu;
    *(u32*)(arg0 + 0x14C) = kHashDefault;
    *(u8**)(arg0 + 0xA8) = NULL;
    *(u8**)(arg0 + 0x228) = def;
    if (def == NULL) {
        fn_80068B50(arg0, 0);
        return 0;
    }
    f32 x = *(f32*)(arg1 + 0x0);
    f32 y = *(f32*)(arg1 + 0x4);
    f32 z = *(f32*)(arg1 + 0x8);
    *(u32*)(arg0 + 0x244) = (*(u32*)(def + 0x148) & ~(u32)arg5) | (u32)arg6;
    u8* child = *(u8**)(arg0 + 0x22C);
    *(u8**)(arg0 + 0x23C) = arg3;
    *(f32*)(arg0 + 0x194) = x;
    *(f32*)(arg0 + 0x198) = y;
    *(f32*)(arg0 + 0x19C) = z;
    *(f32*)(arg0 + 0x1A0) = x;
    *(f32*)(arg0 + 0x1A4) = y;
    *(f32*)(arg0 + 0x1A8) = z;
    *(f32*)(child + 0x14) = x;
    *(f32*)(child + 0x18) = y;
    *(f32*)(child + 0x1C) = z;
    child = *(u8**)(arg0 + 0x22C);
    *(f32*)(child + 0x20) = x;
    *(f32*)(child + 0x24) = y;
    *(f32*)(child + 0x28) = z;
    *(f32*)(*(u8**)(arg0 + 0x22C) + 0x3C) = *(f32*)(*(u8**)(arg0 + 0x228) + 0xBC);
    def = *(u8**)(arg0 + 0x228);
    *(f32*)(arg0 + 0x230) = *(f32*)(def + 0xD0);
    u32 effect = *(u32*)(def + 0xF0);
    if (effect != 0) {
        f32 posAndScale[4] = { x, y, z, 0.0f };
        SpawnEffectAt(lbl_8047C760, effect, posAndScale, (u8**)(arg0 + 0xA8));
    }
    if (fn_8012E420(gWorldState, arg0, 7, 0) == 0) {
        fn_80068B50(arg0, 0);
        return 0;
    }
    def = *(u8**)(arg0 + 0x228);
    if (*(f32*)(def + 0xC8) != 0.0f) {
        u8* e = fn_8005CBC4();
        def = *(u8**)(arg0 + 0x228);
        *(f32*)(e + 0x10) = *(f32*)(def + 0xC8);
        *(f32*)(e + 0x0) = *(f32*)(arg1 + 0x0);
        *(f32*)(e + 0x4) = *(f32*)(arg1 + 0x4);
        *(f32*)(e + 0x8) = *(f32*)(arg1 + 0x8);
        *(f32*)(e + 0xC) = 3.0f + *(f32*)(*(u8**)(arg0 + 0x228) + 0xC4);
        *(f32*)(e + 0x14) = *(f32*)(*(u8**)(arg0 + 0x228) + 0xD0);
    }
    u8* zoneInfo = *(u8**)(gWorldState + 0xE0);
    if (zoneInfo != NULL && ZoneIsLoaded(gWorldState, *(s32*)(zoneInfo + 0x18))) {
        *(s16*)(*(u8**)(arg0 + 0x4) + 0x30) = (s16)*(s32*)(*(u8**)(gWorldState + 0xE0) + 0x18);
    }
    u8* obj = *(u8**)(arg0 + 0x4);
    *(f32*)(obj + 0x0) = *(f32*)(arg1 + 0x0);
    *(f32*)(obj + 0x4) = *(f32*)(arg1 + 0x4);
    *(f32*)(obj + 0x8) = *(f32*)(arg1 + 0x8);
    *(s16*)(*(u8**)(arg0 + 0x4) + 0x32) = 0;
    u32 flags = *(u32*)(arg0 + 0x244) | 0x80000000u;
    *(u32*)(arg0 + 0x244) = flags;
    if (flags & 0x400) {
        u8* att = *(u8**)(arg0 + 0x25C);
        if (att == NULL) {
            att = fn_80063388(NULL);
            *(u8**)(arg0 + 0x25C) = att;
            if (att == NULL) return 0;
        }
        u8* d = *(u8**)(arg0 + 0x228);
        f32 duration = *(f32*)(d + 0x160);
        f32 q[4];
        q[0] = *(f32*)(d + 0x150);
        q[1] = *(f32*)(d + 0x154);
        q[2] = *(f32*)(d + 0x158);
        q[3] = *(f32*)(d + 0x15C);
        fn_80063530(*(u8**)(arg0 + 0x25C), q);
        fn_8006359C(*(u8**)(arg0 + 0x25C), duration);
        u8* o4 = *(u8**)(arg0 + 0x4);
        u8* t = *(u8**)(arg0 + 0x25C);
        *(f32*)(t + 0x20) = *(f32*)(o4 + 0x0);
        *(f32*)(t + 0x24) = *(f32*)(o4 + 0x4);
        *(f32*)(t + 0x28) = *(f32*)(o4 + 0x8);
        fn_800633D4(*(u8**)(arg0 + 0x25C));
    }
    s32 hashKey = *(s32*)(*(u8**)(arg0 + 0x228) + 0x104);
    if (hashKey != 0 && *(u8**)(arg0 + 0x254) == NULL) {
        u8* world = *(u8**)gWorldState;
        u8** found = (u8**)HashTableFind(*(u8**)(world + 0x1A4) + 0xE4, (u32)hashKey);
        *(u8***)(arg0 + 0x254) = found;
        if (found != NULL) {
            *(u8**)(arg0 + 0x90) = *found;
            fn_801DA344(*found, arg0);
        } else {
            fn_80068B50(arg0, 0);
            return 0;
        }
    }
    if (fn_800637AC(arg0, arg2) == 0) return 0;
    if (arg4 != 0) {
        u8* o = *(u8**)(arg0 + 0x4);
        *(u32*)(o + 0x20) = *(u32*)(o + 0x20) | 0x04000000u;
    } else {
        u8* o = *(u8**)(arg0 + 0x4);
        *(u32*)(o + 0x20) = *(u32*)(o + 0x20) & 0xFBFFFFFFu;
        *(s32*)(arg0 + 0x30) = lbl_805FB6AC;
        lbl_805FB6AC = lbl_805FB6AC + 1;
    }
    u8* link = *(u8**)(arg0 + 0x23C);
    if (link != NULL) {
        u8* o4 = *(u8**)(arg0 + 0x4);
        *(s32*)(arg0 + 0x240) = *(u16*)(*(u8**)(link + 0x4) + 0x32);
        u8* src = *(u8**)(link + 0x4);
        *(f32*)(o4 + 0xC)  = *(f32*)(src + 0xC);
        *(f32*)(o4 + 0x10) = *(f32*)(src + 0x10);
        *(f32*)(o4 + 0x14) = *(f32*)(src + 0x14);
        *(f32*)(o4 + 0x18) = *(f32*)(src + 0x18);
    } else {
        u8* o4 = *(u8**)(arg0 + 0x4);
        *(f32*)(o4 + 0xC)  = 0.0f;
        *(f32*)(o4 + 0x10) = 0.0f;
        *(f32*)(o4 + 0x14) = 0.0f;
        *(f32*)(o4 + 0x18) = 1.0f;
    }
    if (*(u8**)(arg0 + 0x254) != NULL) {
        Vec3 face = { *(f32*)(arg0 + 0x1AC), *(f32*)(arg0 + 0x1B0), *(f32*)(arg0 + 0x1B4) };
        if (!NearZero3(face.x, face.y, face.z)) {
            Vec3 d = Vec3Normalized(face);
            Vec3 flat = { d.x, d.y, 0.0f };
            Vec3 ref;
            if (NearZero3(flat.x, flat.y, flat.z)) {
                Vec3 fwd;
                QuatGetForward(&fwd, (Quat*)(*(u8**)(arg0 + 0x4) + 0xC));
                ref = fwd;
            } else {
                Vec3 n = Vec3Normalized(flat);
                // ref = n x (0, 0, 1)
                ref.x = n.y * 1.0f - n.z * 0.0f;
                ref.y = n.z * 0.0f - n.x * 1.0f;
                ref.z = n.x * 0.0f - n.y * 0.0f;
            }
            Vec3 side;
            side.x = ref.y * d.z - ref.z * d.y;
            side.y = ref.z * d.x - ref.x * d.z;
            side.z = ref.x * d.y - ref.y * d.x;
            fn_801458B0((f32*)(*(u8**)(arg0 + 0x4) + 0xC), (f32*)&d, (f32*)&side);
            u8* o = *(u8**)(arg0 + 0x4);
            *(u32*)(o + 0x20) = *(u32*)(o + 0x20) | 0x8000u;
            o = *(u8**)(arg0 + 0x4);
            *(u32*)(o + 0x20) = *(u32*)(o + 0x20) | 0x10000000u;
        }
    }
    u8* o = *(u8**)(arg0 + 0x4);
    *(u32*)(o + 0x20) = *(u32*)(o + 0x20) | 0x8000u;
    o = *(u8**)(arg0 + 0x4);
    *(u32*)(o + 0x20) = *(u32*)(o + 0x20) | 0x4000u;
    fn_801B4EA4(arg0, NULL);
    return 1;
}

// 0x801479D0
// Polls the four Wii remote channels: reads up to 16 KPAD status records per channel, pushes them
// into the channel's history ring (0x84-byte entries), and converts the newest record into the
// register block at lbl_805FB490 + 0x400 + chan * 0xB4 (button bits as 0x2000000x words, analog values
// as float bits >> 3 with the top bit set). lbl_80400FE0[chan] gets the controller state
// (0 none, 6 plain remote, 7 with nunchuk-style stick, 8 other extension).
void fn_801479D0(void) {
    u32 words[0x840 / 4];
    u8* buf = (u8*)words;

    for (s32 chan = 0; chan < 4; chan++) {
        s32 off = chan * 0xB4;
        s32* state = &lbl_80400FE0[chan];
        s32 n = fn_8027F504(chan, buf, 0x10);
        u8* ring = fn_801483BC(chan);
        if (ring != NULL) {
            *(s32*)(ring + 0x14) = n;
            for (s32 k = 0; k < n; k++) {
                s32 cap = *(s32*)(ring + 0xC);
                s32 head = (cap + *(s32*)(ring + 0x10) - 1) % cap;
                *(s32*)(ring + 0x10) = head;
                memcpy(*(u8**)(ring + 0x4) + head * 0x84, buf + (n - (k + 1)) * 0x84, 0x84);
                *(s32*)(*(u8**)(ring + 0x8) + head * 4) = *(s32*)(ring + 0x18);
                s32 serial = *(s32*)(ring + 0x18) + 1;
                *(s32*)(ring + 0x18) = serial;
                if (serial == 0) {
                    *(s32*)(ring + 0x18) = *(s32*)(ring + 0x18) + 1;
                }
            }
        }
        s8 err = *(s8*)(buf + 0x5D);
        if (n < 1 || err != 0) {
            if (err == -1) {
                memset(lbl_805FB490 + off + 0x400, 0, 0xB4);
                *state = 0;
            } else {
                memcpy(lbl_805FB490 + off + 0x400, lbl_805FB494 + off + 0x400, 0xB4);
                u32 type = 0;
                if (fn_802728A4(0, &type) == -1) {
                    *state = 0;
                }
            }
        } else {
            u8 ext = *(u8*)(buf + 0x5C);
            if (ext == 1) {
                *state = 7;
            } else if (ext == 2) {
                *state = 8;
            } else if (ext == 0xFD) {
                *state = 0;
            } else {
                *state = 6;
            }
            u32 hold = *(u32*)(buf + 0x0);
            u8* rec = lbl_805FB490 + off + 0x400;
            *(u32*)(rec + 0x00) = ((hold >> 3) & 1) | 0x20000000u;
            *(u32*)(rec + 0x04) = ((hold >> 2) & 1) | 0x20000000u;
            *(u32*)(rec + 0x08) = (hold & 1) | 0x20000000u;
            *(u32*)(rec + 0x0C) = ((hold >> 1) & 1) | 0x20000000u;
            *(u32*)(rec + 0x10) = ((hold >> 11) & 1) | 0x20000000u;
            *(u32*)(rec + 0x14) = ((hold >> 10) & 1) | 0x20000000u;
            *(u32*)(rec + 0x18) = ((hold >> 15) & 1) | 0x20000000u;
            *(u32*)(rec + 0x1C) = ((hold >> 4) & 1) | 0x20000000u;
            *(u32*)(rec + 0x20) = ((hold >> 12) & 1) | 0x20000000u;
            *(u32*)(rec + 0x24) = ((hold >> 9) & 1) | 0x20000000u;
            *(u32*)(rec + 0x28) = ((hold >> 8) & 1) | 0x20000000u;
            *(u32*)(rec + 0x2C) = (FloatBits(*(f32*)(buf + 0xC)) >> 3) | 0x80000000u;
            *(u32*)(rec + 0x30) = (FloatBits(*(f32*)(buf + 0x10)) >> 3) | 0x80000000u;
            *(u32*)(rec + 0x34) = (FloatBits(*(f32*)(buf + 0x14)) >> 3) | 0x80000000u;
            *(u32*)(rec + 0x38) = (FloatBits(*(f32*)(buf + 0x20)) >> 3) | 0x80000000u;
            *(u32*)(rec + 0x3C) = (FloatBits(*(f32*)(buf + 0x24)) >> 3) | 0x80000000u;
            *(u32*)(rec + 0x40) = (FloatBits((f32)*(s8*)(buf + 0x5E)) >> 3) | 0x80000000u;
            *(u32*)(rec + 0x44) = (FloatBits(*(f32*)(buf + 0x34)) >> 3) | 0x80000000u;
            *(u32*)(rec + 0x48) = (FloatBits(*(f32*)(buf + 0x38)) >> 3) | 0x80000000u;
            *(u32*)(rec + 0x4C) = (FloatBits(*(f32*)(buf + 0x48)) >> 3) | 0x80000000u;
            *(u32*)(rec + 0x50) = (FloatBits(*(f32*)(buf + 0x54)) >> 3) | 0x80000000u;
            *(u32*)(rec + 0x54) = (FloatBits(*(f32*)(buf + 0x58)) >> 3) | 0x80000000u;
            if (*state == 7) {
                *(u32*)(rec + 0x58) = ((hold >> 13) & 1) | 0x20000000u;
                *(u32*)(rec + 0x5C) = ((hold >> 14) & 1) | 0x20000000u;
                f32 sy = *(f32*)(buf + 0x64);
                f32 sx = *(f32*)(buf + 0x60);
                f32 ang = 0.63661975f * fn_8033F1FC(sy, sx);
                f32 len2 = sx * sx + sy * sy;
                f32 len = (len2 <= 0.0f) ? 0.0f : sqrtf(len2);   // original: frsqrte with two Newton steps
                u32 packed;
                if (len < 0.2f) {
                    packed = 0x60000000u;
                } else {
                    f32 t = (len - 0.2f) / 0.8f;
                    if (t < 0.0f) {
                        t = 0.0f;
                    } else if (t > 1.0f) {
                        t = 1.0f;
                    }
                    f32 scaled = 8191.0f * t;
                    s32 mag;
                    if ((s32)scaled < 0) {
                        mag = 0;
                    } else if ((s32)scaled > 0x1FFF) {
                        mag = 0x1FFF;
                    } else {
                        mag = (s32)scaled;
                    }
                    f32 bias = (ang < 0.0f) ? -0.5f : 0.5f;
                    packed = ((u32)mag << 16) | ((u32)(u16)(s32)(4096.0f * ang + bias) | 0x60000000u);
                }
                *(u32*)(rec + 0x60) = packed;
                *(u32*)(rec + 0x64) = (FloatBits(*(f32*)(buf + 0x68)) >> 3) | 0x80000000u;
                *(u32*)(rec + 0x68) = (FloatBits(*(f32*)(buf + 0x6C)) >> 3) | 0x80000000u;
                *(u32*)(rec + 0x6C) = (FloatBits(*(f32*)(buf + 0x70)) >> 3) | 0x80000000u;
            } else {
                *(u32*)(rec + 0x58) = 0;
                *(u32*)(rec + 0x5C) = 0;
                *(u32*)(rec + 0x60) = 0;
                *(u32*)(rec + 0x64) = 0;
                *(u32*)(rec + 0x68) = 0;
                *(u32*)(rec + 0x6C) = 0;
            }
            for (s32 o = 0x70; o <= 0xB0; o += 4) {
                *(u32*)(rec + o) = 0;
            }
        }
    }
}

// 0x80047D54
// Sets up the combat rewards screen (when the HUD screen is active): activates its element set, hides
// the HUD input, fills the reward rows (the dropped part first, then each reward item: icon, name and
// count), shows the money total, decides whether the rematch/continue buttons appear and rescales the
// banner image to the video aspect ratio.
void SetUpCombatRewardsScreen(void) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    if (*(u32*)set != kHashCombatHud) return;
    ScreenSetsActivate(gScreenSets, 0x7A1060A7u);
    u8* el = (u8*)ScreenSetFindElement(set, 0x7A1060A7u);
    if (el != NULL) {
        (*(ElementFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
    }
    fn_800493A8(1);
    CombatHUDDisableInput(lbl_805FB0A0);
    s32 n = 0;
    u8* elText = (u8*)ScreenSetFindElement(set, *(u32*)(lbl_803589D8 + 0x138));
    u8* elIcon = (u8*)ScreenSetFindElement(set, *(u32*)(lbl_803589D8 + 0x148));
    if (elText != NULL && elIcon != NULL) {
        s32* partSlot = *(s32**)((u8*)gLab + 0x3C);
        if (partSlot != NULL) {
            PartDef def;
            PartDbGetDef(&def, gGameDb, *partSlot);
            PartDef copy = def;
            u8* tex = (u8*)fn_8011AD88(&copy, 0);
            lbl_803C5308[0] = tex;
            if (tex != NULL) {
                UISetTexture(elIcon, tex);
            }
            const char* text = TextFromHash(copy.tierName[0]);
            if (text != NULL) {
                UISetText(elText, text, 1);
            }
            UIEvent_SendToScreen(0x5717A459u, 0xCFBD5B9Eu);
            n = 1;
        }
    }
    for (u32 idx = 0; idx < *(u32*)((u8*)gLab + 0x8); idx++) {
        u8* elName = (u8*)ScreenSetFindElement(set, *(u32*)(lbl_803589D8 + 0x138 + n * 4));
        u8* elTex = (u8*)ScreenSetFindElement(set, *(u32*)(lbl_803589D8 + 0x148 + n * 4));
        if (elName == NULL || elTex == NULL) continue;
        u8* item = fn_80218B8C(gLab, (s32)idx);
        if (item == NULL) continue;
        u8* elCount = (u8*)ScreenSetFindElement(set, *(u32*)(lbl_803589D8 + 0x178 + n * 4));
        if (elCount != NULL) {
            char numbuf[16];
            memset(numbuf, 0, sizeof(numbuf));
            SysSprintf(numbuf, lbl_805F866C, (s32)*(u8*)(item + 8));
            UISetText(elCount, numbuf, 1);
        }
        const char* name;
        switch (*(s32*)item) {
        case 0: {
            u8* rec = *(u8**)(gGameDb->data + 0x4F00) + *(s32*)(item + 4) * 0x10;
            name = TextFromHash(*(u32*)(rec + 4));
            break;
        }
        case 1: {
            u8* rec = *(u8**)(gGameDb->data + 0x5328) + *(s32*)(item + 4) * 0x18;
            name = TextFromHash(*(u32*)(rec + 4));
            break;
        }
        default:
            name = NULL;
            break;
        }
        if (name != NULL) {
            UISetText(elName, name, 1);
        }
        u8* icon = fn_801186FC(item);
        lbl_803C5308[n] = icon;
        if (icon != NULL) {
            UISetTexture(elTex, icon);
        }
        n++;
    }
    if (n > 0) {
        UIEvent_SendToScreen(*(u32*)(lbl_803589D8 + 0x128 + (n - 1) * 4), 0xCFBD5B9Eu);
        lbl_805FB0B8 = 0;
        UIEvent_SendToScreen(0xB2133ED4u, 0xCFBD5B9Eu);
    }
    el = (u8*)ScreenSetFindElement(set, 0x252A1B41u);
    if (el != NULL) {
        UISetNumber(el, *(s32*)((u8*)gLab + 0x78));
        (*(ElementFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
    }
    s32 healed = 1;
    s32 allFull = 1;
    u8* fighter = *(u8**)(gBattle + 0xA4C);
    if (fighter != NULL) {
        u8* mon = *(u8**)(fighter + 0x1C);
        if (mon != NULL) mon -= 0xC48;
        mon += 0x898;
        healed = 0;
        if (mon != NULL && MonsterIsFullyHealed(mon)) {
            healed = 1;
        }
        for (s32 i = 0; i < 5; i++) {
            if (fighter != NULL && FighterGetSlot(fighter, i) != NULL) {
                s32 cur = **(s32**)FighterGetSlot(fighter, i);
                if (cur < *(s32*)(*FighterGetSlot(fighter, i) + 4)) {
                    allFull = 0;
                    break;
                }
            }
        }
    }
    if (fighter != *(u8**)(gBattle + 0xA00) || *(u8*)(lbl_805FBBE0 + 0xA25) == 0 || (healed && allFull) ||
        IsMultiPlayer() != 0 || AlertGetLastError() == 0xE38C521Fu) {
        lbl_805F865D = 0;
        el = (u8*)ScreenSetFindElement(set, 0x3F401183u);
        if (el != NULL) {
            (*(ElementFn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        }
    } else {
        lbl_805F865D = 1;
        el = (u8*)ScreenSetFindElement(set, 0xC400769Fu);
        if (el != NULL) {
            (*(ElementFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
        }
    }
    el = (u8*)ScreenSetFindElement(set, 0x1B49766Fu);
    if (el != NULL) {
        f32 x = *(f32*)(el + 0x74);
        f32 z = *(f32*)(el + 0x7C);
        u8* tex = (u8*)fn_801A1208(el);
        f32 w = (f32)*(u16*)(tex + 6);
        f32 screenH = (f32)gScreenHeight;
        f32 videoH = (f32)VideoGetHeight(&gVideo);
        f32 y = (w * screenH) / videoH;
        *(f32*)(el + 0x74) = x;
        *(f32*)(el + 0x78) = y;
        *(f32*)(el + 0x7C) = z;
        (*(ElementFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x18);
    }
}

// 0x80115F28
// Loads the model info shown on the lab part page: finds the selected part (parts page, recipes
// page, or the operating table's selection), asks the model cache for its texture (lbl_805FB250),
// and loads the enhancement/defect texture and text (lbl_805FB254 / lbl_805FB258). Also stores
// whether the part is allowed at its tier in lbl_805FB24D.
void PartInfo_LoadModel(void) {
    s32 partId;
    if (PageActive(0, kHashParts)) {
        u8* page = SubQuery(4, kHashParts);
        if (page != NULL) {
            s32 idx = fn_8011C214(page);
            partId = **(s32**)(lbl_805FBBE0 + 0xA58 + idx * 4);
        } else {
            partId = 0;
        }
    } else if (PageActive(1, kHashRecipes)) {
        u8* page = SubQuery(8, kHashRecipes);
        partId = (page != NULL) ? fn_80120390(page) : 0;
    } else if (LighthingTowerGetSelectingPart() == 1) {
        partId = LightningGetInventoryPartID();
    } else {
        partId = 0;
    }
    PartDef def;
    PartDbGetDef(&def, gGameDb, partId);
    PartDef copy = def;
    lbl_805FB250 = (u8*)fn_8011AD88(&copy, 1);
    lbl_805FB254 = NULL;
    if (PageActive(1, kHashRecipes)) {
        u8* page = SubQuery(8, kHashRecipes);
        u8 tier = (u8)fn_801203C0(page);
        s32 part = fn_80120504(page);
        s32 recipePart = fn_80120390(page);
        if (SavePartTierOwned(lbl_805FBBE0, (u32)part, (s32)tier)) {
            PartDef def2;
            PartDbGetDef(&def2, gGameDb, partId);
            PartDef copy2 = def2;
            if (copy2.unk20 != -1) {
                u8* rec = *(u8**)(gGameDb->data + 0x3610) + copy2.unk20 * 0x24;
                lbl_805FB254 = EnhancementTexture(rec);
            }
        }
        s32 tierIndex = (s32)tier - 1;
        PartComputeRequiredRank(gLab, recipePart, tierIndex);
        lbl_805FB24D = (u8)PartAllowedAtRank(gLab, recipePart, tierIndex);
        return;
    }
    s32 idx;
    if (PageActive(0, kHashParts)) {
        u8* page = SubQuery(4, kHashParts);
        idx = fn_8011C214(page);
    } else {
        idx = LightningGetInventoryPos();
    }
    u8* slot = *(u8**)(lbl_805FBBE0 + 0xA58 + idx * 4);
    s32 enh = *(s32*)(slot + 0x8);
    if (enh != -1) {
        u8* rec = *(u8**)(gGameDb->data + 0x3610) + enh * 0x24;
        lbl_805FB254 = EnhancementTexture(rec);
    } else {
        s32 defect = *(s32*)(slot + 0x4);
        if (defect != -1) {
            lbl_805FB254 = (u8*)DefectGetTexture(defect);
        }
    }
    PartComputeRequiredRank(gLab, *(s32*)slot, (s32)*(u8*)(slot + 0x11) - 1);
    lbl_805FB24D = (u8)PartAllowedAtRank(gLab, *(s32*)slot, (s32)*(u8*)(slot + 0x11) - 1);
}

}  // extern "C"
