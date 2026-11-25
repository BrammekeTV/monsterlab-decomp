// Small functions batch (miss351): event dispatch, float helper, table lookups and a
// fixed-block pool. Verified against tools/dump.sh asm.
#include "types.h"

// Global event manager (C++ linkage, as in ai_states.cpp).
extern "C++" void* gEventMgr;   // 0x805FB6C8

extern "C" {
void    EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
s32     TriggerDoAction(void* action, void* a1, void* a2);                                         // 0x801E0054
void    fn_801DA81C(u8* p);                                                                        // 0x801DA81C
void    fn_801DB160(u8* p);                                                                        // 0x801DB160
void    fn_801B3494(s32 word, f32 t);                                                              // 0x801B3494
void    fn_801EBAB8(u8* pool);                                                                     // 0x801EBAB8

extern u8     gWorldState[];         // 0x803E6D00
extern u8     lbl_80478470[];        // 0x80478470 (256 used flags)
extern u8     lbl_80415470[];        // 0x80415470 (records of 0x21 * 0x30 bytes)
extern f32    lbl_805FFA38;          // 0x805FFA38
extern f32    lbl_805FFA40;          // 0x805FFA40
extern void*  lbl_805FB6A0;          // 0x805FB6A0 (list head)
extern void*  lbl_805FB704;          // 0x805FB704 (sorted float table, 0x10-byte records)
extern s32    lbl_805FB708;          // 0x805FB708 (table count)
}

