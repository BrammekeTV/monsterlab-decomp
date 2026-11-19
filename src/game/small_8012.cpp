// Small functions in 0x8012xxxx (name entry screen, pad button queries, nunchuk stick, save manager).
#include <string.h>

#include "types.h"
#include "engine/math.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
// Globals
extern u8*   lbl_805FB2B8;        // 0x805FB2B8 name entry state (0x18 bytes)
extern void* lbl_805FBBE0;        // 0x805FBBE0 save block pointer
extern u8    lbl_805FB2D5;        // 0x805FB2D5 save error flag
extern void* lbl_805F8FD0;        // 0x805F8FD0 save-related pointer
extern u8    lbl_803C4298[];      // 0x803C4298 level info block
extern u8    gGame[];             // 0x803B53A0
extern "C++" u8    gIgcMode;            // 0x805FAF64 in-game cutscene mode
extern u8    lbl_803E6C20[];      // 0x803E6C20 object block
extern f32   lbl_803E6C90[];      // 0x803E6C90 three floats
extern f32   lbl_805FEDB8;        // 0x805FEDB8 float constant
extern f32   lbl_805FEDD0;        // 0x805FEDD0 float constant
extern f32   lbl_805FEE18;        // 0x805FEE18 float constant (zero)

// Prototypes for callees
void    fn_800E69EC(s32 flag);                                   // 0x800E69EC
void*   fn_800E6228(void);                                       // 0x800E6228
void*   fn_801D40E0(s32 n);                                      // 0x801D40E0
void    operator_delete(void* p);                                // 0x801D426C
void    fn_8018D1FC(const char* fmt, ...);                                       // 0x8018D1FC
void    fn_8022E3D8(void* save, u8* name);                       // 0x8022E3D8
void    fn_801EFA18(void* p);                                    // 0x801EFA18
s32     fn_8020E4E8(s32 hash);                                   // 0x8020E4E8
u32     fn_80122E88(u32 button);                                 // 0x80122E88
u32     fn_801B94F8(void* obj, u32 flags);                       // 0x801B94F8
u32     fn_801B9B6C(s32 chan);                                   // 0x801B9B6C
u32     PadGetHeld(s32 chan);                                    // 0x801B9B98
void    fn_801229D0(void);                                       // 0x801229D0
void    fn_8032F1E8(u8* obj, void (*cb)(void), s32 a, s32 b, s32 c); // 0x8032F1E8
void    __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count); // 0x8032F2E0
u8* fn_8022ED98(u8* p, s32 del);                         // 0x8022ED98
s32     SaveInitDefault(void);                                   // 0x80011754
void    fn_80265D78(void* p);                                    // 0x80265D78
}

