// Small functions from the draft_miss302 batch (0x800DB944, 0x801CA940, 0x801CCE08, 0x801CCF00,
// 0x801D7728, 0x801D7924, 0x801D81FC, 0x801DB1AC).
#include "types.h"
#include <math.h>

// Script function table entry (12 bytes): type, handler, hash.
struct ScriptFunc { u32 type; void* fn; u32 hash; };   // 0xC bytes

extern "C++" ScriptFunc gScriptFuncs[];   // 0x80484230
extern "C++" u8 sRendDLs[];               // 0x80401108 (display-list records, 0x94 bytes each)

extern "C" {
s32  fn_8015BA30(void* p);                                              // 0x8015BA30
void fn_8014DE68(void);                                                 // 0x8014DE68
void fn_80151E38(s32 quads, void* verts);                               // 0x80151E38
void fn_801EBA20(void* list, void* item);                               // 0x801EBA20
void fn_801D79D4(u8* self, u8* node, u8* vec, s32 a, u8 b, f32 c, f32 d);  // 0x801D79D4
void fn_801D82AC(u8* self, u8* node, u8* vec, s32 a, u8 b, f32 c, f32 d);  // 0x801D82AC
void fn_801EBC74(u8* a, void* fn, u8* entry);                           // 0x801EBC74
s32  fn_801EBBE0(u8* a, void* fn, s32 entry);                           // 0x801EBBE0

extern u8*  lbl_805FB81C;                 // 0x805FB81C (list head object)
extern u8*  lbl_805FB820;                 // 0x805FB820 (list head object)
extern u8*  lbl_805FB828;                 // 0x805FB828 (list head object)
extern u8   lbl_803C4298[];               // 0x803C4298 (level info block)
extern s32  lbl_805FB4BC;                 // 0x805FB4BC (current viewport index)
extern s32  lbl_805FB4C0;                 // 0x805FB4C0 (display-list recording flag)
extern s32  lbl_805FB4C8;                 // 0x805FB4C8 (render budget)
}

