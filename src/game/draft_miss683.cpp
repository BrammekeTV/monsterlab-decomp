// Small functions from the draft_miss683 batch (0x800CE6CC, 0x8010C608, 0x801F903C, 0x80197D98,
// 0x80197EC4, 0x801A820C, 0x801ACB7C, 0x801B6444, 0x8020821C, 0x8020832C, 0x8020890C).
#include "types.h"
#include "revolution.h"

// Globals with C++ linkage (defined or declared in C++ files, see the files named below).
extern "C++" void* gEventMgr;       // ai_states.cpp, camera_script.cpp
extern "C++" u8 gFlag805FB948;      // game_state.cpp (two-player mode flag)
extern "C++" s32 lbl_805FBB64;      // disc_error.cpp

extern "C" {
void CameraSetFov(void* camera, f32 fov);                                  // engine.cpp
void EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b); // camera_script.cpp
void* EventPost(s32 id, void* sender);                                     // small_801Bc.cpp
s32 PlaySoundAt(void* sound, void* owner, const void* pos, int b);         // owner: asm proves a pointer, see report
int SysSprintf(char* buf, const char* fmt, ...);                           // engine.cpp
void operator_delete(void* p);                                             // small_800Ec.cpp
void* memcpy(void* d, const void* s, unsigned long n);
char* strcpy(char* d, const char* s);

void fn_8006FAF4(u8* p, s32 a);
u8* fn_800C210C(u8* p, s32 del);                 // small_8009d.cpp
u8* fn_800C42FC(u8* p, s32 del);                 // small_800Cb.cpp
u8* fn_800C7540(u8* p, s32 del);                 // return value unused by callers here
void fn_800C7620(u8* p);
void fn_800CCA88(u8* p);                         // small_800Cc.cpp
u8* fn_800CD9B0(u8* p, s32 del);                 // return value unused by callers here
void fn_800CDC44(u8* p);
void* fn_800CF71C(void);                     // asm passes arg0 in r3; the defs take no arguments
u8* fn_800D01C0(void);                           // src/leaf, takes no arguments
u32 fn_80143974(s32 seed, void* data, u32 len);  // camera_collision.cpp
void fn_801894B8(u8* p);                         // small_8018.cpp
void fn_801894F4(u8* p);
void fn_8018979C(u8*, s8*, s32*, s32*);          // src/leaf
void* fn_8018D428(s32 size, const char* file, void* tag, s32 line);
void fn_8018D4CC(void* p);                       // small_8019.cpp
u32 fn_8019552C(u8* a3, s32 a4);                 // small_8019c.cpp (returns a pointer, declared u32)
void* fn_801D40E0(s32 n);                        // small_801Dc.cpp
s32 fn_801EE46C(u8* handle, const char* path);
void fn_801EE4B8(u8* handle);
s32 fn_801EE504(u8* handle);                     // src/leaf
s32 fn_801EE50C(u8* handle, u8* buf, s32 size, s32 flag);
void fn_801F31A8(void);                          // src/leaf
void fn_801F38E8(s32 a);                         // disc_error.cpp
u32 fn_801F38F8(u8* a3);                         // src/leaf
void fn_801F5724(u32 button);                    // matchup.cpp
void* fn_80209814(void* v);                      // small_801Dc.cpp
s32 fn_80198014(u8* arg0, u8* font, const char* name);
s32 fn_8023A578(u8* p);
void fn_8023A5B0(u8* p);                         // small_8013.cpp
void fn_8023A68C(u8* p);                         // small_8013.cpp
s32 fn_8023A7C0(u8* p);
void fn_8023D3C8(s32 hi, u32 lo);                // small_8014.cpp
s32 fn_80207C5C(u8* arg0, s32 arg_sp0);         // thread entry of fn_8020832C (cast to OSThreadFunc)
u8* fn_801ACB3C(u8* arg0, s32 arg1);            // destructor registered by fn_801ACB7C
void __register_global_object(void* obj, void (*dtor)(void*), void* handle);  // small_801Db.cpp

extern u8 lbl_8035F7B8[];
extern u8 lbl_8047C640[];                        // camera_collision.cpp
extern u8 lbl_80536280[];
extern u8 lbl_80556A80[];
extern u8 lbl_80556AB0[];                        // small_8013.cpp
extern u8 lbl_80556AC8[];                        // small_8013.cpp
extern u8 lbl_804133C0[];
extern u8 lbl_804133CC[];
extern f32 lbl_805FE590;
extern f32 lbl_806001DC;
extern s32 lbl_805F93BC;
extern u16 lbl_805F9918;
extern u16 lbl_805FBB44;
extern s32 lbl_805FB378;                         // small_8013.cpp
extern s32 lbl_805FB380;
extern s32 lbl_805FB384;                         // small_8013.cpp
extern u8 lbl_805FBB35;
extern s32 lbl_805FBB48;                         // small_8013.cpp
extern s32 lbl_805FBB68;
extern s32 lbl_805FBB6C;
extern void* lbl_805FBBAC;                       // small_801Dc.cpp
}

