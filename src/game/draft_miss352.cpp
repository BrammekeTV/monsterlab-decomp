// Misc functions (script arg validation, line parsing, list teardown, small math) from the missing-function batch 352.
#include "types.h"
#include "engine/math.h"

extern "C" {
void*  MemAllocDefault(u32 size);                                              // 0x801D4650
f32    cosf(f32 x);                                                            // 0x8033EC98 (MSL)
f32    sinf(f32 x);                                                            // 0x8033F0A4 (MSL)
s32    fn_80138C40(void* world, f32* a, f32* b, f32* hit, void* p, s32 c, s32 d);  // 0x80138C40
void   fn_80121B08(void* node);                                                // 0x80121B08
void   fn_801EBA20(void* list, void* item);                                    // 0x801EBA20
char*  fn_80338A9C(char* s, const char* delim);                                // 0x80338A9C (strtok)
char*  fn_80338BC0(char* s, const char* sub);                                  // 0x80338BC0 (strstr)
s32    atoi(const char* s);                                             // 0x8033AB2C (atoi)
void*  memcpy(void* d, const void* s, unsigned long n);
u32    strlen(const char* s);

extern u8 gScriptSigs[];      // 0x34 bytes per signature
extern u8 gWorldState[];      // 0x803E6D00
extern u8 lbl_80348F38[];     // 0x80348F38 (dispatch tables: +0x24 set, +0x30 get, indexed by type byte * 4)
extern u8 lbl_803B4DD0[];     // 0x803B4DD0 (+0x38: locale block, +8: char class table)
extern u8 lbl_803C2A20[];     // 0x803C2A20
extern u8 lbl_803C4298[];     // 0x803C4298
extern s32 lbl_805FB708;      // 0x805FB708

typedef s32 (*TblGetFn)(void* obj, s32 key);
typedef void (*TblSetFn)(void* obj, s32 key, s32 val);

// Returns the char class flag 0x100 test for every character: nonzero when the line is empty or all
// characters have the flag set (blank line).
static s32 LineIsBlank(const char* buf) {
    u32 n = strlen(buf);
    u8* loc = *(u8**)(lbl_803B4DD0 + 0x38);
    s32 i;
    for (i = 0; i < (s32)n; i++) {
        s32 c = (s8)buf[i];
        s32 v;
        if (c < 0 || c >= 0x100) {
            v = 0;
        } else {
            v = *(u16*)(*(u8**)(loc + 8) + c * 2) & 0x100;
        }
        if (v == 0) {
            return 0;
        }
    }
    return 1;
}

// Start of the line after the one beginning at s (past the next '\n', or the terminator).
static char* NextLine(char* s) {
    char* p = fn_80338BC0(s, "\n");
    if (p == NULL) {
        p = s + strlen(s);
    }
    if (*p != 0) {
        p++;
    }
    return p;
}

// Copies the line at s (without '\n') into buf.
static void CopyLine(char* buf, char* s) {
    char* p = fn_80338BC0(s, "\n");
    s32 len;
    if (p == NULL) {
        p = s + strlen(s);
    }
    len = (s32)(p - s);
    memcpy(buf, s, len + 1);
    buf[len] = 0;
}

// 0x8003EA7C
// Initialises the 26-byte record slot for a new entry: bumps the count at key 0x3C, then writes
// the default values of the per-entry keys through the type's setter table.
void fn_8003EA7C(u8* arg0) {
    if (arg0 != NULL) {
        u8* tbl = lbl_80348F38 + *(u8*)(arg0 + 0xC) * 4;
        s32 n = (*(TblGetFn*)(tbl + 0x30))(*(void**)(arg0 + 0x8), 0x3C);
        s32 base;
        (*(TblSetFn*)(tbl + 0x24))(*(void**)(arg0 + 0x8), 0x3C, n + 1);
        base = n * 0x1A;
        (*(TblSetFn*)(tbl + 0x24))(*(void**)(arg0 + 0x8), base + 0x158, 0);
        (*(TblSetFn*)(tbl + 0x24))(*(void**)(arg0 + 0x8), base + 0x160, 0);
        (*(TblSetFn*)(tbl + 0x24))(*(void**)(arg0 + 0x8), base + 0x161, 0);
        (*(TblSetFn*)(tbl + 0x24))(*(void**)(arg0 + 0x8), base + 0x162, 0xF);
        (*(TblSetFn*)(tbl + 0x24))(*(void**)(arg0 + 0x8), base + 0x163, 0xF);
        (*(TblSetFn*)(tbl + 0x24))(*(void**)(arg0 + 0x8), base + 0x164, 0xF);
        (*(TblSetFn*)(tbl + 0x24))(*(void**)(arg0 + 0x8), base + 0x165, 0xF);
        (*(TblSetFn*)(tbl + 0x24))(*(void**)(arg0 + 0x8), base + 0x15C, 0);
        (*(TblSetFn*)(tbl + 0x24))(*(void**)(arg0 + 0x8), base + 0x169, 0);
        (*(TblSetFn*)(tbl + 0x24))(*(void**)(arg0 + 0x8), base + 0x16A, 0);
        (*(TblSetFn*)(tbl + 0x24))(*(void**)(arg0 + 0x8), base + 0x16B, 7);
        (*(TblSetFn*)(tbl + 0x24))(*(void**)(arg0 + 0x8), base + 0x16C, 7);
        (*(TblSetFn*)(tbl + 0x24))(*(void**)(arg0 + 0x8), base + 0x16D, 7);
        (*(TblSetFn*)(tbl + 0x24))(*(void**)(arg0 + 0x8), base + 0x16E, 6);
    }
}

// 0x800DE2AC
// Height of the ground below an actor's next position (negative = distance down), 0 if none.
f32 fn_800DE2AC(u8* arg0) {
    f32 hit[0x1B];
    u8* h = (u8*)hit;
    f32 a[3];
    f32 b[3];
    f32 res;
    f32 dx, dy, dz;
    u8* pos;

    if (arg0 == NULL) {
        return 0.0f;
    }
    pos = *(u8**)(arg0 + 0x4);
    if (*(u8*)(pos + 0x34) != 4) {
        return 0.0f;
    }
    dz = *(f32*)(arg0 + 0x858) + *(f32*)(arg0 + 0x84C);
    res = -1.0f;
    if (dz > -1.0f) {
        return 0.0f;
    }
    a[0] = *(f32*)(pos + 0x0);
    a[1] = *(f32*)(pos + 0x4);
    a[2] = *(f32*)(pos + 0x8);
    dx = *(f32*)(arg0 + 0x850) + *(f32*)(arg0 + 0x844);
    dy = *(f32*)(arg0 + 0x854) + *(f32*)(arg0 + 0x848);
    dz = *(f32*)(arg0 + 0x858) + *(f32*)(arg0 + 0x84C);
    b[0] = a[0] + dx * 50.0f;
    b[1] = a[1] + dy * 50.0f;
    b[2] = a[2] + dz * 50.0f;
    *(f32*)(h + 0x00) = 0.0f;
    *(f32*)(h + 0x04) = 0.0f;
    *(f32*)(h + 0x08) = 0.0f;
    *(f32*)(h + 0x0C) = 0.0f;
    *(f32*)(h + 0x10) = 0.0f;
    *(f32*)(h + 0x14) = 0.0f;
    *(f32*)(h + 0x18) = 0.0f;
    *(f32*)(h + 0x1C) = 0.0f;
    *(f32*)(h + 0x20) = 0.0f;
    *(f32*)(h + 0x24) = 0.0f;
    *(f32*)(h + 0x28) = 0.0f;
    *(f32*)(h + 0x2C) = 0.0f;
    *(f32*)(h + 0x30) = 0.0f;
    *(f32*)(h + 0x34) = 0.0f;
    *(f32*)(h + 0x38) = 0.0f;
    *(f32*)(h + 0x3C) = 0.0f;
    *(s32*)(h + 0x50) = 0;
    *(s32*)(h + 0x54) = 1;
    *(s32*)(h + 0x58) = 0;
    *(s32*)(h + 0x5C) = 0;
    *(s32*)(h + 0x60) = 0;
    *(s32*)(h + 0x64) = 0;
    *(s32*)(h + 0x68) = 0;
    if (fn_80138C40(gWorldState, a, b, hit, NULL, 0, 0) != 0) {
        res = a[2] - *(f32*)(h + 0x8);
    }
    if (res >= 0.0f) {
        res = 0.0f;
    }
    return res;
}

// 0x8010F880
// Checks that every pointer argument of script function `id` (mask in sig+0x10, per-arg type in
// sig+0x14) refers to an object whose flag for that arg type is set. Returns 1 when all pass.
s32 fn_8010F880(u32 arg0, s32 arg1, u8** arg2) {
    s32 ok = 1;
    u32 mask;
    u32 count;
    u32 i;
    u8* sig;
    s32 type;
    u8* obj;

    if (arg0 < 0x211U) {
        mask = *(u32*)(gScriptSigs + arg0 * 0x34 + 0x10);
    } else {
        mask = 0;
    }
    if ((u8)mask != 0) {
        ok = 1;
        if (arg0 < 0x211U) {
            count = *(u32*)(gScriptSigs + arg0 * 0x34 + 0x8);
        } else {
            count = 0;
        }
        sig = gScriptSigs + arg0 * 0x34;
        i = 0;
        while (ok != 0 && i < count) {
            if (mask & (1 << i)) {
                obj = arg2[i];
                if (obj != NULL) {
                    if (arg0 < 0x211U && i < *(u32*)(sig + 0x8)) {
                        type = *(s32*)(sig + i * 4 + 0x14);
                    } else {
                        type = 0;
                    }
                    if (type == 0xF) {
                        ok = 1;
                    } else if (type == 0x34) {
                        ok = (*(u32*)(*(u8**)(obj + 0x4) + 0x1C) >> 8) & 1;
                    } else if (type < 0x34) {
                        if (type == 0x2E) {
                            ok = (*(u32*)(*(u8**)(obj + 0x4) + 0x1C) >> 4) & 1;
                        } else if (type > 0x2E && type < 0x30) {
                            ok = (*(u32*)(*(u8**)(obj + 0x4) + 0x1C) >> 5) & 1;
                        } else if (type > 0x30 && type < 0x33) {
                            ok = (*(u32*)(*(u8**)(obj + 0x4) + 0x1C) >> 6) & 1;
                        } else {
                            ok = 0;
                        }
                    } else if (type == 0x55) {
                        ok = (*(u32*)(*(u8**)(obj + 0x4) + 0x1C) >> 13) & 1;
                    } else if (type == 0x3E) {
                        ok = (*(u32*)(*(u8**)(obj + 0x4) + 0x1C) >> 7) & 1;
                    } else if (type > 0x55 && type < 0x5B) {
                        ok = (*(u32*)(*(u8**)(obj + 0x4) + 0x1C) >> 6) & 1;
                    } else {
                        ok = 0;
                    }
                } else {
                    ok = 0;
                }
            }
            i++;
        }
    }
    return ok;
}

// 0x8012189C
// Tears down both effect lists: runs the per-node cleanup and unlinks each node.
void fn_8012189C(void) {
    u8* node;
    u8* next;
    u8* first;

    first = *(u8**)(*(u8**)(lbl_803C2A20 + 0x10) + 0x14);
    node = (first != NULL) ? first + 8 : NULL;
    while (node != NULL) {
        u8* link = *(u8**)(node - 4);
        next = (link != NULL) ? link + 8 : NULL;
        fn_80121B08(node);
        if (!((*(u32*)(lbl_803C4298 + 0x3C) >> 1) & 1)) {
            fn_801EBA20(*(void**)(lbl_803C2A20 + 0x10), node);
        }
        node = next;
    }
    if (!((*(u32*)(lbl_803C4298 + 0x3C) >> 1) & 1)) {
        *(s32*)(lbl_803C2A20 + 0x414) = 0;
    }
    first = *(u8**)(*(u8**)(lbl_803C2A20 + 0x418) + 0x14);
    node = (first != NULL) ? first + 8 : NULL;
    while (node != NULL) {
        u8* link = *(u8**)(node - 4);
        next = (link != NULL) ? link + 8 : NULL;
        fn_80121B08(node);
        fn_801EBA20(*(void**)(lbl_803C2A20 + 0x418), node);
        node = next;
    }
    *(s32*)(lbl_803C2A20 + 0x81C) = 0;
}

// 0x801A9A94
// Rotates vector arg2 by the three Euler angles (quarter turns) at arg1+0x17C/0x180/0x184,
// writing xyz to arg0 and copying w.
void fn_801A9A94(f32* arg0, u8* arg1, f32* arg2) {
    f32 v[3];
    f32 c, s;
    f32 ang;
    f32 x, y, z;

    v[0] = arg2[0];
    ang = 1.5707964f * *(f32*)(arg1 + 0x17C);
    v[1] = arg2[1];
    v[2] = arg2[2];
    c = cosf(ang);
    s = sinf(ang);
    y = v[1] * c + v[2] * -s;
    z = v[2] * c + v[1] * s;
    v[2] = z;
    v[1] = y;
    ang = 1.5707964f * *(f32*)(arg1 + 0x180);
    c = cosf(ang);
    s = sinf(ang);
    z = v[2] * c + v[0] * -s;
    x = v[0] * c + v[2] * s;
    v[0] = x;
    v[2] = z;
    ang = 1.5707964f * *(f32*)(arg1 + 0x184);
    c = cosf(ang);
    s = sinf(ang);
    arg0[3] = arg2[3];
    arg0[2] = v[2];
    y = v[1] * c + v[0] * s;
    x = v[0] * c + v[1] * -s;
    arg0[1] = y;
    arg0[0] = x;
    v[1] = y;
    v[0] = x;
}

// 0x801C5E30
// Counts the lines of a text blob (first line plus every line following a blank one).
void fn_801C5E30(char* arg0) {
    char buf[0x80];
    char* p = arg0;
    s32 count = 1;
    s32 first = 1;

    while (p != NULL && *p != 0) {
        CopyLine(buf, p);
        if (first != 0) {
            atoi(buf);
            first = 0;
            count++;
        }
        if (LineIsBlank(buf) != 0) {
            first = 1;
        }
        p = NextLine(p);
    }
    lbl_805FB708 = count - 1;
}

// 0x801C5F9C
// Parses a subtitle-style block: line 2 holds "h:m:s,ms-..." start and end times (written as
// floats to out+0 / out+4), then the text lines up to the next blank line are split into a
// pointer array (out+8, count at out+0xC). Returns the position after the block.
char* fn_801C5F9C(char* arg0, u8* arg1) {
    char buf[0x80];
    char* line2;
    char* line3;
    char* t0;
    char* t1;
    char* t2;
    char* t3;
    s32 secs;
    f32 base;
    s32 n;
    s32 i;
    char* p;
    char** arr;

    line2 = NextLine(arg0);
    line3 = NextLine(line2);
    t0 = fn_80338A9C(line2, ":");
    t1 = fn_80338A9C(NULL, ":");
    t2 = fn_80338A9C(NULL, ",");
    t3 = fn_80338A9C(NULL, "-");
    secs = atoi(t0) * 0x3C;
    secs = (atoi(t1) + secs) * 0x3C;
    base = (f32)(atoi(t2) + secs);
    *(f32*)(arg1 + 0x0) = 0.001f * (f32)atoi(t3) + base;
    fn_80338A9C(NULL, " ");
    t0 = fn_80338A9C(NULL, ":");
    t1 = fn_80338A9C(NULL, ":");
    t2 = fn_80338A9C(NULL, ",");
    t3 = fn_80338A9C(NULL, " \n");
    secs = atoi(t0) * 0x3C;
    secs = (atoi(t1) + secs) * 0x3C;
    base = (f32)(atoi(t2) + secs);
    p = line3;
    n = 0;
    *(f32*)(arg1 + 0x4) = 0.001f * (f32)atoi(t3) + base;
    for (;;) {
        CopyLine(buf, p);
        if (LineIsBlank(buf) != 0) {
            break;
        }
        n++;
        p = NextLine(p);
    }
    *(s32*)(arg1 + 0xC) = n;
    arr = (char**)MemAllocDefault(n * sizeof(char*));
    *(char***)(arg1 + 0x8) = arr;
    i = 0;
    p = line3;
    for (;;) {
        CopyLine(buf, p);
        if (LineIsBlank(buf) != 0) {
            break;
        }
        arr[i] = p;
        p = NextLine(p);
        if (*p != 0) {
            p[-1] = 0;
        }
        i++;
    }
    return p;
}

// 0x801CE944
// Scales the 0x10C..0x138 float block by 1/256 and sets +0x1E4 to the longest of the three
// vectors there plus the length of the vector at +0x8C.
void fn_801CE944(u8* arg0) {
    f32 k = 0.00390625f;
    f32 x0, y0, z0;
    f32 len0 = 0.0f;
    f32 len1 = 0.0f;
    f32 len2 = 0.0f;
    f32 len3 = 0.0f;
    f32 sq;
    f32 a, b, c;

    x0 = *(f32*)(arg0 + 0x10C) * k;
    y0 = *(f32*)(arg0 + 0x110) * k;
    z0 = *(f32*)(arg0 + 0x114) * k;
    sq = z0 * z0 + (x0 * x0 + y0 * y0);
    *(f32*)(arg0 + 0x10C) = x0;
    *(f32*)(arg0 + 0x110) = y0;
    *(f32*)(arg0 + 0x114) = z0;
    *(f32*)(arg0 + 0x118) = *(f32*)(arg0 + 0x118) * k;
    *(f32*)(arg0 + 0x11C) = *(f32*)(arg0 + 0x11C) * k;
    *(f32*)(arg0 + 0x120) = *(f32*)(arg0 + 0x120) * k;
    *(f32*)(arg0 + 0x124) = *(f32*)(arg0 + 0x124) * k;
    *(f32*)(arg0 + 0x128) = *(f32*)(arg0 + 0x128) * k;
    *(f32*)(arg0 + 0x12C) = *(f32*)(arg0 + 0x12C) * k;
    *(f32*)(arg0 + 0x130) = *(f32*)(arg0 + 0x130) * k;
    *(f32*)(arg0 + 0x134) = *(f32*)(arg0 + 0x134) * k;
    *(f32*)(arg0 + 0x138) = *(f32*)(arg0 + 0x138) * k;
    if (!(sq <= 0.0f)) {
        len0 = sqrtf(sq);
    }

    a = *(f32*)(arg0 + 0x11C);
    b = *(f32*)(arg0 + 0x120);
    c = *(f32*)(arg0 + 0x124);
    sq = c * c + (a * a + b * b);
    if (!(sq <= 0.0f)) {
        len1 = sqrtf(sq);
    }

    a = *(f32*)(arg0 + 0x12C);
    b = *(f32*)(arg0 + 0x130);
    c = *(f32*)(arg0 + 0x134);
    sq = c * c + (a * a + b * b);
    if (!(sq <= 0.0f)) {
        len2 = sqrtf(sq);
    }

    if (!(len0 > len1)) {
        len0 = len1;
    }
    *(f32*)(arg0 + 0x1E4) = len0;
    if (!(len0 > len2)) {
        len0 = len2;
    }

    a = *(f32*)(arg0 + 0x8C);
    b = *(f32*)(arg0 + 0x90);
    c = *(f32*)(arg0 + 0x94);
    sq = c * c + (a * a + b * b);
    *(f32*)(arg0 + 0x1E4) = len0;
    if (!(sq <= 0.0f)) {
        len3 = sqrtf(sq);
    }
    *(f32*)(arg0 + 0x1E4) = *(f32*)(arg0 + 0x1E4) + len3;
}

}  // extern "C"
