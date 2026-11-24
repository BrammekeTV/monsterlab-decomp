// PC port: differential tests (tools/difftest.py, docs/difftest.md). For a game function this runs
// the original PowerPC code in a small interpreter and the decompiled C++ on the same inputs and
// compares what they return and what they write. The original instructions come from the player's
// own main.dol at test time (PortDolImage); nothing of it is in the repository.
//
// Memory: both sides use the same bytes in host order at the width of each access, which is what the
// port's data conversion assumes everywhere. Read-only small data (.sdata2: float constants) is read
// by the interpreter from the original big-endian image, so a wrongly typed constant shows up.
// 64-bit float slots on the interpreter's own stack keep the console's word order (the compiler's
// int <-> float conversions store two words and load a double).
// No original code. The jobs file is written by tools/difftest.py:
//   seed N | calls 0/1 | only CASE | verbose 1 | job ADDR SIG CASES     (SIG: return ':' arguments, see ParseSig)
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>

#include "port/port.h"

BOOL PortDolImage(u8* dst, u32 lo, u32 hi, u32* secAddr, u32* secSize);   // port_runtime.cpp: every section, as on the disc

namespace {

const u32 kGameLo = 0x80006B40u, kGameHi = 0x8022FEC4u;
const u32 kSentinel = 0xFFFF0000u;         // the return address of the function under test
const u32 kPoolBufs = 8, kBufSize = 0x800, kBufMid = 0x200;
const u32 kStackSize = 0x10000;
const u32 kBudget = 400000;                // instructions per case

struct Cpu {
    u32 r[32];
    union { double d; u64 u; } f[32];
    double ps1[32];                        // the second half of each paired-single register (saved and restored by prologues)
    u32 cr, lr, ctr, ca, pc;
};

struct Write { u32 addr, size; u8 old[8]; BOOL isFloat; };
struct Read  { u32 addr, size; u32 value; };

std::vector<u8> sDol;                      // main.dol's sections at their addresses, big-endian, from PORT_TEXT_LO
u32 sDolHi;
u32 sSda2Lo, sSda2Hi;                      // .sdata2
u32 sR2, sR13;
u8* sPool; u8* sStack;                     // sandbox: kPoolBufs buffers, the interpreter's stack
u32 sDataLo, sDataHi;                      // the executable's data and bss
std::vector<u8> sSnap;                     // their bytes before any test
std::vector<Write> sJournal;
std::vector<Read> sReads;
char sWhy[160];                            // why the interpreter stopped (unsupported, or this case is skipped)
BOOL sUnsupported;
BOOL sCalls;
u64 sInsns;

u32 Be32(const u8* p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }

BOOL In(u32 a, u32 n, u32 lo, u32 size) { return a >= lo && a - lo < size && n <= size - (a - lo); }
BOOL InPool(u32 a, u32 n)  { return In(a, n, (u32)(uintptr_t)sPool, kPoolBufs * kBufSize); }
BOOL InStack(u32 a, u32 n) { return In(a, n, (u32)(uintptr_t)sStack, kStackSize); }
BOOL InData(u32 a, u32 n)  { return In(a, n, sDataLo, sDataHi - sDataLo); }

BOOL Stop(BOOL unsupported, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sWhy, sizeof(sWhy), fmt, ap);
    va_end(ap);
    sUnsupported = unsupported;
    return FALSE;
}

const PortFunc* FuncAt(u32 addr) {         // the function this address is in
    s32 lo = 0, hi = (s32)gPortFuncCount - 1, best = -1;
    while (lo <= hi) {
        s32 mid = (lo + hi) / 2;
        if (gPortFuncs[mid].addr <= addr) { best = mid; lo = mid + 1; } else hi = mid - 1;
    }
    return best < 0 ? NULL : &gPortFuncs[best];
}

// ---- memory ----
BOOL MemRead(u32 a, u32 n, void* out) {
    if (In(a, n, sSda2Lo, sSda2Hi - sSda2Lo)) {                 // constants: the original bytes
        const u8* p = &sDol[a - PORT_TEXT_LO];
        u8* o = (u8*)out;
        for (u32 i = 0; i < n; i++) o[i] = p[n - 1 - i];
        return TRUE;
    }
    if (!InPool(a, n) && !InStack(a, n) && !InData(a, n)) return Stop(FALSE, "reads 0x%08X, outside the test's memory", a);
    memcpy(out, (void*)(uintptr_t)a, n);
    if (InPool(a, n) && sReads.size() < 4096) { Read rd = {a, n, 0}; memcpy(&rd.value, out, n < 4 ? n : 4); sReads.push_back(rd); }
    return TRUE;
}

BOOL MemWrite(u32 a, u32 n, const void* in, BOOL isFloat) {
    if (!InPool(a, n) && !InStack(a, n) && !InData(a, n)) return Stop(FALSE, "writes 0x%08X, outside the test's memory", a);
    if (!InStack(a, n)) {
        Write w = {a, n, {0}, isFloat};
        memcpy(w.old, (void*)(uintptr_t)a, n);
        sJournal.push_back(w);
    }
    memcpy((void*)(uintptr_t)a, in, n);
    return TRUE;
}

// A float the test's memory happens to hold as NaN or infinity: comparisons with those differ in ways no game state shows.
// Denormals too (small integers read as floats): the PC build keeps more precision in between than the console.
BOOL Finite(double v) { return v == 0.0 || (v == v && fabs(v) < 1e30 && fabs(v) > 1e-30) ? TRUE : Stop(FALSE, "loads a float that is NaN, huge or denormal"); }

BOOL ReadF64(u32 a, u64* v) {
    if (InStack(a, 8)) {                                    // console word order: high word first
        u32 hi, lo;
        if (!MemRead(a, 4, &hi) || !MemRead(a + 4, 4, &lo)) return FALSE;
        *v = (u64)hi << 32 | lo;
        return TRUE;
    }
    return MemRead(a, 8, v);
}
BOOL WriteF64(u32 a, u64 v) {
    if (InStack(a, 8)) {
        u32 hi = (u32)(v >> 32), lo = (u32)v;
        return MemWrite(a, 4, &hi, FALSE) && MemWrite(a + 4, 4, &lo, FALSE);
    }
    return MemWrite(a, 8, &v, TRUE);
}

