// Small engine functions in 0x801Fxxxx / 0x8020xxxx (string replace, fade task, audio callbacks).
#include "types.h"

extern "C" {
char* fn_80338BC0(char* s, const char* sub);                    // 0x80338BC0 (strstr)
u32   strlen(const char* s);
s32   WPADControlSpeaker(s32 chan, s32 a, void* cb);                   // 0x802737A8
s32   fn_801F2118(u8* task, f32 dt);                                   // 0x801F2118
s32   fn_8025E014(void);                                        // 0x8025E014
s32   fn_8025D7D0(void);                                        // 0x8025D7D0
s32   fn_8025F284(void);                                        // 0x8025F284
s32   fn_8025E7F8(void);                                        // 0x8025E7F8

extern u8   lbl_80348F38[];     // 0x80348F38 (dispatch table)
extern u8   lbl_8051E680[];     // 0x8051E680 (0x28-byte records)
extern u8   lbl_805FBA9C;       // 0x805FBA9C (platform/mode, 2 = alternate)
extern s32  lbl_805FBABC;       // 0x805FBABC
extern s32  lbl_805FBAC0;       // 0x805FBAC0
extern s32  lbl_805FBAC8;       // 0x805FBAC8
extern void* lbl_805FBACC;      // 0x805FBACC
extern s32  lbl_805FBAD0;       // 0x805FBAD0
extern s32  lbl_805FBAE0;       // 0x805FBAE0 (audio busy flag)

typedef s32 (*QueryFn)(void* obj, s32 what);

// 0x801F0448
// Replaces the first occurrence of `pattern` in `text` with `repl` (in place). Returns 1 if replaced.
s32 fn_801F0448(char* text, const char* pattern, const char* repl) {
    s32 ok;
    char* dst;
    char* tail;
    char* end;
    if (text == NULL || *text == 0 || pattern == NULL || *pattern == 0) {
        return 0;
    }
    ok = 0;
    dst = fn_80338BC0(text, pattern);
    if (dst != NULL) {
        tail = dst + strlen(pattern);
        end = text + strlen(text);
        while (*repl != 0) {
            *dst++ = *repl++;
        }
        while (tail < end && *tail != 0) {
            *dst++ = *tail++;
        }
        ok = 1;
        *dst = 0;
    }
    return ok;
}

// 0x801F2018
// Tween task update: returns 0 = keep, 2 = running, 3 = done.
s32 fn_801F2018(u8* task, f32 dt) {
    f32 base;
    u8* src;
    u8* obj;
    f32 d;
    switch (*(u8*)(task + 0x7F)) {
    case 0x20:
        return fn_801F2118(task, dt);
    case 0x08:
        return 2;
    case 0x10:
        src = *(u8**)(task + 0x4);
        if (src != NULL) {
            base = *(f32*)(src + 0x9C);
        } else {
            obj = *(u8**)task;
            if (obj == NULL) {
                return 3;
            }
            {
                u8* tbl = lbl_80348F38 + *(u8*)(obj + 0xC) * 4;
                s32 v = (*(QueryFn*)(tbl + 0x30))(*(void**)(obj + 0x8), 0x10);
                base = *(f32*)&v;
            }
        }
        d = *(f32*)(task + 0x8) - base;
        *(f32*)(task + 0x14) = d;
        if (d == 0.0f) {
            return 3;
        }
        return 2;
    case 0x40:
        return 3;
    default:
        return 0;
    }
}

// 0x801FCD9C
s32 fn_801FCD9C(void) {
    if (lbl_805FBA9C == 2) {
        return fn_8025E014();
    }
    return fn_8025D7D0();
}

// 0x801FCDCC
s32 fn_801FCDCC(void) {
    if (lbl_805FBA9C == 2) {
        return fn_8025F284();
    }
    return fn_8025E7F8();
}

// 0x80202E24
// Audio channel callback: result 0 = finished (restore voice count), otherwise re-arm self.
void fn_80202E24(s32 chan, s32 result) {
    if (result == 0) {
        u8* rec = lbl_8051E680 + chan * 0x28;
        s32 saved;
        if (*(s32*)(rec + 0x24) == 2) {
            *(s32*)(rec + 0x24) = 3;
        }
        if ((&lbl_805FBABC)[chan] == 0) {
            saved = lbl_805FBAE0;
            lbl_805FBAE0 = 1;
            if (WPADControlSpeaker(chan, 2, 0) == 0) {
                (&lbl_805FBAC0)[chan] = (&lbl_805FBABC)[chan];
            }
            lbl_805FBAE0 = saved;
        }
    } else if (result != -1) {
        lbl_805FBAC8 = 5;
        lbl_805FBACC = (void*)fn_80202E24;
        lbl_805FBAD0 = 4;
    }
}

}  // extern "C"
