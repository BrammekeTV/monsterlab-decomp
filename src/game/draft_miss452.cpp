// Small functions at 0x800BE0B8, 0x800BF2AC, 0x800BF710, 0x800BF7F0, 0x801189C4, 0x80118B1C,
// 0x80120990, 0x80187704, 0x801A9E94, 0x801D87B4, 0x801FF590, 0x8002C444, 0x8003FA08, 0x8007CDBC.
#include "types.h"
#include "game/part.h"

extern "C++" void* gEventMgr;           // 0x805FB6C8
extern u8* lbl_805FB830;                // 0x805FB830 (list of queued beam records)
extern s32 lbl_805FBAE0;                // 0x805FBAE0 (audio busy flag)
extern u8 lbl_80524E28[];               // 0x80524E28 (sound table, 0x60 entries of 0x58 bytes)
extern u8 lbl_8047C7D8[];               // 0x8047C7D8
extern const char* lbl_805F8F10;        // 0x805F8F10 (name prefix string pointer)
extern f32 lbl_805FCB80;                // 0.0f
extern f32 lbl_805FE2C8;                // float constant (zero)
extern f32 lbl_805FE308;                // float constant
extern f32 lbl_805FE33C;                // float constant (lower clamp)
extern f32 lbl_805FE340;                // float constant (upper clamp)

extern "C" {
void*  ChoiceDestroy(void* c, s32 del);
void   ChoiceInit(u8* c);
void   EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);
void*  MemAllocDefault(u32 size);
int    SysSprintf(char* buf, const char* fmt, ...);
u8*    fn_801094F4(char* name, s32 flags);
s32    fn_801B41F0(u8* a, u8* b);
void   fn_8003FC00(u8* inner);
void   fn_8003FC6C(u8* inner, s32 v);
void   fn_8003FF30(u8* inner);
u32    fn_8003FFA4(u8* inner, u8* choice);
s32    fn_8003FFD0(u8* inner);
void   fn_80040028(u8* inner);
void   fn_8007D004(s32 dst, u8* src, s32 cur, s32 delta);
void   fn_8020F46C(u8* p, u8* choice);
f64    fn_8033ED6C(f64 x);
u8*    fn_801EB7A4(void* p, s32 a4, s32 a5, s32 a6, s32 a7);
void*  fn_801EB94C(void* p);
void   fn_801FF2A8(u8* audio, s32 handle);
}

// Shared walk used by 0x800BE0B8, 0x800BF2AC and 0x800BF7F0: self+0x8C -> node list head
// (+0xC) -> node (+0x24) -> record. Returns the record, or NULL when a link is missing or
// the node type word (+0x28 / +0x10) is not 1.
static u8* ResolveSlotRecord(u8* self) {
    u8* obj = *(u8**)(self + 0x8C);
    if (obj == NULL) return NULL;
    if (*(u32*)(obj + 0x10) != 1) return NULL;
    u8* node = *(u8**)(*(u8**)(obj + 0xC));
    if (node == NULL) return NULL;
    if (*(u32*)(node + 0x28) != 1) return NULL;
    return *(u8**)(*(u8**)(node + 0x24));
}

