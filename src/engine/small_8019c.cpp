// Small UI/widget functions in 0x80199xxx-0x8019Bxxx: deleting-dtor stubs, widget child lookup,
// vtable setup and list/table lookups.
#include "types.h"

typedef void (*VFnVoid)(void*);
typedef void (*VFnF32)(void*, f32);
typedef s32  (*VFnF32Ret)(void*, f32);

extern "C" {
void    fn_801A8AFC(void* obj);                                 // 0x801A8AFC
void    fn_8020A398(s32 handle);                                // 0x8020A398
void    fn_80199E80(void* p);                                   // 0x80199E80
s32     fn_80199DD4(void* obj);                                 // 0x80199DD4
void    fn_8019BFF8(void* obj, void* cb, void* arg);            // 0x8019BFF8
void    fn_8019BD18(void* widget, u32* ev);                     // 0x8019BD18
void*   fn_8019B088(void* obj, s32 kind, char* name, u32 arg);  // 0x8019B088
void*   fn_8018E8D8(void* table, u32 hash);                     // 0x8018E8D8
void    fn_8018D2B8(const char* fmt, ...);                      // 0x8018D2B8
u32     StringHash(const char* s);                              // 0x801EB704
void    operator_delete(void* p);                               // 0x801D426C
void    fn_80196678(void* gx, void* p, f32 t);                  // 0x80196678 (f1 passed through)
void    fn_80196B4C(void* gx, void* p, f32 t);                  // 0x80196B4C (f1 passed through)
void    fn_80199534(u8* a3);                                    // 0x80199534 (leaf)
s32     fn_80196B10(u8* a3);                                    // 0x80196B10 (leaf)
s32     fn_80196C88(u8* a3);                                    // 0x80196C88 (leaf)
s32     fn_8019555C(u8* a3);                                    // 0x8019555C (leaf)
u32     fn_8019552C(u8* a3, s32 a4);                            // 0x8019552C (leaf)
char*   fn_801D8ADC(char* s);                                   // 0x801D8ADC (path name tail)
}

extern "C++" void* gScreenSets;                                       // 0x805FB654
extern f32   lbl_805FF930;                                      // float constant
extern void* lbl_805FB658;                                      // global object pointer
extern u8    lbl_80382778[];                                    // vtable (second, +0x138)
extern const char lbl_80382808[];                               // string (index out of range)
extern const char lbl_80382868[];                               // string (index out of range)

