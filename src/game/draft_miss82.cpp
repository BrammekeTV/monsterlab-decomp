// Misc engine/audio/render helpers (batch miss82): tree free, audio shutdown, texture slot lookup,
// register-table updates and GX state/colour writers.
#include <stdint.h>

#include "types.h"
#include "revolution.h"
#include "engine/math.h"
#include "engine/rend.h"
#include "system/sys.h"

extern "C" {
void   fn_801EBA20(void* list, void* item);                           // 0x801EBA20
void   fn_80203DE8(u8* audio, u8* state);                             // 0x80203DE8
void   AXFreeVoice(u8* voice);                                   // 0x80257FA8
void   AXARTRemoveSound(u8* p);                                            // 0x80264420
u32    StringHash(const char* s);                                     // 0x801EB704
int    SysSprintf(char* buf, const char* fmt, ...);
char*  fn_801D8ADC(char* s);                                          // 0x801D8ADC
u8*    fn_8016A08C(const char* name);                                 // 0x8016A08C
s32    fn_801E5F48(u8* pool, const char* name, u32 hash);             // 0x801E5F48
void*  IdPoolNext(void* pool);                                        // 0x801E6EB0
u32    fn_80146B28(u8* p, u8* buf, u32 size, u32 offset);             // 0x80146B28
void   operator_delete(void* p);                                      // 0x801D426C
s32    fn_801478C8(u8* table, s32 idx);                               // 0x801478C8
void   fn_8014792C(u8* table, s32 idx, s32 val);                      // 0x8014792C
s32    fn_8015BA30(void* p);                                          // 0x8015BA30
void   fn_8014DE68(void);                                             // 0x8014DE68
void   fn_80142ED0(u8* base, s32 size);                               // 0x80142ED0
f64    fn_8033F20C(f64 x, f64 y);                                     // 0x8033F20C

extern u8      gAudio[];
extern u8      gIdPool[];                  // 0x8048A8F0
extern u8      lbl_803C75C8[];             // 0x803C75C8 (texture slot table, 0x40 bytes per slot)
extern u8      lbl_803F8240[];             // 0x803F8240 (read buffer)
extern f32     lbl_804010D8[12];           // 0x804010D8 (3x4 matrix)
extern u8      sRendDLs[];                 // 0x80401108 (display-list buffers, 0x94 bytes each)
extern u8      sRendDLRecords[];           // 0x80401ADC (display-list records, 0x14 bytes each)
extern u8      lbl_8051DAD0[];             // 0x8051DAD0 (audio state block)
extern u8      lbl_8051E6A8[];             // 0x8051E6A8 (audio stream block)
extern const char lbl_805F8EB8[];          // 0x805F8EB8 ("%s.sir")
extern const char lbl_8036C8DC[];          // 0x8036C8DC ("shared_texture")
extern u8*     lbl_805F91F0;               // 0x805F91F0 (register update list)
extern s32     lbl_805F91F4;               // 0x805F91F4 (register update count)
extern u8      lbl_805F98C0;               // 0x805F98C0
extern u32     lbl_805FB230;               // 0x805FB230 (next texture slot)
extern u32     lbl_805FB234;               // 0x805FB234 (texture slots in use)
extern void*   lbl_805FB478;               // 0x805FB478
extern void*   lbl_805FB488;               // 0x805FB488
extern u8*     lbl_805FB490;               // 0x805FB490 (current register table)
extern s32     lbl_805FB4BC;               // 0x805FB4BC (current display list)
extern s32     lbl_805FB4C0;               // 0x805FB4C0
extern s32     lbl_805FB4C4;               // 0x805FB4C4
extern s32     lbl_805FB4C8;               // 0x805FB4C8
extern s32     lbl_805FB4CC;               // 0x805FB4CC
extern u8*     lbl_805FB4D0;               // 0x805FB4D0
extern s32     lbl_805FB4D4;               // 0x805FB4D4
extern s32     lbl_805FB4D8;               // 0x805FB4D8
extern s32     lbl_805FB4DC;               // 0x805FB4DC
extern s32     lbl_805FB4E0;               // 0x805FB4E0
extern s32     lbl_805FB4E8;               // 0x805FB4E8
extern s32     lbl_805FB4EC;               // 0x805FB4EC
extern u8      lbl_805FB50C;               // 0x805FB50C (GX shadow flags byte)
extern s32     lbl_805FBAE0;               // 0x805FBAE0
extern s32     lbl_805FBB80[2];            // 0x805FBB80
extern u8*     lbl_805FBB88[2];            // 0x805FBB88
extern s32     lbl_805FBB90;               // 0x805FBB90
extern f32     lbl_805FF080;               // 0x805FF080 (1.0f)
extern f32     lbl_805FF084;               // 0x805FF084 (0.0f)
extern f32     lbl_805FF0E4;               // 0x805FF0E4 (1.0f)
extern f32     lbl_805FF0EC;               // 0x805FF0EC (0.0f)
extern f32     lbl_805FF0F0;               // 0x805FF0F0 (255.0f)
extern f32     lbl_805FF0F4;               // 0x805FF0F4 (2.0f)
extern f32     lbl_805FF0F8;               // 0x805FF0F8 (5.0f)
extern f32     lbl_805FF0FC;               // 0x805FF0FC (6.0f)
extern f32     lbl_805FF100;               // 0x805FF100 (-4.0f)
extern f32     lbl_805FF104;               // 0x805FF104 (4.0f)
extern f32     lbl_805FF108;               // 0x805FF108 (10.0f)
extern f64     lbl_805FF110;               // 0x805FF110 (0.0625)
}

