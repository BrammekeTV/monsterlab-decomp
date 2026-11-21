// Drafts miss392: bracket-matching record builder (0x80174788), GX light-direction upload
// (0x80175A0C), two display-list sort routines (0x80182B64 / 0x80182FAC) and the draw-list
// executor (0x801833B8).
#include "types.h"
#include "revolution.h"

extern "C" {
u8*  fn_8016B50C(s32 kind, s32 count);                        // 0x8016B50C
void fn_80155724(u8* m);                                      // 0x80155724
void fn_801561C0(f32 scale);                                  // 0x801561C0
void fn_801727EC(void* handle, s32 idx);                      // 0x801727EC
void fn_80175A0C(s32 idx, const f32* dir);                    // 0x80175A0C
void fn_801873C0(void* a, s32 b, s32 c);                      // 0x801873C0
void fn_8014DE68(void);                                       // 0x8014DE68
s32  fn_8015BA30(void* p);                                    // 0x8015BA30

extern u8   lbl_805F9339;           // 0x805F9339 (draw enabled)
extern u8   lbl_80348F38[];         // 0x80348F38 (per-type dispatch table)
extern u8   lbl_80400FF0[];         // 0x80400FF0 (GX state block)
extern f32  lbl_804010D8[12];       // 0x804010D8 (3x4 matrix)
extern u8   lbl_804129E8[];         // 0x804129E8 (draw statistics)
extern s32  lbl_805FB4BC;           // 0x805FB4BC (current display list)
extern s32  lbl_805FB4C0;           // 0x805FB4C0 (display list open)
extern s32  lbl_805FB4C8;           // 0x805FB4C8 (render budget)
extern u8   lbl_805FB50C;           // 0x805FB50C (GX shadow flags byte)
extern u8   lbl_805FB50D;           // 0x805FB50D (GX shadow mask byte)
extern u8*  lbl_805FB608;           // 0x805FB608 (allocator object, slot +0 = alloc)
extern u8*  lbl_805FB610;           // 0x805FB610 (object being drawn)
extern f32  lbl_805FF6B0;           // 0x805FF6B0 (0.0)
extern f32  lbl_805FF6B4;           // 0x805FF6B4 (1.0)
extern f32  lbl_805FF6E4;           // 0x805FF6E4 (direction scale)
extern f32  lbl_805FF770;           // 0x805FF770 (direction scale)
extern f32  lbl_805FF774;           // 0x805FF774 (0.0)
extern f32  lbl_805FF778;           // 0x805FF778 (1.0)
extern f32  lbl_805FF77C;           // 0x805FF77C (255.0)
}
extern "C++" u8 sRendDLs[];         // 0x80401108 (display-list records, 0x94 bytes each)

typedef void (*DrawFn)(void* a, void* b);

// Writes the shadow mask to the GX pipe when the flag byte says so.
static inline void SyncMask(u8 mask) {
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 0;
    GXWGFifo._u16 = 0x100e;
    GXWGFifo._u32 = ((u32)mask << 2) | 0x742;
}

// Sets (bit) in the shadow mask byte and forwards it.
static inline void MaskSet(u8 bit) {
    u8 flags = lbl_805FB50C;
    u8 m = (u8)(lbl_805FB50D | bit);
    lbl_805FB50D = m;
    if (flags & 1) SyncMask(m);
}

// Clears (bit) in the shadow mask byte and forwards it.
static inline void MaskClear(u8 bit) {
    u8 flags = lbl_805FB50C;
    u8 m = (u8)(lbl_805FB50D & ~bit);
    lbl_805FB50D = m;
    if (flags & 1) SyncMask(m);
}

// Sort order of the first routine: signed key at +8, ties by the float at +0.
static inline bool KeyLess(u8* a, u8* b) {
    s32 ka = *(s32*)(a + 8);
    s32 kb = *(s32*)(b + 8);
    if (ka == kb) return *(f32*)a < *(f32*)b;
    return ka < kb;
}

// Sort order of the second routine: float at +0, ties by record address.
static inline bool FloatLess(u8* a, u8* b) {
    f32 fa = *(f32*)a;
    f32 fb = *(f32*)b;
    if (fa == fb) return a < b;
    return fa < fb;
}

