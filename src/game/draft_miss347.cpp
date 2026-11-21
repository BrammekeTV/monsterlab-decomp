// Particle spawn step, sphere/ring vertex tables, light setup and a few small helpers (0x801C-0x801E).
#include "types.h"
#include "revolution.h"

extern "C" {
void  BufferFree(void* p);                                                  // 0x801D4980
void* MemAllocDefault(u32 size);                                            // 0x801D4650
u32   RandomU32(void);                                                      // 0x80143460
void  RendClear(int flags, int a, int b, f32 depth);                        // 0x801534F0
void  TextureRelease(void* tex);                                            // 0x80184028
f32   cosf(f32 x);                                                          // 0x8033EC98 (MSL)
f32   sinf(f32 x);                                                          // 0x8033F0A4 (MSL)
s32   fn_80138C40(void* world, f32* a, f32* b, f32* hit, void* p, s32 c, s32 d);  // 0x80138C40
void  fn_801514D4(void);                                                    // 0x801514D4
void  fn_801514F0(void);                                                    // 0x801514F0
void  fn_8015150C(const f32* v, f32 a, f32 b);                              // 0x8015150C
void  fn_80154294(void);                                                    // 0x80154294
void  fn_801542DC(void);                                                    // 0x801542DC
void  fn_80155724(u8* m);                                                   // 0x80155724
void  fn_8016E304(void* cam);                                               // 0x8016E304
void  fn_801719DC(void* p);                                                 // 0x801719DC
void  fn_80171ABC(void);                                                    // 0x80171ABC
s32   fn_80173400(void* a, s32 b);                                          // 0x80173400
void  fn_8017514C(s32 a);                                                   // 0x8017514C
void  fn_80175C48(s32 a);                                                   // 0x80175C48
void  fn_801873C0(void* a, s32 b, s32 c);                                   // 0x801873C0
u8*   fn_801D66B4(void);                                                    // 0x801D66B4
void  fn_801D66C4(f32 a, f32 b);                                            // 0x801D66C4
u8*   fn_801D6D78(void);                                                    // 0x801D6D78
void  fn_801E361C(u8* p);                                                   // 0x801E361C
void  fn_801E3CFC(u8* p);                                                   // 0x801E3CFC
void  fn_801E3FA8(u8* p);                                                   // 0x801E3FA8
void  fn_801E40DC(void);                                                    // 0x801E40DC
void  fn_801EB748(void* p);                                                 // 0x801EB748
u8*   fn_801EB7A4(void* p, s32 a4, s32 a5, s32 a6, s32 a7);                 // 0x801EB7A4
void  fn_801EBA20(void* list, void* item);                                  // 0x801EBA20

extern u8   gWorldState[];
extern u8   lbl_80348F38[];         // 0x80348F38 (shader/object dispatch table)
extern f32  lbl_804010D8[12];       // 0x804010D8 (3x4 matrix)
extern u8   lbl_804129E8[];         // 0x804129E8
extern u8   lbl_804785B8[];         // 0x804785B8 (0x30-byte matrices)
extern f32  lbl_8047B5B8[];         // 0x8047B5B8 (4-float records)
extern u8   lbl_8047D378[];         // 0x8047D378 (0x120-byte debug draw vertex buffer)
extern u8   lbl_8047D498[];         // 0x8047D498 (34 x 0x10-byte colour/vertex records)
extern u8   lbl_805F95D0;           // 0x805F95D0
extern u8   lbl_805FB50C;           // 0x805FB50C (GX shadow flags byte)
extern u8   lbl_805FB50D;           // 0x805FB50D (GX shadow mask byte)
extern void* lbl_805FB5F8;          // 0x805FB5F8 (current camera)
extern u8*  lbl_805FB610;           // 0x805FB610
extern u8*  lbl_805FB75C;           // 0x805FB75C (main view)
extern s32  lbl_805FB778;           // 0x805FB778
extern u8*  lbl_805FB800;           // 0x805FB800 (pointer to saved aspect pair)
extern u8   lbl_805FB858;           // 0x805FB858
extern s32* lbl_805FB8A0;           // 0x805FB8A0 (pointer to an s32 passed as the 4th arg of fn_801EB7A4)
}

