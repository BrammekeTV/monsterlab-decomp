// Mixed functions from 0x8017xxxx-0x8020xxxx: orientation helpers, particle/effect records,
// HUD event scan, audio bring-up, heap realloc, disc read state machine.
#include <math.h>
#include "types.h"
#include "game/node_ref.h"
#include "revolution.h"   // MEMAllocator, OSTime, GXWGFifo

// Script function table entry (0xC bytes, as in small_801F.cpp).
struct ScriptFunc { u32 type; void* fn; u32 hash; };

// Globals (C++ linkage).
extern ScriptFunc gScriptFuncs[];       // 0x80484230
extern u8  gIdPool[];                   // id pool object
extern u8  gWorldState[];               // 0x803E6D00 (first word points at the world data)
extern u8  sRendDLs[];                  // 0x80401108 (display-list records, 0x94 bytes each)
extern s32 gViewportIndex;              // 0x805FB4BC
extern s32 gFrameBudget;                // 0x805FB4C8
extern MEMAllocator lbl_803F2870;       // 0x803F2870 (heap allocator)
extern const char lbl_80382048[];       // 0x80382048 ("UIRenderModule : attempt to set blend mode (%d) invalid ...")
extern u8  lbl_8048A8B4[];              // 0x8048A8B4 (disc request queue: ring buffer + callbacks)
extern u8  lbl_8051DAD0[];              // 0x8051DAD0 (audio state block)
extern u8  lbl_8051E600[];              // 0x8051E600 (alarm object)
extern u8  lbl_8051E680[];              // 0x8051E680
extern u8* lbl_805FB608;                // 0x805FB608 (heap object: slot +0 = alloc)
extern s32 lbl_805FB6B8;                // 0x805FB6B8
extern u8  lbl_805FB710[];              // 0x805FB710 (default 4-byte key)
extern u8* lbl_805FB8D8;                // 0x805FB8D8 (table of 0x80-byte nodes)
extern s32 lbl_805FB8E8;                // 0x805FB8E8 (script func id)
extern s32 lbl_805FBA8C;                // 0x805FBA8C
extern void* lbl_805FBA90;              // 0x805FBA90
extern void* lbl_805FBA94;              // 0x805FBA94 (id from gIdPool)
extern u8  lbl_805FBA9C;                // 0x805FBA9C
extern u8  lbl_805FBAB8;                // 0x805FBAB8
extern s32 lbl_805FBABC;                // 0x805FBABC
extern s32 lbl_805FBAC0;                // 0x805FBAC0
extern s8  lbl_805FBAC4;                // 0x805FBAC4
extern void* lbl_805FBADC;              // 0x805FBADC
extern s32 lbl_805FBAE0;                // 0x805FBAE0 (re-entrancy guard)
extern s32 lbl_805FBB80[2];             // 0x805FBB80
extern u8* lbl_805FBB88[2];             // 0x805FBB88
extern u8  lbl_805FBB10[];              // 0x805FBB10 (flag, see fn_802041F4)
extern u8  lbl_805FBB11[];              // 0x805FBB11

