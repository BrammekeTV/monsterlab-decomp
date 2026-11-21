// Mixed leftovers: record constructors, tree insert, UI texture lookups, direction -> angles.
#include "types.h"
#include "game/part.h"

extern "C" {
u8*   fn_800DEC78(u8* arg0, s32 arg1);                          // 0x800DEC78
s32   fn_800D5DE4(u8* arg0, u8* arg1, s32 arg2);                // 0x800D5DE4
void  fn_800A2AEC(u8* node, u8* root);                          // 0x800A2AEC
void  fn_800A53B8(u8* dst, u8* src);                            // 0x800A53B8
void* fn_801D40E0(s32 n);                                       // 0x801D40E0
s32   fn_80336BF8(u8* file, const char* fmt, ...);              // 0x80336BF8
void  fn_8033B09C(void);                                        // 0x8033B09C
f64   fn_8033E8EC(f64 x);                                       // 0x8033E8EC
f64   fn_8033F1F8(f64 x);                                       // 0x8033F1F8
void  operator_delete(void* p);                                 // 0x801D426C
void* fn_801094F4(char* name, s32 flags);                       // 0x801094F4
void  PartAnimKey(char* out, s32 limb, s32 defId);              // 0x802152FC
int   SysSprintf(char* buf, const char* fmt, ...);              // 0x80142EFC
void* memset(void* p, int c, unsigned long n);
char* strcat(char* dst, const char* src);
char* strcpy(char* d, const char* s);

extern u8 __files[];                // 0x803B4978
extern u8 lbl_8035CEF8[];           // 0x8035CEF8 (vtable)
extern u8 lbl_8035CF68[];           // 0x8035CF68 (vtable of the subobject at +0x98)
extern u8 lbl_80360200[];           // 0x80360200 (vtable)
extern const char* lbl_805F8F10;    // 0x805F8F10 ("ui/msa_ingredients/")
extern const char* lbl_805F8F50;    // 0x805F8F50

// 0x8009E818
// Constructor: base init with kind 0x11, sets the vtables and clears the fields.
void fn_8009E818(u8* p) {
    fn_800DEC78(p, 0x11);
    *(void**)p = lbl_8035CEF8;
    *(s32*)(p + 0x58) = 0;
    *(void**)(p + 0x98) = lbl_8035CF68;
    *(s32*)(p + 0x9C) = 0;
    *(s32*)(p + 0xA0) = 0;
    *(s32*)(p + 0xA4) = 0;
    *(s32*)(p + 0xA8) = 0;
    *(s32*)(p + 0xAC) = 0;
    *(s32*)(p + 0xB0) = 1;
}

// 0x800E5550
// Constructor: base init with kind 0x10, sets the vtable, zeroes the float block, scale = 1.
void fn_800E5550(u8* p) {
    fn_800DEC78(p, 0x10);
    *(f32*)(p + 0x68) = 0.0f;
    *(void**)p = lbl_80360200;
    *(f32*)(p + 0x6C) = 0.0f;
    *(f32*)(p + 0x70) = 0.0f;
    *(f32*)(p + 0x74) = 0.0f;
    *(f32*)(p + 0x78) = 0.0f;
    *(f32*)(p + 0x7C) = 0.0f;
    *(f32*)(p + 0x80) = 0.0f;
    *(f32*)(p + 0x84) = 0.0f;
    *(f32*)(p + 0x88) = 0.0f;
    *(f32*)(p + 0x98) = 0.0f;
    *(f32*)(p + 0x9C) = 0.0f;
    *(f32*)(p + 0xA0) = 0.0f;
    *(u8*)(p + 0xA4) = 0;
    *(s32*)(p + 0x64) = 0;
    *(s32*)(p + 0x60) = 0;
    *(s32*)(p + 0x5C) = 0;
    *(s32*)(p + 0x58) = 0;
    *(f32*)(p + 0x44) = 1.0f;
}

// 0x800A526C
// Red-black tree insert: allocates a 0x1C-byte node (key copied to +0xC, payload at +0x10),
// links it under 'parent' (left if isLeft, else right), bumps the count, rebalances against the
// root, and optionally records it as the head (tree +0xC).
u8* fn_800A526C(u8* tree, u8* parent, s32 isLeft, s32 setHead, u8* key) {
    u8* guard;
    u8* node;
    u8* sub;
    if ((u32)(*(s32*)tree + 0x10000) == 0xFFFFu) {
        fn_80336BF8(__files + 0xA0, "tree::insert length error");
        fn_8033B09C();
    }
    node = (u8*)fn_801D40E0(0x1C);
    if (node == NULL) {
        fn_80336BF8(__files + 0xA0, "Memory allocation failure");
        fn_8033B09C();
    }
    guard = node;
    sub = node + 0xC;
    if (sub != NULL) {
        *(s32*)(node + 0xC) = *(s32*)key;
        *(s32*)(sub + 0x4) = 0;
        *(s32*)(sub + 0x8) = 0;
        *(u8**)(sub + 0xC) = sub + 8;
        fn_800A53B8(sub + 4, key + 4);
    }
    node = guard;
    *(s32*)(node + 0x4) = 0;
    guard = NULL;
    *(s32*)(node + 0x0) = 0;
    if (node + 8 != NULL) *(u8**)(node + 8) = parent;
    if (isLeft != 0) *(u8**)(parent + 0x0) = node;
    else             *(u8**)(parent + 0x4) = node;
    *(s32*)tree = *(s32*)tree + 1;
    fn_800A2AEC(node, *(u8**)(tree + 4));
    if (setHead != 0) *(u8**)(tree + 0xC) = node;
    if (guard != NULL) operator_delete(guard);
    return node;
}

// 0x80118858
// Looks up the ingredient icon texture for a record: kind 0 uses the table at db+0x4F00 (0x10-byte
// entries), kind 1 the table at db+0x5328 (0x18-byte entries). Falls back to the placeholder icon.
void* fn_80118858(s32* arg0) {
    char buf[0x100];
    u8* rec;
    void* tex;
    s32 kind = arg0[0];
    if (kind == 0) {
        rec = *(u8**)(gGameDb->data + 0x4F00) + (arg0[1] << 4);
        SysSprintf(buf, "%sinventory_icon_%03d", lbl_805F8F10, (s32)*(s8*)(rec + 0xA));
        tex = fn_801094F4(buf, 0);
        if (tex == NULL) {
            SysSprintf(buf, "%sinventory_icon_placeholder", lbl_805F8F10);
            return fn_801094F4(buf, 0);
        }
        return tex;
    }
    if (kind == 1) {
        rec = *(u8**)(gGameDb->data + 0x5328) + arg0[1] * 0x18;
        SysSprintf(buf, "%sinventory_icon_sec_%03d", lbl_805F8F10, (s32)*(s8*)(rec + 0x12));
        tex = fn_801094F4(buf, 0);
        if (tex == NULL) {
            SysSprintf(buf, "%sinventory_icon_placeholder", lbl_805F8F10);
            return fn_801094F4(buf, 0);
        }
        return tex;
    }
    return NULL;
}

// 0x8011AE30
// Looks up the part model "ui/msa_parts/<animkey>[_h]" (highlight variant when arg1 != 0),
// falling back to "ui/msa_parts/placeholder".
void* fn_8011AE30(PartDef* arg0, s32 arg1) {
    char key[0x100];
    char path[0x100];
    void* r;
    PartAnimKey(key, arg0->slot, arg0->index);
    strcpy(path, "ui/msa_parts/");
    strcat(path, key);
    if (arg1 != 0) strcat(path, "_h");
    r = fn_801094F4(path, 0);
    if (r == NULL) {
        strcpy(path, "ui/msa_parts/");
        strcat(path, "placeholder");
        r = fn_801094F4(path, 0);
    }
    return r;
}

// 0x8011CC5C
// Looks up the badge texture "<prefix>inventory_badge_%03d" for entry idx (0x18-byte records at
// db+0x10A8, id at +0x14), falling back to the placeholder badge.
void* fn_8011CC5C(s32 idx) {
    char buf[0x100];
    u8* rec = *(u8**)(gGameDb->data + 0x10A8) + idx * 0x18;
    void* r;
    SysSprintf(buf, "%sinventory_badge_%03d", lbl_805F8F50, (s32)*(s16*)(rec + 0x14));
    r = fn_801094F4(buf, 0);
    if (r == NULL) {
        SysSprintf(buf, "%sinventory_badge_placeholder", lbl_805F8F50);
        r = fn_801094F4(buf, 0);
    }
    return r;
}

// Angle of (x, y) in half-turns: atan(y/x) * 2/pi, shifted by -2 for the lower half-plane.
static f32 HalfTurns(f32 x, f32 y) {
    f32 a = 1.0f;
    if (x != 0.0f) a = 0.63661975f * (f32)fn_8033E8EC((f64)(y / x));
    if (x < 0.0f) a -= 2.0f;
    else if (x == 0.0f && y < 0.0f) a -= 2.0f;
    return a;
}

// 0x801449A4
// Converts a 3x3 rotation matrix (9 floats, row-major) into three normalised angles out[0..2];
// returns 1 normally, 0 in the two singular cases (m[2] <= -1 or m[2] >= 1).
s32 fn_801449A4(f32* m, f32* out) {
    f32 a, b, c;
    if (m[2] < 1.0f) {
        if (m[2] > -1.0f) {
            f32 t = -m[2];
            f32 s;
            a = HalfTurns(m[0], m[1]);
            if (t < -1.0f) s = -1.0f;
            else if (t > 1.0f) s = 1.0f;
            else s = t;
            b = 0.63661975f * (f32)fn_8033F1F8((f64)s);
            c = HalfTurns(m[8], m[5]);
            out[0] = c;
            out[1] = b;
            out[2] = a;
            return 1;
        }
        a = HalfTurns(m[6], m[3]);
        out[0] = 0.0f;
        out[1] = 1.0f;
        out[2] = -a;
        return 0;
    }
    a = HalfTurns(-m[6], -m[3]);
    out[0] = 0.0f;
    out[1] = -1.0f;
    out[2] = a;
    return 0;
}

// 0x800D63CC
// Picks a target record from the owner at *arg0 (+0x14, else +0x10's +4/+8 chosen by flag +4),
// clears the 0x3FC-byte work area, installs it and runs fn_800D5DE4; 0 if none or flagged (+0x64).
s32 fn_800D63CC(u8* arg0, s32 arg1) {
    u8* owner = *(u8**)arg0;
    u8* t = *(u8**)(owner + 0x14);
    u8* alt;
    if (t == NULL) {
        alt = *(u8**)(owner + 0x10);
        if (alt != NULL) {
            if (*(u8*)(owner + 0x4) != 0) t = *(u8**)(alt + 4);
            else                           t = *(u8**)(alt + 8);
        }
    }
    if (t == NULL) return 0;
    if (*(u8*)(t + 0x64) != 0) return 0;
    memset(arg0 + 4, 0, 0x3FC);
    *(u8**)(arg0 + 4) = t;
    *(s32*)(arg0 + 0x400) = 1;
    return fn_800D5DE4(arg0, t, arg1);
}

}  // extern "C"
