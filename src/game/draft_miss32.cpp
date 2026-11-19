// Misc. game code (script callbacks, part UI image, book screen switch, particles, path
// helpers, voice SFX, PlayAnim tail calls). Original: 0x8010FB48-0x8010FF78 (sound/book),
// 0x8013A654-0x8013F394 (friend list / screen helpers), 0x800D5724-0x800D648C (path),
// 0x800F1E3C (part UI image), 0x80174FF0 (particles), 0x8020E7B4 (part slot name),
// 0x801C71EC (zone check), 0x80103788-0x801037D8 (PlayAnim tail calls).
#include "types.h"
#include <string.h>
#include "game/part.h"
#include "game/character.h"

// Globals with C++ linkage (same declarations as their defining files).
extern "C++" void* gScreenSets;               // 0x805FB654
extern void*       gLab;                      // 0x805FBBC8
extern u8          gWorldState[];             // 0x803E6D00
extern u8          lbl_805FB248[];            // 0x805FB248 (book table pointer)
extern void*       lbl_805FB964;              // 0x805FB964 (movie player)
extern void*       lbl_805FBBAC;              // 0x805FBBAC (voice SFX handle)
extern s32         lbl_805F9028;              // 0x805F9028 (friend list flag)
extern u8          lbl_80556AC8[];            // 0x80556AC8 (lock object)
extern u8          lbl_80556E80[];            // 0x80556E80 (player data block)
extern u8          lbl_803F2298[];            // 0x803F2298 (empty text)
extern f32         lbl_803C6F80[];            // 0x803C6F80 (default direction, 3 floats)
extern u8          lbl_80348F38[];            // 0x80348F38 (particle dispatch table)
extern u8          lbl_80388D28[];            // 0x80388D28 (part slot names)

extern "C" {
#include "game/script_callbacks.h"
}

// C++ linkage: defined in small_8013.cpp outside extern "C".
void fn_8013A03C(void);

