// Hard leftovers: UIGrid scroll steps, UIGrid highlight flags, collision dispatch, angle helper,
// network-state name/poll.
#include "types.h"
#include "engine/math.h"
#include <math.h>

typedef s32 (*VFnII)(u8*, s32);

extern "C" {
s32 fn_801A61F4(u8* grid);                                                // 0x801A61F4
s32 fn_801A5ED0(u8* grid);                                                // 0x801A5ED0
u32 fn_801A665C(void* obj);                                               // 0x801A665C
u32 fn_801A65A0(void* obj);                                               // 0x801A65A0
u8*  fn_8019552C(u8* list, s32 index);                                     // 0x8019552C
void fn_8018D2B8(const char* fmt, ...);                                   // 0x8018D2B8 (debug log)
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner); // 0x801C80E0
s32 fn_801A9174(u8* p, f32* pos, f32* out);                               // 0x801A9174
void fn_801C7E0C(u8* p, s32 del);                                         // 0x801C7E0C
void Vec3Assign(Vec3* out, const Vec3* v);                                // 0x80053354
void Vec3Copy(Vec3* out, const Vec3* v);                                  // 0x80052640
void fn_80052638(void);                                                // 0x80052638 (empty leaf)
s32 fn_801AD47C(u8* a, u8* b, u8* c, u8* out, s32 flag, f32 margin);      // 0x801AD47C
s32 fn_801AD6AC(u8* a, u8* ext, u8* rot, u8* b, u8* out, s32 flag, f32 margin); // 0x801AD6AC
s32 fn_801ADB44(u8* arg0, u8* arg1, u8* arg2, s32 arg3, u8* arg4, f32 farg0, f32 farg1); // 0x801ADB44
u8* fn_801ADF4C(u8* a3);                                                  // 0x801ADF4C
u8* fn_801ADF54(u8* a3);                                                  // 0x801ADF54
u8* fn_801ADF5C(u8* a3);                                                  // 0x801ADF5C
u8* fn_801ADF64(u8* a3);                                                  // 0x801ADF64
s32 fn_801C2C60(f32 r, u8* a, u8* b, u8* c, s32* n, f32* pos, f32* dist); // 0x801C2C60
s32 fn_801C3170(f32 r, u8* a, u8* b, u8* c, s32* n, f32* pos, f32* dist); // 0x801C3170
s32 fn_801F72F0(u8* arg0, f32* arg1, u8* arg2);                           // 0x801F72F0
f64 fn_8033F1F4(f64 x);                                                   // 0x8033F1F4 (MSL acos)
s32 fn_802EA048(void);                                                    // 0x802EA048
s32 fn_802E48B8(s32* a, s32* b);                                          // 0x802E48B8
void DWC_ClearError(void);                                                   // 0x802E4970
s32 fn_802E925C(void);                                                    // 0x802E925C
void fn_80320968(void);                                                   // 0x80320968
s32 fn_8032D3C0(s32 code);                                                // 0x8032D3C0

extern u8 lbl_8047C760[];            // 0x8047C760 (effect manager)
extern u8 lbl_80386ED0[];            // 0x80386ED0 (string block)
extern const char lbl_805F9938[];    // 0x805F9938 "INIT"
extern const char lbl_805F9940[];    // 0x805F9940 "LOGIN:"
extern const char lbl_805F9948[];    // 0x805F9948 "ONLINE"
extern s32 lbl_805FBB4C;             // 0x805FBB4C
extern s32 lbl_805FBB50;             // 0x805FBB50
extern u8  lbl_805FBB54[];           // 0x805FBB54 (byte flag)
extern s32 lbl_805FBB58;             // 0x805FBB58
extern s32 lbl_805FBB5C;             // 0x805FBB5C

static const char kScrollFail[] = "UIGrid : unable to scroll grid to valid item ...\n";

// Cell lookup shared by both scroll steps: grid cell list at +0x1BC.
static inline u8* UIGridCellAt(u8* arg0, s32 index) {
    u8** slot = (u8**)fn_8019552C(*(u8**)(arg0 + 0x1BC), index);
    return slot != NULL ? *slot : NULL;
}

