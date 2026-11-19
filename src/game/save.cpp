// Save game: Wii NAND save file "game.msa", banner/icon setup and the loading picture
// drawn while saving.
// Original object: .text 0x80011018 - 0x80011C90 (auto_03_80011018_text.s)
//
// The save code was inherited from the studio's previous game: the default banner names
// are "SAVEDATA_TITLE" / "DEATHJR2" (Death Jr. 2).
//
// PC port note: the platform layer maps the save file API (fopen/fread/fwrite on the
// "Wii NAND" wrapper, 0x80206xxx) to a file in the user's save folder.
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "engine/engine.h"
#include "game/game.h"

extern "C" {
s32   NANDCheck(u32 fsBlock, u32 inode, u32* answer);       // 0x80268194
void  SaveSetParams(void* state, void* params);              // 0x80206AA0
s32   SaveSetBanner(const char* title, const char* sub, s32* out);   // 0x80206AA4
s32   SaveOpen(const char* name, const char* mode);          // 0x80206474
s32   SaveRead(s32 handle, void* buf, u32 size);             // 0x8020681C
s32   SaveWriteData(s32 handle, const void* buf, u32 size);  // 0x8020690C
s32   SaveClose(s32 handle);                                 // 0x802067BC
s32   SaveSetup(const char* name);                           // 0x802069CC
s32  SysSprintf(char* buf, const char* fmt, ...);           // 0x80142EFC
void  BufferFree(void* p);                                   // 0x801D4980
void* TextureLoad(const char* name);                         // 0x801D8E80
void  TextureRelease(void* tex);                             // 0x80184028
void  TextureCacheRemove(void* cache, void* tex);            // 0x80169704
void  fn_8015B7D4(void);
void  RendBeginFrame(void);  void RendEndFrame(void);  void RendFlush(void);   // 0x8014F0A0 / 0x801508AC / 0x80150148
void  RendClear(int flags, int a, int b, f32 depth);         // 0x801534F0
void  RendSetDefaultState(void);                             // 0x8014855C
void  DrawFullscreenTexture(void* tex);                      // 0x80011C90
void  fn_80150DEC(void); void fn_80151014(void); void fn_801F0564(void);
void  fn_801FC0A0(void); void fn_801FC1EC(void);
}

// Parameters handed to the save wrapper (0x40 bytes).
struct SaveParams {
    u32 unk00[6];
    u32 unk18;        // = 1
    u8  unk1C;        // = 1
    u8  pad1D[3];
    u32 unk20[8];
};

static inline void SaveParamsInit(SaveParams& p) {
    memset(p.unk00, 0, sizeof(p.unk00));
    p.unk18 = 1;
    p.unk1C = 1;
    memset(p.unk20, 0, sizeof(p.unk20));
}

s32  DrawLoadingImage(BOOL draw);
void SaveSetNames(const char* title, const char* file);
void SaveSelectBannerLayout(void);

struct StringTable {
    const char** strings;
    u8           pad[8];
    u32          count;     // 0x0C
};

// ---- data ----
#define SAVE_SLOT_COUNT 20
static const char kSaveIcon[]     = "ui\\NEWICON";   ORIG(0x803547A8)
static const char kSlotFormat[]   = "%s_%c_%02d";    ORIG(0x803547B4)
static const char kSaveFile[]     = "game.msa";      ORIG(0x803547C0)
static const char kSaveComment[]  = "New Game";      ORIG(0x803547CC)

static u8   sSaveState;                ORIG(0x805FAEF8)
static u8   sBannerInit;               ORIG(0x805FAEF9)
static char sBannerTitle[0x58];        ORIG(0x803C49C4)
static char sBannerFile[0x58];         ORIG(0x803C4A1C)
char        gLoadingImage[0x58];       ORIG(0x803C4A74)
static char sSaveIconPath[0x58];       ORIG(0x803C4ACC)
static char sSaveCommentText[0x58];    ORIG(0x803C4B24)
u32         gBannerRect[8];            ORIG(0x805FAEFC)   // active layout
static u32  sBannerUnk;                ORIG(0x805FAF1C)
static u32  sBannerRectA[8];           ORIG(0x805FAF20)
static u32  sBannerRectB[8];           ORIG(0x805FAF40)
extern u8   gFlag805FB948;               ORIG(0x805FB948)
extern StringTable* gStringTable;      // 0x805FB910
extern StringTable* gStringTable2;     ORIG(0x805FB90C)
extern u8*  gSaveSystem;               ORIG(0x805FBBE0)
extern s32  lbl_805FB51C;
extern void* gTextureCache;            ORIG(0x805FB5A8)
extern u8   gGame[];                   // 0x803B53A0

