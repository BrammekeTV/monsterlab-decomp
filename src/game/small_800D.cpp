// Small functions in 0x800Dxxxx (object refs, paths).
#include "types.h"
#include "game/character.h"

extern "C" {
void  operator_delete(void* p);                                        // 0x801D426C
void  fn_800DEC78(u8* p, s32 a);                                       // 0x800DEC78
u8*  fn_800DE5E4(u8* p, s32 del);                                     // 0x800DE5E4
void  fn_8006FA44(u8* a3, s32 a4, s32 a5, s32 a6, s32 a7, s32 a8);     // 0x8006FA44
void  GameFlowSignal(s32 id, s32 arg1, s32 arg2);                      // 0x8000C7F4
void  fn_800D14F0(u8* p);                                              // 0x800D14F0
void  fn_800D0DC0(u8* p);                                              // 0x800D0DC0
void  fn_800D5008(u8* p);                                              // 0x800D5008
void  fn_800D50C4(u8* p);                                              // 0x800D50C4
void  fn_801C83AC(u8* mgr, u8* item);                                  // 0x801C83AC
void  fn_800D5110(u8* holder, u8* obj, u8* data);                      // 0x800D5110
}

extern u8  lbl_8035F850[];   // 0x8035F850 (vtable)
extern u8  lbl_803C4298[];   // 0x803C4298 (level info block)
extern u8  lbl_803C6F10[];   // 0x803C6F10 (block passed to fn_8006FA44)
extern u8  lbl_803C6F24[];   // 0x803C6F24 (three floats)
extern u8  lbl_803C6F40[];   // 0x803C6F40 (block passed to fn_8006FA44)
extern u8  lbl_803C6F54[];   // 0x803C6F54 (three floats)
extern u8  lbl_8047C760[];   // 0x8047C760 (effect manager)
extern f32 lbl_805FE558;     // 0x805FE558
extern f32 lbl_805FE598;     // 0x805FE598
extern f32 lbl_805FE5B0;     // 0x805FE5B0
extern f32 lbl_805FE5B4;     // 0x805FE5B4
extern f32 lbl_805FE5B8;     // 0x805FE5B8
extern f32 lbl_805FE5E4;     // 0x805FE5E4
extern f32 lbl_805FE5E8;     // 0x805FE5E8
extern f32 lbl_805FE5F4;     // 0x805FE5F4
extern f32 lbl_805FE618;     // 0x805FE618

