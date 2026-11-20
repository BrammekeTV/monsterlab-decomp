// Mixed small game functions (bounce step, volume fade, ring-buffer queue entries, GX list draw, ...).
#include <math.h>
#include "types.h"
#include "engine/rend.h"
#include "revolution.h"
extern "C" {
#include "game/script_callbacks.h"
}

// C++ linkage, defined in small_8015.cpp / small_8019.cpp.
void fn_80154358(void);                                           // 0x80154358
void   fn_8015A2E8(u8* p, u32 unused4, u32 flush);                  // 0x8015A2E8
void*  fn_801970D4(void* p);                                        // 0x801970D4

extern "C" {
void   ChoiceInit(u8* c);                                           // 0x8020F36C
void*  ChoiceDestroy(void* c, s32 del);                             // 0x8020F394
void   EulerToQuat(Quat* out, const Vec3* euler);                   // 0x80145B00
u32    StringHash(const char* s);                                   // 0x801EB704
void   WaypointParamsInit(void* params, int a);                     // 0x801ED314
void   fn_8012CDF8(u8* p);                                          // 0x8012CDF8
void   fn_80154318(void);                                           // 0x80154318
u8*    fn_8015A210(u8* p, s32 a, s32* out, s32 b);                  // 0x8015A210
void   fn_8015DE74(RendVideo* v, s32* a, s32* b, s32* c, s32 d);   // 0x8015DE74
void   fn_8015DE90(RendVideo* v, u8* a, s32 b);                     // 0x8015DE90
void   fn_8015DF1C(RendVideo* v, u8* a, u8* b, s32 c, s32 d);       // 0x8015DF1C
void   fn_8018D2B8(const char* fmt, ...);                           // 0x8018D2B8 (debug log)
void*  fn_8018D428(s32 size, const char* file, void* tag, s32 line);   // 0x8018D428 (alloc)
void   fn_8018D4CC(void* p);                                        // 0x8018D4CC (free)
u8*    fn_80196E98(void* obj, const char* name, void* tag);         // 0x80196E98
void   fn_801BB57C(u8* a, u8* b);                                   // 0x801BB57C
void   fn_801BDB6C(u8* node, f32 dt);                               // 0x801BDB6C
void   fn_801BF398(u8* list, u8* item);                             // 0x801BF398
void   fn_801C4338(u8* node);                                       // 0x801C4338
void   fn_801C5B0C(u8* node);                                       // 0x801C5B0C
s32    fn_801D0DDC(u8* p);                                          // 0x801D0DDC
void   fn_801EBA20(void* list, void* item);                         // 0x801EBA20
void   fn_801F31A8(void);                                           // 0x801F31A8
void   fn_801F5E48(void* a, s32 b);                                 // 0x801F5E48
void   fn_801F5E4C(void* a, s32 b);                                 // 0x801F5E4C
void*  fn_801F70AC(u8* p, s32 a);                                   // 0x801F70AC
void   fn_801FFC58(f32 v);                                          // 0x801FFC58
void   fn_802044B8(u8* p, f32 v);                                   // 0x802044B8
void   fn_8020C164(u8* obj);                                        // 0x8020C164

struct ChoicePair {
    u8* p;
    u32 v;
};
ChoicePair fn_8020C614(u8* obj, s32 a);                             // 0x8020C614 (returns r3/r4 pair)
s32    fn_8020DD90(u8* obj, s32 a);                                 // 0x8020DD90
void   fn_8020E30C(u8* obj, void* choice, s32 v);                   // 0x8020E30C

extern u8* gBattle;
extern const char lbl_803820A8[];   // 0x803820A8 (file name string, then log formats)
extern u8 lbl_80348F38[];           // 0x80348F38 (dispatch table)
extern u8 lbl_80386128[];           // 0x80386128 (vtable)
extern u8 lbl_8048A8B4[];           // 0x8048A8B4 (queue object)
extern u8 lbl_804DD180[];           // 0x804DD180
extern u8 lbl_8051E6A8[];           // 0x8051E6A8 (sound voices, 0x114 each)
extern u8 lbl_805F9212;             // 0x805F9212 (copy filter value)
extern s32 lbl_805F9398;            // 0x805F9398
extern s32 lbl_805F939C;            // 0x805F939C
extern u32 lbl_805F9690;            // 0x805F9690 (invalid-handle sentinel)
extern f32 lbl_805F98D4;            // 0x805F98D4
extern u8 lbl_805F98B0;             // 0x805F98B0
extern u8 lbl_805FB50C;             // 0x805FB50C (GX shadow flags byte)
extern u8 lbl_805FB7E8[];           // 0x805FB7E8
extern void* lbl_805FB964;          // 0x805FB964
extern s32 lbl_805FB970;            // 0x805FB970
extern s32 lbl_805FB974;            // 0x805FB974
extern f32 lbl_805FB99C;            // 0x805FB99C
extern f32 lbl_805FB9A0;            // 0x805FB9A0
extern s32 lbl_805FB9A8;            // 0x805FB9A8
extern s32 lbl_805FB9EC;            // 0x805FB9EC
extern u8 lbl_805FBA38;             // 0x805FBA38
extern u8* lbl_805FBA3C;            // 0x805FBA3C
extern s32 (*lbl_805FBA40)(void);   // 0x805FBA40
extern s32 (*lbl_805FBA44)(void);   // 0x805FBA44
extern s32 (*lbl_805FBA48)(void);   // 0x805FBA48
extern s32 (*lbl_805FBA4C)(void);   // 0x805FBA4C
extern s32 lbl_805FBA50;            // 0x805FBA50
extern s32 lbl_805FBA54;            // 0x805FBA54
extern u8* lbl_805FBA58;            // 0x805FBA58
extern s32 lbl_805FBA5C;            // 0x805FBA5C
extern s32 lbl_805FBA60;            // 0x805FBA60
extern s32 lbl_805FBA64;            // 0x805FBA64
extern s32 lbl_805FBA68;            // 0x805FBA68
extern s32 lbl_805FBAE0;            // 0x805FBAE0
extern f32 lbl_805FBAEC;            // 0x805FBAEC
extern s32 lbl_805FBAF8;            // 0x805FBAF8
extern s32 lbl_805FBB64;            // 0x805FBB64
extern s32 lbl_805FBB68;            // 0x805FBB68
}