// ---- the interpreter ----
inline u32 Rotl(u32 x, u32 n) { n &= 31; return n ? (x << n) | (x >> (32 - n)) : x; }
inline u32 Mask(u32 mb, u32 me) {
    u32 a = 0xFFFFFFFFu >> mb, b = 0xFFFFFFFFu << (31 - me);
    return mb <= me ? (a & b) : (a | b);
}
inline void SetCr(Cpu& c, u32 field, u32 bits) { u32 sh = (7 - field) * 4; c.cr = (c.cr & ~(15u << sh)) | (bits << sh); }
inline void Cr0(Cpu& c, u32 v) { SetCr(c, 0, (s32)v < 0 ? 8 : v ? 4 : 2); }
inline double Single(double v) { return (double)(float)v; }

// A call or a jump to another function: may the interpreter follow it?
BOOL MayEnter(u32 target, u32 self) {
    const PortFunc* f = FuncAt(target);
    if (target < PORT_TEXT_LO || target >= sDolHi || !f) return Stop(FALSE, "calls through a pointer that is no code (0x%08X)", target);
    const char* name = PortFuncName(f);
    if (target >= kGameLo && target < kGameHi) {
        if (f->addr == self && target != self) return TRUE;                  // inside the function
        if (f->addr == self) return sCalls ? TRUE : Stop(TRUE, "calls itself");
        if (!sCalls) return Stop(TRUE, "calls %s", name);
        if (!f->impl) return Stop(TRUE, "calls %s, which is not decompiled", name);
        return TRUE;
    }
    // Compiler helpers: register save and restore always; 64-bit arithmetic and conversions with calls allowed.
    if (!strncmp(name, "_save", 5) || !strncmp(name, "_rest", 5) || !strncmp(name, "__save", 6) || !strncmp(name, "__rest", 6)) return TRUE;
    if (sCalls && (!strncmp(name, "__div", 5) || !strncmp(name, "__mod", 5) || !strncmp(name, "__shl", 5) || !strncmp(name, "__shr", 5) || !strncmp(name, "__cvt", 5))) return TRUE;
    return Stop(TRUE, "calls %s (library)", name);
}

