// Small functions in 0x8005xxxx (conversation/dialogue script callbacks, character race).
#include "types.h"
#include "game/character.h"
#include "game/part.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

// memset is declared the same way as in small_8007.cpp.
extern "C" void* memset(void* p, int c, unsigned long n);

extern u8    gGame[];                 // 0x803B53A0
extern void* gLab;                    // 0x805FBBC8
extern u8    gWorldState[];           // 0x803E6D00
extern void* gEventMgr;               // 0x805FB6C8
extern void* gStringTable2;           // 0x805FB90C
extern u8    lbl_803C2A20[];          // block holding the GameDb pointer at +0xDC8
extern u8    lbl_803C4298[];          // level info block
extern u8    lbl_803C53E8[];          // 0x80-byte text scratch buffer
extern void* lbl_805FB0DC;            // active conversation object
extern void* lbl_805FB0F4;            // pending conversation voice SFX handle
extern u32   lbl_805FB0F0;            // conversation state flags
extern u8    lbl_805FB0E2;            // player option flag
extern s32   lbl_805FB0D0;            // NPC line id
extern s32   lbl_805FB0D8;            // name id
extern u8    lbl_805FB100;            // one-shot flag for the GameDb release
extern void* lbl_805FBBE0;            // save block pointer
extern f32   lbl_805FD2D4;            // float constant
extern f32   lbl_805F8738;            // float global

