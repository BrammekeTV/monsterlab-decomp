// UI helpers in 0x8019-0x801B: colour space conversion, track cursor search, UI model stubs.
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  fn_8018D370(const char* fmt, ...);                                  // 0x8018D370 (debug log)
u8*    fn_8019552C(u8* list, s32 index);                                   // 0x8019552C
s32   fn_8019555C(u8* list);                                              // 0x8019555C
s32   fn_801AA974(u8* self, s32 loop, s32 rev, f32 pos);                  // 0x801AA974
s32   UIAnimTrack_Evaluate(u8* self, f32 pos);                                     // 0x801AAC08
f32   fn_801AA728(u8* entry);                                             // 0x801AA728 (leaf)
void  fn_801B737C(void* a, void* b, void* c, s32 d);                      // 0x801B737C
void  fn_801B75AC(void* p);                                               // 0x801B75AC
s32   fn_801B7C5C(u8* a, void* b, f32 dt);                                // 0x801B7C5C
void  fn_801BA74C(void* a3, u16* text, s32 a5, s32 a6, f32* a7, f32* a8); // 0x801BA74C

extern const char lbl_80381C98[];   // 0x80381C98 (log string)
extern u16        lbl_80413C40[];   // 0x80413C40 (decoded text buffer)

// 0x80191A5C
// Unimplemented script callback: only logs a message (args unused).
void UIModel_SetModel(u32 a0, BOOL a1) {
    (void)a0;
    (void)a1;
    fn_8018D370(lbl_80381C98);
}

// 0x8019E024
// Converts the RGB triple at c[0..2] to HSV in place (H in degrees at [0], S at [1], V at [2]).
void fn_8019E024(f32* c) {
    f32 r = c[0];
    f32 g = c[1];
    f32 b = c[2];
    f32 mx;
    f32 mn;
    f32 d;

    if (r > g) {
        mx = r;
    } else {
        mx = g;
    }
    if (!(mx > b)) {
        mx = b;
    }
    if (mx == 0.0f) {
        return;
    }
    if (r < g) {
        mn = r;
    } else {
        mn = g;
    }
    if (!(mn < b)) {
        mn = b;
    }
    d = mx - mn;
    if (d == 0.0f) {
        c[0] = 0.0f;
    } else if (mx == r) {
        if (g >= b) {
            c[0] = 60.0f * ((g - b) / d);
        } else {
            c[0] = 360.0f + 60.0f * ((g - b) / d);
        }
    } else if (mx == g) {
        c[0] = 120.0f + 60.0f * ((b - r) / d);
    } else {
        c[0] = 240.0f + 60.0f * ((r - g) / d);
    }
    c[2] = mx;
    c[1] = (mx - mn) / mx;
}

// 0x8019E140
// Converts the HSV triple at c[0..2] to RGB in place.
void fn_8019E140(f32* c) {
    f32 s = c[1];
    f32 v = c[2];
    if (s == 0.0f) {
        c[2] = v;
        c[1] = v;
        c[0] = v;
        return;
    }
    f32 h = c[0] / 60.0f;
    f32 p = v * (1.0f - s);
    s32 sector = (s32)h % 6;
    f32 f = h - (f32)sector;
    f32 q = v * (1.0f - f * s);
    f32 t = v * (1.0f - (1.0f - f) * s);
    switch (sector) {
    case 0:
        c[0] = v;
        c[1] = t;
        c[2] = p;
        break;
    case 1:
        c[0] = q;
        c[1] = v;
        c[2] = p;
        break;
    case 2:
        c[0] = p;
        c[1] = v;
        c[2] = t;
        break;
    case 3:
        c[0] = p;
        c[1] = q;
        c[2] = v;
        break;
    case 4:
        c[0] = t;
        c[1] = p;
        c[2] = v;
        break;
    case 5:
        c[0] = v;
        c[1] = p;
        c[2] = q;
        break;
    }
}

// 0x801AA7D8
// Updates a track at the given position when its list has at least two entries.
void fn_801AA7D8(void* track, s32 loop, s32 rev, f32 pos) {
    u8* self = (u8*)track;
    if (fn_8019555C(*(u8**)(self + 0x4)) < 2) {
        return;
    }
    fn_801AA974(self, loop, rev, pos);
    UIAnimTrack_Evaluate(self, pos);
}

// 0x801AB3DC
// Finds the track entry whose time reaches pos (forward from the start, or backward from the
// end when rev != 0) and stores its index at +0x8 (-2 if none).
void fn_801AB3DC(u8* obj, f32 pos, s32 loop, s32 rev) {
    (void)loop;
    s32 count = fn_8019555C(*(u8**)(obj + 0x4));
    *(s32*)(obj + 0x8) = -2;
    if (rev == 0) {
        for (s32 i = 0; i < fn_8019555C(*(u8**)(obj + 0x4)); i++) {
            u8* entry = fn_8019552C(*(u8**)(obj + 0x4), i);
            if (entry != NULL && fn_801AA728(entry) >= pos) {
                *(s32*)(obj + 0x8) = i;
                break;
            }
        }
    } else {
        for (s32 i = count - 1; i >= 0; i--) {
            u8* entry = fn_8019552C(*(u8**)(obj + 0x4), i);
            if (entry != NULL && fn_801AA728(entry) <= pos) {
                *(s32*)(obj + 0x8) = i;
                break;
            }
        }
    }
}

// 0x801B7484
// Sets up a node from a, b with start vector pos (+0x10) and end vector e (+0x20), then finalizes it.
void fn_801B7484(u8* self, u8* a, u8* b, s32 idx, f32* pos, f32* e) {
    if (a == NULL) {
        return;
    }
    fn_801B737C(self, a, b, 0);
    *(f32*)(self + 0x10) = pos[0];
    *(f32*)(self + 0x14) = pos[1];
    *(f32*)(self + 0x18) = pos[2];
    *(s32*)(self + 0xC) = idx;
    *(f32*)(self + 0x20) = e[0];
    *(f32*)(self + 0x24) = e[1];
    *(f32*)(self + 0x28) = e[2];
    *(u8*)(self + 0x1D) = 1;
    fn_801B75AC(self);
}

// 0x801B7C58
// Thunk to fn_801B7C5C (vtable entry), same arguments.
s32 fn_801B7C58(u8* a, void* b, f32 dt) {
    return fn_801B7C5C(a, b, dt);
}

// 0x801BA8C8
// Decodes a 0-terminated byte string into 16-bit chars in lbl_80413C40 (0xFF escapes: the next
// byte becomes the high byte of the following chars), then forwards to fn_801BA74C.
void fn_801BA8C8(void* a3, const u8* str, s32 a5, s32 a6, f32* a7, f32* a8) {
    u16* out = lbl_80413C40;
    u32 hi = 0;
    s32 escape = 0;
    while (*str != 0) {
        u32 c = *str;
        if (escape != 0) {
            hi = c << 8;
            escape = 0;
        } else if (c == 0xFF) {
            escape = 1;
        } else {
            *out++ = (u16)(hi | c);
        }
        str++;
    }
    *out = 0;
    fn_801BA74C(a3, lbl_80413C40, a5, a6, a7, a8);
}

}  // extern "C"
