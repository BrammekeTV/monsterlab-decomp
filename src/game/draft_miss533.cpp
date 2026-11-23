#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void BufferFree(void* p);
void CatchMinigame_UpdateObjects(void* g, f32 dt);
void fn_80021F2C(u8* p, f32 v);
void fn_80022390(void);
void fn_800223D8(void);
void fn_8018D2B8(const char* fmt, ...);
void fn_8020A398(s32 handle);
void operator_delete(void* p);
extern u8 lbl_803C4298[];
extern u8 lbl_803813DC[];
extern u8 lbl_805FB1A0;
extern u8 lbl_805FB1A1;
}

extern "C" {

// 0x8002F6DC
void fn_8002F6DC(u8* arg0, f32 farg0) {
    fn_80021F2C(arg0, farg0);
    CatchMinigame_UpdateObjects(arg0, farg0);
}

// 0x8002FCD4
// Releases the handle at +0xF78 (if set), then runs the shared cleanup fn_80022390.
void fn_8002FCD4(u8* arg0) {
    s32 handle = *(s32*)(arg0 + 0xF78);
    if (handle > 0) {
        fn_8020A398(handle);
    }
    fn_80022390();
}

// 0x8002FD18
// Releases the handle at +0xF78 (if set), then runs the shared cleanup fn_800223D8.
void fn_8002FD18(u8* arg0) {
    s32 handle = *(s32*)(arg0 + 0xF78);
    if (handle > 0) {
        fn_8020A398(handle);
    }
    fn_800223D8();
}

// 0x8007DD38
// Searches the table at +0x28 (count at +0x24, 0x20-byte entries) for the u32 key arg1.
// Returns the entry index, or -1 when not found.
s32 fn_8007DD38(u8* arg0, u32 arg1) {
    u32 count = *(u32*)(arg0 + 0x24);
    u8* table = *(u8**)(arg0 + 0x28);
    u32 i;

    for (i = 0; i < count; i++) {
        if (arg1 == *(u32*)(table + i * 0x20)) {
            return (s32)i;
        }
    }
    return -1;
}

// 0x8007E8B8
// Stops rumble on the channel stored in the global block lbl_803C4298 (+0x38) if it is rumbling.
void fn_8007E8B8(void) {
    if (IsRumbling(*(s32*)(lbl_803C4298 + 0x38)) != 0) {
        StopRumble(*(s32*)(lbl_803C4298 + 0x38));
    }
}

// 0x800DBCCC
// Frees the buffer at +0x4 and clears the pointer.
void fn_800DBCCC(u8* arg0) {
    u8* buf = *(u8**)(arg0 + 0x4);
    if (buf != NULL) {
        BufferFree(buf);
        *(u8**)(arg0 + 0x4) = NULL;
    }
}

// 0x800DD85C
// For states 5..8: sets the state field at +0x8 for 6 and 7, then raises both global flags.
void fn_800DD85C(u8* arg0, s32 arg1) {
    if (arg1 <= 4 || arg1 >= 9) {
        return;
    }
    if (arg1 == 6) {
        *(s32*)(arg0 + 0x8) = 3;
    } else if (arg1 == 7) {
        *(s32*)(arg0 + 0x8) = 4;
    }
    lbl_805FB1A1 = 1;
    lbl_805FB1A0 = 1;
}

// 0x8018D6C8
// Returns the cursor block at +0x7C (offset +0x68), or NULL with a debug log if no cursor.
// The asm does not set r4 before the log call, so arg1 (the caller's r4) is forwarded.
u8* UIEngine_GetCursorPos(u8* arg0, s32 arg1) {
    u8* cursor = *(u8**)(arg0 + 0x7C);
    if (cursor != NULL) {
        return cursor + 0x68;
    }
    fn_8018D2B8((const char*)lbl_803813DC, arg1);
    return NULL;
}

// 0x800CD9B0
// Deletes arg0 when it is non-null and the delete flag arg1 is positive; returns arg0.
u8* fn_800CD9B0(u8* arg0, s32 arg1) {
    if (arg0 != NULL && arg1 > 0) {
        operator_delete(arg0);
    }
    return arg0;
}

// 0x800DBBA4
// Deletes arg0 when it is non-null and the delete flag arg1 is positive; returns arg0.
u8* fn_800DBBA4(u8* arg0, s32 arg1) {
    if (arg0 != NULL && arg1 > 0) {
        operator_delete(arg0);
    }
    return arg0;
}

// 0x800DBBE4
// Deletes arg0 when it is non-null and the delete flag arg1 is positive; returns arg0.
u8* fn_800DBBE4(u8* arg0, s32 arg1) {
    if (arg0 != NULL && arg1 > 0) {
        operator_delete(arg0);
    }
    return arg0;
}

// 0x800DBC24
// Deletes arg0 when it is non-null and the delete flag arg1 is positive; returns arg0.
u8* fn_800DBC24(u8* arg0, s32 arg1) {
    if (arg0 != NULL && arg1 > 0) {
        operator_delete(arg0);
    }
    return arg0;
}

// 0x8019CE9C
// Deletes arg0 when it is non-null and the delete flag arg1 is positive; returns arg0.
u8* fn_8019CE9C(u8* arg0, s32 arg1) {
    if (arg0 != NULL && arg1 > 0) {
        operator_delete(arg0);
    }
    return arg0;
}

// 0x8019F978
// Deletes arg0 when it is non-null and the delete flag arg1 is positive; returns arg0.
u8* fn_8019F978(u8* arg0, s32 arg1) {
    if (arg0 != NULL && arg1 > 0) {
        operator_delete(arg0);
    }
    return arg0;
}

}  // extern "C"
