// Misc engine/UI functions (batch pz1d): video projection setup, UI string tidy, anim callbacks,
// pool free-list builder, event ring copy.
#include <string.h>

#include "types.h"
#include "engine/rend.h"

struct View;
struct ScriptFunc { u32 type; void* fn; u32 hash; };   // 0xC bytes (as in script_dispatch.cpp)

extern "C++" View* gMainView;   // 0x805FB75C

extern "C" {
void* MemAllocDefault(u32 size);                              // 0x801D4650
void* ObjAlloc(u32 size);                                     // 0x801D4414
u32   StringHash(const char* s);                              // 0x801EB704
int   SysSprintf(char* buf, const char* fmt, ...);            // 0x80142EF8
void  fn_8016E304(void* cam);                                 // 0x8016E304
void  fn_8016FB20(void*);                                     // 0x8016FB20
void  fn_80170044(void*);                                     // 0x80170044
void  fn_8018D2B8(const char* fmt, ...);                      // 0x8018D2B8 (debug log)
void  fn_8018D370(const char* fmt, ...);                      // 0x8018D370 (debug log)
s32   UIText_SubstituteText(void* el);                                  // 0x8019EB94
s32   fn_8019F5A8(void* el);                                  // 0x8019F5A8
void* fn_80196CCC(void* unused, const char* name);              // 0x80196CCC (loads texture, returns it)
u8*   fn_80196D04(void* obj, const char* name, const char* arg); // 0x80196D04
u8*   fn_80196D0C(void* obj, const char* name, s32 flag);     // 0x80196D0C
s32 fn_801AA85C(u8* arg0, u8* arg1, s32 arg2);              // 0x801AA85C
void  fn_801AB3CC(u8* arg, u8* owner, s32 flag);              // 0x801AB3CC
u8*   fn_801ABF0C(u8* table, u32 hash, s32 key);              // 0x801ABF0C
s32 fn_801B7E7C(u8* arg0, u8* arg1, f32 farg0);                 // 0x801B7E7C (anim callback)
u8*   fn_801C6BCC(u8* key);                                   // 0x801C6BCC
s32   fn_801EAFA4(u8* task, f32 t);                           // 0x801EAFA4
f64   tan(f64 x);                                     // 0x8033F17C

extern ScriptFunc gScriptFuncs[];       // 0x80484230
extern const char lbl_803828C8[];       // 0x803828C8 ("UIAssetMgr.cpp" + messages)
extern const char lbl_80383E58[];       // 0x80383E58 ("FadeToOverlayAlphaTask")
extern void* lbl_805FB5F8;              // 0x805FB5F8 (current camera)
extern void* lbl_805FB658;              // 0x805FB658 (global render object)
extern void* lbl_805FB660;              // 0x805FB660
extern s8    lbl_805FB6C1;              // 0x805FB6C1 (hash cached flag)
extern u32   lbl_80601104;              // 0x80601104 (cached "FadeToOverlayAlphaTask" hash)
}

typedef void (*VFnII)(void*, s32);
typedef void (*ScriptCbA)(u8*, s32, u8*);
typedef void (*ScriptCbB)(u8*, u8*);
typedef void (*ScriptCbC)(u8*, u8*, u8*);