extern "C" {
void   AIInit(u8* stack);
void   AXInit(void);
void   DCFlushRange(void* addr, u32 size);
void*  IdPoolNext(void* pool);
void*  MEMAllocFromAllocator(MEMAllocator* allocator, u32 size);
void*  MemAllocDefault(u32 size);
void   OSCreateAlarm(u8* alarm);
OSTime OSGetTime(void);
u32    OSGetBusClock(void);
s32    TriggerDoAction(void* action, void* a1, void* a2);        // 0x801E0054
void   ZoneEffectsUpdate(void* effects, f32 dt);                 // 0x801E97FC
s32    DVDGetDriveStatus(void);                                  // 0x8025394C
s32    DVDReadAsync(u8* fileInfo, void* addr, s32 len, s32 offset, void* cb, s32 prio);
u8*    fn_8016B50C(void* a, s32 b);                              // 0x8016B50C
u8*    fn_801ABF0C(u8* table, u32 hash, s32 key);                // 0x801ABF0C
u8*    fn_801AC024(u8* table, u32 hash, s32 key, u8* prev);      // 0x801AC024
void   fn_801ACBEC(void* p, f32 dt);                             // 0x801ACBEC
void*  fn_801B7198(void* anim, s32 a);                           // 0x801B7198
void   fn_801BBB68(void* mgr, s32 id, s32 a, s32 b);            // 0x801BBB68
s32    fn_801BBE40(void* cond, u8* hud, s32 a);                  // 0x801BBE40
s32    fn_801BC4C4(u8* a, s32 b);                                // 0x801BC4C4
u8*    fn_801C668C(u8* key);                                     // 0x801C668C
u8*    fn_801C6BCC(u8* key);                                     // 0x801C6BCC
void   fn_801C6C80(u8* out, u8* rec, u8* color, s32 a);          // 0x801C6C80
s32    fn_801C71EC(u8* key);                                     // 0x801C71EC
void   fn_801D949C(void* p, f32 dt);                             // 0x801D949C
void   fn_801E93EC(u8* list, u8* entry);                         // 0x801E93EC
void*  fn_801EEB20(u8* heap, u32 size);                          // 0x801EEB20
void   fn_801EF070(u8* heap, void* p);                           // 0x801EF070
void   fn_801F7240(u8* p, f32* v);                               // 0x801F7240
void   fn_80147988(void* cb);                                    // 0x80147988
void   fn_8018D2B8(const char* fmt, ...);                        // 0x8018D2B8 (debug log)
s32    fn_8015BA30(void* p);                                     // 0x8015BA30
void   fn_8014DE68(void);                                        // 0x8014DE68
void   fn_80201F38(void);                                        // 0x80201F38 (callback)
void   fn_80202CAC(void);                                        // 0x80202CAC (alarm callback)
void fn_80202FFC(s32 chan, s32 status);                                        // 0x80202FFC (callback)
void   fn_802041F4(s32 a3);                                      // 0x802041F4 (read-done callback)
s32    fn_80204210(u8* e);                                       // 0x80204210
void   fn_80203E8C(void);                                        // 0x80203E8C
void   fn_80234394(u8* alarm, u32 a, OSTime start, OSTime period, void* handler, u32 extra); // 0x80234394
s32    fn_8024F1E8(char* path, u8* p);                           // 0x8024F1E8
void   fn_8024F494(u8* p);                                       // 0x8024F494
s32    fn_80259418(s32 a);                                       // 0x80259418
void*  AXRegisterCallback(void* cb);                                    // 0x80259B2C
s32    fn_8026099C(void);                                        // 0x8026099C
s32    fn_80260AC4(s32 a);                                       // 0x80260AC4
s32    fn_80263F18(void);                                        // 0x80263F18
s32    fn_80264CC8(f32 a);                                       // 0x80264CC8
s32    fn_80264CD0(f32 a);                                       // 0x80264CD0
u8     fn_8026AC14(void);                                        // 0x8026AC14
void*  memcpy(void* d, const void* s, size_t n);
void*  memset(void* p, int c, unsigned long n);
void   operator_delete(void* p);                                 // 0x801D426C
}

static const u32 kHashTimeout = 0xBE048B88u;   // "Timeout"

// 4-byte key used by the effect / color tables: two bytes then a halfword.
struct Key4 { u8 a; u8 b; u16 c; };

// Normalize with 1/sqrt (original: frsqrte with two Newton steps); zero stays zero.
static inline void Normalize3(f32* out, const f32* v) {
    f32 len2 = v[2] * v[2] + (v[0] * v[0] + v[1] * v[1]);
    if (len2 == 0.0f) {
        out[0] = 0.0f; out[1] = 0.0f; out[2] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);
        out[0] = v[0] * inv; out[1] = v[1] * inv; out[2] = v[2] * inv;
    }
}

// Node of the 0x80-byte table at lbl_805FB8D8 named by a packed id (low byte index, byte 1 != 0
// and low byte < 0xFF means valid).
static inline u8* NodeFromId(u16 h) {
    u32 lo = h & 0xFF;
    if (lo < 0xFF && ((h >> 8) & 0xFF) != 0) return lbl_805FB8D8 + lo * 0x80;
    return NULL;
}

// Runs a node's update callback once (state 0x40 -> 0x80).
static inline void NodeRun(u8* n) {
    typedef s32 (*NodeFn)(u8*, s32, f32);
    *(u8*)(n + 0x7F) = 0x40;
    if (*(void**)(n + 0x6C) != NULL) {
        *(f32*)(n + 0x70) = *(f32*)(n + 0x70) + 0.0f;
        *(u8*)(n + 0x7E) = (u8)(*(NodeFn*)(n + 0x6C))(n, *(s32*)(n + 0x68), 0.0f);
    }
    *(u8*)(n + 0x7F) = 0x80;
}

// Ring-buffer record at the tail index of q (0x28-byte records), or NULL when the index is
// outside the buffer.
static inline u8* RingTail(u8* q) {
    s32 head = *(s32*)(q + 0x10);
    s32 idx  = *(s32*)(q + 0x14);
    s32 cap  = *(s32*)(q + 0x18);
    s32 ok = 0;
    if (idx >= 0 && idx < cap) {
        s32 v = idx, hi = idx;
        if (idx < head) { v = idx + cap; hi = idx + cap; }
        if (head <= v && v <= hi) ok = 1;
    }
    return ok ? *(u8**)(q + 0x1C) + idx * 0x28 : NULL;
}

// Pops the tail index of a ring buffer (empties it when head == tail).
static inline void RingPop(u8* q) {
    s32 tail = *(s32*)(q + 0x14);
    if (tail == *(s32*)(q + 0x10)) {
        *(s32*)(q + 0x14) = -1;
        *(s32*)(q + 0x10) = -1;
    } else {
        s32 t = tail - 1;
        if (t < 0) t = *(s32*)(q + 0x18) - 1;
        *(s32*)(q + 0x14) = t;
    }
}