extern "C" {

// 0x800CE6CC
// Resets the object at arg0: stores its tag, resets the camera and event state, tears down the
// sub-objects, and frees arg0 when arg1 > 0. Returns arg0.
u8* fn_800CE6CC(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)(arg0 + 0x4) = lbl_8035F7B8;
        CameraSetFov(lbl_8047C640, lbl_805FE590);
        EventSendTo(gEventMgr, 0x9B50E644u, NULL, 0x9B51AA64u, 0, 0);
        fn_800CDC44(arg0 + 0xA8);
        fn_800CCA88(*(u8**)(arg0 + 0xA4));
        (*(s32 (**)(u8*))(*(u8**)(arg0 + 0x30F4) + 0x10))(arg0 + 0x3020);
        (*(s32 (**)(u8*))(*(u8**)(arg0 + 0x3008) + 0x10))(arg0 + 0x2FFC);
        fn_800C7620(arg0 + 0xF4);
        fn_800CF71C();
        fn_800D01C0();                           // r3 is fn_800CF71C's result here; the callee takes none
        fn_800C42FC(arg0 + 0x3020, -1);
        fn_800C210C(arg0 + 0x2FFC, 0);           // arg0 is non-NULL here, so the asm always takes this call
        fn_800C7540(arg0 + 0xF4, -1);
        fn_800CD9B0(arg0 + 0xA8, -1);
        fn_8006FAF4(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8010C608
// Path point by ring index: maps arg1 (0 .. count-1) to a 0x1C-byte entry of the ring at arg0+0xAF8.
// Returns NULL when the index is out of range or the ring is not valid.
u8* fn_8010C608(u8* arg0, s32 arg1) {
    u8* temp = arg0 + 0xAF8;
    s32 mode = *(s32*)(temp + 0x0);
    s32 start = *(s32*)(temp + 0x8);
    s32 end = *(s32*)(temp + 0xC);
    s32 len = *(s32*)(temp + 0x10);
    s32 wrap = *(s32*)(temp + 0x4);

    if (start < 0 || end < 0) {
        return NULL;
    }
    if (arg1 < 0) {
        return NULL;
    }
    {
        s32 span = end;
        if (span < start) {
            span += len;
        }
        if (arg1 >= (span - start) + 1) {
            return NULL;
        }
    }
    s32 idx;
    if (mode == 1) {
        if (wrap == 0) {
            idx = arg1 + start;
            if (idx >= len) {
                idx -= len;
            }
        } else {
            idx = end - arg1;
            if (idx < 0) {
                idx += len;
            }
        }
    } else if (wrap == 0) {
        idx = end - arg1;
        if (idx < 0) {
            idx += len;
        }
    } else {
        idx = arg1 + start;
        if (idx >= len) {
            idx -= len;
        }
    }
    return *(u8**)(temp + 0x14) + idx * 0x1C;
}

// 0x801F903C
// Sets the 26 floats at +0x2B4..+0x318 to farg0 when the vector arg1 is zero; otherwise sets the
// entry at +0x2B4 + 4*i to farg0 for every i whose 3-float record at +0xA8 + 12*i equals arg1.
void fn_801F903C(u8* arg0, f32* arg1, f32 farg0) {
    if (arg1[0] == lbl_806001DC && arg1[1] == lbl_806001DC && arg1[2] == lbl_806001DC) {
        for (s32 off = 0x2B4; off <= 0x318; off += 4) {
            *(f32*)(arg0 + off) = farg0;
        }
        return;
    }
    for (s32 i = 0; i < *(s32*)(arg0 + 0x1E0); i++) {
        f32* ent = (f32*)(arg0 + 0xA8 + i * 0xC);
        if (arg1[0] == ent[0] && arg1[1] == ent[1] && arg1[2] == ent[2]) {
            *(f32*)(arg0 + 0x2B4 + i * 4) = farg0;
        }
    }
}

// 0x80197D98
// Copies the text range arg1[arg2 .. arg3) and measures it with fn_8018979C; writes the width and
// the height (both scaled by farg0 and divided by the byte at +2 of the font at arg0+8) to
// arg4 and arg5 when they are not NULL.
void fn_80197D98(u8* arg0, const char* arg1, s32 arg2, s32 arg3, f32* arg4, f32* arg5, f32 farg0) {
    char buf[0x1000];   // stack copy of the text range (size from the frame layout)
    s32 spC;
    s32 sp8;

    strcpy(buf, arg1 + arg2);
    buf[arg3 - arg2] = 0;
    fn_8018979C(*(u8**)(arg0 + 0x8), (s8*)buf, &spC, &sp8);
    if (arg4 != NULL) {
        f32 t = (f32)spC * farg0;
        *arg4 = t;
        *arg4 = t / (f32)*(s8*)(*(u8**)(arg0 + 0x8) + 0x2);
    }
    if (arg5 != NULL) {
        f32 t = (f32)sp8 * farg0;
        *arg5 = t;
        *arg5 = t / (f32)*(s8*)(*(u8**)(arg0 + 0x8) + 0x2);
    }
}

// 0x80197EC4
// Loads the font "fonts\<name>.rf" into arg0+0x8 (NULL on failure). The file is read into a new
// UIFont block; the block is freed again when fn_80198014 reports fewer entries than it holds.
void fn_80197EC4(u8* arg0, const char* arg1) {
    char path[0x100];   // stack path buffer (size from the frame layout)
    u8 handle[0x10];    // stack file handle (size from the frame layout)
    s32 size;
    u8* font = NULL;

    SysSprintf(path, "fonts\\%s.rf", arg1);
    if (fn_801EE46C(handle, path) != 0) {
        size = fn_801EE504(handle);
        font = (u8*)fn_8018D428(size, "UIFont.h", &lbl_805F93BC, 0x43);
        fn_801EE50C(handle, font, size, 1);
        fn_801EE4B8(handle);
    }
    if (font == NULL) {
        *(u8**)(arg0 + 0x8) = NULL;
        return;
    }
    fn_801894F4(font);
    fn_801894B8(font);
    if (fn_80198014(arg0, font, arg1) < (s32)*(u8*)(font + 0x3)) {
        fn_8018D4CC(font);
        *(u8**)(arg0 + 0x8) = NULL;
        return;
    }
    *(u8**)(arg0 + 0x8) = font;
}

// 0x801A820C
// Searches the grid (rows at +0x13C, columns at +0x140) of entries at +0x1BC for the first entry
// whose vtable slot 0x44 accepts (farg0, farg1). Stores the column in +0x1F0 and the row in +0x1F4
// (both -1 when nothing matches). An entry is only tried when arg1 != 0 or its flag bit 0 (+0x64) is set.
s32 fn_801A820C(u8* arg0, s32 arg1, f32 farg0, f32 farg1) {
    *(s32*)(arg0 + 0x1F0) = -1;
    *(s32*)(arg0 + 0x1F4) = -1;
    for (s32 row = 0; row < *(s32*)(arg0 + 0x13C); row++) {
        for (s32 col = 0; col < *(s32*)(arg0 + 0x140); col++) {
            u8** slot = (u8**)(uintptr_t)fn_8019552C(*(u8**)(arg0 + 0x1BC), col + row * *(s32*)(arg0 + 0x140));
            u8* ent = (slot != NULL) ? *slot : NULL;
            if (ent != NULL && (arg1 != 0 || (*(s32*)(ent + 0x64) & 1)) &&
                (*(s32 (**)(u8*, f32, f32))(*(u8**)(ent + 0x138) + 0x44))(ent, farg0, farg1) != 0) {
                *(s32*)(arg0 + 0x1F0) = col;
                *(s32*)(arg0 + 0x1F4) = row;
                return 1;
            }
        }
    }
    return 0;
}

// 0x801ACB7C
// Static initializer: registers the global object lbl_804133CC with its destructor fn_801ACB3C.
void fn_801ACB7C(void) {
    __register_global_object(lbl_804133CC, (void (*)(void*))fn_801ACB3C, lbl_804133C0);
}

// 0x801B6444
// Plays the sound arg1 from the emitter arg0 (only when arg1 is not marked by bit 0 of +0x5).
// When the two-player flag is set, it also posts a sound event for the emitter's id.
void fn_801B6444(u8* arg0, u8* arg1, s32 arg2) {
    if (arg1 != NULL && !(*(u8*)(arg1 + 0x5) & 1)) {
        if (gFlag805FB948 != 0 && !(*(u32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x04000000u)) {
            if (fn_801F38F8(arg0) != 0) {
                u8* ev = (u8*)EventPost(0x73, arg0);
                if (ev != NULL) {
                    *(s32*)(ev + 0x4) = -1;
                    *(u8**)(ev + 0x8) = arg1;
                    *(s32*)(ev + 0xC) = arg2;
                }
            } else if ((u32)(*(u32*)(arg0 + 0x30) + 0x10000u) != 0x0000FFFFu) {
                u8* ev = (u8*)EventPost(0x73, NULL);
                if (ev != NULL) {
                    *(s32*)(ev + 0x4) = *(s32*)(arg0 + 0x30);
                    *(u8**)(ev + 0x8) = arg1;
                    *(s32*)(ev + 0xC) = arg2;
                }
            }
        }
        if (lbl_805FBBAC == NULL) {
            void* v = fn_801D40E0(1);
            if (v != NULL) {
                v = fn_80209814(v);
            }
            lbl_805FBBAC = v;
        }
        PlaySoundAt(arg1, arg0, *(const void**)(arg0 + 0x4), arg2);
    }
}

// 0x8020821C
// Pops the oldest entry from the 0x400-byte record ring at lbl_80536280 (under the lock
// lbl_80556A80): checks its CRC, copies its payload to dst and writes the payload size to *outSize.
// Returns 0 on success, -1 when the ring is empty, -2 on a bad record. arg0 (r3) is unused.
s32 fn_8020821C(u8* arg0, u8* dst, u32* outSize) {
    if (lbl_805FBB44 == 0) {
        return -1;
    }
    u32 slotOff = (u32)(((s32)lbl_805F9918 - (s32)lbl_805FBB44 + 1) % 129) << 10;
    u8* rec = lbl_80536280 + slotOff;
    fn_8023A5B0(lbl_80556A80);
    lbl_805FBB44 = (u16)(lbl_805FBB44 - 1);
    fn_8023A68C(lbl_80556A80);
    u32 len = *(u32*)(rec + 0x10);
    if (len > 0x400u) {
        return -2;
    }
    if (fn_80143974(0, rec + 0x18, len - 0x18) != *(u32*)rec) {
        return -2;
    }
    memcpy(dst, rec, *(u32*)(rec + 0x10));
    *outSize = *(u32*)(rec + 0x10);
    return 0;
}

// 0x8020832C
// Starts the background thread (lbl_80536280+0x21760, entry fn_80207C5C, param arg0). If a
// thread is already running (lbl_805FBB64 or lbl_805FBB68), it only sets lbl_805FB384.
void fn_8020832C(u8* arg0) {
    u8* base = lbl_80536280 + 0x20000;
    if (lbl_805FBB64 != 0 || lbl_805FBB68 != 0) {
        fn_801F31A8();
        lbl_805FB384 = 1;
        return;
    }
    fn_801F38E8(0);
    lbl_805FBB64 = 1;
    lbl_805FB380 = 0;
    *(s32*)(arg0 + 0xB4) = 0;
    *(u8*)(arg0 + 0xB0) = 0;
    *(u8*)(arg0 + 0xB1) = 0;
    if (lbl_805FBB35 == 0) {
        fn_8023A578(base + 0x818);
        fn_8023A578(base + 0x800);
        fn_8023A578(base + 0x830);
        fn_8023A578(base + 0x848);
        lbl_805FBB35 = 1;
    }
    lbl_805FBB6C = 1;
    fn_8023A5B0(base + 0x830);
    if (lbl_805FBB48 < 0) {
        lbl_805FBB48 = 0;
    }
    lbl_805FBB48 += 1;
    OSCreateThread((OSThread*)(lbl_80536280 + 0x21760), (OSThreadFunc)fn_80207C5C, arg0,
                   lbl_80536280 + 0x21A78 + 0x2000, 0x2000, 0xE, 1);
    OSResumeThread((OSThread*)(lbl_80536280 + 0x21760));
}

// 0x8020890C
// Per-frame step for the connection lock lbl_80556AB0: while the lock is free and the
// counter at +0xB4 is positive it spins (at most 1000 iterations) and then sets the flag at +0xB0.
void fn_8020890C(u8* arg0) {
    if (fn_8023A7C0(lbl_80556AB0) == 0) {
        if (*(u8*)(arg0 + 0xB0) == 0) {
            s32 spin = 0;
            for (;;) {
                u32 ticks = *(u32*)0x800000F8;
                u32 hi = (u32)(((unsigned long long)(ticks >> 2) * 0x10624DD3ull) >> 32);
                fn_8023D3C8(0, hi >> 6);
                if (fn_8023A7C0(lbl_80556AB0) != 0) {
                    break;
                }
                if (*(s32*)(arg0 + 0xB4) <= 0) {
                    fn_8023A5B0(lbl_80556AB0);
                    break;
                }
                spin += 1;
                if (spin >= 0x3E8) {
                    break;
                }
            }
            if (spin >= 0x3E8) {
                fn_8023A5B0(lbl_80556AC8);
                *(u8*)(arg0 + 0xB0) = 1;
                lbl_805FBB6C = 0;
                if (lbl_805FB378 == 0) {
                    fn_801F5724(0x10000u);
                }
            }
        } else if (*(s32*)(arg0 + 0xB4) <= 0) {
            fn_8023A5B0(lbl_80556AB0);
            *(u8*)(arg0 + 0xB0) = 0;
        }
    } else if (*(u8*)(arg0 + 0xB0) != 0) {
        *(u8*)(arg0 + 0xB0) = 0;
    }
}

}  // extern "C"