BOOL Run(Cpu& c, u32 entry) {
    c.pc = entry;
    c.lr = kSentinel;
    for (u32 n = 0; c.pc != kSentinel; n++, sInsns++) {
        if (n >= kBudget) return Stop(FALSE, "runs more than %u instructions", kBudget);
        if (c.pc < PORT_TEXT_LO || c.pc + 4 > sDolHi || (c.pc & 3)) return Stop(FALSE, "jumps to 0x%08X", c.pc);
        u32 op = Be32(&sDol[c.pc - PORT_TEXT_LO]);
        u32 pc = c.pc, next = pc + 4;
        u32 opc = op >> 26, rd = (op >> 21) & 31, ra = (op >> 16) & 31, rb = (op >> 11) & 31, rc = (op >> 6) & 31;
        s32 simm = (s16)op;
        u32 uimm = op & 0xFFFF;
        u32* R = c.r;
        #define EA_D  ((ra ? R[ra] : 0) + (u32)simm)
        #define EA_X  ((ra ? R[ra] : 0) + R[rb])
        #define BAD() return Stop(TRUE, "instruction %08X at 0x%08X is not implemented", op, pc)
        switch (opc) {
        case 7:  R[rd] = (u32)((s32)R[ra] * simm); break;
        case 8:  { u64 t = (u64)(u32)~R[ra] + (u32)simm + 1; R[rd] = (u32)t; c.ca = (u32)(t >> 32) & 1; break; }
        case 10: SetCr(c, rd >> 2, R[ra] < uimm ? 8 : R[ra] > uimm ? 4 : 2); break;
        case 11: SetCr(c, rd >> 2, (s32)R[ra] < simm ? 8 : (s32)R[ra] > simm ? 4 : 2); break;
        case 12: case 13: { u64 t = (u64)R[ra] + (u32)simm; R[rd] = (u32)t; c.ca = (u32)(t >> 32) & 1; if (opc == 13) Cr0(c, R[rd]); break; }
        case 14: R[rd] = (ra ? R[ra] : 0) + (u32)simm; break;
        case 15: R[rd] = (ra ? R[ra] : 0) + (uimm << 16); break;
        case 16: case 18: case 19: {
            u32 target = 0;
            BOOL take = TRUE, link = op & 1, isBranch = TRUE;
            if (opc == 18) {
                s32 li = (s32)(op << 6) >> 6 & ~3;
                target = (op & 2) ? (u32)li : pc + (u32)li;
            } else {
                u32 xo = (op >> 1) & 0x3FF;
                if (opc == 19 && xo != 16 && xo != 528) {   // condition register logic
                    u32 a = (c.cr >> (31 - ra)) & 1, b = (c.cr >> (31 - rb)) & 1, v;
                    switch (xo) {
                    case 0:   SetCr(c, rd >> 2, (c.cr >> ((7 - (ra >> 2)) * 4)) & 15); v = 2; break;   // mcrf
                    case 33:  v = !(a | b); break;
                    case 129: v = a & !b; break;
                    case 193: v = a ^ b; break;
                    case 225: v = !(a & b); break;
                    case 257: v = a & b; break;
                    case 289: v = !(a ^ b); break;
                    case 417: v = a | !b; break;
                    case 449: v = a | b; break;
                    default: BAD();
                    }
                    if (v < 2) c.cr = (c.cr & ~(1u << (31 - rd))) | (v << (31 - rd));
                    isBranch = FALSE;
                } else {
                    u32 bo = rd, bi = ra;
                    BOOL ctrOk = TRUE;
                    if (!(bo & 4) && !(opc == 19 && xo == 528)) { c.ctr--; ctrOk = (c.ctr != 0) != ((bo >> 1) & 1); }
                    BOOL condOk = (bo & 16) || (((c.cr >> (31 - bi)) & 1) == ((bo >> 3) & 1));
                    take = ctrOk && condOk;
                    if (opc == 16) target = (op & 2) ? (u32)(s32)(s16)(op & 0xFFFC) : pc + (u32)(s32)(s16)(op & 0xFFFC);
                    else target = (xo == 16 ? c.lr : c.ctr) & ~3u;
                }
            }
            if (!isBranch) break;
            if (link) c.lr = pc + 4;
            if (!take) break;
            if (target != kSentinel) {
                const PortFunc* from = FuncAt(pc);
                BOOL ret = opc == 19 && ((op >> 1) & 0x3FF) == 16;      // blr: back into a caller we already entered
                if (!ret && (link || from != FuncAt(target)) && !MayEnter(target, from ? from->addr : 0)) return FALSE;
            }
            next = target;
            break;
        }
        case 20: { u32 m = Mask(rc, (op >> 1) & 31); R[ra] = (Rotl(R[rd], rb) & m) | (R[ra] & ~m); if (op & 1) Cr0(c, R[ra]); break; }
        case 21: R[ra] = Rotl(R[rd], rb) & Mask(rc, (op >> 1) & 31); if (op & 1) Cr0(c, R[ra]); break;
        case 23: R[ra] = Rotl(R[rd], R[rb]) & Mask(rc, (op >> 1) & 31); if (op & 1) Cr0(c, R[ra]); break;
        case 24: R[ra] = R[rd] | uimm; break;
        case 25: R[ra] = R[rd] | (uimm << 16); break;
        case 26: R[ra] = R[rd] ^ uimm; break;
        case 27: R[ra] = R[rd] ^ (uimm << 16); break;
        case 28: R[ra] = R[rd] & uimm; Cr0(c, R[ra]); break;
        case 29: R[ra] = R[rd] & (uimm << 16); Cr0(c, R[ra]); break;
        case 31: {
            u32 xo = (op >> 1) & 0x3FF;
            BOOL rcBit = op & 1, toRa = FALSE;
            u32 a = R[ra], b = R[rb], s = R[rd];
            switch (xo & 0x1FF) {                            // arithmetic: bit 9 is the overflow form (same result)
            case 8:   { u64 t = (u64)(u32)~a + b + 1; R[rd] = (u32)t; c.ca = (u32)(t >> 32) & 1; goto arith; }
            case 10:  { u64 t = (u64)a + b; R[rd] = (u32)t; c.ca = (u32)(t >> 32) & 1; goto arith; }
            case 40:  R[rd] = b - a; goto arith;
            case 104: R[rd] = 0u - a; goto arith;
            case 136: { u64 t = (u64)(u32)~a + b + c.ca; R[rd] = (u32)t; c.ca = (u32)(t >> 32) & 1; goto arith; }
            case 138: { u64 t = (u64)a + b + c.ca; R[rd] = (u32)t; c.ca = (u32)(t >> 32) & 1; goto arith; }
            case 200: { u64 t = (u64)(u32)~a + c.ca; R[rd] = (u32)t; c.ca = (u32)(t >> 32) & 1; goto arith; }
            case 202: { u64 t = (u64)a + c.ca; R[rd] = (u32)t; c.ca = (u32)(t >> 32) & 1; goto arith; }
            case 232: { u64 t = (u64)(u32)~a + c.ca + 0xFFFFFFFFull; R[rd] = (u32)t; c.ca = (u32)(t >> 32) & 1; goto arith; }
            case 234: { u64 t = (u64)a + c.ca + 0xFFFFFFFFull; R[rd] = (u32)t; c.ca = (u32)(t >> 32) & 1; goto arith; }
            case 235: R[rd] = (u32)((s32)a * (s32)b); goto arith;
            case 266: R[rd] = a + b; goto arith;
            case 459: if (!b) return Stop(FALSE, "divides by zero (undefined on the console)"); R[rd] = a / b; goto arith;
            case 491: if (!b || (a == 0x80000000u && b == 0xFFFFFFFFu)) return Stop(FALSE, "divides by zero (undefined on the console)");
                      R[rd] = (u32)((s32)a / (s32)b); goto arith;
            default: break;
            }
            switch (xo) {
            case 0:   SetCr(c, rd >> 2, (s32)a < (s32)b ? 8 : (s32)a > (s32)b ? 4 : 2); rcBit = FALSE; break;
            case 32:  SetCr(c, rd >> 2, a < b ? 8 : a > b ? 4 : 2); rcBit = FALSE; break;
            case 11:  R[rd] = (u32)(((u64)a * b) >> 32); goto arith;
            case 75:  R[rd] = (u32)(((s64)(s32)a * (s32)b) >> 32); goto arith;
            case 19:  R[rd] = c.cr; rcBit = FALSE; break;
            case 144: { u32 m = 0; for (u32 i = 0; i < 8; i++) if (op & (0x80000u >> i)) m |= 0xF0000000u >> (i * 4); c.cr = (c.cr & ~m) | (s & m); rcBit = FALSE; break; }
            case 339: { u32 spr = (op >> 16 & 31) | (op >> 6 & 0x3E0); if (spr == 8) R[rd] = c.lr; else if (spr == 9) R[rd] = c.ctr; else BAD(); rcBit = FALSE; break; }
            case 467: { u32 spr = (op >> 16 & 31) | (op >> 6 & 0x3E0); if (spr == 8) c.lr = s; else if (spr == 9) c.ctr = s; else BAD(); rcBit = FALSE; break; }
            case 24:  R[ra] = (b & 32) ? 0 : s << (b & 31); toRa = TRUE; break;
            case 536: R[ra] = (b & 32) ? 0 : s >> (b & 31); toRa = TRUE; break;
            case 792: { u32 n2 = b & 63; if (n2 > 31) { R[ra] = (u32)((s32)s >> 31); c.ca = (s32)s < 0 && s; } else { c.ca = (s32)s < 0 && (s & ((1u << n2) - 1)); R[ra] = (u32)((s32)s >> n2); } toRa = TRUE; break; }
            case 824: { c.ca = (s32)s < 0 && rb && (s & ((1u << rb) - 1)); R[ra] = (u32)((s32)s >> rb); toRa = TRUE; break; }
            case 26:  { u32 k = 0; while (k < 32 && !(s & (0x80000000u >> k))) k++; R[ra] = k; toRa = TRUE; break; }
            case 28:  R[ra] = s & b; toRa = TRUE; break;
            case 60:  R[ra] = s & ~b; toRa = TRUE; break;
            case 124: R[ra] = ~(s | b); toRa = TRUE; break;
            case 284: R[ra] = ~(s ^ b); toRa = TRUE; break;
            case 316: R[ra] = s ^ b; toRa = TRUE; break;
            case 412: R[ra] = s | ~b; toRa = TRUE; break;
            case 444: R[ra] = s | b; toRa = TRUE; break;
            case 476: R[ra] = ~(s & b); toRa = TRUE; break;
            case 922: R[ra] = (u32)(s32)(s16)s; toRa = TRUE; break;
            case 954: R[ra] = (u32)(s32)(s8)s; toRa = TRUE; break;
            // indexed loads and stores (bit 0x20 of xo: update)
            case 23: case 55:   { u32 ea = EA_X; u32 v; if (!MemRead(ea, 4, &v)) return FALSE; R[rd] = v; if (xo == 55) R[ra] = ea; rcBit = FALSE; break; }
            case 87: case 119:  { u32 ea = EA_X; u8 v; if (!MemRead(ea, 1, &v)) return FALSE; R[rd] = v; if (xo == 119) R[ra] = ea; rcBit = FALSE; break; }
            case 279: case 311: { u32 ea = EA_X; u16 v; if (!MemRead(ea, 2, &v)) return FALSE; R[rd] = v; if (xo == 311) R[ra] = ea; rcBit = FALSE; break; }
            case 343: case 375: { u32 ea = EA_X; s16 v; if (!MemRead(ea, 2, &v)) return FALSE; R[rd] = (u32)(s32)v; if (xo == 375) R[ra] = ea; rcBit = FALSE; break; }
            case 151: case 183: { u32 ea = EA_X; if (!MemWrite(ea, 4, &s, FALSE)) return FALSE; if (xo == 183) R[ra] = ea; rcBit = FALSE; break; }
            case 215: case 247: { u32 ea = EA_X; u8 v = (u8)s; if (!MemWrite(ea, 1, &v, FALSE)) return FALSE; if (xo == 247) R[ra] = ea; rcBit = FALSE; break; }
            case 407: case 439: { u32 ea = EA_X; u16 v = (u16)s; if (!MemWrite(ea, 2, &v, FALSE)) return FALSE; if (xo == 439) R[ra] = ea; rcBit = FALSE; break; }
            case 535: case 567: { u32 ea = EA_X; f32 v; if (!MemRead(ea, 4, &v) || !Finite(v)) return FALSE; c.f[rd].d = v; if (xo == 567) R[ra] = ea; rcBit = FALSE; break; }
            case 599: case 631: { u32 ea = EA_X; if (!ReadF64(ea, &c.f[rd].u)) return FALSE; if (xo == 631) R[ra] = ea; rcBit = FALSE; break; }
            case 663: case 695: { u32 ea = EA_X; f32 v = (f32)c.f[rd].d; if (!MemWrite(ea, 4, &v, TRUE)) return FALSE; if (xo == 695) R[ra] = ea; rcBit = FALSE; break; }
            case 727: case 759: { u32 ea = EA_X; if (!WriteF64(ea, c.f[rd].u)) return FALSE; if (xo == 759) R[ra] = ea; rcBit = FALSE; break; }
            case 983: { u32 ea = EA_X; u32 v = (u32)c.f[rd].u; if (!MemWrite(ea, 4, &v, FALSE)) return FALSE; rcBit = FALSE; break; }
            default: BAD();
            }
            if (toRa && rcBit) Cr0(c, R[ra]);
            break;
        arith:
            if (op & 1) Cr0(c, R[rd]);
            break;
        }
        case 32: case 33: { u32 ea = EA_D; u32 v; if (!MemRead(ea, 4, &v)) return FALSE; R[rd] = v; if (opc & 1) R[ra] = ea; break; }
        case 34: case 35: { u32 ea = EA_D; u8 v; if (!MemRead(ea, 1, &v)) return FALSE; R[rd] = v; if (opc & 1) R[ra] = ea; break; }
        case 36: case 37: { u32 ea = EA_D; u32 v = R[rd]; if (!MemWrite(ea, 4, &v, FALSE)) return FALSE; if (opc & 1) R[ra] = ea; break; }
        case 38: case 39: { u32 ea = EA_D; u8 v = (u8)R[rd]; if (!MemWrite(ea, 1, &v, FALSE)) return FALSE; if (opc & 1) R[ra] = ea; break; }
        case 40: case 41: { u32 ea = EA_D; u16 v; if (!MemRead(ea, 2, &v)) return FALSE; R[rd] = v; if (opc & 1) R[ra] = ea; break; }
        case 42: case 43: { u32 ea = EA_D; s16 v; if (!MemRead(ea, 2, &v)) return FALSE; R[rd] = (u32)(s32)v; if (opc & 1) R[ra] = ea; break; }
        case 44: case 45: { u32 ea = EA_D; u16 v = (u16)R[rd]; if (!MemWrite(ea, 2, &v, FALSE)) return FALSE; if (opc & 1) R[ra] = ea; break; }
        case 46: { u32 ea = EA_D; for (u32 i = rd; i < 32; i++, ea += 4) { u32 v; if (!MemRead(ea, 4, &v)) return FALSE; R[i] = v; } break; }
        case 47: { u32 ea = EA_D; for (u32 i = rd; i < 32; i++, ea += 4) if (!MemWrite(ea, 4, &R[i], FALSE)) return FALSE; break; }
        case 48: case 49: { u32 ea = EA_D; f32 v; if (!MemRead(ea, 4, &v) || !Finite(v)) return FALSE; c.f[rd].d = v; if (opc & 1) R[ra] = ea; break; }
        case 50: case 51: { u32 ea = EA_D; if (!ReadF64(ea, &c.f[rd].u)) return FALSE; if (opc & 1) R[ra] = ea; break; }
        case 52: case 53: { u32 ea = EA_D; f32 v = (f32)c.f[rd].d; if (!MemWrite(ea, 4, &v, TRUE)) return FALSE; if (opc & 1) R[ra] = ea; break; }
        case 54: case 55: { u32 ea = EA_D; if (!WriteF64(ea, c.f[rd].u)) return FALSE; if (opc & 1) R[ra] = ea; break; }
        case 56: case 57: case 60: case 61: {                // psq_l(u), psq_st(u): unscaled floats only (quantisation register 0)
            u32 ea = (ra ? R[ra] : 0) + (u32)((s32)(op << 20) >> 20);
            BOOL one = (op >> 15) & 1;
            if ((op >> 12) & 7) BAD();
            if (opc < 60) {
                f32 v0, v1 = 1.0f;
                if (!MemRead(ea, 4, &v0) || (!one && !MemRead(ea + 4, 4, &v1))) return FALSE;
                c.f[rd].d = v0; c.ps1[rd] = v1;
            } else {
                f32 v0 = (f32)c.f[rd].d, v1 = (f32)c.ps1[rd];
                if (!MemWrite(ea, 4, &v0, TRUE) || (!one && !MemWrite(ea + 4, 4, &v1, TRUE))) return FALSE;
            }
            if (opc & 1) R[ra] = ea;
            break;
        }
        case 59: case 63: {
            u32 xo5 = (op >> 1) & 31, xo = (op >> 1) & 0x3FF;
            double a = c.f[ra].d, b = c.f[rb].d, m = c.f[rc].d, v = 0;
            BOOL single = opc == 59, set = TRUE;
            if (xo5 >= 18 && xo5 != 22 && !(opc == 63 && (xo == 32 || xo == 40 || xo == 72 || xo == 136 || xo == 264 || xo == 583 || xo == 711 || xo == 70 || xo == 38 || xo == 134))) {
                switch (xo5) {
                case 18: v = a / b; break;
                case 20: v = a - b; break;
                case 21: v = a + b; break;
                case 23: if (single) BAD(); v = a >= 0.0 ? m : b; break;                      // fsel
                case 24: if (!single) BAD(); v = 1.0 / b; break;                             // fres
                case 25: v = a * m; break;
                case 26: if (single) BAD(); v = 1.0 / sqrt(b); break;                        // frsqrte (the console's is an estimate)
                case 28: v = fma(a, m, -b); break;
                case 29: v = fma(a, m, b); break;
                case 30: v = -fma(a, m, -b); break;
                case 31: v = -fma(a, m, b); break;
                default: BAD();
                }
                if (single) v = Single(v);
            } else if (opc == 63) {
                switch (xo) {
                case 0: case 32: SetCr(c, rd >> 2, (a != a || b != b) ? 1 : a < b ? 8 : a > b ? 4 : 2); set = FALSE; break;
                case 12: v = Single(b); break;
                case 14: case 15: {
                    double t = xo == 15 ? trunc(b) : nearbyint(b);
                    if (b != b || t > 2147483647.0 || t < -2147483648.0) return Stop(FALSE, "converts a float outside the int range (undefined in C++)");
                    c.f[rd].u = (u64)(u32)(s32)t; set = FALSE; break;
                }
                case 40:  v = -b; break;
                case 72:  c.f[rd].u = c.f[rb].u; set = FALSE; break;
                case 136: v = -fabs(b); break;
                case 264: v = fabs(b); break;
                case 583: c.f[rd].u = 0; set = FALSE; break;       // mffs: rounding to nearest, no flags
                case 711: case 70: case 38: case 134: set = FALSE; break;   // mtfsf, mtfsb0, mtfsb1, mtfsfi: the mode stays as it is
                default: BAD();
                }
            } else BAD();
            if (set) c.f[rd].d = v;
            break;
        }
        default: BAD();
        }
        #undef EA_D
        #undef EA_X
        #undef BAD
        c.pc = next;
    }
    return TRUE;
}