extern "C" {

// 0x80120AAC
// Opens the name entry screen and allocates its 0x18-byte state: name buffer (16 bytes),
// length at +0x10, flag byte at +0x14.
void EnterName_Init(void) {
    UIScreen_Push(0x68B693B2);
    u8* st = (u8*)fn_801D40E0(0x18);
    lbl_805FB2B8 = st;
    memset(st, 0, 0x10);
    *(s32*)(st + 0x10) = 0;
    st[0x14] = 0;
    fn_800E69EC(1);
}

// 0x80120B08
// Closes the name entry screen and frees its state.
void EnterName_Destroy(void) {
    UIScreen_PopScreen(0x68B693B2);
    fn_800E69EC(0);
    operator_delete(lbl_805FB2B8);
    lbl_805FB2B8 = 0;
}

// 0x80120C60
// Returns 1 when the entered name has at least one character after trimming trailing spaces,
// 0 when it is empty. Trimmed characters are zeroed in the buffer.
u32 EnterName_HasValidName(void) {
    u8* st = lbl_805FB2B8;
    s32 n = *(s32*)(st + 0x10);
    if (n <= 0) return 0;
    for (;;) {
        n = *(s32*)(st + 0x10);
        if (n <= 1) return 1;
        if (st[n - 1] != 0x20) return 1;
        n--;
        *(s32*)(st + 0x10) = n;
        st[n] = 0;
    }
}

// 0x80120CC4
// Copies the entered name into the save block (only while a name is being entered).
void EnterName_SaveNameToPlayerName(void) {
    u8* st = lbl_805FB2B8;
    if (st == 0) return;
    fn_8018D1FC((const char*)st); // lint:ok r4 is whatever the caller left (printf-style varargs)
    fn_8022E3D8(lbl_805FBBE0, st);
}

// 0x801208F8
// Maps a hash through fn_8020E4E8 to a slot 0..2 (3 when unknown) stored at obj+0xA650.
// When the slot changes, obj+0xA648 is reset to 0.
void fn_801208F8(u8* obj, s32 hash) {
    s32 v = fn_8020E4E8(hash);
    s32 old = *(s32*)(obj + 0xA650);
    if (v < 0 || v >= 3) v = 3;
    *(s32*)(obj + 0xA650) = v;
    if (v != old) *(s32*)(obj + 0xA648) = 0;
}

// 0x801203C0
// Returns the float at +0x38 of the entry selected by obj+0x925C (index into the pointer
// table at obj+0x9264), converted to an unsigned value. Returns 0 when the index is past
// the count at obj+0xA644.
u32 fn_801203C0(u8* p) {
    s32 idx = *(s32*)(p + 0x925C);
    s32 count = *(s32*)(p + 0xA644);
    if (idx >= count) return 0;
    u8* entry = *(u8**)(p + 0x9264 + idx * 4);
    return (u32)*(f32*)(entry + 0x38);
}

// 0x80120DC0
// Deleting-destructor helper: frees the object when del > 0 and returns it.
void* fn_80120DC0(void* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x801210F8
// Calls fn_801EFA18 on the object, then clears its first word.
void fn_801210F8(void* p) {
    fn_801EFA18(p);
    *(s32*)p = 0;
}

// 0x80121498
// Deleting-destructor helper: frees the object when flags > 0. (The asm also returns the
// pointer in r3; game_spawn.cpp declares this as void, so the return is omitted here.)
void fn_80121498(void* member, s32 flags) {
    if (member != 0 && flags > 0) operator_delete(member);
}

// 0x801229D4
// Sets three floats at lbl_803E6C90, then starts the 0x1C-byte block at lbl_803E6C20
// with callback fn_801229D0 (tail call).
void fn_801229D4(void) {
    lbl_803E6C90[0] = lbl_805FEDB8;
    lbl_803E6C90[1] = lbl_805FEDD0;
    lbl_803E6C90[2] = lbl_805FEDB8;
    fn_8032F1E8(lbl_803E6C20, fn_801229D0, 0, 0x1C, 4);
}

// 0x80122B00
// Releases the slot index stored at obj+0x40 from the table at gGame+0x1C4 (4-byte entries).
// If that index equals the count at gGame+0x1C0, the count drops by one. obj+0x40 becomes 0xFF.
// The second argument is not read.
void fn_80122B00(void* a, u8* b) {
    u8* obj = (u8*)a;
    u32 idx = obj[0x40];
    *(s32*)(gGame + idx * 4 + 0x1C4) = 0;
    u8 count = gGame[0x1C0];
    if (idx == count) gGame[0x1C0] = (u8)(count - 1);
    obj[0x40] = 0xFF;
}

// 0x80122B5C
// Clears the object: four floats at +0x0..+0xC set to lbl_805FEE18, ints at +0x10, +0x14, +0x18 set to 0.
void fn_80122B5C(u8* p) {
    f32 z = lbl_805FEE18;
    *(f32*)(p + 0x0) = z;
    *(f32*)(p + 0x4) = z;
    *(f32*)(p + 0x8) = z;
    *(f32*)(p + 0xC) = z;
    *(s32*)(p + 0x10) = 0;
    *(s32*)(p + 0x14) = 0;
    *(s32*)(p + 0x18) = 0;
}

// 0x80122B9C
// Same as fn_80122B5C, but the int at +0x18 is set to the argument.
void fn_80122B9C(u8* p, s32 v) {
    f32 z = lbl_805FEE18;
    *(f32*)(p + 0x0) = z;
    *(f32*)(p + 0x4) = z;
    *(f32*)(p + 0x8) = z;
    *(f32*)(p + 0xC) = z;
    *(s32*)(p + 0x10) = 0;
    *(s32*)(p + 0x14) = 0;
    *(s32*)(p + 0x18) = v;
}

// 0x80122E2C
// Reads the nunchuk stick into out (x, y, z) and the dead zone into *deadzone.
// In cutscene mode (gIgcMode != 0) everything is set to zero instead.
void NunchukGetStick(void* nunchuk, Vec3* out, f32* deadzone) {
    if (gIgcMode != 0) {
        f32 zero = lbl_805FEE18;
        out->x = zero;
        out->y = zero;
        out->z = zero;
        *deadzone = zero;
    } else {
        u8* n = (u8*)nunchuk;
        out->x = *(f32*)(n + 0x0);
        out->y = *(f32*)(n + 0x4);
        out->z = *(f32*)(n + 0x8);
        *deadzone = *(f32*)(n + 0xC);
    }
}

// 0x80122F30
// True when button a0 is held. Uses fn_80122E88 when fn_800E6228 is set, otherwise fn_801B94F8.
u32 IsButtonDown(u32 a0) {
    u32 mask;
    if (fn_800E6228() != 0) {
        mask = fn_80122E88(a0);
    } else {
        mask = fn_801B94F8(0, a0);
    }
    u32 held = PadGetHeld(*(s32*)(lbl_803C4298 + 0x38));
    return (mask & held) != 0;
}

// 0x8012304C
// True when button a0 was released this frame. Same mask logic as IsButtonDown, against fn_801B9B6C.
u32 WasButtonReleased(u32 a0) {
    u32 mask;
    if (fn_800E6228() != 0) {
        mask = fn_80122E88(a0);
    } else {
        mask = fn_801B94F8(0, a0);
    }
    u32 released = fn_801B9B6C(*(s32*)(lbl_803C4298 + 0x38));
    return (mask & released) != 0;
}

// 0x80123364
// Deleting destructor: destroys the 0x988-byte array of 4 entries at +0xB98, then frees the
// object when del > 0. Returns the object.
void* fn_80123364(u8* p, s32 del) {
    if (p != 0) {
        __destroy_arr(p + 0xB98, (void (*)(void*, s32))fn_8022ED98, 0x988, 4);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801233D0
// Erases the save data: clears the error flag, sets up default save data, then writes the
// options when there is enough free memory. Without enough memory the error flag is set to 1.
void SaveManager_EraseAllData(void) {
    void* save = lbl_805F8FD0;
    lbl_805FB2D5 = 0;
    fn_80265D78(save);
    SaveInitDefault();
    if (SaveManager_HasEnoughFreeMemory() != 0) {
        SaveManager_SaveOptions();
    } else {
        lbl_805FB2D5 = 1;
    }
}

}  // extern "C"