extern "C" {

// 0x800DB944
// Copies the position (3 floats) of the current entry of the level info block into arg0.
// Returns 1 when a block is present, 0 otherwise.
s32 fn_800DB944(f32* out, u8* obj) {
    u8* p = *(u8**)(lbl_803C4298 + 0x4);
    if (p == NULL) {
        return 0;
    }
    p = *(u8**)(p + 0x4);
    out[0] = *(f32*)(p + 0x0);
    out[1] = *(f32*)(p + 0x4);
    out[2] = *(f32*)(p + 0x8);
    return 1;
}

// Checks the render budget for the current display-list record; flushes if it is over.
static void CheckRendBudget(void) {
    if (lbl_805FB4C0 != 0 && (fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x10C0) > lbl_805FB4C8) {
        fn_8014DE68();
    }
}

// 0x801CA940
// Renders a range of 24-byte records (96 bytes = 4 records per call) between arg0 and arg1,
// with the render budget checked before and after.
void fn_801CA940(u8* arg0, u8* arg1) {
    s32 diff = (s32)(arg1 - arg0);
    if (diff / 24 > 0) {
        CheckRendBudget();
        fn_80151E38(diff / 96, arg0);
        CheckRendBudget();
    }
}

// Clamps a float channel value (already scaled to 0..255) to a byte.
static s32 ClampByte(s32 v) {
    if (v < 0) {
        return 0;
    }
    if (v > 0xFF) {
        return 0xFF;
    }
    return v;
}

// 0x801CCE08
// Packs four float channels (0..1, arg0[0..3]) into one 32-bit colour, each clamped to a byte.
u32 fn_801CCE08(f32* arg0) {
    s32 c0 = ClampByte((s32)(255.0f * arg0[0]));
    s32 c1 = ClampByte((s32)(255.0f * arg0[1]));
    s32 c2 = ClampByte((s32)(255.0f * arg0[2]));
    s32 c3 = ClampByte((s32)(255.0f * arg0[3]));
    return (u32)c0 | ((u32)c1 << 8) | ((u32)c2 << 16) | ((u32)c3 << 24);
}

// 0x801CCF00
// Phase to wave: arg0 is scaled by 0.25 and folded around its nearest integer, then two
// parabolic-sine samples are written: arg2 for the folded value, arg1 for the same value
// shifted by a quarter cycle and wrapped back into range.
// Constants (0.25, 4, 2, -1, 1) are taken from the m2c draft; not checked against the data.
void fn_801CCF00(f32* arg0, f32* arg1, f32* arg2) {
    f32 t = *arg0 * 0.25f;
    s32 n = (s32)(t < 0.0f ? t - 0.5f : t + 0.5f);   // round to nearest, as fctiw does
    f32 u = (t - (f32)n) * 4.0f;
    f32 w = u + 1.0f;
    if (w > 2.0f) {
        w -= 4.0f;
    }
    *arg1 = (2.0f * w) + (-1.0f * w * (f32)fabs(w));
    *arg2 = (2.0f * u) + (-1.0f * u * (f32)fabs(u));
}

// 0x801D7728
// Walks the node list hanging off lbl_805FB81C and calls fn_801EBA20 on every node.
// arg0 is not used by the original.
void fn_801D7728(u8* arg0) {
    if (lbl_805FB81C == NULL) {
        return;
    }
    u8* head = *(u8**)(lbl_805FB81C + 0x14);
    u8* node = (head != NULL) ? head + 0x8 : NULL;
    while (node != NULL) {
        u8* raw = *(u8**)(node - 0x4);
        u8* next = (raw != NULL) ? raw + 0x8 : NULL;
        fn_801EBA20(lbl_805FB81C, node);
        node = next;
    }
}

// 0x801D7924
// Walks the node list hanging off lbl_805FB820 and runs fn_801D79D4 (with the node's fields)
// and then removes each node with fn_801EBA20.
void fn_801D7924(u8* arg0) {
    if (lbl_805FB820 == NULL) {
        return;
    }
    u8* head = *(u8**)(lbl_805FB820 + 0x14);
    u8* node = (head != NULL) ? head + 0x8 : NULL;
    while (node != NULL) {
        u8* raw = *(u8**)(node - 0x4);
        u8* next = (raw != NULL) ? raw + 0x8 : NULL;
        fn_801D79D4(arg0, node, node + 0xC, *(s32*)(node + 0x20), *(u8*)(node + 0x24),
                    *(f32*)(node + 0x18), *(f32*)(node + 0x1C));
        fn_801EBA20(lbl_805FB820, node);
        node = next;
    }
}

// 0x801D81FC
// Same walk as fn_801D7924 over the list at lbl_805FB828, calling fn_801D82AC instead.
void fn_801D81FC(u8* arg0) {
    if (lbl_805FB828 == NULL) {
        return;
    }
    u8* head = *(u8**)(lbl_805FB828 + 0x14);
    u8* node = (head != NULL) ? head + 0x8 : NULL;
    while (node != NULL) {
        u8* raw = *(u8**)(node - 0x4);
        u8* next = (raw != NULL) ? raw + 0x8 : NULL;
        fn_801D82AC(arg0, node, node + 0xC, *(s32*)(node + 0x20), *(u8*)(node + 0x24),
                    *(f32*)(node + 0x18), *(f32*)(node + 0x1C));
        fn_801EBA20(lbl_805FB828, node);
        node = next;
    }
}

// 0x801DB1AC
// Walks the table at arg0 (count at +4, base pointer at +0, 0x2C-byte entries) and, for every
// entry whose tag is not -1, calls fn_801EBC74 and fn_801EBBE0 with the script handler of that tag.
void fn_801DB1AC(u8* arg0) {
    u32 count = *(u32*)(arg0 + 0x4);
    u8* base = *(u8**)arg0;
    for (u32 i = 0; i < count; i++) {
        u8* entry = base + i * 0x2C;
        s32 tag = *(s32*)(entry + 0xC);
        if (tag != -1) {
            void* fnp = gScriptFuncs[tag].fn;
            fn_801EBC74(*(u8**)(entry + 0x4), fnp, entry);
            fn_801EBBE0(*(u8**)(entry + 0x4), fnp, (s32)(intptr_t)entry);
        }
    }
}

}  // extern "C"