extern "C" {

// 0x801D89C8
// Sets the main view's projection from a vertical FOV: stores the FOV scale (+0x18C), aspect
// ratio (+0x190), tan of the half angle (+0x184) and tan*aspect (+0x180), then refreshes the view.
void fn_801D89C8(f32 fov) {
    s32 saved = 0;
    if (lbl_805FB5F8 != NULL) {
        saved = 1;
        fn_80170044(gMainView);
    }
    if (VideoGetHeight(&gVideo) > 0) {
        s32 h = VideoGetHeight(&gVideo);
        s32 w = VideoGetWidth(&gVideo);
        f32 scale = 0.63661975f * fov;
        u8* v = (u8*)gMainView;
        *(f32*)(v + 0x18C) = scale;
        f32 aspect = (f32)w / (f32)h;
        *(f32*)(v + 0x190) = aspect;
        f32 t = (f32)tan((f64)(1.5707964f * (0.5f * scale)));
        *(f32*)(v + 0x184) = t;
        *(f32*)(v + 0x180) = t * aspect;
        fn_8016E304(gMainView);
    }
    if (saved != 0) {
        fn_8016FB20(gMainView);
    }
}

// 0x801ED1F8
// Searches `count` records (stride 0xC8, array at +4) for a u32 equal to `key` in the 8 words
// at record+0x68..0x84. Returns 1 if found, else 0.
s32 fn_801ED1F8(u8* arg0, u32 key) {
    s32 count = *(s32*)arg0;
    u8* rec = *(u8**)(arg0 + 4);
    for (s32 i = 0; i < count; i++) {
        for (s32 k = 0; k < 8; k++) {
            if (key == *(u32*)(rec + 0x68 + k * 4)) return 1;
        }
        rec += 0xC8;
    }
    return 0;
}

// 0x801B6AD4
// Looks up a record in the table at +0x84 by hash 0xDC2E57C9 and the object's virtual slot 0x10,
// updates the 0x10 flag in the state at +4 (+0x20) from record+0x18, then calls virtual slot 0x78
// with record+0x14 (or 0 if there is no record / the flag is clear).
void fn_801B6AD4(u8* arg0) {
    if (*(u8**)(arg0 + 0x84) == NULL) return;
    s32 key = (*(s32 (**)(u8*))(*(u8**)arg0 + 0x10))(arg0);
    u8* rec = fn_801ABF0C(*(u8**)(arg0 + 0x84), 0xDC2E57C9, key);
    if (rec != NULL) {
        u8* st = *(u8**)(arg0 + 4);
        if (*(s32*)(rec + 0x18) != 0) {
            *(u32*)(st + 0x20) |= 0x10;
        } else {
            *(u32*)(st + 0x20) &= ~0x10u;
        }
        s32 v = *(s32*)(rec + 0x14);
        if (v != 0) {
            (*(VFnII*)(*(u8**)arg0 + 0x78))(arg0, v);
            return;
        }
    }
    u8* st = *(u8**)(arg0 + 4);
    *(u32*)(st + 0x20) &= ~0x10u;
    (*(VFnII*)(*(u8**)arg0 + 0x78))(arg0, 0);
}

// 0x8019F3B4
// Sets the line count at +0x160 and replaces every CR/LF in the text at +0x170 by a space; when
// the flag 0x20000 at +0x64 is set, calls UIText_SubstituteText. If the count is positive it then returns
// fn_8019F5A8(el), else 0.
s32 fn_8019F3B4(u8* arg0, s32 arg1) {
    *(s32*)(arg0 + 0x160) = arg1;
    char* p = *(char**)(arg0 + 0x170);
    if (arg1 > 0) {
        if (p != NULL && strlen(p) != 0) {
            while (*p != 0) {
                if (*p == 10 || *p == 13) *p = 0x20;
                p++;
            }
            if (*(u32*)(arg0 + 0x64) & 0x20000) UIText_SubstituteText(arg0);
        }
        return fn_8019F5A8(arg0);
    }
    if (p == NULL || strlen(p) == 0) return 0;
    while (*p != 0) {
        if (*p == 10 || *p == 13) *p = 0x20;
        p++;
    }
    if (*(u32*)(arg0 + 0x64) & 0x20000) UIText_SubstituteText(arg0);
    return 0;
}

// 0x801B7D5C
// Initialises a fade-to-overlay-alpha task record (0x64 bytes zeroed, flags/ids at +0x64..+0x7F),
// with start value a1, duration a2 and its reciprocal at +8 (0 if a2 <= 0); the mode at +0x10 is
// 1 if a3 != 0 else 2. Type hash "FadeToOverlayAlphaTask" is cached on first use. Returns the record.
u8* fn_801B7D5C(u8* arg0, u32 a3, f32 a1, f32 a2) {
    *(u16*)(arg0 + 0x78) = 0xFFFF;
    *(u16*)(arg0 + 0x7A) = 0xFFFF;
    *(u16*)(arg0 + 0x7C) = 0xFFFF;
    memset(arg0, 0, 0x64);
    f32 inv = 0.0f;
    *(u32*)(arg0 + 0x64) = 0;
    *(u32*)(arg0 + 0x68) = 0;
    *(f32*)(arg0 + 0x70) = 0.0f;
    *(u16*)(arg0 + 0x78) = 0xFFFF;
    *(u16*)(arg0 + 0x7A) = 0xFFFF;
    *(u16*)(arg0 + 0x7C) = 0xFFFF;
    *(u32*)(arg0 + 0x74) = 0;
    *(u8*)(arg0 + 0x7E) = 0;
    *(u8*)(arg0 + 0x7F) = 0;
    *(void**)(arg0 + 0x6C) = (void*)fn_801B7E7C;   // lint:ok callback slot
    *(f32*)(arg0 + 0x0) = a1;
    *(f32*)(arg0 + 0x4) = a2;
    if (a2 > 0.0f) inv = 1.0f / a2;
    *(f32*)(arg0 + 0x8) = inv;
    *(s32*)(arg0 + 0x10) = (a3 != 0) ? 1 : 2;
    if (lbl_805FB6C1 == 0) {
        lbl_80601104 = StringHash(lbl_80383E58);
        lbl_805FB6C1 = 1;
    }
    *(u32*)(arg0 + 0x64) = lbl_80601104;
    return arg0;
}

// 0x801EBAB8
// Builds the free list of a fixed-size object pool. If +0x18 is set the pool is in error state
// (+0x10 = 0x69696969). Otherwise allocates a block (ObjAlloc if +0x28 == -2, else MemAllocDefault,
// unless +0x24 already holds one), chains it at +0x1C, aligns the first slot to the element
// alignment (+8), and links `count` (+0) slots of size +4 (+8 header) into a doubly linked list.
void fn_801EBAB8(u8* arg0) {
    if (*(s32*)(arg0 + 0x18) != 0) {
        *(u32*)(arg0 + 0x10) = 0x69696969;
        return;
    }
    u8* block = *(u8**)(arg0 + 0x24);
    if (block == NULL) {
        s32 cnt = *(s32*)(arg0 + 0x0);
        u32 size = cnt * 8 + *(s32*)(arg0 + 0x8) + *(s32*)(arg0 + 0x4) * cnt + 4;
        if (*(s32*)(arg0 + 0x28) == -2) {
            block = (u8*)ObjAlloc(size);
        } else {
            block = (u8*)MemAllocDefault(size);
        }
    }
    *(u8**)block = *(u8**)(arg0 + 0x1C);
    *(u8**)(arg0 + 0x1C) = block;
    s32 align = *(s32*)(arg0 + 0x8);
    u8* first = (u8*)((((uintptr_t)block + 4 + align + 7) & ~(uintptr_t)(align - 1)) - 8);
    *(u8**)(arg0 + 0x10) = first;
    *(u8**)first = NULL;
    u8* cur = *(u8**)(arg0 + 0x10);
    for (s32 i = 0; i < *(s32*)arg0 - 1; i++) {
        u8* next = cur + *(s32*)(arg0 + 0x4) + 8;
        *(u8**)(cur + 4) = next;
        *(u8**)next = cur;
        cur = *(u8**)(cur + 4);
    }
    *(u8**)(cur + 4) = NULL;
}

// 0x8019B1C8
// UI asset loader (UIAssetMgr.cpp): kind 0/1 -> fn_80196D0C(flag 0/1), 2 -> fn_80196CCC,
// 4 -> fn_80196D04, 3 -> logs an unsupported-kind error with the "dir\name" path, other kinds log.
void* UIAssetMgr_BuildPath(void* obj, s32 kind, char* name, char* arg) {
    char buf[0xD0];
    u8* result = NULL;
    switch (kind) {
    case 0:
        result = fn_80196D0C(lbl_805FB660, name, 0);
        break;
    case 1:
        result = fn_80196D0C(lbl_805FB660, name, 1);
        break;
    case 2:
        result = (u8*)fn_80196CCC(lbl_805FB658, name);
        break;
    case 3:
        if (arg != NULL) {
            SysSprintf(buf, "%s\\%s", arg, name);
        } else {
            SysSprintf(buf, "%s", name);   // r5 still holds name
        }
        fn_8018D2B8(lbl_803828C8 + 0xCC, buf);
        fn_8018D370(lbl_803828C8 + 0xF4);
        break;
    case 4:
        result = fn_80196D04(lbl_805FB658, name, arg);
        break;
    default:
        fn_8018D370(lbl_803828C8 + 0xA8, kind);
        break;
    }
    return result;
}

// 0x801EACF4
// Anim/task callback by event type at +0x7F. Type 8: looks up a record by the 4-byte key at +4,
// runs fn_801EAFA4 (sets flag 4 at +0x38 when it returns 0) and calls the script function of
// +0x48 with (record, +0x40, self); returns 2. Type 32: if flag 4 is set and the script function
// index at +0x4C is not -1, calls it with (arg1, self); returns 3. Otherwise returns 2.
s32 fn_801EACF4(u8* arg0, u8* arg1, f32 t) {
    switch (*(u8*)(arg0 + 0x7F)) {
    case 8: {
        u16 key[2];
        key[0] = *(u16*)(arg0 + 4);
        key[1] = *(u16*)(arg0 + 6);
        u8* rec = fn_801C6BCC((u8*)key);
        if (fn_801EAFA4(arg0, t) == 0) {
            *(u32*)(arg0 + 0x38) |= 4;
        }
        if (rec != NULL) {
            ScriptFunc* f = &gScriptFuncs[*(s32*)(arg0 + 0x48)];
            ((ScriptCbA)f->fn)(rec, *(s32*)(arg0 + 0x40), arg0);
        }
        return 2;
    }
    case 32: {
        if (*(u32*)(arg0 + 0x38) & 4) {
            s32 idx = *(s32*)(arg0 + 0x4C);
            if (idx != -1) {
                ScriptFunc* f = &gScriptFuncs[idx];
                ((ScriptCbB)f->fn)(arg1, arg0);
            }
            return 3;
        }
        return 2;
    }
    default:
        return 2;
    }
}

// 0x801AB820
// For each of the 5 slot objects at +4..+0x14: copies bit 3 of the flags at +0x1C into the slot
// object's byte +0x1C and runs fn_801AA85C on the matching sub-record of arg1 (+0x68, +0x74, +0x80
// (count 4), +0x90, +0x9C). If +0x18 is set, calls fn_801AB3CC(*(+0x18), arg1, 0). Returns 0.
s32 fn_801AB820(u8* arg0, u8* arg1) {
    for (s32 i = 0; i < 5; i++) {
        u8* slot = *(u8**)(arg0 + 4 + i * 4);
        if (slot != NULL) {
            *(u8*)(slot + 0x1C) = (u8)((*(u32*)(arg0 + 0x1C) >> 3) & 1);
            switch (i) {
            case 0: fn_801AA85C(slot, arg1 + 0x68, 3); break;
            case 1: fn_801AA85C(slot, arg1 + 0x74, 3); break;
            case 2: fn_801AA85C(slot, arg1 + 0x80, 4); break;
            case 3: fn_801AA85C(slot, arg1 + 0x90, 3); break;
            case 4: fn_801AA85C(slot, arg1 + 0x9C, 3); break;
            default: fn_8018D370("UIAnim : how did you get here ...\n"); break;
            }
        }
    }
    u8* p = *(u8**)(arg0 + 0x18);
    if (p != NULL) {
        fn_801AB3CC(p, arg1, 0);
    }
    return 0;
}

// 0x801EB5B0
// Anim/task callback by event type at +0x7F. Type 16: copies the vec3 at +0 and calls script
// function [+0x20] with (&vec, arg1, self); then, if the index at +0x24 is not -1, copies the
// vec4 at +0xC and calls that function with (&vec4, arg1, self); returns 1. Type 32: returns 3 if
// +0x1C > 0 and +0x70 > +0x1C, else 1. Other types return 1.
s32 fn_801EB5B0(u8* arg0, u8* arg1) {
    switch (*(u8*)(arg0 + 0x7F)) {
    case 16: {
        f32 v3[3];
        v3[0] = *(f32*)(arg0 + 0x0);
        v3[1] = *(f32*)(arg0 + 0x4);
        v3[2] = *(f32*)(arg0 + 0x8);
        ((ScriptCbC)gScriptFuncs[*(s32*)(arg0 + 0x20)].fn)((u8*)v3, arg1, arg0);
        s32 idx = *(s32*)(arg0 + 0x24);
        if (idx != -1) {
            f32 v4[4];
            v4[0] = *(f32*)(arg0 + 0xC);
            v4[1] = *(f32*)(arg0 + 0x10);
            v4[2] = *(f32*)(arg0 + 0x14);
            v4[3] = *(f32*)(arg0 + 0x18);
            ((ScriptCbC)gScriptFuncs[idx].fn)((u8*)v4, arg1, arg0);
        }
        return 1;
    }
    case 32: {
        f32 a = *(f32*)(arg0 + 0x1C);
        if (a > 0.0f && *(f32*)(arg0 + 0x70) > a) return 3;
        return 1;
    }
    default:
        return 1;
    }
}

// 0x801B9CBC
// Event ring buffer filter: stores the callback at +0xC28, resets the output ring (+0x81C head,
// +0x820 tail, +0x824 count) and walks the input ring (64 slots of 16 bytes at +0x10, read index
// +4, write index +0xC). Each entry passing cb() is appended to the output ring at +0x828 (the
// head advances when it is full). Finally +0x824 = output tail and the read index is set to the
// write index.
void fn_801B9CBC(u8* mgr, BOOL (*cb)(void**)) {
    *(void**)(mgr + 0xC28) = (void*)cb;   // lint:ok callback slot
    u32 rd = *(u32*)(mgr + 4);
    *(u32*)(mgr + 0x824) = 0;
    *(u32*)(mgr + 0x820) = 0;
    *(u32*)(mgr + 0x81C) = 0;
    while (rd != *(u32*)(mgr + 0xC)) {
        u8* ent = mgr + 0x10 + rd * 16;
        rd++;
        if (rd == 0x40) rd = 0;
        if (cb((void**)ent) != 0) {
            u32 tail = *(u32*)(mgr + 0x820);
            u8* dst = mgr + 0x828 + tail * 16;
            u32 w0 = *(u32*)(ent + 0);
            u32 w1 = *(u32*)(ent + 4);
            u32 w2 = *(u32*)(ent + 8);
            u32 w3 = *(u32*)(ent + 12);
            tail++;
            *(u32*)(mgr + 0x820) = tail;
            if (tail == 0x40) *(u32*)(mgr + 0x820) = 0;
            if (*(u32*)(mgr + 0x820) == *(u32*)(mgr + 0x81C)) {
                u32 head = *(u32*)(mgr + 0x81C) + 1;
                *(u32*)(mgr + 0x81C) = head;
                if (head == 0x40) *(u32*)(mgr + 0x81C) = 0;
            }
            *(u32*)(dst + 0) = w0;
            *(u32*)(dst + 4) = w1;
            *(u32*)(dst + 8) = w2;
            *(u32*)(dst + 12) = w3;
        }
    }
    *(u32*)(mgr + 0x824) = *(u32*)(mgr + 0x820);
    *(u32*)(mgr + 4) = *(u32*)(mgr + 0xC);
}

}  // extern "C"
