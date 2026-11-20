// Misc functions: renderer display-list ring helpers, GX BP/viewport setup, shader overlay
// resource teardown, and the overlay object factory.
#include <string.h>
#include "types.h"
#include "revolution.h"
#include "engine/rend.h"

extern "C" {
void   fn_80142ED0(u8* base, s32 size);          // 0x80142ED0 (DCFlushRange)
void   fn_80157324(void);                        // 0x80157324
void   fn_80157D6C(u8* dst, u8* src, s32 w, s32 h);   // 0x80157D6C
void   fn_801588D8(void);                        // 0x801588D8
void   fn_80159258(void);                        // 0x80159258
u8*    fn_8015ECA0(u8* a3);                      // 0x8015ECA0
void   fn_801697E0(u8* a, u8* b);                // 0x801697E0
void   fn_801698C0(u8* a, u8* b);                // 0x801698C0
void   fn_8016A2B8(u8* p);                       // 0x8016A2B8
void   fn_801731C8(void* p);                     // 0x801731C8
s32    fn_80174C2C(u8* p);                       // 0x80174C2C
void   fn_80184268(void* p);                     // 0x80184268
void   fn_80185C94(void* p);                     // 0x80185C94
void   TextureRelease(void* tex);                // 0x80184028
void   fn_801732B0(u8* p);                       // 0x801732B0

extern RendDL       sRendDLs[];          // 0x80401108 (display-list buffers)
extern RendDLRecord sRendDLRecords[];    // 0x80401ADC (submit records)
extern s32  gViewportIndex;              // 0x805FB4BC (display list being recorded)
extern s32  gViewportCount;              // 0x805FB4C0 (1 while recording)
extern s32  gFrameBudget;                // 0x805FB4C8 (bytes still needing a cache flush start here)
extern s32  lbl_805FB4C4;                // 0x805FB4C4 (display-list capacity)
extern s32  lbl_805FB4CC;                // 0x805FB4CC
extern u8*  lbl_805FB4D0;                // 0x805FB4D0 (display-list write cursor)
extern s32  lbl_805FB4D4;                // 0x805FB4D4 (defer-submit flag)
extern s32  lbl_805FB4D8;                // 0x805FB4D8 (defer queue head)
extern s32  lbl_805FB4DC;                // 0x805FB4DC (defer queue count)
extern s32  lbl_805FB4E0;                // 0x805FB4E0 (keep-display-lists flag)
extern s32  lbl_805FB4E4;                // 0x805FB4E4 (kept display-list ring start)
extern s32  lbl_805FB4E8;                // 0x805FB4E8 (kept display-list count)
extern s32  lbl_805FB4EC;                // 0x805FB4EC (display-list balance)
extern s32  lbl_805FB500;                // 0x805FB500
extern s32  lbl_805FB504;                // 0x805FB504
extern u8   lbl_805FB50C;                // 0x805FB50C (GX shadow flags byte)
extern s32  lbl_805FB514;                // 0x805FB514
extern u8   lbl_803488F0[];              // 0x803488F0 (24 byte GX table)
extern u8   lbl_80348F38[];              // 0x80348F38 (shader overlay dispatch table)
extern f32  lbl_80376BD8[];              // 0x80376BD8 (3 floats)
extern u8   lbl_8040AD70[];              // 0x8040AD70 (blur/downsample target records)
extern u8   lbl_8040ADD0[];              // 0x8040ADD0
extern u8   lbl_8040D500[];              // 0x8040D500 (record passed to fn_8015ECA0)
extern s32  lbl_805F9234;                // 0x805F9234
extern s32  lbl_805F9238;                // 0x805F9238
extern u8*  lbl_805FB608;                // 0x805FB608 (heap object: alloc fn at +0, free fn at +4)
}

typedef void* (*HeapAllocFn)(u32 size, u32 align, s32 tag, const char* name, u32 extra);
typedef void  (*HeapFreeFn)(void* p, void* heap);
typedef void  (*OverlayFn)(void* p);