// ---- test cases ----
struct Rng { u64 s; u32 Next() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return (u32)(s >> 16); } };

// Signature from tools/difftest.py: return kind ':' argument kinds.
//   v nothing; z bool; b u8; c s8; h u16; s s16; i 32-bit integer; p pointer; f float; d double
struct Sig { char ret; char arg[16]; u32 n; };
BOOL ParseSig(const char* text, Sig* sig) {
    memset(sig, 0, sizeof(*sig));
    if (!text[0] || text[1] != ':' || !strchr("vbzchsipfd", text[0])) return FALSE;
    sig->ret = text[0];
    for (const char* p = text + 2; *p; p++) {
        if (!strchr("bzchsipfd", *p) || sig->n >= 16) return FALSE;
        sig->arg[sig->n++] = *p;
    }
    return TRUE;
}

const u32 kEdge[] = {0, 1, 2, 0xFFFFFFFFu, 0x7FFFFFFFu, 0x80000000u, 0xFF, 0x100, 0x7F, 0x80, 0xFFFF, 0x8000, 3, 16, 31, 32};
const f32 kFloats[] = {0.0f, 1.0f, -1.0f, 0.5f, 2.0f, 100.0f, -0.25f, 1e-3f, 3.14159274f, 640.0f, 60.0f, 255.0f};

