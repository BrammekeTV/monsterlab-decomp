// Audio/world functions in 0x801F-0x8020: world data teardown, path-segment side test, DSP voice load/start, reverb preset.
#include <stdint.h>
#include "types.h"
#include "revolution.h"

struct WorldParams {
    s32   dynamicSlots;   // 0x00
    s32   staticSlots;    // 0x04
    void** out;           // 0x08: receives the world data
};
extern "C++" u8* sWorldParams;   // 0x805FB944 (defined in world_alloc.cpp)

extern "C" {
void   BufferFree(void* p);
s32    fn_80146808(u8* file, const char* name, const char* mode);
s32    fn_80146930(u8* file, void* buf, s32 size);
void   fn_801469FC(u8* file);
s32    fn_80146A58(u8* file);
u8*    fn_801483BC(s32 idx);
s32    fn_80203664(u8* audio, const char* name, s32 flag);    // 0x80203664
void   fn_80203340(u8* data, u8* voice, s32 flag);           // 0x80203340
void   fn_80202CA8(void);                                      // 0x80202CA8
s32    fn_80258024(s32 priority, void* cb, void* ctx);        // 0x80258024
void   fn_80258904(void* cb, void* p);                         // 0x80258904
void   fn_80258968(void* cb, void* p);                         // 0x80258968
void   fn_8025B65C(void* p);                                   // 0x8025B65C
void   fn_8025B72C(void* p);                                   // 0x8025B72C
void   fn_8025C52C(void* p);                                   // 0x8025C52C
void   fn_8025C5FC(void* p);                                   // 0x8025C5FC
void   fn_801FCDB4(void* a, u8* b);                                    // 0x801FCDB4
s32    fn_80263FEC(u8* p, s32 handle, s32 a);                  // 0x80263FEC
s32    fn_80264020(u8* p);                                     // 0x80264020
s32    fn_80264508(u8* p);                                     // 0x80264508
s32    fn_80264524(u8* p);                                     // 0x80264524
s32    fn_80264538(u8* p);                                     // 0x80264538
s32    fn_8026454C(u8* p);                                     // 0x8026454C
s32    fn_80264560(u8* p);                                     // 0x80264560
s32    fn_80264574(u8* p);                                     // 0x80264574
s32    fn_80264A14(u8* p, u8* q);                              // 0x80264A14
f64    fn_8033F204(f64 x);                                     // 0x8033F204 (natural log)
void*  memset(void* p, int c, unsigned long n);
char*  strcat(char* dst, const char* src);
u32    strlen(const char* s);
char*  strncpy(char* dst, const char* src, size_t n);

extern MEMAllocator lbl_803F2870;   // 0x803F2870
extern u8   lbl_804FDAA0[];         // 0x804FDAA0
extern u8   lbl_8051DAD0[];         // 0x8051DAD0 (audio state block)
extern u8   lbl_8051E6A8[];         // 0x8051E6A8 (sound voices, 0x114 each)
extern u8   lbl_805F98C0;           // 0x805F98C0
extern f32  lbl_805F98C4;           // 0x805F98C4
extern u32  lbl_805F98E0;           // 0x805F98E0
extern const char lbl_805F98E4[];   // 0x805F98E4 (".dsp")
extern const char lbl_805F98EC[];   // 0x805F98EC ("rb")
extern u8   lbl_805FBA9C;           // 0x805FBA9C (platform/mode, 2 = alternate)
extern s32  lbl_805FBAE0;           // 0x805FBAE0 (audio busy flag)
extern u8*  lbl_805FBB80[2];        // 0x805FBB80 (aligned DSP data per slot)
extern u8*  lbl_805FBB88[2];        // 0x805FBB88 (raw DSP allocation per slot)
extern s32  lbl_805FBB90;           // 0x805FBB90
extern f32  lbl_8060022C;           // 0x8060022C (0.0)
extern f32  lbl_80600224;           // 0x80600224 (-1.0)
extern f32  lbl_80600320;  // 0x80600320
extern f32  lbl_80600354;  // 0x80600354
extern f32  lbl_80600358;  // 0x80600358
extern f32  lbl_8060035C;  // 0x8060035C
extern f32  lbl_80600388;  // 0x80600388
extern f32  lbl_8060038C;  // 0x8060038C
extern f32  lbl_80600390;  // 0x80600390
extern f32  lbl_80600394;  // 0x80600394
extern f32  lbl_80600398;  // 0x80600398
extern f32  lbl_8060039C;  // 0x8060039C
extern f32  lbl_806003A0;  // 0x806003A0
extern f32  lbl_806003A4;  // 0x806003A4
extern f32  lbl_806003A8;  // 0x806003A8
extern f32  lbl_806003AC;  // 0x806003AC
extern f32  lbl_806003B0;  // 0x806003B0
extern f32  lbl_806003B4;  // 0x806003B4
extern f32  lbl_806003B8;  // 0x806003B8
extern f32  lbl_806003BC;  // 0x806003BC
extern f32  lbl_806003C0;  // 0x806003C0
extern f32  lbl_806003C4;  // 0x806003C4
extern f32  lbl_806003C8;  // 0x806003C8
extern f32  lbl_806003CC;  // 0x806003CC
extern f32  lbl_806003D0;  // 0x806003D0
}