static inline const char* GetString(StringTable* t, u32 id, const char* missing) {
    if (t->strings != NULL && id < t->count) return t->strings[id];
    return missing;
}

// Resolve a string id; bit 31 selects the secondary table.
static inline const char* LookupString(u32 id) {
    if (id == 0xFFFFFFFF) return "Not found";
    if ((id & 0x80000000) && gStringTable2 != NULL) {
        u32 index = id & 0x7FFFFFFF;
        if (gStringTable2->strings != NULL && index < gStringTable2->count) return gStringTable2->strings[index];
        return index == 0x7FFFFFFF ? "NOT FOUND" : "";
    }
    if (gStringTable->strings != NULL && id < gStringTable->count) return gStringTable->strings[id];
    return id == 0xFFFFFFFF ? "NOT FOUND" : "";
}

static inline void SetIconAndComment(const char* icon, const char* comment) {
    if (icon != NULL) {
        strncpy(sSaveIconPath, icon, 0x55);
        strcat(sSaveIconPath, ".png");
    } else {
        sSaveIconPath[0] = '\0';
    }
    if (comment != NULL) strncpy(sSaveCommentText, comment, 0x55);
    else                 sSaveCommentText[0] = '\0';
}

// The save banner variant ("a" or "b") follows the coin flip made in GameStart.
static inline void SetBannerVariant(void) {
    char buf[0x58];
    s32 variant = gGame[0xC460] ? *(s32*)(gGame + 0xC464) : *(s32*)(gGame + 0xC468);
    snprintf(buf, 0x55, "%s", variant == 0 ? "a" : "b");
    SaveSetNames(buf, NULL);
}

// Slot names "<name>_S_01" .. "_S_20" (allocated and freed again; unused by the wrapper).
static inline void BuildSlotNames(char** names, const char* name) {
    for (int i = 0; i < SAVE_SLOT_COUNT; i++) {
        names[i] = (char*)MemAllocDefault(strlen(name) + 6);
        if (names[i] == NULL) {
            names[i - 1] = (char*)"";   // original writes the previous slot (bug)
            break;
        }
        SysSprintf(names[i], kSlotFormat, name, 'S', i + 1);
    }
}

static inline void FreeSlotNames(char** names) {
    for (int i = 0; i < SAVE_SLOT_COUNT; i++) BufferFree(names[i]);
}

// If the NAND has no save data yet, write the banner using string #0x5B5 as its title.
static inline void SetupBannerIfNew(void) {
    u32 answer;
    NANDCheck(4, 1, &answer);
    if (answer == 0) {
        char title[0x100], sub[0x100];
        strcpy(title, GetString(gStringTable, 0x5B5, ""));
        strcpy(sub, "");
        s32 out = 0;
        SaveSetBanner(title, sub, &out);
        *(s32*)(gSaveSystem + 0x924) = out;
    }
}

// 0x80011018
void SaveResetBanner(void) {
    sBannerInit = 0;
}

// 0x80011024
// Write the save file. Returns -1 on error.
s32 SaveGameWrite(const char* name, const void* data, u32 size) {
    SaveParams params;
    SaveParamsInit(params);
    if (data == NULL || size == 0) return -1;

    SetIconAndComment(kSaveIcon, LookupString(*(u32*)(sSirHeader + 8)));
    SaveSelectBannerLayout();
    SetBannerVariant();

    char* slots[SAVE_SLOT_COUNT];
    BuildSlotNames(slots, name);
    SaveSetParams(&sSaveState, &params);

    s32 f = SaveOpen(kSaveFile, "wb");
    if (f < 0) return -1;
    if (SaveWriteData(f, data, size) < 0) return -1;
    if (SaveClose(f) < 0) return -1;

    FreeSlotNames(slots);
    SetupBannerIfNew();
    return DrawLoadingImage(FALSE);
}

// 0x800113CC
// Read the save file. Returns a negative NAND error code, or 3 on success.
s32 SaveGameRead(const char* name, void* data, u32 size) {
    SaveParams params;
    SaveParamsInit(params);

    SetIconAndComment(kSaveIcon, kSaveComment);
    SaveSelectBannerLayout();
    SetBannerVariant();

    char* slots[SAVE_SLOT_COUNT];
    BuildSlotNames(slots, name);
    SaveSetParams(&sSaveState, &params);

    s32 f = SaveOpen(kSaveFile, "rb");
    s32 result = SaveRead(f, data, size);
    SaveClose(f);
    FreeSlotNames(slots);

    switch (result) {
    case -4: case -1: case -15: case -5: case -8:   // NAND_RESULT_* errors
        return result;
    }
    return DrawLoadingImage(FALSE);
}

