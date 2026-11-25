// Script system set-up, called once from GameStart: clears the function table, defines the
// signatures and registers every script function.
// Original: 0x800505F0 (auto_03_8004E318_text.s), 0x800E75B0 (auto_03_800E75B0_text.s),
// 0x8010F794 (auto_03_8010CD18_text.s), 0x801DC59C, 0x801DC870 (auto_03_801D9028_text.s)
#include "types.h"

extern "C" {
s32  RegisterScriptFunc(const char* name, s32 type, void* fn);   // 0x801DC91C
void ScriptSigDefine(u32 id, u32 ret, u32 argc, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4,
                     u32 a5, u32 a6, u32 a7);                    // 0x801DC7BC
void ScriptSigFinalize(u8* sig);                                 // 0x801DC658
void RegisterEngineScriptSigs(void);
void RegisterGameScriptFuncs(void);                              // 0x800FB958
void RegisterHooks(void);
void RegisterCIWaypointPathScripts(void);                                          // 0x800E2EBC
void fn_801068A8(void);                                          // 0x801068A8
void fn_801E0318(s32 a3);                                        // 0x801E0318
void fn_801E0320(s32 a3);                                        // 0x801E0320
void fn_801E0328(s32 a3);                                        // 0x801E0328
void fn_80106884(void);                                          // 0x80106884: the game's signatures
u32 fn_8010F7F8(s32 func, u32 sig, u32* args, s32 flag);                                          // 0x8010F7F8 (only its address is used here)
BOOL fn_8010CD18(s32 a3, s32 a4, u8* a5, u8* a6);                // 0x8010CD18
void fn_8010F784(s32 a3);                                        // 0x8010F784

// Conversation script functions (addresses only).
void SetNameID(s32 a0);
void SetNpcLineID(s32 a0);
void SetPlayerLineID(s32 a3);
void set_conversation_reg(s32 idx, s32 value);
void SetPlayerOption(s32 a0, u32 a1);
s32  Conversation_reg_eq(s32 idx, s32 value);
void ShowConversationPopup(u32 character, s32 textId);
u8   IsConversationPopupVisible(void);
void ShowEventPopup(s32 unusedR3, u32 a1);
BOOL IsEventPopupVisible(void);
s32  GetTalkingToCharacterEntity(void);
void SetConversationReady(s32 a3);

extern const char lbl_803592F8[];       // 0x803592F8: the names of the conversation functions
extern u8  gScriptSigs[];               // 0x34 bytes per signature
extern u8  gScriptFuncs[];              // 0xC bytes per function
extern s32 lbl_805FB880;                // 0x805FB880: script functions registered
extern s32 lbl_805F9650;                // 0x805F9650: capacity of gScriptFuncs (0x880)
}

void RegisterScriptFunctions(void);
void RegisterCharacterStateCallbacks(void);

// 0x800505F0
// Registers the conversation script functions. The second argument is the signature id.
extern "C" void fn_800505F0(void) {
    const char* names = lbl_803592F8;
    RegisterScriptFunc(names + 0x00, 0x65, (void*)&SetNameID);
    RegisterScriptFunc(names + 0x0C, 0x65, (void*)&SetNpcLineID);
    RegisterScriptFunc(names + 0x20, 0x65, (void*)&SetPlayerLineID);
    RegisterScriptFunc(names + 0x30, 0x187, (void*)&set_conversation_reg);
    RegisterScriptFunc(names + 0x48, 0x8D, (void*)&SetPlayerOption);
    RegisterScriptFunc(names + 0x58, 0x9, (void*)&Conversation_reg_eq);
    RegisterScriptFunc(names + 0x6C, 0x73, (void*)&ShowConversationPopup);
    RegisterScriptFunc(names + 0x84, 0xA, (void*)&IsConversationPopupVisible);
    RegisterScriptFunc(names + 0xA0, 0x65, (void*)&ShowEventPopup);
    RegisterScriptFunc(names + 0xB0, 0xA, (void*)&IsEventPopupVisible);
    RegisterScriptFunc(names + 0xC4, 0x22, (void*)&GetTalkingToCharacterEntity);
    RegisterScriptFunc(names + 0xE0, 0x67, (void*)&SetConversationReady);
}

// 0x800E75B0
extern "C" void fn_800E75B0(void) {
    RegisterGameScriptFuncs();
    RegisterCIWaypointPathScripts();
    fn_800505F0();
}

// 0x801DC59C
// Gives all 0x211 signatures the default (returns type 1, no arguments), lets the engine and then
// the game define theirs, and finalises each one.
extern "C" s32 ScriptSigsInit(void (*gameSigs)(void)) {
    for (u32 i = 0; i < 0x211; i++) {
        ScriptSigDefine(i, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    RegisterEngineScriptSigs();
    if (gameSigs != NULL) {
        gameSigs();
    }
    for (u32 i = 0; i < 0x211; i++) {
        ScriptSigFinalize(gScriptSigs + i * 0x34);
    }
    return 1;
}

// 0x801DC870
// Empties the script function table: clears the first two words of each of its 0x880 entries.
extern "C" void fn_801DC870(void) {
    lbl_805FB880 = 0;
    lbl_805F9650 = 0x880;
    for (s32 i = 0; i < 0x880; i++) {
        *(u32*)(gScriptFuncs + i * 0xC) = 0;
        *(u32*)(gScriptFuncs + i * 0xC + 4) = 0;
    }
}

// 0x8010F794
extern "C" void fn_8010F794(void) {
    fn_801068A8();
    fn_801E0318((s32)(uintptr_t)&fn_8010F7F8);   // lint:ok
    fn_801E0320((s32)(uintptr_t)&fn_8010CD18);   // lint:ok
    fn_801E0328((s32)(uintptr_t)&fn_8010F784);   // lint:ok
    ScriptSigsInit(&fn_80106884);
    fn_801DC870();
    RegisterHooks();
    fn_800E75B0();
    RegisterScriptFunctions();
    RegisterCharacterStateCallbacks();
}