extern "C" {

// 0x80174524
// Copy constructor of a 0x24-byte skeleton record: allocates the matrix block, copies the
// first matrix array from src, resets the second to identity, copies the (word pair) table.
u8* fn_80174524(u8* src) {
    typedef void* (*AllocFn)(s32, s32, s32, s32, s32);
    typedef void* (*AllocFn6)(s32, s32, s32, s32, s32, void*);
    u8* n = (u8*)(*(AllocFn*)lbl_805FB608)(0x24, 0x10, 0x10, 0, 0);
    *(s32*)(n + 0x20) = 0x52;
    s32 count = *(s32*)(src + 0x8);
    *(s32*)(n + 0x8) = count;
    u8* buf = (u8*)(*(AllocFn6*)lbl_805FB608)(count * 0x68, 0x10, 0x11, 0, 0, lbl_805FB608);
    *(u8**)(n + 0x18) = buf;
    *(u8**)(n + 0x0) = buf;
    *(u8**)(n + 0x4) = buf + *(s32*)(n + 0x8) * 0x30;
    *(u8**)(n + 0xC) = *(u8**)(n + 0x4) + *(s32*)(n + 0x8) * 0x30;
    for (s32 i = 0, off = 0; i < *(s32*)(n + 0x8); i++, off += 0x30) {
        u8* s = *(u8**)(src + 0x0) + off;
        u8* d = *(u8**)(n + 0x0) + off;
        for (s32 k = 0; k < 0x30; k += 4) *(f32*)(d + k) = *(f32*)(s + k);
    }
    *(u8**)(n + 0x14) = fn_8016B50C(NULL, *(s32*)(n + 0x8));
    for (s32 i = 0, off = 0; i < *(s32*)(n + 0x8); i++, off += 0x30) {
        u8* d = *(u8**)(n + 0x4) + off;
        // 3x4 identity matrix (the original also fills two dead stack matrices)
        *(f32*)(d + 0x00) = 1.0f;
        *(f32*)(d + 0x04) = 0.0f;
        *(f32*)(d + 0x08) = 0.0f;
        *(f32*)(d + 0x0C) = 0.0f;
        *(f32*)(d + 0x10) = 1.0f;
        *(f32*)(d + 0x14) = 0.0f;
        *(f32*)(d + 0x18) = 0.0f;
        *(f32*)(d + 0x1C) = 0.0f;
        *(f32*)(d + 0x20) = 1.0f;
        *(f32*)(d + 0x24) = 0.0f;
        *(f32*)(d + 0x28) = 0.0f;
        *(f32*)(d + 0x2C) = 0.0f;
    }
    for (s32 i = 0, off = 0; i < *(s32*)(n + 0x8); i++, off += 8) {
        u8* s = *(u8**)(src + 0xC) + off;
        u8* d = *(u8**)(n + 0xC) + off;
        *(s32*)(d + 0x0) = *(s32*)(s + 0x0);
        *(s32*)(d + 0x4) = *(s32*)(s + 0x4);
    }
    *(s32*)(n + 0x10) = 0;
    *(s32*)(n + 0x1C) = 0;
    return n;
}

// 0x8019645C
// UIRenderModule blend mode setter: checks the display-list budget, then writes the BP
// register selecting blend mode 0..7; other modes log an error. Returns 0.
s32 fn_8019645C(u8* self, u32 mode) {
    if (fn_8015BA30(sRendDLs + gViewportIndex * 0x94) + 0x10CA > gFrameBudget) {
        fn_8014DE68();
        fn_8015BA30(sRendDLs + gViewportIndex * 0x94);
    }
    u32 val;
    switch (mode) {
    case 0: val = 0x41000100u; break;
    case 1: val = 0x410004A0u; break;
    case 2: val = 0x41000420u; break;
    case 3: val = 0x41000920u; break;
    case 4: val = 0x41000360u; break;
    case 5: val = 0x410002A0u; break;
    case 6: val = 0x41000420u; break;
    case 7: val = 0x41000920u; break;
    default:
        fn_8018D2B8(lbl_80382048, mode);
        return 0;
    }
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0xFE000FE0u;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = val;
    return 0;
}

// 0x801B4C6C
// Per-frame update of an effect object: pushes the position (+ w = 0) from the info block into
// the child records, then runs either the "killed" path (flag 0x04000000) or the normal update.
void fn_801B4C6C(u8* obj, f32 dt) {
    typedef void (*VFn)(u8*);
    typedef s32 (*VFnR)(u8*);
    u8* info = *(u8**)(obj + 0x4);
    if (*(u32*)(info + 0x20) & 1) {
        u8* list = *(u8**)(obj + 0xA8);
        if (list != NULL) {
            f32 x = *(f32*)(info + 0x0);
            f32 y = *(f32*)(info + 0x4);
            f32 z = *(f32*)(info + 0x8);
            f32 w = 0.0f;
            u8* slot = list;
            for (s32 i = 0; i < *(s32*)(list + 0xC); i++, slot += 4) {
                u8* rec = *(u8**)(slot + 0x14);
                if (rec != NULL) {
                    *(f32*)(rec + 0x10) = x;
                    *(f32*)(rec + 0x14) = y;
                    *(f32*)(rec + 0x18) = z;
                    *(f32*)(rec + 0x1C) = w;
                }
            }
        }
        if (*(u32*)(*(u8**)(obj + 0x4) + 0x20) & 0x04000000) {
            (*(VFn*)(*(u8**)obj + 0x7C))(obj);
            if (*(u8**)(obj + 0x90) != NULL) fn_801D949C(*(u8**)(obj + 0x90), dt);
            return;
        }
        if (*(u8**)(obj + 0xA4) != NULL) fn_801ACBEC(*(u8**)(obj + 0xA4), dt);
        if (*(u16*)(obj + 0x100) != 0) ZoneEffectsUpdate(obj + 0xFC, dt);
        if (*(u8**)(obj + 0x90) != NULL) fn_801D949C(*(u8**)(obj + 0x90), dt);
        (*(VFn*)(*(u8**)obj + 0x7C))(obj);
        if (*(s32*)(*(u8**)(obj + 0x9C) + 0x48) != 0) {
            u8* table = *(u8**)(obj + 0x84);
            if (table != NULL) {
                u8* e = fn_801ABF0C(table, 0xA83AFCFDu, (*(VFnR*)(*(u8**)obj + 0x10))(obj));
                while (e != NULL) {
                    if ((*(u32*)(e + 0x18) >> 27) & 0xF) TriggerDoAction(e + 0x18, obj, NULL);
                    e = fn_801AC024(table, 0xA83AFCFDu, (*(VFnR*)(*(u8**)obj + 0x10))(obj), e);
                }
            }
        }
        u8* inf2 = *(u8**)(obj + 0x4);
        if (*(u32*)(inf2 + 0x20) & 0x10) {
            *(u32*)(inf2 + 0x20) |= 0x4000;
            *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x10000000;
        }
    }
}

// 0x801BB8A0
// Scans the HUD's event table of the current entry for a triggered record: the "Timeout" event
// fires once when the countdown at +0x4C has run out; other records fire when their condition
// (and optional second condition) holds. Then does the same over the owner's shared records
// whose key differs from the current entry's key. Stops at the first record that fires with
// no pending work (+0x10 == 0).
void fn_801BB8A0(u8* hud) {
    s32 i = 0;
    s32 off = 0;
    for (;;) {
        s32 idx = *(s32*)(hud + 0x4);
        u8* ent = idx <= -1 ? NULL : *(u8**)(*(u8**)hud) + idx * 0x30;
        if (i >= *(s32*)(ent + 0x2C)) break;
        idx = *(s32*)(hud + 0x4);
        ent = idx > -1 ? *(u8**)(*(u8**)hud) + idx * 0x30 : NULL;
        u32 flags = *(u32*)(hud + 0x28);
        u8* recs = *(u8**)(ent + 0x20);
        s32 id = *(s32*)(recs + off);
        u8* rec = recs + off;
        if (!(flags & 0x100) && (u32)id == kHashTimeout && *(f32*)(hud + 0x4C) <= 0.0f) {
            *(f32*)(hud + 0x4C) = 0.0f;
            *(u32*)(hud + 0x28) = flags | 0x100;
            fn_801BBB68(hud, id, 0, 0);
            if (*(s32*)(*(u8**)(rec + 0x8) + 0x10) == 0) return;
        } else if (fn_801BBE40(rec + 4, hud, 0) != 0) {
            u32 second = *(u32*)(*(u8**)(rec + 0x8) + 0x8);
            s32 ok = 1;
            if ((second >> 27) & 0xF) ok = fn_801BBE40(&second, hud, 0);
            if (ok != 0) {
                fn_801BBB68(hud, id, 0, 0);
                if (*(s32*)(*(u8**)(rec + 0x8) + 0x10) == 0) return;
            }
        }
        off += 0xC;
        i++;
    }
    {
        s32 idx = *(s32*)(hud + 0x4);
        u8* ent = idx > -1 ? *(u8**)(*(u8**)hud) + idx * 0x30 : NULL;
        s32 key = *(s32*)(ent + 0x4);
        u8* owner = *(u8**)hud;
        s32 j = 0;
        off = 0;
        while (j < *(s32*)(owner + 0x2C)) {
            u8* rec = *(u8**)(owner + 0x20) + off;
            if (fn_801BC4C4(*(u8**)(rec + 0x4), key) == 0 && fn_801BBE40(rec + 8, hud, 0) != 0) {
                fn_801BBB68(hud, *(s32*)(rec + 0x0), 0, 0);
                if (*(s32*)(*(u8**)(rec + 0x4) + 0x10) == 0) return;
            }
            off += 0xC;
            j++;
            owner = *(u8**)hud;
        }
    }
}

// 0x801C9DF0
// Builds an orthonormal basis from the direction v: out[3..5] = normalized v, out[0..2] =
// v x up, out[6..8] = (v x up) x v, out[9..11] = 0. up is +Y, or +Z when v is (anti)parallel
// to the default (0,0,1) up. The extra float argument is unused.
void fn_801C9DF0(u8* /*unused*/ obj, const f32* v, f32* out, f32 /*unused*/ s) {
    f32 n[3];
    f32 up[3] = { 0.0f, 0.0f, 1.0f };
    Normalize3(n, v);
    f32 d = up[2] * n[2] + (up[0] * n[0] + up[1] * n[1]);
    d = (f32)fabs(d) - 1.0f;
    if (d > -0.0001f && d < 0.0001f) {
        up[0] = 0.0f; up[1] = 1.0f; up[2] = 0.0f;
    }
    f32 cx = n[1] * up[2] - n[2] * up[1];
    f32 cy = n[2] * up[0] - n[0] * up[2];
    f32 cz = n[0] * up[1] - n[1] * up[0];
    f32 dx = cy * n[2] - cz * n[1];
    f32 dy = cz * n[0] - cx * n[2];
    f32 dz = cx * n[1] - cy * n[0];
    out[3] = n[0];
    out[4] = n[1];
    out[0] = cx;
    out[1] = cy;
    out[2] = cz;
    out[5] = n[2];
    out[6] = dx;
    out[7] = dy;
    out[8] = dz;
    out[9] = 0.0f;
    out[10] = 0.0f;
    out[11] = 0.0f;
}

// 0x801E5220
// Drains the finished head of a request queue: resolves the handle (slot = id & 0x7F,
// generation = id >> 7) and, while its slot is still busy (+4 != 0), pops ring-buffer entries
// that are done (fn_80204210 == 1 clears the slot and pops the global queue lbl_8048A8B4).
void fn_801E5220(u8* q, s32* handle) {
    u32 h = (u32)*handle;
    u8* slot;
    if (h == 0xFFFFFFFFu) {
        slot = NULL;
    } else {
        slot = *(u8**)(q + 0x4) + (h & 0x7F) * 0x28;
        if ((h >> 7) != *(u32*)(slot + 0x24)) slot = NULL;
    }
    while (slot != NULL && *(s32*)(slot + 0x4) != 0) {
        s32 head = *(s32*)(q + 0x10);
        s32 tail = *(s32*)(q + 0x14);
        if (head >= 0 && tail >= 0) {
            u8* rec = RingTail(q);
            while (rec != NULL && *(s32*)(rec + 0x20) != 0) {
                RingPop(q);
                rec = RingTail(q);
            }
            if (rec != NULL) {
                // the original also passes (1, 0.01f); fn_80204210 ignores both
                if (fn_80204210(rec) == 1) {
                    *(s32*)(rec + 0x4) = 0;
                    RingPop(lbl_8048A8B4);
                }
            }
        }
    }
}

// 0x801E7BC4
// Deleting destructor of a node-list owner: when it has nodes, runs every pending node's update
// callback (state 0x40 -> 0x80) along the chain, sets flag 2 at +6 and, if flag 1 is clear,
// releases nodes that finished (state 0x80) through fn_801E93EC; then frees p if del > 0.
u8* fn_801E7BC4(u8* p, s32 del) {
    if (p != NULL) {
        if (*(u16*)(p + 0x4) != 0) {
            u8* n = NodeFromId(*(u16*)(p + 0x0));
            while (n != NULL) {
                if (*(u8*)(n + 0x7F) != 0x80) NodeRun(n);
                n = NodeFromId(*(u16*)(n + 0x78));
            }
            u16 fl = *(u16*)(p + 0x6) | 2;
            *(u16*)(p + 0x6) = fl;
            if (!(fl & 1) && (fl & 2)) {
                n = NodeFromId(*(u16*)(p + 0x0));
                while (n != NULL) {
                    u8* next = NodeFromId(*(u16*)(n + 0x78));
                    u8 st = *(u8*)(n + 0x7F);
                    if (st == 0x40) NodeRun(n);
                    if (st == 0x80) fn_801E93EC(p, n);
                    n = next;
                }
                *(u16*)(p + 0x6) &= ~2;
            }
        }
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801EAAEC
// Builds an effect record: chains four 4-byte color keys through fn_801C6BCC / fn_801C6C80
// (each key's successor comes from the table record of the previous), then fills the scalar
// fields. Script function id `id` is kept only when its type is 0x66, else the default id.
void fn_801EAAEC(u8* e, s32 a1, s32 a2, u8* col, s32 a4, s32 a5, s32 a6, s32 a7, f32 f1, f32 f2,
                 s32 s8, s32 sC, u8* s10, s32 s14, s32 id, s32 s1C) {
    *(s32*)(e + 0x38) = 0;
    Key4 prev = { 0, 0, 0xFFFF };
    Key4 cur  = { col[0], col[1], *(u16*)(col + 2) };
    Key4 nxt  = { 0, 0, 0xFFFF };
    u8* rec = fn_801C6BCC((u8*)&cur);
    if (rec == NULL) { cur.c = 0xFFFF; cur.a = 0; }
    for (u32 i = 0; i < 4; i++) {
        if (rec != NULL) {
            Key4 out;
            fn_801C6C80((u8*)&out, rec, (u8*)&prev, 0);
            nxt = out;
        }
        u8* e4 = e + i * 4;
        *(u16*)(e4 + 6) = cur.c;
        prev.c = cur.c;
        prev.b = cur.b;
        prev.a = cur.a;
        *(u8*)(e4 + 5) = cur.b;
        cur.b = nxt.b;
        cur.c = nxt.c;
        *(u8*)(e4 + 4) = cur.a;
        cur.a = nxt.a;
        Key4 look = { nxt.a, nxt.b, nxt.c };
        rec = fn_801C6BCC((u8*)&look);
        if (rec == NULL) { cur.c = 0xFFFF; cur.a = 0; }
    }
    *(s32*)(e + 0x18) = a4;
    *(s32*)(e + 0x1C) = a5;
    *(f32*)(e + 0x14) = f1;
    *(s32*)(e + 0x20) = a6;
    *(f32*)(e + 0x0)  = f2;
    *(s32*)(e + 0x28) = a7;
    *(s32*)(e + 0x2C) = s8;
    *(s32*)(e + 0x30) = sC;
    *(u16*)(e + 0x46) = *(u16*)(s10 + 2);
    *(u8*)(e + 0x45) = *(u8*)(s10 + 1);
    *(u8*)(e + 0x44) = *(u8*)(s10 + 0);
    *(u16*)(e + 0x24) = 0xFFFF;
    *(s32*)(e + 0x4C) = s1C;
    *(u16*)(e + 0x34) = 0xFFFF;
    u32 type = (id == -1) ? 0 : gScriptFuncs[id].type;
    if (type != 0x66) id = lbl_805FB8E8;
    *(s32*)(e + 0x48) = id;
    *(s32*)(e + 0x3C) = a1;
    *(s32*)(e + 0x40) = a2;
    if (s14 != 0) *(u32*)(e + 0x38) |= 2;
}

// 0x801EDD00
// Looks up a float parameter named by a packed reference: kind 1 = animation record (+0x14),
// 2 = color table record (+0x30), 3 = record by key (+0x10), 4 = world table entry (value 0);
// 0.0 when not found.
// The reference word is read as a whole so the code does not depend on byte order.
f32 fn_801EDD00(u8* ref) {
    u32 w = NodeRefWord(ref);
    switch (NodeRefKind(w)) {
    case 1: {
        s32 v = lbl_805FB6B8;
        if (NodeRefIsId(w)) v = (s32)w;
        s32 tmp = v;
        u8* r = (u8*)fn_801B7198(&tmp, 0);
        if (r != NULL) return *(f32*)(r + 0x14);
        return 0.0f;
    }
    case 2: {
        Key4 k = { lbl_805FB710[0], lbl_805FB710[1], *(u16*)(lbl_805FB710 + 2) };
        if (NodeRefKind(w) == 2) {
            k.b = NodeRefByte1(w);
            k.a = (u8)NodeRefNibble(w);
            k.c = NodeRefHalf2(w);
        }
        Key4 k2;
        if (fn_801C71EC((u8*)&k) != 0) {
            k2 = k;
        } else {
            k2.a = lbl_805FB710[0];
            k2.b = lbl_805FB710[1];
            k2.c = *(u16*)(lbl_805FB710 + 2);
        }
        u8* r = fn_801C6BCC((u8*)&k2);
        if (r != NULL) return *(f32*)(r + 0x30);
        return 0.0f;
    }
    case 3: {
        Key4 k = { 0xFF, 0, 0 };
        if (NodeRefKind(w) == 3) {
            k.a = (u8)NodeRefHalf2(w);
            k.b = NodeRefByte1(w);
            k.c = (u16)(u8)NodeRefNibble(w);
        }
        u8* r = fn_801C668C((u8*)&k);
        if (r != NULL) return *(f32*)(r + 0x10);
        return 0.0f;
    }
    case 4:
        // The original looks up world[0x28] + idx * 0x20 (idx < world[0x24]) here, but both
        // outcomes return the same constant 0.0.
        return 0.0f;
    default:
        return 0.0f;
    }
}

// 0x801EF484
// Heap realloc (dlmalloc-style chunks: size|flags word at -4 of the payload): returns p when
// it already fits, grows in place into the top chunk or a free neighbour, otherwise allocates a
// new block, copies and frees the old one. NULL p allocates; NULL when the size is too large
// or the chunk is flagged 2.
void* fn_801EF484(u8* heap, u8* p, u32 size) {
    if (p == NULL) return fn_801EEB20(heap, size);
    if (size >= 0xFFFFFFDFu) return NULL;
    u32 need = size + 0x13;
    u32 nsz = 0x10;
    if (need >= 0x10) nsz = need & ~0xFu;
    u32 hdr = *(u32*)(p - 4);
    u8* c = p - 8;
    u32 cur = hdr & ~3u;
    if (hdr & 2) return NULL;
    u32 total;
    if (cur >= nsz) {
        total = cur;
    } else {
        u8* top = *(u8**)(heap + 0x30);
        u8* next = c + cur;
        if (next == top) {
            u32 sum = cur + (*(u32*)(next + 4) & ~3u);
            if (sum >= nsz + 0x10) {
                *(u32*)(c + 4) = (hdr & 1) | nsz;
                u8* nt = c + nsz;
                *(u8**)(heap + 0x30) = nt;
                *(u32*)(nt + 4) = (sum - nsz) | 1;
                return c + 8;
            }
        }
        bool merged = false;
        if (next != top) {
            u32 nh = *(u32*)(next + 4);
            u8* nn = next + (nh & ~1u);
            if (!(*(u32*)(nn + 4) & 1)) {
                total = cur + (nh & ~3u);
                if (total >= nsz) {
                    u8* fd = *(u8**)(next + 8);
                    u8* bk = *(u8**)(next + 0xC);
                    *(u8**)(fd + 0xC) = bk;
                    *(u8**)(bk + 0x8) = fd;
                    merged = true;
                }
            }
        }
        if (!merged) {
            u8* np = (u8*)fn_801EEB20(heap, nsz - 0xF);
            if (np == NULL) return NULL;
            u8* nc = np - 8;
            u32 nsize = *(u32*)(np - 4) & ~3u;
            if (nc == next) {
                total = nsize + cur;
            } else {
                u32 bytes = cur - 4;
                u32 words = bytes >> 2;
                if (words > 9) {
                    memcpy(np, p, bytes);
                } else {
                    *(s32*)(np + 0x0) = *(s32*)(p + 0x0);
                    *(s32*)(np + 0x4) = *(s32*)(p + 0x4);
                    *(s32*)(np + 0x8) = *(s32*)(p + 0x8);
                    if (words > 4) {
                        *(s32*)(np + 0xC)  = *(s32*)(p + 0xC);
                        *(s32*)(np + 0x10) = *(s32*)(p + 0x10);
                        if (words > 6) {
                            *(s32*)(np + 0x14) = *(s32*)(p + 0x14);
                            *(s32*)(np + 0x18) = *(s32*)(p + 0x18);
                            if (words > 8) {
                                *(s32*)(np + 0x1C) = *(s32*)(p + 0x1C);
                                *(s32*)(np + 0x20) = *(s32*)(p + 0x20);
                            }
                        }
                    }
                }
                fn_801EF070(heap, p);
                return nc + 8;
            }
        }
    }
    // Trim: split off the tail when at least 0x10 bytes remain.
    u32 rem = total - nsz;
    if (rem < 0x10) {
        u8* nx = c + total;
        *(u32*)(c + 4) = (*(u32*)(c + 4) & 1) | total;
        *(u32*)(nx + 4) |= 1;
    } else {
        u8* mid = c + nsz;
        u8* after = mid + rem;
        *(u32*)(c + 4) = (*(u32*)(c + 4) & 1) | nsz;
        *(u32*)(mid + 4) = rem | 1;
        *(u32*)(after + 4) |= 1;
        fn_801EF070(heap, mid + 8);
    }
    return c + 8;
}

// 0x801F6CC4
// Sets the +0xA8 basis rows of an object from two direction vectors a and b: when the
// normalized vectors are (nearly) perpendicular (|dot| < 0.0001) it calls fn_801F7240 with a,
// stores b at +0xA8 and b x (object's +0x90 axis) at +0xB4.
void fn_801F6CC4(u8* obj, const f32* a, const f32* b) {
    f32 na[3];
    f32 nb[3];
    Normalize3(na, a);
    Normalize3(nb, b);
    f32 d = na[2] * nb[2] + (na[0] * nb[0] + na[1] * nb[1]);
    if (d > -0.0001f && d < 0.0001f) {
        fn_801F7240(obj, na);
        f32 ax = *(f32*)(obj + 0x90);
        f32 ay = *(f32*)(obj + 0x94);
        f32 az = *(f32*)(obj + 0x98);
        *(f32*)(obj + 0xA8) = nb[0];
        *(f32*)(obj + 0xAC) = nb[1];
        f32 cy = nb[2] * ax - nb[0] * az;
        f32 cx = nb[1] * az - nb[2] * ay;
        *(f32*)(obj + 0xB0) = nb[2];
        f32 cz = nb[0] * ay - nb[1] * ax;
        *(f32*)(obj + 0xB4) = cx;
        *(f32*)(obj + 0xB8) = cy;
        *(f32*)(obj + 0xBC) = cz;
    }
}

// 0x801FCFB4
// Audio bring-up on the audio object: once only (flag at +0x2AC), allocates the sound heap
// and scratch buffer, resets the audio state block, initializes AI/AX, picks the output mode
// from fn_8026AC14, installs callbacks, starts the periodic alarm and clears the object fields.
void fn_801FCFB4(u8* audio) {
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    if (*(u8*)(audio + 0x2AC) != 0) {
        lbl_805FBAE0 = saved;
        return;
    }
    lbl_805FBA8C = 0;
    lbl_805FBA90 = MEMAllocFromAllocator(&lbl_803F2870, 0x60000);
    lbl_805FBADC = MemAllocDefault(0x100);
    memset(lbl_805FBADC, 0, 0x100);
    DCFlushRange(lbl_805FBADC, 0x100);
    lbl_805FBB80[0] = 0;
    lbl_805FBB88[0] = NULL;
    lbl_805FBB80[1] = 0;
    lbl_805FBB88[1] = NULL;
    *(s32*)(lbl_8051DAD0 + 0x9C) = 0;
    *(s32*)(lbl_8051DAD0 + 0x15C) = 1;
    *(s32*)(lbl_8051DAD0 + 0x160) = 1;
    lbl_805FBA94 = IdPoolNext(gIdPool);
    AIInit(NULL);
    AXInit();
    fn_8026099C();
    fn_80263F18();
    lbl_805FBA9C = fn_8026AC14();
    switch (lbl_805FBA9C) {
    case 0:
        fn_80259418(0);
        fn_80260AC4(0);
        break;
    case 1:
        fn_80259418(0);
        fn_80260AC4(1);
        break;
    case 2:
        fn_80259418(2);
        fn_80260AC4(3);
        break;
    }
    AXRegisterCallback((void*)fn_80201F38);
    fn_80203E8C();
    u8 started = lbl_805FBAB8;
    lbl_805FBABC = 0;
    lbl_8051E680[0] = 0;
    *(s32*)(lbl_8051E680 + 0x24) = 0;
    lbl_805FBAC0 = 0;
    lbl_805FBAC4 = 0;
    if (started == 0) {
        OSCreateAlarm(lbl_8051E600);
        OSTime now = OSGetTime();
        u32 timer = OSGetBusClock() >> 2;
        u32 ms = (u32)(((u64)0x431BDE83u * timer) >> 32) >> 15;
        u32 period = (u32)(((u64)0x10624DD3u * (u32)(ms * 0x65B9ABu)) >> 32) >> 9;
        fn_80234394(lbl_8051E600, ms, now, (OSTime)period, (void*)fn_80202CAC, 0x10624DD3u);
        lbl_805FBAB8 = 1;
    }
    fn_80147988((void*)fn_80202FFC);
    fn_80264CC8(20.0f);
    fn_80264CD0(40.0f);
    *(s32*)(audio + 0x2B0) = 0;
    *(s32*)(audio + 0x2B4) = 0;
    *(u8*)(audio + 0x2B9) = 0;
    *(u8*)(audio + 0x2B8) = 0;
    *(u8*)(audio + 0x2AC) = 1;
    lbl_805FBAE0 = saved;
}

// 0x80204210
// Steps one disc read request e (state at +0, sub-state at +4): state 0 opens the file
// (fn_8024F1E8), state 1 starts the async read and then polls the drive status (calling the
// queue's error callbacks at lbl_8048A8B4+0x24 / +0x28), state 3 closes the file. Returns 1 when
// the request is finished, 0 while pending; a request pending for more than 0x14 steps is
// aborted through the callback at lbl_8048A8B4+0x24.
// Callers pass extra (s32, f32) arguments that are ignored.
s32 fn_80204210(u8* e) {
    typedef void (*CbName)(u8*, s32);
    typedef void (*CbFile)(u8*, u8*);
    u8* rec = *(u8**)(e + 0xC);
    switch (*(s32*)(e + 0x0)) {
    case 0:
        if (*(s32*)(e + 0x4) == 1) {
            if (fn_8024F1E8((char*)(rec + 1), rec + 0x128) != 0) return 1;
            *(s32*)(e + 0x8) += 1;
            return 0;
        }
        break;
    case 1:
        if (*(s32*)(e + 0x4) == 1) {
            lbl_805FBB11[0] = 0;
            lbl_805FBB10[0] = 1;
            if (DVDReadAsync(rec + 0x128, *(void**)(e + 0x10), *(s32*)(e + 0x14),
                             *(s32*)(e + 0x18) << 11, (void*)fn_802041F4, 2) != 0) {
                *(s32*)(e + 0x4) = 2;
            } else {
                (*(CbName*)(lbl_8048A8B4 + 0x24))(rec + 1, 0);
            }
        } else if (*(s32*)(e + 0x4) == 2) {
            if (lbl_805FBB10[0] != 0) {
                u32 idx = (u32)(DVDGetDriveStatus() + 1);
                if (idx <= 12) {
                    if (idx == 0) {
                        // status -1 (fatal error)
                        (*(CbName*)(lbl_8048A8B4 + 0x24))(rec + 1, 0);
                    } else if (idx == 5 || idx == 7 || idx == 12) {
                        // status 4 (no disc), 6 (wrong disc), 11 (retry)
                        (*(CbFile*)(lbl_8048A8B4 + 0x28))(rec + 1, rec + 0x128);
                    }
                }
            }
            if (lbl_805FBB10[0] == 0) return 1;
        }
        break;
    case 3:
        if (*(s32*)(e + 0x4) == 1) {
            fn_8024F494(rec + 0x128);
            return 1;
        }
        break;
    default:
        break;
    }
    if (*(s32*)(e + 0x8) > 0x14) {
        (*(CbName*)(lbl_8048A8B4 + 0x24))(rec + 1, 0);
        return 1;
    }
    return 0;
}

}  // extern "C"
