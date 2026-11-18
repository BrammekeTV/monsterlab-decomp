// Small functions in 0x8004xxxx (0x800498D0 - 0x8004E370): object destructors and constructors,
// per-character limb and blend updates, a few character state queries, and part/zone lookups.
#include "types.h"
#include "game/part.h"

// Copied from their definitions (C++ linkage, so declared outside extern "C").
extern "C" f32 fn_8018BE30(void* obj, f32 a);                               // 0x8018BE30

extern "C" {
void  operator_delete(void* p);                                 // 0x801D426C
void  fn_8020F23C(u8* p, s32 del);                              // 0x8020F23C
void  fn_8020F30C(u8* p);                                       // 0x8020F30C
void  fn_8020F31C(u8* p, s32 del);                              // 0x8020F31C
void  fn_8004B604(u8* p);                                       // 0x8004B604
void  fn_8004B754(u8* p, s32 del);                              // 0x8004B754
void  fn_8020EB44(u8* p, s32 del);                              // 0x8020EB44
void  fn_8005CAE8(u8* p, s32 del);                              // 0x8005CAE8
void  fn_80053558(u8* p);                                       // 0x80053558
void  fn_8020E59C(u8* p, s32 del);                              // 0x8020E59C
void  fn_8005E11C(u8* p);                                       // 0x8005E11C
void  fn_8004C718(u8* p, s32 idx, void* elem);                  // 0x8004C718
void  fn_8020ECB0(u8* p, s32 b);                                // 0x8020ECB0
void  LimbStatusInit(u8** field, PartInstance** part);          // 0x80214A58
s32   PartGetCurrentHealth(u8* p);                              // 0x8021EF68
BOOL  CharacterOnContact(u8* c, u8* ct);                        // 0x800575E8

extern u8  lbl_80358D18[];   // 0x80358D18 (vtable)
extern u8  lbl_80358D80[];   // 0x80358D80 (vtable)
extern u8  gGame[];          // 0x803B53A0
extern s32 gZoneResourceSize;// 0x805FB94C
extern u8  gWorldState[];    // 0x803E6D00
extern f32 lbl_805FD228;     // 0x805FD228
extern f32 lbl_805FD240;     // 0x805FD240
extern f32 lbl_805FD244;     // 0x805FD244
extern f32 lbl_805FD25C;     // 0x805FD25C
extern f32 lbl_805FD268;     // 0x805FD268
}

// Virtual slot call returning a pointer: (*(Fn*)(*(u8**)obj + OFF))(obj).
typedef void* (*VFnPtr)(void*);