u8* Buf(u32 i) { return sPool + (i % kPoolBufs) * kBufSize; }

u32 RandomWord(Rng& g, u32 style) {
    u32 k = g.Next() % 100;
    if (style == 0) return g.Next() % 17;                    // small numbers only
    if (style == 2) {                                        // floats, zeros and pointers: for code that reads float fields
        if (k < 20) return 0;
        if (k < 40) return (u32)(uintptr_t)Buf(g.Next()) + kBufMid + (g.Next() % 16) * 4;
        f32 f = k < 70 ? kFloats[g.Next() % 12] * (f32)((g.Next() % 7) + 1) : (f32)((s32)(g.Next() % 20001) - 10000) / 16.0f;
        u32 v; memcpy(&v, &f, 4); return v;
    }
    if (k < 25) return 0;
    if (k < 45) return g.Next() % 9;
    if (k < 65) return (u32)(uintptr_t)Buf(g.Next()) + kBufMid + (g.Next() % 16) * 4;   // a pointer into the test's memory
    if (k < 80) { f32 f = kFloats[g.Next() % 12] * (f32)((g.Next() % 7) + 1); u32 v; memcpy(&v, &f, 4); return v; }
    if (k < 88) return kEdge[g.Next() % 16];
    return g.Next() << 16 ^ g.Next();
}

