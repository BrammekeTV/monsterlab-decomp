// Misc small game functions (script callbacks, FSM bit helpers, loading anim, name entry).
#include "types.h"
#include "game/node_ref.h"
#include "game/part.h"

// Virtual call through a vtable slot: (*(Fn*)(*(u8**)obj + OFF))(obj, ...)
typedef void (*FnVoidObjU32)(void*, u32);
typedef void (*FnVoidObjPtr)(void*, void*);
typedef void (*FnVoidObjS32)(void*, s32);
typedef void (*FnVoidObjSlot50)(void*, void*, s32, s32, s32, f32);
typedef s32  (*FnS32Obj)(void*);

struct ScopedLock { u8 pad[4]; };

void GameRequestModeState(s32 mode, s32 sub);  // C++ linkage (game_flow.cpp)

extern "C" {
#include "game/script_callbacks.h"

void  GameSetNextMode(s32 mode);
void  PartCopy(const PartInstance* src, PartInstance* dst);
void  ScopedLockCtor(ScopedLock* l);
void  ScopedLockDtor(ScopedLock* l, s32 flags);
void* ScreenSetFindWidget(void* set, u32 hash);
void  UISetText(void* el, const char* text, s32 a);
void* UIWidgetFindChild(void* widget, u32 hash);
void* fn_800701A4(void);
BOOL  fn_80142358(void);
void  fn_801B4160(u8* p);  // 0x801B4160
s32   fn_801C71EC(u8* key);
void* fn_801D40E0(s32 n);
BOOL  fn_801F56FC(s32 flags);
u8    fn_8021EFE8(u8* a3);

extern "C++" void*  gScreenSets;
extern u8*    gSaveSystem;          // 0x805FBBE0
extern u8     gWorldState[];
extern "C++" s32    gZoneResourceSize;
extern s32    lbl_80375900;
extern u8     lbl_803C4298[];
extern char   lbl_803F2230[];
extern u8     lbl_8047C7D8[];
extern u32    lbl_805F81C0;
extern "C++" u8     lbl_805FAECA;
extern u8*    lbl_805FAF78;
extern s32    lbl_805FB408;
extern u8     lbl_805FB710[];
extern f32    lbl_805FD424;
extern f32    lbl_805FED5C;
}

static const u32 kHashNAME = 0x68B693B2u;  // "NAME"