extern "C" {

// 0x80172F70
// Allocates a named 0x64+len object, type 0x4C, with identity-ish transform fields (0/1
// floats), and a normalised direction at +0x2C..+0x34 (zeroed when the dot product is ~0).
// The stack stores to a local matrix/vector in the original are dead and omitted.
void* fn_80172F70(s32 a, const void* name) {
    u8* obj = (u8*)(*(HeapAllocFn*)lbl_805FB608)(strlen((const char*)name) + 0x64, 0x10, 0xD, 0, 0);
    *(s32*)(obj + 0x0) = 0x4C;
    strcpy((char*)(obj + 0x60), (const char*)name);
    *(s16*)(obj + 0x10) = (s16)a;
    *(s16*)(obj + 0x12) = 0;
    *(f32*)(obj + 0x54) = 1.0f;
    *(f32*)(obj + 0x58) = 0.0f;
    *(f32*)(obj + 0x5C) = 0.0f;
    *(f32*)(obj + 0x44) = 0.0f;
    *(f32*)(obj + 0x48) = 0.0f;
    *(f32*)(obj + 0x4C) = 0.0f;
    *(f32*)(obj + 0x50) = 0.0f;
    *(f32*)(obj + 0x20) = 0.0f;
    *(f32*)(obj + 0x24) = 0.0f;
    *(f32*)(obj + 0x28) = 0.0f;
    *(f32*)(obj + 0x38) = 0.0f;
    *(f32*)(obj + 0x3C) = 0.0f;
    *(f32*)(obj + 0x40) = 1.0f;
    *(f32*)(obj + 0x14) = 1.0f;
    *(f32*)(obj + 0x18) = 0.0f;
    *(f32*)(obj + 0x1C) = 0.0f;
    *(s32*)(obj + 0x4) = 0;
    *(s32*)(obj + 0x8) = 0;
    *(s32*)(obj + 0xC) = 0;
    f32 zero = 0.0f;
    f32 dot = zero * lbl_80376BD8[2] + (zero * lbl_80376BD8[0] + zero * lbl_80376BD8[1]);
    if (dot <= 0.0001f && dot >= -0.0001f) {
        *(f32*)(obj + 0x2C) = 0.0f;
        *(f32*)(obj + 0x30) = 0.0f;
        *(f32*)(obj + 0x34) = 0.0f;
    } else {
        f32 inv = 1.0f / dot;
        f32 x = *(f32*)(obj + 0x54) * inv;
        f32 y = *(f32*)(obj + 0x58) * inv;
        f32 z = *(f32*)(obj + 0x5C) * inv;
        *(f32*)(obj + 0x2C) = x;
        *(f32*)(obj + 0x30) = y;
        *(f32*)(obj + 0x34) = z;
    }
    return obj;
}

// 0x8014E090
// Finishes the display list being recorded: flushes the unflushed tail, submits it (or queues
// it when submits are deferred), resets the recording state and advances to the next list.
void fn_8014E090(void) {
    RendDLEnd(&sRendDLs[gViewportIndex]);
    s32 unflushed = 0x24000 - gFrameBudget;
    if (unflushed != 0) {
        fn_80142ED0(RendDLGetBase(&sRendDLs[gViewportIndex]) + gFrameBudget, unflushed);
    }
    if (lbl_805FB4D4 != 0) {
        lbl_805FB4DC = lbl_805FB4DC + 1;
    } else {
        RendThreadSubmitDL(&sRendDLRecords[gViewportIndex], lbl_805FB4E0 == 0);
    }
    gViewportCount = 0;
    lbl_805FB4D0 = NULL;
    lbl_805FB4C4 = 0;
    gFrameBudget = 0;
    lbl_805FB4CC = 0;
    if (lbl_805FB4E0 != 0) {
        lbl_805FB4E8 = lbl_805FB4E8 + 1;
    }
    gViewportIndex = gViewportIndex + 1;
    if (gViewportIndex >= 0x11) {
        gViewportIndex = 0;
    }
}

// 0x8014E198
// Flushes any deferred display lists (when the free pool is empty), then starts recording the
// next display list.
void fn_8014E198(void) {
    if (lbl_805FB4D4 != 0 && RendHasFreeDL() == 0 && lbl_805FB4D4 != 0) {
        RendThreadSync();
        s32 head = lbl_805FB4D8;
        s32 count = lbl_805FB4DC;
        while (count > 0) {
            RendThreadSubmitDL(&sRendDLRecords[head], lbl_805FB4E0 == 0);
            head = lbl_805FB4D8 + 1;
            lbl_805FB4D8 = head;
            if (head >= 0x11) {
                lbl_805FB4D8 = 0;
                head = 0;
            }
            count = lbl_805FB4DC - 1;
            lbl_805FB4DC = count;
        }
        lbl_805FB4D4 = 0;
        lbl_805FB4D8 = 0;
        lbl_805FB4DC = 0;
    }
    RendWaitFreeDL();
    RendDLBegin(&sRendDLs[gViewportIndex], &sRendDLRecords[gViewportIndex]);
    gViewportCount = 1;
    lbl_805FB4C4 = 0x24000;
    gFrameBudget = 0x24000;
    lbl_805FB4CC = 0;
    lbl_805FB4D0 = RendDLGetBase(&sRendDLs[gViewportIndex]);
    lbl_805FB4EC = lbl_805FB4EC - 1;
}

// 0x8014E2E4
// Drains the deferred display lists, then (when the balance is non-negative) resubmits the
// kept display lists in ring order.
void fn_8014E2E4(void) {
    if (lbl_805FB4D4 != 0) {
        RendThreadSync();
        s32 head = lbl_805FB4D8;
        s32 count = lbl_805FB4DC;
        while (count > 0) {
            RendThreadSubmitDL(&sRendDLRecords[head], lbl_805FB4E0 == 0);
            head = lbl_805FB4D8 + 1;
            lbl_805FB4D8 = head;
            if (head >= 0x11) {
                lbl_805FB4D8 = 0;
                head = 0;
            }
            count = lbl_805FB4DC - 1;
            lbl_805FB4DC = count;
        }
        lbl_805FB4D4 = 0;
        lbl_805FB4D8 = 0;
        lbl_805FB4DC = 0;
    }
    if (lbl_805FB4EC >= 0) {
        for (s32 i = 0; i < lbl_805FB4E8; i++) {
            s32 idx = lbl_805FB4E4 + i;
            if (idx >= 0x11) idx -= 0x11;
            RendThreadSubmitDL(&sRendDLRecords[idx], 1);
        }
    }
}

// 0x8014EAF4
// Writes the 24-byte table lbl_803488F0 to the GX BP registers (0x41/0x11 header, then the
// 0x01..0x04 packed nibble words), a pair of 0xFE masks/0x0F clears, and the 0x43 mode word.
// The stack copy of the table in the original is dead and omitted.
void fn_8014EAF4(void) {
    const u8* t = lbl_803488F0;
    u32 w1 = ((u32)t[4] << 16) | ((u32)t[3] << 12) | (((u32)t[2] << 8) | ((u32)t[0] | ((u32)t[1] << 4)))
             | (((u32)t[5] << 20) | 0x01000000);
    u32 w2 = ((u32)t[11] << 20);
    u32 w3 = ((u32)t[9] << 12);
    u32 w4 = ((u32)t[8] << 8);
    u32 w5 = (u32)t[6] | ((u32)t[7] << 4);
    u32 w6 = ((u32)t[10] << 16) | w3;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = ((u32)lbl_805FB504 << 2) | 0x41000000 | 0x4B9;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x11;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = w1;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = (w6 | (w4 | w5)) | (w2 | 0x02000000);
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = ((u32)t[16] << 16) | ((u32)t[15] << 12) | (((u32)t[14] << 8) | ((u32)t[12] | ((u32)t[13] << 4)))
                    | (((u32)t[17] << 20) | 0x03000000);
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = ((u32)t[22] << 16) | ((u32)t[21] << 12) | (((u32)t[20] << 8) | ((u32)t[18] | ((u32)t[19] << 4)))
                    | (((u32)t[23] << 20) | 0x04000000);
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFEFFFF00;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x0F000000;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFEFFFF00;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x0F000000;
    s32 mode = lbl_805FB500 | 0x40;
    lbl_805FB514 = mode;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = (u32)mode | 0x43000000;
    lbl_805FB50C = (u8)(lbl_805FB50C & ~4);
}

// 0x80156898
// Sets the GX viewport and scissor to the clamped rectangle (x, y)-(x2, y2) of the video
// output, and records its size in lbl_805F9234 / lbl_805F9238.
void fn_80156898(s32 x, s32 y, s32 x2, s32 y2) {
    s32 w = VideoGetWidth(&gVideo);
    s32 h = VideoGetHeight(&gVideo);
    s32 left = x & ~(x >> 31);
    s32 top = y & ~(y >> 31);
    if (x2 < left + 1) x2 = left + 1;
    if (y2 < top + 1) y2 = top + 1;
    if (x2 > w) x2 = w;
    if (y2 > h) y2 = h;
    if (left > x2 - 1) left = x2 - 1;
    if (top > y2 - 1) top = y2 - 1;
    s32 rw = x2 - left;
    s32 rh = y2 - top;
    f32 halfW = 0.5f * (f32)rw;
    f32 halfH = -0.5f * (f32)rh;
    f32 cx = 342.0f + 0.5f * (f32)(x2 + left);
    f32 cy = 342.0f + 0.5f * (f32)(top + y2);
    lbl_805F9234 = rw;
    lbl_805F9238 = rh;
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 5;
    GXWGFifo._u16 = 0x101A;
    GXWGFifo._f32 = halfW;
    GXWGFifo._f32 = halfH;
    GXWGFifo._f32 = 16777216.0f;
    GXWGFifo._f32 = cx;
    GXWGFifo._f32 = cy;
    GXWGFifo._f32 = 16777216.0f;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = ((((u32)(left + 0x156)) & 0xFFFF) << 12) | (u16)(top + 0x156) | 0x20000000;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = ((((u32)(x2 + 0x155)) & 0xFFFF) << 12) | (u16)(y2 + 0x155) | 0x21000000;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x5902ACAB;
}

// 0x8015A02C
// Builds the downsample chain (1/4, 1/8, then 0x30x0x24) when the overlay record is enabled,
// emitting the 0x43 mode word before each pass and the 0x48/0x47 BP writes at the end.
void fn_8015A02C(void) {
    fn_80159258();
    fn_80157324();
    if (*fn_8015ECA0(lbl_8040D500) != 0) {
        s32 w = VideoGetWidth(&gVideo);
        s32 h = VideoGetHeight(&gVideo);
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = (u32)lbl_805FB514 | 0x43000000;
        fn_80157D6C(lbl_8040AD70 + 0x20, lbl_8040AD70, w >> 2, h >> 2);
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = (u32)lbl_805FB514 | 0x43000000;
        fn_80157D6C(lbl_8040AD70 + 0x40, lbl_8040AD70 + 0x20, w >> 3, h >> 3);
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = (u32)lbl_805FB514 | 0x43000000;
        fn_80157D6C(lbl_8040ADD0, lbl_8040AD70 + 0x40, 0x30, 0x24);
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = (u32)lbl_805FB514 | 0x43000000;
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = 0x48000100;
        GXWGFifo._u8 = 0x61;
        GXWGFifo._u32 = 0x47000100;
        fn_801588D8();
    }
}

// 0x801608C8
// Initialises 4 x 8 sixteen-byte-ish entries (stride 0x1C) with an index/link pattern, then
// resets the trailing fields at +0x36A / +0x380..+0x388.
void fn_801608C8(u8* arg0) {
    u8* p = arg0;
    s32 n = 0;
    for (s32 blk = 0; blk < 4; blk++) {
        for (s32 k = 0; k < 8; k++) {
            *(s32*)(p + k * 0x1C + 0x0) = (n + k) | 0x20;
            *(s16*)(p + k * 0x1C + 0x4) = 0;
            *(s16*)(p + k * 0x1C + 0x6) = (s16)(n + k + 1);
        }
        n += 8;
        p += 0xE0;
    }
    *(s16*)(arg0 + 0x36A) = -1;
    *(s32*)(arg0 + 0x380) = 0;
    *(s32*)(arg0 + 0x384) = 0;
    *(s32*)(arg0 + 0x388) = 0;
}

// 0x8016992C
// Releases every resource still linked to this object through its four intrusive lists
// (+0x28, +0x10, +0x4, +0x1C). A node is released only when its owner link (+8) points back
// at the object; each list head is re-read after every step.
void fn_8016992C(u8* arg0) {
    u8* node;
    u8* owner;
    u8* base;
    s32 cnt;

    while ((node = *(u8**)(arg0 + 0x28)) != NULL) {
        base = node - 4;
        owner = *(u8**)(node + 8);
        owner = owner != NULL ? owner - 0x28 : NULL;
        if (owner == arg0) {
            fn_801731C8(base + 4);
            cnt = *(s32*)(base + 0x10) - 1;
            *(s32*)(base + 0x10) = cnt;
            if (cnt <= 0 && cnt == 0) fn_801732B0(base);
        }
    }
    while ((node = *(u8**)(arg0 + 0x10)) != NULL) {
        base = node - 0x10;
        owner = *(u8**)(node + 8);
        owner = owner != NULL ? owner - 0x10 : NULL;
        if (owner == arg0) {
            fn_801731C8(base + 0x10);
            fn_80174C2C(base);
        }
    }
    while ((node = *(u8**)(arg0 + 0x4)) != NULL) {
        base = node - 0x28;
        owner = *(u8**)(node + 8);
        owner = owner != NULL ? owner - 4 : NULL;
        if (owner == arg0) {
            fn_801731C8(base + 0x28);
            cnt = *(s32*)(base + 0x24) - 1;
            *(s32*)(base + 0x24) = cnt;
            if (cnt <= 0 && cnt == 0) TextureRelease(base);
        }
    }
    while ((node = *(u8**)(arg0 + 0x1C)) != NULL) {
        base = node - 4;
        owner = *(u8**)(node + 8);
        owner = owner != NULL ? owner - 0x1C : NULL;
        if (owner == arg0) {
            fn_801731C8(base + 4);
            cnt = *(s32*)(base + 0x10) - 1;
            *(s32*)(base + 0x10) = cnt;
            if (cnt <= 0 && cnt == 0) fn_8016A2B8(base);
        }
    }
}

// 0x801732B0
// Destroys the object: detaches from its parent (+0xC), releases the per-slot resources
// (+0x64/+0x68/+0x74 pairs, +0x70 entries via fn_80174C2C), drops the +0x14 reference and
// frees the object through the heap.
void fn_801732B0(u8* arg0) {
    u8* p = *(u8**)(arg0 + 0xC);
    u8* parent = p != NULL ? p - 0x28 : NULL;
    if (parent != NULL) {
        fn_801698C0(parent, arg0);
    }
    if (*(u8**)(arg0 + 0x14) == NULL) {
        u8* q = arg0;
        for (s32 i = 0; i < *(s32*)(arg0 + 0x6C); i++) {
            fn_80185C94(*(void**)(q + 0x74));
            q += 8;
        }
        if (*(void**)(arg0 + 0x68) != NULL) {
            fn_80184268(*(void**)(arg0 + 0x68));
        }
        if (*(void**)(arg0 + 0x64) != NULL) {
            fn_80185C94(*(void**)(arg0 + 0x64));
        }
    }
    u8* q = arg0;
    for (s32 i = 0; i < *(s32*)(arg0 + 0x6C); i++) {
        fn_80174C2C(*(u8**)(q + 0x70));
        *(void**)(q + 0x70) = NULL;
        (*(HeapFreeFn*)(lbl_805FB608 + 4))(*(void**)(q + 0x74), lbl_805FB608);
        *(void**)(q + 0x74) = NULL;
        q += 8;
    }
    u8* ref = *(u8**)(arg0 + 0x14);
    if (ref != NULL) {
        s32 cnt = *(s32*)(ref + 0x10) - 1;
        *(s32*)(ref + 0x10) = cnt;
        if (cnt <= 0 && cnt == 0) fn_801732B0(ref);
    }
    *(s32*)(arg0 + 0x64) = 0;
    *(s32*)(arg0 + 0x68) = 0;
    *(s32*)(arg0 + 0x5C) = 0;
    *(s32*)(arg0 + 0x0) = 0;
    (*(HeapFreeFn*)(lbl_805FB608 + 4))(arg0, lbl_805FB608);
}

// 0x80174C2C
// Drops one reference; at zero tears the object down (textures, buffers, sub-objects, the
// type dispatch entry's release hook) and frees it. Returns 0 while references remain, 1 once
// the count reached zero or below.
s32 fn_80174C2C(u8* arg0) {
    s32 refs = *(s32*)(arg0 + 0x4) - 1;
    *(s32*)(arg0 + 0x4) = refs;
    if (refs > 0) {
        return 0;
    }
    if (refs == 0) {
        u8* link = *(u8**)(arg0 + 0x18);
        u8* owner = link != NULL ? link - 0x10 : NULL;
        if (owner != NULL) {
            fn_801697E0(owner, arg0);
        }
        u8* slot = arg0;
        for (s32 i = 0; i < 8; i++) {
            u8* tex = *(u8**)(slot + 0x1C);
            if (tex != NULL) {
                s32 cnt = *(s32*)(tex + 0x24) - 1;
                *(s32*)(tex + 0x24) = cnt;
                if (cnt <= 0 && cnt == 0) TextureRelease(tex);
                *(void**)(slot + 0x1C) = NULL;
            }
            void* buf = *(void**)(slot + 0x3C);
            if (buf != NULL) {
                (*(HeapFreeFn*)(lbl_805FB608 + 4))(buf, lbl_805FB608);
                *(void**)(slot + 0x3C) = NULL;
            }
            u8* sub = *(u8**)(slot + 0x5C);
            if (sub != NULL) {
                u8* res = *(u8**)(sub + 0x14);
                if (res != NULL) {
                    s32 cnt = *(s32*)(res + 0x10) - 1;
                    *(s32*)(res + 0x10) = cnt;
                    if (cnt <= 0 && cnt == 0) fn_8016A2B8(res);
                    *(void**)(sub + 0x14) = NULL;
                }
                *(s32*)(sub + 0x0) = 0;
                (*(HeapFreeFn*)(lbl_805FB608 + 4))(sub, lbl_805FB608);
                *(void**)(slot + 0x5C) = NULL;
            }
            slot += 4;
        }
        u8* sub = *(u8**)(arg0 + 0x7C);
        if (sub != NULL) {
            u8* res = *(u8**)(sub + 0x14);
            if (res != NULL) {
                s32 cnt = *(s32*)(res + 0x10) - 1;
                *(s32*)(res + 0x10) = cnt;
                if (cnt <= 0 && cnt == 0) fn_8016A2B8(res);
                *(void**)(sub + 0x14) = NULL;
            }
            *(s32*)(sub + 0x0) = 0;
            (*(HeapFreeFn*)(lbl_805FB608 + 4))(sub, lbl_805FB608);
            *(void**)(arg0 + 0x7C) = NULL;
        }
        (*(OverlayFn*)(lbl_80348F38 + *(u8*)(arg0 + 0xC) * 4 + 0x18))(*(void**)(arg0 + 0x8));
        *(s32*)(arg0 + 0x8) = 0;
        *(s32*)(arg0 + 0x0) = 0;
        (*(HeapFreeFn*)(lbl_805FB608 + 4))(arg0, lbl_805FB608);
    }
    return 1;
}

}  // extern "C"