extern "C" {

// 0x800BE0B8
// Returns the slot record of self when it is not yet armed (+0xC == 0); clears its two
// float fields when self's flag byte at +0x264 is set.
u8* fn_800BE0B8(u8* p) {
    u8* rec = ResolveSlotRecord(p);
    if (rec == NULL) return NULL;
    if (*(s32*)(rec + 0xC) != 0) return NULL;
    if (*(u8*)(p + 0x264) != 0) {
        *(f32*)(rec + 0x24) = lbl_805FE2C8;
        *(f32*)(rec + 0x18) = lbl_805FE2C8;
    }
    return rec;
}

// 0x800BF2AC
// Shifts the slot record's current vector (+0x14..) into its previous slot (+0x20..) and
// loads a new vector from self+0x4; clears the +0x18/+0x24 fields unless self+0x290 == 5.
u8* fn_800BF2AC(u8* p) {
    u8* rec = ResolveSlotRecord(p);
    if (rec == NULL) return NULL;
    if (*(s32*)(rec + 0xC) != 0) return NULL;
    *(f32*)(rec + 0x20) = *(f32*)(rec + 0x14);
    *(f32*)(rec + 0x24) = *(f32*)(rec + 0x18);
    *(f32*)(rec + 0x28) = *(f32*)(rec + 0x1C);
    u8* src = *(u8**)(p + 0x4);
    *(f32*)(rec + 0x14) = *(f32*)(src + 0x0);
    *(f32*)(rec + 0x18) = *(f32*)(src + 0x4);
    *(f32*)(rec + 0x1C) = *(f32*)(src + 0x8);
    if (*(s32*)(p + 0x290) != 5) {
        *(f32*)(rec + 0x24) = lbl_805FE308;
        *(f32*)(rec + 0x18) = lbl_805FE308;
    }
    return rec;
}

// 0x800BF710
// Adds delta to the value at +0x264, clamps it to [lbl_805FE33C, lbl_805FE340], and fires
// the two change events (below the lower bound, and above the stored maximum at +0x268).
void fn_800BF710(u8* p, f32 delta) {
    if (delta < lbl_805FE33C) {
        EventSend(gEventMgr, p, NULL, 0x80E3B472u, 0, 0, 1, 0);
        *(f32*)(p + 0x268) = *(f32*)(p + 0x264);
    }
    f32 v = delta + *(f32*)(p + 0x264);
    f32 clamped;
    if (v < lbl_805FE33C) {
        clamped = lbl_805FE33C;
    } else if (v > lbl_805FE340) {
        clamped = lbl_805FE340;
    } else {
        clamped = v;
    }
    *(f32*)(p + 0x264) = clamped;
    if (clamped > *(f32*)(p + 0x268)) {
        EventSend(gEventMgr, p, NULL, 0xAE21DFE0u, 0, 0, 1, 0);
    }
}

// 0x800BF7F0
// Zeroes the slot record's +0x24 and +0x18 float fields and returns the record.
u8* fn_800BF7F0(u8* p) {
    u8* rec = ResolveSlotRecord(p);
    if (rec == NULL) return NULL;
    if (*(s32*)(rec + 0xC) != 0) return NULL;
    *(f32*)(rec + 0x24) = lbl_805FE33C;
    *(f32*)(rec + 0x18) = lbl_805FE33C;
    return rec;
}

// 0x801189C4
// Inventory icon texture for table entry idx (0x10-byte rows at db+0x4F00). Falls back to the
// placeholder icon when the named one does not exist.
u8* fn_801189C4(s32 idx) {
    char buf[0x100];
    u8* rec = *(u8**)(gGameDb->data + 0x4F00) + idx * 0x10;
    SysSprintf(buf, "%sinventory_icon_%03d", lbl_805F8F10, (s32)*(s8*)(rec + 0xA));
    u8* tex = fn_801094F4(buf, 0);
    if (tex == NULL) {
        SysSprintf(buf, "%sinventory_icon_placeholder", lbl_805F8F10);
        tex = fn_801094F4(buf, 0);
    }
    return tex;
}

// 0x80118B1C
// Secondary inventory icon texture for table entry idx (0x18-byte rows at db+0x5328).
u8* fn_80118B1C(s32 idx) {
    char buf[0x100];
    u8* rec = *(u8**)(gGameDb->data + 0x5328) + idx * 0x18;
    SysSprintf(buf, "%sinventory_icon_sec_%03d", lbl_805F8F10, (s32)*(s8*)(rec + 0x12));
    u8* tex = fn_801094F4(buf, 0);
    if (tex == NULL) {
        SysSprintf(buf, "%sinventory_icon_placeholder", lbl_805F8F10);
        tex = fn_801094F4(buf, 0);
    }
    return tex;
}

// 0x80120990
// Index of the first entry in the 0x18-byte table at db+0x5328 whose word +0xC == a, word +0x8 == b
// and signed byte +0x13 == c. Returns -1 when none matches.
s32 fn_80120990(s32 a, s32 b, s32 c) {
    s32 count = *(s32*)(gGameDb->data + 0x532C);
    for (s32 i = 0; i < count; i++) {
        u8* e = *(u8**)(gGameDb->data + 0x5328) + i * 0x18;
        if (*(s32*)(e + 0xC) == a && *(s32*)(e + 0x8) == b && *(s8*)(e + 0x13) == c) {
            return i;
        }
    }
    return -1;
}

// 0x80187704
// Looks up key (word at table[0x10][idx] + 0x8C) among entries 1..count-1 of the 0xC-byte entry
// array at table+0x4. If absent, appends it at the first empty slot and shifts the two s16
// history fields (+0x14/+0x18) into +0x16/+0x1A. Returns the slot index, or 0 when full.
s32 fn_80187704(u8* table, s32 idx) {
    s32 count = *(s32*)(table + 0x0);
    u32 key = *(u32*)(*(u8**)(table + 0x10) + idx * 4 + 0x8C);
    u8* entries = *(u8**)(table + 0x4);
    s32 slot = 1;
    if (count > 1) {
        for (; slot < count; slot++) {
            u32 k = *(u32*)(entries + slot * 0xC);
            if (k == key) return slot;
            if (k == 0) break;
        }
    }
    if (slot >= count) return 0;
    *(u32*)(entries + slot * 0xC) = key;
    s16 prevA = *(s16*)(table + 0x14);
    s16 prevB = *(s16*)(table + 0x18);
    *(s16*)(table + 0x16) = prevA;
    *(s16*)(table + 0x14) = (s16)idx;
    *(s16*)(table + 0x1A) = prevB;
    *(s16*)(table + 0x18) = (s16)slot;
    return slot;
}

// 0x801A9E94
// Stores three floats as the vector at obj+0x140.
s32 fn_801A9E94(u8* obj, f32 a, f32 b, f32 c) {
    *(f32*)(obj + 0x140) = a;
    *(f32*)(obj + 0x144) = b;
    *(f32*)(obj + 0x148) = c;
    return 0;
}

// 0x801D87B4
// Queues one beam record (from, dir, colour, a, len, radius) on lbl_805FB830, creating the
// list on first use. The view argument is not read by the function.
void fn_801D87B4(u8* view, f32* from, f32* dir, u32 colour, s32 a, f32 len, f32 radius) {
    if (lbl_805FB830 == NULL) {
        lbl_805FB830 = (u8*)MemAllocDefault(0x2C);
        if (lbl_805FB830 != NULL) {
            fn_801EB7A4(lbl_805FB830, 0x28, 4, 8, 1);
        }
    }
    u8* p = (u8*)fn_801EB94C(lbl_805FB830);
    if (p != NULL) {
        *(f32*)(p + 0x0) = from[0];
        *(f32*)(p + 0x4) = from[1];
        *(f32*)(p + 0x8) = from[2];
        *(f32*)(p + 0xC) = dir[0];
        *(f32*)(p + 0x10) = dir[1];
        *(f32*)(p + 0x14) = dir[2];
        *(f32*)(p + 0x18) = len;
        *(f32*)(p + 0x1C) = radius;
        *(u32*)(p + 0x20) = colour;
        *(u8*)(p + 0x24) = (u8)a;
    }
}

// 0x801FF590
// Releases every sound voice in the sound table whose record zone (signed halfword +0xE) equals
// zone. lbl_805FBAE0 is held at 1 while it runs.
void fn_801FF590(u8* audio, s32 zone) {
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    u8* entry = lbl_80524E28;
    for (s32 i = 0; i < 0x60; i++, entry += 0x58) {
        if (*(s32*)(entry + 0x10) != 0) {
            u8* rec = *(u8**)(entry + 0xC);
            if (rec != NULL && zone == *(s16*)(rec + 0xE)) {
                fn_801FF2A8(audio, *(s32*)entry);
            }
        }
    }
    lbl_805FBAE0 = saved;
}

// 0x8002C444
// Advances the timed value at rec: elapsed (+0x2C) += dt. While elapsed <= duration (+0x28, and
// duration > 0) the current vector (+0x1C..+0x24) lerps from +0x4..+0xC to +0x10..+0x18; otherwise
// it snaps to the target. The result is copied to out.
void fn_8002C444(u8* out, u8* rec, f32 dt) {
    f32 elapsed = *(f32*)(rec + 0x2C) + dt;
    *(f32*)(rec + 0x2C) = elapsed;
    f32 duration = *(f32*)(rec + 0x28);
    if (duration > lbl_805FCB80 && elapsed <= duration) {
        f32 t = elapsed / duration;
        f32 w = (f32)(1.0 - (f64)t);
        *(f32*)(rec + 0x1C) = *(f32*)(rec + 0x4) * w + *(f32*)(rec + 0x10) * t;
        *(f32*)(rec + 0x20) = *(f32*)(rec + 0x8) * w + *(f32*)(rec + 0x14) * t;
        *(f32*)(rec + 0x24) = *(f32*)(rec + 0xC) * w + *(f32*)(rec + 0x18) * t;
    } else {
        *(f32*)(rec + 0x1C) = *(f32*)(rec + 0x10);
        *(f32*)(rec + 0x20) = *(f32*)(rec + 0x14);
        *(f32*)(rec + 0x24) = *(f32*)(rec + 0x18);
    }
    *(f32*)(out + 0x0) = *(f32*)(rec + 0x1C);
    *(f32*)(out + 0x4) = *(f32*)(rec + 0x20);
    *(f32*)(out + 0x8) = *(f32*)(rec + 0x24);
}

// 0x8003FA08
// Refreshes the object's choice state: fills the inner block when empty, recomputes the flag at
// obj+0x50 from the global at lbl_8047C7D8, then runs the choice step (apply and finish, or only
// finish) depending on the result.
void fn_8003FA08(u8* obj) {
    u8 choice[12];
    u8* inner = *(u8**)(obj + 0x4C);
    if (*(s32*)(inner + 0x84) == 0) {
        fn_8003FC00(inner);
    }
    u8* inner2 = *(u8**)(obj + 0x4C);
    s32 cur = *(s32*)(inner2 + 0x84);
    u8 match = (fn_801B41F0(lbl_8047C7D8, inner2) == cur);
    u8* inner3 = *(u8**)(obj + 0x4C);
    if (*(u8*)(inner3 + 0x98) == 0 || (match == 1 && *(u8*)(obj + 0x50) == 0)) {
        fn_8003FC6C(inner3, *(s32*)(obj + 0x8));
    }
    *(u8*)(obj + 0x50) = match;
    fn_8003FF30(*(u8**)(obj + 0x4C));
    ChoiceInit(choice);
    if (fn_8003FFA4(*(u8**)(obj + 0x4C), choice) == 1U) {
        fn_8020F46C(obj, choice);
        fn_80040028(*(u8**)(obj + 0x4C));
        ChoiceDestroy(choice, -1);
        return;
    }
    fn_8003FFD0(*(u8**)(obj + 0x4C));
    ChoiceDestroy(choice, -1);
}

// 0x8007CDBC
// Streams the sound/anim data block at self+0x24 into dst. t is the current time: before the
// start time (+0x30) returns 0; after the end time (+0x28) copies the whole block once and
// returns 1; in between copies the fraction of it that the time reaches and returns 0.
s32 fn_8007CDBC(u8* self, s32 dst, f32 t) {
    f32 start = *(f32*)(self + 0x30);
    if (start < t) return 0;
    f32 end = *(f32*)(self + 0x28);
    u8* data = *(u8**)(self + 0x24);
    u16 total = *(u16*)(data + 0x6);
    u8* bytes = data + 4;
    if (end > t) {
        s32 cur = *(s32*)(self + 0x38);
        if (cur < (s32)total) {
            fn_8007D004(dst, bytes, cur, (s32)total - cur);
            *(s32*)(self + 0x38) = (s32)total;
            return 1;
        }
        return 0;
    }
    s32 cur = *(s32*)(self + 0x38);
    f32 frac = (t - start) / (end - start);
    s32 newCount = (s32)(f32)fn_8033ED6C((f64)(frac * (f32)total));
    fn_8007D004(dst, bytes, cur, newCount - cur);
    *(s32*)(self + 0x38) = newCount;
    return 0;
}

}  // extern "C"
