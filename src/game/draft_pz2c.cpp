// Small functions from the 0x8020xxxx-0x8021xxxx batch (pz2c): audio volume, file open helper,
// string/table helpers, sequence/record setup helpers.
#include "types.h"

// Globals (C++ linkage, as defined in the C++ leaf and game files).
extern u8 lbl_805F98C0;               // 0x805F98C0 (byte flag)
extern const char lbl_805F98FC[];     // 0x805F98FC (file mode string)
extern u8 lbl_805FB470[];             // 0x805FB470 (function-pointer slot, called with 0)
extern s32 lbl_805FBAA8;              // 0x805FBAA8
extern s32 lbl_805FBAAC;              // 0x805FBAAC
extern const char lbl_80386E80[];     // 0x80386E80 (string appended to the buffer)
extern u16 lbl_805F9918;              // 0x805F9918 (ring counter)
extern u8 lbl_80536280[];             // 0x80536280 (ring buffer, 129 slots of 0x400 bytes)
extern u8 lbl_80556A80[];             // 0x80556A80 (sync object)
extern u16 lbl_805FBB44;              // 0x805FBB44 (counter)
extern u8 gAudio[];                   // audio manager object
extern u8 lbl_80559D78[];             // 0x80559D78 (0x28-byte audio voice records)
extern f32 lbl_806004F0;              // 0x806004F0 (lower volume bound)
extern f32 lbl_806004F4;              // 0x806004F4 (upper volume bound)
extern s32 lbl_80349620[];            // 0x80349620 (s32 table)
extern u8 lbl_80388EE8[];             // 0x80388EE8 (vtable)
extern const char lbl_80388EC8[];     // 0x80388EC8 (string copied into the object)

