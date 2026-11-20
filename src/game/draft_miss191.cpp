#include "types.h"
#include "game/part.h"
extern "C" {
#include "game/script_callbacks.h"
}

typedef s32 (*FnObjS32)(u8* obj);

extern "C" {
void     fn_8013AFF0(void);                                  // 0x8013AFF0
u32      fn_80143A5C(const char* s);                         // 0x80143A5C
void*    memset(void* p, int c, unsigned long n);
char*    strcpy(char* d, const char* s);
BOOL     fn_801E4B2C(u8* obj, u32* handle);                  // 0x801E4B2C
u32      fn_80220890(u8* data, s32 len);                     // CRC-32
s32      fn_80220A2C(void);
u8*      fn_801842C8(u8* a);                                 // 0x801842C8
void     fn_80184590(u8* a, u8* b);                          // 0x80184590

extern u32  lbl_805F9030;
extern u8   lbl_80556E80[];
extern u32  lbl_803F0E30[];
extern u32  lbl_803F0F30[];
extern s32  lbl_805FB35C;
extern u8   lbl_805FBBE0[];
extern u8   lbl_8047D248[];
extern u8   lbl_8047D348[];
extern void* lbl_805FB5F8;
extern u8   lbl_805FB7F0;
extern f32  lbl_805FB7F4;
extern u8   lbl_805FB7FC;
extern u8   lbl_805FB810;
extern u8   lbl_805F9608;
extern f32  lbl_805F960C;
extern f32  lbl_805FFEA8;
extern f32  lbl_805FFEAC;
extern f32  lbl_805FFEB0;
extern u8   lbl_805FB874[];
extern u8   lbl_805FB878[];
extern s32  lbl_805FB970;
extern s32  lbl_805FB974;
extern void* lbl_805FB964;
extern s32  lbl_805F9688;
extern u8   lbl_8048A8B4[];
extern u8   lbl_805FB8C8[];
}