// 0x801A5830
// UIGrid: step the cursor row backwards (+0x14C) until an enabled cell (flag 1 in +0x64) is
// found, wrapping or scrolling as the grid flags allow. Returns -1 if it cannot scroll at all.
s32 fn_801A5830(u8* arg0) {
    s32 count = 0;
    *(s32*)(arg0 + 0x154) = *(s32*)(arg0 + 0x14C);
    *(s32*)(arg0 + 0x158) = *(s32*)(arg0 + 0x150);
    if (*(s32*)(arg0 + 0x64) & 0x40000) {
        if (fn_801A61F4(arg0) == 0) {
            fn_8018D2B8(kScrollFail);
            return -1;
        }
        return 0;
    }
    for (;;) {
        count += 1;
        s32 row = *(s32*)(arg0 + 0x14C) - 1;
        *(s32*)(arg0 + 0x14C) = row;
        if (row < 0) {
            *(s32*)(arg0 + 0x14C) = *(s32*)(arg0 + 0x154);
            if ((*(s32*)(arg0 + 0x64) & 0x80000) || *(s32*)(arg0 + 0x144) < *(s32*)(arg0 + 0x13C)) {
                if (fn_801A61F4(arg0) == 0) {
                    u8* b = *(u8**)(arg0 + 0x1B4);
                    if (b != NULL && !(*(s32*)(b + 0x64) & 0x100000)) {
                        *(s32*)(arg0 + 0x14C) = *(s32*)(arg0 + 0x154);
                    } else {
                        *(s32*)(arg0 + 0x14C) = *(s32*)(arg0 + 0x144) - 1;
                        s32 r4 = *(s32*)(arg0 + 0x144);
                        if ((s32)fn_801A665C(arg0) > r4) {
                            *(s32*)(arg0 + 0x15C) = (*(s32*)(arg0 + 0x13C) - r4) + 1;
                            fn_801A61F4(arg0);
                        }
                    }
                }
            } else {
                u8* b = *(u8**)(arg0 + 0x1B4);
                if (b != NULL && (*(s32*)(b + 0x64) & 0x100000)) {
                    *(s32*)(arg0 + 0x14C) = *(s32*)(arg0 + 0x13C) - 1;
                } else if (*(s32*)(arg0 + 0x14C) == *(s32*)(arg0 + 0x154)) {
                    return 0;
                }
            }
        }

        s32 r4 = *(s32*)(arg0 + 0x14C) + *(s32*)(arg0 + 0x15C);
        s32 r5 = *(s32*)(arg0 + 0x150) + *(s32*)(arg0 + 0x160);
        if (r4 < 0) {
            r4 += *(s32*)(arg0 + 0x13C);
        } else if (r4 >= *(s32*)(arg0 + 0x13C)) {
            r4 -= *(s32*)(arg0 + 0x13C);
        }
        if (r5 < 0) {
            r5 += *(s32*)(arg0 + 0x140);
        } else if (r5 >= *(s32*)(arg0 + 0x140)) {
            r5 -= *(s32*)(arg0 + 0x140);
        }
        u8* elem = UIGridCellAt(arg0, r5 + r4 * *(s32*)(arg0 + 0x140));
        s32 enabled = 0;
        if (elem != NULL) {
            enabled = *(s32*)(elem + 0x64) & 1;
        }
        s32 found = 0;
        if (elem != NULL && enabled != 0) {
            found = 1;
        }
        if (found != 0 || count > *(s32*)(arg0 + 0x13C)) {
            if (count > *(s32*)(arg0 + 0x13C)) {
                *(s32*)(arg0 + 0x14C) = *(s32*)(arg0 + 0x154);
            }
            break;
        }
    }
    return 0;
}

