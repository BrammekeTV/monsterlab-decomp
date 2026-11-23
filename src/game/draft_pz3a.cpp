// Small functions from the pz3a batch: a sound/stream helper, font object setup, texture-cache
// lookups, a debug-log stub, a 1D interpolation step, and a few audio and heap helpers.
// Functional, checked against the asm via tools/dump.sh.
#include <string.h>
#include "types.h"

typedef u8* (*VFnRetPtr)(u8*);
typedef u8* (*HeapAllocFn)(s32, s32, s32, s32, s32);

extern "C" {
// Callees and small externs (prototypes copied from their definitions where they exist).
void AudioSetPaused(void* audio, int paused);                          // 0x801FF708
s32 LoadingScreenTick2(int unused, f32 dt);                             // 0x8000E390
void* MemAllocDefault(u32 size);                                        // 0x801D4650
void TextureRelease(void* tex);                                         // 0x80184028
void* fn_80109330(const char* name);                                    // 0x80109330
s32 fn_80146808(u8* file, const char* name, const char* mode);          // 0x80146808
s32 fn_80146930(u8* file, void* buf, s32 size);                         // 0x80146930
void fn_801469FC(u8* file);                                             // 0x801469FC
s32 fn_80146A58(u8* file);                                              // 0x80146A58
void* fn_80150DD4(void);                                                // 0x80150DD4
void fn_80151014(void);                                                 // 0x80151014
void fn_80151244(void);                                                 // 0x80151244
u8* fn_801736EC(void);                                                  // 0x801736EC (return type assumed)
void fn_80174AC4(u8* p);                                         // 0x80174AC4
void* fn_8018D428(s32 size, const char* file, void* tag, s32 line);     // 0x8018D428 (alloc)
void fn_8018D2B8(const char* fmt, ...);                                 // 0x8018D2B8 (debug log)
void fn_801BA638(void* p);                                              // 0x801BA638
void fn_801BB478(u8* p, s32 a1);                                        // 0x801BB478
void fn_80234418(void* p);                                              // 0x80234418
s32 WPADControlSpeaker(s32 a, s32 b, s32 c);                                   // 0x802737A8 (args assumed)

extern u8 gAudio[];                    // audio manager object
extern u8 lbl_803821D0[];              // vtable of the font object
extern u8 lbl_803821C0[];              // source-file name string
extern u8 lbl_803813DC[];              // debug-log format string
extern s32 lbl_805F93A8;               // heap tag for the font allocation
extern f32 lbl_805F98C8;               // float scale constant
extern f32 lbl_80600070;               // loading-screen tick delta constant
extern f32 lbl_80600354;               // lower clamp bound
extern f32 lbl_80600358;               // upper clamp bound
extern f32 lbl_80600370;               // 32768.0 style scale constant
extern f32 lbl_805FF648;               // float default for the object's float fields
extern f32 lbl_805FF64C;               // float default for the object's scale field
extern f32 lbl_805FFB68;               // float zero
extern s32 lbl_805FB550;               // stride element count
extern s32 lbl_805FB554;               // row count
extern u8* lbl_805FB608;               // heap object (slot +0 = alloc)
extern u8 lbl_8051E600[];              // object passed to fn_80234418
extern u8 lbl_8051E680[];              // 0x28-byte records
extern u8 lbl_8051E6A8[];              // sound voices, 0x114 each
extern u8 lbl_805FBAB8;                // flag byte, cleared after fn_80234418
extern s32 lbl_805FBABC;               // count of unpaused looping voices
extern s32 lbl_805FBAC0;               // saved copy of lbl_805FBABC
extern s32 lbl_805FBAE0;               // audio busy flag
extern u8 lbl_805F96B8[];              // file mode string used by fn_801EE394
}