extern "C" {

// StringHash constants (see tools/hashes.py).
static const u32 kHashInventory = 0xF5EF54D9u;  // "inventory"
static const u32 kHashJournal   = 0xFEE21957u;  // "Journal"

void*  ScreenSetsCurrent(void* sets);
void*  ScreenSetFindElement(void* set, u32 hash);
void*  ScreenSetFindWidget(void* set, u32 hash);
void*  UIWidgetFindChild(void* widget, u32 hash);
void   UISetText(void* el, const char* text, s32 a);
s32    UISetTexture(void* el, void* tex);
void   TextureRelease(void* tex);
BOOL   ZoneIsLoaded(void* world, s32 zone);
s32    PlaySoundAt(void* sound, int a, const void* pos, int b);
void*  fn_801D40E0(s32 n);
void*  fn_80209814(void* v);
void*  fn_8011AD88(void* def, s32 flag);
ScriptValue fn_801A1208(u8* el);
void   fn_8016A64C(void* sys);
s32    fn_800D5FDC(PathFollower* p, u8* cur, s32 node, s32 a);
void   fn_802E65C0(u8* rec);
void   fn_802E6698(void* cb, s32 a);
void   fn_8023A5B0(u8* p);
void   fn_8023A68C(u8* p);

// 0x8010FF78
// Book screen switch. lbl_805FB248 is a table: [0] = current index (-1 = none), then the
// screen objects at +4 + index*4. Leaving a screen calls its vfunc 0x10, entering calls 0x0C.
void SetBook(u32 a0) {
    s32 var;
    u8* top = *(u8**)lbl_805FB248;
    if (top == NULL) return;
    if (a0 == kHashInventory) {
        var = 0;
    } else if (a0 == kHashJournal) {
        var = 1;
    } else {
        var = -1;
    }
    s32 old = *(s32*)top;
    if (var != old) {
        if (old != -1) {
            u8* obj = *(u8**)(top + 4 + old * 4);
            (*(void (**)(void*))(*(u8**)obj + 0x10))(obj);
        }
        *(s32*)(*(u8**)lbl_805FB248) = var;
        s32 cur = *(s32*)(*(u8**)lbl_805FB248);
        if (cur != -1) {
            u8* obj = *(u8**)(*(u8**)lbl_805FB248 + 4 + cur * 4);
            (*(void (**)(void*))(*(u8**)obj + 0x0C))(obj);
        }
    }
}

// 0x8013A654
// Clears the player record at lbl_80556E80 + arg0*0x20 under the lock, clears its counters,
// marks the movie player's flag at +0xB1, and re-sorts the friend list.
void fn_8013A654(s32 arg0) {
    fn_8023A5B0(lbl_80556AC8);
    u8* rec = lbl_80556E80 + (arg0 << 5);
    *(u8*)(rec + 0x340) = 0;
    *(s32*)(rec + 0x354) = 0;
    *(s32*)(rec + 0x350) = 0;
    *(s32*)(rec + 0x35C) = 0;
    *(s32*)(rec + 0x358) = 0;
    fn_802E65C0(lbl_80556E80 + arg0 * 0xC + 0x40);
    if (lbl_805FB964 != NULL && *(u8*)((u8*)lbl_805FB964 + 0xB1) != 0) {
        *(u8*)((u8*)lbl_805FB964 + 0xB1) = 1;
    }
    FriendListSort((BOOL)lbl_805F9028);
    fn_8023A68C(lbl_80556AC8);
}

// 0x8013A6F4
// Under the lock: when arg0 == 0, registers fn_8013A03C; then if arg1 is set, sets the movie
// player flag at +0xB1 (when it is already non-zero).
void fn_8013A6F4(s32 arg0, s32 arg1) {
    fn_8023A5B0(lbl_80556AC8);
    if (arg0 == 0) {
        fn_802E6698((void*)fn_8013A03C, 0);
        if (arg1 != 0 && lbl_805FB964 != NULL && *(u8*)((u8*)lbl_805FB964 + 0xB1) != 0) {
            *(u8*)((u8*)lbl_805FB964 + 0xB1) = 1;
        }
    }
    fn_8023A68C(lbl_80556AC8);
}

// 0x8013F394
// Sets the text of the widget with hash 0x645DBC95 under the current screen set's widget
// 0x43538242, then calls the child 0xFE89BC8F's sub-object method (0x2C when arg0 == 1, else 0x30).
void fn_8013F394(s32 arg0) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    u8* w = (u8*)ScreenSetFindWidget(set, 0x43538242u);
    if (w == NULL) return;
    u8* t = (u8*)UIWidgetFindChild(w, 0x645DBC95u);
    if (t == NULL) return;
    UISetText(t, (const char*)lbl_803F2298, 1);
    u8* c = (u8*)UIWidgetFindChild(w, 0xFE89BC8Fu);
    if (c == NULL) return;
    if ((u32)arg0 == 1u) {
        ((void (*)(void*, s32))(*(void**)(*(u8**)(c + 0x138) + 0x2C)))(c, 2);
        return;
    }
    ((void (*)(void*, s32))(*(void**)(*(u8**)(c + 0x138) + 0x30)))(c, 2);
}

// 0x801C71EC
// 1 when the record's zone (u8 at +1) is loaded and its id (u16 at +2) is not 0xFFFF.
s32 fn_801C71EC(u8* arg0) {
    s32 r = 0;
    if (*(u16*)(arg0 + 2) != 0xFFFF && ZoneIsLoaded(gWorldState, *(u8*)(arg0 + 1)) != 0) {
        r = 1;
    }
    return r;
}

// 0x800F1E3C
// Sets the UI element for hash a0 to the texture of part a1 (0..0xB9); releases the element's
// previous texture (refcount at +0x24) when it drops to zero.
void PartSetPartUIImage(u32 a0, s32 a1) {
    u8* el = (u8*)ScreenSetFindElement(ScreenSetsCurrent(gScreenSets), a0);
    if (a1 >= 0 && a1 < 0xBA) {
        PartDef def;
        PartDbGetDef(&def, gGameDb, a1);
        PartDef copy = def;
        void* tex = fn_8011AD88(&copy, 0);
        u8* old = (u8*)(intptr_t)fn_801A1208(el);
        s32 refs = *(s32*)(old + 0x24) - 1;
        *(s32*)(old + 0x24) = refs;
        if (refs == 0) TextureRelease(old);
        UISetTexture(el, tex);
    }
}

// 0x8020E7B4
// Name of part slot `slot` from the string table lbl_80388D28 (4-byte entries).
const char* PartSlotName(s32 slot) {
    return (const char*)(uintptr_t)*(u32*)(lbl_80388D28 + slot * 4);
}