// Sign of an edge (A -> B, both relative to the point) against the plane with normal (nx,ny,nz):
// -1 if A is in front and B behind (or on), 1 for the reverse, else 0.
static s32 PlaneSide(f32 nx, f32 ny, f32 nz, const f32* a, const f32* b) {
    f32 da = (nz * a[2]) + ((nx * a[0]) + (ny * a[1]));
    f32 db = (nz * b[2]) + ((nx * b[0]) + (ny * b[1]));
    if (da > 0.0f && db <= 0.0f) return -1;
    if (da < 0.0f && db >= 0.0f) return 1;
    return 0;
}

extern "C" {

// 0x801F1920
// Frees the world data: its three tables (when owned), the block itself, then the params struct.
void fn_801F1920(void) {
    u8* w = *(u8**)((WorldParams*)sWorldParams)->out;
    u8* b = *(u8**)(w + 0x2C);
    if (b != NULL) {
        s32 t = *(s32*)(w + 0x34);
        if (t == 2) BufferFree(b);
        else if (t == 1) BufferFree(b);
    }
    *(void**)(w + 0x2C) = NULL;
    *(s32*)(w + 0x30) = 0;
    *(s32*)(w + 0x34) = 0;

    w = *(u8**)((WorldParams*)sWorldParams)->out;
    b = *(u8**)(w + 0x38);
    if (b != NULL) {
        s32 t = *(s32*)(w + 0x40);
        if (t == 2) BufferFree(b);
        else if (t == 1) BufferFree(b);
    }
    *(void**)(w + 0x38) = NULL;
    *(s32*)(w + 0x3C) = 0;
    *(s32*)(w + 0x40) = 0;

    w = *(u8**)((WorldParams*)sWorldParams)->out;
    b = *(u8**)(w + 0x44);
    if (b != NULL) {
        s32 t = *(s32*)(w + 0x4C);
        if (t == 2) BufferFree(b);
        else if (t == 1) BufferFree(b);
    }
    *(void**)(w + 0x44) = NULL;
    *(s32*)(w + 0x48) = 0;
    *(s32*)(w + 0x4C) = 0;

    w = *(u8**)((WorldParams*)sWorldParams)->out;
    if (w != NULL) BufferFree(*(void**)(w - 4));
    BufferFree(sWorldParams);
    sWorldParams = NULL;
}

// 0x801F95F0
// Walks the recorded path points (newest first) and checks whether the segment to the next point
// crosses the three axis planes around the position at +0x9C; on a crossing the progress at +0xAC is
// clamped to >= 0. When progress reaches the limit at +0xA8 the done flag (+0x90) is set.
void fn_801F95F0(u8* p, s32 a4, f32 dt) {
    f32 prog = *(f32*)(p + 0xAC);
    if (prog >= lbl_8060022C) *(f32*)(p + 0xAC) = prog + dt;

    u8* path = fn_801483BC(*(s32*)(p + 0x4));
    s32 i = *(s32*)(path + 0x14) - 1;
    for (; i >= 0; i--) {
        s32 head = *(s32*)(path + 0x10);
        s32 cnt = *(s32*)(path + 0xC);
        s32 alt = *(s32*)(p + 0x98);
        u8* base = *(u8**)(path + 0x4);
        f32 a[3], b[3], limit;
        u8* e = base + ((head + i) % cnt) * 0x84;
        if (alt == 0) {
            b[0] = *(f32*)(e + 0xC);  b[1] = *(f32*)(e + 0x10); b[2] = *(f32*)(e + 0x14);
            limit = *(f32*)(e + 0x1C);
        } else {
            b[0] = *(f32*)(e + 0x68); b[1] = *(f32*)(e + 0x6C); b[2] = *(f32*)(e + 0x70);
            limit = *(f32*)(e + 0x78);
        }
        e = base + ((head + i + 1) % cnt) * 0x84;
        if (alt == 0) {
            a[0] = *(f32*)(e + 0xC);  a[1] = *(f32*)(e + 0x10); a[2] = *(f32*)(e + 0x14);
        } else {
            a[0] = *(f32*)(e + 0x68); a[1] = *(f32*)(e + 0x6C); a[2] = *(f32*)(e + 0x70);
        }
        f32 px = *(f32*)(p + 0x9C), py = *(f32*)(p + 0xA0), pz = *(f32*)(p + 0xA4);
        b[0] -= px; b[1] -= py; b[2] -= pz;
        a[0] -= px; a[1] -= py; a[2] -= pz;
        if (limit >= *(f32*)(p + 0xB0)) {
            if (PlaneSide(0.0f, -1.0f, 0.0f, a, b) == 0 &&
                PlaneSide(1.0f, 0.0f, 0.0f, a, b) == 0 &&
                PlaneSide(0.0f, 0.0f, -1.0f, a, b) == 0) {
                continue;
            }
            if (*(f32*)(p + 0xAC) < lbl_8060022C) *(f32*)(p + 0xAC) = lbl_8060022C;
            break;
        }
    }
    if (*(f32*)(p + 0xAC) >= *(f32*)(p + 0xA8)) {
        *(u8*)(p + 0x90) = 1;
        *(f32*)(p + 0xAC) = lbl_80600224;
    }
}

// 0x801FD2D4
// Loads "<name>.dsp" into an aligned buffer for DSP slot 'flag' (0/1). Returns 0 on success, -1 on failure.
s32 fn_801FD2D4(u8* audio, const char* name, s32 flag) {
    char buf[0x80];
    if (lbl_805F98C0 != 0) return fn_80203664(audio, name, flag);
    s32 room = 0x7F - (s32)strlen(lbl_805F98E4);
    strncpy(buf, name, room - 1);
    buf[room - 1] = 0;
    strcat(buf, lbl_805F98E4);
    u8* v = (flag != 0) ? audio + 0x20 : audio + 0x1C8;
    fn_80146808(v + 0x1C, buf, lbl_805F98EC);
    s32 size = fn_80146A58(v + 0x1C);
    *(s32*)(v + 0x7C) = size;
    if (size == 0) return -1;
    lbl_805FBB90 = size + 0x20;
    u8* mem = (u8*)MEMAllocFromAllocator(&lbl_803F2870, (u32)(size + 0x20));
    lbl_805FBB88[flag] = mem;
    if (mem == NULL) return -1;
    memset(mem, 0, lbl_805FBB90);
    lbl_805FBB80[flag] = (u8*)(((uintptr_t)lbl_805FBB88[flag] + 0x1F) & ~(uintptr_t)0x1F);
    s32 got = fn_80146930(v + 0x1C, lbl_805FBB80[flag], size);
    fn_801469FC(v + 0x1C);
    return (got != size) ? -1 : 0;
}

// 0x801FD420
// Starts DSP voice 'idx': acquires an AX voice, sets up its parameter blocks (volume from the
// level at lbl_805F98C4 in dB, 'pan' byte at +0x58) and marks the audio slot as playing.
// Returns 1 on success. (The original leaves a stale value in r3 on the early-out paths; 0 here.)
s32 fn_801FD420(u8* audio, s32 idx, s32 pan) {
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    if (lbl_805F98C0 != 0 && *(s32*)(lbl_8051DAD0 + 0xB4) != 0) {
        lbl_805FBAE0 = saved;
        return 0;
    }
    if (idx == -1) {
        lbl_805FBAE0 = saved;
        return 0;
    }
    u8* v = lbl_8051E6A8 + idx * 0x114;
    s32 silent = 0;
    *(u8*)(v + 0xFE) = 0;
    *(u8*)(v + 0x105) = 0;
    *(s32*)(v + 0x100) = 0x1F;
    s32 handle = fn_80258024(0x1F, (void*)fn_80202CA8, v);
    *(s32*)(v + 0x0) = handle;
    if (handle == 0) {
        lbl_805FBAE0 = saved;
        return 0;
    }
    *(s32*)(v + 0xC) = 0;
    fn_80263FEC(v + 0x10, *(s32*)(v + 0x0), *(s32*)(lbl_805FBB80[idx] + 0x8));
    fn_80264538(v + 0x68);
    fn_80264A14(v + 0x10, v + 0x68);
    f32 level = lbl_805F98C4;
    if (level > -0.0001f && level < 0.0001f) silent = 1;
    f32 db;
    if (silent != 0) {
        db = -1000.0f;
    } else {
        f32 ln2 = (f32)fn_8033F204(2.0);
        f32 ratio = (f32)fn_8033F204(10.0) / ln2;
        f32 ln2b = (f32)fn_8033F204(2.0);
        db = (30.0f * ((f32)fn_8033F204((f64)level) / ln2b)) / ratio;
    }
    s32 att = (s32)(10.0f * db);
    if (att < -0x8000) att = -0x8000;
    else if (att > 0x7FFF) att = 0x7FFF;
    *(s32*)(v + 0x70) = att << 16;
    fn_80264508(v + 0x50);
    fn_80264A14(v + 0x10, v + 0x50);
    fn_80264524(v + 0x5C);
    fn_80264A14(v + 0x10, v + 0x5C);
    *(s32*)(v + 0x64) = 0x10000;
    *(u8*)(v + 0x58) = (u8)pan;
    fn_8026454C(v + 0x88);
    fn_80264A14(v + 0x10, v + 0x88);
    *(s32*)(v + 0x90) = 0xFC400000;
    fn_80264560(v + 0x94);
    fn_80264A14(v + 0x10, v + 0x94);
    *(s32*)(v + 0x9C) = 0xFC400000;
    fn_80264574(v + 0xA0);
    fn_80264A14(v + 0x10, v + 0xA0);
    *(s32*)(v + 0xA8) = 0xFC400000;
    *(u8*)(v + 0xFE) = 1;
    *(u8*)(v + 0x105) = 0;
    fn_80264020(v + 0x10);
    if (lbl_805F98C0 != 0) {
        if (idx == 0) {
            *(s32*)(lbl_8051DAD0 + idx * 4 + 0x8C) = *(s32*)(v + 0x0);
            *(u8*)(audio + 0x28) = 1;
        } else if (idx == 1) {
            *(s32*)(lbl_8051DAD0 + idx * 4 + 0x8C) = *(s32*)(v + 0x0);
            *(u8*)(audio + 0x28) = 1;
        }
    } else {
        *(u16*)(lbl_805FBB80[idx] + 0xC) = (u16)lbl_805F98E0;
        fn_80203340(lbl_805FBB80[idx], *(u8**)(v + 0x0), lbl_805F98E0 != 0);
        *(u8*)(audio + 0x28) = 1;
        *(u8*)(audio + 0x29) = 1;
    }
    *(s32*)(v + 0x4) = 1;
    *(s32*)(v + 0x8) = 0;
    lbl_805FBAE0 = saved;
    return 1;
}

// 0x80200920
// Loads reverb preset 'a' (1..12; 0 = leave as is) into the effect block for slot 'idx' and then
// enables (a != 0, with callback fn_801FCDB4) or clears the effect.
void fn_80200920(void* p, s32 a, s32 idx) {
    u8* r;
    u8 mode = lbl_805FBA9C;
    u8* base = lbl_804FDAA0 + 0x20000;
    if (mode == 2) r = (idx == 0) ? base + 0x210 : base + 0x4A0;
    else           r = (idx == 0) ? base + 0x728 : base + 0x968;
    switch ((u32)a) {
    case 1:
        *(s32*)(r + 0x110) = 5;
        *(f32*)(r + 0x114) = lbl_80600388;
        *(f32*)(r + 0x118) = lbl_8060038C;
        *(s32*)(r + 0x11C) = 0;
        *(f32*)(r + 0x120) = lbl_8060035C;
        *(f32*)(r + 0x124) = lbl_80600320;
        *(f32*)(r + 0x128) = lbl_80600320;
        *(f32*)(r + 0x12C) = lbl_80600390;
        *(f32*)(r + 0x130) = lbl_80600354;
        *(f32*)(r + 0x134) = lbl_80600320;
        *(s32*)(r + 0x138) = 0;
        *(s32*)(r + 0x13C) = 0;
        *(f32*)(r + 0x140) = lbl_80600358;
        *(f32*)(r + 0x144) = lbl_80600354;
        break;
    case 2:
        *(s32*)(r + 0x110) = 7;
        *(f32*)(r + 0x114) = lbl_80600388;
        *(f32*)(r + 0x118) = lbl_80600388;
        *(s32*)(r + 0x11C) = 0;
        *(f32*)(r + 0x120) = lbl_80600394;
        *(f32*)(r + 0x124) = lbl_80600320;
        *(f32*)(r + 0x128) = lbl_80600398;
        *(f32*)(r + 0x12C) = lbl_80600390;
        *(f32*)(r + 0x130) = lbl_8060039C;
        *(f32*)(r + 0x134) = lbl_80600320;
        *(s32*)(r + 0x138) = 0;
        *(s32*)(r + 0x13C) = 0;
        *(f32*)(r + 0x140) = lbl_80600358;
        *(f32*)(r + 0x144) = lbl_80600354;
        break;
    case 3:
        *(s32*)(r + 0x110) = 0;
        *(f32*)(r + 0x114) = lbl_80600388;
        *(f32*)(r + 0x118) = lbl_80600354;
        *(s32*)(r + 0x11C) = 1;
        *(f32*)(r + 0x120) = lbl_80600320;
        *(f32*)(r + 0x124) = lbl_80600320;
        *(f32*)(r + 0x128) = lbl_806003A0;
        *(f32*)(r + 0x12C) = lbl_80600390;
        *(f32*)(r + 0x130) = lbl_806003A4;
        *(f32*)(r + 0x134) = lbl_806003A8;
        *(s32*)(r + 0x138) = 0;
        *(s32*)(r + 0x13C) = 0;
        *(f32*)(r + 0x140) = lbl_80600358;
        *(f32*)(r + 0x144) = lbl_80600354;
        break;
    case 4:
        *(s32*)(r + 0x110) = 6;
        *(f32*)(r + 0x114) = lbl_80600388;
        *(f32*)(r + 0x118) = lbl_8060038C;
        *(s32*)(r + 0x11C) = 1;
        *(f32*)(r + 0x120) = lbl_80600394;
        *(f32*)(r + 0x124) = lbl_80600320;
        *(f32*)(r + 0x128) = lbl_806003AC;
        *(f32*)(r + 0x12C) = lbl_80600390;
        *(f32*)(r + 0x130) = lbl_806003B0;
        *(f32*)(r + 0x134) = lbl_806003A8;
        *(s32*)(r + 0x138) = 0;
        *(s32*)(r + 0x13C) = 0;
        *(f32*)(r + 0x140) = lbl_80600358;
        *(f32*)(r + 0x144) = lbl_80600354;
        break;
    case 5:
        *(s32*)(r + 0x110) = 4;
        *(f32*)(r + 0x114) = lbl_80600388;
        *(f32*)(r + 0x118) = lbl_806003B4;
        *(s32*)(r + 0x11C) = 2;
        *(f32*)(r + 0x120) = lbl_80600320;
        *(f32*)(r + 0x124) = lbl_806003B8;
        *(f32*)(r + 0x128) = lbl_8060039C;
        *(f32*)(r + 0x12C) = lbl_80600390;
        *(f32*)(r + 0x130) = lbl_806003A8;
        *(f32*)(r + 0x134) = lbl_806003A8;
        *(s32*)(r + 0x138) = 0;
        *(s32*)(r + 0x13C) = 0;
        *(f32*)(r + 0x140) = lbl_80600358;
        *(f32*)(r + 0x144) = lbl_80600354;
        break;
    case 6:
        *(s32*)(r + 0x110) = 5;
        *(f32*)(r + 0x114) = lbl_80600388;
        *(f32*)(r + 0x118) = lbl_806003BC;
        *(s32*)(r + 0x11C) = 2;
        *(f32*)(r + 0x120) = lbl_80600358;
        *(f32*)(r + 0x124) = lbl_806003B8;
        *(f32*)(r + 0x128) = lbl_8060039C;
        *(f32*)(r + 0x12C) = lbl_80600390;
        *(f32*)(r + 0x130) = lbl_806003A8;
        *(f32*)(r + 0x134) = lbl_806003A8;
        *(s32*)(r + 0x138) = 0;
        *(s32*)(r + 0x13C) = 0;
        *(f32*)(r + 0x140) = lbl_80600358;
        *(f32*)(r + 0x144) = lbl_80600354;
        break;
    case 7:
        *(s32*)(r + 0x110) = 7;
        *(f32*)(r + 0x114) = lbl_80600388;
        *(f32*)(r + 0x118) = lbl_8060038C;
        *(s32*)(r + 0x11C) = 3;
        *(f32*)(r + 0x120) = lbl_8060035C;
        *(f32*)(r + 0x124) = lbl_806003B8;
        *(f32*)(r + 0x128) = lbl_8060039C;
        *(f32*)(r + 0x12C) = lbl_80600390;
        *(f32*)(r + 0x130) = lbl_806003B0;
        *(f32*)(r + 0x134) = lbl_806003C0;
        *(s32*)(r + 0x138) = 0;
        *(s32*)(r + 0x13C) = 0;
        *(f32*)(r + 0x140) = lbl_80600358;
        *(f32*)(r + 0x144) = lbl_80600354;
        break;
    case 8:
        *(s32*)(r + 0x110) = 3;
        *(f32*)(r + 0x114) = lbl_80600388;
        *(f32*)(r + 0x118) = lbl_806003BC;
        *(s32*)(r + 0x11C) = 3;
        *(f32*)(r + 0x120) = lbl_806003C4;
        *(f32*)(r + 0x124) = lbl_806003B8;
        *(f32*)(r + 0x128) = lbl_8060039C;
        *(f32*)(r + 0x12C) = lbl_80600390;
        *(f32*)(r + 0x130) = lbl_806003A8;
        *(f32*)(r + 0x134) = lbl_806003C8;
        *(s32*)(r + 0x138) = 0;
        *(s32*)(r + 0x13C) = 0;
        *(f32*)(r + 0x140) = lbl_80600358;
        *(f32*)(r + 0x144) = lbl_80600354;
        break;
    case 9:
        *(s32*)(r + 0x110) = 4;
        *(f32*)(r + 0x114) = lbl_80600388;
        *(f32*)(r + 0x118) = lbl_806003B4;
        *(s32*)(r + 0x11C) = 4;
        *(f32*)(r + 0x120) = lbl_806003C4;
        *(f32*)(r + 0x124) = lbl_806003B8;
        *(f32*)(r + 0x128) = lbl_8060039C;
        *(f32*)(r + 0x12C) = lbl_80600390;
        *(f32*)(r + 0x130) = lbl_806003C0;
        *(f32*)(r + 0x134) = lbl_806003C8;
        *(s32*)(r + 0x138) = 0;
        *(s32*)(r + 0x13C) = 0;
        *(f32*)(r + 0x140) = lbl_80600358;
        *(f32*)(r + 0x144) = lbl_80600354;
        break;
    case 10:
        *(s32*)(r + 0x110) = 6;
        *(f32*)(r + 0x114) = lbl_80600388;
        *(f32*)(r + 0x118) = lbl_806003BC;
        *(s32*)(r + 0x11C) = 4;
        *(f32*)(r + 0x120) = lbl_80600394;
        *(f32*)(r + 0x124) = lbl_806003B8;
        *(f32*)(r + 0x128) = lbl_806003A8;
        *(f32*)(r + 0x12C) = lbl_80600390;
        *(f32*)(r + 0x130) = lbl_806003A8;
        *(f32*)(r + 0x134) = lbl_806003C8;
        *(s32*)(r + 0x138) = 0;
        *(s32*)(r + 0x13C) = 0;
        *(f32*)(r + 0x140) = lbl_80600358;
        *(f32*)(r + 0x144) = lbl_80600354;
        break;
    case 11:
        *(s32*)(r + 0x110) = 7;
        *(f32*)(r + 0x114) = lbl_80600388;
        *(f32*)(r + 0x118) = lbl_80600388;
        *(s32*)(r + 0x11C) = 5;
        *(f32*)(r + 0x120) = lbl_806003CC;
        *(f32*)(r + 0x124) = lbl_806003B8;
        *(f32*)(r + 0x128) = lbl_80600398;
        *(f32*)(r + 0x12C) = lbl_80600390;
        *(f32*)(r + 0x130) = lbl_80600320;
        *(f32*)(r + 0x134) = lbl_80600320;
        *(s32*)(r + 0x138) = 0;
        *(s32*)(r + 0x13C) = 0;
        *(f32*)(r + 0x140) = lbl_80600358;
        *(f32*)(r + 0x144) = lbl_80600354;
        break;
    case 12:
        *(s32*)(r + 0x110) = 7;
        *(f32*)(r + 0x114) = lbl_80600388;
        *(f32*)(r + 0x118) = lbl_80600388;
        *(s32*)(r + 0x11C) = 5;
        *(f32*)(r + 0x120) = lbl_806003D0;
        *(f32*)(r + 0x124) = lbl_806003B8;
        *(f32*)(r + 0x128) = lbl_806003A8;
        *(f32*)(r + 0x12C) = lbl_80600390;
        *(f32*)(r + 0x130) = lbl_806003C0;
        *(f32*)(r + 0x134) = lbl_80600320;
        *(s32*)(r + 0x138) = 0;
        *(s32*)(r + 0x13C) = 0;
        *(f32*)(r + 0x140) = lbl_80600358;
        *(f32*)(r + 0x144) = lbl_80600354;
        break;
    default:
        break;
    }
    if (a != 0) {
        if (mode == 2) fn_8025C52C(r);
        else           fn_8025B65C(r);
        if (idx == 0) fn_80258904((void*)fn_801FCDB4, r);
        else          fn_80258968((void*)fn_801FCDB4, r);
    } else {
        if (mode == 2) fn_8025C5FC(r);
        else           fn_8025B72C(r);
        if (idx == 0) fn_80258904(NULL, NULL);
        else          fn_80258968(NULL, NULL);
    }
}

}  // extern "C"
