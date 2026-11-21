// Effect-slot vertex fades (0x801630F0, 0x80163514) and per-slot state handlers (timer / ramp
// up / hold / ramp down) around 0x80163A4C..0x8016891C.
#include "types.h"

extern "C" {
u32 RandomU32(void);                                            // 0x80143460
void fn_80156CDC(s32 idx, s32 flag, void* c, u32** out);        // 0x80156CDC
void fn_80156F3C(s32 idx);                                      // 0x80156F3C (empty)
void fn_8015AAE8(void* p);                                      // 0x8015AAE8
u8* fn_8015ECA0(u8* a3);                                        // 0x8015ECA0
void fn_8016599C(u8* mgr, u8* slot, f32 dt);                    // 0x8016599C
extern u8 lbl_8040D500[];                                       // 0x8040D500
}

extern "C" {

// 0x801630F0
// Scales three 4-float vectors (+0x60, +0x70, +0x80) of the global record by a blend toward the
// slot's factors, and fills a 576-word buffer with white whose alpha follows the slot fade.
void fn_801630F0(u8* arg0, u8* arg1) {
    if (*(f32*)(arg1 + 0x18) > 0.0f) {
        f32 t = *(f32*)(arg1 + 0x10) * *(f32*)(arg1 + 0x1C);
        f32 k;
        f32 v[4];
        u8* r;
        f32* pf;
        u32* buf;
        u32 color;
        s32 i;

        k = (1.0f - t) + (*(f32*)(arg1 + 0x0) * t);
        r = fn_8015ECA0(lbl_8040D500);
        v[0] = *(f32*)(r + 0x60) * k;
        v[1] = *(f32*)(r + 0x64) * k;
        v[2] = *(f32*)(r + 0x68) * k;
        v[3] = *(f32*)(r + 0x6C) * k;
        r = fn_8015ECA0(lbl_8040D500);
        *(f32*)(r + 0x60) = v[0];
        *(f32*)(r + 0x64) = v[1];
        *(f32*)(r + 0x68) = v[2];
        *(f32*)(r + 0x6C) = v[3];

        k = (1.0f - t) + (*(f32*)(arg1 + 0x4) * t);
        r = fn_8015ECA0(lbl_8040D500);
        v[0] = *(f32*)(r + 0x70) * k;
        v[1] = *(f32*)(r + 0x74) * k;
        v[2] = *(f32*)(r + 0x78) * k;
        v[3] = *(f32*)(r + 0x7C) * k;
        r = fn_8015ECA0(lbl_8040D500);
        *(f32*)(r + 0x70) = v[0];
        *(f32*)(r + 0x74) = v[1];
        *(f32*)(r + 0x78) = v[2];
        *(f32*)(r + 0x7C) = v[3];

        k = (1.0f - t) + (*(f32*)(arg1 + 0x8) * t);
        r = fn_8015ECA0(lbl_8040D500);
        v[0] = *(f32*)(r + 0x80) * k;
        v[1] = *(f32*)(r + 0x84) * k;
        v[2] = *(f32*)(r + 0x88) * k;
        v[3] = *(f32*)(r + 0x8C) * k;
        r = fn_8015ECA0(lbl_8040D500);
        *(f32*)(r + 0x80) = v[0];
        *(f32*)(r + 0x84) = v[1];
        *(f32*)(r + 0x88) = v[2];
        *(f32*)(r + 0x8C) = v[3];

        pf = *(f32**)(arg1 + 0x14);
        k = (pf != NULL) ? *pf : 0.5f;
        buf = NULL;
        color = 0xFFFFFFu | ((u32)(s32)(255.0f * (*(f32*)(arg1 + 0x18) * (1.0f - k))) << 24);
        fn_80156CDC(1, *(s32*)(arg0 + 0x360C) == 0, NULL, &buf);
        for (i = 0; i < 0x24 * 16; i++) {
            buf[i] = color;
        }
        fn_80156F3C(1);
        *(s32*)(arg0 + 0x360C) = 1;
    }
}

// 0x80163514
// Fills a 576-word color buffer with white at alpha fade*level and scales 144 float pairs of a
// second buffer by the slot scale, then marks slot idx-1 used.
void fn_80163514(u8* arg0, u8* arg1) {
    f32 fade = *(f32*)(arg1 + 0x14);
    if (fade > 0.0f) {
        s32 idx = *(s32*)(arg1 + 0x10) - 1;
        u32* colors = NULL;
        u8* fl = NULL;
        u8* slot = arg0 + idx * 4;
        u32 color = 0xFFFFFFu | ((u32)(s32)(255.0f * (fade * *(f32*)(arg1 + 0xC))) << 24);
        s32 i, j;
        s32 off;
        static const s32 kOffs[16] = {0x00, 0x04, 0x0C, 0x10, 0x18, 0x1C, 0x24, 0x28,
                                      0x30, 0x34, 0x3C, 0x40, 0x48, 0x4C, 0x54, 0x58};
        fn_80156CDC(idx, *(s32*)(slot + 0x3608) == 0, &fl, &colors);
        for (i = 0; i < 0x48; i++) {
            for (j = 0; j < 8; j++) {
                colors[i * 8 + j] = color;
            }
            for (j = 0; j < 16; j++) {
                off = i * 0x60 + kOffs[j];
                *(f32*)(fl + off) = *(f32*)(fl + off) * *(f32*)(arg1 + 0x8);
            }
        }
        fn_80156F3C(idx);
        *(s32*)(slot + 0x3608) = 1;
    }
}

// 0x80163A4C
// Timed state machine: 1 ramps +0x10 up to +0x0, 2 holds, 3 ramps down to 0 (then 4); the
// +0x14 timer wraps at +0xC and rerolls the random seed at +0x18 each wrap.
void fn_80163A4C(u8* mgr, u8* slot, s32* state, f32* dt, u32 b) {
    switch (*state) {
    case 1:
        if ((s32)b != 0) {
            *state = 3;
            break;
        }
        *(f32*)(slot + 0x14) = *(f32*)(slot + 0x14) + *dt;
        if (*(f32*)(slot + 0x14) >= *(f32*)(slot + 0xC)) {
            *(u32*)(slot + 0x18) = RandomU32();
            while (*(f32*)(slot + 0x14) >= *(f32*)(slot + 0xC)) {
                *(f32*)(slot + 0x14) = *(f32*)(slot + 0x14) - *(f32*)(slot + 0xC);
            }
        }
        *(f32*)(slot + 0x10) = *(f32*)(slot + 0x10) + *dt * 1.0f;
        *dt = 0.0f;
        if (*(f32*)(slot + 0x10) >= *(f32*)(slot + 0x0)) {
            *(f32*)(slot + 0x10) = *(f32*)(slot + 0x0);
            *state = 2;
        }
        break;
    case 2:
        if ((s32)b != 0) {
            *state = 3;
            break;
        }
        *(f32*)(slot + 0x14) = *(f32*)(slot + 0x14) + *dt;
        if (*(f32*)(slot + 0x14) >= *(f32*)(slot + 0xC)) {
            *(u32*)(slot + 0x18) = RandomU32();
            while (*(f32*)(slot + 0x14) >= *(f32*)(slot + 0xC)) {
                *(f32*)(slot + 0x14) = *(f32*)(slot + 0x14) - *(f32*)(slot + 0xC);
            }
        }
        *dt = 0.0f;
        break;
    case 3:
        *(f32*)(slot + 0x10) = *(f32*)(slot + 0x10) - *dt * 1.0f;
        *(f32*)(slot + 0x14) = *(f32*)(slot + 0x14) + *dt;
        if (*(f32*)(slot + 0x14) >= *(f32*)(slot + 0xC)) {
            *(u32*)(slot + 0x18) = RandomU32();
            while (*(f32*)(slot + 0x14) >= *(f32*)(slot + 0xC)) {
                *(f32*)(slot + 0x14) = *(f32*)(slot + 0x14) - *(f32*)(slot + 0xC);
            }
        }
        *dt = 0.0f;
        if (*(f32*)(slot + 0x10) <= 0.0f) {
            *state = 4;
        }
        break;
    default:
        break;
    }
}

// 0x80164864
// Fade state machine on +0x18 (target +0x0) with a running time at +0x1C; on finish frees the
// two sub-objects at +0x28/+0x48 when the flag byte at +0x20 is set.
void fn_80164864(u8* mgr, u8* slot, s32* state, f32* dt, u32 b) {
    switch (*state) {
    case 1:
        if ((s32)b != 0) {
            *state = 3;
            break;
        }
        *(f32*)(slot + 0x18) = *(f32*)(slot + 0x18) + *dt * 1.0f;
        *(f32*)(slot + 0x1C) = *(f32*)(slot + 0x1C) + *dt;
        *dt = 0.0f;
        if (*(f32*)(slot + 0x18) >= *(f32*)(slot + 0x0)) {
            *(f32*)(slot + 0x18) = *(f32*)(slot + 0x0);
            *state = 2;
        }
        break;
    case 2:
        if ((s32)b != 0) {
            *state = 3;
            break;
        }
        *(f32*)(slot + 0x1C) = *(f32*)(slot + 0x1C) + *dt;
        *dt = 0.0f;
        break;
    case 3:
        *(f32*)(slot + 0x18) = *(f32*)(slot + 0x18) - *dt * 1.0f;
        *(f32*)(slot + 0x1C) = *(f32*)(slot + 0x1C) + *dt;
        *dt = 0.0f;
        if (*(f32*)(slot + 0x18) <= 0.0f) {
            *state = 4;
            if (*(u8*)(slot + 0x20) != 0) {
                fn_8015AAE8(slot + 0x28);
                fn_8015AAE8(slot + 0x48);
                *(u8*)(slot + 0x20) = 0;
            }
        }
        break;
    default:
        break;
    }
}

// 0x80165DB4
// Fade state machine on +0x10 (target +0x0); calls fn_8016599C with the elapsed time each step.
void fn_80165DB4(u8* mgr, u8* slot, s32* state, f32* dt, u32 b) {
    switch (*state) {
    case 1:
        if ((s32)b != 0) {
            *state = 3;
            break;
        }
        fn_8016599C(mgr, slot, *dt);
        *(f32*)(slot + 0x10) = *(f32*)(slot + 0x10) + *dt * 1.0f;
        *dt = 0.0f;
        if (*(f32*)(slot + 0x10) >= *(f32*)(slot + 0x0)) {
            *(f32*)(slot + 0x10) = *(f32*)(slot + 0x0);
            *state = 2;
        }
        break;
    case 2:
        if ((s32)b != 0) {
            *state = 3;
            break;
        }
        fn_8016599C(mgr, slot, *dt);
        *dt = 0.0f;
        break;
    case 3:
        *(f32*)(slot + 0x10) = *(f32*)(slot + 0x10) - *dt * 1.0f;
        fn_8016599C(mgr, slot, *dt);
        *dt = 0.0f;
        if (*(f32*)(slot + 0x10) <= 0.0f) {
            *state = 4;
        }
        break;
    default:
        break;
    }
}

// 0x8016633C
// Normalized 0..1 ramp in +0xC (mirrored to +0x10): 1 ramps up over +0x4 seconds, 2 holds for
// +0x8 seconds, 3 ramps down over +0x4 to 0 (then 4).
void fn_8016633C(u8* mgr, u8* slot, s32* state, f32* dt, u32 b) {
    switch (*state) {
    case 1:
        if ((s32)b != 0) {
            *state = 3;
            break;
        }
        *(f32*)(slot + 0xC) = *(f32*)(slot + 0xC) + *dt / *(f32*)(slot + 0x4);
        *(f32*)(slot + 0x10) = *(f32*)(slot + 0xC);
        *dt = 0.0f;
        if (*(f32*)(slot + 0xC) >= 1.0f) {
            *(f32*)(slot + 0xC) = 0.0f;
            *(f32*)(slot + 0x10) = 1.0f;
            *state = 2;
        }
        break;
    case 2:
        if ((s32)b != 0) {
            *(f32*)(slot + 0xC) = 1.0f;
            *(f32*)(slot + 0x10) = 1.0f;
            *state = 3;
            break;
        }
        *(f32*)(slot + 0xC) = *(f32*)(slot + 0xC) + *dt / *(f32*)(slot + 0x8);
        *dt = 0.0f;
        if (*(f32*)(slot + 0xC) >= 1.0f) {
            *(f32*)(slot + 0xC) = 1.0f;
            *state = 3;
        }
        break;
    case 3:
        *(f32*)(slot + 0xC) = *(f32*)(slot + 0xC) - *dt / *(f32*)(slot + 0x4);
        *(f32*)(slot + 0x10) = *(f32*)(slot + 0xC);
        *dt = 0.0f;
        if (*(f32*)(slot + 0xC) <= 0.0f) {
            *state = 4;
        }
        break;
    default:
        break;
    }
}

// 0x8016891C
// Same ramp as fn_8016633C on +0x1C/+0x20 with durations +0x14 (up/down) and +0x18 (hold; a
// non-positive hold just swallows the delta).
void fn_8016891C(u8* mgr, u8* slot, s32* state, f32* dt, u32 b) {
    switch (*state) {
    case 1:
        if ((s32)b != 0) {
            *state = 3;
            break;
        }
        *(f32*)(slot + 0x1C) = *(f32*)(slot + 0x1C) + *dt / *(f32*)(slot + 0x14);
        *(f32*)(slot + 0x20) = *(f32*)(slot + 0x1C);
        *dt = 0.0f;
        if (*(f32*)(slot + 0x1C) >= 1.0f) {
            *(f32*)(slot + 0x1C) = 0.0f;
            *(f32*)(slot + 0x20) = 1.0f;
            *state = 2;
        }
        break;
    case 2:
        if ((s32)b != 0) {
            *(f32*)(slot + 0x1C) = 1.0f;
            *(f32*)(slot + 0x20) = 1.0f;
            *state = 3;
            break;
        }
        if (*(f32*)(slot + 0x18) > 0.0f) {
            *(f32*)(slot + 0x1C) = *(f32*)(slot + 0x1C) + *dt / *(f32*)(slot + 0x18);
            *dt = 0.0f;
            if (*(f32*)(slot + 0x1C) >= 1.0f) {
                *(f32*)(slot + 0x1C) = 1.0f;
                *state = 3;
            }
        } else {
            *dt = 0.0f;
        }
        break;
    case 3:
        *(f32*)(slot + 0x1C) = *(f32*)(slot + 0x1C) - *dt / *(f32*)(slot + 0x14);
        *(f32*)(slot + 0x20) = *(f32*)(slot + 0x1C);
        *dt = 0.0f;
        if (*(f32*)(slot + 0x1C) <= 0.0f) {
            *state = 4;
        }
        break;
    default:
        break;
    }
}

}  // extern "C"