// 0x8001161C
s32 SaveInit(const char* name) {
    SaveParams params;
    SaveParamsInit(params);

    SaveSelectBannerLayout();
    SaveSetNames(NULL, NULL);
    SaveSetParams(&sSaveState, &params);

    char* slots[SAVE_SLOT_COUNT];
    BuildSlotNames(slots, name);
    SaveSetup(name);
    FreeSlotNames(slots);
    return DrawLoadingImage(FALSE);
}

// 0x80011754
extern "C" s32 SaveInitDefault(void) {
    return SaveInit(kSaveFile);
}

// 0x80011760
// Draw gLoadingImage (extension stripped) full screen for one frame when `draw` is set.
s32 DrawLoadingImage(BOOL draw) {
    u8* tex = NULL;
    if (draw) {
        char name[0x58];
        strncpy(name, gLoadingImage, 0x55);
        name[0x55] = '\0';
        for (s32 i = (s32)strlen(name); i >= 0; i--) {
            if (name[i] == '.') { name[i] = '\0'; break; }
        }
        tex = strlen(name) == 0 ? NULL : (u8*)TextureLoad(name);
        fn_8015B7D4();
        if (lbl_805FB51C == 0) RendBeginFrame();
    }

    if (draw) {
        DrawFullscreenTexture(tex);
        fn_80150DEC();
        fn_80151014();
    }
    fn_801F0564();
    if (draw) {
        fn_801FC0A0();
        fn_801FC1EC();
        RendEndFrame();
        RendBeginFrame();
        RendFlush();
        RendClear(7, 0, 0, 1.0f);
    }
    if (draw) {
        if (tex != NULL) {
            TextureCacheRemove(gTextureCache, tex);
            if (--*(s32*)(tex + 0x24) == 0) TextureRelease(tex);
        }
        RendSetDefaultState();
    }
    return 3;
}

// 0x800118C4
s32 SaveSetupBanner(void) {
    SetupBannerIfNew();
    return 0;
}

// 0x80011960
// TRUE if <dir><file> exists ("game.msa" when file is "").
BOOL SaveFileExists(const char* dir, const char* file) {
    if (dir == NULL) return FALSE;
    char path[0x80];
    strncpy(path, dir, 0x7D);
    path[0x7C] = '\0';
    strcat(path, file);
    if (strcmp(file, "") == 0) strcpy(path, kSaveFile);

    s32 f = SaveOpen(path, "rb");
    if (f < 0) return FALSE;
    SaveClose(f);
    return TRUE;
}

// 0x80011A08
extern "C" void fn_80011A08(void) {
}

static inline void ToUpper(char* s) {
    for (u32 i = 0; i < strlen(s); i++) {
        s32 c = (s8)s[i];
        if (c >= 0 && c < 0x100) c = toupper(c);
        s[i] = (char)c;
    }
}

// 0x80011A0C
// Banner title and file base name ("<base>.BIN", base truncated to 8 characters), upper case.
void SaveSetNames(const char* title, const char* file) {
    if (title == NULL) title = "SAVEDATA_TITLE";
    if (file == NULL)  file = "DEATHJR2";

    strncpy(sBannerTitle, title, 0x55);
    sBannerTitle[0x55] = '\0';
    strncpy(sBannerFile, file, 0x55);
    sBannerFile[0x55] = '\0';

    if (strlen(sBannerFile) <= 8) strcat(sBannerFile, ".BIN");
    else                          strcpy(sBannerFile + 8, ".BIN");
    sBannerFile[0x55] = '\0';

    ToUpper(sBannerTitle);
    ToUpper(sBannerFile);
}

// 0x80011BC0
void SaveSelectBannerLayout(void) {
    if (!sBannerInit) {
        for (int i = 0; i < 8; i += 2) { sBannerRectA[i] = 0; sBannerRectB[i] = 0; }
        sBannerUnk = 0;
        sBannerInit = 1;
    }
    memcpy(gBannerRect, gFlag805FB948 ? sBannerRectB : sBannerRectA, sizeof(gBannerRect));
}