extern "C" {

// Copies the current slice of the row buffer into dst, one row every arg2 bytes.
// arg0 is unused by the original.
// 0x8015DE90
void fn_8015DE90(void* arg0, u8* arg1, s32 arg2) {
    s32 i = 0;
    s32 off = 0;
    (void)arg0;

    fn_80151014();
    fn_80151244();
    while (i < lbl_805FB554) {
        u8* src = (u8*)fn_80150DD4();
        memcpy(arg1 + off, src + i * lbl_805FB550 * 2, lbl_805FB550 * 2);
        off += arg2;
        i += 1;
    }
}

// Allocates and initialises a 0xC4-byte object. arg0 selects whether a value from
// fn_801736EC is stored at +0x4 (otherwise zero).
// 0x801724D4
u8* fn_801724D4(s32 arg0) {
    u8* obj = ((HeapAllocFn)(*(void**)lbl_805FB608))(0xC4, 0x10, 0xB, 0, 0);
    *(s32*)(obj + 0x0) = 0x47;
    if (arg0 != 0) {
        *(u8**)(obj + 0x4) = fn_801736EC();
    } else {
        *(s32*)(obj + 0x4) = 0;
    }
    *(s32*)(obj + 0xB0) = 0;
    *(s32*)(obj + 0x38) = 0;
    *(s32*)(obj + 0xB4) = -1;
    *(s32*)(obj + 0x10) = 0;
    *(s32*)(obj + 0x30) = 0;
    *(s32*)(obj + 0x34) = 0;
    *(s32*)(obj + 0xB8) = 0;
    *(s32*)(obj + 0xBC) = 0;
    *(s32*)(obj + 0xC0) = 0;
    *(s32*)(obj + 0xC) = 0;
    *(f32*)(obj + 0x8) = lbl_805FF64C;
    fn_80174AC4(obj + 0x3C);
    *(f32*)(obj + 0x14) = lbl_805FF648;
    *(f32*)(obj + 0x18) = lbl_805FF648;
    *(f32*)(obj + 0x1C) = lbl_805FF648;
    *(f32*)(obj + 0x20) = lbl_805FF648;
    *(f32*)(obj + 0x24) = lbl_805FF648;
    *(f32*)(obj + 0x28) = lbl_805FF648;
    *(f32*)(obj + 0x2C) = lbl_805FF648;
    return obj;
}

// Sets the vtable of a font object, allocates its 0x20-byte payload and runs the
// font init on it. Returns the object. name and tag are only passed on to the
// init in the original, and the callee does not read them (see fn_801BA638).
// 0x80196E98
u8* fn_80196E98(u8* obj, const char* name, void* tag) {
    u8* p;
    (void)name;
    (void)tag;

    *(u8**)obj = lbl_803821D0;
    p = (u8*)fn_8018D428(0x20, (const char*)lbl_803821C0, &lbl_805F93A8, 0x1F);
    *(u8**)(obj + 0x8) = p;
    if (p != NULL) {
        fn_801BA638(p);
    }
    *(u8**)(obj + 0x8) = p;
    return obj;
}

// Returns 0 if self+0x64 lacks bit 0x200. Otherwise walks the list linked through +0x128
// starting at self+0x128: returns 0 when an entry has +0x60 == 0 and bit 0x1000 set in
// +0x64, and 1 when the walk finishes without finding one.
// 0x8019CCAC
s32 fn_8019CCAC(u8* self) {
    u8* p;

    if ((*(u32*)(self + 0x64) & 0x200) != 0x200) {
        return 0;
    }
    p = *(u8**)(self + 0x128);
    while (p != NULL) {
        if (*(s32*)(p + 0x60) == 0 && (*(u32*)(p + 0x64) & 0x1000) == 0x1000) {
            return 0;
        }
        p = *(u8**)(p + 0x128);
    }
    return 1;
}

// Advances a float parameter of arg0 by farg0 * (arg0 +0xC * arg0 +0x8) and clamps it to
// arg0 +0x0 when it overshoots, while arg1 supplies the current value via its vtable (slot 0xC).
// Returns 3 when the step has finished, or arg0's +0x10 otherwise.
// 0x801B7F40
s32 fn_801B7F40(u8* arg0, u8* arg1, f32 farg0) {
    f32 var_f31;
    s32 var_r31 = 0;

    if (*(f32*)(arg0 + 0x70) < *(f32*)(arg0 + 0x4)) {
        f32 step = *(f32*)(arg0 + 0xC) * *(f32*)(arg0 + 0x8);
        u8* cur = (*(VFnRetPtr*)(*(u8**)arg1 + 0xC))(arg1);
        var_f31 = farg0 * step + *(f32*)(cur + 0x30);
        if ((*(f32*)(arg0 + 0xC) > lbl_805FFB68 && var_f31 > *(f32*)(arg0 + 0x0)) ||
            (*(f32*)(arg0 + 0xC) < lbl_805FFB68 && var_f31 < *(f32*)(arg0 + 0x0))) {
            var_f31 = *(f32*)(arg0 + 0x0);
        }
    } else {
        var_f31 = *(f32*)(arg0 + 0x0);
        var_r31 = 1;
    }
    {
        u8* cur = (*(VFnRetPtr*)(*(u8**)arg1 + 0xC))(arg1);
        *(f32*)(cur + 0x30) = var_f31;
    }
    if (var_r31 != 0) {
        return 3;
    }
    return *(s32*)(arg0 + 0x10);
}

// Opens the file, reads its contents into a freshly allocated buffer (arg0+0x4, size at +0x8).
// Shows the loading ticks in between. Returns 1 when the full size was read.
// 0x801EE394
s32 fn_801EE394(u8* arg0, const char* name) {
    u8 file[0x60];
    s32 got;
    u8* buf;

    if (fn_80146808(file, name, (const char*)lbl_805F96B8) == 0) {
        return 0;
    }
    *(s32*)(arg0 + 0x8) = fn_80146A58(file);
    LoadingScreenTick2(0, lbl_80600070);
    LoadingScreenTick2(0, lbl_80600070);
    buf = (u8*)MemAllocDefault((u32)*(s32*)(arg0 + 0x8));
    *(u8**)(arg0 + 0x4) = buf;
    if (buf == NULL) {
        fn_801469FC(file);
        return 0;
    }
    LoadingScreenTick2(0, lbl_80600070);
    got = fn_80146930(file, *(void**)(arg0 + 0x4), *(s32*)(arg0 + 0x8));
    LoadingScreenTick2(1, lbl_80600070);
    fn_801469FC(file);
    return *(s32*)(arg0 + 0x8) == got;
}

// Clamps farg0 to [lbl_80600354, lbl_80600358], then stores it as a 15-bit fixed value at
// +0x24 on the matching voice record (entry with first word == arg1, found within 0x5E
// records of lbl_8051E6A8+0x228), and rescales it into +0x28. audio is unused (r3).
// 0x8020001C
void fn_8020001C(u8* audio, s32 arg1, f32 farg0) {
    f32 clamped;
    s32 saved;
    u8* var_r3;
    s32 ctr;
    (void)audio;

    if (farg0 < lbl_80600354) {
        clamped = lbl_80600354;
    } else if (farg0 > lbl_80600358) {
        clamped = lbl_80600358;
    } else {
        clamped = farg0;
    }

    saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    var_r3 = lbl_8051E6A8 + 0x228;
    ctr = 0x5E;
    do {
        if (var_r3 != NULL && *(u8**)(var_r3 + 0xC) != NULL && *(s32*)(*(u8**)(var_r3 + 0xC)) == arg1) {
            u8* rec = *(u8**)(var_r3 + 0xC);
            *(s32*)(rec + 0x24) = (s32)(lbl_80600370 * clamped);
            *(s32*)(rec + 0x28) = (s32)((f32)*(s32*)(rec + 0x24) * lbl_805F98C8);
            break;
        }
        var_r3 += 0x114;
        ctr -= 1;
    } while (ctr != 0);
    lbl_805FBAE0 = saved;
}

// Pauses the audio manager and does the per-frame audio housekeeping: bumps its pause
// counter, drops the looping voice count (and remembers it when it was zero), clamps
// the lbl_8051E680 record to 6 and releases the pending object.
// 0x802030A8
void fn_802030A8(void) {
    s32 saved;

    AudioSetPaused(gAudio, 1);
    *(u8*)(gAudio + 0x29) = 0;
    *(s32*)(gAudio + 0x2B4) = *(s32*)(gAudio + 0x2B4) + 1;
    if (lbl_805FBABC == 0) {
        saved = lbl_805FBAE0;
        lbl_805FBAE0 = 1;
        if (WPADControlSpeaker(0, 2, 0) == 0) {
            lbl_805FBAC0 = lbl_805FBABC;
        }
        lbl_805FBAE0 = saved;
    }
    if (*(s32*)(lbl_8051E680 + 0x24) > 0) {
        *(s32*)(lbl_8051E680 + 0x24) = 6;
    }
    if (lbl_805FBAB8 != 0) {
        fn_80234418(lbl_8051E600);
        lbl_805FBAB8 = 0;
    }
}

// Looks up a cached texture entry by name. If the entry matches the requested
// size, format and dimensions, copies its data into dst. Drops one reference and
// releases the entry when the count reaches zero. dst is cleared first.
// 0x80206E40
void fn_80206E40(const char* name, u8* dst, s32 w, s32 h, s32 size, s32 fmt) {
    u8* ent;
    s32 refs;

    if (dst == NULL) {
        return;
    }
    memset(dst, 0, (u32)size);
    ent = (u8*)fn_80109330(name);
    if (ent != NULL) {
        u8* p = ent + 4;
        if (p != NULL && *(u32*)(p + 0x14) == (u32)size && w == (s32)*(u16*)(p + 0x4) &&
            h == (s32)*(u16*)(p + 0x6) && fmt == (s32)*(u8*)(p + 0x9)) {
            memcpy(dst, *(void**)(p + 0x18), (u32)size);
        }
        refs = *(s32*)(ent + 0x24) - 1;
        *(s32*)(ent + 0x24) = refs;
        if (refs == 0) {
            TextureRelease(ent);
        }
    }
}

// Returns 1 unless the entry at self + arg1*0x14 + arg2*4 + 0x80 is -1 (the bit trick
// tests for an all-ones value: 0 when t == -1, else 1).
// 0x80215274
s32 fn_80215274(u8* arg0, s32 arg1, s32 arg2) {
    s32 t = *(s32*)(arg0 + arg1 * 0x14 + arg2 * 4 + 0x80);
    return (s32)(((u32)((-1 - t) | (t + 1))) >> 31);
}

// Copies three floats from the source object (sets->+0x7C) into out. When the source is
// missing, logs the debug string instead (no float args).
// 0x8018D68C
void fn_8018D68C(f32* out, void* sets) {
    u8* src = *(u8**)((u8*)sets + 0x7C);

    if (src != NULL) {
        *(f32*)((u8*)out + 0x0) = *(f32*)(src + 0x68);
        *(f32*)((u8*)out + 0x4) = *(f32*)(src + 0x6C);
        *(f32*)((u8*)out + 0x8) = *(f32*)(src + 0x70);
        return;
    }
    fn_8018D2B8((const char*)lbl_803813DC, (void*)src);
}

// Pops the next entry off the queue at arg0: when the queue is non-empty, sets the
// cursor to -1 and to (first byte of arg1 - 1) and restarts from the first step (1);
// otherwise restarts with 0. The callee fn_801BB478 does the actual work.
// 0x801BB57C
void fn_801BB57C(u8* arg0, u8* arg1) {
    u8* sub = *(u8**)arg0;

    if (*(u8*)(sub + 0x44) == 0) {
        return;
    }
    if (arg1 != NULL) {
        *(s32*)(arg0 + 0x4) = -1;
        *(s32*)(arg0 + 0xC) = (s32)*arg1 - 1;
        fn_801BB478(arg0, 1);
    } else {
        fn_801BB478(arg0, 0);
    }
}

}  // extern "C"
