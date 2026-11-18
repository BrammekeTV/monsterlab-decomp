/* Type context for m2c drafts (tools/decomp.py). Plain C, no comments with //.
   Names must match the asm symbols (re-run the dtk split after renaming).
   tools/decomp.py appends prototypes from include/game/script_callbacks.h. */
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;
typedef int BOOL;

typedef struct Vec3 { f32 x, y, z; } Vec3;
typedef struct Quat { f32 x, y, z, w; } Quat;

/* ---- parts (include/game/part.h) ---- */
typedef struct PartAction {
    u32 key;
    s32 type;
    u32 unk08;
    s16 unk0C;
    s16 unk0E;
    u8 unk10;
    u8 pad11[3];
} PartAction;

typedef struct PartActionType {
    u32 unk00;
    u32 unk04;
    u32 statKey;
    u32 unk0C;
    u32 nameHash;
    u32 unk14[4];
} PartActionType;

typedef struct PartDef {
    s32 index;
    u32 tierName[4];
    s32 action1;
    s32 slot;
    s32 discipline;
    s32 unk20;
    s32 unk24;
    s32 unk28;
    s32 action2;
    s32 unk30;
    s32 unk34;
    s32 unk38;
    s32 unk3C;
    u16 unk40, unk42, unk44, unk46;
    u8 unk48[4];
    u8 unk4C[4];
} PartDef;

typedef struct PartInstance {
    s32 defIndex;
    s32 defect;
    s32 enhancement;
    s32 uid;
    s8 quality;
    u8 tier;
    u8 tierScale[4];
    u8 pad16[2];
    void *vtable;
    s8 madnessBase;
    s8 unk1D;
    u8 unk1E, unk1F;
    s32 unk20;
    s32 unk24;
    s32 health;
    s32 unk2C;
} PartInstance;

typedef struct GameDbData {
    u8 pad0000[8];
    PartActionType *actionTypes;
    u8 pad000C[0x424];
    PartAction *actions;
    u8 pad0434[0x5744];
    PartDef *partDefs;
    s32 partDefCount;
} GameDbData;

typedef struct GameDb {
    u8 pad00[0xC];
    GameDbData *data;
} GameDb;

/* ---- battle (src/game/battle_damage.cpp) ---- */
typedef struct DamageRecord {
    u8 hit;
    u8 pad[3];
    s32 **value;
    u32 flags;
} DamageRecord;

typedef struct PartStatus {
    s32 hp;
    s32 maxHp;
    u8 pad08[0x18];
    s32 ability;
    s32 resistance;
} PartStatus;

typedef struct BattlePart {
    PartInstance *inst;
    s32 slot;
    u8 pad08[0x20];
    PartStatus *status;
} BattlePart;

typedef struct FighterStats {
    u8 pad00[0x18];
    u32 actionKey;
    u8 charged;
    u8 pad1D[3];
    s32 targetSlot;
    s32 side;
} FighterStats;

typedef struct Fighter {
    BattlePart *slots[8];
    FighterStats stats;
} Fighter;

/* ---- world / characters (include/game/character.h) ---- */
typedef struct CharacterDef {
    Vec3 pos;
    Quat rot;
    u32 flags;
    u32 stateFlags;
} CharacterDef;

typedef struct ScriptCtx {
    u8 pad00[0x10];
    void *chr;
    u8 pad14[8];
    s32 arg;
    s32 arg2;
    u8 pad24[8];
    f32 timer;
} ScriptCtx;

/* ---- globals ---- */
extern GameDb *gGameDb;
extern u8 *gBattle;
extern void *gScreenSets;
extern u8 *gSaveSystem;
extern s32 gZoneResourceSize;

/* ---- engine functions with known signatures ---- */
void PartDbGetDef(PartDef *out, GameDb *db, s32 index);
s32 TextIdFromHash(u32 hash);
u8 PartGetTier(PartInstance *p);
s8 PartGetQuality(PartInstance *p);
s32 PartGetMaxHealth(PartInstance *p);
s16 PartGetStatModifier(PartInstance *p, s32 stat);
const char *GetText(s32 id);
u32 StringHash(const char *s);
void SysSprintf(char *buf, const char *fmt, ...);
void *ScreenSetsCurrent(void *sets);
void *ScreenSetFindWidget(void *set, u32 hash);
void *UIWidgetFindChild(void *w, u32 hash);
void UISetText(void *el, const char *text, s32 a);
void *EventPost(s32 id, void *target);
u8 *GetGlobalSettings(void);
void *CharacterFind(s32 id);
u32 RandomU32(void);
void *MemAllocDefault(u32 size);
void BufferFree(void *p);
void *memset(void *p, s32 c, u32 n);
void *memcpy(void *d, const void *s, u32 n);
s32 strcmp(const char *a, const char *b);
char *strcpy(char *d, const char *s);
u32 strlen(const char *s);