extern "C" {

// Callee prototypes (C linkage).
s32  fn_80146808(u8* file, const char* name, const char* mode);  // 0x80146808
s32  fn_80146930(u8* p, void* a, s32 c);                         // 0x80146930 (src/engine/small_8014c.cpp)
void fn_801469FC(u8* p);                                         // 0x801469FC (src/engine/small_8014c.cpp)
void fn_80262384(s32 handle, s32 code);                          // 0x80262384 (args from asm, return ignored)
s32  fn_80266944(u8* path, s32* out);                            // 0x80266944 (return used as status)
s32  fn_80267B14(u8* buf);                                       // 0x80267B14
char* strcat(char* d, const char* s);
void* memcpy(void* d, const void* s, u32 n);
char* StrCopy(char* dst, const char* src);                       // 0x8022127C
void fn_8023A5B0(u8* p);                                         // 0x8023A5B0
void fn_8023A68C(u8* p);                                         // 0x8023A68C
u8*  fn_8020F3D4(u8* arg0, s32 arg1);                            // 0x8020F3D4
void fn_8021FF78(u8* p);                                         // 0x8021FF78 (src/leaf)
u16  fn_8021F448(u8* p);                                         // 0x8021F448 (src/leaf)
u32  fn_8021F454(u8* p);                                         // 0x8021F454 (src/leaf)
void fn_8020001C(u8* audio, s32 id, f32 v);                      // 0x8020001C (tail call target, return type assumed void)

// 0x80203DE8
// Resets per-object counters from the state at arg1+0x9C; on state 1 also clears two globals.
// When the flag lbl_805F98C0 is set, flushes pending entries (fn_80262384) and sets arg1+0xB4 to 3.
void fn_80203DE8(u8* arg0, u8* arg1) {
    s32 state;

    if (arg1 == NULL) return;
    state = *(s32*)(arg1 + 0x9C);
    if (state == 0) {
        *(u8*)(arg0 + 0x29) = 0;
        *(u8*)(arg0 + 0x28) = 0;
        *(u8*)(arg0 + 0x1D1) = 0;
        *(u8*)(arg0 + 0x1D0) = 0;
    } else if ((u32)state == 1) {
        lbl_805FBAAC = 0;
        lbl_805FBAA8 = 0;
    }
    if (lbl_805F98C0 != 0) {
        if (*(s32*)(arg1 + 0x8C) != 0) {
            fn_80262384(*(s32*)(arg1 + 0x8C), -0x3C0);
        }
        if (*(s32*)(arg1 + 0x90) != 0) {
            fn_80262384(*(s32*)(arg1 + 0x90), -0x3C0);
        }
        *(s32*)(arg1 + 0xB4) = 3;
    }
}

// 0x80204978
// Opens the named file in read-binary mode and, if that works, reads one item and closes it.
// Otherwise calls the function stored in lbl_805FB470 with 0 (when it is set).
void fn_80204978(const char* name) {
    u8 file[0x60];      // file handle block (size from the stack frame)
    s32 sp8;

    if (fn_80146808(file, name, lbl_805F98FC) != 0) {
        fn_80146930(file, &sp8, 1);
        fn_801469FC(file);
        return;
    }
    {
        void* cb = *(void**)lbl_805FB470;
        if (cb != NULL) {
            ((void (*)(s32))cb)(0);
        }
    }
}

// 0x80206DA4
// Builds a 64-byte path buffer with fn_80267B14, appends lbl_80386E80 and checks it with fn_80266944.
// Returns 1 when that check reports success (status 0), 0 otherwise.
s32 fn_80206DA4(void) {
    u32 buf[16] = {0};
    s32 sp8;

    if (fn_80267B14((u8*)buf) != 0) {
        return 0;
    }
    strcat((char*)buf, lbl_80386E80);
    return fn_80266944((u8*)buf, &sp8) == 0;
}

// 0x80207A2C
// Advances the u16 counter lbl_805F9918, copies one 0x400-byte block from src into the
// 129-slot ring at lbl_80536280 (slot = new counter % 129), then bumps lbl_805FBB44 under the sync object.
// r3 is overwritten before use; only r4 (src) is an input.
void fn_80207A2C(u32 unusedR3, const void* src) {
    u32 slot;

    lbl_805F9918 = (u16)(lbl_805F9918 + 1);
    slot = lbl_805F9918 % 129;
    memcpy(lbl_80536280 + (slot << 10), src, 0x400);
    fn_8023A5B0(lbl_80556A80);
    lbl_805FBB44 = (u16)(lbl_805FBB44 + 1);
    fn_8023A68C(lbl_80556A80);
}

// 0x8020B0AC
// Clamps the volume v to [lbl_806004F0, lbl_806004F4] (NaN passes through, as the original compares do).
// Voice handles tagged 0x0AB (top 12 bits) with index < 0x100 store the volume in their record's
// object (+0x08) and, unless that voice is paused or muted or has no id, forward it to the audio manager.
// Any other handle is forwarded to the audio manager directly.
void fn_8020B0AC(s32 id, f32 v) {
    f32 c;
    u32 idx;
    u8* rec;
    u8* obj;
    s32 flags;
    s32 voiceId;

    if (v < lbl_806004F0) {
        c = lbl_806004F0;
    } else if (v > lbl_806004F4) {
        c = lbl_806004F4;
    } else {
        c = v;
    }

    if (((u32)id & 0xFFF00000u) != 0x0AB00000u) {
        fn_8020001C(gAudio, id, c);
        return;
    }
    idx = (u32)id & 0xFFFFFu;
    if (idx >= 0x100) return;

    rec = lbl_80559D78 + idx * 0x28;
    obj = *(u8**)rec;
    if (obj == NULL) return;
    *(f32*)(obj + 0x8) = c;

    flags = *(s32*)(rec + 0x24);
    if ((flags & 0x2) != 0) return;
    if ((flags & 0x1) != 0) return;

    voiceId = *(s32*)(rec + 0x14);
    if (voiceId == -1) return;
    fn_8020001C(gAudio, voiceId, c);
}

// 0x8020E7C8
// Looks up lbl_80349620[idx]; idx -1 gives -1.
s32 fn_8020E7C8(s32 arg0) {
    if (arg0 == -1) {
        return -1;
    }
    return lbl_80349620[arg0];
}

// 0x8020F564
// Runs fn_8020F3D4 on the object, installs its vtable at lbl_80388EE8 and copies a name string to +0xC.
u8* fn_8020F564(u8* arg0, s32 arg1) {
    fn_8020F3D4(arg0, arg1);
    *(void**)arg0 = lbl_80388EE8;
    StrCopy((char*)(arg0 + 0xC), lbl_80388EC8);
    return arg0;
}

// 0x80214F50
// Returns 1 when the field at +0x7C is 0, otherwise 0 (1 - value, for a flag that is 0 or 1).
s32 fn_80214F50(u8* arg0) {
    return 1 - *(s32*)(arg0 + 0x7C);
}

// 0x8021F464
// Returns the pointer stored at index arg1 of the table pointed to by +0x8.
u8* fn_8021F464(u8* arg0, s32 arg1) {
    u8* base = *(u8**)(arg0 + 0x8);
    return *(u8**)(base + arg1 * 4);
}

// 0x8021F7C0
// Passes the pointer at +0 to fn_8021F454 and returns its low 16 bits.
u32 fn_8021F7C0(u8* arg0) {
    return (u16)fn_8021F454(*(u8**)arg0);
}

// 0x8021F8D8
// Calls fn_8021FF78 on each 0x20-byte entry (base +0x4) for the count reported by fn_8021F448
// of the object at +0, then clears +0x14. The second argument is not read.
void fn_8021F8D8(u8* arg0) {
    s32 i = 0;
    s32 off = 0;

    while (i < (s32)fn_8021F448(*(u8**)arg0)) {
        fn_8021FF78(*(u8**)(arg0 + 0x4) + off);
        off += 0x20;
        i += 1;
    }
    *(s32*)(arg0 + 0x14) = 0;
}

// 0x8021FEFC
// Initialises a cursor block: stores the source (arg1), the entry from fn_8021F464(arg1, arg2) at +0x4,
// +0x8 = entry + 8, zeroes +0x10 and stores arg3, arg4 and arg2. Returns the entry.
u8* fn_8021FEFC(u8* arg0, u8* arg1, s32 arg2, u8* arg3, s32 arg4) {
    u8* temp;

    *(u8**)arg0 = arg1;
    temp = fn_8021F464(arg1, arg2);
    *(u8**)(arg0 + 0x4) = temp;
    *(u8**)(arg0 + 0x8) = temp + 8;
    *(s32*)(arg0 + 0x10) = 0;
    *(u8**)(arg0 + 0x14) = arg3;
    *(s32*)(arg0 + 0x18) = arg4;
    *(s32*)(arg0 + 0xC) = arg2;
    return temp;
}

}  // extern "C"