extern "C" {

// 0x8006D774
// Walks five 8-byte slots at +0x1D4. Each pending entry is cleared, its owner/self action
// bits (+0xA4 of the owner at +0x9C, and +0x74) are checked and TriggerDoAction runs,
// then the entity gets event 0xA29C69BA.
void fn_8006D774(u8* p) {
    for (u32 i = 0; i < 5; i++) {
        u8* entry = *(u8**)(p + 0x1D4 + i * 8);
        if (entry == NULL) continue;
        *(void**)(p + 0x1D4 + i * 8) = NULL;
        u8* owner = *(u8**)(p + 0x9C);
        if (owner != NULL && ((*(u32*)(owner + 0xA4) >> 27) & 0xF) != 0) {
            TriggerDoAction(owner + 0xA4, entry, p);
        }
        if (((*(u32*)(p + 0x74) >> 27) & 0xF) != 0) {
            TriggerDoAction(p + 0x74, entry, p);
        }
        EventSend(gEventMgr, p, p, 0xA29C69BAu, 0, 0, 1, 0);
    }
}

// 0x8012FF5C
// Releases the object in r4 through both helpers when it is non-null.
void fn_8012FF5C(u8* self, void* obj) {
    (void)self;
    if (obj != NULL) {
        fn_801DA81C((u8*)obj);   // asm also passes r4 = 0; the 1-arg callee ignores it
        fn_801DB160((u8*)obj);
    }
}

// 0x801A9114
// Calls the getter at slot +0x40 of the object at +0x138, scales the result by the float at +0x74
// and returns the absolute value.
f32 fn_801A9114(u8* arg0) {
    f32 sp8;
    u8* sub = *(u8**)(arg0 + 0x138);
    (*(void (**)(f32*, u8*))(sub + 0x40))(&sp8, arg0);
    f32 v = *(f32*)(arg0 + 0x74) * sp8;
    if (v < lbl_805FFA38) {
        v = v * lbl_805FFA40;
    }
    return v;
}

// 0x801B327C
// Sends the world frame delta (gWorldState +0xC) to each entry of the global list at lbl_805FB6A0.
void fn_801B327C(void) {
    u8* list = (u8*)lbl_805FB6A0;
    if (list == NULL) return;
    f32 t = *(f32*)(gWorldState + 0xC);
    s32 i = 0;
    s32 off = 0;
    while (i < *(s32*)(list + 0x0)) {
        fn_801B3494(*(s32*)(*(u8**)(list + 0x4) + off), t);
        off += 4;
        i += 1;
    }
}

// 0x801BC910
// Claims the first free slot (flag byte 0) of the 256-entry table at lbl_80478470, marks it used and
// returns its 0x21 * 0x30-byte record in lbl_80415470. Returns NULL when the table is full.
u8* fn_801BC910(void) {
    for (s32 i = 0; i < 256; i++) {
        if (lbl_80478470[i] == 0) {
            lbl_80478470[i] = 1;
            return lbl_80415470 + i * 0x21 * 0x30;
        }
    }
    return NULL;
}

// 0x801C6564
// Binary search over the 0x10-byte records at lbl_805FB704 (count lbl_805FB708). Returns the index
// whose key range (float +0 .. float +4) contains key, or -1.
s32 fn_801C6564(f32 key) {
    u8* tbl = (u8*)lbl_805FB704;
    s32 count = lbl_805FB708;
    s32 lo = -1;
    s32 hi = count;
    while (hi - lo > 1) {
        s32 mid = (hi + lo) >> 1;
        f32 v = *(f32*)(tbl + mid * 16 + 0x4);
        if (key <= v) {
            hi = mid;
        } else {
            lo = mid;
        }
    }
    if (hi < 0 || hi >= count) return -1;
    f32 v0 = *(f32*)(tbl + hi * 16);
    if (!(v0 <= key)) return -1;
    f32 v4 = *(f32*)(tbl + hi * 16 + 0x4);
    if (v4 >= key) return hi;
    return -1;
}

// 0x801EB858
// Sets up a fixed-block pool at pool: block size is at least 16 and rounded to 8-byte steps,
// buf is the backing store, and the block count is (size - 4 - blockSize) / stride.
// Then the free list is initialised by fn_801EBAB8.
void* fn_801EB858(u8* pool, s32 elemSize, s32 align, u8* buf, u32 size) {
    s32 b = align;
    s32 a = elemSize;
    if (b < 0x10) b = 0x10;
    if (a < 1) a = 1;
    if (a < b) a = b;

    *(u8*)(pool + 0x20) = 0;
    *(void**)(pool + 0x24) = buf;
    *(s32*)(pool + 0x8) = b;
    u32 stride = ((u32)(a + b + 7)) & ~((u32)(b - 1));
    *(s32*)(pool + 0x4) = (s32)(stride - 8);
    *(s32*)(pool + 0x1C) = 0;
    *(s32*)(pool + 0x18) = 0;
    *(s32*)(pool + 0x10) = 0;
    *(s32*)(pool + 0x14) = 0;
    *(s32*)(pool + 0xC) = 0;
    *(s32*)(pool + 0x0) = (s32)((size - 4 - (u32)b) / stride);
    fn_801EBAB8(pool);
    return pool;
}

// 0x801EBD2C
// Pops the head node of the free list at n + 0x10 (n = arg0 +0x4), links it in front of *arg0
// and returns its payload (node + 8), or NULL.
void* fn_801EBD2C(void* node) {
    u8* arg0 = (u8*)node;
    u8* n = *(u8**)(arg0 + 0x4);
    if (*(void**)(n + 0x10) == NULL) {
        if (*(u8*)(n + 0x20) == 0) return NULL;
        fn_801EBAB8(n);
    }
    // the fields are read again after each store, as the original does
    if (*(u8**)arg0 != NULL) {
        u8* x = *(u8**)arg0;
        *(void**)x = *(void**)(*(u8**)(arg0 + 0x4) + 0x10);
    }
    n = *(u8**)(arg0 + 0x4);
    u8* x = *(u8**)arg0;
    u8* nx = *(u8**)(n + 0x10);
    u8* old = *(u8**)(nx + 0x4);
    *(void**)(nx + 0x4) = x;
    n = *(u8**)(arg0 + 0x4);
    *(void**)arg0 = *(void**)(n + 0x10);
    *(void**)(n + 0x10) = old;
    void* head = *(void**)(*(u8**)(arg0 + 0x4) + 0x10);
    if (head != NULL) {
        *(void**)head = NULL;
    }
    u8* first = *(u8**)arg0;
    return first != NULL ? first + 8 : NULL;
}

}  // extern "C"