// 0x801A5C94
// Same as fn_801A5830 for the column axis (+0x150, size +0x140, flags 0x10000/0x20000).
s32 fn_801A5C94(u8* arg0) {
    s32 count = 0;
    *(s32*)(arg0 + 0x158) = *(s32*)(arg0 + 0x150);
    *(s32*)(arg0 + 0x154) = *(s32*)(arg0 + 0x14C);
    if (*(s32*)(arg0 + 0x64) & 0x40000) {
        if (fn_801A5ED0(arg0) == 0) {
            fn_8018D2B8(kScrollFail);
            return -1;
        }
        return 0;
    }
    for (;;) {
        count += 1;
        s32 col = *(s32*)(arg0 + 0x150) - 1;
        *(s32*)(arg0 + 0x150) = col;
        if (col < 0) {
            *(s32*)(arg0 + 0x150) = *(s32*)(arg0 + 0x158);
            if ((*(s32*)(arg0 + 0x64) & 0x10000) || *(s32*)(arg0 + 0x148) < *(s32*)(arg0 + 0x140)) {
                if (fn_801A5ED0(arg0) == 0) {
                    u8* b = *(u8**)(arg0 + 0x1B4);
                    if (b != NULL && !(*(s32*)(b + 0x64) & 0x20000)) {
                        *(s32*)(arg0 + 0x150) = *(s32*)(arg0 + 0x158);
                    } else {
                        *(s32*)(arg0 + 0x150) = *(s32*)(arg0 + 0x148) - 1;
                        s32 r4 = *(s32*)(arg0 + 0x148);
                        if ((s32)fn_801A65A0(arg0) > r4) {
                            *(s32*)(arg0 + 0x160) = (*(s32*)(arg0 + 0x140) - r4) + 1;
                            fn_801A5ED0(arg0);
                        }
                    }
                }
            } else {
                u8* b = *(u8**)(arg0 + 0x1B4);
                if (b != NULL && (*(s32*)(b + 0x64) & 0x20000)) {
                    *(s32*)(arg0 + 0x150) = *(s32*)(arg0 + 0x140) - 1;
                } else if (*(s32*)(arg0 + 0x150) == *(s32*)(arg0 + 0x158)) {
                    return 0;
                }
            }
        }

        s32 r4 = *(s32*)(arg0 + 0x14C) + *(s32*)(arg0 + 0x15C);
        s32 r5 = *(s32*)(arg0 + 0x150) + *(s32*)(arg0 + 0x160);
        if (r4 < 0) {
            r4 += *(s32*)(arg0 + 0x13C);
        } else if (r4 >= *(s32*)(arg0 + 0x13C)) {
            r4 -= *(s32*)(arg0 + 0x13C);
        }
        if (r5 < 0) {
            r5 += *(s32*)(arg0 + 0x140);
        } else if (r5 >= *(s32*)(arg0 + 0x140)) {
            r5 -= *(s32*)(arg0 + 0x140);
        }
        u8* elem = UIGridCellAt(arg0, r5 + r4 * *(s32*)(arg0 + 0x140));
        s32 enabled = 0;
        if (elem != NULL) {
            enabled = *(s32*)(elem + 0x64) & 1;
        }
        s32 found = 0;
        if (elem != NULL && enabled != 0) {
            found = 1;
        }
        if (found != 0 || count > *(s32*)(arg0 + 0x140)) {
            if (count > *(s32*)(arg0 + 0x140)) {
                *(s32*)(arg0 + 0x150) = *(s32*)(arg0 + 0x158);
            }
            break;
        }
    }
    return 0;
}

// 0x801A8B88
// Sets flag bits in +0x64. If bit 2 is set: spawns the highlight effect (once) at the screen
// position and links its entries back to this object, then sets/clears bit 2 on every entry
// (copying +0x10..0x1C to +0x20..0x2C when the byte at +0x188 is set) and calls fn_801C7E0C on each.
void fn_801A8B88(u8* arg0, s32 arg1) {
    f32 args[4];
    f32 out[3];
    f32 pos[3];

    *(s32*)(arg0 + 0x64) = *(s32*)(arg0 + 0x64) | arg1;
    if (arg1 & 2) {
        if (*(s32*)(arg0 + 0x178) == 0 && *(s32*)(arg0 + 0x140) != -1) {
            fn_801A9174(arg0, pos, out);
            args[0] = pos[0];
            args[1] = pos[1];
            args[2] = pos[2];
            args[3] = 0.0f;
            SpawnEffectAt(lbl_8047C760, *(u32*)(arg0 + 0x140), args, (u8**)(arg0 + 0x178));
            u8* list = *(u8**)(arg0 + 0x178);
            if (list != NULL) {
                u8* cur = list;
                s32 i;
                *(u8*)(list + 0x8) = 0;
                for (i = 0; i < *(s32*)(list + 0xC); i++) {
                    s32* e = *(s32**)(cur + 0x14);
                    if (e != NULL) *e &= ~0x4;
                    cur += 4;
                }
                s32 off = 0;
                for (i = 0; i < *(s32*)(*(u8**)(arg0 + 0x178) + 0xC); i++) {
                    u8* e = *(u8**)(*(u8**)(arg0 + 0x178) + off + 0x14);
                    if (e != NULL) {
                        *(u8**)(e + 0x74) = arg0;
                        *(s32*)(*(u8**)(*(u8**)(arg0 + 0x178) + off + 0x14) + 0x7C) = *(s32*)(lbl_8047C760 + 0x24);
                    }
                    off += 4;
                }
            }
            (*(VFnII*)(*(u8**)(arg0 + 0x138) + 0x2C))(arg0, 0x10);
        }
        u8* tbl = *(u8**)(arg0 + 0x178);
        if (tbl != NULL) {
            u8 flag = *(u8*)(arg0 + 0x188);
            u8* cur = tbl;
            s32 i;
            for (i = 0; i < *(s32*)(tbl + 0xC); i++) {
                u8* e = *(u8**)(cur + 0x14);
                if (e != NULL) {
                    if (flag != 0) {
                        *(u32*)e = *(u32*)e | 2;
                    } else {
                        *(u32*)e = *(u32*)e & ~2u;
                    }
                    if (flag != 0) {
                        e = *(u8**)(cur + 0x14);
                        *(f32*)(e + 0x20) = *(f32*)(e + 0x10);
                        *(f32*)(e + 0x24) = *(f32*)(e + 0x14);
                        *(f32*)(e + 0x28) = *(f32*)(e + 0x18);
                        *(f32*)(e + 0x2C) = *(f32*)(e + 0x1C);
                    }
                }
                cur += 4;
            }
            s32 off = 0;
            for (i = 0; i < *(s32*)(*(u8**)(arg0 + 0x178) + 0xC); i++) {
                u8* e = *(u8**)(*(u8**)(arg0 + 0x178) + off + 0x14);
                if (e != NULL) fn_801C7E0C(e, 1);
                off += 4;
            }
        }
    }
}

