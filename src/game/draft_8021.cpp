// MIDI-style sequence player in 0x8021Fxxx (sequence table, delta-time stepping, event skipping).
#include "types.h"

typedef s32 (*SeqEventFn)(u8* ev);

extern "C" {
void* MemAllocSmall(u32 size);                                       // 0x801D41CC
void  MemFreeSmall(void* p);                                         // 0x801D4340
u32   StrLength(const char* s);                                      // 0x80221284
void* MemCopy(void* dst, const void* src, u32 n);
void* fn_801D40E0(s32 n);                                            // 0x801D40E0
void  operator_delete(void* p);                                      // 0x801D426C
u8*   fn_8021F32C(u8* p);                                            // 0x8021F32C
void  fn_8021F338(void* p, u8* ev);                                  // 0x8021F338
u8*   fn_8021F350(u8* p, s32 idx);                                   // 0x8021F350
u16   fn_8021F448(u8* seqData);                                      // 0x8021F448
u32   fn_8021F454(u8* seqData);                                      // 0x8021F454
u8*   fn_8021F464(u8* seqData, s32 track);                           // 0x8021F464
void  fn_8021FA50(void);                                             // 0x8021FA50
void  fn_8021FA80(void);                                             // 0x8021FA80
void  fn_8021FAD4(void);                                             // 0x8021FAD4
void  fn_8021FB28(void);                                             // 0x8021FB28
void* fn_8021FEFC(u8* p, u8* seqData, s32 idx, void* cb, s32 arg);   // 0x8021FEFC
void  fn_8021FF9C(u8* p, u8* q);                                     // 0x8021FF9C
void* fn_8032F028(void* mem, void* ctor, s32 a, s32 size, s32 count); // 0x8032F028
s32   fn_80221288(const char* a, const char* b, u32 n);              // 0x80221288
void* fn_802209CC(void* dst, s32 val, u32 n);                        // 0x802209CC
void  fn_80220188(u8* p, u8* buf, s32 arg);                          // 0x80220188

extern u8 lbl_80389160[];            // 0x80389160 (vtable)
extern u8 lbl_80389198[];            // 0x80389198 (vtable)
extern const char* lbl_805FA0C0;     // 0x805FA0C0 (name string pointer, SDA)

u8*  fn_8021F9C0(u8* p, u8* seqData, s32 track, void* cb, void* user);
void fn_8021FB30(u8* p, u32* ticks);
s32  fn_8021FCF4(u8* p, u8* ev);
u8*  fn_8021FDC8(u8* p, u32* outLen);

// 0x8021F368
// Binds the sequence data (arg1) and builds the table of chunk pointers at +8
// (each chunk: u32 size at +4, payload follows an 8 byte header).
u8* fn_8021F368(u8* arg0, u8* arg1) {
    u8* chunk = arg1;
    *(u8**)(arg0 + 0x4) = arg1;
    *(void**)(arg0 + 0x8) = MemAllocSmall(*(u16*)(arg1 + 0xA) * 4);
    for (u16 i = 0; i < *(u16*)(*(u8**)(arg0 + 0x4) + 0xA); i++) {
        chunk = chunk + *(s32*)(chunk + 0x4) + 8;
        *(u8**)(*(u8**)(arg0 + 0x8) + (u32)(i * 4)) = chunk;
    }
    return arg0;
}

// 0x8021F474
// Constructor: sets up one track player per track of the sequence (arg1), decodes each
// track's name into +8/+0xC, scales the length by farg0 and finds the track named lbl_805FA0C0.
u8* fn_8021F474(u8* arg0, u8* arg1, f32 farg0) {
    *(u8**)(arg0 + 0x1C) = lbl_80389160;
    *(u8**)(arg0 + 0x0) = arg1;
    *(s32*)(arg0 + 0x4) = 0;
    *(s32*)(arg0 + 0x8) = 0;
    *(s32*)(arg0 + 0xC) = 0;
    *(s32*)(arg0 + 0x10) = 0;
    *(s32*)(arg0 + 0x14) = 0;
    *(s32*)(arg0 + 0x18) = 0;
    u16 count = fn_8021F448(arg1);
    void* mem = MemAllocSmall((u32)(count << 5) + 0x10);
    *(void**)(arg0 + 0x4) = fn_8032F028(mem, (void*)fn_8021FA50, 0, 0x20, count);
    *(void**)(arg0 + 0x8) = MemAllocSmall((u32)(fn_8021F448(*(u8**)(arg0 + 0x0)) << 2));
    *(void**)(arg0 + 0xC) = MemAllocSmall((u32)(fn_8021F448(*(u8**)(arg0 + 0x0)) << 2));
    s32 i = 0;
    s32 off4 = 0;
    s32 off32 = 0;
    while (i < (s32)fn_8021F448(*(u8**)(arg0 + 0x0))) {
        fn_8021FEFC(*(u8**)(arg0 + 0x4) + off32, *(u8**)(arg0 + 0x0), i, (void*)fn_8021FB28, 0);
        u8* name = fn_8021FDC8(*(u8**)(arg0 + 0x4) + off32, (u32*)(*(u8**)(arg0 + 0xC) + off4));
        off32 += 0x20;
        i++;
        *(u8**)(*(u8**)(arg0 + 0x8) + off4) = name;
        off4 += 4;
    }
    *(u32*)(arg0 + 0x10) = (u32)(farg0 * (f32)fn_8021F454(*(u8**)(arg0 + 0x0)));
    s32 j = 0;
    s32 joff = 0;
    while (j < (s32)fn_8021F448(*(u8**)(arg0 + 0x0))) {
        const char* name = *(const char**)(*(u8**)(arg0 + 0x8) + joff);
        if (name != 0 &&
            fn_80221288(name, lbl_805FA0C0, StrLength(lbl_805FA0C0)) == 0) {
            *(s32*)(arg0 + 0x18) = j;
            break;
        }
        joff += 4;
        j++;
    }
    return arg0;
}

// 0x8021F644
// Deleting destructor: frees the track array (+4, allocated 0x10 before), name table (+8)
// and length table (+0xC), then the object itself if del > 0.
u8* fn_8021F644(u8* arg0, s32 arg1) {
    if (arg0 != 0) {
        u8* tracks = *(u8**)(arg0 + 0x4);
        *(u8**)(arg0 + 0x1C) = lbl_80389160;
        if (tracks != 0) {
            MemFreeSmall(tracks - 0x10);
        }
        void* names = *(void**)(arg0 + 0x8);
        if (names != 0) {
            MemFreeSmall(names);
        }
        void* lens = *(void**)(arg0 + 0xC);
        if (lens != 0) {
            MemFreeSmall(lens);
        }
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8021F6D8
// Advances playback by *arg1 ticks: while still inside the lead-in (+0x14 < +0x10) just
// accumulates; otherwise steps the main track (+0x18), then all other tracks by the ticks
// that were actually consumed. Returns 0 if the main track did not consume them all.
s32 fn_8021F6D8(u8* arg0, u32* arg1) {
    u32 lead = *(u32*)(arg0 + 0x14);
    u32 limit = *(u32*)(arg0 + 0x10);
    s32 ret = 1;
    if (lead < limit) {
        u32 t = lead + *arg1;
        *(u32*)(arg0 + 0x14) = t;
        if (t < limit) {
            return 1;
        }
        *arg1 = t - limit;
    }
    s32 requested = *arg1;
    fn_8021FB30(*(u8**)(arg0 + 0x4) + (*(s32*)(arg0 + 0x18) << 5), arg1);
    if (*arg1 != 0) {
        requested -= *arg1;
        ret = 0;
    }
    u32 i = 0;
    s32 off = 0;
    while (i < fn_8021F448(*(u8**)(arg0 + 0x0))) {
        if (i != *(u32*)(arg0 + 0x18)) {
            u32 ticks = requested;
            fn_8021FB30(*(u8**)(arg0 + 0x4) + off, &ticks);
        }
        off += 0x20;
        i++;
    }
    return ret;
}

// 0x8021F7E8
// Computes the song tempo-based rate: runs track 0 until the first tempo meta event
// (default 0x7A120 us per beat) and returns ticks * 1000000 / tempo.
u32 fn_8021F7E8(u8* arg0) {
    s32 ticks = 1;
    u32 tempo = 0x7A120;
    u8 seq[0x20];
    u16 div = fn_8021F454(*(u8**)arg0);
    u8* buf = (u8*)fn_801D40E0(0x1004);
    if (buf != 0) {
        buf = fn_8021F32C(buf);
    }
    fn_8021F9C0(seq, *(u8**)arg0, 0, (void*)fn_8021FAD4, buf);
    fn_8021FB30(seq, (u32*)&ticks);
    if (*(u32*)(buf + 0x1000) == 1) {
        MemCopy(&tempo, fn_8021F350(buf, 0) + 2, 4);
        tempo &= 0xFFFFFF;
    }
    operator_delete(buf);
    return (u32)(div * 1000000) / tempo;
}

// 0x8021F9C0
// Track reader constructor: binds the track data, event callback (arg3) and user pointer (arg4).
u8* fn_8021F9C0(u8* arg0, u8* arg1, s32 arg2, void* arg3, void* arg4) {
    *(u8**)(arg0 + 0x0) = arg1;
    *(s32*)(arg0 + 0x4) = 0;
    *(s32*)(arg0 + 0x8) = 0;
    *(s32*)(arg0 + 0xC) = arg2;
    *(s32*)(arg0 + 0x10) = 0;
    *(void**)(arg0 + 0x14) = arg3;
    *(void**)(arg0 + 0x18) = arg4;
    *(u8*)(arg0 + 0x1C) = 0;
    u8* track = fn_8021F464(arg1, arg2);
    *(u8**)(arg0 + 0x4) = track;
    *(u8**)(arg0 + 0x8) = track + 8;
    *(s32*)(arg0 + 0x10) = 0;
    *(void**)(arg0 + 0x14) = arg3;
    *(void**)(arg0 + 0x18) = arg4;
    *(s32*)(arg0 + 0xC) = arg2;
    return arg0;
}

// 0x8021FB30
// Steps one track by *arg1 ticks: reads delta/event pairs (variable-length deltas), calls the
// event callback (+0x14) for each due event, handles end-of-track (FF 2F) and the Bx 16 marker.
void fn_8021FB30(u8* arg0, u32* arg1) {
    u32 total = *(u32*)(arg0 + 0x10) + *arg1;
    u32 remain = total;
    s32 extra = 0;
    s32 marker = 0;
    u32 delta = 0;
    u8 b;
    u8* pos = *(u8**)(arg0 + 0x8);
    while ((b = *pos) & 0x80) {
        pos += 1;
        delta = (delta | (b & 0x7F)) << 7;
    }
    u8* ev = pos + 1;
    delta |= b & 0x7F;
    u8* next = ev + fn_8021FCF4(arg0, ev);
    while (remain > delta) {
        if (ev[0] == 0xFF && ev[1] == 0x2F) {
            *arg1 = 0;
            *(u32*)(arg0 + 0x10) = 0;
            *(u8*)(arg0 + 0x1C) = 1;
            return;
        }
        *(u8**)(arg0 + 0x8) = next;
        remain -= delta;
        if ((ev[0] >> 4) == 0xB && ev[1] == 0x16 && marker == 0) {
            extra = remain - 1;
            marker = 1;
            remain = 1;
        }
        if ((*(SeqEventFn*)(arg0 + 0x14))(ev) != 0) {
            fn_8021F338(*(void**)(arg0 + 0x18), ev);
        }
        pos = *(u8**)(arg0 + 0x8);
        delta = 0;
        while ((b = *pos) & 0x80) {
            pos += 1;
            delta = (delta | (b & 0x7F)) << 7;
        }
        ev = pos + 1;
        delta |= b & 0x7F;
        next = ev + fn_8021FCF4(arg0, ev);
    }
    u32 left = total - (remain + extra);
    u32 cur = *(u32*)(arg0 + 0x10);
    u32 consumed;
    if (left > cur) {
        consumed = left - cur;
        *(u32*)(arg0 + 0x10) = 0;
    } else {
        consumed = 0;
        *(u32*)(arg0 + 0x10) = cur - left;
    }
    s32 rest = *arg1 - consumed;
    *arg1 = rest;
    if (marker == 0) {
        *(u32*)(arg0 + 0x10) = *(u32*)(arg0 + 0x10) + rest;
        *arg1 = 0;
    }
}

// 0x8021FCF4
// Returns the byte length of the event's data after its status byte(s): 3 for note/CC/pitch
// (8x-Bx, Ex), 2 for program/pressure (Cx-Dx), VLQ-prefixed length for meta (FF) and sysex (F0).
s32 fn_8021FCF4(u8* arg0, u8* ev) {
    u32 hi = *ev >> 4;
    if (hi == 0xE) {
        return 3;
    }
    if (hi < 0xE) {
        if (hi >= 0xC) {
            return 2;
        }
        if (hi >= 8) {
            return 3;
        }
    }
    u8 status = *ev;
    if (status == 0xFF || status == 0xF0) {
        u8* p = ev + (status == 0xFF ? 2 : 1);
        u32 len = 0;
        u8 b;
        while ((b = *p) & 0x80) {
            p += 1;
            len = (len | (b & 0x7F)) << 7;
        }
        len |= b & 0x7F;
        return (s32)((p + 1 + len) - ev);
    }
    return 0;
}

// 0x8021FDC8
// Finds the first meta event of the track (via a temporary reader) and returns a pointer past
// its VLQ length, storing the length in *arg1; returns NULL with *arg1 = 0 if there is none.
u8* fn_8021FDC8(u8* arg0, u32* arg1) {
    s32 ticks;
    u8 seq[0x20];
    u8* buf = (u8*)fn_801D40E0(0x1004);
    if (buf != 0) {
        buf = fn_8021F32C(buf);
    }
    s32 track = *(s32*)(arg0 + 0xC);
    u8* data = *(u8**)(arg0 + 0x0);
    *(u8**)(seq + 0x0) = data;
    *(s32*)(seq + 0x4) = 0;
    *(s32*)(seq + 0x8) = 0;
    *(s32*)(seq + 0xC) = track;
    *(s32*)(seq + 0x10) = 0;
    *(void**)(seq + 0x14) = (void*)fn_8021FA80;
    *(u8**)(seq + 0x18) = buf;
    *(u8*)(seq + 0x1C) = 0;
    u8* tr = fn_8021F464(data, track);
    *(u8**)(seq + 0x4) = tr;
    *(u8**)(seq + 0x8) = tr + 8;
    *(s32*)(seq + 0x10) = 0;
    *(void**)(seq + 0x14) = (void*)fn_8021FA80;
    *(u8**)(seq + 0x18) = buf;
    *(s32*)(seq + 0xC) = track;
    ticks = 1;
    fn_8021FB30(seq, (u32*)&ticks);
    if (*(u32*)(buf + 0x1000) == 1) {
        u8* p = fn_8021F350(buf, 0) + 2;
        u8 b;
        *arg1 = 0;
        while ((b = *p) & 0x80) {
            p += 1;
            *arg1 = (*arg1 | (b & 0x7F)) << 7;
        }
        *arg1 = *arg1 | (b & 0x7F);
        operator_delete(buf);
        return p + 1;
    }
    *arg1 = 0;
    operator_delete(buf);
    return 0;
}

// 0x8021FFA4
// Full player constructor: base constructor (fn_8021F474, forwarding arg1/farg0), then
// initialises the 4 event buffers at +0x5C (stride 0x1004) plus one at +0x406C, and for each
// track after the first whose flag from fn_80220188 is 0 runs fn_8021FF9C.
u8* fn_8021FFA4(u8* arg0, u8* arg1, s32 arg2, s32 arg3, s32 arg4, f32 farg0) {
    u8 flags[0x64];
    fn_8021F474(arg0, arg1, farg0);
    *(s32*)(arg0 + 0x54) = arg4;
    u8* buf = arg0 + 0x5C;
    *(u8**)(arg0 + 0x1C) = lbl_80389198;
    u8* bufEnd = arg0 + 0x406C;
    *(s32*)(arg0 + 0x58) = arg3;
    do {
        fn_8021F32C(buf);
        buf += 0x1004;
    } while (buf < bufEnd);
    fn_8021F32C(bufEnd);
    *(s32*)(arg0 + 0x5070) = 0;
    fn_802209CC(flags, 0, 0x64);
    fn_80220188(arg0, flags, arg2);
    s32 i = 1;
    s32 off = 0x20;
    while (i < (s32)fn_8021F448(*(u8**)(arg0 + 0x0))) {
        if (flags[i] == 0) {
            fn_8021FF9C(*(u8**)(arg0 + 0x4) + off, arg0 + 0x406C);
        }
        off += 0x20;
        i++;
    }
    return arg0;
}

}  // extern "C"