struct Case { u32 ints[8]; double flts[8]; u32 words[16]; u32 nInts, nFlts, nWords; };

// Case 0: zeros; 1: ones; 2-5: edge values with empty memory; from 6: random memory and arguments.
BOOL MakeCase(const Sig& sig, u32 index, u64 seed, Case* cs) {
    Rng g = {seed * 0x9E3779B97F4A7C15ull + index * 0xD1B54A32D192ED03ull + 0x1234567};
    for (int i = 0; i < 8; i++) g.Next();
    memset(cs, 0, sizeof(*cs));
    memset(sPool, 0, kPoolBufs * kBufSize);
    if (index >= 6) {
        u32 style = index < 12 ? 0 : (index % 3 == 2) ? 2 : 1;
        for (u32 i = 0; i < kPoolBufs * kBufSize; i += 4) { u32 v = RandomWord(g, style); memcpy(sPool + i, &v, 4); }
    }
    u32 nextBuf = 0;
    for (u32 i = 0; i < sig.n; i++) {
        char k = sig.arg[i];
        if (k == 'f' || k == 'd') {
            if (cs->nFlts >= 8) return FALSE;
            f32 v = index == 0 ? 0.0f : index == 1 ? 1.0f : kFloats[g.Next() % 12];
            if (index >= 10 && (g.Next() & 1)) v = (f32)((s32)(g.Next() % 200001) - 100000) / 100.0f;
            cs->flts[cs->nFlts++] = v;
            if (k == 'f') memcpy(&cs->words[cs->nWords++], &v, 4);
            else { double d = v; memcpy(&cs->words[cs->nWords], &d, 8); cs->nWords += 2; }
            continue;
        }
        if (cs->nInts >= 8) return FALSE;
        u32 v;
        if (k == 'p') v = (index >= 20 && g.Next() % 16 == 0) ? 0 : (u32)(uintptr_t)Buf(nextBuf++) + kBufMid;
        else {
            v = index == 0 ? 0 : index == 1 ? 1 : index < 6 ? kEdge[(index * 5 + i * 3 + g.Next() % 3) % 16] : (g.Next() % 3 == 0 ? kEdge[g.Next() % 16] : g.Next() % 3 == 0 ? g.Next() << 16 ^ g.Next() : g.Next() % 40);
            if (k == 'z') v = index == 0 ? 0 : index == 1 ? 1 : g.Next() & 1; else if (k == 'b') v &= 0xFF; else if (k == 'c') v = (u32)(s32)(s8)v; else if (k == 'h') v &= 0xFFFF; else if (k == 's') v = (u32)(s32)(s16)v;
        }
        cs->ints[cs->nInts++] = v;
        cs->words[cs->nWords++] = v;
    }
    return TRUE;
}

// The C++ function: cdecl takes its arguments as 32-bit stack words (a double is two), whatever their types.
typedef u32    (__cdecl *FnInt)(u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);
typedef double (__cdecl *FnFlt)(u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32, u32);

BOOL FloatClose(double a, double b) {
    if ((a != a && b != b) || a == b) return TRUE;
    double d = fabs(a - b), m = std::max(1.0, std::max(fabs(a), fabs(b)));
    return d <= 1e-4 * m;
}

struct Diff { u32 addr; u32 want, got; BOOL isFloat; };