// 0x801AD248
// Collision test dispatch on the shape type at arg2+0xC (0 sphere-like, 4/7 oriented shapes,
// 5/6 segment/capsule tests). arg6 selects which out buffer (arg3 or arg1) the result goes to.
s32 fn_801AD248(u8* arg0, u8* arg1, u8* arg2, u8* arg3, s32 arg4, u8* arg5, s32 arg6) {
    Vec3 sp5C;
    Vec3 sp50;
    Vec3 sp44;
    Vec3 sp38;
    Vec3 sp2C;
    Vec3 sp20;
    f32 sp18;
    f32 sp10;
    s32 spC;
    s32 sp8;
    u8* var_r30 = arg3;
    u8* a;
    u8* b;

    switch (*(s32*)(arg2 + 0xC)) {
    case 4:
        if (arg6 == 0) var_r30 = arg1;
        // Vec3Assign returns void; the original passes the leftover r3 (= &sp2C) on.
        Vec3Assign(&sp2C, (Vec3*)(arg2 + 0x3C));
        return fn_801AD47C(arg2 + 0x14, (u8*)&sp2C, arg0 + 0x14, var_r30, arg4, *(f32*)(arg0 + 0x3C));
    case 7:
        if (arg6 == 0) var_r30 = arg1;
        // Vec3Assign leftover r3 (= &sp20) is the second argument.
        Vec3Assign(&sp20, (Vec3*)(arg2 + 0x6C));
        return fn_801AD6AC(arg2 + 0x14, (u8*)&sp20, arg2 + 0x3C, arg0 + 0x14, var_r30, arg4, *(f32*)(arg0 + 0x3C));
    case 0:
        a = (arg6 != 0) ? arg3 : arg1;
        return fn_801ADB44(arg2 + 0x14, arg0 + 0x14, a, arg4, arg5, *(f32*)(arg2 + 0x3C), *(f32*)(arg0 + 0x3C));
    case 5:
        fn_80052638();
        fn_80052638();
        b = fn_801ADF54(arg2);
        a = fn_801ADF4C(arg2);
        if (fn_801C2C60(*(f32*)(arg0 + 0x3C), arg0 + 0x14, a, b, &spC, &sp50.x, &sp18) != 0) {
            if (spC > 0) {
                Vec3Copy((Vec3*)arg3, &sp50);
                *(f32*)(arg3 + 0x18) = sp18;
            }
            return 1;
        }
        return 0;
    case 6:
        fn_80052638();
        fn_80052638();
        b = fn_801ADF64(arg2);
        a = fn_801ADF5C(arg2);
        if (fn_801C3170(*(f32*)(arg0 + 0x3C), arg0 + 0x14, a, b, &sp8, &sp38.x, &sp10) != 0) {
            if (sp8 > 0) {
                Vec3Copy((Vec3*)arg3, &sp38);
                *(f32*)(arg3 + 0x18) = sp10;
            }
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}

// 0x801F74C8
// Angle between the vector at +0x90 and the one from fn_801F72F0, stored as acos(cos) in
// (2/pi)*(pi/2) units at +0xA0 (0 if either vector is degenerate); +0xA4 is 1 when a valid
// vector was found, 0 when fn_801F72F0 flagged it invalid.
void fn_801F74C8(u8* arg0) {
    f32 v[3];
    u8 flag = 0;
    v[2] = 0.0f;
    v[1] = 0.0f;
    v[0] = 0.0f;
    if (fn_801F72F0(arg0, v, &flag) != 0) {
        if (flag != 0) {
            *(u8*)(arg0 + 0xA4) = 0;
            return;
        }
        f32 x = *(f32*)(arg0 + 0x90);
        f32 y = *(f32*)(arg0 + 0x94);
        *(u8*)(arg0 + 0xA4) = 1;
        f32 z = *(f32*)(arg0 + 0x98);
        f32 l1sq = z * z + (x * x + y * y);
        f32 dot = z * v[2] + (x * v[0] + y * v[1]);
        f32 len1 = 0.0f;
        if (!(l1sq <= 0.0f)) {
            // original: frsqrte with two Newton steps, then x * rsqrt(x)
            len1 = l1sq * (1.0f / sqrtf(l1sq));
        }
        f32 l2sq = v[2] * v[2] + (v[0] * v[0] + v[1] * v[1]);
        f32 len2 = 0.0f;
        if (!(l2sq <= 0.0f)) {
            len2 = l2sq * (1.0f / sqrtf(l2sq));
        }
        f32 ang = 0.0f;
        f32 prod = len1 * len2;
        if (!(prod <= 0.0f)) {
            f32 c = dot / prod;
            if (!(c < -1.0f)) {
                if (c > 1.0f) c = 1.0f;
            } else {
                c = -1.0f;
            }
            if (!(c < -1.0f)) {
                if (c > 1.0f) c = 1.0f;
            } else {
                c = -1.0f;
            }
            ang = 0.63661975f * (f32)fn_8033F1F4((f64)c);
        }
        *(f32*)(arg0 + 0xA0) = 1.5707964f * ang;
    }
}

// 0x80207D40
// Name of the current network state (fn_802EA048): jump table of 7 strings, default at +0x50C.
const char* fn_80207D40(void) {
    const char* r;
    u8* base = lbl_80386ED0;
    switch ((u32)fn_802EA048()) {
    case 0: r = lbl_805F9938; break;            // "INIT"
    case 1: r = (const char*)(base + 0x518); break;
    case 2: r = lbl_805F9940; break;            // "LOGIN:"
    case 3: r = lbl_805F9948; break;            // "ONLINE"
    case 4: r = (const char*)(base + 0x528); break;
    case 5: r = (const char*)(base + 0x538); break;
    case 6: r = (const char*)(base + 0x544); break;
    default: r = (const char*)(base + 0x50C); break;
    }
    return r;
}

// 0x80208A54
// Network connection poll: error codes -0x1C/-0x31 (or a pending lbl_805FBB50) latch the
// "busy" flag and re-query; otherwise refreshes the cached state at +0xAC/+0xB8..0xC0 and runs
// the per-state action. Returns 1 while a connection is in progress (state not 0/1).
s32 fn_80208A54(u8* arg0) {
    s32 code = lbl_805FBB4C;
    if (code == -0x1C || code == -0x31) {
        lbl_805FBB54[0] = 1;
        lbl_805FBB58 = fn_8032D3C0(code);
        return 1;
    }
    s32 pending = lbl_805FBB50;
    if (pending != 0) {
        lbl_805FBB54[0] = 1;
        lbl_805FBB58 = fn_8032D3C0(pending);
        return 1;
    }
    {
        s32 r = fn_802E48B8(&lbl_805FBB58, &lbl_805FBB5C);
        *(s32*)(arg0 + 0xAC) = r;
        if (r != *(s32*)(arg0 + 0xB8) || lbl_805FBB58 != *(s32*)(arg0 + 0xBC) ||
            lbl_805FBB5C != *(s32*)(arg0 + 0xC0)) {
            *(s32*)(arg0 + 0xB8) = *(s32*)(arg0 + 0xAC);
            *(s32*)(arg0 + 0xBC) = lbl_805FBB58;
            *(s32*)(arg0 + 0xC0) = lbl_805FBB5C;
            switch ((u32)lbl_805FBB5C) {
            case 0:
                lbl_805FBB54[0] = 0;
                break;
            case 1:
                lbl_805FBB54[0] = 0;
                DWC_ClearError();
                break;
            case 2:
                lbl_805FBB54[0] = 1;
                break;
            case 3:
                lbl_805FBB54[0] = 1;
                fn_802E925C();
                break;
            case 4:
                lbl_805FBB54[0] = 1;
                break;
            case 5:
                lbl_805FBB54[0] = 1;
                fn_80320968();
                break;
            case 6:
                lbl_805FBB54[0] = 1;
                fn_802E925C();
                fn_80320968();
                break;
            case 7:
                lbl_805FBB54[0] = 1;
                break;
            default:
                break;
            }
        }
    }
    if (*(s32*)(arg0 + 0xAC) != 0 && lbl_805FBB5C != 0 && lbl_805FBB5C != 1) {
        return 1;
    }
    return 0;
}

}  // extern "C"
