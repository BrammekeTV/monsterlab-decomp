// Save-file helpers, script signature table, screen-set draw, save event queue and the
// saved-game memory card callback. Verified against the asm (tools/dump.sh).
#include <stdint.h>
#include <string.h>

#include "revolution.h"
#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C++" void* gScreenSets;   // 0x805FB654 (game_ui.cpp)
extern void* lbl_805FB658;        // 0x805FB658 (game_ui.cpp), global render object
extern u8 gGame[];                // 0x803B53A0
extern u8 lbl_805FB238[];         // 0x805FB238, byte flag
extern u8 lbl_80386C10[];         // 0x80386C10
extern u8 lbl_80527140[];         // 0x80527140, save buffer
extern u8 gScriptSigs[];          // script signature table, 0x34 bytes per entry

extern "C" {
void   BufferFree(void* p);
void*  MemAllocDefault(u32 size);
s32    fn_8019600C(void* gx);                         // 0x8019600C
void   fn_801960E4(void* gx);                         // 0x801960E4, result unused by caller
void   fn_801985BC(void* set, s32 pass);              // 0x801985BC, result unused by caller
void   fn_8019BFF8(void* obj, void* cb, void* arg);   // 0x8019BFF8
s32    fn_8018CD34(void* obj);                        // 0x8018CD34, draw callback
u32    fn_80143974(s32 seed, void* data, u32 len);    // 0x80143974, checksum
s32    fn_80265F80(u8* h, void* buf, u32 n);          // 0x80265F80
s32    fn_80266060(u8* h, void* buf, u32 n);          // 0x80266060
s32    fn_80266614(u8* h, u32* out);                  // 0x80266614
s32    fn_80267A54(u32* buf);                         // 0x80267A54
s32    fn_80267B14(u32* buf);                         // 0x80267B14
s32    fn_80265D78(void* p);                          // 0x80265D78
void   fn_80206B6C(s32 code, const u8* msg);          // 0x80206B6C
void   ScriptSigFinalize(u8* sig);                    // 0x801DC658
void   NoticePosted(u8* s, s32 idx);                  // 0x8022DF3C
}