// The same thing by two addresses: a function (console address against the PC function), or a string
// constant (the original's in the executable's data, the C++'s own literal).
BOOL SamePointer(u32 want, u32 got) {
    if (want == got) return TRUE;
    if (want >= PORT_TEXT_LO && want < PORT_TEXT_HI) {
        const PortFunc* fn = PortFindFunc(want);
        return fn && fn->impl && got == (u32)(uintptr_t)fn->impl;
    }
    if (!InData(want, 1) || PortIsWiiAddr(got) || !got) return FALSE;
    MEMORY_BASIC_INFORMATION mi;
    if (!VirtualQuery((void*)(uintptr_t)got, &mi, sizeof(mi)) || mi.State != MEM_COMMIT || (mi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return FALSE;
    const char* a = (const char*)(uintptr_t)want;
    const char* b = (const char*)(uintptr_t)got;
    u32 room = (u32)((u8*)mi.BaseAddress + mi.RegionSize - (u8*)b);
    for (u32 i = 0; i < 256 && i < room && InData(want + i, 1); i++) {
        if (a[i] != b[i]) return FALSE;
        if (!a[i]) return i > 0;
    }
    return FALSE;
}

const char* Where(u32 addr, char* buf, u32 n) {
    if (InPool(addr, 1)) {
        u32 off = addr - (u32)(uintptr_t)sPool;
        snprintf(buf, n, "buf%u%+d", off / kBufSize, (s32)(off % kBufSize) - (s32)kBufMid);
    } else snprintf(buf, n, "0x%08X", addr);
    return buf;
}

struct Result { u32 ok, skipped, mismatched, cases; char firstSkip[160]; char unsupported[160]; };

void RestoreData(void) {
    for (u32 off = 0; off < sSnap.size(); off += 0x1000) {
        u32 n = (u32)std::min<size_t>(0x1000, sSnap.size() - off);
        if (memcmp((u8*)(uintptr_t)sDataLo + off, &sSnap[off], n)) memcpy((u8*)(uintptr_t)sDataLo + off, &sSnap[off], n);
    }
}

// One case: FALSE when it could not be judged (sWhy says why), else *match says whether both sides agree.
BOOL RunCase(const PortFunc* f, const Sig& sig, u32 index, u64 seed, BOOL verbose, BOOL* match) {
    Case cs;
    if (!MakeCase(sig, index, seed, &cs)) return Stop(TRUE, "more than 8 integer or 8 float arguments");
    std::vector<u8> input(sPool, sPool + kPoolBufs * kBufSize);

    // The original.
    Cpu c;
    memset(&c, 0, sizeof(c));
    memset(sStack, 0, kStackSize);
    c.r[1] = (u32)(uintptr_t)sStack + kStackSize - 0x400;
    c.r[2] = sR2; c.r[13] = sR13;
    for (u32 i = 0; i < cs.nInts; i++) c.r[3 + i] = cs.ints[i];
    for (u32 i = 0; i < cs.nFlts; i++) c.f[1 + i].d = cs.flts[i];
    sJournal.clear();
    sReads.clear();
    BOOL ran = Run(c, f->addr);
    std::vector<u8> want(sPool, sPool + kPoolBufs * kBufSize);
    std::map<u32, u8> wantData;                              // bytes of the executable's data the original wrote
    std::map<u32, u32> floatAt;                              // address -> size of a float store
    for (const Write& w : sJournal) {
        if (w.isFloat) floatAt[w.addr] = w.size;
        if (InData(w.addr, w.size)) for (u32 i = 0; i < w.size; i++) wantData[w.addr + i] = *(u8*)(uintptr_t)(w.addr + i);
    }
    for (size_t i = sJournal.size(); i-- > 0;) memcpy((void*)(uintptr_t)sJournal[i].addr, sJournal[i].old, sJournal[i].size);
    if (!ran) return FALSE;

    // The C++.
    memcpy(sPool, input.data(), input.size());
    const u32* a = cs.words;
    u32 gotInt = 0;
    double gotFlt = 0;
    printf("DTC %u\n", index);
    fflush(stdout);
    if (sig.ret == 'f' || sig.ret == 'd') gotFlt = ((FnFlt)f->impl)(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15]);
    else gotInt = ((FnInt)f->impl)(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15]);

    // Compare.
    std::vector<Diff> diffs;
    auto word = [&](u32 addr, const u8* w, const u8* g) {
        u32 base = addr & ~3u;
        if (!diffs.empty() && diffs.back().addr == base) return;
        Diff d = {base, 0, 0, FALSE};
        memcpy(&d.want, w - (addr - base), 4);
        memcpy(&d.got, g - (addr - base), 4);
        if (SamePointer(d.want, d.got)) return;
        auto fl = floatAt.find(base);
        if (fl != floatAt.end() && fl->second == 4) {
            f32 x, y; memcpy(&x, &d.want, 4); memcpy(&y, &d.got, 4);
            if (FloatClose(x, y)) return;
            d.isFloat = TRUE;
        }
        diffs.push_back(d);
    };
    for (u32 i = 0; i < kPoolBufs * kBufSize; i++)
        if (want[i] != sPool[i]) word((u32)(uintptr_t)sPool + i, &want[i], &sPool[i]);
    std::vector<u8> page(0x1000);
    for (u32 off = 0; off < sSnap.size(); off += 0x1000) {   // the executable's data: what the C++ changed, against what the original changed
        u32 n = (u32)std::min<size_t>(0x1000, sSnap.size() - off);
        u8* live = (u8*)(uintptr_t)sDataLo + off;
        auto lo = wantData.lower_bound(sDataLo + off);
        BOOL origWrote = lo != wantData.end() && lo->first < sDataLo + off + n;
        if (!origWrote && !memcmp(live, &sSnap[off], n)) continue;
        memcpy(page.data(), &sSnap[off], n);
        for (; lo != wantData.end() && lo->first < sDataLo + off + n; ++lo) page[lo->first - sDataLo - off] = lo->second;
        for (u32 i = 0; i < n; i++) if (page[i] != live[i]) word(sDataLo + off + i, &page[i], &live[i]);
    }
    RestoreData();
    u32 wantInt = c.r[3];
    BOOL retOk = TRUE;
    switch (sig.ret) {
    case 'z': case 'b': case 'c': retOk = (wantInt & 0xFF) == (gotInt & 0xFF); break;
    case 'h': case 's': retOk = (wantInt & 0xFFFF) == (gotInt & 0xFFFF); break;
    case 'i': case 'p': retOk = SamePointer(wantInt, gotInt); break;
    case 'f': case 'd': retOk = FloatClose(c.f[1].d, gotFlt); break;
    }
    *match = retOk && diffs.empty();
    if (*match && !verbose) return TRUE;

    // The reproducer: arguments, the memory the original read, and every difference.
    printf("DTM %s case %u seed %llu:", PortFuncName(f), index, seed);
    u32 ii = 0, fi = 0;
    char buf[48];
    for (u32 i = 0; i < sig.n; i++) {
        if (sig.arg[i] == 'f' || sig.arg[i] == 'd') printf(" f%u=%g", fi + 1, cs.flts[fi]), fi++;
        else if (sig.arg[i] == 'p' && cs.ints[ii]) printf(" r%u=%s", ii + 3, Where(cs.ints[ii], buf, sizeof(buf))), ii++;
        else printf(" r%u=0x%X", ii + 3, cs.ints[ii]), ii++;
    }
    printf("\n");
    std::map<u32, Read> seen;
    for (const Read& r : sReads) if (!seen.count(r.addr)) seen[r.addr] = r;
    u32 shown = 0;
    for (auto& kv : seen) {
        if (!kv.second.value || shown >= 24) continue;
        printf("DTM   in  %s (%u bytes) = 0x%X\n", Where(kv.first, buf, sizeof(buf)), kv.second.size, kv.second.value);
        shown++;
    }
    if (!retOk) {
        if (sig.ret == 'f' || sig.ret == 'd') printf("DTM   return: original %g, C++ %g\n", c.f[1].d, gotFlt);
        else printf("DTM   return: original 0x%X, C++ 0x%X\n", wantInt, gotInt);
    }
    for (size_t i = 0; i < diffs.size() && i < 12; i++) {
        const Diff& d = diffs[i];
        if (d.isFloat) { f32 x, y; memcpy(&x, &d.want, 4); memcpy(&y, &d.got, 4); printf("DTM   out %s: original %g, C++ %g\n", Where(d.addr, buf, sizeof(buf)), x, y); }
        else printf("DTM   out %s: original 0x%08X, C++ 0x%08X\n", Where(d.addr, buf, sizeof(buf)), d.want, d.got);
    }
    if (diffs.size() > 12) printf("DTM   ... and %u more words\n", (u32)diffs.size() - 12);
    if (*match) printf("DTM   both sides agree (return 0x%X)\n", wantInt);
    return TRUE;
}

}  // namespace