// Quicksort (median of three, explicit range stack) down to ranges of at most 5 elements,
// then a selection step for the minimum and an insertion sort. Shared body of both sorts.
template <class L>
static void SortPtrs(u8** arr, u32 count, L less) {
    u8* stk[32];
    u8** sp = stk + 2;
    u8** lo = arr;
    u8** hi = arr + count - 1;
    stk[0] = NULL;
    stk[1] = NULL;
    if (count > 4) {
        while (stk < sp) {
            u8** i;
            u8** j;
            u8** pm = lo + (hi - lo) / 2;
            u8* x = *lo;
            u8* y = *pm;
            u8* P;
            u8* E;
            u8* G;
            s32 n1;
            s32 n2;
            if (less(y, x)) {
                *pm = x;
                *lo = y;
            }
            x = *pm;
            y = *hi;
            if (less(y, x)) {
                *pm = y;
                *hi = x;
                x = *lo;
                y = *pm;
                if (less(y, x)) {
                    *pm = x;
                    *lo = y;
                }
            }
            i = lo + 1;
            j = hi - 1;
            for (;;) {
                for (;;) {
                    P = *pm;
                    E = *i;
                    if (!less(E, P)) break;
                    i++;
                }
                for (;;) {
                    G = *j;
                    if (!less(P, G)) break;
                    j--;
                }
                if (i < j) {
                    *i = G;
                    *j = E;
                    if (pm == i) {
                        pm = j;
                    } else if (pm == j) {
                        pm = i;
                    }
                    i++;
                    j--;
                    if (i <= j) continue;
                    break;
                }
                if (i == j) {
                    i++;
                    j--;
                }
                break;
            }
            n1 = (s32)(j - lo);
            if (n1 <= 4) {
                if ((s32)(hi - i) <= 4) {
                    lo = (u8**)sp[-1];
                    hi = (u8**)sp[-2];
                    sp -= 2;
                } else {
                    lo = i;
                }
            } else {
                n2 = (s32)(hi - i);
                if (n2 <= 4) {
                    hi = j;
                } else if (n1 > n2) {
                    sp[1] = (u8*)lo;
                    lo = i;
                    sp[0] = (u8*)j;
                    sp += 2;
                } else {
                    sp[1] = (u8*)i;
                    sp[0] = (u8*)hi;
                    hi = j;
                    sp += 2;
                }
            }
        }
    }
    {
        u8** last = arr + count - 1;
        u8** lim = arr + 4;
        u8** m = arr;
        u8** p;
        u8** q;
        if (lim > last) lim = last;
        for (p = arr + 1; p <= lim; p++) {
            if (less(*p, *m)) m = p;
        }
        if (m != arr) {
            u8* t = *m;
            *m = *arr;
            *arr = t;
        }
        for (q = arr + 2; q <= last; q++) {
            u8* X = *q;
            u8** r = q - 1;
            u8** ins;
            while (less(X, *r)) r--;
            ins = r + 1;
            if (ins != q) {
                u8** s = q + 1;
                for (;;) {
                    u8** d;
                    u8** w;
                    s--;
                    if (!(s >= q)) break;
                    X = *s;
                    d = s;
                    w = s;
                    for (;;) {
                        d--;
                        if (!(d >= ins)) break;
                        *w = *d;
                        w = d;
                    }
                    *w = X;
                }
            }
        }
    }
}