extern "C" {

// 0x800D0068
// Sets two float slots of the block at lbl_803C6F24, then calls the table setup with a fixed key.
void fn_800D0068(void) {
    f32 a = lbl_805FE558;
    f32 b = lbl_805FE598;
    *(f32*)(lbl_803C6F24 + 0x0) = a;
    *(f32*)(lbl_803C6F24 + 0x4) = b;
    *(f32*)(lbl_803C6F24 + 0x8) = a;
    fn_8006FA44(lbl_803C6F10, (s32)0xE0349458, 1, (s32)0x8A1B3F32, (s32)0x85AB5092, (s32)0x95F4DBAB);
}

// 0x800D00B4
// Constructor for a record: runs the base setup with type 0x12, then sets the vtable.
u8* fn_800D00B4(u8* p) {
    fn_800DEC78(p, 0x12);
    *(void**)(p + 0x0) = lbl_8035F850;
    return p;
}

// 0x800D00F4
// Deleting destructor: clears the flag at +0xFC of the current level info record, runs the
// base dtor on the subobject at +0xC, then frees the record if del > 0.
u8* fn_800D00F4(u8* p, s32 del) {
    if (p) {
        *(void**)(p + 0x0) = lbl_8035F850;
        u8* lvl = *(u8**)(lbl_803C4298 + 0x4);
        *(u8*)(lvl + 0xFC) = 0;
        fn_800DE5E4(p + 0xC, -1);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800D0174
// Zeroes the six-float block at +0x58..+0x6C, and sets +0x70 to the second constant.
void fn_800D0174(u8* p) {
    f32 a = lbl_805FE5B0;
    f32 b = lbl_805FE5B4;
    *(f32*)(p + 0x58) = a;
    *(f32*)(p + 0x5C) = a;
    *(f32*)(p + 0x60) = a;
    *(f32*)(p + 0x64) = a;
    *(f32*)(p + 0x68) = a;
    *(f32*)(p + 0x6C) = a;
    *(f32*)(p + 0x70) = b;
}

// 0x800D1C8C
// Adds delta to the accumulator at +0x17C; when it stays below the threshold, signals game flow 0x88C6E941.
void fn_800D1C8C(u8* p, f32 delta) {
    f32 sum = *(f32*)(p + 0x17C) + delta;
    *(f32*)(p + 0x17C) = sum;
    if (sum >= lbl_805FE5E4) return;
    GameFlowSignal((s32)0x88C6E941, 0, 0);
}

// 0x800D1CD0
// Same as fn_800D1C8C with a different threshold constant.
void fn_800D1CD0(u8* p, f32 delta) {
    f32 sum = *(f32*)(p + 0x17C) + delta;
    *(f32*)(p + 0x17C) = sum;
    if (sum >= lbl_805FE5E8) return;
    GameFlowSignal((s32)0x88C6E941, 0, 0);
}

// 0x800D1D08
// Runs two per-object update steps in order.
void fn_800D1D08(u8* p) {
    fn_800D14F0(p);
    fn_800D0DC0(p);
}

// 0x800D33E0
// Sets three floats at lbl_803C6F54 ({a, b, a}) and tail-calls the table setup with a fixed key.
void fn_800D33E0(void) {
    f32 a = lbl_805FE5B8;
    f32 b = lbl_805FE5F4;
    *(f32*)(lbl_803C6F54 + 0x0) = a;
    *(f32*)(lbl_803C6F54 + 0x4) = b;
    *(f32*)(lbl_803C6F54 + 0x8) = a;
    fn_8006FA44(lbl_803C6F40, 0, 0, (s32)0x9A1B7746, (s32)0x85AB5092, (s32)0x714859F7);
}

// 0x800D393C
// Selects entry idx of the table at +0x13284 (entry size 0x1AC, count at +0x2CD8; out-of-range
// requests use the base entry) and binds the object handle at +0x404 to it. Returns p.
u8* fn_800D393C(u8* p, s32 idx) {
    s32 count = *(s32*)(p + 0x2CD8);
    s32 sel;
    if (idx < 0) {
        sel = 0;
    } else {
        sel = (idx > count) ? count : idx;
    }
    u8* entry = (idx == sel) ? p + 0x13284 + idx * 0x1AC : p + 0x13284;
    fn_800D5110(p, p + 0x404, entry);
    return p;
}

// 0x800D41F4
// Appends one entry at index count (up to 0xFF): initialises it via fn_800D5008, then bumps the count at +0x13280.
void fn_800D41F4(u8* p) {
    s32 n = *(s32*)(p + 0x13280);
    if (n < 0xFF) {
        fn_800D5008(p + n * 0xDC + 0x575C);
        *(s32*)(p + 0x13280) += 1;
    }
}

// 0x800D46B0
// Clears the word at +0xA0 of every table entry, from the last entry down to the first.
void fn_800D46B0(u8* p) {
    s32 count = *(s32*)(p + 0x2CD8);
    for (s32 i = count - 1; i >= 0; i--) {
        *(s32*)(p + 0x13284 + i * 0x1AC + 0xA0) = 0;
    }
}

// 0x800D4B74
// Deleting destructor for a plain block: frees it if del > 0. Returns p.
u8* fn_800D4B74(u8* p, s32 del) {
    if (p && del > 0) operator_delete(p);
    return p;
}

// 0x800D4BB4
// Clears the word at the subobject +0x24. Returns p.
u8* fn_800D4BB4(u8* p) {
    fn_800D50C4(p + 0x24);
    return p;
}

// 0x800D50D0
// Base destructor for an object handle: nothing to release, frees the block if flags > 0. Returns ref.
u8* ObjRefDtor(u8* ref, s32 flags) {
    if (ref && flags > 0) operator_delete(ref);
    return ref;
}

// 0x800D4BE8
// Deleting destructor: releases the object handle at +0x24, then frees the record if del > 0.
u8* fn_800D4BE8(u8* p, s32 del) {
    if (p) {
        ObjRefDtor(p + 0x24, -1);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800D5110
// Binds an object record to the holder (stores obj at holder+0), sets its default fields, stores the
// data block at obj+0x14 and copies three floats from data+0x34..0x3C to obj+0x18..0x20.
void fn_800D5110(u8* holder, u8* obj, u8* data) {
    *(u8**)(holder + 0x0) = obj;
    *(f32*)(obj + 0x0) = lbl_805FE618;
    *(u8*)(obj + 0x4) = 1;
    *(s32*)(obj + 0x8) = 0;
    *(s32*)(obj + 0xC) = -1;
    *(s32*)(obj + 0x10) = 0;
    *(u8**)(obj + 0x14) = data;
    *(f32*)(obj + 0x18) = *(f32*)(data + 0x34);
    *(f32*)(obj + 0x1C) = *(f32*)(data + 0x38);
    *(f32*)(obj + 0x20) = *(f32*)(data + 0x3C);
}

// 0x800D59A4
// Returns the word at index idx (low byte, times 4) of the data block of the record held by p, or 0 if none.
void* fn_800D59A4(u8* p, s32 idx) {
    u8* rec = *(u8**)p;
    u8* data = *(u8**)(rec + 0x14);
    if (data == 0) return 0;
    if (idx >= 3) return 0;
    u32 off = ((u32)idx & 0xFFu) << 2;
    return *(void**)(data + 0x44 + off);
}

// 0x800D5BD4
// If the record's current item at data+0x50 has a matching key (first word of its sub-record),
// releases it through the effect manager and clears the slot.
void fn_800D5BD4(u8* p, u32 key) {
    u8* rec = *(u8**)p;
    u8* data = *(u8**)(rec + 0x14);
    if (data == 0) return;
    u8* item = *(u8**)(data + 0x50);
    if (item == 0) return;
    u8* sub = *(u8**)(item + 0x4);
    if (key != *(u32*)sub) return;
    fn_801C83AC(lbl_8047C760, item);
    rec = *(u8**)p;
    data = *(u8**)(rec + 0x14);
    *(void**)(data + 0x50) = 0;
}

// 0x800D5D5C
// Starts (on != 0) or stops the path: state 0 -> 1 when starting, 1 -> 0 when stopping.
void PathSetMoving(PathFollower* p, BOOL on) {
    PathState* st = p->s;
    if (on == 0) {
        if (st->state == 1) st->state = 0;
        return;
    }
    if (st->state != 0) return;
    st->state = 1;
}

// 0x800D6998
// True if the given exit of the path's node table is blocked: the linked node is either
// flagged at +0x64 or has type 0x10 at +0x40.
BOOL PathExitBlocked(PathFollower* p, s32 exit) {
    u8* st = (u8*)p->s;
    u8* table = *(u8**)(st + 0x14);
    if (table == 0) return 0;
    u8* node = 0;
    u8* entry = *(u8**)(table + 0x4 + exit * 4);
    if (entry != 0) {
        u8* link = *(u8**)(entry + 0x4);
        if (table == link) {
            node = *(u8**)(entry + 0x8);
        } else {
            node = link;
        }
    }
    if (node == 0) return 0;
    if (*(u8*)(node + 0x64) != 0) return 1;
    if (*(s32*)(node + 0x40) != 0x10) return 0;
    return 1;
}

}
