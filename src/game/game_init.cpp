// Game-side startup: settings block, system event hook, loading global_binary.sir.
// Original object: .text 0x8000CF8C - 0x8000F8xx (auto_03_8000CF8C_text.s)
#include <stdlib.h>
#include <string.h>

#include "engine/engine.h"
#include "system/sys.h"

// ---- not yet decompiled ----
extern "C" {
void  SysRequestExit(int mode);                 // 0x80142DE8
void  fn_8020B194(f32);                         // 0x8020B194
void  SetCallback_80146800(void (*cb)(void));   // 0x80146800: stores to 0x805FB470
void  SetCallback_80207164(void (*cb)(void));   // 0x80207164: stores to 0x805FBB24
void  fn_80010F64(void);
void  fn_80010F88(void);
void  RendSetCallbackA(void (*cb)(void));       // 0x8014DD74: stores to 0x805FB4F0
void  RendSetCallbackB(void (*cb)(void));       // 0x8014DD7C
void  fn_8000CFA8(void);
void  fn_8000CFD0(void);
void  GameFrameCallback(void);                  // 0x8000F5BC
void  fn_801210E4(void*);
void  fn_801F052C(void);  void fn_801F2BCC(void); void fn_8000E488(void); void fn_801D6780(void);
void  CameraSetMatrix(void* camera, const f32 mtx[3][4]);   // 0x801D6814
void* SirLoad(void* loader, const char* path, void* dst, u32 size, s32 flags);   // 0x801E6138
void  SirFinishLoad(void* loader, void** handle);           // 0x801E6C44
u32*  GetGlobalBinarySize(void);                // 0x801D2BF8: returns 0x805FB7A4
void  fn_800214D4(u32); void fn_801FC038(u32); void fn_801D5D0C(u32);
void  fn_80011018(void); void fn_8010F794(void); void fn_801F2EB8(void); void fn_8000C9E8(void);
void  fn_80013D54(void*); void fn_80013F8C(void*); void fn_8000ED5C(void);
void  StringTableInit(void* table, u32 a, void* b);          // 0x801EFC7C
void  fn_80011A08(void); void fn_8010711C(void);
u32   GetRandomSeed(void);                       // 0x801427F0
void  fn_8012148C(void); void fn_801230DC(void);
}

struct StateEntry { s32 id; s32 a, b, c; };
typedef void* (*AllocFn)(u32 size, const char* name);

// ---- data ----
extern u8         gGame[];                  ORIG(0x803B53A0) // start of .bss; large game state block
static StateEntry sStateTable[10];          ORIG(0x803C4400)
s32               gStateCount = 4;          ORIG(0x805F81C0)
s32               gStartState = 9;          ORIG(0x805F81C4)
static u8         sSirHeader[0x1000];       ORIG(0x803C2A20) // first 4 KB of global_binary.sir
extern u8         gSirLoader[];             ORIG(0x8048A8F0)
extern u8         gCamera[];                ORIG(0x8047C640)
extern u8         lbl_805FAEB0[];
u32               gGlobalBinaryField844;    ORIG(0x805FB888)
extern AllocFn    gStringTableAllocVtbl[];  ORIG(0x80385F00)
AllocFn*          gStringTableAlloc;        ORIG(0x805FB914)
void*             gStringTable;             ORIG(0x805FB910)
extern u32        lbl_805F81B4;
extern u8         lbl_805F81DC[];
s32               gLastSysEvent;            ORIG(0x805FAEAC)
extern u8         gIsReturningPlayer;       ORIG(0x805FAEA8)

#define MEM2_GLOBAL_BINARY ((void*)0x90400000)   // fixed load address right after the engine pool

// 0x8000EE4C
// Installed as gSysEventCallback: escalate the pending exit request (reset < power).
static void GameSysEventCallback(int event) {
    if (SysGetRequestedExitMode() <= event) {
        gLastSysEvent = event;
        SysRequestExit(event);
    }
}

// 0x8000EE8C
void GameInit(void) {
    GameSettings* settings = (GameSettings*)MemAllocDefault(sizeof(GameSettings));
    memset(settings, 0, sizeof(GameSettings));
    settings->skipDebugOverlay = 1;

    gSysEventCallback = GameSysEventCallback;
    fn_8020B194(0.5f);
    SetCallback_80146800(fn_80010F64);
    SetCallback_80207164(fn_80010F88);
    EngineSetGameSettings(settings);
}

// 0x8000EF04
void GameStart(int argc, char** argv) {
    (void)argv;
    RendSetCallbackA(fn_8000CFA8);
    RendSetCallbackB(fn_8000CFD0);
    EngineSetFrameCallback(GameFrameCallback);

    // Command-line parsing was compiled out of the final build; only the empty loop remains.
    for (int i = argc; i > 0; i--) {
    }

    fn_801210E4(lbl_805FAEB0);

    for (int i = 0; i < 10; i++) {
        sStateTable[i].id = i;
        sStateTable[i].a = sStateTable[i].b = sStateTable[i].c = 0;
    }
    gStateCount = 4;
    gStartState = 9;

    fn_801F052C();
    fn_801F2BCC();
    fn_8000E488();
    fn_801D6780();

    static const f32 kIdentity[3][4] = { {1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0} };
    CameraSetMatrix(gCamera, kIdentity);

    // global_binary.sir: read the 4 KB header, then the whole file into MEM2.
    SirLoad(gSirLoader, "global_binary.sir", sSirHeader, 0x1000, -1);
    void* handle = SirLoad(gSirLoader, "global_binary.sir", MEM2_GLOBAL_BINARY, *GetGlobalBinarySize(), 0);
    SirFinishLoad(gSirLoader, &handle);

    const u32* hdr = (const u32*)sSirHeader;
    gGlobalBinaryField844 = hdr[0x844 / 4];
    fn_800214D4(hdr[0xDC4 / 4]);
    fn_801FC038(hdr[0x824 / 4]);
    fn_801D5D0C(hdr[0x828 / 4]);

    fn_80011018();
    fn_8010F794();
    fn_801F2EB8();
    fn_8000C9E8();
    fn_80013D54(gGame + 0xC448);
    fn_80013F8C(gGame + 0xC448);
    fn_8000ED5C();

    gStringTableAlloc = gStringTableAllocVtbl;
    gStringTable = gStringTableAlloc[0](0x14, "String Table");
    StringTableInit(gStringTable, lbl_805F81B4, lbl_805F81DC);

    fn_80011A08();
    fn_8010711C();

    // Coin flip between two variants of something stored in the game block.
    srand(GetRandomSeed());
    s32* flip = (s32*)(gGame + 0xC464);
    if (rand() % 2 == 0) { flip[0] = 0; flip[1] = 1; }
    else                 { flip[0] = 1; flip[1] = 0; }

    if (gStartState != 8) {
        if (EngineGetGameSettings()->field_0x38 == 1 && gIsReturningPlayer) gStartState = 6;
        else                                                                gStartState = 2;
    }

    fn_8012148C();
    fn_801230DC();
}
