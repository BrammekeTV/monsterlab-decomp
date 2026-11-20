// Drafts miss284: global-settings table lookups (zone/bank counts) and the hook script interpreter.
#include <string.h>

#include "types.h"

extern "C++" u8* gGlobalSettings;     // 0x805FB7A4 (pointer to the settings block)
extern "C++" u8  gLevelSingleBank;    // 0x805FB7E0 (level flag: one bank of zone memory)
extern u8        lbl_805FB89C[];      // 0x805FB89C (script result register, 32 bits)
extern u8        lbl_805FB890[];      // 0x805FB890 (fallback hook handler pointer)
extern const char lbl_80385BD8[];     // 0x80385BD8 ("Hook Parameters failed NULL check for HOOK%03d\n")

extern "C" {
void  GameLog(int level, int category, const char* fmt, ...);        // 0x8000B2F0
f32   fn_801D0354(u32 bits);                                         // 0x801D0354 (u32 bits -> f32)
u32   fn_801D0340(f32 v);                                            // 0x801D0340 (f32 -> u32 bits)
s32   TriggerDoAction(void* action, void* a1, void* a2);            // 0x801E0054
BOOL  ScriptDispatchEngine(s32 func, u32 sig, u32* args, u32* result); // 0x801E03AC
u32   fn_801E2368(u8* code);                                         // 0x801E2368 (first word has opcode 1)
u32   fn_801E2380(u32 value, u32 width, u32 shift);                  // 0x801E2380 (bit field extract)
u32   fn_801E239C(s32 word);                                         // 0x801E239C (signed 12-bit jump offset)
u32   fn_801E23B0(u32 idx);                                          // 0x801E23B0 (gScriptFuncs[idx].type, 0 if idx == -1)
s32   fn_801E23D8(s32 type);                                         // 0x801E23D8 (gScriptSigs[type] kind)
void  fn_801E2400(u32 idx, void* a);                                 // 0x801E2400 (tail call gScriptFuncs[idx].fn(a))
s32   fn_801E2420(s32 type);                                         // 0x801E2420 (gScriptSigs[type] argument count)
s32   fn_801E2448(s32 type);                                         // 0x801E2448 (gScriptSigs[type] pointer-argument mask)
s32   fn_801E2470(u32* out, u32 count, u32 mask, void* obj, void* a2); // 0x801E2470 (resolve hook arguments)
void  fn_801E2724(u32 idx, u32 a, void* b);                          // 0x801E2724 (tail call gScriptFuncs[idx].fn(a, b))
void  fn_801E2748(u32 idx, u32 a, void* b);                          // 0x801E2748 (tail call gScriptFuncs[idx].fn(a, b))
void  fn_801E276C(u32 idx, u32 a, u32 b, void* c);                   // 0x801E276C (tail call gScriptFuncs[idx].fn(a, b, c))
void  fn_801E2794(u32 idx, u32 a, u32 b, void* c);                   // 0x801E2794 (tail call gScriptFuncs[idx].fn(a, b, c))
}

typedef u32 (*HookFallbackFn)(u32 func, u32 sig, u32* args, u32* extra);

// Pops two ints (top -> hi, below -> lo) and clears both slots.
static void PopTwoInts(u32*& sp, s32& lo, s32& hi) {
    hi = (s32)sp[-1];
    sp[-1] = 0;
    lo = (s32)sp[-2];
    sp -= 2;
    sp[0] = 0;
}

// Pops two floats (top -> hi, below -> lo) and clears both slots.
static void PopTwoFloats(u32*& sp, f32& lo, f32& hi) {
    hi = fn_801D0354(sp[-1]);
    sp[-1] = 0;
    sp -= 2;
    lo = fn_801D0354(sp[0]);
    sp[0] = 0;
}