extern "C" {

// 0x8020681C
// Reads a save block: allocates an aligned buffer, reads it through the NAND wrapper,
// checks the checksum word and copies the payload out. Returns the byte count or a negative error.
s32 SaveRead(s32 handle, void* buf, u32 size) {
    (void)handle;  // unused: the wrapper always uses lbl_80527140
    u32 total = (size + 0x23) & ~(u32)0x1F;
    s32 result = -1;
    u8* raw = (u8*)MemAllocDefault(total + 0x20);
    if (raw != NULL) {
        u32 sz = 0;
        if (fn_80266614(lbl_80527140, &sz) == 0 && sz == total) {
            u8* data = (u8*)(((uintptr_t)raw + 0x1F) & ~(uintptr_t)0x1F);
            result = fn_80265F80(lbl_80527140, data, total);
            if (result > 0) {
                if (*(u32*)data == fn_80143974(0, data + 4, total - 4)) {
                    memcpy(buf, data + 4, size);
                } else {
                    result = -4;
                }
            }
        } else {
            result = -4;
        }
        BufferFree(raw);
    }
    return (result < (s32)size) ? result : (s32)size;
}

// 0x8020690C
// Writes a save block: copies the payload behind a checksum word, then sends it through the wrapper.
// Returns the wrapper's byte count, or -1.
s32 SaveWriteData(s32 handle, const void* buf, u32 size) {
    (void)handle;  // unused: the wrapper always uses lbl_80527140
    u32 total = (size + 0x23) & ~(u32)0x1F;
    s32 result = -1;
    u8* raw = (u8*)MemAllocDefault(total + 0x20);
    if (raw != NULL) {
        u8* data = (u8*)(((uintptr_t)raw + 0x1F) & ~(uintptr_t)0x1F);
        memset(data, 0, total);
        memcpy(data + 4, buf, size);
        *(u32*)data = fn_80143974(0, data + 4, total - 4);
        result = fn_80266060(lbl_80527140, data, total);
        if (result <= 0) result = -1;
        BufferFree(raw);
    }
    return result;
}

// 0x802069CC
// Sets up the save file name and shows the save error banner if the device check fails.
// Returns -1 when the error banner was shown, else 0.
s32 SaveSetup(const char* name) {
    u32 buf[16];
    memset(buf, 0, sizeof buf);
    s32 r = fn_80267B14(buf);
    if (r != 0) {
        fn_80206B6C(r, lbl_80386C10 + 0x38);
    } else {
        r = fn_80267A54(buf);
        if (r != 0) fn_80206B6C(r, lbl_80386C10 + 0x48);
    }
    r = fn_80265D78((void*)name);
    if (r != 0) {
        fn_80206B6C(r, lbl_80386C10 + 0x80);
        return -1;
    }
    return 0;
}

// 0x8010A38C
// Memory card callback: clears the flag byte and copies the received 0xCFA8-byte save block
// into gGame at +0x5C8.
void SavedGameMCReceivedCallback(u8* a0) {
    lbl_805FB238[0] = 0;
    memcpy(gGame + 0x5C8, *(void**)(a0 + 0x8), 0x10000 - 0x3058);
}

// 0x8018CD70
// Draws the screen sets for one pass. Pass 2 also runs the object's draw slot 0x1C and the
// fn_8019BFF8 callback, then finishes with two GX pipe command pairs.
// Returns -1 when there is nothing to draw, else 0.
s32 ScreenSetsDraw(void* sets, s32 pass) {
    u8* s = (u8*)sets;
    if (gScreenSets == NULL) return -1;
    if (*(s32*)(s + 0x84) == 0) return -1;
    fn_8019600C(lbl_805FB658);
    fn_801960E4(lbl_805FB658);
    fn_801985BC(*(void**)(s + 0x84), pass);
    if (pass == 2) {
        u8* obj = *(u8**)(s + 0x7C);
        if (obj != NULL && (*(s32*)(obj + 0x64) & 2)) {
            u8* sub = *(u8**)(obj + 0x138);
            s32 (*slot)(void*) = (s32 (*)(void*))(*(void**)(sub + 0x1C));
            slot(obj);
            obj = *(u8**)(s + 0x7C);
            if (*(s32*)(obj + 0x60) == 0) fn_8019BFF8(obj, (void*)fn_8018CD34, NULL);
        }
    }
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE00000F;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x80000005;
    return 0;
}

// 0x801DC7D4
// Fills one entry of the script signature table (0x34 bytes, indexed by id) and finalizes it.
// Ignores ids >= 0x211 and argc > 8.
void ScriptSigDefine(u32 id, u32 ret, u32 argc, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4,
                     u32 a5, u32 a6, u32 a7) {
    if (id >= 0x211) return;
    if (argc > 8) return;
    u8* sig = gScriptSigs + id * 0x34;
    *(u32*)(sig + 0x0) = id;
    *(u32*)(sig + 0x4) = ret;
    *(u32*)(sig + 0x8) = argc;
    *(s32*)(sig + 0xC) = -1;
    *(u32*)(sig + 0x14) = a0;
    *(u32*)(sig + 0x18) = a1;
    *(u32*)(sig + 0x1C) = a2;
    *(u32*)(sig + 0x20) = a3;
    *(u32*)(sig + 0x24) = a4;
    *(u32*)(sig + 0x28) = a5;
    *(u32*)(sig + 0x2C) = a6;
    *(u32*)(sig + 0x30) = a7;
    ScriptSigFinalize(sig);
}

// 0x8022DEE8
// Appends an event to the save queue (up to 15 entries of 0x1C bytes at +0x744) and posts a notice.
void SaveQueueEvent(u8* s, u32 a, u32 b, u32 c, s32 d, s32 e, s32 f) {
    s32 n = *(s32*)(s + 0x8E8);
    if (n >= 0xF) return;
    u8* ev = s + n * 0x1C;
    *(u32*)(ev + 0x744) = a;
    *(s32*)(ev + 0x748) = d;
    *(s32*)(ev + 0x74C) = e;
    *(u32*)(ev + 0x750) = b;
    *(u32*)(ev + 0x754) = c;
    *(s32*)(ev + 0x758) = f;
    s32 cnt = *(s32*)(s + 0x910) + 1;
    *(s32*)(s + 0x910) = cnt;
    *(s32*)(ev + 0x75C) = cnt;
    *(s32*)(s + 0x8E8) = n + 1;
    NoticePosted(s, n);
}

}  // extern "C"