typedef void (*DispatchFn)(void* obj, f32* buf, u8* table);

// Writes an XF register block header (opcode 0x10): count-1, register.
static inline void XfHead(u16 countMinus1, u16 reg) {
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = countMinus1;
    GXWGFifo._u16 = reg;
}

// Clamps an int to 0..255.
static inline s32 ClampByte(s32 i) {
    if (i < 0) return 0;
    if (i > 0xff) return 0xff;
    return i;
}

// Clamps v between lo and hi; the bounds may be given in either order.
static inline f32 ClampPair(f32 v, f32 lo, f32 hi) {
    if (lo < hi) {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }
    if (v < hi) return hi;
    if (v > lo) return lo;
    return v;
}

extern "C" {

// 0x801C9C0C
// Particle step: starts the fade (flag 0x10) once the age passes (1 - fadeStart) * lifetime, then
// advances the stored value with a randomised rate and updates the reciprocal/derived fields.
void fn_801C9C0C(u8* arg0, f32 dt) {
    u32 flags = *(u32*)(arg0 + 0x0);
    if ((flags & 1) && !((flags >> 4) & 1)) {
        u8* cfg = *(u8**)(arg0 + 0x5C);
        f32 one = 1.0f;
        if (*(f32*)(arg0 + 0x8) < (one - *(f32*)(cfg + 0x13C)) * *(f32*)(cfg + 0x184)) {
            *(u32*)(arg0 + 0x0) = flags | 0x10;
            *(f32*)(arg0 + 0x58) = (*(f32*)(cfg + 0x148) - *(f32*)(cfg + 0x144)) *
                                   (one / (*(f32*)(cfg + 0x184) * (one - *(f32*)(cfg + 0x13C))));
        }
    }
    f32 f2 = *(f32*)(arg0 + 0x4);
    f32 f1 = -f2;
    f32 f3 = *(f32*)(arg0 + 0xC);
    f32 range = *(f32*)(*(u8**)(arg0 + 0x5C) + 0x174);
    f32 base = (f3 - (f1 - (f3 * (f1 / f3)))) - f2;
    f32 r = 2.3283064e-10f * (f32)RandomU32();
    f32 neg = -range;
    f32 cur = *(f32*)(arg0 + 0x50) + dt * *(f32*)(arg0 + 0x58);
    f32 prev = *(f32*)(arg0 + 0x4);
    *(f32*)(arg0 + 0x50) = cur;
    f32 val = (neg + (range - neg) * r) + cur;
    f32 inv = 1.0f / val;
    *(f32*)(arg0 + 0x54) = val;
    f32 nprev = -prev;
    *(f32*)(arg0 + 0xC) = inv;
    *(f32*)(arg0 + 0x4) = (inv - (nprev - (inv * (nprev / inv)))) - base;
}

// 0x801CAA10
// Draws the debug records of the objects in arg0 in three passes: for each pass with something to
// draw, every object whose pass byte (+0xD) matches runs its dispatch hooks around a draw of each
// of the arg1 positions.
void fn_801CAA10(u8* arg0, s32 arg1, u8* arg2) {
    f32 sp[29];
    if (lbl_805F95D0 == 0) return;
    for (s32 i = 0; i < 29; i++) ((s32*)sp)[i] = ((s32*)arg2)[i];
    s32 count = *(s32*)(arg0 + 0x6C);
    for (s32 pass = 1; pass <= 3; pass++) {
        if (fn_80173400(arg0, pass) == 0) continue;
        s32 pass2 = pass;
        if (pass == 1) pass2 = 3;
        fn_8017514C(pass2);
        u8* node = arg0;
        for (s32 j = 0; j < count; j++, node += 8) {
            u8* obj = *(u8**)(node + 0x70);
            if (pass != *(u8*)(obj + 0xD)) continue;
            f32* rec = lbl_8047B5B8;
            u8* mtx = lbl_804785B8;
            for (s32 k = 0; k < arg1; k++, rec += 4, mtx += 0x30) {
                f32 a = rec[0], b = rec[1], c = rec[2], d = rec[3];
                s32 counter = *(s32*)(lbl_804129E8 + 0x34);
                sp[0] = a; sp[1] = b; sp[2] = c; sp[3] = d;
                sp[4] = a; sp[5] = b; sp[6] = c; sp[7] = d;
                lbl_805FB610 = obj;
                *(s32*)(lbl_804129E8 + 0x34) = counter + 1;
                u8* e = lbl_80348F38 + *(u8*)(obj + 0xC) * 4;
                (*(DispatchFn*)(e + 0x48))(*(void**)(obj + 0x8), sp, e);
                lbl_805FB610 = obj;
                void* drawObj = *(void**)(node + 0x74);
                fn_80155724(mtx);
                fn_801873C0(drawObj, 0, 1);
                lbl_805FB610 = NULL;
                e = lbl_80348F38 + *(u8*)(obj + 0xC) * 4;
                (*(DispatchFn*)(e + 0x54))(*(void**)(obj + 0x8), sp, e);
            }
        }
        fn_80175C48(pass2);
    }
}

// 0x801CE54C
// Clamps the four floats at arg1 into the range [*arg2, *arg3] (either order) and stores them at arg0.
void fn_801CE54C(f32* arg0, f32* arg1, f32* arg2, f32* arg3) {
    arg0[0] = ClampPair(arg1[0], *arg2, *arg3);
    arg0[1] = ClampPair(arg1[1], *arg2, *arg3);
    arg0[2] = ClampPair(arg1[2], *arg2, *arg3);
    arg0[3] = ClampPair(arg1[3], *arg2, *arg3);
}

// 0x801D77C0
// Fills the 0x120-byte debug vertex buffer with a quarter-circle arc (16 steps of sin/cos, w = 1),
// then duplicates the first arc point at +0x110.
void fn_801D77C0(void) {
    u8* p = lbl_8047D378 + 0x10;
    *(f32*)(lbl_8047D378 + 0x4) = 0.0f;
    *(f32*)(lbl_8047D378 + 0x8) = 0.0f;
    *(f32*)(lbl_8047D378 + 0xC) = 0.0f;
    for (s32 i = 1; i <= 0x10; i++) {
        f32 ang = 1.5707964f * (0.25f * (f32)i);
        f32 c = cosf(ang);
        f32 s = sinf(ang);
        *(f32*)(p + 0x4) = s;
        *(f32*)(p + 0x8) = c;
        *(f32*)(p + 0xC) = 1.0f;
        p += 0x10;
    }
    u8* b = lbl_8047D378;
    s32 w0 = *(s32*)(b + 0x10);
    f32 f1 = *(f32*)(b + 0x14);
    f32 f2 = *(f32*)(b + 0x18);
    f32 f3 = *(f32*)(b + 0x1C);
    *(s32*)(b + 0x110) = w0;
    *(f32*)(b + 0x114) = f1;
    *(f32*)(b + 0x118) = f2;
    *(f32*)(b + 0x11C) = f3;
}

// 0x801D7F18
// Applies a fog/clear colour record: packs the four colour floats (+0x50..+0x5C) to bytes into
// lbl_805FB778, clamps the distance at +0x68 to the view limit, updates the main view and sets the fog.
void fn_801D7F18(u8* arg0) {
    f32 k = 255.0f;
    s32 r = (s32)(k * *(f32*)(arg0 + 0x50));
    s32 g = (s32)(k * *(f32*)(arg0 + 0x54));
    s32 b = (s32)(k * *(f32*)(arg0 + 0x58));
    s32 a = (s32)(k * *(f32*)(arg0 + 0x5C));
    r = ClampByte(r);
    g = ClampByte(g);
    b = ClampByte(b);
    a = ClampByte(a);
    lbl_805FB778 = r | ((g << 8) | ((a << 24) | (b << 16)));
    f32 d = *(f32*)(arg0 + 0x68);
    f32 limit = *(f32*)(lbl_805FB800 + 0x10);
    if (d > limit) d = limit;
    *(f32*)(lbl_805FB800 + 0xC) = d;
    *(f32*)(lbl_805FB75C + 0x198) = d;
    fn_8016E304(lbl_805FB75C);
    f32 col[4];
    col[0] = *(f32*)(arg0 + 0x50);
    col[1] = *(f32*)(arg0 + 0x54);
    col[2] = *(f32*)(arg0 + 0x58);
    col[3] = *(f32*)(arg0 + 0x5C);
    fn_8015150C(col, *(f32*)(arg0 + 0x60), *(f32*)(arg0 + 0x64));
    fn_801514D4();
}

// 0x801D8090
// Fills the 34-record ring vertex table with sin/cos pairs (even steps give w = 0, odd w = 1) and
// duplicates the first two records at +0x200.
void fn_801D8090(void) {
    u8* p = lbl_8047D498;
    for (s32 i = 0; i < 0x22; i += 2) {
        f32 ang = 1.5707964f * (0.25f * (f32)i);
        f32 c = cosf(ang);
        f32 s = sinf(ang);
        *(f32*)(p + 0x4) = s;
        *(f32*)(p + 0x8) = c;
        *(f32*)(p + 0xC) = 0.0f;
        *(f32*)(p + 0x14) = s;
        *(f32*)(p + 0x18) = c;
        *(f32*)(p + 0x1C) = 1.0f;
        p += 0x20;
    }
    u8* b = lbl_8047D498;
    s32 w0 = *(s32*)(b + 0x0);
    f32 f5 = *(f32*)(b + 0x4);
    f32 f4 = *(f32*)(b + 0x8);
    f32 f3 = *(f32*)(b + 0xC);
    s32 w1 = *(s32*)(b + 0x10);
    f32 f2 = *(f32*)(b + 0x14);
    f32 f1 = *(f32*)(b + 0x18);
    f32 f0 = *(f32*)(b + 0x1C);
    *(s32*)(b + 0x200) = w0;
    *(f32*)(b + 0x204) = f5;
    *(f32*)(b + 0x208) = f4;
    *(f32*)(b + 0x20C) = f3;
    *(s32*)(b + 0x210) = w1;
    *(f32*)(b + 0x214) = f2;
    *(f32*)(b + 0x218) = f1;
    *(f32*)(b + 0x21C) = f0;
}

// 0x801DAAFC
// Sets up the four hardware lights with zero/default parameters (position and direction run
// through the 3x4 matrix, colour, attenuation), then draws the preview object: clears, resets the
// camera record of arg0's object and renders it. (Dead stack stores of the original omitted.)
void fn_801DAAFC(u8* arg0) {
    f32 z = 0.0f;
    f32 one = 1.0f;
    f32 nz = -0.0f;
    f32 k = 255.0f * z;
    f32* m = lbl_804010D8;
    for (s32 i = 0; i < 4; i++) {
        s32 base = i * 0x10;
        f32 px = m[9] + ((m[6] * z) + ((m[0] * z) + (m[3] * z)));
        XfHead(2, (u16)(base + 0x60A));
        GXWGFifo._f32 = px;
        f32 py = m[10] + ((m[7] * z) + ((m[1] * z) + (m[4] * z)));
        GXWGFifo._f32 = py;
        f32 pz = m[11] + ((m[8] * z) + ((m[2] * z) + (m[5] * z)));
        GXWGFifo._f32 = pz;
        f32 dx = (m[6] * nz) + ((m[0] * nz) + (m[3] * nz));
        XfHead(2, (u16)(base + 0x60D));
        f32 dy = (m[7] * nz) + ((m[1] * nz) + (m[4] * nz));
        GXWGFifo._f32 = dx;
        f32 dz = (m[8] * nz) + ((m[2] * nz) + (m[5] * nz));
        GXWGFifo._f32 = dy;
        GXWGFifo._f32 = dz;
        XfHead(2, (u16)(base + 0x604));
        GXWGFifo._f32 = (f32)(-4.0 * 0.0);
        GXWGFifo._f32 = 4.0f;
        GXWGFifo._f32 = 0.0f;
        s32 c = (s32)k;
        s32 c0 = ClampByte(c);
        s32 c1 = ClampByte(c);
        s32 c2 = ClampByte(c);
        s32 c3 = ClampByte(c);
        XfHead(0, (u16)(base + 0x603));
        GXWGFifo._u32 = (u32)(c3 | ((c2 << 8) | ((c0 << 24) | (c1 << 16))));
        XfHead(2, (u16)(base + 0x607));
        GXWGFifo._f32 = one;
        GXWGFifo._f32 = z;
        GXWGFifo._f32 = z;
        u8 mask = (u8)(lbl_805FB50D & ~(1 << i));
        lbl_805FB50D = mask;
        if (lbl_805FB50C & 1) {
            XfHead(0, 0x100E);
            GXWGFifo._u32 = (u32)((((u32)mask & 0xff) << 2) | 0x742);
        }
    }
    u8* obj = *(u8**)(arg0 + 0x48);
    if (obj == NULL || lbl_805FB858 != 0) {
        RendClear(5, 0, 0, 1.0f);
        return;
    }
    void* cam = lbl_805FB5F8;
    fn_80171ABC();
    if (cam == NULL) {
        RendClear(5, 0, 0, 1.0f);
        return;
    }
    RendClear(4, 0, 0, 1.0f);
    fn_801514F0();
    fn_801542DC();
    u8* c = fn_801D6D78();
    if (c == NULL) return;
    u8* t = c + 0x24;
    if (t == NULL) return;
    u8* mat = *(u8**)(*(u8**)(arg0 + 0x48) + 0x4);
    if (mat == NULL) return;
    *(f32*)(mat + 0x0) = 1.0f;
    *(f32*)(mat + 0x4) = 0.0f;
    *(f32*)(mat + 0x8) = 0.0f;
    *(f32*)(mat + 0xC) = 0.0f;
    *(f32*)(mat + 0x10) = 1.0f;
    *(f32*)(mat + 0x14) = 0.0f;
    *(f32*)(mat + 0x18) = 0.0f;
    *(f32*)(mat + 0x1C) = 0.0f;
    *(f32*)(mat + 0x20) = 1.0f;
    *(f32*)(mat + 0x24) = *(f32*)(c + 0x24);
    *(f32*)(mat + 0x28) = *(f32*)(t + 0x4);
    *(f32*)(mat + 0x2C) = *(f32*)(t + 0x8);
    f32 scale = *(f32*)(*(u8**)(arg0 + 0x48) + 0x20);
    fn_801D66C4(*(f32*)(fn_801D66B4() + 0x8), 3.0f * scale);
    fn_801719DC(*(void**)(arg0 + 0x48));
    fn_801514D4();
    fn_80154294();
}

// 0x801E27D0
// Rebuilds the list object at arg0+4: releases every node's texture reference (freeing at zero),
// removes the nodes, frees the list, then allocates and initialises a fresh one.
void fn_801E27D0(u8* arg0) {
    u8* list = *(u8**)(arg0 + 0x4);
    if (list != NULL) {
        u8* head = *(u8**)(list + 0x14);
        u8* node = head != NULL ? head + 8 : NULL;
        while (node != NULL) {
            u8* link = *(u8**)(node - 4);
            u8* next = link != NULL ? link + 8 : NULL;
            u8* tex = *(u8**)(node + 0x34);
            if (tex != NULL) {
                s32 n = *(s32*)(tex + 0x24) - 1;
                *(s32*)(tex + 0x24) = n;
                if (n <= 0 && n == 0) TextureRelease(tex);
            }
            fn_801EBA20(*(void**)(arg0 + 0x4), node);
            node = next;
        }
        fn_801EB748(*(void**)(arg0 + 0x4));
        BufferFree(*(void**)(arg0 + 0x4));
        *(void**)(arg0 + 0x4) = NULL;
    }
    if (*(void**)(arg0 + 0x4) == NULL) {
        u8* n = (u8*)MemAllocDefault(0x2C);
        *(u8**)(arg0 + 0x4) = n;
        if (n != NULL) {
            fn_801EB7A4(n, 0x20C, 0x10, *lbl_805FB8A0, 0);
        }
        fn_801E40DC();
    }
}

// 0x801E29D4
// Pending-move resolve for a body: with flag 0x40000 set, either copies the position to +0xC
// (when any of 0x700000 is set) or sweeps from the position along the +0x18 direction (scaled by 8)
// through the world; on a hit it backs off by +0x30 and stores the result at +0xC. Then refreshes
// the body (fn_801E361C/3CFC/3FA8) and clears the flag; on a miss clears the low 18 flag bits.
void fn_801E29D4(u8* arg0) {
    u8 sp[0xC0];
    s32 flags = *(s32*)(arg0 + 0x4C);
    if (!(flags & 0x40000)) return;
    if (flags & 0x700000) {
        f32 x = *(f32*)(arg0 + 0x0);
        f32 y = *(f32*)(arg0 + 0x4);
        f32 z = *(f32*)(arg0 + 0x8);
        *(f32*)(arg0 + 0xC) = x;
        *(f32*)(arg0 + 0x10) = y;
        *(f32*)(arg0 + 0x14) = z;
    } else {
        f32 eight = 8.0f;
        f32 zero = 0.0f;
        f32 dx = *(f32*)(arg0 + 0x18) * eight;
        f32 dy = *(f32*)(arg0 + 0x1C) * eight;
        f32 dz = *(f32*)(arg0 + 0x20) * eight;
        *(f32*)(sp + 0x38) = dx;
        *(f32*)(sp + 0x10) = zero;
        *(f32*)(sp + 0x44) = *(f32*)(arg0 + 0x0) + dx;
        *(f32*)(sp + 0x3C) = dy;
        *(f32*)(sp + 0x40) = dz;
        *(f32*)(sp + 0x48) = *(f32*)(arg0 + 0x4) + dy;
        *(f32*)(sp + 0xC) = zero;
        *(f32*)(sp + 0x8) = zero;
        *(f32*)(sp + 0x4C) = *(f32*)(arg0 + 0x8) + dz;
        *(f32*)(sp + 0x5C) = zero;
        *(f32*)(sp + 0x60) = zero;
        *(f32*)(sp + 0x64) = zero;
        *(f32*)(sp + 0x50) = zero;
        *(f32*)(sp + 0x54) = zero;
        *(f32*)(sp + 0x58) = zero;
        *(f32*)(sp + 0x68) = zero;
        *(s32*)(sp + 0xA8) = 0;
        *(s32*)(sp + 0xAC) = 0;
        *(s32*)(sp + 0xB0) = 0;
        *(s32*)(sp + 0xB4) = 0;
        *(s32*)(sp + 0xB8) = 0;
        *(s32*)(sp + 0xA0) = 0;
        *(s32*)(sp + 0xA4) = 4;
        *(f32*)(sp + 0x1C) = zero;
        *(f32*)(sp + 0x18) = zero;
        *(f32*)(sp + 0x14) = zero;
        *(f32*)(sp + 0x84) = zero;
        *(f32*)(sp + 0x88) = zero;
        *(f32*)(sp + 0x8C) = zero;
        *(f32*)(sp + 0x78) = zero;
        *(f32*)(sp + 0x7C) = zero;
        *(f32*)(sp + 0x80) = zero;
        *(f32*)(sp + 0x6C) = zero;
        *(f32*)(sp + 0x70) = zero;
        *(f32*)(sp + 0x74) = zero;
        if (fn_80138C40(gWorldState, (f32*)arg0, (f32*)(sp + 0x44), (f32*)(sp + 0x50), NULL, 0, 0) == 0) {
            *(s32*)(arg0 + 0x4C) = *(s32*)(arg0 + 0x4C) & 0xFFFC0000;
            return;
        }
        f32 back = *(f32*)(arg0 + 0x30);
        f32 bx = *(f32*)(arg0 + 0x18) * back;
        f32 by = *(f32*)(arg0 + 0x1C) * back;
        f32 bz = *(f32*)(arg0 + 0x20) * back;
        f32 hx = *(f32*)(sp + 0x50) - bx;
        f32 hy = *(f32*)(sp + 0x54) - by;
        f32 hz = *(f32*)(sp + 0x58) - bz;
        *(f32*)(sp + 0x20) = bx;
        *(f32*)(sp + 0x24) = by;
        *(f32*)(sp + 0x28) = bz;
        *(f32*)(sp + 0x2C) = hx;
        *(f32*)(sp + 0x30) = hy;
        *(f32*)(sp + 0x34) = hz;
        *(f32*)(arg0 + 0xC) = hx;
        *(f32*)(arg0 + 0x10) = hy;
        *(f32*)(arg0 + 0x14) = hz;
    }
    fn_801E361C(arg0);
    fn_801E3CFC(arg0);
    fn_801E3FA8(arg0);
    *(s32*)(arg0 + 0x4C) = *(s32*)(arg0 + 0x4C) & 0xFFFBFFFF;
}

}  // extern "C"