extern "C" {

// 0x8010A49C
// Tests the record at p against kind (0x32..0x35) and, if it matches, runs fn_80220890 over it.
s32 fn_8010A49C(u8* p, s32 kind) {
    s32 match = 0;
    u32 want;
    s32 saved;
    s32 result;

    if (*(s32*)(p + 0x4) == fn_80220A2C()) {
        match = 1;
    }
    switch (kind) {
    case 0x32: want = 0x170; break;
    case 0x33:
    case 0x34: want = 0x30; break;
    case 0x35: want = 0x28; break;
    default: return 0;
    }
    result = 0;
    if (match != 0 && *(u32*)(p + 0xC) == want) {
        result = 1;
    }
    saved = *(s32*)(p + 0x14);
    *(s32*)(p + 0x14) = 0;
    fn_80220890(p, (s32)want);
    *(s32*)(p + 0x14) = saved;
    return result;
}

// 0x8011BC40
// Scans the save block's list of part slots and writes the index of every slot whose
// PartDef field at +0x18 equals target into out (s32 array). Returns the number written.
s32 fn_8011BC40(u8* out, s32 target) {
    u8 def[0x50];
    u8* save;
    s32 count = 0;
    s32 idx = 0;
    s32 i = 0;
    s32 n;

    save = *(u8**)lbl_805FBBE0;
    n = *(s32*)(save + 0x10EC);
    while (i < n) {
        u8* ent = *(u8**)(save + i * 4 + 0xA58);
        PartDbGetDef((PartDef*)def, gGameDb, *(s32*)ent);
        if (*(s32*)(def + 0x18) == target) {
            *(s32*)(out + idx) = i;
            count += 1;
            idx += 4;
        }
        i += 1;
    }
    return count;
}

// 0x8013C06C
// Stores the name string into the active slot record (+0x340), clears bit 0 of +0x35C and
// sets the flag bit for the four slots that follow the current list index.
void fn_8013C06C(u8* name) {
    u8* rec;
    s32 k;

    if (lbl_805F9030 != 0x40) {
        rec = lbl_80556E80 + (lbl_805F9030 << 5);
        strcpy((char*)(rec + 0x340), (const char*)name);
        rec = lbl_80556E80 + (lbl_805F9030 << 5);
        *(u32*)(rec + 0x35C) &= ~1u;
        *(u32*)(rec + 0x358) &= ~0u;
        SaveManager_SaveOptions();
        for (k = 0; k < 4; k++) {
            u32 slot = lbl_803F0E30[lbl_805FB35C + k];
            lbl_803F0F30[slot] |= 1;
        }
        fn_8013AFF0();
    }
}

// 0x801AA79C
// Copies the record at src into dst (fields +0x4, +0x8..+0x14), clears +0x0 (byte) and +0x18.
s32 fn_801AA79C(u8* dst, u8* src) {
    *(s32*)(dst + 0x8) = *(s32*)(src + 0x8);
    *(s32*)(dst + 0xC) = *(s32*)(src + 0xC);
    *(s32*)(dst + 0x10) = *(s32*)(src + 0x10);
    *(s32*)(dst + 0x14) = *(s32*)(src + 0x14);
    *(s32*)(dst + 0x18) = 0;
    *(u8*)(dst + 0x0) = 0;
    *(s32*)(dst + 0x4) = *(s32*)(src + 0x4);
    return 0;
}

// 0x801D5F98
// Advances the fader timer by dt (capped at lbl_805F960C) and writes the clamped, optionally
// inverted, fade factor to lbl_8047D248 +0xC.
void fn_801D5F98(f32 dt) {
    f32 v;
    f32 t;
    f32 f;

    if (lbl_805FB7F0 == 0) {
        return;
    }
    if (lbl_805FB7FC == 1) {
        return;
    }
    v = lbl_805FB7F4 + dt;
    lbl_805FB7F4 = v;
    if (v > lbl_805F960C) {
        v = lbl_805F960C;
        lbl_805FB7F4 = lbl_805F960C;
    }
    f = lbl_805FFEAC;
    if (lbl_805F960C > lbl_805FFEB0) {
        t = v / lbl_805F960C;
        if (t < lbl_805FFEA8) {
            f = lbl_805FFEA8;
        } else if (t > lbl_805FFEAC) {
            f = lbl_805FFEAC;
        } else {
            f = t;
        }
    }
    if (lbl_805F9608 == 1) {
        f = lbl_805FFEAC - f;
    }
    *(f32*)(lbl_8047D248 + 0xC) = f;
}

// 0x801D65CC
// Copies the four floats of lbl_8047D248 into out.
void fn_801D65CC(u8* out) {
    *(f32*)(out + 0x0) = *(f32*)(lbl_8047D248 + 0x0);
    *(f32*)(out + 0x4) = *(f32*)(lbl_8047D248 + 0x4);
    *(f32*)(out + 0x8) = *(f32*)(lbl_8047D248 + 0x8);
    *(f32*)(out + 0xC) = *(f32*)(lbl_8047D248 + 0xC);
}

// 0x801D6D78
// Fills lbl_8047D348 with the negated/copied floats of the matrix at lbl_805FB5F8 and returns it.
u8* fn_801D6D78(void) {
    u8* src;
    u8* d;

    if (lbl_805FB810 == 0) {
        lbl_805FB810 = 1;
    }
    src = (u8*)lbl_805FB5F8;
    d = lbl_8047D348;
    *(f32*)(d + 0x0) = -*(f32*)(src + 0x0);
    *(f32*)(d + 0x4) = -*(f32*)(src + 0x4);
    *(f32*)(d + 0x8) = -*(f32*)(src + 0x8);
    *(f32*)(d + 0xC) = *(f32*)(src + 0xC);
    *(f32*)(d + 0x10) = *(f32*)(src + 0x10);
    *(f32*)(d + 0x14) = *(f32*)(src + 0x14);
    *(f32*)(d + 0x18) = -*(f32*)(src + 0x18);
    *(f32*)(d + 0x1C) = -*(f32*)(src + 0x1C);
    *(f32*)(d + 0x20) = -*(f32*)(src + 0x20);
    *(f32*)(d + 0x24) = *(f32*)(src + 0x24);
    *(f32*)(d + 0x28) = *(f32*)(src + 0x28);
    *(f32*)(d + 0x2C) = *(f32*)(src + 0x2C);
    return d;
}

// 0x801DAA74
// If the owner pointer at a+0x4C matches the owner of b's node (+0x48) and the node type at
// +0x10 is 1 or 2, calls fn_80184590 on them.
void fn_801DAA74(u8* a, u8* b) {
    u8* owner;
    u8* node;
    s16 type;

    owner = *(u8**)(a + 0x4C);
    if (owner == NULL) {
        return;
    }
    node = *(u8**)(b + 0x48);
    if (node == NULL) {
        return;
    }
    if (fn_801842C8(*(u8**)(node + 0xC)) != owner) {
        return;
    }
    type = *(s16*)(node + 0x10);
    if (type == 1 || type == 2) {
        fn_80184590(owner, node);
    }
}

// 0x801DC3AC
// Returns 1 unless kind is not 0xF/0x10 and the hook at lbl_805FB874 rejects it.
s32 fn_801DC3AC(s32 kind) {
    s32 result = 1;
    void* hook;

    if (kind != 0xF && kind != 0x10) {
        s32 ok = 0;
        hook = *(void**)lbl_805FB874;
        if (hook != NULL && ((s32 (*)(s32))hook)(kind) != 0) {
            ok = 1;
        }
        if (ok == 0) {
            result = 0;
        }
    }
    return result;
}

// 0x801DC4D4
// Returns 1 for a fixed set of kinds; any other kind asks the hook at lbl_805FB878.
s32 fn_801DC4D4(s32 kind) {
    s32 result = 0;

    switch (kind) {
    case 0x2: case 0x3: case 0x4: case 0x5: case 0x6:
    case 0x11: case 0x20: case 0x28: case 0x2A:
        result = 1;
        break;
    default:
        if (*(void**)lbl_805FB878 != NULL && ((s32 (*)(s32))*(void**)lbl_805FB878)(kind) != 0) {
            result = 1;
        }
        break;
    }
    return result;
}

// 0x801DFAE0
// Returns lbl_805F9688 while the movie player is active and its virtual 0x34 reports ready.
s32 fn_801DFAE0(void) {
    s32 active = 0;
    u8* obj;

    if (lbl_805FB970 != 0 && lbl_805FB974 != 0) {
        obj = (u8*)lbl_805FB964;
        if (obj != NULL && (*(FnObjS32*)(*(u8**)obj + 0x34))(obj) != 0) {
            active = 1;
        }
    }
    if (active != 0) {
        return lbl_805F9688;
    }
    return 0;
}

// 0x801E5E90
// Looks up the table entry addressed by the handle at arg1 and marks it as loaded.
s32 fn_801E5E90(u8* arg0, u32* arg1) {
    u32 h = *arg1;
    u8* e = NULL;
    u32 sp8;

    if (h != 0xFFFFFFFFu) {
        e = *(u8**)(arg0 + 0x4) + (h & 0x7F) * 0x88;
        if ((h >> 7) != *(u32*)(e + 0x74)) {
            e = NULL;
        }
    }
    if (e != NULL) {
        if (*(s32*)(e + 0x4) == 2) {
            return 0;
        }
        if (*(s32*)(e + 0x78) != 0) {
            lbl_805FB8C8[0] = 0;
        }
        sp8 = *(u32*)(e + 0x50);
        fn_801E4B2C(lbl_8048A8B4, &sp8);
        *(s32*)(e + 0x70) = 1;
    }
    return 1;
}

// 0x801E6040
// Copies up to 0x38 bytes of name into a stack buffer, hashes it with fn_80143A5C and returns
// the value at +0x10 of the table entry whose key matches arg2 and whose hash at +0xC matches.
s32 fn_801E6040(u8* arg0, u8* name, s32 key) {
    u8 buf[0x40];
    u8* tbl;
    u8* base;
    u32 h;
    s32 i;
    s32 n;
    s32 j;
    u8* ent = NULL;

    tbl = *(u8**)(arg0 + 0x7B4);
    memset(buf + 8, 0, 0x38);
    i = 0;
    while (*name != 0 && i < 0x38) {
        buf[i] = *name;
        i += 1;
        name += 1;
    }
    buf[0x3F] = 0;
    h = fn_80143A5C((const char*)(buf + 8));

    n = *(s32*)(tbl + 0x4);
    base = *(u8**)(tbl + 0x0);
    for (j = 0; j < n; j++) {
        u8* e = base + j * 0x18;
        if (*(u32*)(e + 0x0) == (u32)key && *(u32*)(e + 0xC) == h) {
            ent = e;
            break;
        }
    }
    if (ent == NULL) {
        return 0;
    }
    return *(s32*)(ent + 0x10);
}

}  // extern "C"
