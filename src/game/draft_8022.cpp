// Menu slot binding, CRC32, MT seeding and notice queue helpers in 0x8022xxxx.
#include "types.h"
#include "game/part.h"

extern "C" {
s32   RandomUpTo(s32 max);
u32   StrLength(const char* s);
u16   fn_8021F448(void* p);                                     // 0x8021F448: u16 count at (*(p+4))+0xA
void  fn_8021F8D8(u8* p, u32 arg);                              // 0x8021F8D8
void  fn_8021FB28(void);                                        // 0x8021FB28: returns 0
void  fn_8021FF94(void* p, void* v);                            // 0x8021FF94: stores v at +0x14
void  fn_8021FF9C(void* p, void* v);                            // 0x8021FF9C: stores v at +0x18
void* MemCopy(void* dst, const void* src, u32 n);               // 0x802209C8 (tail call memcpy)
void* fn_802209CC(void* dst, s32 val, u32 n);                   // 0x802209CC (tail call memset)
s32   fn_80221288(const char* a, const char* b, u32 n);         // 0x80221288 (tail call string compare)
extern const char* lbl_80389188[4];                             // 0x80389188
extern const char* lbl_805FA0E4;                                // 0x805FA0E4
extern u32 lbl_803497F0[];                                      // 0x803497F0 (CRC32 table)
u64   __shl2i(s32, s32, s32);                                   // 0x8032F9A0
u64   __shr2u(s32, s32, s32);                                   // 0x8032F9C4
void  NoticePosted(u8* p, s32 idx);                             // 0x8022DF3C

// 0x80220188
// Binds the menu entries named by the strings at +0x8 (char 2 = row, char 3 = 'a'+column) to the
// 3x4 slot table at +0x20; marks handled entries in `handled`.
void fn_80220188(u8* arg0, u8* handled, u32 arg2) {
    u8 flags[3];
    s32 i;
    s32 off = 0;
    fn_802209CC(arg0 + 0x20, 0, 0x30);
    *(u8**)(arg0 + 0x50) = 0;
    fn_802209CC(flags, 0, 3);
    for (i = 0; i < (s32)fn_8021F448(*(void**)arg0); i++, off += 0x20) {
        if (*(s32*)(*(u8**)(arg0 + 0x8) + i * 4) != 0) {
            u32 k = 4;
            u32 j;
            for (j = 0; j < 4; j++) {
                if (fn_80221288(*(const char**)(*(u8**)(arg0 + 0x8) + i * 4), lbl_80389188[j], StrLength(lbl_80389188[j])) == 0) {
                    k = j;
                    break;
                }
            }
            if ((s32)k < 4) {
                const u8* s = *(const u8**)(*(u8**)(arg0 + 0x8) + i * 4);
                s32 col = (s8)s[3] - 0x61;
                if ((u32)((s8)s[2] - 0x30) == arg2) {
                    flags[col] = 1;
                    *(u8**)(arg0 + k * 0xC + col * 4 + 0x20) = *(u8**)(arg0 + 0x4) + off;
                } else {
                    fn_8021FF94(*(u8**)(arg0 + 0x4) + off, (void*)fn_8021FB28);
                }
                handled[i] = 1;
            } else {
                if (fn_80221288(*(const char**)(*(u8**)(arg0 + 0x8) + i * 4), lbl_805FA0E4, StrLength(lbl_805FA0E4)) == 0) {
                    const u8* s = *(const u8**)(*(u8**)(arg0 + 0x8) + i * 4);
                    if ((s8)s[2] - 0x30 == 0) {
                        *(u8**)(arg0 + 0x50) = *(u8**)(arg0 + 0x4) + off;
                    } else {
                        fn_8021FF94(*(u8**)(arg0 + 0x4) + off, (void*)fn_8021FB28);
                    }
                    handled[i] = 1;
                }
            }
        }
    }
    fn_8021FF9C(*(void**)(arg0 + 0x50), arg0 + 0x406C);
    for (i = 0; (u32)i < 3; i++) {
        u8* slot = arg0 + i * 4;
        u8* v = arg0 + 0x5C;
        u32 j;
        for (j = 0; j < 4; j++, slot += 0xC, v += 0x1004) {
            fn_8021FF9C(*(void**)(slot + 0x20), v);
        }
    }
    fn_8021FF94(*(void**)(arg0 + 0x50), *(void**)(arg0 + 0x58));
    for (i = 0; (u32)i < 3; i++) {
        u8* slot = arg0 + i * 4;
        u32 j;
        for (j = 0; j < 4; j++, slot += 0xC) {
            if (i == 0) fn_8021FF94(*(void**)(slot + 0x20), *(void**)(arg0 + 0x54));
            else        fn_8021FF94(*(void**)(slot + 0x20), (void*)fn_8021FB28);
        }
    }
}

// 0x80220404
// Same slot binding as fn_80220188 without the handled flags, then reset the panel via fn_8021F8D8.
void fn_80220404(u8* arg0, u32 arg1) {
    s32 i;
    s32 off = 0;
    for (i = 0; i < (s32)fn_8021F448(*(void**)arg0); i++, off += 0x20) {
        if (*(s32*)(*(u8**)(arg0 + 0x8) + i * 4) != 0) {
            u32 k = 4;
            u32 j;
            for (j = 0; j < 4; j++) {
                if (fn_80221288(*(const char**)(*(u8**)(arg0 + 0x8) + i * 4), lbl_80389188[j], StrLength(lbl_80389188[j])) == 0) {
                    k = j;
                    break;
                }
            }
            if ((s32)k < 4) {
                const u8* s = *(const u8**)(*(u8**)(arg0 + 0x8) + i * 4);
                s32 col = (s8)s[3] - 0x61;
                if ((u32)((s8)s[2] - 0x30) == arg1) {
                    *(u8**)(arg0 + k * 0xC + col * 4 + 0x20) = *(u8**)(arg0 + 0x4) + off;
                } else {
                    fn_8021FF94(*(u8**)(arg0 + 0x4) + off, (void*)fn_8021FB28);
                }
            }
        }
    }
    fn_8021FF9C(*(void**)(arg0 + 0x50), arg0 + 0x406C);
    for (i = 0; (u32)i < 3; i++) {
        u8* slot = arg0 + i * 4;
        u8* v = arg0 + 0x5C;
        u32 j;
        for (j = 0; j < 4; j++, slot += 0xC, v += 0x1004) {
            fn_8021FF9C(*(void**)(slot + 0x20), v);
        }
    }
    fn_8021FF94(*(void**)(arg0 + 0x50), *(void**)(arg0 + 0x58));
    for (i = 0; i < 3; i++) {
        u8* slot = arg0 + i * 4;
        u32 j;
        for (j = 0; j < 4; j++, slot += 0xC) {
            if (i == 0) fn_8021FF94(*(void**)(slot + 0x20), *(void**)(arg0 + 0x54));
            else        fn_8021FF94(*(void**)(slot + 0x20), (void*)fn_8021FB28);
        }
    }
    fn_8021F8D8(arg0, arg1);
    *(s32*)(arg0 + 0x5070) = 0;
}

// 0x802205F0
// Highlights row `arg1` (selected callback) and gives the other rows the empty callback.
void fn_802205F0(u8* arg0, u32 arg1) {
    u32 i;
    fn_8021FF94(*(void**)(arg0 + 0x50), *(void**)(arg0 + 0x58));
    for (i = 0; i < 3; i++) {
        u8* slot = arg0 + i * 4;
        u32 j;
        for (j = 0; j < 4; j++, slot += 0xC) {
            if (i == arg1) fn_8021FF94(*(void**)(slot + 0x20), *(void**)(arg0 + 0x54));
            else           fn_8021FF94(*(void**)(slot + 0x20), (void*)fn_8021FB28);
        }
    }
}

// 0x80220890
// CRC32 (reflected, table at lbl_803497F0) over `len` bytes; 0 if buf is null.
u32 fn_80220890(const u8* buf, s32 len) {
    u32 crc = 0xFFFFFFFFu;
    if (buf == 0) return 0;
    if (len > 0) {
        s32 i;
        for (i = 0; i < len; i++) {
            crc = (crc >> 8) ^ lbl_803497F0[(crc ^ buf[i]) & 0xFF];
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

// 0x80220A8C
// Seeds a 624-word Mersenne-twister-style state at p (seed stored at +0x9C4 if nonzero, index 0x270 at +0x9C0).
void fn_80220A8C(u8* p, s32 seed) {
    u32 s;
    s32 i;
    if (seed != 0) *(s32*)(p + 0x9C4) = seed;
    s = *(u32*)(p + 0x9C4);
    for (i = 0; i < 0x270; i++) {
        u32 w = s & 0xFFFF0000u;
        s = s * 0x10DCDu + 1;
        w |= s >> 16;
        *(u32*)(p + i * 4) = w;
        s = s * 0x10DCDu + 1;
    }
    *(s32*)(p + 0x9C0) = 0x270;
    *(u32*)(p + 0x9C4) = s + 1;
}

// 0x8022DF3C
// Removes older queued notices (type 2, subtype 0) that match the same owner id as entry `idx`.
void NoticePosted(u8* p, s32 idx) {
    s32 count = *(s32*)(p + 0x8E8);
    if (count != 0) {
        u8* e = p + idx * 0x1C;
        if (*(s32*)(e + 0x744) == 2 && *(s32*)(e + 0x748) == 0) {
            s32 i = count - 1;
            s32 id = *(s32*)(e + 0x758);
            u8* cur = p + i * 0x1C;
            while (i >= 0) {
                if (i != idx) {
                    bool match;
                    if (id == 0) match = *(s32*)(cur + 0x744) == 2;
                    else         match = *(s32*)(cur + 0x744) == 2 && id == *(s32*)(cur + 0x758);
                    if (match) {
                        s32 n = *(s32*)(p + 0x8E8);
                        if (i == n - 1) {
                            *(s32*)(p + 0x8E8) = n - 1;
                        } else if (i < n && i >= 0) {
                            MemCopy(cur + 0x744, p + (i + 1) * 0x1C + 0x744, (n - i - 1) * 0x1C);
                            *(s32*)(p + 0x8E8) = *(s32*)(p + 0x8E8) - 1;
                        }
                    }
                }
                cur -= 0x1C;
                i--;
            }
        }
    }
}

// 0x8022E0B0
// Removes the queued notice whose id (+0x75C) equals `id`, shifting the rest down.
void fn_8022E0B0(u8* p, u32 id) {
    s32 n = *(s32*)(p + 0x8E8);
    s32 i;
    u8* cur = p;
    if (n > 0) {
        for (i = 0; i < n; i++, cur += 0x1C) {
            if (id == *(u32*)(cur + 0x75C)) {
                if (i == n - 1) {
                    *(s32*)(p + 0x8E8) = *(s32*)(p + 0x8E8) - 1;
                    return;
                }
                if (i < n && i >= 0) {
                    MemCopy(p + i * 0x1C + 0x744, p + (i + 1) * 0x1C + 0x744, (n - i - 1) * 0x1C);
                    *(s32*)(p + 0x8E8) = *(s32*)(p + 0x8E8) - 1;
                }
                return;
            }
        }
    }
}

// 0x8022E178
// Pushes a new message into slot `slot` (0..3) of the 0x3C-byte table at +0x5B8 (old one is kept
// as previous at +0x5CC) and optionally queues a type-2 notice.
void fn_8022E178(u8* p, s32 slot, s32 a, s32 b, s32 c, s32 queue) {
    if (slot >= 0) {
        u8* e;
        u8 tmp[0x28];
        if (slot >= 4) return;
        e = p + slot * 0x3C;
        if (*(s32*)(e + 0x5C0) == 1) *(s32*)(e + 0x5C0) = 2;
        MemCopy(tmp, e + 0x5B8, 0x28);
        MemCopy(e + 0x5CC, tmp, 0x28);
        *(s32*)(e + 0x5C0) = 1;
        *(s32*)(e + 0x5B8) = a;
        *(s32*)(e + 0x5BC) = b;
        *(s32*)(e + 0x5C4) = 0;
        *(s32*)(e + 0x5C8) = c;
        if (queue != 0) {
            s32 n = *(s32*)(p + 0x8E8);
            if (n < 0xF) {
                u8* q = p + n * 0x1C;
                s32 id;
                *(s32*)(q + 0x744) = 2;
                *(s32*)(q + 0x748) = c;
                *(s32*)(q + 0x74C) = 0;
                *(s32*)(q + 0x750) = a;
                *(s32*)(q + 0x754) = b;
                *(s32*)(q + 0x758) = slot;
                id = *(s32*)(p + 0x910) + 1;
                *(s32*)(p + 0x910) = id;
                *(s32*)(q + 0x75C) = id;
                *(s32*)(p + 0x8E8) = *(s32*)(p + 0x8E8) + 1;
                NoticePosted(p, n);
            }
        }
        if (c > 0) *(s32*)(p + 0xA20) = 1;
    }
}

// 0x8022E354
// Re-queues the current message of slot `slot` as a type-2 notice (owner -1); tail-calls NoticePosted.
void fn_8022E354(u8* p, s32 slot) {
    u8* e;
    s32 n, a, b, c, id;
    u8* q;
    if (slot < 0) return;
    if (slot >= 4) return;
    e = p + slot * 0x3C;
    *(s32*)(e + 0x5C0) = 2;
    n = *(s32*)(p + 0x8E8);
    a = *(s32*)(e + 0x5B8);
    b = *(s32*)(e + 0x5BC);
    c = *(s32*)(e + 0x5C8);
    if (n >= 0xF) return;
    q = p + n * 0x1C;
    *(s32*)(q + 0x744) = 2;
    *(s32*)(q + 0x748) = c;
    *(s32*)(q + 0x74C) = c;
    *(s32*)(q + 0x750) = a;
    *(s32*)(q + 0x754) = b;
    *(s32*)(q + 0x758) = -1;
    id = *(s32*)(p + 0x910) + 1;
    *(s32*)(p + 0x910) = id;
    *(s32*)(q + 0x75C) = id;
    *(s32*)(p + 0x8E8) = *(s32*)(p + 0x8E8) + 1;
    NoticePosted(p, n);
}

// 0x8022E440
// Picks one random entry from each of two filtered subsets of the 0x2D 16-byte records at
// db+0x4F00 (matching arg1/-1 and level <= +0x118, or matching the per-entry counter at +0xBF0);
// stores the picks at +0x73C / +0x740. Returns 1 on success, 0 (picks -1) if either set is empty.
s32 fn_8022E440(u8* p, s32 arg1) {
    s32 cntA = 0, cntB = 0;
    u64 maskA = 0, maskB = 0;
    u8 i;
    u8* ent;
    u8* cnt = p;
    *(s32*)(p + 0x73C) = -1;
    *(s32*)(p + 0x740) = -1;
    ent = *(u8**)(gGameDb->data + 0x4F00);
    i = 0;
    do {
        s16 lvl = *(s16*)(ent + 0x8);
        s8 key = *(s8*)(ent + 0xB);
        if (key == -1 || key == arg1) {
            if ((u32)(s32)lvl <= *(u32*)(p + 0x118)) {
                maskA |= (u64)1 << i;
                cntA++;
            }
        } else if (key > 0 && (s32)*(u8*)(cnt + 0xBF0) >= key) {
            maskB |= (u64)1 << i;
            cntB++;
        }
        i++;
        ent += 0x10;
        cnt += 0xC;
    } while ((s32)i < 0x2D);
    if (cntA > 0 && cntB > 0) {
        s32 ia = 0, ib = 0;
        s32 pickA = RandomUpTo(cntA - 1);
        s32 pickB = RandomUpTo(cntB - 1);
        for (i = 0; (s32)i < 0x2D; i++) {
            if ((maskA >> i) & 1) {
                if (ia == pickA) {
                    *(s32*)(p + 0x740) = (s8)i;
                    if (*(s32*)(p + 0x73C) != -1) break;
                }
                ia++;
            } else if ((maskB >> i) & 1) {
                if (ib == pickB) {
                    *(s32*)(p + 0x73C) = (s8)i;
                    if (*(s32*)(p + 0x740) != -1) break;
                }
                ib++;
            }
        }
        return 1;
    }
    *(s32*)(p + 0x73C) = -1;
    *(s32*)(p + 0x740) = -1;
    return 0;
}

}  // extern "C"