extern "C" {

// 0x80174788
// Scans nested begin/end records (type 1 pushes, 2 pops, 3 sets the current parent) of a script
// block, then builds a 0x24-byte descriptor over the records that form the first balanced run.
u8* fn_80174788(u8* arg0, s32 arg1) {
    s32 stack[32];
    s32* sp = stack;
    u8* rec = *(u8**)(arg0 + 0xC);
    s32 parent = -1;
    s32 idx = 0;
    s32 cnt;
    s32 depth;
    s32 total;
    u8* out;
    u8* scan;
    s32 ctr = arg1;
    if (arg1 > 0) {
        do {
            switch (*(s32*)(rec + 4)) {
            case 1:
                *sp = parent;
                parent = idx;
                sp++;
                break;
            case 2:
                sp--;
                parent = *sp;
                break;
            case 3:
                parent = idx;
                break;
            default:
                break;
            }
            rec += 8;
            idx++;
            ctr--;
        } while (ctr != 0);
    }
    cnt = 0;
    depth = 0;
    scan = *(u8**)(arg0 + 0xC) + arg1 * 8;
    while (depth >= 0) {
        s32 t = *(s32*)(scan + 4);
        if (t & 1) {
            if (cnt != 0 && !(t & 2)) depth++;
        } else if (t & 2) {
            depth--;
        }
        scan += 8;
        cnt++;
    }
    total = cnt;
    out = (*(u8*(**)(u32, u32, u32, u32, u32))lbl_805FB608)(0x24, 0x10, 0x10, 0, 0);
    *(s32*)(out + 0x20) = 0x52;
    *(s32*)(out + 0x8) = total;
    *(u8**)(out + 0x18) = (*(u8*(**)(u32, u32, u32, u32, u32))lbl_805FB608)(total * 0x60, 0x10, 0x11, 0, 0);
    *(u8**)(out + 0x0) = *(u8**)(arg0 + 0x0) + arg1 * 0x30;
    *(u8**)(out + 0x4) = *(u8**)(arg0 + 0x4) + arg1 * 0x30;
    *(u8**)(out + 0xC) = *(u8**)(arg0 + 0xC) + arg1 * 8;
    *(u8**)(out + 0x14) = fn_8016B50C(0, total);
    *(s32*)(out + 0x10) = 0;
    *(u8**)(out + 0x1C) = (parent >= 0) ? *(u8**)(arg0 + 0x4) + parent * 0x30 : NULL;
    return out;
}

// 0x80175A0C
// Sends light direction `dir` (transformed by the 3x3 part of lbl_804010D8 and scaled) to the
// GX transform unit as register block `idx`, followed by three constant vectors.
void fn_80175A0C(s32 idx, const f32* dir) {
    s32 base = idx * 0x10;
    f32 x = dir[0];
    f32 y = dir[1];
    f32 z = dir[2];
    const f32* m = lbl_804010D8;
    f32 tx = (m[6] * z) + ((m[0] * x) + (m[3] * y));
    f32 ty = (m[7] * z) + ((m[1] * x) + (m[4] * y));
    f32 tz = (m[8] * z) + ((m[2] * x) + (m[5] * y));
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 2;
    GXWGFifo._u16 = (u16)(base + 0x60A);
    GXWGFifo._f32 = tx * lbl_805FF6E4;
    GXWGFifo._f32 = ty * lbl_805FF6E4;
    GXWGFifo._f32 = tz * lbl_805FF6E4;
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 2;
    GXWGFifo._u16 = (u16)(base + 0x60D);
    GXWGFifo._f32 = lbl_805FF6B0;
    GXWGFifo._f32 = lbl_805FF6B0;
    GXWGFifo._f32 = lbl_805FF6B4;
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 2;
    GXWGFifo._u16 = (u16)(base + 0x604);
    GXWGFifo._f32 = lbl_805FF6B4;
    GXWGFifo._f32 = lbl_805FF6B0;
    GXWGFifo._f32 = lbl_805FF6B0;
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 2;
    GXWGFifo._u16 = (u16)(base + 0x607);
    GXWGFifo._f32 = lbl_805FF6B4;
    GXWGFifo._f32 = lbl_805FF6B0;
    GXWGFifo._f32 = lbl_805FF6B0;
}

// 0x80182B64
// Sorts an array of record pointers by the s32 key at +8, ties broken by the f32 at +0.
void fn_80182B64(u8** arr, u32 count) {
    SortPtrs(arr, count, KeyLess);
}

// 0x80182FAC
// Sorts an array of record pointers by the f32 at +0, ties broken by address.
void fn_80182FAC(u8** arr, u32 count) {
    SortPtrs(arr, count, FloatLess);
}

// 0x801833B8
// Executes a list of draw entries: for each one binds the matrix (+0x10 / +0x18), the three
// texture slots of the record at +0x1C, the light vector at +0x20 (uploaded with its colour),
// then runs the type's pre-draw and post-draw hooks around fn_801873C0.
void fn_801833B8(u8* ctx, u8** list, s32 count) {
    s32 i;
    if (lbl_805F9339 == 0) return;
    *(u8**)(ctx + 0x3C) = NULL;
    *(u8**)(ctx + 0x40) = NULL;
    *(u8**)(ctx + 0x44) = NULL;
    MaskClear(1);
    MaskClear(2);
    MaskClear(4);
    MaskClear(8);
    *(void**)(ctx + 0x48) = NULL;
    *(void**)(ctx + 0x4C) = NULL;
    *(void**)(ctx + 0x50) = NULL;
    for (i = 0; i < count; i++) {
        u8* e = list[i];
        u8* rec;
        u8* vec;
        u8* inst;
        u8* tbl;
        u8* mdl;
        u8 savedByte;
        u8* m;
        if (*(u8**)(e + 4) == NULL) {
            (*(void (**)(void*))(e + 8))(*(void**)(e + 0xC));
            continue;
        }
        m = *(u8**)(e + 0x10);
        if (m != NULL) {
            if (*(u8**)(ctx + 0x3C) != m) {
                *(u8**)(ctx + 0x3C) = m;
                fn_80155724(m);
            }
        } else {
            *(u8**)(ctx + 0x3C) = NULL;
            fn_801561C0(*(f32*)(e + 0x18));
        }
        rec = *(u8**)(e + 0x1C);
        if (*(u8**)(ctx + 0x40) != rec) {
            void* h;
            *(u8**)(ctx + 0x40) = rec;
            h = *(void**)(rec + 0);
            if (h != *(void**)(ctx + 0x48)) {
                if (h != NULL) {
                    fn_801727EC(h, 0);
                    if (*(void**)(ctx + 0x48) == NULL) MaskSet(1);
                } else if (*(void**)(ctx + 0x48) != NULL) {
                    MaskClear(1);
                }
                *(void**)(ctx + 0x48) = h;
            }
            h = *(void**)(rec + 4);
            if (h != *(void**)(ctx + 0x4C)) {
                if (h != NULL) {
                    fn_801727EC(h, 1);
                    if (*(void**)(ctx + 0x4C) == NULL) MaskSet(2);
                } else if (*(void**)(ctx + 0x4C) != NULL) {
                    MaskClear(2);
                }
                *(void**)(ctx + 0x4C) = h;
            }
            h = *(void**)(rec + 8);
            if (h != *(void**)(ctx + 0x50)) {
                if (h != NULL) {
                    fn_801727EC(h, 2);
                    if (*(void**)(ctx + 0x50) == NULL) MaskSet(4);
                } else if (*(void**)(ctx + 0x50) != NULL) {
                    MaskClear(4);
                }
                *(void**)(ctx + 0x50) = h;
            }
        }
        vec = *(u8**)(e + 0x20);
        if (*(u8**)(ctx + 0x44) != vec) {
            *(u8**)(ctx + 0x44) = vec;
            if (vec != NULL) {
                s32 c0;
                s32 c1;
                s32 c2;
                s32 c3;
                fn_80175A0C(3, (const f32*)vec);
                c0 = (s32)(lbl_805FF77C * *(f32*)(vec + 0xC));
                c1 = (s32)(lbl_805FF77C * *(f32*)(vec + 0x10));
                c2 = (s32)(lbl_805FF77C * *(f32*)(vec + 0x14));
                c3 = (s32)(lbl_805FF77C * *(f32*)(vec + 0x18));
                if (c0 < 0) c0 = 0; else if (c0 > 0xFF) c0 = 0xFF;
                if (c1 < 0) c1 = 0; else if (c1 > 0xFF) c1 = 0xFF;
                if (c2 < 0) c2 = 0; else if (c2 > 0xFF) c2 = 0xFF;
                if (c3 < 0) c3 = 0; else if (c3 > 0xFF) c3 = 0xFF;
                GXWGFifo._u8 = 0x10;
                GXWGFifo._u16 = 0;
                GXWGFifo._u16 = 0x633;
                GXWGFifo._u32 = (u32)(c3 | (c2 << 8) | (c0 << 24) | (c1 << 16));
                MaskSet(8);
                *(s32*)(lbl_804129E8 + 0x3C) += 1;
            } else {
                MaskClear(8);
            }
        }
        mdl = *(u8**)(e + 0xC);
        *(f32*)(lbl_80400FF0 + 0) = *(f32*)(mdl + 0x24);
        *(f32*)(lbl_80400FF0 + 4) = *(f32*)(mdl + 0x28);
        *(f32*)(lbl_80400FF0 + 8) = *(f32*)(mdl + 0x2C);
        mdl = *(u8**)(e + 0xC);
        inst = *(u8**)(e + 8);
        savedByte = *(u8*)(inst + 0xD);
        if (*(f32*)(mdl + 0x58) != lbl_805FF778 || (*(s32*)(*(u8**)(e + 4) + 0xC) & 1)) {
            *(u8*)(inst + 0xD) = 3;
        }
        inst = *(u8**)(e + 8);
        lbl_805FB610 = inst;
        *(s32*)(lbl_804129E8 + 0x34) += 1;
        tbl = lbl_80348F38 + *(u8*)(inst + 0xC) * 4;
        (*(DrawFn*)(tbl + 0x48))(*(void**)(inst + 8), *(u8**)(e + 0xC) + 0x3C);
        lbl_805FB610 = inst;
        fn_801873C0(*(void**)(e + 4), *(s32*)(e + 0x14), *(s32*)(*(u8**)(e + 0xC) + 0xC) & 0x80);
        inst = *(u8**)(e + 8);
        lbl_805FB610 = NULL;
        tbl = lbl_80348F38 + *(u8*)(inst + 0xC) * 4;
        (*(DrawFn*)(tbl + 0x54))(*(void**)(inst + 8), *(u8**)(e + 0xC) + 0x3C);
        *(u8*)(*(u8**)(e + 8) + 0xD) = savedByte;
    }
    if (lbl_805FB4C0 != 0 && fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x10C0 > lbl_805FB4C8) {
        fn_8014DE68();
    }
}

}  // extern "C"