extern "C" {

// 0x8000B7A8
void fn_8000B7A8(u8* arg0, s32 arg1, s32 arg2) {
    *(u8*)(lbl_803C4298 + 0x1D) = (u8)arg1;
    fn_801B4160(lbl_8047C7D8);
    if (!(*(s32*)(lbl_803C4298 + 0x3C) & 1)) {
        GameSetNextMode(0);
    } else {
        if ((u32)arg1 == 0x43U && lbl_805F81C0 == 1U) {
            GameSetNextMode(0);
        }
        GameRequestModeState(0, 6);
    }
    if (arg2 != 0) {
        *(s32*)(lbl_803C4298 + 0x3C) = *(s32*)(lbl_803C4298 + 0x3C) | 0x100;
    } else {
        *(s32*)(lbl_803C4298 + 0x3C) = *(s32*)(lbl_803C4298 + 0x3C) & 0xFFFFFEFF;
    }
    *(s32*)(lbl_803C4298 + 0x18) = 6;
}

// 0x8000E248
void Loading_SetHomeButtonGone(void) {
    lbl_805FAECA = 1;
}

// 0x8005F204
// Object with a sub-record at +4: when the sub-record flags 0x3 are clear, runs the
// slot-0x50 update and sets flag 0x80; if sub-record flag 1 is set, locks and calls slot 0x18.
void fn_8005F204(u8* arg0) {
    s32 flags = *(s32*)(*(u8**)(arg0 + 0x4) + 0x20);
    if (((~flags) & 3) == 0) {
        (*(FnVoidObjSlot50*)(*(u8**)arg0 + 0x50))(arg0, arg0, 0xB, 0, 0, lbl_805FD424);
        *(s32*)(arg0 + 0x838) = *(s32*)(arg0 + 0x838) | 0x80;
        if (*(s32*)(*(u8**)(arg0 + 0x4) + 0x20) & 1) {
            ScopedLock lock;
            ScopedLockCtor(&lock);
            (*(FnVoidObjS32*)(*(u8**)arg0 + 0x18))(arg0, 2);
            ScopedLockDtor(&lock, -1);
        }
    }
}

// 0x801C6BCC
// Returns the record pointer for (zone = byte 1, index = u16 at 2), or NULL if out of range.
u8* fn_801C6BCC(u8* arg0) {
    u8* w = *(u8**)gWorldState;
    u8* zr;
    if ((s32)arg0[1] > *(s32*)(w + 0x4)) {
        return NULL;
    }
    zr = *(u8**)(w + 0x1A4) + arg0[1] * gZoneResourceSize;
    if (*(u16*)(arg0 + 0x2) >= *(u32*)(zr + 0xCC)) {
        return NULL;
    }
    if (fn_801C71EC(arg0) == 0) {
        return NULL;
    }
    zr = *(u8**)(w + 0x1A4) + arg0[1] * gZoneResourceSize;
    return *(u8**)(zr + 0xD0) + (*(u16*)(arg0 + 0x2)) * 0x50;
}

// 0x801DFEA8
// Sets (arg2 != 0) or clears bit arg1 in a bitset of 32-bit words (arg1 < 0x3FF).
void fn_801DFEA8(u8* arg0, s32 arg1, s32 arg2) {
    u32 word;
    s32 bit;
    u8* p;
    if ((u32)arg1 >= 0x3FFU) {
        return;
    }
    word = (u32)arg1 >> 5;
    bit = arg1 & 0x1F;
    p = arg0 + word * 4;
    if (arg2 != 0) {
        *(s32*)(p + 0x4) = *(s32*)(p + 0x4) | (s32)((u32)1 << bit);
        return;
    }
    *(s32*)(p + 0x4) = *(s32*)(p + 0x4) & (s32)~((u32)1 << bit);
}

// 0x801DFF74
// Stores float farg0 into slot arg1 (of 0x20 floats at +4) when arg1 < 0x20.
void fn_801DFF74(u8* arg0, u32 arg1, f32 farg0) {
    if (arg1 < 0x20U) {
        *(f32*)(arg0 + arg1 * 4 + 0x4) = farg0;
    }
}

// 0x801DFFCC
// Adds arg2 to slot arg1 (of 0x20 ints at +4) when arg1 < 0x20.
void fn_801DFFCC(u8* arg0, s32 arg1, s32 arg2) {
    if ((u32)arg1 < 0x20U) {
        *(s32*)(arg0 + arg1 * 4 + 0x4) = arg2 + *(s32*)(arg0 + arg1 * 4 + 0x4);
    }
}

// 0x801ED554
// Builds a 4-byte key from the default record (lbl_805FB710) or from arg1 (mode 2), looks it up,
// and copies the found or default record into arg0.
void fn_801ED554(u8* arg0, u8* arg1) {
    u8 key[4];
    u32 mode;
    u32 w;

    key[0] = lbl_805FB710[0];
    key[1] = lbl_805FB710[1];
    *(u16*)(key + 0x2) = *(u16*)(lbl_805FB710 + 0x2);

    // The reference word is read as a whole so the code does not depend on byte order.
    w = NodeRefWord(arg1);
    mode = NodeRefKind(w);
    if (mode == 2) {
        key[1] = NodeRefByte1(w);
        key[0] = (u8)NodeRefNibble(w);
        *(u16*)(key + 0x2) = NodeRefHalf2(w);
    }
    if (fn_801C71EC(key) != 0) {
        *(u16*)(arg0 + 0x2) = *(u16*)(key + 0x2);
        arg0[1] = key[1];
        arg0[0] = key[0];
        return;
    }
    *(u16*)(arg0 + 0x2) = *(u16*)(lbl_805FB710 + 0x2);
    arg0[1] = lbl_805FB710[1];
    arg0[0] = lbl_805FB710[0];
}

// 0x8020ECB0
// Copies a part record: count and u16 table at +0x20/+0x22, scalar fields, and five part slots
// (each slot copies the part into the 0x30-byte block at +0x4C, or is cleared to -1 / NULL).
void fn_8020ECB0(u8* arg0, u8* arg1) {
    u32 i = 0;
    s32 k;

    *(u16*)(arg0 + 0x20) = *(u16*)(arg1 + 0x20);
    while (i < *(u16*)(arg0 + 0x20)) {
        *(u16*)(arg0 + 0x22 + i * 2) = *(u16*)(arg1 + 0x22 + i * 2);
        i++;
    }
    *(u16*)(arg0 + 0x22 + i * 2) = 0;
    *(s32*)(arg0 + 0x18) = *(s32*)(arg1 + 0x18);
    *(s32*)(arg0 + 0x1C) = *(s32*)(arg1 + 0x1C);
    *(u8*)(arg0 + 0x48) = *(u8*)(arg1 + 0x48);
    *(u8*)(arg0 + 0x49) = *(u8*)(arg1 + 0x49);
    *(u8*)(arg0 + 0x4A) = *(u8*)(arg1 + 0x4A);
    *(s32*)(arg0 + 0x44) = *(s32*)(arg1 + 0x44);
    for (k = 0; k < 5; k++) {
        u8* srcPart = *(u8**)(arg1 + 0x4 + k * 4);
        u8* dstPart = arg0 + 0x4C + k * 0x30;
        if (srcPart != NULL) {
            PartCopy((const PartInstance*)srcPart, (PartInstance*)dstPart);
            *(u8**)(arg0 + 0x4 + k * 4) = dstPart;
        } else {
            *(s32*)(arg0 + 0x4C + k * 0x30) = -1;
            *(u8**)(arg0 + 0x4 + k * 4) = NULL;
        }
    }
}

// 0x80074C10
void LoadExperiment(void) {
    LoadLevel(*(u32*)((u8*)fn_800701A4() + 0x4));
}

// 0x80120FD8
// Creates a loading-anim record (0x38 bytes) from six object pointers; slot 0 gets the
// global vtable and slots 1..5 get a call through the object's sub-record at +0x138.
void* LoadingAnimCreate(void** lines) {
    u8* out = (u8*)fn_801D40E0(0x38);
    s32 i;
    if (out != NULL) {
        *(void**)out = &lbl_80375900;
    }
    for (i = 0; i < 6; i++) {
        *(void**)(out + 0x8 + i * 4) = lines[i];
        if (i != 0) {
            u8* obj = (u8*)lines[i];
            /* two-level load in the asm: sub = obj->0x138, fn = sub->0x30 */
            (*(FnVoidObjS32*)(*(u8**)(obj + 0x138) + 0x30))(obj, 2);
        }
    }
    *(f32*)(out + 0x4) = lbl_805FED5C;
    *(u8*)(out + 0x20) = 0;
    *(f32*)(out + 0x24) = *(f32*)(*(u8**)(out + 0x8) + 0x68);
    *(f32*)(out + 0x28) = *(f32*)(*(u8**)(out + 0x8) + 0x6C);
    *(f32*)(out + 0x2C) = *(f32*)(*(u8**)(out + 0x8) + 0x70);
    *(f32*)(out + 0x30) = *(f32*)(*(u8**)(out + 0x8) + 0x68);
    *(f32*)(out + 0x34) = *(f32*)(*(u8**)(out + 0x1C) + 0x68);
    return out;
}

// 0x8001BEF0
// Looks up the current entry of the list at lbl_805FAF78+0x698 and writes byte a0 to its
// record's +0x1F (record pointer from gSaveSystem table at +0xA58).
void LightningTowerChargeMonster(u32 a0) {
    u8* w = lbl_805FAF78;
    u8* p;
    s32 count;
    s32 idx = -1;
    if (w == NULL || *(s32*)w == 0) {
        return;
    }
    p = w + 0x698;
    count = *(s32*)(p + 0x198);
    if (count > 0) {
        s32 i = *(s32*)(p + 0x190);
        if (i < count) {
            idx = *(s32*)(p + i * 4);
        }
    }
    if (idx < 0) {
        return;
    }
    *(u8*)(*(u8**)(gSaveSystem + idx * 4 + 0xA58) + 0x1F) = (u8)a0;
}

// 0x8001DCEC
// Returns 1 if the current entry's record passes fn_8021EFE8, else 0.
s32 LightningMonsterCharged(void) {
    u8* w = lbl_805FAF78;
    u8* p;
    s32 count;
    s32 idx = -1;
    if (w == NULL || *(s32*)w == 0) {
        return 0;
    }
    p = w + 0x698;
    count = *(s32*)(p + 0x198);
    if (count > 0) {
        s32 i = *(s32*)(p + 0x190);
        if (i < count) {
            idx = *(s32*)(p + i * 4);
        }
    }
    if (idx < 0) {
        return 0;
    }
    if (fn_8021EFE8(*(u8**)(gSaveSystem + idx * 4 + 0xA58)) != 0) {
        return 1;
    }
    return 0;
}

// 0x8013ECD4
// Appends one character (*a0) to the name buffer (max 15) and refreshes the name widget.
void MPNameAddLetter(u8* a0) {
    s32 idx = lbl_805FB408;
    u8* screens;
    if ((u32)idx >= 0xFU) {
        return;
    }
    lbl_803F2230[idx] = (char)*a0;
    lbl_805FB408 = idx + 1;
    lbl_803F2230[idx + 1] = 0;
    screens = *(u8**)((u8*)gScreenSets + 0x88);
    if (screens != NULL) {
        u8* w = (u8*)ScreenSetFindWidget(screens, kHashNAME);
        if (w != NULL) {
            u8* child = (u8*)UIWidgetFindChild(w, kHashNAME);
            if (child != NULL) {
                UISetText(child, lbl_803F2230, 1);
            }
        }
    }
}

// 0x80140700
// Returns 1 when flag 0x100 is set and neither the 0x10 nor the 0x4/0x8 flag check passes
// with fn_80142358; otherwise 0.
ScriptValue LeaveInterlabLink(void) {
    s32 r31 = 0;
    if (fn_801F56FC(0x100) == 0) {
        return 0;
    }
    if (fn_801F56FC(0x10) != 0 && fn_80142358() != 0) {
        r31 = 1;
    } else if ((fn_801F56FC(0x4) != 0 || fn_801F56FC(0x8) != 0) && fn_80142358() != 0) {
        r31 = 1;
    }
    return (r31 == 0) ? 1 : 0;
}

// 0x800AEBDC
// Tail call: if a0 and a0->+0x26C are non-NULL, calls slot 0x84 of the sub-object with a1.
void GrowJuiceTentacle_HeadPlayAnimation(void* a0, u32 a1) {
    u8* sub;
    if (a0 == NULL) {
        return;
    }
    sub = *(u8**)((u8*)a0 + 0x26C);
    if (sub == NULL) {
        return;
    }
    (*(FnVoidObjU32*)(*(u8**)sub + 0x84))(sub, a1);
}

// 0x800EBE2C
// Tail call: returns the result of slot 0x4C of a0's vtable.
s32 IsDead(void* a0) {
    return (*(FnS32Obj*)(*(u8**)a0 + 0x4C))(a0);
}

// 0x801DDDB0
// Tail call: if a0 is non-NULL, calls slot 0xB4 of its vtable with a1.
void Log(void* a0, u32 a1) {
    if (a0 == NULL) {
        return;
    }
    (*(FnVoidObjU32*)(*(u8**)a0 + 0xB4))(a0, a1);
}

// 0x8020F27C
// Stores listener->+4 = round, then tail call to slot 0x10 of the listener's vtable.
void ListenerStart(u8* listener, void* round) {
    *(void**)(listener + 0x4) = round;
    (*(FnVoidObjPtr*)(*(u8**)listener + 0x10))(listener, round);
}

}  // extern "C"
