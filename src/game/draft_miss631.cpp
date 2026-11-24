// Mixed small functions from 0x8002xxxx-0x8005xxxx (spawn-point picker, grid marker,
// HUD widget placement, floating numbers, camera quat blend, object death).
#include "types.h"
#include "engine/math.h"

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

extern "C" {
u32   RandomU32(void);                                              // 0x80143460
void  CameraSetMatrix(void* camera, const f32 mtx[3][4]);           // 0x801D6814
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void  EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);                    // 0x801BA2FC
s32   PlaySoundAt(void* sound, int a, const void* pos, int b);      // 0x80209ABC
void  ScopedLockCtor(ScopedLock* l);                                // 0x801F5384
void  ScopedLockDtor(ScopedLock* l, s32 flags);                     // 0x801F53B0
s32   SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner);  // 0x801C80E0
int   SysSprintf(char* buf, const char* fmt, ...);                  // 0x80142EF8
void  UIWidgetPlayAnimLoop(void* widget, u32 anim, s32 count);      // 0x8019BB1C
void  fn_80009840(u8* p);                                           // 0x80009840
void  fn_80029F30(u8* p, void* vec);                                // 0x80029F30
void  fn_8002C15C(u8* a, u8* b, u8* c, u8* d);                      // 0x8002C15C (signature from asm)
f32   fn_8002C428(u8* out, u8* in);                                 // 0x8002C428
s32   fn_80031114(void* list, f32* v);                              // 0x80031114
s32   fn_800D3C60(void* db, void* path, f32 a, f32 b);             // 0x800D3C60
void  fn_80121508(char* text, s32 x, s32 y, s32 a, s32 b, s32 c, f32 d, f32 e);  // 0x80121508 (signature from asm)
void  fn_80145F9C(f32* out, f32* a, f32* b, f32 t);                 // 0x80145F9C
void  fn_8016F854(f32* out, u8* view, const f32* pos);              // 0x8016F854
s32   fn_8019BC80(u8* arg0, u32 arg1);                              // 0x8019BC80
void  fn_801B1778(void* p, s32 a);                                  // 0x801B1778
void  fn_801F916C(f32* out, u8* p, s32 a);                          // 0x801F916C
f64   fn_8033F1F4(f64 x);                                           // 0x8033F1F4 (acos)
Vec3  QuatGetAxisY(const Quat* q);                                  // 0x801452A8

extern void* gObjDb;                // 0x805FB180
extern s32   gScreenHeight;         // 0x805F938C
extern s32   gScreenWidth;          // 0x805F9388
extern f32   lbl_80347684[];        // 0x80347684 (float table)
extern u8    lbl_803C2A20[];        // 0x803C2A20
extern u8    lbl_803C4298[];        // 0x803C4298
extern u8    lbl_8047C640[];        // 0x8047C640 (camera)
extern u8    lbl_8047C760[];        // 0x8047C760 (effect manager)
extern void* lbl_805FB5F8;          // 0x805FB5F8 (current camera)
extern const char lbl_805F853C[];   // 0x805F853C ("%d")

extern f32 lbl_805FCB04;            // 2.3283064e-10 (1 / 2^32)
extern f32 lbl_805FCB08;            // 10000.0
extern f32 lbl_805FCB18;            // 0.0
extern f32 lbl_805FCB1C;            // 3.1
extern f32 lbl_805FCB20;            // 20.0
extern f32 lbl_805FCEBC;            // 0.0
extern f32 lbl_805FCEC0;            // 1.0
extern f32 lbl_805FCEC8;            // 32.0
extern f32 lbl_805FCECC;            // 3.0
extern f32 lbl_805FCED0;            // 127.0
extern f32 lbl_805FCF00;            // 0.0
extern f32 lbl_805FCF74;            // 0.5
extern f32 lbl_805FCF78;            // -67.0
extern f32 lbl_805FCF7C;            // 50.0
extern f32 lbl_805FD118;            // 0.0
extern f32 lbl_805FD120;            // 3.0
extern f32 lbl_805FD124;            // 15.0
extern f32 lbl_805FD128;            // 40.0
extern f32 lbl_805FD300;            // 0.0
extern f32 lbl_805FD304;            // 1.0
extern f32 lbl_805FD308;            // -1.0
extern f32 lbl_805FD30C;            // 0.63661975
extern f32 lbl_805FD368;            // 0.5
extern f32 lbl_805FD370;            // 0.001
extern f32 lbl_805FD3C4;            // low angle threshold
extern f32 lbl_805FD3C8;            // high angle threshold
extern f32 lbl_805FD3CC;            // low angle threshold (flag)
extern f32 lbl_805FD3D0;            // high angle threshold (flag)
extern f32 lbl_805FD3D4;            // 1.25
}
extern "C++" void* gEventMgr;       // 0x805FB6C8