// Clamps a converted colour channel to 0..255.
static inline s32 ClampByte(s32 v) {
    if (v < 0) return 0;
    if (v > 0xFF) return 0xFF;
    return v;
}

// Reinterprets (v << 3) as a float.
static inline f32 BitsToF32(u32 v) {
    return *(f32*)&v;
}

// max(f1, f0), clamped to [0, 1], returned as packed register bits (float bits >> 3, kind 2 in the top bits).
static inline u32 PackClamped(f32 f1, f32 f0) {
    f32 lo = lbl_805FF084;
    f32 hi = lbl_805FF080;
    if (!(f1 > f0)) f1 = f0;
    if (f1 < lo) {
        f1 = lo;
    } else if (f1 > hi) {
        f1 = hi;
    }
    return (*(u32*)&f1 >> 3) | 0x40000000;
}

extern "C" {

// 0x801BF398
// Frees a 6-ary tree four levels deep: each node's slots (+4, 6 pointers) are walked recursively,
// and every node is then released through fn_801EBA20 on the list at (*node + 0x68).
void fn_801BF398(u8* arg0, u8* arg1) {
    if (arg1 != NULL) {
        u8* p1 = arg1;
        for (s32 i1 = 0; i1 < 6; i1++, p1 += 4) {
            u8* n1 = *(u8**)(p1 + 4);
            if (n1 != NULL) {
                u8* p2 = n1;
                for (s32 i2 = 0; i2 < 6; i2++, p2 += 4) {
                    u8* n2 = *(u8**)(p2 + 4);
                    if (n2 != NULL) {
                        u8* p3 = n2;
                        for (s32 i3 = 0; i3 < 6; i3++, p3 += 4) {
                            u8* n3 = *(u8**)(p3 + 4);
                            if (n3 != NULL) {
                                u8* p4 = n3;
                                for (s32 i4 = 0; i4 < 6; i4++, p4 += 4) {
                                    u8* leaf = *(u8**)(p4 + 4);
                                    if (leaf != NULL) {
                                        fn_801BF398(arg0, leaf);
                                    }
                                }
                                fn_801EBA20(*(void**)(*(u8**)n3 + 0x68), n3);
                            }
                        }
                        fn_801EBA20(*(void**)(*(u8**)n2 + 0x68), n2);
                    }
                }
                fn_801EBA20(*(void**)(*(u8**)n1 + 0x68), n1);
            }
        }
        fn_801EBA20(*(void**)(*(u8**)arg1 + 0x68), arg1);
    }
}

// 0x801FD730
// Audio shutdown: clears two flag bytes on arg0, waits for pending audio work, then stops and frees
// both streams (channels 0 and 1) with the busy flag lbl_805FBAE0 raised.
void fn_801FD730(u8* arg0) {
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    *(u8*)(arg0 + 0x29) = 0;
    *(u8*)(arg0 + 0x28) = 0;
    lbl_805FBAE0 = saved;
    if (lbl_805F98C0 != 0 && *(s32*)(lbl_8051DAD0 + 0xB4) != 0) {
        fn_80203DE8(gAudio, lbl_8051DAD0);
        while (*(volatile s32*)(lbl_8051DAD0 + 0xB4) != 0) {
            WAIT_FOR_INTERRUPT();
        }
    }
    s32 saved2 = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    if (lbl_805FBB80[0] != 0) {
        if (*(s32*)(lbl_8051E6A8 + 0x0) != 0) {
            if (*(u8*)(lbl_8051E6A8 + 0xFE) != 0) {
                AXARTRemoveSound(lbl_8051E6A8 + 0x10);
            }
            *(s32*)(lbl_8051DAD0 + 0x8C) = 0;
            AXFreeVoice(*(u8**)(lbl_8051E6A8 + 0x0));
            *(s32*)(lbl_8051E6A8 + 0x0) = 0;
            *(s32*)(lbl_8051E6A8 + 0x4) = 0;
        }
        if (lbl_805FBB88[0] != NULL) {
            MEMFreeToAllocator(&gSysAllocator, lbl_805FBB88[0]);
        }
        lbl_805FBB80[0] = 0;
        lbl_805FBB88[0] = NULL;
        *(s32*)(lbl_8051DAD0 + 0x94) = 0;
    }
    if (lbl_805FBB80[1] != 0) {
        if (*(s32*)(lbl_8051E6A8 + 0x114) != 0) {
            if (*(u8*)(lbl_8051E6A8 + 0x212) != 0) {
                AXARTRemoveSound(lbl_8051E6A8 + 0x124);
            }
            *(s32*)(lbl_8051DAD0 + 0x90) = 0;
            AXFreeVoice(*(u8**)(lbl_8051E6A8 + 0x114));
            *(s32*)(lbl_8051E6A8 + 0x114) = 0;
            *(s32*)(lbl_8051E6A8 + 0x118) = 0;
        }
        if (lbl_805FBB88[1] != NULL) {
            MEMFreeToAllocator(&gSysAllocator, lbl_805FBB88[1]);
        }
        lbl_805FBB80[1] = 0;
        lbl_805FBB88[1] = NULL;
        *(s32*)(lbl_8051DAD0 + 0x98) = 0;
    }
    lbl_805FBAE0 = saved2;
    lbl_805FBB90 = 0;
}

// 0x801094F4
// Looks up (or registers) a shared texture by name. An existing entry gets its refcount (+0x24) bumped
// and is returned; otherwise, while fewer than 0x7D0 slots are used, a new 0x40-byte slot is filled in
// (id, formatted name, hash, arg1, hash lookup result) and the old slot index is returned.
u8* fn_801094F4(char* arg0, s32 arg1) {
    char buf[0x80];
    u32 hash = StringHash(arg0);
    SysSprintf(buf, lbl_805F8EB8, lbl_8036C8DC);
    s32 found = fn_801E5F48(gIdPool, buf, hash);
    char* name = fn_801D8ADC(arg0);
    u8* tex = fn_8016A08C(name);
    if (tex != NULL) {
        *(s32*)(tex + 0x24) = *(s32*)(tex + 0x24) + 1;
        return tex;
    }
    if (lbl_805FB234 < 0x7CF) {
        void* id = IdPoolNext(gIdPool);
        u8* slot = lbl_803C75C8 + (lbl_805FB230 << 6);
        *(void**)slot = id;
        SysSprintf((char*)(slot + 8), name);
        u32 idx = lbl_805FB230;
        u32 next = idx + 1;
        slot = lbl_803C75C8 + (idx << 6);
        *(u32*)(slot + 0x38) = hash;
        *(s32*)(slot + 0x3C) = arg1;
        *(s32*)(slot + 0x4) = found;
        lbl_805FB234 = lbl_805FB234 + 1;
        lbl_805FB230 = next;
        if (next >= 0x7D0) {
            lbl_805FB230 = 1;
        }
        return (u8*)(uintptr_t)idx;
    }
    return tex;
}

// 0x80146C44
// Cached file read window: returns a pointer into the buffer lbl_803F8240 for file offset arg1 (clamping
// *arg2 to what is cached), refilling a window of up to 0x8000 bytes via fn_80146B28 when the offset is
// outside the cached range. Returns NULL (and sets *arg2 = 0) at/after the end or on a short read.
u8* fn_80146C44(u8* arg0, u32 arg1, s32* arg2) {
    if (*arg2 == 0) {
        return NULL;
    }
    if (lbl_805FB478 != (void*)arg0) {
        lbl_805FB478 = arg0;
        *(u32*)(arg0 + 0x48) = 0;
        *(u32*)(arg0 + 0x44) = 0;
    }
    u32 start = *(u32*)(arg0 + 0x48);
    u32 end = start + *(u32*)(arg0 + 0x44);
    if (arg1 >= start && arg1 < end) {
        s32 n = *arg2;
        u32 e = arg1 + n;
        if (e > end) {
            *arg2 = n - (e - end);
        }
        return lbl_803F8240 + (arg1 - start);
    }
    u32 limit = *(u32*)(arg0 + 0x3C);
    if (arg1 >= limit) {
        *(s32*)(arg0 + 0x50) = -1;
        *arg2 = 0;
        return NULL;
    }
    u32 len = ((limit - arg1) + 0x1F) & 0xFFFFFFE0u;
    *(u32*)(arg0 + 0x44) = len;
    if (len > 0x8000) {
        *(u32*)(arg0 + 0x44) = 0x8000;
    }
    u32 aligned = arg1 & 0xFFFFFFFCu;
    *(u32*)(arg0 + 0x48) = aligned;
    u32 got = fn_80146B28(arg0, lbl_803F8240, *(u32*)(arg0 + 0x44), aligned);
    u32 want = *(u32*)(arg0 + 0x44);
    if (got != want) {
        *arg2 = 0;
        *(u32*)(arg0 + 0x44) = 0;
        *(s32*)(arg0 + 0x4C) = -1;
        return NULL;
    }
    if ((u32)*arg2 > want) {
        *arg2 = (s32)want;
    }
    return lbl_803F8240;
}

// 0x80147150
// Base destructor: when the object (arg0) is live (+4 set), clears +4, sets +8, and if its u16 at +0
// is nonzero removes it from the pointer list of lbl_805FB488 (entries at +0x20, count at +0xA0).
// Frees arg0 when arg1 > 0. Returns arg0.
u8* fn_80147150(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        if (*(s32*)(arg0 + 0x4) != 0) {
            u16 flag = *(u16*)(arg0 + 0x0);
            *(s32*)(arg0 + 0x4) = 0;
            *(s32*)(arg0 + 0x8) = 1;
            u32 present = (flag != 0);
            if (present) {
                u8* list = (u8*)lbl_805FB488;
                s32 found = 0;
                if (list != NULL) {
                    u32 n = *(u32*)(list + 0xA0);
                    u8* e = list;
                    for (u32 k = 0; k < n; k++, e += 4) {
                        if (*(u8**)(e + 0x20) == arg0) {
                            found = 1;
                            break;
                        }
                    }
                }
                present = (found != 0);
            }
            if (present) {
                u8* list = (u8*)lbl_805FB488;
                if (list != NULL) {
                    u32 n = *(u32*)(list + 0xA0);
                    u8* e = list;
                    u32 i = 0;
                    for (u32 k = 0; k < n; k++, e += 4, i++) {
                        if (*(u8**)(e + 0x20) == arg0) {
                            u8* slot = list + i * 4;
                            *(u8**)(slot + 0x20) = NULL;
                            *(u32*)(list + 0xA0) = *(u32*)(list + 0xA0) - 1;
                            u32 cnt;
                            while (cnt = *(u32*)(list + 0xA0), i < cnt) {
                                i++;
                                *(u8**)(slot + 0x20) = *(u8**)(slot + 0x24);
                                slot += 4;
                            }
                            *(u8**)(list + cnt * 4 + 0x20) = NULL;
                            break;
                        }
                    }
                }
            }
        }
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x801472A8
// Applies the update list (lbl_805F91F0, lbl_805F91F4 entries of two u16 register ids): combines the
// current values of the two registers (kind in the top 3 bits: 0 plain, 1 bit, 2 float) and writes the
// result to the second register.
void fn_801472A8(void) {
    s32 count = lbl_805F91F4;
    u8* e = lbl_805F91F0;
    for (s32 i = 0; i < count; i++, e += 4) {
        u32 a = (u32)fn_801478C8(lbl_805FB490, *(u16*)(e + 0));
        u32 b = (u32)fn_801478C8(lbl_805FB490, *(u16*)(e + 2));
        u32 kb = b >> 29;
        u32 ka = a >> 29;
        u32 out = b;
        if (kb == 0) {
            out = a;
        } else if (kb == 1) {
            if (ka == 1) {
                out = (((a & 1) | (b & 1)) != 0) | 0x20000000;
            } else if (ka == 2) {
                f32 f0 = (b & 1) ? lbl_805FF080 : lbl_805FF084;
                out = PackClamped(BitsToF32(a << 3), f0);
            }
        } else if (kb == 2) {
            if (ka == 1) {
                f32 f0 = (a & 1) ? lbl_805FF080 : lbl_805FF084;
                out = PackClamped(BitsToF32(b << 3), f0);
            } else if (ka == 2) {
                out = PackClamped(BitsToF32(a << 3), BitsToF32(b << 3));
            }
        }
        fn_8014792C(lbl_805FB490, *(u16*)(e + 2), (s32)out);
    }
}

// 0x80148D94
// Sets the GX clear colour: makes sure the current display list has room (0x1107 bytes), resets the
// shadowed Z/colour-update state if bit 0 of lbl_805FB50C is set, then writes arg0 (4 floats 0..1 as RGBA)
// to the BP register 0x100C.
void fn_80148D94(f32* arg0) {
    if (fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x1107 > lbl_805FB4C8) {
        fn_8014DE68();
        fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94);
    }
    u8 flags = lbl_805FB50C;
    if (flags & 1) {
        u32 bit1 = flags & 2;
        lbl_805FB50C = (u8)(flags & 0xFE);
        if (bit1) {
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xFE00000Fu;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 1;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x103F;
            GXWGFifo._u32 = 1;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xFE003C00u;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x100E;
            GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x1010;
            GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0x28380040u;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xC008F8AFu;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xC108F2F0u;
        } else {
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xFE00000Fu;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x103F;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xFE003C00u;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x100E;
            GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x1010;
            GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0x28380000u;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xC008FFFAu;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xC108FFD0u;
        }
    }
    s32 c0 = ClampByte((s32)(lbl_805FF0F0 * arg0[0]));
    s32 c1 = ClampByte((s32)(lbl_805FF0F0 * arg0[1]));
    s32 c2 = ClampByte((s32)(lbl_805FF0F0 * arg0[2]));
    s32 c3 = ClampByte((s32)(lbl_805FF0F0 * arg0[3]));
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 0;
    GXWGFifo._u16 = 0x100C;
    GXWGFifo._u32 = (u32)(c3 | (c2 << 8) | ((c0 << 24) | (c1 << 16)));
}

// 0x801490AC
// Sets up GX light arg0: position (arg1, transformed by the 3x4 matrix lbl_804010D8) and negated
// direction (arg2, rotated by its 3x3 part) are written to light registers 0x60A+/0x60D+, then the
// attenuation triple at 0x604+ (from distance arg3 / scale arg4: linear below 2, pow-based above).
void fn_801490AC(s32 arg0, Vec3* arg1, Vec3* arg2, f32 arg3, f32 arg4) {
    f32* m = lbl_804010D8;
    s32 base = arg0 * 0x10;
    f32 x = arg1->x;
    f32 y = arg1->y;
    f32 z = arg1->z;
    f32 px = m[9] + ((m[6] * z) + ((m[0] * x) + (m[3] * y)));
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 2;
    GXWGFifo._u16 = (u16)(base + 0x60A);
    GXWGFifo._f32 = px;
    f32 py = m[10] + ((m[7] * z) + ((m[1] * x) + (m[4] * y)));
    GXWGFifo._f32 = py;
    f32 pz = m[11] + ((m[8] * z) + ((m[2] * x) + (m[5] * y)));
    GXWGFifo._f32 = pz;
    f32 dx = -arg2->x;
    f32 dy = -arg2->y;
    f32 dz = -arg2->z;
    f32 rx = (m[6] * dz) + ((m[0] * dx) + (m[3] * dy));
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 2;
    GXWGFifo._u16 = (u16)(base + 0x60D);
    GXWGFifo._f32 = rx;
    f32 ry = (m[7] * dz) + ((m[1] * dx) + (m[4] * dy));
    f32 rz = (m[8] * dz) + ((m[2] * dx) + (m[5] * dy));
    GXWGFifo._f32 = ry;
    GXWGFifo._f32 = rz;
    f32 a0;
    f32 a1;
    f32 a2;
    if (arg3 < lbl_805FF0F4) {
        a0 = lbl_805FF100 * ((lbl_805FF0F8 * arg4) / lbl_805FF0FC);
        a1 = lbl_805FF104;
        a2 = lbl_805FF0EC;
    } else {
        f32 d = lbl_805FF0E4;
        if (!(arg3 < lbl_805FF0E4)) {
            d = lbl_805FF108;
            if (!(arg3 > lbl_805FF108)) {
                d = arg3;
            }
        }
        a0 = lbl_805FF100 * (f32)fn_8033F20C(lbl_805FF110, (f64)(lbl_805FF0E4 / d));
        a1 = lbl_805FF0EC;
        a2 = lbl_805FF104;
    }
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 2;
    GXWGFifo._u16 = (u16)(base + 0x604);
    GXWGFifo._f32 = a0;
    GXWGFifo._f32 = a1;
    GXWGFifo._f32 = a2;
}

// 0x8014930C
// Writes the RGBA colour arg1 (4 floats 0..1) for light arg0 to BP register 0x603 + arg0 * 0x10.
void fn_8014930C(s32 arg0, f32* arg1) {
    s32 base = arg0 * 0x10;
    s32 c0 = ClampByte((s32)(lbl_805FF0F0 * arg1[0]));
    s32 c1 = ClampByte((s32)(lbl_805FF0F0 * arg1[1]));
    s32 c2 = ClampByte((s32)(lbl_805FF0F0 * arg1[2]));
    s32 c3 = ClampByte((s32)(lbl_805FF0F0 * arg1[3]));
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 0;
    GXWGFifo._u16 = (u16)(base + 0x603);
    GXWGFifo._u32 = (u32)(c3 | (c2 << 8) | ((c0 << 24) | (c1 << 16)));
}

// 0x80149594
// When bit 1 of the GX shadow flags is set, clears it and re-sends the matching state block (which one
// depends on whether bit 0 was set).
void fn_80149594(void) {
    u8 flags = lbl_805FB50C;
    if (flags & 2) {
        u32 bit0 = flags & 1;
        lbl_805FB50C = (u8)(flags & 0xFD);
        if (bit0) {
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xFE00000Fu;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x103F;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xFE003C00u;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x1010;
            GXWGFifo._u32 = 0x441;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0x28380000u;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xC008F2A4u;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xC108E6F0u;
        } else {
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xFE00000Fu;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x103F;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xFE003C00u;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x100E;
            GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x1010;
            GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0x28380000u;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xC008FFFAu;
            GXWGFifo._u8 = 0x61;
            GXWGFifo._u32 = 0xC108FFD0u;
        }
    }
}

// 0x8014E3EC
// Reserves arg0 bytes (rounded up to 0x4000) in the current display list. If it does not fit, ends the
// list, flushes its unflushed tail, submits it (or queues it when deferring), advances to the next list
// slot and starts it; then records the new capacity/remaining counters.
void fn_8014E3EC(s32 arg0) {
    s32 size = (arg0 + 0x3FFF) & 0xFFFFC000;
    if (fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + size + 0x10C0 > lbl_805FB4C8) {
        RendDLEnd((RendDL*)(sRendDLs + lbl_805FB4BC * 0x94));
        s32 rem = 0x24000 - lbl_805FB4C8;
        if (rem != 0) {
            fn_80142ED0(RendDLGetBase((RendDL*)(sRendDLs + lbl_805FB4BC * 0x94)) + lbl_805FB4C8, rem);
        }
        if (lbl_805FB4D4 != 0) {
            lbl_805FB4DC = lbl_805FB4DC + 1;
        } else {
            RendThreadSubmitDL((RendDLRecord*)(sRendDLRecords + lbl_805FB4BC * 0x14), lbl_805FB4E0 == 0);
        }
        lbl_805FB4C0 = 0;
        lbl_805FB4D0 = NULL;
        lbl_805FB4C4 = 0;
        lbl_805FB4C8 = 0;
        lbl_805FB4CC = 0;
        if (lbl_805FB4E0 != 0) {
            lbl_805FB4E8 = lbl_805FB4E8 + 1;
        }
        lbl_805FB4BC = lbl_805FB4BC + 1;
        if (lbl_805FB4BC >= 0x11) {
            lbl_805FB4BC = 0;
        }
        if (lbl_805FB4D4 != 0 && !RendHasFreeDL() && lbl_805FB4D4 != 0) {
            RendThreadSync();
            s32 head = lbl_805FB4D8;
            s32 count = lbl_805FB4DC;
            while (count > 0) {
                RendThreadSubmitDL((RendDLRecord*)(sRendDLRecords + head * 0x14), lbl_805FB4E0 == 0);
                head = lbl_805FB4D8 + 1;
                lbl_805FB4D8 = head;
                if (head >= 0x11) {
                    lbl_805FB4D8 = 0;
                    head = 0;
                }
                count = lbl_805FB4DC - 1;
                lbl_805FB4DC = count;
            }
            lbl_805FB4D4 = 0;
            lbl_805FB4D8 = 0;
            lbl_805FB4DC = 0;
        }
        RendWaitFreeDL();
        RendDLBegin((RendDL*)(sRendDLs + lbl_805FB4BC * 0x94), (RendDLRecord*)(sRendDLRecords + lbl_805FB4BC * 0x14));
        lbl_805FB4C0 = 1;
        lbl_805FB4C4 = 0x24000;
        lbl_805FB4C8 = 0x24000;
        lbl_805FB4CC = 0;
        lbl_805FB4D0 = RendDLGetBase((RendDL*)(sRendDLs + lbl_805FB4BC * 0x94));
        s32 balance = lbl_805FB4EC;
        s32 left = 0x24000 - size;
        lbl_805FB4EC = balance - 1;
        lbl_805FB4C8 = left;
        lbl_805FB4C4 = left;
        lbl_805FB4CC = size;
    } else {
        s32 left = lbl_805FB4C8 - size;
        lbl_805FB4CC = size;
        lbl_805FB4C8 = left;
        lbl_805FB4C4 = left;
    }
}

}  // extern "C"
