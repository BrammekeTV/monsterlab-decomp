// Mixed batch: heap bin consolidation, menu debug list, emitter init, movie event queue,
// zone-object activation, sound master volume, camera matrices, proximity fades.
#include "types.h"
#include "revolution.h"
#include <math.h>
#include <string.h>

struct ScriptFunc { u32 type; void* fn; u32 hash; };   // 0xC bytes (as in script_dispatch.cpp)
struct ScopedLock { u8 pad[4]; };                       // as in props.cpp

extern "C++" void* gEventMgr;               // 0x805FB6C8

extern "C" {

void*  EventPost(s32 id, void* sender);                                  // 0x801F3918
u8*    GetGlobalSettings(void);                                          // 0x801D2BF8
u32    RandomU32(void);                                                  // 0x80143460
void   ScopedLockCtor(ScopedLock* l);                                    // 0x801F5384
void   ScopedLockDtor(ScopedLock* l, s32 flags);                         // 0x801F53B0
BOOL   ZoneIsLoaded(void* world, s32 zone);                              // 0x8012F408
s32    fn_8012F59C(void* world, s32 zone);                               // 0x8012F59C
void   fn_80144C60(f32* m);                                              // 0x80144C60
s32    fn_801604FC(u8* p);                                               // 0x801604FC
u8* fn_8016056C(u8* table, s32 handle, f32* v);                                // 0x8016056C
u8*    fn_8016060C(u8* p, s32 h, f32 v);                                 // 0x8016060C
void   fn_80160650(u8* p, f32* v, f32 a);                                // 0x80160650
void   fn_8016E304(void* cam);                                           // 0x8016E304
u8*    fn_801842B0(u8* p);                                               // 0x801842B0
void   fn_80188F00(const char* fmt, ...);                                // 0x80188F00
void   fn_801B9EAC(void* mgr, u8* obj);                                  // 0x801B9EAC
void   fn_801BB530(u8* a, u8* b);                                        // 0x801BB530
void   fn_801C83AC(u8* mgr, u8* item);                                   // 0x801C83AC
void*  fn_801DA758(void* v);                                             // 0x801DA758
void   fn_801DA9C8(u8* a, u8* owner);                                    // 0x801DA9C8
void   fn_801E0330(s32 func, u32 sig, u32* args, u32* result);           // 0x801E0330
void   fn_801E839C(u8* p);                                               // 0x801E839C
void   fn_801ED130(void* p);                                             // 0x801ED130
void   fn_801F4E8C(u8* p);                                               // 0x801F4E8C
BOOL   fn_801F5220(void);                                                // 0x801F5220
u8*    fn_801F59F0(u8* p);                                               // 0x801F59F0
u8*    fn_801F5A14(u8* p, u8* q);                                        // 0x801F5A14
void   fn_801F5E48(void* a, s32 b);                                      // 0x801F5E48
void   fn_801F5E4C(void* a, s32 b);                                      // 0x801F5E4C
void   fn_8020A398(s32 handle);                                          // 0x8020A398
f64    fn_8033F204(f64 x);                                               // 0x8033F204 (natural log)

extern u8  gWorldState[];           // 0x803E6D00
extern ScriptFunc gScriptFuncs[];   // 0x80484230
extern u8  lbl_80348F38[];          // 0x80348F38 (dispatch table, handlers at +0x24 + idx*4)
extern u8  lbl_8040E260[];          // 0x8040E260 (effect handle pool)
extern u8  lbl_8047C760[];          // 0x8047C760
extern u8  lbl_804EC590[];          // 0x804EC590 (movie packet ring: +8 head, +0xC tail, +0x10 count, +0x14 base)
extern u8  lbl_804FB5E0[];          // 0x804FB5E0
extern u8  lbl_8051E6A8[];          // 0x8051E6A8 (sound voices, 0x114 each)
extern s32 lbl_805F954C;            // 0x805F954C (menu text colour)
extern s32 lbl_805F9688;            // 0x805F9688
extern f32 lbl_805F98C4;            // 0x805F98C4 (master volume)
extern s32 lbl_805FB628;            // 0x805FB628
extern s32 lbl_805FB62C;            // 0x805FB62C
extern s32 lbl_805FB630;            // 0x805FB630
extern s32 lbl_805FB640;            // 0x805FB640
extern s32 lbl_805FB644;            // 0x805FB644
extern f32 lbl_805FB80C;            // 0x805FB80C (camera shake amount)
extern void* lbl_805FB964;          // 0x805FB964 (movie player)
extern s32 lbl_805FB970;            // 0x805FB970
extern s32 lbl_805FB974;            // 0x805FB974
extern s32 lbl_805FB9AC;            // 0x805FB9AC
extern s32 lbl_805FB9D0;            // 0x805FB9D0
extern s32 lbl_805FB9D4;            // 0x805FB9D4
extern s32 lbl_805FB9D8;            // 0x805FB9D8
extern s32 lbl_805FBAE0;            // 0x805FBAE0 (audio busy flag)
extern s32 lbl_805FBB80[2];         // 0x805FBB80

typedef u8*  (*GetObjFn)(void*);
typedef void (*VoidSelfFn)(void*, s32);
typedef void (*HandlerFn)(void*, s32, u32);

// Movie player "ready" test: both movie flags set, a player exists and its virtual slot 0x34 says yes.
static BOOL MoviePlayerReady() {
    if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != NULL) {
        return (*(s32 (**)(void*))(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964) != 0;
    }
    return 0;
}

// Signed 0.1-unit level used by the sound mixer for a gain: 10 * (30 * log10(v)), clamped to s16 range
// and shifted into the high half-word; gains within +-0.0001 of 0 map to the floor level -1000.
static s32 GainToLevel(f32 v) {
    f32 r;
    if (v > -0.0001f && v < 0.0001f) {
        r = -1000.0f;
    } else {
        f32 ln2 = (f32)fn_8033F204(2.0);
        f32 ln10 = (f32)fn_8033F204(10.0);
        f32 k = ln10 / ln2;
        f32 ln2b = (f32)fn_8033F204(2.0);
        f32 lnv = (f32)fn_8033F204((f64)v);
        r = (30.0f * (lnv / ln2b)) / k;
    }
    s32 t = (s32)(10.0f * r);
    s32 c;
    if (t < -0x8000) c = -0x8000;
    else if (t > 0x7FFF) c = 0x7FFF;
    else c = t;
    return c << 16;
}

// 0x801EF1A8
// Heap consolidation (dlmalloc malloc_consolidate): if the fastbin flag is set, merges every chunk in the
// fast bins with its free neighbours and files it in the unsorted bin (or makes it the top chunk);
// otherwise initialises the 95 bin heads and the heap tuning fields.
void fn_801EF1A8(u8* heap, u8* arg) {
    s32 flags = *(s32*)(arg + 0x0);
    if (flags != 0) {
        *(s32*)(arg + 0x0) = flags & ~2;
        u8* unsorted = arg + 0x38;
        u8** bin = (u8**)(arg + 0x4);
        u8** binEnd = (u8**)(arg + 4 + (((u32)flags >> 3) - 2) * 4);
        for (;;) {
            u8* p = *bin;
            if (p != NULL) {
                *bin = NULL;
                do {
                    u32 psize = *(u32*)(p + 0x4);
                    u8* next = *(u8**)(p + 0x8);
                    u32 size = psize & ~1u;
                    u8* nchunk = p + size;
                    u32 nsize = *(u32*)(nchunk + 0x4) & ~3u;
                    if (!(psize & 1)) {
                        u32 prev = *(u32*)(p + 0x0);
                        p -= prev;
                        size += prev;
                        u8* fd = *(u8**)(p + 0x8);
                        u8* bk = *(u8**)(p + 0xC);
                        *(u8**)(fd + 0xC) = bk;
                        *(u8**)(bk + 0x8) = fd;
                    }
                    if (nchunk != *(u8**)(arg + 0x30)) {
                        u32 nnext = *(u32*)(nchunk + nsize + 0x4);
                        *(u32*)(nchunk + 0x4) = nsize;
                        if (!(nnext & 1)) {
                            u8* fd = *(u8**)(nchunk + 0x8);
                            u8* bk = *(u8**)(nchunk + 0xC);
                            size += nsize;
                            *(u8**)(fd + 0xC) = bk;
                            *(u8**)(bk + 0x8) = fd;
                        }
                        u8* first = *(u8**)(unsorted + 0x8);
                        *(u8**)(unsorted + 0x8) = p;
                        *(u8**)(first + 0xC) = p;
                        *(u32*)(p + 0x4) = size | 1;
                        *(u8**)(p + 0xC) = unsorted;
                        *(u8**)(p + 0x8) = first;
                        *(u32*)(p + size) = size;
                    } else {
                        // next chunk is the top chunk: merge into it
                        *(u32*)(p + 0x4) = (size + nsize) | 1;
                        *(u8**)(arg + 0x30) = p;
                    }
                    p = next;
                } while (p != NULL);
            }
            u8** cur = bin;
            bin++;
            if (cur == binEnd) break;
        }
        return;
    }
    // first use: every bin head points at itself (bins 1..95, 8 bytes apart)
    for (s32 i = 1; i <= 95; i++) {
        u8* b = arg + 0x30 + i * 8;
        *(u8**)(b + 0xC) = b;
        *(u8**)(b + 0x8) = b;
    }
    *(s32*)(arg + 0x34C) = 0;
    *(s32*)(arg + 0x358) = 0;
    *(s32*)(arg + 0x350) = 0x40000;
    *(s32*)(arg + 0x348) = 0x40000;
    *(u32*)(arg + 0x364) = *(u32*)(arg + 0x364) & ~1u;
    *(u32*)(arg + 0x0) = (*(u32*)(arg + 0x0) & 3) | 0x50;
    *(u8**)(arg + 0x30) = arg + 0x38;
    *(s32*)(arg + 0x360) = 0x1000;
}

// 0x801B3940
// Debug menu list: writes two GX BP register words, then prints the title (object text at +0) and the
// visible window of list entries (scroll offset s16 +0x6A, count s16 +0x68, selected node +0x64).
// The selected line is marked with ">>" and drawn in white.
void fn_801B3940(void* menu) {
    u8* obj = (u8*)menu;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0xFE000FE0u;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0x410004A0u;

    // find the last node of the list (nodes link through the base pointer stored at node-4 / node-8)
    u8* head = *(u8**)(*(u8**)(obj + 0x60) + 0x14);
    u8* node = (head != NULL) ? head + 8 : NULL;
    u8* last = node;
    while (node != NULL) {
        u8* nb = *(u8**)(node - 4);
        last = node;
        node = (nb != NULL) ? nb + 8 : NULL;
    }

    f32 y = (f32)(lbl_805FB644 * 4);
    lbl_805FB630 = 0xFFBFBFBF;
    lbl_805FB62C = (s32)y;
    lbl_805FB628 = (s32)(f32)(lbl_805FB640 * 8);
    fn_80188F00("%s", (const char*)obj);
    y = y + (f32)lbl_805FB644;

    // walk back by the scroll offset
    s16 skipped = 0;
    u8* cur = last;
    s32 scroll = *(s16*)(obj + 0x6A);
    while (skipped < scroll && cur != NULL) {
        u8* pb = *(u8**)(cur - 8);
        last = cur;
        skipped++;
        cur = (pb != NULL) ? pb + 8 : NULL;
    }

    s16 shown = 0;
    while (last != NULL && shown < *(s16*)(obj + 0x68)) {
        s32 colour = lbl_805F954C;
        if (last == *(u8**)(obj + 0x64)) {
            colour = -1;
            lbl_805FB630 = -1;
            lbl_805FB62C = (s32)y;
            f32 h = (f32)lbl_805FB640;
            lbl_805FB628 = (s32)((f32)(lbl_805FB640 * 8) - 2.5f * h);
            fn_80188F00("%s", ">>");
        }
        lbl_805FB630 = colour;
        lbl_805FB62C = (s32)y;
        lbl_805FB628 = (s32)(f32)(lbl_805FB640 * 8);
        fn_80188F00("%s", (const char*)(last + 0x60));
        shown++;
        u8* pb = *(u8**)(last - 8);
        y = y + (f32)lbl_805FB644;
        last = (pb != NULL) ? pb + 8 : NULL;
    }
}

// 0x801C7B20
// Initialises a particle effect instance fx from its descriptor desc (spawn position pos[4]) owned by
// manager mgr: timing/scale fields, position, a slot taken from the manager's slot ring (fx+0x60),
// optional effect handles for light (+0x78) and a pooled marker; bumps the manager's live count.
// Returns fx.
u8* fn_801C7B20(u8* fx, u8* desc, const f32* pos, u8* mgr) {
    *(s32*)(fx + 0x7C) = 0;
    if ((*(u32*)(desc + 0x1A8) >> 6) & 1) {
        f32 one = 1.0f;
        *(u32*)(fx + 0x0) |= 1;
        f32 d = *(f32*)(desc + 0x140);
        *(f32*)(fx + 0x50) = d;
        *(f32*)(fx + 0x54) = d;
        *(f32*)(fx + 0xC) = one / d;
        f32 e = *(f32*)(desc + 0x184) * *(f32*)(desc + 0x13C);
        f32 span = *(f32*)(desc + 0x144) - *(f32*)(desc + 0x140);
        *(f32*)(fx + 0x58) = span * (one / e);
    } else {
        *(s32*)(fx + 0x0) = 0;
        f32 d = *(f32*)(desc + 0x170);
        *(f32*)(fx + 0x50) = d;
        *(f32*)(fx + 0x54) = d;
        *(f32*)(fx + 0x58) = 0.0f;
        *(f32*)(fx + 0xC) = 1.0f / d;
    }
    *(f32*)(fx + 0x4) = *(f32*)(fx + 0xC) + *(f32*)(desc + 0x38);
    f32 life = *(f32*)(desc + 0x184);
    if (life < 0.0f) *(f32*)(fx + 0x8) = -1.0f;
    else             *(f32*)(fx + 0x8) = life + *(f32*)(desc + 0x38);

    f32 c0 = *(f32*)(desc + 0x6C);
    f32 c1 = *(f32*)(desc + 0x70);
    f32 c2 = *(f32*)(desc + 0x74);
    f32 c3 = *(f32*)(desc + 0x78);
    u32 flags = *(u32*)(fx + 0x0);
    *(f32*)(fx + 0x30) = c0;
    *(f32*)(fx + 0x34) = c1;
    *(f32*)(fx + 0x38) = c2;
    *(f32*)(fx + 0x3C) = c3;
    *(f32*)(fx + 0x40) = c0;
    *(f32*)(fx + 0x44) = c1;
    *(f32*)(fx + 0x48) = c2;
    *(f32*)(fx + 0x4C) = c3;
    f32 p0 = *(f32*)(desc + 0x2C) + pos[0];
    f32 p1 = *(f32*)(desc + 0x30) + pos[1];
    f32 p2 = *(f32*)(desc + 0x34) + pos[2];
    f32 p3 = *(f32*)(desc + 0x38) + pos[3];
    *(f32*)(fx + 0x10) = p0;
    *(f32*)(fx + 0x14) = p1;
    *(f32*)(fx + 0x18) = p2;
    *(f32*)(fx + 0x1C) = p3;
    *(f32*)(fx + 0x20) = p0;
    *(f32*)(fx + 0x24) = p1;
    *(f32*)(fx + 0x28) = p2;
    *(f32*)(fx + 0x2C) = p3;
    *(u8**)(fx + 0x5C) = desc;
    *(u32*)(fx + 0x0) = flags & ~4u;
    *(s32*)(fx + 0x64) = 0;

    // take a free slot from the manager's ring (pool at mgr+0x10: +0 size, +4 cursor, +8 used flags, +0xC ids)
    u8* pool = *(u8**)(mgr + 0x10);
    s32 idx = *(s32*)(pool + 0x4);
    s32 wrapped = 0;
    s32 id;
    for (;;) {
        if (wrapped == 1 && idx == *(s32*)(pool + 0x4)) { id = 0; break; }
        if (idx == *(s32*)(pool + 0x0)) { idx = 0; wrapped = 1; }
        s32* used = *(s32**)(pool + 0x8);
        s32 at = idx;
        if (used[at] == 0) {
            used[at] = 1;
            idx++;
            *(s32*)(pool + 0x4) = idx % *(s32*)(pool + 0x0);
            id = (*(s32**)(pool + 0xC))[at];
            break;
        }
        idx++;
    }
    *(s32*)(fx + 0x60) = id;

    if (*(f32*)(desc + 0x1B8) > 0.0f) {
        f32 v[3];
        v[0] = *(f32*)(fx + 0x10);
        v[1] = *(f32*)(fx + 0x14);
        v[2] = *(f32*)(fx + 0x18);
        fn_80160650(lbl_8040E260, v, *(f32*)(desc + 0x1B8));
    }
    f32 glow = *(f32*)(desc + 0x1B4);
    if (glow > 0.0f) {
        *(s32*)(fx + 0x78) = fn_801604FC(lbl_8040E260);
        f32 v[3];
        v[0] = *(f32*)(fx + 0x10);
        v[1] = *(f32*)(fx + 0x14);
        v[2] = *(f32*)(fx + 0x18);
        fn_8016056C(lbl_8040E260, *(s32*)(fx + 0x78), v);
        fn_8016060C(lbl_8040E260, *(s32*)(fx + 0x78), glow);
    } else {
        *(s32*)(fx + 0x78) = 0;
    }
    *(s32*)(fx + 0x74) = 0;
    *(s32*)(mgr + 0x4) = *(s32*)(mgr + 0x4) + 1;
    return fx;
}

// 0x801F487C
// Movie packet pump: while the movie system is active and not locked, walks the ring of 1KB packet
// blocks from head to tail; in each block, calls fn_801F4E8C on every entry of a "movie" type (ids
// 0x6D..0x70) or belonging to the current owner (+0x14 == lbl_805FB9AC), resuming at the saved index
// lbl_805FB9D4 after an interruption. Fully processed blocks are popped off the ring.
void fn_801F487C(void) {
    if (lbl_805FB970 == 0) return;
    if (lbl_805FB974 == 0) return;
    if (lbl_805FB9D0 != 0) return;

    if (lbl_805FB9D8 == 0) fn_801F5E4C(lbl_804FB5E0, 5);
    lbl_805FB9D8++;

    u8* ring = lbl_804EC590;
    s32 head = *(s32*)(ring + 0x8);
    u8* blk;
    {
        s32 ok;
        s32 h = head;
        if (head < 0 || head >= *(s32*)(ring + 0x10)) {
            ok = 0;
        } else {
            s32 tail = *(s32*)(ring + 0xC);
            s32 cap = *(s32*)(ring + 0x10);
            if (tail < head) {
                if (head <= tail) h = head + cap;
                tail += cap;
            }
            ok = (head <= h && h <= tail);
        }
        blk = ok ? *(u8**)(ring + 0x14) + (head << 10) : NULL;
    }

    while (blk != NULL) {
        u8* ent = fn_801F59F0(blk);
        s32 resume = lbl_805FB9D4;
        s32 count = 0;
        if (resume > 0) {
            lbl_805FB9D4 = 0;
            count = resume;
            for (s32 i = 0; i < resume; i++) ent = fn_801F5A14(blk, ent);
        }
        while (ent != NULL) {
            s32 take = 0;
            s32 id = (s32)((*(u32*)ent >> 20) & 0x7F);
            if (id < 0x71 && id >= 0x6D) take = 1;
            else if (*(s32*)(blk + 0x14) == lbl_805FB9AC) take = 1;
            if (take) fn_801F4E8C(ent);
            count++;
            if (lbl_805FB9D0 != 0) {
                lbl_805FB9D4 = count;
                break;
            }
            ent = fn_801F5A14(blk, ent);
        }
        s32 locked = lbl_805FB9D0;
        if (locked == 0) {
            s32 h = *(s32*)(ring + 0x8);
            if (*(s32*)(ring + 0xC) == h) {
                *(s32*)(ring + 0xC) = -1;
                *(s32*)(ring + 0x8) = -1;
            } else {
                *(s32*)(ring + 0x8) = (h + 1) % *(s32*)(ring + 0x10);
            }
        }
        if (locked != 0) {
            blk = NULL;
            continue;
        }
        u8* base = *(u8**)(ring + 0x14);
        s32 tail = *(s32*)(ring + 0xC);
        s32 cur = (s32)((blk - base) / 1024);
        if (cur == tail) {
            blk = NULL;
            continue;
        }
        s32 cap = *(s32*)(ring + 0x10);
        s32 idx = (cur + 1) % cap;
        s32 h = idx;
        s32 ok;
        if (idx < 0 || idx >= cap) {
            ok = 0;
        } else {
            s32 nhead = *(s32*)(ring + 0x8);
            if (tail < nhead) {
                if (idx <= tail) h = idx + cap;
                tail += cap;
            }
            ok = (nhead <= h && h <= tail);
        }
        blk = ok ? base + (idx << 10) : NULL;
    }

    lbl_805FB9D8--;
    if (lbl_805FB9D8 == 0) fn_801F5E48(lbl_804FB5E0, 5);
}

// 0x801FF910
// Sets the master volume (clamped to 0..1): stores it, rescales the two reserved voices' levels
// (+0x70 / +0x184 when their enable flags are set) and every voice of type 3..5's +0x28 from its
// +0x24 base value. Restores the audio busy flag afterwards.
void fn_801FF910(f32 v) {
    f32 vol = 0.0f;
    if (v < 0.0f) {
        vol = 0.0f;
    } else {
        vol = 1.0f;
        if (!(v > 1.0f)) vol = v;
    }
    s32 busy = lbl_805FBAE0;
    lbl_805F98C4 = vol;
    lbl_805FBAE0 = 1;
    if (lbl_805FBB80[0] != 0) *(s32*)(lbl_8051E6A8 + 0x70) = GainToLevel(vol);
    if (lbl_805FBB80[1] != 0) *(s32*)(lbl_8051E6A8 + 0x184) = GainToLevel(vol);

    u8* e = lbl_8051E6A8 + 0x228;
    for (s32 i = 0; i < 0x2F; i++) {
        u8* a = *(u8**)(e + 0xC);
        if (a != NULL && (u32)(*(s32*)(a + 0x14) - 3) <= 2u) {
            *(s32*)(a + 0x28) = (s32)((f32)*(s32*)(a + 0x24) * vol);
        }
        u8* b = *(u8**)(e + 0x120);
        if (b != NULL && (u32)(*(s32*)(b + 0x14) - 3) <= 2u) {
            *(s32*)(b + 0x28) = (s32)((f32)*(s32*)(b + 0x24) * vol);
        }
        e += 0x228;
    }
    lbl_805FBAE0 = busy;
}

// 0x801B4944
// Deactivates a zone object (obj+4 = its record): clears its state bits (record +0x20), with mode 2 sets
// bit 0x100; mode 1 notifies (+0xA4) through fn_801BB530; any mode != 0 also releases the event
// registration, the held item (+0xA8), the handle (+0xAC) and the +0x94 sub-object. If an owner object
// exists and is not flagged, hands it to fn_801DA9C8 for its zone (or the zone-1 entry); finally calls
// virtual slot 0x18 on every listener in the list at +0xB0 and, for mode != 0, finishes with fn_801E839C.
void fn_801B4944(u8* obj, s32 mode) {
    if (*(s32*)(obj + 0x4) == 0) return;

    u8* owner = (*(GetObjFn*)(*(u8**)obj + 0xC))(obj);
    if ((*(GetObjFn*)(*(u8**)obj + 0xC))(obj) != NULL) {
        u8* o = (*(GetObjFn*)(*(u8**)obj + 0xC))(obj);
        *(u32*)(o + 0x8) &= ~2u;
    }
    u8* rec = *(u8**)(obj + 0x4);
    *(u32*)(rec + 0x20) &= 0xFFF9F8FCu;
    if (mode == 2) *(u32*)(*(u8**)(obj + 0x4) + 0x20) |= 0x100;
    if (mode == 1) {
        u8** n = *(u8***)(obj + 0xA4);
        if (n != NULL && *n != NULL) fn_801BB530(*n, *(u8**)(obj + 0x4) + 0x37);
    }
    if (mode != 0) {
        if (*(s32*)(obj + 0xA4) != 0) fn_801B9EAC(gEventMgr, obj);
        u8* item = *(u8**)(obj + 0xA8);
        if (item != NULL) {
            fn_801C83AC(lbl_8047C760, item);
            *(s32*)(obj + 0xA8) = 0;
        }
        s32 handle = *(s32*)(obj + 0xAC);
        if (handle != -1) {
            fn_8020A398(handle);
            *(s32*)(obj + 0xAC) = -1;
        }
        fn_801ED130(obj + 0x94);
    }

    if (owner != NULL && !(*(u32*)(*(u8**)(obj + 0x4) + 0x20) & 0x40) &&
        fn_801DA758(owner) != NULL &&
        fn_801842B0(*(u8**)((u8*)fn_801DA758(owner) + 0xC0)) != NULL) {
        s32 zone16 = *(u16*)(*(u8**)(obj + 0x4) + 0x30);
        if (ZoneIsLoaded(gWorldState, zone16) || fn_8012F59C(gWorldState, zone16)) {
            s32 zone = *(u16*)(*(u8**)(obj + 0x4) + 0x30);
            u8* entry = (*(u8***)(*(u8**)gWorldState + 0x1B0))[zone];
            if (entry != NULL) {
                fn_801DA9C8(entry, owner);
            } else if (*(s32*)(GetGlobalSettings() + 0x68) + 1 <= zone) {
                s32 lo = *(s32*)(GetGlobalSettings() + 0x68);
                s32 a7c = *(s32*)(GetGlobalSettings() + 0x7C);
                s32 a6c = *(s32*)(GetGlobalSettings() + 0x6C);
                s32 sum = a7c + lo;
                s32 a70 = *(s32*)(GetGlobalSettings() + 0x70);
                sum = a6c + sum;
                s32 a74 = *(s32*)(GetGlobalSettings() + 0x74);
                sum = a70 + sum;
                s32 a78 = *(s32*)(GetGlobalSettings() + 0x78);
                s32 limit = a78 + (a74 + sum);
                if (!(zone > limit) &&
                    ZoneIsLoaded(gWorldState, zone) &&
                    ZoneIsLoaded(gWorldState, *(u16*)(*(u8**)(obj + 0x4) + 0x32))) {
                    u8* first = (*(u8***)(*(u8**)gWorldState + 0x1B0))[1];
                    if (first == NULL) return;   // no zone-1 entry: skips the listener pass too
                    fn_801DA9C8(first, owner);
                }
            }
        }
    }

    for (u8* n = *(u8**)(obj + 0xB0); n != NULL; n = *(u8**)(n + 0x8)) {
        u8* l = *(u8**)(n + 0x4);
        if (l != NULL) {
            ScopedLock lock;
            ScopedLockCtor(&lock);
            (*(VoidSelfFn*)(*(u8**)l + 0x18))(l, 0);
            ScopedLockDtor(&lock, -1);
        }
    }
    if (mode != 0) fn_801E839C(obj + 0xFC);
}

// 0x801CA004
// Builds an orientation from the direction vector v (normalised; the up vector (0,0,1), or (0,1,0) when
// v is parallel to it): right = dir x up, up' = right x dir. Then stores the offset (right, dir, up')
// applied to the descriptor's vector at descriptor(+0x5C)+0x1BC into obj+0x30..0x3C (w = 0).
void fn_801CA004(u8* obj, const f32* v) {
    f32 vx = v[0], vy = v[1], vz = v[2];
    f32 n2 = vz * vz + (vx * vx + vy * vy);
    f32 ux = 0.0f, uy = 0.0f, uz = 1.0f;
    f32 nx, ny, nz;
    if (n2 == 0.0f) {
        nx = ny = nz = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(n2);
        nx = vx * inv;
        ny = vy * inv;
        nz = vz * inv;
    }
    f32 d = (uz * nz) + ((ux * nx) + (uy * ny));
    f32 t = fabsf(d) - 1.0f;
    if (t > -0.0001f && t < 0.0001f) {
        ux = 0.0f;
        uy = 1.0f;
        uz = 0.0f;
    }
    u8* desc = *(u8**)(obj + 0x5C);
    f32 p1 = *(f32*)(desc + 0x1C0);
    f32 p0 = *(f32*)(desc + 0x1BC);
    f32 p2 = *(f32*)(desc + 0x1C4);
    f32 a1 = ny * uz - nz * uy;     // right
    f32 a2 = nz * ux - nx * uz;
    f32 a3 = nx * uy - ny * ux;
    f32 u1 = a2 * nz - a3 * ny;     // corrected up
    f32 u2 = a3 * nx - a1 * nz;
    f32 u3 = a1 * ny - a2 * nx;
    f32 o0 = 0.0f + (u1 * p2 + (a1 * p0 + nx * p1));
    f32 o1 = 0.0f + (u2 * p2 + (a2 * p0 + ny * p1));
    f32 o2 = 0.0f + (u3 * p2 + (a3 * p0 + nz * p1));
    *(f32*)(obj + 0x30) = o0;
    *(f32*)(obj + 0x34) = o1;
    *(f32*)(obj + 0x38) = o2;
    *(f32*)(obj + 0x3C) = 0.0f;
}

// 0x801C7264
// Proximity fade for a set of 0x24-byte records (count at set+0, array at set+4): for each record finds
// the smallest squared distance to the given points (16 bytes each, +0xC = layer mask) that share a layer
// bit with the record's flags (+0x14). Inside the radius (+0xC) the record's target (+0x20; kind +0x1C:
// 1 = object with handler call via the dispatch table, 2 = float at +0x9C) gets fade = (radius - dist)
// / (+0x10) clamped to 0..1 and the record's bit 0 is set; leaving the radius while bit 0 was set resets the
// fade to 0 and clears the bit.
void fn_801C7264(u8* set, u8* pts, s32 count) {
    if (count == 0) return;
    if (*(s32*)(set + 0x0) == 0) return;
    if (pts == NULL) return;
    for (u32 n = 0; n < *(u32*)(set + 0x0); n++) {
        u8* e = *(u8**)(set + 0x4) + n * 0x24;
        s32 inside = 0;
        f32 radius = *(f32*)(e + 0xC);
        u32 eflags = *(u32*)(e + 0x14);
        u32 wasOn = eflags & 1;
        f32 r2 = radius * radius;
        f32 best = r2 * 2.0f;
        const u8* q = pts;
        for (u32 i = 0; i < (u32)count; i++, q += 0x10) {
            if (eflags & *(u32*)(q + 0xC)) {
                f32 dx = *(f32*)(e + 0x0) - *(f32*)(q + 0x0);
                f32 dy = *(f32*)(e + 0x4) - *(f32*)(q + 0x4);
                f32 dz = *(f32*)(e + 0x8) - *(f32*)(q + 0x8);
                f32 d2 = dz * dz + (dx * dx + dy * dy);
                if (!(best < d2)) best = d2;
            }
        }
        f32 fade = 0.0f;
        BOOL fadeIn = 0;
        if (best < r2) {
            f32 dist = (best <= 0.0f) ? 0.0f : sqrtf(best);
            fade = (*(f32*)(e + 0xC) - dist) / *(f32*)(e + 0x10);
            if (fade < 0.0f) fade = 0.0f;
            else if (fade > 1.0f) fade = 1.0f;
            fadeIn = 1;
            inside = 1;
        } else if (wasOn != 0) {
            fade = 0.0f;
            fadeIn = 1;
        }
        if (fadeIn) {
            s32 kind = *(s32*)(e + 0x1C);
            u8* tgt = *(u8**)(e + 0x20);
            if (kind == 1) {
                if (tgt != NULL) {
                    u32 bits;
                    memcpy(&bits, &fade, sizeof(bits));
                    HandlerFn fn = *(HandlerFn*)(lbl_80348F38 + *(u8*)(tgt + 0xC) * 4 + 0x24);
                    fn(*(void**)(tgt + 0x8), 0x10, bits);
                }
            } else if (kind == 2) {
                if (tgt != NULL) *(f32*)(tgt + 0x9C) = fade;
            }
        }
        if (inside != 0) *(u32*)(e + 0x14) |= 1;
        else             *(u32*)(e + 0x14) &= ~1u;
    }
}

// 0x801DFB58
// Movie-synchronised script command (cmd+0 = script function index, +4 = 16 bytes of arguments, +0x14 =
// result). Modes: 0 = run it now through fn_801E0330 (lbl_805F9688 = 0 meanwhile), 1 = when the movie
// player is ready and the queue is busy, queue an 0x66 event carrying it then run it now, 2 = queue the
// event only when the queue is idle, 4 = run now under the lock when idle, else mark lbl_805F9688 = 3
// and run immediately. lbl_805F9688 is restored on exit.
void fn_801DFB58(u8* cmd, s32 mode) {
    s32 saved = 0;
    if (MoviePlayerReady()) saved = lbl_805F9688;

    switch (mode) {
    case 0: {
        lbl_805F9688 = 0;
        *(s32*)(cmd + 0x14) = 0;
        u32 buf[5];
        memset(buf, 0, 0x14);
        memcpy(buf + 1, cmd + 4, 0x10);
        s32 idx = *(s32*)cmd;
        u32 sig = (idx == -1) ? 0 : *(u32*)((u8*)gScriptFuncs + idx * 0xC);
        fn_801E0330(idx, sig, buf, (u32*)(cmd + 0x14));
        break;
    }
    case 1:
        if (MoviePlayerReady() && fn_801F5220() != 0) {
            u8* ev = (u8*)EventPost(0x66, NULL);
            if (ev != NULL) {
                *(s32*)(ev + 0x4) = *(s32*)cmd;
                memcpy(ev + 8, cmd + 4, 0x10);
            }
            fn_801DFB58(cmd, 0);
        }
        break;
    case 2:
        if (MoviePlayerReady() && fn_801F5220() == 0) {
            u8* ev = (u8*)EventPost(0x66, NULL);
            if (ev != NULL) {
                *(s32*)(ev + 0x4) = *(s32*)cmd;
                memcpy(ev + 8, cmd + 4, 0x10);
            }
        }
        break;
    case 4:
        if (MoviePlayerReady()) {
            if (fn_801F5220() == 0) {
                ScopedLock lock;
                ScopedLockCtor(&lock);
                fn_801DFB58(cmd, 0);
                ScopedLockDtor(&lock, -1);
            } else {
                lbl_805F9688 = 3;
                *(s32*)(cmd + 0x14) = 0;
                u32 buf[5];
                memset(buf, 0, 0x14);
                memcpy(buf + 1, cmd + 4, 0x10);
                s32 idx = *(s32*)cmd;
                u32 sig = (idx == -1) ? 0 : *(u32*)((u8*)gScriptFuncs + idx * 0xC);
                fn_801E0330(idx, sig, buf, (u32*)(cmd + 0x14));
            }
        }
        break;
    }
    lbl_805F9688 = saved;
}

// 0x801D6A4C
// Builds the camera's 4x3 view matrix in cam (12 floats) from the 4x3 matrix a: rotation rows 1 and 2
// both come from a[6..8], row 0 is a[0..2] negated, translation a[9..11] (plus a random camera shake of
// +-lbl_805FB80C/2 per axis when the shake amount is positive). The matrix goes through fn_80144C60, rows
// 0 and 2 are negated again, and the result is stored and passed to fn_8016E304.
void fn_801D6A4C(void* cam, const f32* a) {
    f32 m[12];
    m[1] = 0.0f;
    m[0] = 1.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;
    m[4] = 1.0f;
    m[5] = 0.0f;
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = 1.0f;
    m[9] = 0.0f;
    m[10] = 0.0f;
    m[11] = 0.0f;
    m[6] = a[6];
    m[7] = a[7];
    m[8] = a[8];
    m[3] = a[6];
    m[4] = a[7];
    m[5] = a[8];
    m[0] = -a[0];
    m[1] = -a[1];
    m[2] = -a[2];
    m[9] = a[9];
    m[10] = a[10];
    m[11] = a[11];
    if (lbl_805FB80C > 0.0f) {
        f32 h = 0.5f * lbl_805FB80C;
        f32 sx = -h + ((h - -h) * (2.3283064e-10f * (f32)RandomU32()));
        f32 sy = -h + ((h - -h) * (2.3283064e-10f * (f32)RandomU32()));
        f32 sz = -h + ((h - -h) * (2.3283064e-10f * (f32)RandomU32()));
        m[9] = m[9] + sx;
        m[10] = m[10] + sy;
        m[11] = m[11] + sz;
    }
    fn_80144C60(m);
    m[0] = -m[0];
    m[1] = -m[1];
    m[2] = -m[2];
    m[6] = -m[6];
    m[7] = -m[7];
    m[8] = -m[8];
    f32* out = (f32*)cam;
    for (s32 i = 0; i < 12; i++) out[i] = m[i];
    fn_8016E304(cam);
}

}  // extern "C"