typedef s32 (*VirtS32Fn)(void* self);
typedef void (*CallbackFn)(u8* node);

extern "C" {

// 0x801F1EB0
// Bounce step of a scalar value (on the object at +4, or through a type-dispatched accessor on the
// handle at +0): adds dt * speed(+8) going up (+0x18 set) or subtracts it going down, reflecting
// at the limits +0xC (low) and +0x10 (high). Returns 2 (3 when there is no target, 2 when the
// state byte at +0x7F is neither 0 nor 0x20).
s32 fn_801F1EB0(u8* arg0, f32 dt) {
    u8 mode = *(u8*)(arg0 + 0x7F);
    if (mode != 0x20 && mode != 0) {
        return 2;
    }
    u8* target = *(u8**)(arg0 + 0x4);
    if (target == NULL && *(u8**)(arg0 + 0x0) == NULL) {
        return 3;
    }
    f32 cur;
    if (target != NULL) {
        cur = *(f32*)(target + 0x9C);
    } else {
        u8* h = *(u8**)(arg0 + 0x0);
        u8* tbl = lbl_80348F38 + *(u8*)(h + 0xC) * 4;
        typedef s32 (*GetFn)(void* sub, s32 mode);
        s32 bits = (*(GetFn*)(tbl + 0x30))(*(void**)(h + 0x8), 0x10);
        cur = *(f32*)&bits;
    }
    f32 step = dt * *(f32*)(arg0 + 0x8);
    f32 v;
    if (*(u8*)(arg0 + 0x18) != 0) {
        v = cur + step;
        f32 hi = *(f32*)(arg0 + 0x10);
        if (v >= hi) {
            *(u8*)(arg0 + 0x18) = 0;
            v = 2.0f * hi - v;
        }
    } else {
        v = cur - step;
        f32 lo = *(f32*)(arg0 + 0xC);
        if (v < lo) {
            *(u8*)(arg0 + 0x18) = 1;
            v = lo + (lo - v);
        }
    }
    target = *(u8**)(arg0 + 0x4);
    if (target != NULL) {
        *(f32*)(target + 0x9C) = v;
    } else {
        u8* h = *(u8**)(arg0 + 0x0);
        u8* tbl = lbl_80348F38 + *(u8*)(h + 0xC) * 4;
        typedef void (*SetFn)(void* sub, s32 mode, s32 bits);
        (*(SetFn*)(tbl + 0x24))(*(void**)(h + 0x8), 0x10, *(s32*)&v);
    }
    return 2;
}

// 0x801FFDB0
// Sets the master volume fraction (clamped to 0..1): forwards it to fn_802044B8 and, when
// lbl_805FBAF8 is clear, to fn_801FFC58 and every sound voice of type 6/7 (scales its +0x28 from
// its +0x24); otherwise just remembers it in lbl_805FBAEC.
void fn_801FFDB0(u8* audio, f32 vol) {
    (void)audio;  // unused by the asm (callers pass the audio manager)
    f32 v = 0.0f;
    if (!(vol < 0.0f)) {
        v = 1.0f;
        if (!(vol > 1.0f)) {
            v = vol;
        }
    }
    fn_802044B8(lbl_805FB7E8, v);
    if (lbl_805FBAF8 != 0) {
        lbl_805FBAEC = v;
        return;
    }
    fn_801FFC58(v);
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    lbl_805F98D4 = v;
    u8* e = lbl_8051E6A8 + 0x228;
    for (s32 i = 0; i < 0x2F; i++) {
        u8* a = *(u8**)(e + 0xC);
        if (a != NULL && (u32)(*(s32*)(a + 0x14) - 6) <= 1u) {
            *(s32*)(a + 0x28) = (s32)((f32)*(s32*)(a + 0x24) * v);
        }
        u8* b = *(u8**)(e + 0x120);
        if (b != NULL && (u32)(*(s32*)(b + 0x14) - 6) <= 1u) {
            *(s32*)(b + 0x28) = (s32)((f32)*(s32*)(b + 0x24) * v);
        }
        e += 0x228;
    }
    lbl_805FBAE0 = saved;
}

// 0x80196D0C
// Creates a typed object (kind 0: 0xC bytes, kind 1: 0x128 bytes) via the allocator and its
// constructor, then calls its virtual slot 0x18 (init); on failure logs and frees (or falls back
// to the object at arg0+4 for kind 0). Returns the object, NULL on failure.
u8* fn_80196D0C(u8* arg0, const char* name, s32 kind) {
    u8* obj = NULL;
    if (kind == 1) {
        u8* mem = (u8*)fn_8018D428(0x128, lbl_803820A8, &lbl_805F9398, 0x87);
        obj = mem;
        if (mem != NULL) {
            obj = (u8*)fn_801970D4(mem);   // asm also passes (name, &lbl_805F939C), unused by the callee
            *(s32*)(obj + 0x4) = 1;
        }
    } else if (kind == 0) {
        u8* mem = (u8*)fn_8018D428(0xC, lbl_803820A8, &lbl_805F9398, 0x73);
        obj = mem;
        if (mem != NULL) {
            obj = fn_80196E98(mem, name, &lbl_805F939C);
            *(s32*)(obj + 0x4) = 0;
        }
        if ((*(VirtS32Fn*)(*(u8**)obj + 0x18))(obj) == 0) {
            fn_8018D2B8(lbl_803820A8 + 0x48, name);
            if (*(u8**)(arg0 + 0x4) != NULL) {
                fn_8018D2B8(lbl_803820A8 + 0x68, name);
                obj = *(u8**)(arg0 + 0x4);
            }
        }
    }
    if (obj != NULL && (*(VirtS32Fn*)(*(u8**)obj + 0x18))(obj) == 0) {
        fn_8018D4CC(obj);
        return NULL;
    }
    StringHash(name);
    return obj;
}

// 0x801F6848
// Constructor: runs the base constructor, sets the vtable and builds an orthonormal frame:
// forward (+0xA8..0xB0) = normalized (0,0,1), and +0xB4..0xBC = forward x (+0x90..0x98).
u8* fn_801F6848(u8* arg0, s32 arg1) {
    fn_801F70AC(arg0, arg1);
    *(void**)arg0 = lbl_80386128;
    f32 x = 0.0f, y = 0.0f, z = 1.0f;
    f32 len2 = z * z + (x * x + y * y);
    f32 nx, ny, nz;
    if (len2 == 0.0f) {
        nx = 0.0f;
        ny = 0.0f;
        nz = 0.0f;
    } else {
        f32 r = (f32)(1.0 / sqrt((double)len2));
        nx = x * r;
        ny = y * r;
        nz = z * r;
    }
    f32 c = *(f32*)(arg0 + 0x98);
    f32 a = *(f32*)(arg0 + 0x90);
    f32 b = *(f32*)(arg0 + 0x94);
    *(f32*)(arg0 + 0xA8) = nx;
    *(f32*)(arg0 + 0xAC) = ny;
    *(f32*)(arg0 + 0xB0) = nz;
    *(f32*)(arg0 + 0xB8) = nz * a - nx * c;
    *(f32*)(arg0 + 0xB4) = ny * c - nz * b;
    *(f32*)(arg0 + 0xBC) = nx * b - ny * a;
    return arg0;
}

// 0x8020BFBC
// Battle: credits the fighter's pending hit counters (battle +0x9CC/0x9D0/0x9D4) to the object,
// clears them, resets the object's float block, picks the next choice via fn_8020C614 and
// registers it with fn_8020E30C.
void fn_8020BFBC(u8* arg0, u8* arg1) {
    u8* b = gBattle + *(s32*)(arg1 + 0x4) * 0xC;
    if (*(s32*)(b + 0x9CC) != -1) {
        u8* e = arg1 + *(s32*)(b + 0x9D0) * 8;
        *(s32*)(e + 0x10C) = *(s32*)(e + 0x10C) + 1;
        b = gBattle + *(s32*)(arg1 + 0x4) * 0xC;
        e = arg1 + *(s32*)(b + 0x9D0) * 8;
        *(s32*)(e + 0x110) = *(s32*)(e + 0x110) + *(s32*)(b + 0x9D4);
        b = gBattle + *(s32*)(arg1 + 0x4) * 0xC;
        *(s32*)(b + 0x9CC) = -1;
        b = gBattle + *(s32*)(arg1 + 0x4) * 0xC;
        *(s32*)(b + 0x9D0) = -1;
        b = gBattle + *(s32*)(arg1 + 0x4) * 0xC;
        *(s32*)(b + 0x9D4) = 0;
    }
    for (s32 off = 0xC; off <= 0x30; off += 4) {
        *(f32*)(arg1 + off) = 0.0f;
    }
    fn_8020C164(arg1);
    s32 k = fn_8020DD90(arg1, 4);
    for (s32 off = 0xC; off <= 0x30; off += 4) {
        *(f32*)(arg1 + off) = 0.0f;
    }
    ChoicePair r = fn_8020C614(arg1, k);
    ChoiceInit(arg0);
    *(u8*)(arg0 + 0x4) = 0;
    s32 x = *(s32*)(r.p + 0x4);
    *(s32*)(arg0 + 0x8) = x;
    *(u32*)(arg0 + 0x0) = r.v;
    u8 tmp[0xC];
    *(u32*)(tmp + 0x0) = r.v;
    *(u8*)(tmp + 0x4) = *(u8*)(arg0 + 0x4);
    *(s32*)(tmp + 0x8) = *(s32*)(arg0 + 0x8);
    fn_8020E30C(arg1, tmp, x);
    ChoiceDestroy(tmp, -1);
}

// 0x801F33C8
// Per-frame update of the list/overlay object lbl_805FB964: runs its virtual slot 0x10 and expires
// slots (3 per 0x1E0-byte record, flag byte set and time older than 10s) decrementing the counter
// at lbl_804DD180+0x3C0. Stores the two time arguments first; defers to fn_801F31A8 on the
// one-shot flags.
void fn_801F33C8(f32 a0, f32 a1) {
    if (lbl_805FB9A8 != 0) {
        fn_801F31A8();
        lbl_805FB9A8 = 0;
        return;
    }
    lbl_805FB9A0 = a0;
    lbl_805FB99C = a1;
    if (lbl_805FB9EC != 0 && lbl_805FB964 != NULL && lbl_805FBB64 == 0 && lbl_805FBB68 == 0) {
        fn_801F31A8();
        lbl_805FB9EC = 0;
        return;
    }
    if (lbl_805FB970 == 0) {
        return;
    }
    (*(void (**)(void*))(*(u8**)lbl_805FB964 + 0x10))(lbl_805FB964);
    if (lbl_805FB974 == 0) {
        return;
    }
    fn_801F5E4C(lbl_804DD180 + 0x3C4, 0);
    u8* rec = lbl_804DD180;
    for (s32 i = 0; i < 2; i++) {
        static const s32 kFlag[3] = {0x94, 0x134, 0x1D4};
        static const s32 kTime[3] = {0x9C, 0x13C, 0x1DC};
        for (s32 j = 0; j < 3; j++) {
            if (*(u8*)(rec + kFlag[j]) != 0 && a1 > 10.0f + *(f32*)(rec + kTime[j])) {
                *(u8*)(rec + kFlag[j]) = 0;
                s32 n = *(s32*)(lbl_804DD180 + 0x3C0) - 1;
                *(s32*)(lbl_804DD180 + 0x3C0) = n;
                if (n < 0) {
                    *(s32*)(lbl_804DD180 + 0x3C0) = 0;
                }
            }
        }
        rec += 0x1E0;
    }
    fn_801F5E48(lbl_804DD180 + 0x3C4, 0);
}

// 0x801B5BD8
// Resets an object after an animation step: resyncs the node at +4 from the object's position and
// euler angles (+0xC8..0xDC) when arg2 is 0, clears flag 0x10 on the node's +0x20, clears
// +0x88/+0xC0/+0xC4, and restarts the waypoint path when the node's +0x3A is not 0xFF. Returns 1.
s32 fn_801B5BD8(u8* arg0, s32 arg1, s32 arg2, u8* arg3) {
    (void)arg1;
    (void)arg3;
    u8** parentSlot = *(u8***)(arg0 + 0xA4);
    if (parentSlot != NULL) {
        u8* parent = *parentSlot;
        if (parent != NULL) {
            *(u8**)(parent + 0x10) = arg0;
        }
    }
    u8* node = *(u8**)(arg0 + 0x4);
    if (node != NULL && arg2 == 0) {
        *(f32*)(node + 0x0) = *(f32*)(arg0 + 0xC8);
        *(f32*)(node + 0x4) = *(f32*)(arg0 + 0xCC);
        *(f32*)(node + 0x8) = *(f32*)(arg0 + 0xD0);
        Vec3 euler;
        euler.x = *(f32*)(arg0 + 0xD4) * 0.63661975f;
        euler.y = *(f32*)(arg0 + 0xD8) * 0.63661975f;
        euler.z = *(f32*)(arg0 + 0xDC) * 0.63661975f;
        EulerToQuat((Quat*)(*(u8**)(arg0 + 0x4) + 0xC), &euler);
    }
    node = *(u8**)(arg0 + 0x4);
    *(u32*)(node + 0x20) = *(u32*)(node + 0x20) & 0xFFFFFFEFu;
    *(s32*)(arg0 + 0x88) = 0;
    *(s32*)(arg0 + 0xC4) = 0;
    *(s32*)(arg0 + 0xC0) = 0;
    fn_8012CDF8(arg0);
    parentSlot = *(u8***)(arg0 + 0xA4);
    if (parentSlot != NULL && *parentSlot != NULL) {
        fn_801BB57C(*parentSlot, *(u8**)(arg0 + 0x4) + 0x37);
    }
    node = *(u8**)(arg0 + 0x4);
    if (*(u8*)(node + 0x3A) != 0xFF) {
        u8 params[4];
        params[0] = 0;
        *(u16*)(params + 2) = 0xFFFF;
        params[1] = 0;
        params[1] = (u8)*(u16*)(*(u8**)(arg0 + 0x4) + 0x32);
        f32 speed = (f32)*(u8*)(node + 0x3B) * 0.125f;
        *(u16*)(params + 2) = *(u8*)(*(u8**)(arg0 + 0x4) + 0x3A);
        WaypointParamsInit(params, 2);
        s32 copy = *(s32*)params;
        FollowWaypointPath(arg0, (u32)(uintptr_t)&copy, speed);  // lint:ok header takes u32 for the params pointer
    }
    node = *(u8**)(arg0 + 0x4);
    s32 timer = *(s32*)(node + 0x28);
    if (timer != 0 && arg2 == 0) {
        *(s32*)(node + 0x28) = timer - 1;
    }
    return 1;
}

// Queue slot allocation shared by fn_801E4E20 / fn_801E4FE0: pops the previous slot of the ring
// (head +0x10, tail +0x14, capacity +0x18, slots of 0x28 bytes at the pointer +0x1C), or NULL
// when the ring is full.
static u8* QueueTakeSlot(void) {
    u8* t = lbl_8048A8B4;
    s32 head = *(s32*)(t + 0x10);
    s32 tail = *(s32*)(t + 0x14);
    s32 cap = *(s32*)(t + 0x18);
    if (head == (tail + 1) % cap) {
        return NULL;
    }
    head -= 1;
    if (head < 0) {
        head = cap - 1;
    }
    tail = *(s32*)(t + 0x14);
    *(s32*)(t + 0x10) = head;
    if (tail < 0) {
        *(s32*)(t + 0x14) = head;
        *(s32*)(t + 0x8) = 0;
    }
    s32 ok = 0;
    if (head >= 0 && head < *(s32*)(t + 0x18)) {
        cap = *(s32*)(t + 0x18);
        s32 mid = head;
        s32 end = *(s32*)(t + 0x14);
        if (end < head) {
            if (head <= end) {
                mid = head + cap;
            }
            end += cap;
        }
        if (head <= mid && mid <= end) {
            ok = 1;
        }
    }
    if (ok == 0) {
        return NULL;
    }
    return *(u8**)(t + 0x1C) + head * 0x28;
}

// Initializes a taken slot (state 1, fresh generation) and returns its handle
// (generation << 7 | slot index); with no slot returns the invalid-handle sentinel.
static u32 QueueMakeHandle(u8* slot) {
    u8* t = lbl_8048A8B4;
    u32 handle = lbl_805F9690;
    if (slot != NULL) {
        *(s32*)(slot + 0x4) = 1;
        *(s32*)(slot + 0x20) = 0;
        *(s32*)(slot + 0x8) = 0;
        u32 gen = *(u32*)(t + 0x20);
        *(u32*)(slot + 0x24) = gen;
        *(u32*)(t + 0x20) = gen + 1;
        if (gen + 1 >= 0x01FFFFFFu) {
            *(u32*)(t + 0x20) = 0;
        }
        handle = (*(u32*)(slot + 0x24) << 7) | (u32)((s32)(slot - *(u8**)(t + 0x1C)) / 40);
    }
    return handle;
}

// Finds the slot of a handle in owner's table (+4), NULL when invalid or the generation differs.
static u8* QueueFindSlot(u8* owner, u32 handle) {
    if (handle + 0x10000 == 0xFFFF) {
        return NULL;
    }
    u8* slot = *(u8**)(owner + 0x4) + (handle & 0x7F) * 0x28;
    if ((handle >> 7) != *(u32*)(slot + 0x24)) {
        return NULL;
    }
    return slot;
}

// 0x801E4E20
// Queues a kind-3 entry (value arg1 at +0xC) in the ring at lbl_8048A8B4 and returns its handle.
s32 fn_801E4E20(u8* arg0, s32 arg1) {
    u8* taken = QueueTakeSlot();
    u32 handle = QueueMakeHandle(taken);
    u8* slot = QueueFindSlot(arg0, handle);
    if (slot != NULL) {
        *(s32*)(slot + 0xC) = arg1;
        *(s32*)(slot + 0x0) = 3;
    }
    return (s32)handle;
}

// 0x801E4FE0
// Queues a kind-1 entry (value arg1 at +0xC, buffer arg2/size arg3/flag arg4 at +0x10/0x14/0x18)
// in the ring at lbl_8048A8B4 and returns its handle.
s32 fn_801E4FE0(u8* arg0, s32 arg1, u8* arg2, u32 arg3, s32 arg4) {
    u8* taken = QueueTakeSlot();
    u32 handle = QueueMakeHandle(taken);
    u8* slot = QueueFindSlot(arg0, handle);
    if (slot != NULL) {
        *(s32*)(slot + 0xC) = arg1;
        *(s32*)(slot + 0x0) = 1;
        *(u8**)(slot + 0x10) = arg2;
        *(u32*)(slot + 0x14) = arg3;
        *(s32*)(slot + 0x18) = arg4;
    }
    return (s32)handle;
}

// 0x801C3A64
// Draws the scene's two display-list groups (entries of 0x80 bytes at +0x1C, count +0x18) with
// GX BP register writes around them: entries with +0x4C == 0 walk their +0x78 list through
// fn_801C4338, the others their +0x7C list through fn_801C5B0C.
void fn_801C3A64(u8* arg0) {
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0xFE00000Fu;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0x80000001u;
    fn_80154358();
    u8* ent = *(u8**)(arg0 + 0x1C);
    for (s32 i = 0; i < *(s32*)(arg0 + 0x18); i++) {
        if (*(s32*)(ent + 0x4C) == 0) {
            u8* head = *(u8**)(*(u8**)(ent + 0x78) + 0x14);
            u8* node = head != NULL ? head + 8 : NULL;
            while (node != NULL) {
                fn_801C4338(node);
                u8* next = *(u8**)(node - 4);
                node = next != NULL ? next + 8 : NULL;
            }
        }
        ent += 0x80;
    }
    ent = *(u8**)(arg0 + 0x1C);
    for (s32 i = 0; i < *(s32*)(arg0 + 0x18); i++) {
        if (*(s32*)(ent + 0x4C) != 0) {
            u8* head = *(u8**)(*(u8**)(ent + 0x7C) + 0x14);
            u8* node = head != NULL ? head + 8 : NULL;
            while (node != NULL) {
                fn_801C5B0C(node);
                u8* next = *(u8**)(node - 4);
                node = next != NULL ? next + 8 : NULL;
            }
        }
        ent += 0x80;
    }
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0xFE000FE0u;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0x410004A0u;
    lbl_805FB50C |= 8;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0xFE00C000u;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = (u32)lbl_805F9212 << 14;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0xFE00000Fu;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0x80000005u;
    fn_80154318();
}

// 0x801FBE68
// Incremental texture-copy job (state lbl_805FBA38): on the first step runs the optional
// callbacks and sets up the copy (fn_8015DE74 / fn_8015A210 / fn_8015DE90), then copies one slice
// of the rows per call (slice lbl_805FBA54 of lbl_805FBA50) with fn_8015DF1C, and on the last
// slice finishes (fn_8015A2E8) and clears the job. A callback returning 0 aborts the step.
void fn_801FBE68(void) {
    if (lbl_805FBA38 == 0) {
        return;
    }
    if (lbl_805FBA54 == 0) {
        if (lbl_805FBA40 != NULL && lbl_805FBA40() == 0) {
            return;
        }
        u8* tex = lbl_805FBA3C + 4;
        u8 fmt = *(u8*)(tex + 0x9);
        lbl_805FBA60 = fmt;
        s32 sp8 = 0;
        fn_8015DE74(&gVideo, &sp8, &lbl_805FBA64, &lbl_805FBA68, fmt);
        lbl_805FBA58 = fn_8015A210(tex, 0, &lbl_805FBA5C, 0);
        fn_8015DE90(&gVideo, lbl_805FBA58, (u16)lbl_805FBA5C);
        if (lbl_805FBA44 != NULL && lbl_805FBA44() == 0) {
            return;
        }
    }
    s32 row = 0;
    s32 end = lbl_805FBA68;
    if (lbl_805FBA50 > 0) {
        if (lbl_805FBA54 == 0) {
            end = 0;
        } else if (lbl_805FBA50 > 1) {
            s32 per = end / lbl_805FBA50;
            row = per * (lbl_805FBA54 - 1);
            if (lbl_805FBA54 != lbl_805FBA50) {
                end = per * lbl_805FBA54;
            }
        }
    }
    while (row < end) {
        u8* p = lbl_805FBA58 + row * lbl_805FBA5C;
        fn_8015DF1C(&gVideo, p, p, lbl_805FBA64, lbl_805FBA60);
        row++;
    }
    if (lbl_805FBA48 != NULL && lbl_805FBA48() == 0) {
        return;
    }
    if (lbl_805FBA54 == lbl_805FBA50) {
        lbl_805F98B0 = 1;
        lbl_805FBA38 = 0;
        fn_8015A2E8(lbl_805FBA3C + 4, 0, 1);
        if (lbl_805FBA4C != NULL && lbl_805FBA4C() == 0) {
            return;
        }
        lbl_805FBA3C = NULL;
    }
    lbl_805FBA54 = lbl_805FBA54 + 1;
}

// Recursively releases the 6-way child tree below node: leaves (4th level) are handed to
// fn_801BF398, every visited child is freed through its owner's list (+0x68 of its object).
static void SweepTree(u8* list, u8* node, s32 depth) {
    for (s32 i = 0; i < 6; i++) {
        u8* c = *(u8**)(node + 4 + i * 4);
        if (c == NULL) {
            continue;
        }
        if (depth < 2) {
            SweepTree(list, c, depth + 1);
        } else {
            for (s32 k = 0; k < 6; k++) {
                u8* leaf = *(u8**)(c + 4 + k * 4);
                if (leaf != NULL) {
                    fn_801BF398(list, leaf);
                }
            }
        }
        fn_801EBA20(*(void**)(*(u8**)c + 0x68), c);
    }
}

// 0x801BE44C
// Per-frame update of the node lists (entries of 0x70 bytes at +0x10, count +0xC) unless +0x18 is
// set: calls fn_801BDB6C on every node, and for finished nodes (+0x44) runs the entry's callback
// (+0x6C, skipped when +0x45 is set) then releases the node's child tree and the node.
void fn_801BE44C(u8* arg0, f32 dt) {
    if (*(u8*)(arg0 + 0x18) != 0) {
        return;
    }
    u8* ent = *(u8**)(arg0 + 0x10);
    for (s32 i = 0; i < *(s32*)(arg0 + 0xC); i++) {
        u8* head = *(u8**)(*(u8**)(ent + 0x68) + 0x14);
        u8* node = head != NULL ? head + 8 : NULL;
        while (node != NULL) {
            u8* nextRaw = *(u8**)(node - 4);
            u8* next = nextRaw != NULL ? nextRaw + 8 : NULL;
            fn_801BDB6C(node, dt);
            if (*(u8*)(node + 0x44) != 0) {
                CallbackFn cb = *(CallbackFn*)(ent + 0x6C);
                if (cb != NULL && *(u8*)(node + 0x45) == 0) {
                    cb(node);
                }
                SweepTree(arg0, node, 0);
                fn_801EBA20(*(void**)(*(u8**)node + 0x68), node);
            }
            node = next;
        }
        ent += 0x70;
    }
}

// 0x801D0C04
// Verlet step for every particle (0x28 bytes, count at +4, array at +0): with acceleration
// (particle +0x18..0x20 plus global +0x14..0x1C), pos += (pos - prev) + accel * dt^2, then the
// accumulated accel is cleared. Ends with fn_801D0DDC.
s32 fn_801D0C04(u8* arg0, f64 dt64) {
    for (s32 i = 0, off = 0; i < *(s32*)(arg0 + 0x4); i++, off += 0x28) {
        u8* p = *(u8**)arg0 + off;
        if (*(u8*)(p + 0x24) != 0) {
            continue;
        }
        f32 dt = (f32)dt64;
        f32 cx = *(f32*)(p + 0xC);
        f32 cy = *(f32*)(p + 0x10);
        f32 cz = *(f32*)(p + 0x14);
        f32 ax = *(f32*)(p + 0x18) + *(f32*)(arg0 + 0x14);
        f32 dx = cx - *(f32*)(p + 0x0);
        f32 dy = cy - *(f32*)(p + 0x4);
        f32 ay = *(f32*)(p + 0x1C) + *(f32*)(arg0 + 0x18);
        f32 dz = cz - *(f32*)(p + 0x8);
        f32 az = *(f32*)(p + 0x20) + *(f32*)(arg0 + 0x1C);
        f32 vx = ax * dt;
        f32 sx = vx * dt;
        f32 vy = ay * dt;
        f32 sy = vy * dt;
        f32 vz = az * dt;
        f32 sz = vz * dt;
        *(f32*)(p + 0xC) = *(f32*)(p + 0xC) + (dx + sx);
        *(f32*)(p + 0x10) = *(f32*)(p + 0x10) + (dy + sy);
        *(f32*)(p + 0x14) = *(f32*)(p + 0x14) + (dz + sz);
        p = *(u8**)arg0 + off;
        *(f32*)(p + 0x0) = cx;
        *(f32*)(p + 0x4) = cy;
        *(f32*)(p + 0x8) = cz;
        p = *(u8**)arg0 + off;
        *(f32*)(p + 0x18) = 0.0f;
        *(f32*)(p + 0x1C) = 0.0f;
        *(f32*)(p + 0x20) = 0.0f;
    }
    return fn_801D0DDC(arg0);
}

}  // extern "C"