extern "C" {
void     EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
void     fn_8020A474(s32 handle);                                // 0x8020A474
void     fn_800508EC(void);                                      // 0x800508EC
void     fn_801B55F4(u8* obj, u8* out, s32 idx);                 // 0x801B55F4
void     fn_8033B310(s32 value, u8* buf, s32 n);                 // 0x8033B310 (tail-branches to fn_803340F8)
s32      AnimSetFindClip(void* set, s32 anim);                   // 0x801AC2FC
u8*      SaveGetCurrentMonster(u8* s);                           // 0x8022E170
void     MonsterGetName(u8* m, char* out, s32 size);             // 0x8021C968
void     GetBuildString(char* out);                              // 0x8001426C
void     BufferFree(void* p);                                    // 0x801D4980
s32      fn_800D56C0(u8* a3);                                    // 0x800D56C0
u32      fn_800D5660(u8* a3);                                    // 0x800D5660
s32      fn_800D5680(u8* a3);                                    // 0x800D5680
s32      fn_800D56A0(u8* a3);                                    // 0x800D56A0

// Conversation register table: gGame+0xD634 -> state object, +0x434 -> 4 s32 registers at +0x2C.
static u8* ConversationRegs(void) {
    u8* obj = *(u8**)(gGame + 0xD634);
    return *(u8**)(obj + 0x434);
}

// Stops the pending conversation voice SFX (if any) and clears its handle.
static void ConversationStopVoiceHandle(void) {
    void* h = lbl_805FB0F4;
    if (h != 0) {
        fn_8020A474(*(s32*)h);
        lbl_805FB0F4 = 0;
    }
}

// 0x8005070C
// Sets conversation register idx (0..3) to value. Any pending voice SFX is stopped first.
void set_conversation_reg(s32 idx, s32 value) {
    ConversationStopVoiceHandle();
    if ((u32)idx >= 4) return;
    u8* regs = ConversationRegs();
    if (regs == 0) return;
    *(s32*)(regs + idx * 4 + 0x2C) = value;
}

// 0x8005078C
// Shows a player option string: choice 1 uses the first text id, anything else the second.
void SetPlayerOption(s32 a0, u32 a1) {
    lbl_805FB0E2 = 1;
    u32 textId = (a0 == 1) ? 0xB3EF541Eu : 0x2AE605A4u;
    UIText_SetString_TextID(textId, a1);
}

// 0x800507C8
// Returns 1 if conversation register idx (0..3) equals value, otherwise 0.
s32 Conversation_reg_eq(s32 idx, s32 value) {
    if ((u32)idx >= 4) return 0;
    u8* regs = ConversationRegs();
    if (regs == 0) return 0;
    return *(s32*)(regs + idx * 4 + 0x2C) == value;
}

// 0x80050810
// Sets the NPC line shown in the conversation HUD (line id is also kept in lbl_805FB0D0).
void SetNpcLineID(s32 a0) {
    ConversationStopVoiceHandle();
    UIActivationGrid_SetEnabled(0);
    lbl_805FB0D0 = a0;
    UIText_SetString_TextID(0xF8F01FEC, 0x557);
    UIText_SetString_TextID(0xF8F01FEC, a0);
}

// 0x80050884
// Sets the speaker name text (name id is also kept in lbl_805FB0D8).
void SetNameID(s32 a0) {
    UIText_SetString_TextID(0x009D8A77, a0);
    UIText_SetString_TextID(0xF8F01FEC, 0x557);
    lbl_805FB0D8 = a0;
}

// 0x80050EB0
// Forwards a float (f1) to the conversation's presentation object, if a conversation is active.
void fn_80050EB0(f32 a0) {
    if (lbl_805FB0DC != 0) {
        fn_800508EC();
        u8* obj = *(u8**)(lbl_803C4298 + 4);
        void (*fn)(void*, f32) = *(void (**)(void*, f32))(*(u8**)obj + 0x30);
        fn(obj, a0);
    }
}

// 0x80051298
// Stops the conversation voice SFX.
void ConversationStopVoiceSFX(void) {
    ConversationStopVoiceHandle();
}

// 0x80051658
// Plays a conversation animation on the active conversation and marks it as playing (flag bit 0x2).
void ConversationPlayAnim(u32 a0) {
    u8* obj = (u8*)lbl_805FB0DC;
    if (obj != 0) {
        void (*fn)(void*, u32) = *(void (**)(void*, u32))(*(u8**)obj + 0x84);
        fn(obj, a0);
        lbl_805FB0F0 |= 2;
    }
}

// 0x800516A4
// Queues a conversation animation clip (looked up by name hash) if the actor has an animation set.
void ConversationEnqueAnim(u32 a0) {
    u8* obj = (u8*)lbl_805FB0DC;
    if (obj == 0) return;
    u8* sub = *(u8**)(obj + 4);
    if ((*(u32*)(sub + 0x1C) & 0x10) == 0) return;
    u8* p = *(u8**)(obj + 0x9C);
    if (p == 0) return;
    void* set = *(void**)(p + 0x48);
    if (set == 0) return;
    s32 clip = AnimSetFindClip(set, (s32)a0);
    if (clip < 0) return;
    void (*fn)(void*, s32) = *(void (**)(void*, s32))(*(u8**)obj + 0x98);
    fn(obj, clip);
}

// 0x80051744
// Points the string table at the one used by the active conversation (or the world's level table).
void SetLocalStringTable(void) {
    u8* src = (u8*)lbl_805FB0DC;
    if (src == 0) {
        u8* w = *(u8**)gWorldState;
        src = *(u8**)(w + 8);
    }
    u8* t = *(u8**)(src + 0xA4);
    t = *(u8**)t;
    t = *(u8**)t;
    gStringTable2 = *(void**)(t + 0xC);
}

// 0x8005178C
// Sends an event to the active conversation through the event manager (no-op without a conversation).
void ConversationSignal(u32 a0, BOOL a1, BOOL a2) {
    void* conv = lbl_805FB0DC;
    if (conv == 0) return;
    EventSend(gEventMgr, conv, NULL, a0, a1, a2, 0, 0);
}

// 0x800517F0
// Shows the event popup: sets the popup float and plays its UI animation. The second text
// argument (r4) passes through to UIText_SetString_TextID as written in the original.
void ShowEventPopup(s32 /*r3: overwritten before use*/, u32 a1) {
    lbl_805F8738 = lbl_805FD2D4;
    UIText_SetString_TextID(0x1B191FC3, a1);
    UIElement_PlayUIAnim(0x24542991, 0x5C877396);
}

// 0x80051F04
// Writes the current monster's name into the scratch buffer and returns it.
u8* fn_80051F04(void) {
    u8* m = SaveGetCurrentMonster((u8*)lbl_805FBBE0);
    MonsterGetName(m, (char*)lbl_803C53E8, 0x20);
    return lbl_803C53E8;
}

// 0x80052200
// Clears the scratch buffer, then fills it via fn_8033B310 with the number of save blocks still
// needing to be freed. Returns the buffer.
u8* fn_80052200(void) {
    memset(lbl_803C53E8, 0, 0x80);
    s32 n = SaveManager_GetNumBlocksNeededToBeFreed();
    fn_8033B310(n, lbl_803C53E8, 10);
    return lbl_803C53E8;
}

// 0x8005224C
// Clears the scratch buffer and fills it with the difference between the gLab counter at +0x68
// and the save block counter at +0x128. Returns the buffer.
u8* fn_8005224C(void) {
    memset(lbl_803C53E8, 0, 0x80);
    s32 v = 0;
    if (gLab != 0) {
        v = *(s32*)((u8*)gLab + 0x68) - *(s32*)((u8*)lbl_805FBBE0 + 0x128);
    }
    fn_8033B310(v, lbl_803C53E8, 10);
    return lbl_803C53E8;
}

// 0x800522B8
// Writes the build string into the scratch buffer and returns it.
u8* fn_800522B8(void) {
    GetBuildString((char*)lbl_803C53E8);
    return lbl_803C53E8;
}

// 0x8005230C
// Releases the GameDb buffer. The first call while the GameDb object exists only sets a flag;
// later calls destroy the object (vtable slot 0x8 with -1) and free the buffer. Always clears both.
void fn_8005230C(void) {
    u8* obj = *(u8**)(lbl_803C2A20 + 0xDC8);
    if (obj != 0) {
        if (lbl_805FB100 == 0) {
            lbl_805FB100 = 1;
        } else {
            void (*fn)(void*, s32) = *(void (**)(void*, s32))(*(u8**)obj + 0x8);
            fn(obj, -1);
            BufferFree(gGameDb);
        }
    }
    gGameDb = 0;
    *(void**)(lbl_803C2A20 + 0xDC8) = 0;
}

// 0x8005265C
// Copies the current position into the actor's stored vector (+0x7DC). With an active path index
// (+0x7E8 != -1) the vector comes from fn_801B55F4; otherwise from the object at +4.
void fn_8005265C(u8* obj) {
    s32 idx = *(s32*)(obj + 0x7E8);
    if (idx != -1) {
        u8 tmp[0x34];
        fn_801B55F4(obj, tmp, idx);
        *(f32*)(obj + 0x7DC) = *(f32*)(tmp + 0x24);
        *(f32*)(obj + 0x7E0) = *(f32*)(tmp + 0x28);
        *(f32*)(obj + 0x7E4) = *(f32*)(tmp + 0x2C);
    } else {
        u8* src = *(u8**)(obj + 4);
        *(f32*)(obj + 0x7DC) = *(f32*)(src + 0x0);
        *(f32*)(obj + 0x7E0) = *(f32*)(src + 0x4);
        *(f32*)(obj + 0x7E4) = *(f32*)(src + 0x8);
    }
}

// 0x80053258
// Returns 1 when the actor's id at +0x9C -> +0x8 is one of the three known race/character ids.
s32 fn_80053258(u8* obj) {
    u32 id = *(u32*)(*(u8**)(obj + 0x9C) + 0x8);
    if (id == 0x148685B7 || id == 0x06332A59) return 1;
    return id == 0x84EA3654;
}

// 0x800534C0
// Resets the character's race state from its race data (+0xA00): stores four derived values
// and clears the two counters, then sets the flags at +0x894 (1) and +0x895 (0).
void CharacterResetRace(Character* c) {
    u8* p = (u8*)c;
    u8* race = *(u8**)(p + 0xA00);
    if (race == 0) return;
    *(s32*)(p + 0x87C) = fn_800D56C0(race);
    *(u32*)(p + 0x884) = fn_800D5660(race);
    *(s32*)(p + 0x890) = fn_800D5680(race);
    *(s32*)(p + 0x880) = fn_800D56A0(race);
    *(s32*)(p + 0x88C) = 0;
    *(s32*)(p + 0x888) = 0;
    *(u8*)(p + 0x894) = 1;
    *(u8*)(p + 0x895) = 0;
}
}  // extern "C"