// 0x80174FF0
// Calls fn_8016A64C on the system's list, then for every element of the list at +0x7C calls
// the dispatch entry lbl_80348F38[sys->type].unk24 with (sys+8, element word, element+8, offset, dt).
void ParticleSystemUpdateExtra(u8* sys, f32 dt) {
    fn_8016A64C(*(void**)(sys + 0x7C));
    s32 i = 0;
    while (i < *(s32*)(*(u8**)(sys + 0x7C) + 0x10)) {
        u8* list = *(u8**)(sys + 0x7C);
        s32 off = i * *(s32*)(list + 0x18);
        u8* elem = list + *(s32*)(list + 0x8) + off;
        s32 type = *(u8*)(sys + 0xC);
        typedef void (*ParticleElemFn)(s32 sysField, s32 elemA, s32 elemB, s32 off, f32 dt);
        ParticleElemFn fn = (ParticleElemFn)(uintptr_t)*(u32*)(lbl_80348F38 + type * 4 + 0x24);
        fn(*(s32*)(sys + 0x8), *(s32*)elem, *(s32*)(elem + 0x8), off, dt);
        i += 1;
    }
}

// 0x800D648C
// Resets the path visit table (0x3FC bytes at +4), marks +0x400, copies the path's first
// node word to +4, and searches from the path's first node unless it already is `node`.
s32 PathFindNode(PathFollower* p, s32 node, int a) {
    u8* pu = (u8*)p;
    if (node == 0) return -1;
    memset(pu + 4, 0, 0x3FC);
    *(s32*)(pu + 0x400) = 1;
    u8* st = *(u8**)pu;
    *(s32*)(pu + 4) = *(s32*)(st + 0x14);
    u8* cur = *(u8**)(st + 0x14);
    if (cur != NULL && *(u32*)cur != (u32)node) {
        return fn_800D5FDC(p, cur, node, a);
    }
    return -1;
}

// 0x800D5724
// Direction from the path's current position to its target point. Uses the default vector
// lbl_803C6F80 when the state flag at +0x14 is set; otherwise the point is picked by the
// index at +0xC (-4 and -3 select the special points, others index the point list).
Vec3 PathGetDirection(PathFollower* p) {
    u8* st = *(u8**)p;
    Vec3 out;
    if (*(s32*)(st + 0x14) != 0) {
        out.x = lbl_803C6F80[0];
        out.y = lbl_803C6F80[1];
        out.z = lbl_803C6F80[2];
        return out;
    }
    s32 kind = *(s32*)(st + 0xC);
    u8* pt;
    if (kind == -4) {
        pt = *(u8**)(*(u8**)(st + 0x10) + 0x8) + 0x34;
    } else if (kind == -3) {
        pt = *(u8**)(*(u8**)(st + 0x10) + 0x4) + 0x34;
    } else {
        u8* g = *(u8**)(st + 0x10);
        pt = g + 0x18;
        s32 count = *(s32*)(g + 0xD8);
        s32 idx;
        if (kind < 0) {
            idx = 0;
        } else {
            idx = kind;
            if (kind > count) idx = count;
        }
        if (kind == idx) pt += kind * 0x10;
    }
    out.x = *(f32*)(pt + 0x0) - *(f32*)(st + 0x18);
    out.y = *(f32*)(pt + 0x4) - *(f32*)(st + 0x1C);
    out.z = *(f32*)(pt + 0x8) - *(f32*)(st + 0x20);
    return out;
}

// 0x8010FB48
// Ensures the voice SFX handle lbl_805FBBAC exists, plays sound a0 (id 6) and sound a1 (id 7),
// creating the handle again before the second play if it was cleared.
void PlayBackgroundSFX(u32 a0, u32 a1) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    PlaySoundAt((void*)(uintptr_t)a0, 0, NULL, 6);
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    PlaySoundAt((void*)(uintptr_t)a1, 0, NULL, 7);
}

// 0x80103788
// Tail call: forwards to vtable slot 0x80 of obj (NULL obj does nothing).
void PlayAnim(void* a0, u32 a1) {
    if (a0 == NULL) return;
    ((void (*)(void*, u32))(*(void**)(*(u8**)a0 + 0x80)))(a0, a1);
}

// 0x801037A4
// Tail call: forwards to vtable slot 0x84 of obj (NULL obj does nothing).
void PlayAnimByID(void* a0, u32 a1) {
    if (a0 == NULL) return;
    ((void (*)(void*, u32))(*(void**)(*(u8**)a0 + 0x84)))(a0, a1);
}

// 0x801037C0
// Tail call: forwards to vtable slot 0x88 of obj (NULL obj does nothing).
void PlayAnimByIDLoop(void* a0, u32 a1) {
    if (a0 == NULL) return;
    ((void (*)(void*, u32))(*(void**)(*(u8**)a0 + 0x88)))(a0, a1);
}

}  // extern "C"