// Runs the jobs file and ends the process. One line per function on stdout:
//   DT <addr> <name> <MATCH|MISMATCH|UNSUPPORTED|UNTESTED|NOIMPL> ok=.. skipped=.. mismatched=.. | reason
void PortDiffTestRun(const char* jobsPath) {
    FILE* jobs = fopen(jobsPath, "r");
    if (!jobs) { fprintf(stderr, "[port] difftest: cannot open %s\n", jobsPath); ExitProcess(2); }
    sDolHi = PORT_TEXT_LO;
    sDol.assign(0x00800000, 0);
    u32 secAddr[18], secSize[18];
    if (!PortDolImage(sDol.data(), PORT_TEXT_LO, PORT_TEXT_LO + (u32)sDol.size(), secAddr, secSize)) { fprintf(stderr, "[port] difftest: main.dol cannot be read\n"); ExitProcess(2); }
    for (int i = 0; i < 7; i++) if (secSize[i] && secAddr[i] + secSize[i] > sDolHi) sDolHi = secAddr[i] + secSize[i];
    // The start-up code loads the two small-data bases: lis r2 / ori r2 / lis r13 / ori r13.
    for (u32 a = PORT_TEXT_LO; a + 16 <= PORT_TEXT_LO + 0x400; a += 4) {
        const u8* p = &sDol[a - PORT_TEXT_LO];
        u32 w0 = Be32(p), w1 = Be32(p + 4), w2 = Be32(p + 8), w3 = Be32(p + 12);
        if ((w0 >> 16) == 0x3C40 && (w1 >> 16) == 0x6042 && (w2 >> 16) == 0x3DA0 && (w3 >> 16) == 0x61AD) {
            sR2 = (w0 << 16) | (w1 & 0xFFFF);
            sR13 = (w2 << 16) | (w3 & 0xFFFF);
        }
    }
    if (!sR2) { fprintf(stderr, "[port] difftest: the small-data bases were not found in main.dol\n"); ExitProcess(2); }
    for (int i = 7; i < 18; i++) if (secSize[i] && secAddr[i] == sR2 - 0x8000) { sSda2Lo = secAddr[i]; sSda2Hi = secAddr[i] + secSize[i]; }
    sDataLo = PORT_TEXT_HI;
    sDataHi = PortArenaLoMEM1() - 0x10000;
    sPool = (u8*)VirtualAlloc(NULL, kPoolBufs * kBufSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    sStack = (u8*)VirtualAlloc(NULL, kStackSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    sSnap.assign((u8*)(uintptr_t)sDataLo, (u8*)(uintptr_t)sDataHi);

    u64 seed = 1;
    s32 only = -1;
    BOOL verbose = FALSE;
    char line[256];
    while (fgets(line, sizeof(line), jobs)) {
        u32 addr = 0, cases = 0;
        char sigText[64] = "";
        unsigned long long v = 0;
        if (sscanf(line, "seed %llu", &v) == 1) { seed = v; continue; }
        if (sscanf(line, "calls %llu", &v) == 1) { sCalls = v != 0; continue; }
        if (sscanf(line, "only %llu", &v) == 1) { only = (s32)v; continue; }
        if (sscanf(line, "verbose %llu", &v) == 1) { verbose = v != 0; continue; }
        if (sscanf(line, "job %x %63s %u", &addr, sigText, &cases) != 3) continue;
        const PortFunc* f = PortFindFunc(addr);
        Sig sig;
        printf("DTRUN %08X\n", addr);
        fflush(stdout);
        if (!f || !f->impl) { printf("DT %08X %s NOIMPL | no C++ implementation is linked\n", addr, f ? PortFuncName(f) : "?"); continue; }
        if (!ParseSig(sigText, &sig)) { printf("DT %08X %s UNSUPPORTED | signature %s\n", addr, PortFuncName(f), sigText); continue; }
        Result res;
        memset(&res, 0, sizeof(res));
        u64 fnSeed = seed ^ ((u64)addr << 20);
        for (u32 i = (only >= 0 ? (u32)only : 0); i < (only >= 0 ? (u32)only + 1 : cases); i++) {
            BOOL match = FALSE;
            sWhy[0] = 0;
            sUnsupported = FALSE;
            res.cases++;
            if (!RunCase(f, sig, i, fnSeed, verbose, &match)) {
                if (sUnsupported) { snprintf(res.unsupported, sizeof(res.unsupported), "%s", sWhy); break; }
                if (!res.skipped++) snprintf(res.firstSkip, sizeof(res.firstSkip), "%s", sWhy);
                if (verbose) printf("DTM %s case %u: not judged: %s\n", PortFuncName(f), i, sWhy);
                continue;
            }
            if (match) res.ok++;
            else if (++res.mismatched >= 3 && !verbose) break;   // the first ones are the simplest; three are enough to report
        }
        const char* status = res.mismatched ? "MISMATCH" : res.unsupported[0] ? "UNSUPPORTED" : res.ok ? "MATCH" : "UNTESTED";
        printf("DT %08X %s %s ok=%u skipped=%u mismatched=%u | %s\n", addr, PortFuncName(f), status, res.ok, res.skipped, res.mismatched,
               res.mismatched ? "" : res.unsupported[0] ? res.unsupported : res.firstSkip);
        fflush(stdout);
    }
    fclose(jobs);
    printf("DTEND %llu instructions\n", sInsns);
    fflush(stdout);
    TerminateProcess(GetCurrentProcess(), 0);
}
#endif