// sqrt that returns 0 for non-positive input (the original guards frsqrte the same way).
static inline f32 SqrtPos(f32 x) {
    return x > 0.0f ? sqrtf(x) : 0.0f;  // original: frsqrte with two Newton steps
}

extern "C" {

// 0x8002AC78
// Picks up to 10 random points in the rectangle (+0x1C..+0x24 / +0x20..+0x28): the first two on a
// random edge just outside it, the rest inside and at least 100 apart, each accepted by fn_80031114.
// Then builds the path data from them.
void fn_8002AC78(u8* arg0) {
    f32 sp20[3];
    *(s32*)(arg0 + 0xB4) = 0;
    if (*(s32*)(arg0 + 0x18) != 0) {
        f32 x = lbl_805FCB18;
        f32 y = lbl_805FCB18;
        s32 n = (s32)(lbl_805FCB18 + lbl_805FCB1C * (lbl_805FCB04 * (f32)RandomU32()));
        switch (n % 4) {
        case 0: {
            x = *(f32*)(arg0 + 0x1C) - lbl_805FCB20;
            f32 hi = *(f32*)(arg0 + 0x28);
            f32 lo = *(f32*)(arg0 + 0x20);
            y = lo + (hi - lo) * (lbl_805FCB04 * (f32)RandomU32());
            break;
        }
        case 1: {
            x = lbl_805FCB20 + *(f32*)(arg0 + 0x24);
            f32 hi = *(f32*)(arg0 + 0x28);
            f32 lo = *(f32*)(arg0 + 0x20);
            y = lo + (hi - lo) * (lbl_805FCB04 * (f32)RandomU32());
            break;
        }
        case 2: {
            f32 hi = *(f32*)(arg0 + 0x24);
            f32 lo = *(f32*)(arg0 + 0x1C);
            y = *(f32*)(arg0 + 0x20) - lbl_805FCB20;
            x = lo + (hi - lo) * (lbl_805FCB04 * (f32)RandomU32());
            break;
        }
        case 3: {
            f32 hi = *(f32*)(arg0 + 0x24);
            f32 lo = *(f32*)(arg0 + 0x1C);
            y = lbl_805FCB20 + *(f32*)(arg0 + 0x20);
            x = lo + (hi - lo) * (lbl_805FCB04 * (f32)RandomU32());
            break;
        }
        }
        // Same point stored twice (z from the object at +0x104, +0x70).
        u8* p = arg0 + *(s32*)(arg0 + 0xB4) * 0xC;
        f32 z = *(f32*)(*(u8**)(arg0 + 0x104) + 0x70);
        *(f32*)(p + 0x3C) = x;
        *(f32*)(p + 0x40) = y;
        *(f32*)(p + 0x44) = z;
        s32 n2 = *(s32*)(arg0 + 0xB4) + 1;
        *(s32*)(arg0 + 0xB4) = n2;
        z = *(f32*)(*(u8**)(arg0 + 0x104) + 0x70);
        p = arg0 + n2 * 0xC;
        *(f32*)(p + 0x3C) = x;
        *(f32*)(p + 0x40) = y;
        *(f32*)(p + 0x44) = z;
        *(s32*)(arg0 + 0xB4) = *(s32*)(arg0 + 0xB4) + 1;
    }
    while (*(s32*)(arg0 + 0xB4) < 10) {
        s32 cnt = *(s32*)(arg0 + 0xB4);
        u8* prev = NULL;
        if (cnt >= 1) prev = arg0 + (cnt - 1) * 0xC + 0x3C;
        f32* cur = (f32*)(arg0 + cnt * 0xC + 0x3C);
        for (;;) {
            f32 z = *(f32*)(*(u8**)(arg0 + 0x104) + 0x70);
            f32 xlo = *(f32*)(arg0 + 0x20);
            f32 xhi = *(f32*)(arg0 + 0x28);
            f32 px = xlo + (xhi - xlo) * (lbl_805FCB04 * (f32)RandomU32());
            f32 ylo = *(f32*)(arg0 + 0x1C);
            f32 yhi = *(f32*)(arg0 + 0x24);
            f32 py = ylo + (yhi - ylo) * (lbl_805FCB04 * (f32)RandomU32());
            cur[0] = py;
            cur[1] = px;
            cur[2] = z;
            if (prev != NULL) {
                f32 dx = *(f32*)(prev + 0x0) - cur[0];
                f32 dy = *(f32*)(prev + 0x4) - cur[1];
                f32 dz = *(f32*)(prev + 0x8) - cur[2];
                if (dz * dz + (dx * dx + dy * dy) < lbl_805FCB08) continue;  // too close to the previous point
            }
            if (fn_80031114(*(void**)(arg0 + 0x124), cur) != 0) break;
        }
        *(s32*)(arg0 + 0xB4) = *(s32*)(arg0 + 0xB4) + 1;
    }
    fn_8002C15C(arg0 + 0xB8, arg0 + 0x3C, arg0 + 0x48, arg0 + 0x54);
    *(s32*)(arg0 + 0xB4) = 2;
    fn_8002C428((u8*)sp20, arg0 + 0xB8);
    fn_80029F30(arg0, sp20);
}

// 0x80036B90
// Marks the bitmap cells within a radius (table lbl_80347684[+0x820], clamped 0..32) of the grid
// point arg1: 4 blocks of 128 rows x 32 columns (bits), counting each newly set bit in the float at +0x81C.
void fn_80036B90(u8* arg0, f32* arg1) {
    s32 x = (s32)arg1[0];
    s32 y = (s32)arg1[1];
    f32 t = lbl_80347684[*(s32*)(arg0 + 0x820)];
    f32 rad = lbl_805FCEBC;
    if (t < lbl_805FCEBC) {
    } else {
        rad = lbl_805FCEC8;
        if (t > rad) {
        } else {
            rad = t;
        }
    }
    s32 row = (s32)((f32)y - rad);
    for (; (f32)row < (f32)y + rad; row++) {
        s32 col32 = x % 32;
        s32 blk = x / 32;
        // clamp the block index to 0..3
        f32 fb = (f32)blk;
        f32 cb;
        if (fb < lbl_805FCEBC) cb = lbl_805FCEBC;
        else if (fb > lbl_805FCECC) cb = lbl_805FCECC;
        else cb = (f32)blk;
        s32 b = (s32)cb;
        // clamp the row to 0..127
        f32 fr = (f32)row;
        f32 cr;
        if (fr < lbl_805FCEBC) cr = lbl_805FCEBC;
        else if (fr > lbl_805FCED0) cr = lbl_805FCED0;
        else cr = (f32)row;
        s32 r4 = (s32)cr * 4;
        for (s32 c = (s32)((f32)col32 - rad); (f32)c <= (f32)col32 + rad; c++) {
            u32* word = (u32*)(arg0 + (b << 9) + r4 + 4);
            s32 bit = c;
            if (c < 0) {
                // spill into the previous block
                s32 pb = b - 1;
                f32 fp = (f32)pb;
                f32 cp;
                if (fp > lbl_805FCEBC) cp = (f32)pb;
                else cp = lbl_805FCEBC;
                word = (u32*)(arg0 + ((s32)cp << 9) + r4 + 4);
                if (b - 1 < 0) bit = 0;
            }
            if (bit > 0x1F) {
                // spill into the next block
                s32 nb = b + 1;
                f32 fn = (f32)nb;
                f32 cn;
                if (fn < lbl_805FCECC) cn = (f32)nb;
                else cn = lbl_805FCECC;
                word = (u32*)(arg0 + ((s32)cn << 9) + r4 + 4);
                if (b + 1 > 3) bit = 0x1F;
            }
            u32 mask = 0x80000000u >> ((bit + 0x20) % 32);
            if ((mask & *word) == 0) {
                *(f32*)(arg0 + 0x81C) = *(f32*)(arg0 + 0x81C) + lbl_805FCEC0;
            }
            *word |= mask;
        }
    }
}

// 0x80039268
// Places the HUD widget (at +0x90) on screen from the projected position (scaled by +0x154/+0x158),
// then sends it virtual message 0x18. When the position at +0x160 is behind (z < 0), plays a loop anim.
// The original also writes the pos into stack temporaries that the call does not take (dead stores).
void fn_80039268(u8* arg0, f32 farg0) {
    if (*(s32*)(arg0 + 0x5E4) > 0) {
        s32 behind = 0;
        if (*(u8*)(arg0 + 0x1F0) != 0) {
            f32 v = *(f32*)(arg0 + 0x1F4);
            if (v > lbl_805FCF00) *(f32*)(arg0 + 0x168) = v;
        }
        if (*(u8*)(arg0 + 0x1F0) != 0) {
            f32 out[4];
            fn_801F916C(out, arg0 + 0x160, 0);
            if (out[1] < lbl_805FCF00) behind = 1;
        }
        if (behind != 0) {
            fn_8019BC80(*(u8**)(arg0 + 0x90), 1);
        }
        f32 sx = *(f32*)(arg0 + 0x154) * *(f32*)(arg0 + 0x134);
        f32 sy = *(f32*)(arg0 + 0x158) * *(f32*)(arg0 + 0x138);
        f32 hw = lbl_805FCF74 * (f32)gScreenWidth;
        f32 hh = lbl_805FCF74 * (f32)gScreenHeight;
        f32 cx = hw + hw * sx;
        f32 cy = hh + hh * sy;
        u8* wd = *(u8**)(arg0 + 0x90);
        *(f32*)(wd + 0x68) = cx + lbl_805FCF78;
        *(f32*)(wd + 0x6C) = cy + lbl_805FCF7C;
        wd = *(u8**)(arg0 + 0x90);
        typedef s32 (*WidgetFn)(void*, s32);
        (*(WidgetFn*)(*(u8**)(wd + 0x138) + 0x2C))(wd, 0x18);  // other args (f1..) are leftovers
        if (behind != 0) {
            UIWidgetPlayAnimLoop(*(u8**)(arg0 + 0x90), 0x0AA2B0FDu, 10);
        }
    }
}

// 0x800410AC
// Draws the floating score numbers: 2 groups x 5 slots, each with a timer (+0x4AC) and a value
// (+0x484); shown at the projected screen position, rising as the timer (3..0) runs down.
void fn_800410AC(u8* arg0, f32 farg0) {
    u8 savedFlag = lbl_803C2A20[0x820];
    lbl_803C2A20[0x820] = 1;
    for (s32 i = 0; i < 2; i++) {
        for (s32 j = 0; j < 5; j++) {
            f32 timer = *(f32*)(arg0 + i * 0x14 + j * 4 + 0x4AC);
            if (timer > lbl_805FD118) {
                f32 rise = lbl_805FD120 - timer;
                f32 mtx[12];
                u8* obj = *(u8**)(lbl_803C4298 + 4);
                typedef void* (*MtxFn)(void*, f32*);
                (*(MtxFn*)(*(u8**)obj + 0x38))(obj, mtx);
                CameraSetMatrix(lbl_8047C640, (const f32(*)[4])mtx);
                fn_80009840(lbl_803C4298);
                f32 pos[3];                         // fn_8016F854 writes three ratios
                fn_8016F854(pos, (u8*)lbl_805FB5F8, (const f32*)(arg0 + i * 0x3C + j * 0xC + 0x40C));
                s32 sx = (s32)((f32)gScreenWidth * pos[0]);
                s32 sy = (s32)((f32)gScreenHeight * pos[1]) - (s32)(lbl_805FD124 * rise);
                char buf[64];
                SysSprintf(buf, lbl_805F853C, *(s32*)(arg0 + i * 0x14 + j * 4 + 0x484));
                fn_80121508(buf, sx, sy, -1, 1, 0, lbl_805FD128, lbl_805FD118);
                *(f32*)(arg0 + i * 0x14 + j * 4 + 0x4AC) = *(f32*)(arg0 + i * 0x14 + j * 4 + 0x4AC) - farg0;
            }
        }
    }
    lbl_803C2A20[0x820] = savedFlag;
}

// 0x8005B0B8
// Turns the object's orientation (quat at obj+0x4 -> +0xC) toward the target quat at +0x738.
// Snaps when the angle is tiny or near pi; otherwise slerps by dt (slowed for big up-vector turns).
void fn_8005B0B8(u8* arg0, f32 farg0) {
    f32 dt = farg0;
    u8* q = *(u8**)(arg0 + 0x4);
    f32 dot = *(f32*)(q + 0x18) * *(f32*)(arg0 + 0x744) +
              (*(f32*)(q + 0x14) * *(f32*)(arg0 + 0x740) +
               (*(f32*)(q + 0xC) * *(f32*)(arg0 + 0x738) + *(f32*)(q + 0x10) * *(f32*)(arg0 + 0x73C)));
    if (dot > lbl_805FD304) dot = lbl_805FD304;
    f32 ang = (f32)fn_8033F1F4((f64)dot);
    if (ang <= lbl_805FD3C4 || ang >= lbl_805FD3C8) {
        // already aligned (or exactly opposite): copy the target
        q = *(u8**)(arg0 + 0x4);
        *(f32*)(q + 0xC) = *(f32*)(arg0 + 0x738);
        *(f32*)(q + 0x10) = *(f32*)(arg0 + 0x73C);
        *(f32*)(q + 0x14) = *(f32*)(arg0 + 0x740);
        *(f32*)(q + 0x18) = *(f32*)(arg0 + 0x744);
        if (ang > lbl_805FD3CC && ang < lbl_805FD3D0) {
            *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x8000;
            *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
            *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x4000;
            *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
        }
        *(u32*)(arg0 + 0x838) &= 0xEFFFFFFFu;
        return;
    }
    q = *(u8**)(arg0 + 0x4);
    f32 dx = *(f32*)(q + 0xC) - *(f32*)(arg0 + 0x738);
    f32 dy = *(f32*)(q + 0x10) - *(f32*)(arg0 + 0x73C);
    f32 dz = *(f32*)(q + 0x14) - *(f32*)(arg0 + 0x740);
    f32 dw = *(f32*)(q + 0x18) - *(f32*)(arg0 + 0x744);
    if (dw * dw + (dz * dz + (dx * dx + dy * dy)) > lbl_805FD370) {
        // orientation still changing: flag the object dirty
        *(u32*)(q + 0x20) |= 0x8000;
        *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
        *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x4000;
        *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x10000000;
    }
    if (*(u32*)(arg0 + 0x838) & 0x40000) {
        Vec3 a = QuatGetAxisY((const Quat*)(*(u8**)(arg0 + 0x4) + 0xC));
        Vec3 b = QuatGetAxisY((const Quat*)(arg0 + 0x738));
        f32 d = a.z * b.z + (a.x * b.x + a.y * b.y);
        f32 len1 = SqrtPos(a.z * a.z + (a.x * a.x + a.y * a.y));
        f32 len2 = SqrtPos(b.z * b.z + (b.x * b.x + b.y * b.y));
        f32 den = len1 * len2;
        f32 turn = lbl_805FD300;
        if (!(den <= lbl_805FD300)) {
            f32 c = d / den;
            if (c < lbl_805FD308) c = lbl_805FD308;
            else if (c > lbl_805FD304) c = lbl_805FD304;
            turn = lbl_805FD30C * (f32)fn_8033F1F4((f64)c);
        }
        if (turn > lbl_805FD368) {
            dt = dt / (lbl_805FD3D4 * turn);
        }
    }
    f32 out[4];
    fn_80145F9C(out, (f32*)(*(u8**)(arg0 + 0x4) + 0xC), (f32*)(arg0 + 0x738), dt);
    q = *(u8**)(arg0 + 0x4);
    *(f32*)(q + 0xC) = out[0];
    *(f32*)(q + 0x10) = out[1];
    *(f32*)(q + 0x14) = out[2];
    *(f32*)(q + 0x18) = out[3];
}

// 0x8005C480
// Object death/despawn: frees the linked db entry (+0xA00), releases +0x8C, sends the death event,
// notifies the owner (+0xF8) once, spawns the death effect, plays the death sound, clears flag bits,
// and runs the virtual slot 6 (cleanup) under a scoped lock unless flag 0x04000000 is set.
void fn_8005C480(u8* arg0) {
    if ((*(u32*)(*(u8**)(arg0 + 0x4) + 0x20) & 3) == 0) return;
    if (gObjDb != NULL) {
        void* entry = *(void**)(arg0 + 0xA00);
        if (entry != NULL) {
            fn_800D3C60(gObjDb, entry, 0.0f, 0.0f);  // f1/f2 are leftovers in the asm: pass 0
            *(void**)(arg0 + 0xA00) = NULL;
        }
    }
    void* rel = *(void**)(arg0 + 0x8C);
    if (rel != NULL) {
        fn_801B1778(rel, 0);
    }
    EventSend(gEventMgr, arg0, arg0, 0x233E0AD4u, 0, 0, 1, 0);
    if (!(*(u32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x40000)) {
        u32 owner = *(u32*)(arg0 + 0xF8);
        if (owner != 0) {
            EventSendTo(gEventMgr, owner, arg0, 0x73B5A74Eu, 0, 0);
        }
        *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) |= 0x40000;
    }
    u8** handle = NULL;
    u32 effect = *(u32*)(*(u8**)(arg0 + 0x9C) + 0x1E8);
    if (effect != 0) {
        f32 pos[4];
        pos[0] = *(f32*)(arg0 + 0x18);
        pos[1] = *(f32*)(arg0 + 0x1C);
        pos[2] = *(f32*)(arg0 + 0x20);
        pos[3] = lbl_805FD300;
        SpawnEffectAt(lbl_8047C760, effect, pos, (u8**)&handle);
    }
    if (handle != NULL) {
        u8* target = *handle;
        if (target != NULL) {
            *(s32*)target = 0;
            *handle = NULL;
        }
    }
    void* snd = *(void**)(*(u8**)(arg0 + 0x9C) + 0x1F8);
    if (snd != NULL) {
        PlaySoundAt(snd, 0, *(void**)(arg0 + 0x4), 0);
    }
    *(u32*)(*(u8**)(arg0 + 0x4) + 0x20) &= 0xFFDFFFFFu;
    if (!(*(u32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x04000000)) {
        ScopedLock lock;
        ScopedLockCtor(&lock);
        typedef void (*DeathFn)(void*, s32);
        (*(DeathFn*)(*(u8**)arg0 + 0x18))(arg0, 2);
        ScopedLockDtor(&lock, -1);
    }
    *(f32*)(*(u8**)(arg0 + 0x4) + 0x24) = lbl_805FD300;
}

}  // extern "C"