extern "C" {

// 0x80199094
// Calls the slot 0x10 virtual of the second vtable, then runs fn_801A8AFC when the object is
// in state 2 with sub-state 3. Returns -2 for NULL, else 0.
s32 fn_80199094(void* obj) {
    if (obj == NULL) {
        return -2;
    }
    u8* o = (u8*)obj;
    (*(VFnVoid*)(*(u8**)(o + 0x138) + 0x10))(obj);
    if (*(s32*)(o + 0x60) == 2 && *(s32*)(o + 0x13C) == 3) {
        fn_801A8AFC(obj);
    }
    return 0;
}

// 0x80199100
// Runs fn_801A8AFC when the object is in state 2 with sub-state 3. Returns -2 for NULL, else 0.
s32 fn_80199100(void* obj) {
    if (obj == NULL) {
        return -2;
    }
    u8* o = (u8*)obj;
    if (*(s32*)(o + 0x60) == 2 && *(s32*)(o + 0x13C) == 3) {
        fn_801A8AFC(obj);
    }
    return 0;
}

// 0x80199420
// Deleting-destructor stub: frees obj when the delete flag is positive. Returns obj.
void* fn_80199420(void* obj, s32 del) {
    if (obj != NULL && del > 0) {
        operator_delete(obj);
    }
    return obj;
}

// 0x80199460
// Deleting-destructor stub: frees obj when the delete flag is positive. Returns obj.
void* fn_80199460(void* obj, s32 del) {
    if (obj != NULL && del > 0) {
        operator_delete(obj);
    }
    return obj;
}

// 0x801994A0
// Deleting-destructor stub: frees obj when the delete flag is positive. Returns obj.
void* fn_801994A0(void* obj, s32 del) {
    if (obj != NULL && del > 0) {
        operator_delete(obj);
    }
    return obj;
}

// 0x801994E0
// Object setup callback (used when walking a table): installs the second vtable at +0x138 and
// runs fn_80199534 through fn_8019BFF8. Returns -2 for NULL, else 0.
s32 fn_801994E0(void* a, void* obj) {
    (void)a;
    if (obj == NULL) {
        return -2;
    }
    *(void**)((u8*)obj + 0x138) = lbl_80382778;
    fn_8019BFF8(obj, (void*)fn_80199534, NULL);
    return 0;
}

// 0x80199618
// Clears gScreenSets+0x6C, registers fn_80199DD4 through fn_8019BFF8, calls the slot 0x18
// virtual of the second vtable with lbl_805FF930, then sets gScreenSets+0x6C to 1. Returns 0.
s32 fn_80199618(void* obj) {
    *(s32*)((u8*)gScreenSets + 0x6C) = 0;
    fn_8019BFF8(obj, (void*)fn_80199DD4, NULL);
    (*(VFnF32*)(*(u8**)((u8*)obj + 0x138) + 0x18))(obj, lbl_805FF930);
    *(s32*)((u8*)gScreenSets + 0x6C) = 1;
    return 0;
}

// 0x80199ABC
// Runs fn_80199E80 on the pointer stored at +0x144 when it is set. Returns 0.
s32 fn_80199ABC(void* obj) {
    void* p = *(void**)((u8*)obj + 0x144);
    if (p != NULL) {
        fn_80199E80(p);
    }
    return 0;
}

// 0x80199AEC
// Searches the child list at +0x140 (count at +0x13C) for an entry whose first word equals hash.
// Returns that entry, or NULL.
void* UIWidgetFindChild(void* widget, u32 hash) {
    u8* w = (u8*)widget;
    s32 count = *(s32*)(w + 0x13C);
    void** list = *(void***)(w + 0x140);
    for (s32 i = 0; i < count; i++) {
        if (hash == *(u32*)list[i]) {
            return list[i];
        }
    }
    return NULL;
}

// 0x80199B38
// Plays the animation with hash anim on the widget, through fn_8019BFF8 and fn_8019BD18.
void UIWidgetPlayAnim(void* w, u32 anim) {
    u32 ev = anim;
    fn_8019BFF8(w, (void*)fn_8019BD18, &ev);
}

// 0x80199C30
// Releases the handle stored at +0x14C (if any) through fn_8020A398 and clears it to -1.
void fn_80199C30(void* obj) {
    u8* o = (u8*)obj;
    s32 handle = *(s32*)(o + 0x14C);
    if (handle != -1) {
        fn_8020A398(handle);
        *(s32*)(o + 0x14C) = -1;
    }
}

// 0x80199C74
// Calls the slot 0x18 virtual of the second vtable with the float at t, then runs fn_80196678 and
// fn_80196B4C on the global object with the object's +0xF0 and +0x80 sub-blocks. Returns the
// virtual's result, or -2 for NULL.
s32 fn_80199C74(void* obj, f32* t) {
    f32 v = *t;
    if (obj == NULL) {
        return -2;
    }
    u8* o = (u8*)obj;
    s32 r = (*(VFnF32Ret*)(*(u8**)(o + 0x138) + 0x18))(obj, v);
    fn_80196678(lbl_805FB658, o + 0xF0, v);
    fn_80196B4C(lbl_805FB658, o + 0x80, v);
    return r;
}

// 0x80199DA4
// Runs fn_80196B10 and fn_80196C88 on the global object. Returns 0.
s32 fn_80199DA4(void) {
    fn_80196B10((u8*)lbl_805FB658);
    fn_80196C88((u8*)lbl_805FB658);
    return 0;
}

// 0x80199E40
// Deleting-destructor stub: frees obj when the delete flag is positive. Returns obj.
void* fn_80199E40(void* obj, s32 del) {
    if (obj != NULL && del > 0) {
        operator_delete(obj);
    }
    return obj;
}

// 0x8019A994
// Resets the record: when the count at +0x8 is nonzero the zero-numerator quotient and remainder
// at +0xC and +0x10 are stored as 0; then +0x14 and +0x18 become -1 and the flag bits 3..4
// (value 0x18) of the word at +0 are cleared.
void fn_8019A994(void* p) {
    u8* o = (u8*)p;
    s32 n = *(s32*)(o + 0x8);
    if (n != 0) {
        *(s32*)(o + 0xC) = 0;
        *(s32*)(o + 0x10) = 0;
    }
    *(s32*)(o + 0x18) = -1;
    *(s32*)(o + 0x14) = -1;
    *(u32*)o &= ~0x18u;
}

// 0x8019AC00
// Returns the first word of entry index in the list at obj+4. Logs and returns 0 when index is
// out of range.
u32 fn_8019AC00(void* obj, s32 index) {
    if (index >= 0) {
        u8* list = *(u8**)((u8*)obj + 0x4);
        if (index < fn_8019555C(list)) {
            u8* ent = (u8*)(uintptr_t)fn_8019552C(list, index);
            return *(u32*)ent;
        }
    }
    fn_8018D2B8(lbl_80382808, index);
    return 0;
}

// 0x8019AD84
// Returns the second word of entry index in the list at obj+4. Logs and returns 0 when index is
// out of range.
u32 fn_8019AD84(void* obj, s32 index) {
    if (index >= 0) {
        u8* list = *(u8**)((u8*)obj + 0x4);
        if (index < fn_8019555C(list)) {
            u8* ent = (u8*)(uintptr_t)fn_8019552C(list, index);
            return *(u32*)(ent + 0x4);
        }
    }
    fn_8018D2B8(lbl_80382868, index);
    return 0;
}

// 0x8019AF88
// Looks up the child named name (hashed) in the table at obj+0x20. When it is missing, creates it
// through fn_8019B088 (kind 2). Returns the entry at +0xCC.
void* fn_8019AF88(void* obj, char* name, u32 arg) {
    u32 hash = StringHash(fn_801D8ADC(name));
    void* found = fn_8018E8D8(*(void**)((u8*)obj + 0x20), hash);
    if (found != NULL) {
        return (u8*)found + 0xCC;
    }
    void* r = fn_8019B088(obj, 2, name, arg);
    return (u8*)r + 0xCC;
}

// 0x8019B008
// Same as fn_8019AF88 but on the table at obj+0x28 and with kind 4 when it has to create the entry.
void* fn_8019B008(void* obj, char* name, u32 arg) {
    u32 hash = StringHash(fn_801D8ADC(name));
    void* found = fn_8018E8D8(*(void**)((u8*)obj + 0x28), hash);
    if (found != NULL) {
        return (u8*)found + 0xCC;
    }
    void* r = fn_8019B088(obj, 4, name, arg);
    return (u8*)r + 0xCC;
}

}