extern "C" {

// 0x800498D0
// Deleting destructor: base dtor on the subobject (fn_8020F23C with 0), then free if del > 0.
u8* fn_800498D0(u8* p, s32 del) {
    if (p) {
        fn_8020F23C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x8004AC68
// Constructor: runs the base constructor, then sets the vtable, two floats, a -1 id and clears
// the four flag bytes.
void fn_8004AC68(u8* p) {
    fn_8020F30C(p);
    *(f32*)(p + 0x8)  = lbl_805FD228;
    *(void**)p        = lbl_80358D18;
    *(f32*)(p + 0xc)  = lbl_805FD228;
    *(s32*)(p + 0x10) = -1;
    *(u8*)(p + 0x14)  = 0;
    *(u8*)(p + 0x4)   = 0;
    *(u8*)(p + 0x5)   = 0;
    *(u8*)(p + 0x6)   = 0;
}

// 0x8004ACCC
// Deleting destructor: base dtor (fn_8020F31C with 0), then free if del > 0.
u8* fn_8004ACCC(u8* p, s32 del) {
    if (p) {
        fn_8020F31C(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x8004B498
// Constructor: runs fn_8004B604, sets the vtable and the secondary vtable at +0xC48, and if an
// attached object is present sets its 0x4000 flag and byte 0xE at +0x34.
u8* fn_8004B498(u8* p) {
    fn_8004B604(p);
    u8* obj = *(u8**)(p + 0x4);
    *(void**)p         = lbl_80358D80;
    *(void**)(p + 0xc48) = lbl_80358D80 + 0xdc;
    if (obj) {
        *(u32*)(obj + 0x1c) |= 0x4000;
        *(u8*)(obj + 0x34) = 0xE;
    }
    return p;
}

// 0x8004B500
// Deleting destructor: fn_8004B754 with 0, then free if del > 0.
u8* fn_8004B500(u8* p, s32 del) {
    if (p) {
        fn_8004B754(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x8004B6E8
// Deleting destructor: releases the embedded object at +0x898 (fn_8020EB44 with -1) and runs
// fn_8005CAE8 on this record, then frees it if del > 0.
u8* fn_8004B6E8(u8* p, s32 del) {
    if (p) {
        fn_8020EB44(p + 0x898, -1);
        fn_8005CAE8(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x8004B82C
// Releases the object at +0xC4C (if any, with fn_8020E59C(obj, 1)) and clears the pointer,
// then runs fn_80053558 on this record.
void fn_8004B82C(u8* p) {
    u8* obj = *(u8**)(p + 0xc4c);
    if (obj) {
        fn_8020E59C(obj, 1);
        *(void**)(p + 0xc4c) = NULL;
    }
    fn_80053558(p);
}

// 0x8004BAFC
// Calls the virtual slot 0xC on this record; if it returns something, clears bit 0x2 of the
// returned object's +8 field. Then runs fn_8005E11C on this record.
void fn_8004BAFC(u8* p) {
    if ((*(VFnPtr*)(*(u8**)p + 0xc))(p)) {
        u8* o = (u8*)(*(VFnPtr*)(*(u8**)p + 0xc))(p);
        *(u32*)(o + 0x8) &= ~0x2u;
    }
    fn_8005E11C(p);
}

// 0x8004BD68
// For each of the five object slots at +0xC50..+0xC60 (highest first): calls the object's
// slot-0xC function and sets flag 0x80 on the returned object's +8 field.
void fn_8004BD68(u8* p) {
    for (s32 i = 4; i >= 0; i--) {
        u8* obj = *(u8**)(p + 0xc50 + i * 4);
        if (obj) {
            u8* o = (u8*)(*(VFnPtr*)(*(u8**)obj + 0xc))(obj);
            *(u32*)(o + 0x8) |= 0x80;
        }
    }
}

// 0x8004BDD0
// Same walk as fn_8004BD68 over the five object slots, clearing flag 0x80 instead.
void fn_8004BDD0(u8* p) {
    for (s32 i = 4; i >= 0; i--) {
        u8* obj = *(u8**)(p + 0xc50 + i * 4);
        if (obj) {
            u8* o = (u8*)(*(VFnPtr*)(*(u8**)obj + 0xc))(obj);
            *(u32*)(o + 0x8) &= ~0x80u;
        }
    }
}

// 0x8004CAB4
// Stores elem into the table entry idx (table at +0xC4C), runs fn_8004C718 on that entry, and
// if elem is set, rebuilds the entry's limb status from its part.
void fn_8004CAB4(u8* p, s32 idx, void* elem) {
    u8* table = *(u8**)(p + 0xc4c);
    u8* obj = *(u8**)(table + idx * 4);
    fn_8004C718(p, idx, obj);
    *(void**)obj = elem;
    if (elem != NULL) LimbStatusInit((u8**)(obj + 0x28), (PartInstance**)obj);
}

// 0x8004D170
// Returns the flag byte at +0xC80 when the record has no pending value at +0xC64 and the global
// timer at gGame+0x7CC has not passed the threshold; otherwise returns TRUE.
BOOL fn_8004D170(u8* chr) {
    if (*(s32*)(chr + 0xc64) == 0 && *(f32*)(gGame + 0x7cc) <= lbl_805FD240) {
        return *(u8*)(chr + 0xc80);
    }
    return TRUE;
}

// 0x8004D248
// Sets the blend value (float at +0xB0) of each of the five object slots at +0xC50..+0xC60 to t.
void CharacterSetBlend(u8* c, f32 t) {
    for (s32 i = 0; i < 5; i++) {
        u8* obj = *(u8**)(c + 0xc50 + i * 4);
        if (obj) {
            u8* o = (u8*)(*(VFnPtr*)(*(u8**)obj + 0x10))(obj);
            *(f32*)(o + 0xb0) = t;
        }
    }
}

// 0x8004D39C
// Clears the byte at +0x28C of each of the five object slots at +0xC50..+0xC60.
void fn_8004D39C(u8* p) {
    for (s32 i = 0; i < 5; i++) {
        u8* obj = *(u8**)(p + 0xc50 + i * 4);
        if (obj) *(u8*)(obj + 0x28c) = 0;
    }
}

// 0x8004D5BC
// Returns TRUE when the value fn_8018BE30 gives for the object at +0xC60 equals the constant
// lbl_805FD244 (the animation/attack has finished).
BOOL CharacterAttackDone(u8* c) {
    u8* obj = *(u8**)(c + 0xc60);
    u8* shot = (u8*)(*(VFnPtr*)(*(u8**)obj + 0x10))(obj);
    f32 v = fn_8018BE30(shot, lbl_805FD240);
    return lbl_805FD244 == v;
}

// 0x8004D700
// Same test as CharacterAttackDone: TRUE when the value from fn_8018BE30 equals lbl_805FD244.
BOOL CharacterAnimationDone(u8* c) {
    u8* obj = *(u8**)(c + 0xc60);
    u8* shot = (u8*)(*(VFnPtr*)(*(u8**)obj + 0x10))(obj);
    f32 v = fn_8018BE30(shot, lbl_805FD240);
    return lbl_805FD244 == v;
}

// 0x8004D760
// Picks the value at +0x9F4 from the object's flag 0x4000 (one of two constants). With b set,
// runs fn_8020ECB0 on the embedded object at +0x898, flags +0xC84 and reads the current health
// of the part at +0x8AC; with b clear, clears the flag at +0xC84.
void fn_8004D760(u8* p, s32 b) {
    u8* obj = *(u8**)(p + 0x4);
    if (*(u32*)(obj + 0x1c) & 0x4000) {
        *(f32*)(p + 0x9f4) = lbl_805FD268;
    } else {
        *(f32*)(p + 0x9f4) = lbl_805FD25C;
    }
    if (b != 0) {
        fn_8020ECB0(p + 0x898, b);
        u8* part = *(u8**)(p + 0x8ac);
        *(u8*)(p + 0xc84) = 1;
        if (part) PartGetCurrentHealth(part);
    } else {
        *(u8*)(p + 0xc84) = 0;
    }
}

// 0x8004DCEC
// Refreshes the cached zone value for slot idx (at +0xC6C) from the zone table: if the stored
// zone number is 1..999, copies the word at +0x10C of that zone's record in the world zone table.
void fn_8004DCEC(u8* p, s32 idx) {
    u32 zone = *(u32*)(p + 0xc6c + idx * 4);
    if (zone == 0 || zone >= 0x3e8) return;
    u8* world = *(u8**)gWorldState;
    u8* table = *(u8**)(world + 0x1a4);
    *(u32*)(p + 0xc6c + idx * 4) = *(u32*)(table + (zone - 1) * gZoneResourceSize + 0x10c);
}

// 0x8004E300
// Returns a value from the part definition of the part at slot idx (+0x89C), or -1 if the slot
// is empty. The definition is read with PartDbGetDef into a local copy; the result is its
// word at +0x34.
s32 fn_8004E300(u8* p, s32 idx) {
    u8* entry = *(u8**)(p + 0x89c + idx * 4);
    if (!entry) return -1;
    u8 def[0x50];
    PartDbGetDef((PartDef*)def, gGameDb, *(s32*)entry);
    return *(s32*)(def + 0x34);
}

// 0x8004E370
// Contact check for the object in r4: if it has no contact record, passes. Otherwise runs
// CharacterOnContact unless the contact's object has both flags 0x40 (at +0x1C) and 0x2000000
// (at +0x20) set. Always returns TRUE.
BOOL fn_8004E370(u8* a, u8* b) {
    u8* e = *(u8**)(b + 0x5c);
    if (e == NULL) return TRUE;
    u8* o = *(u8**)(e + 0x4);
    if ((*(u32*)(o + 0x1c) & 0x40) && (*(u32*)(o + 0x20) & 0x2000000)) return TRUE;
    CharacterOnContact(a, b);
    return TRUE;
}

}