extern "C" {

// 0x801D329C
// Maps a global item id onto its 1-based index inside its group. The settings block holds 13
// group sizes (0x34..0x64) and 13 id-range lengths (0x68..0x98). 0x1FF gives -1, 0 gives 0.
s32 fn_801D329C(s32 id) {
    if (id == 0x1FF) {
        return -1;
    }
    if (id == 0) {
        return 0;
    }
    u8* s = gGlobalSettings;
    s32 idBase = 0;     // sum of the id-range lengths of the earlier groups
    s32 sizeBase = 0;   // sum of the sizes of the earlier groups
    for (s32 i = 0; i < 13; i++) {
        s32 size = *(s32*)(s + 0x34 + i * 4);
        s32 idEnd = idBase + *(s32*)(s + 0x68 + i * 4);
        if (size > 0 && id <= idEnd) {
            return sizeBase + (id - (idBase + 1)) % size + 1;
        }
        idBase = idEnd;
        sizeBase += size;
    }
    return 0;
}

// 0x801D3A84
// Memory budget for an item id: 0 gives the base amount, ids in the first 6 groups give their
// per-group amount (with the level-single-bank adjustments), later groups give derived values.
u32 fn_801D3A84(s32 id) {
    s32 result = 0;
    if (id == 0) {
        return *(s32*)(gGlobalSettings + 0x4);
    }
    u8* s = gGlobalSettings;
    s32 size0 = *(s32*)(s + 0x34);
    if (size0 > 0 && id <= *(s32*)(s + 0x68)) {
        result = *(s32*)(s + 0x8);
        if (gLevelSingleBank != 0) {
            result -= *(s32*)(s + 0xC) + *(s32*)(s + 0x10) + *(s32*)(s + 0x14) + *(s32*)(s + 0x18) + *(s32*)(s + 0x1C);
        }
        return result;
    }
    s32 size1 = *(s32*)(s + 0x38);
    s32 size2 = *(s32*)(s + 0x3C);
    s32 size3 = *(s32*)(s + 0x40);
    s32 size4 = *(s32*)(s + 0x44);
    s32 size5 = *(s32*)(s + 0x48);
    // Groups 1..5: one amount each, forced to 1 on odd offsets when the level has two banks.
    s32 idBase = 0;
    for (s32 i = 1; i <= 5; i++) {
        s32 size = *(s32*)(s + 0x34 + i * 4);
        idBase += *(s32*)(s + 0x68 + (i - 1) * 4);
        s32 idEnd = idBase + *(s32*)(s + 0x68 + i * 4);
        if (size > 0 && id <= idEnd) {
            result = *(s32*)(s + 0x8 + i * 4);
            if (gLevelSingleBank == 0 && (id - (idBase + 1)) % 2 == 1) {
                result = 1;
            }
            return result;
        }
    }
    // Groups 6..12: cumulative id ranges.
    s32 idEnd = 0;
    for (s32 i = 0; i < 6; i++) {
        idEnd += *(s32*)(s + 0x68 + i * 4);
    }
    for (s32 i = 6; i < 13; i++) {
        s32 size = *(s32*)(s + 0x34 + i * 4);
        idEnd += *(s32*)(s + 0x68 + i * 4);
        if (size > 0 && id <= idEnd) {
            switch (i) {
            case 6:
                return *(s32*)(s + 0x20);
            case 7:
                return *(s32*)(s + 0x24);
            case 8:
                return (size1 - 1) * *(s32*)(s + 0xC) + (size2 - 1) * *(s32*)(s + 0x10) +
                       (size3 - 1) * *(s32*)(s + 0x14) + (size4 - 1) * *(s32*)(s + 0x18) +
                       (size5 - 1) * *(s32*)(s + 0x1C);
            case 9:
                return *(s32*)(s + 0x28);
            case 10:
                return (*(s32*)(s + 0x58) * *(s32*)(s + 0x28)) / size;
            case 11:
                return *(s32*)(s + 0x28);
            default:
                return *(s32*)(s + 0x2C);
            }
        }
    }
    return result;
}

// 0x801E1BD4
// Hook script interpreter: runs a word-coded program (opcode in bits 0-7, length in bits 8-15)
// with a small value stack. Returns the result register.
ScriptValue fn_801E1BD4(u8* obj, u8* a2, u32* pc) {
    u32 stack[64];
    u32* sp = stack;

    if (fn_801E2368((u8*)pc) == 0) {
        return 0;
    }
    if (pc == NULL) {
        return 0;
    }
    while ((fn_801E2380(*pc, 8, 0) & 0xFF) != 0xFF) {
        u32 word = *pc;
        u32 op = fn_801E2380(word, 8, 0) & 0xFF;
        u32 len = fn_801E2380(word, 8, 8) & 0xFF;

        if (op == 0x16) {
            // Call a script function through its signature.
            u32 funcIdx = fn_801E2380(word, 12, 16);
            u32 type = fn_801E23B0(funcIdx);
            if (fn_801E23D8((s32)type) == -2) {
                u32 buf[9];
                switch (type) {
                case 0x47:
                case 0x4A:
                case 0x53:
                case 0x54: {
                    u32 n = (u32)fn_801E2420((s32)type) - 1;
                    memcpy(buf, pc, (n + 1) * 4);
                    s32 mask = fn_801E2448((s32)type);
                    if (mask != 0) {
                        fn_801E2470(&buf[1], n, (u32)mask, obj, a2);
                    }
                    if (type == 0x4A) {
                        fn_801E2724(funcIdx, buf[1], pc + 2);
                    } else if (type == 0x54) {
                        fn_801E2748(funcIdx, buf[1], pc + 2);
                    } else if (type == 0x47) {
                        fn_801E276C(funcIdx, buf[1], buf[2], pc + 3);
                    } else if (type == 0x53) {
                        fn_801E2794(funcIdx, buf[1], buf[2], pc + 3);
                    }
                    pc += len;
                    continue;
                }
                case 0x8B:
                    fn_801E2400(funcIdx, pc + 1);
                    pc += len;
                    continue;
                default:
                    // Unknown special type: the original leaves the pc unchanged.
                    continue;
                }
            }
            u32 count = (u32)fn_801E2420((s32)type);
            if (count > 8) {
                count = 8;
            }
            u32 args[9];
            memcpy(args, pc, (count + 1) * 4);
            s32 ok = 1;
            s32 mask = fn_801E2448((s32)type);
            if (mask != 0) {
                ok = fn_801E2470(&args[1], count, (u32)mask, obj, a2);
            }
            if (ok == 0) {
                GameLog(4, 3, lbl_80385BD8, funcIdx);
                pc += len;
                continue;
            }
            if (ScriptDispatchEngine((s32)funcIdx, type, args, (u32*)lbl_805FB89C) == 0) {
                HookFallbackFn fallback = *(HookFallbackFn*)lbl_805FB890;
                if (fallback != NULL) {
                    u32 extra = 0;
                    *(u32*)lbl_805FB89C = fallback(funcIdx, type, args, &extra);
                }
            }
            pc += len;
            continue;
        }
        if (op > 0x1B) {
            pc += 1;
            continue;
        }

        switch (op) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 21:
            pc += len;
            break;
        case 4:  // unconditional jump
            pc += (s32)fn_801E239C((s32)word);
            break;
        case 5:  // jump if the result register is zero
            if (*(u32*)lbl_805FB89C == 0) {
                pc += (s32)fn_801E239C((s32)word);
            } else {
                pc += len;
            }
            break;
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11: {  // integer compare-and-jump (lo = value below the top, hi = top)
            s32 lo, hi;
            PopTwoInts(sp, lo, hi);
            bool jump = false;
            switch (op) {
            case 6:  jump = (lo == hi); break;
            case 7:  jump = (lo != hi); break;
            case 8:  jump = (lo > hi);  break;
            case 9:  jump = (lo < hi);  break;
            case 10: jump = (lo >= hi); break;
            default: jump = (lo <= hi); break;
            }
            if (jump) {
                pc += (s32)fn_801E239C((s32)word);
            } else {
                pc += len;
            }
            break;
        }
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17: {  // float compare-and-jump
            f32 lo, hi;
            PopTwoFloats(sp, lo, hi);
            bool jump = false;
            switch (op) {
            case 12: jump = (lo == hi); break;
            case 13: jump = (lo != hi); break;
            case 14: jump = (lo > hi);  break;
            case 15: jump = (lo < hi);  break;
            case 16: jump = (lo >= hi); break;
            default: jump = (lo <= hi); break;
            }
            if (jump) {
                pc += (s32)fn_801E239C((s32)word);
            } else {
                pc += len;
            }
            break;
        }
        case 18:  // push int immediate
            *sp++ = pc[1];
            pc += len;
            break;
        case 19:  // push float immediate
            *sp++ = fn_801D0340(*(f32*)(pc + 1));
            pc += len;
            break;
        case 20: {  // int -> float on top of stack
            s32 v = (s32)sp[-1];
            sp[-1] = 0;
            sp[-1] = fn_801D0340((f32)v);
            pc += len;
            break;
        }
        case 23: {  // run a trigger action
            u32 action = pc[1];
            *(u32*)lbl_805FB89C = (u32)TriggerDoAction(&action, obj, a2);
            pc += len;
            break;
        }
        case 24:  // push result register
            *sp++ = *(u32*)lbl_805FB89C;
            pc += len;
            break;
        case 25: {  // pop into result register
            u32 v = sp[-1];
            pc += len;
            sp -= 1;
            *sp = 0;
            *(u32*)lbl_805FB89C = v;
            break;
        }
        case 26:  // push result register as float bits
            *sp++ = fn_801D0340(fn_801D0354(*(u32*)lbl_805FB89C));
            pc += len;
            break;
        case 27: {  // pop float into result register as int
            sp -= 1;
            f32 f = fn_801D0354(*sp);
            pc += len;
            *sp = 0;
            *(u32*)lbl_805FB89C = (u32)(s32)f;
            break;
        }
        default:
            pc += 1;
            break;
        }
    }
    return (ScriptValue) * (u32*)lbl_805FB89C;
}

}  // extern "C"
