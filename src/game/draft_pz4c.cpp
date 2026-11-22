// Mixed small functions (UI text, loader slots, quaternion records, YCbCr row, node walker).
#include <string.h>

#include "types.h"
#include "game/node_ref.h"
#include "engine/math.h"

typedef char* (*SubstFn)(void* arg);
typedef void* (*AllocFn)(u32 size, const char* tag, void* self);

extern "C" {
void   EulerToQuat(Quat* out, const Vec3* euler);               // 0x80145B00
u8*    GetGlobalSettings(void);                                 // 0x801D2BF8
void*  MemAllocSmall(u32 size);                                 // 0x801D41CC
void   MemFreeSmall(void* p);                                   // 0x801D4234
s32    SysSprintf(char* buf, const char* fmt, ...);             // 0x8033..
s32    TriggerDoAction(void* action, void* a1, void* a2);       // 0x801E0054
BOOL   ZoneIsLoaded(void* world, s32 zone);                     // 0x8012F408
void   fn_8018D2B8(const char* fmt, ...);                       // 0x8018D2B8 (debug log)
void   fn_8018D370(const char* fmt, ...);                       // 0x8018D370 (debug log)
u8*     fn_8019552C(u8* list, s32 index);                        // 0x8019552C (returns slot address)
s32    fn_8019555C(u8* list);                                   // 0x8019555C (element count)
void   fn_8019E27C(void* el);                                   // 0x8019E27C
void   fn_801A1B2C(u8* node);                                   // 0x801A1B2C
void*  fn_801B7198(void* anim, s32 a);                          // 0x801B7198
u8*    fn_801C6BCC(u8* key);                                    // 0x801C6BCC
s32    fn_801C71EC(u8* key);                                    // 0x801C71EC
void   fn_801D6780(void);                                       // 0x801D6780
void   fn_801DA91C(u8* slot, u8* obj);                          // 0x801DA91C
s32    fn_801E4FE0(u8* obj, void* a, void* buf, u32 size, s32 b);  // 0x801E4FE0
u8*    fn_801E6570(s32 a, s32 b, s32 c);                        // 0x801E6570
s32    fn_801E65A0(void);                                       // 0x801E65A0
u8*    fn_801EBEFC(const char* path, s32* outSize, void* alloc);  // 0x801EBEFC (load file)
void   fn_801EC080(u8* a, u8* b);                               // 0x801EC080
void   fn_801F00DC(u8* self, u8* line, u8* table);              // 0x801F00DC
void   fn_801F020C(u8* self, u8* line);                         // 0x801F020C
s32    fn_8033861C(const char* s, const char* fmt, ...);        // 0x8033861C (sscanf)
s32    fn_8033B1B8(const char* a, const char* b, s32 n);        // 0x8033B1B8 (strncmp thunk)

extern u8   gIdPool[];             // 0x8048A8F0
extern u8   gWorldState[];         // 0x803E6D00
extern u8   lbl_8047C640[];        // 0x8047C640 (counters)
extern u8   lbl_8048A8B4[];        // 0x8048A8B4
extern u32  lbl_805F969C;          // 0x805F969C
extern u8*  lbl_805F96D8;          // 0x805F96D8 (default string slot)
extern u8*  lbl_805FB678;          // 0x805FB678 (substitution table)
extern s32  lbl_805FB6B8;          // 0x805FB6B8
extern u8   lbl_805FB710[];        // 0x805FB710 (3-byte default key)
extern u8*  lbl_805FB914;          // 0x805FB914 (allocator object, first word = alloc fn)
extern f32  lbl_805FFF50;          // 0x805FFF50
extern f32  lbl_805FFF54;          // 0x805FFF54
extern f32  lbl_805FFF58;          // 0x805FFF58
extern u8   lbl_80382C78[];        // 0x80382C78
extern u8   lbl_80382DA4[];        // 0x80382DA4
extern u8   lbl_80382E2C[];        // 0x80382E2C
extern u8   lbl_80382F1C[];        // 0x80382F1C
extern u8   lbl_803830B4[];        // 0x803830B4
extern u8   lbl_80383278[];        // 0x80383278
extern u8   lbl_803832F4[];        // 0x803832F4
extern u8   lbl_8038336C[];        // 0x8038336C
extern u8   lbl_803833E0[];        // 0x803833E0
extern u8   lbl_803835E8[];        // 0x803835E8
extern u8   lbl_80383638[];        // 0x80383638
extern u8   lbl_803836EC[];        // 0x803836EC
extern u8   lbl_80383790[];        // 0x80383790
}
extern "C++" s32 gZoneResourceSize;    // 0x805FB94C

extern "C" {

// Sum of the 13 zone-count words at +0x34..+0x64 of the global settings.
static s32 SettingsTotal(void) {
    u8* g = GetGlobalSettings();
    s32 sum = 0;
    for (s32 o = 0x34; o <= 0x64; o += 4) sum += *(s32*)(g + o);
    return sum;
}

// 0x801F4B6C
// Finds the live entity whose handle (+0x30) equals id: walks the zone table in the world state
// (one 0x30-byte record per zone, kind 6 = zone slot) and each zone's two entity lists.
void* fn_801F4B6C(s32 id) {
    if ((u32)id == 0xFFFFFFFFu) return NULL;
    u8* found = NULL;
    s32 i = 0;
    s32 off = 0;
    while (i < SettingsTotal() + 1) {
        s32 zone = -1;
        if (i == SettingsTotal()) {
            zone = 0;
        } else {
            u8* rec = *(u8**)(gWorldState + 0xB0) + off;
            if (*(s32*)(rec + 4) == 6) zone = *(s32*)(rec + 8);
        }
        if (zone >= 0) {
            u8* world = *(u8**)gWorldState;
            u8* zr = *(u8**)(world + 0x1A4) + zone * gZoneResourceSize;
            s32 count = *(s32*)(zr + 0x94) + *(s32*)(zr + 0x98);
            bool hit = false;
            if (count > 0) {
                s32 j = 0;
                s32 byteOff = 0;
                do {
                    u8* z2 = *(u8**)(world + 0x1A4) + zone * gZoneResourceSize;
                    s32 n1 = *(s32*)(z2 + 0x94);
                    u8* ent;
                    if (j < n1) ent = *(u8**)(*(u8**)(z2 + 0x120) + byteOff);
                    else        ent = *(u8**)(*(u8**)(z2 + 0x12C) + (j - n1) * 4);
                    if (ent != NULL && *(u32*)(ent + 0x30) == (u32)id) {
                        found = ent;
                        hit = true;
                        break;
                    }
                    byteOff += 4;
                    j++;
                } while (j < count);
            }
            if (hit) break;
        }
        off += 0x30;
        i++;
    }
    return found;
}

// 0x801E6934
// Takes a free slot from the 0x88-byte id pool (gIdPool), returns its handle
// ((generation << 7) | index) and fills it with a load request: source a, destination dst,
// sizes b/c, completion callbacks fn_801E6570/fn_801E65A0 and, unless d is 0, e as the
// immediate size, else starts the read (fn_801E4FE0).
u32 fn_801E6934(u8* loader, void* a, void* dst, u32 b, u32 c, s32 d, s32 e, s32 f, s32 flags) {
    u8* slot;
    s32 cap = *(s32*)(gIdPool + 0x18);
    s32 head = *(s32*)(gIdPool + 0x10);
    if (head == (*(s32*)(gIdPool + 0x14) + 1) % cap) {
        slot = NULL;
    } else {
        s32 nh = head - 1;
        if (nh < 0) nh = cap - 1;
        *(s32*)(gIdPool + 0x10) = nh;
        if (*(s32*)(gIdPool + 0x14) < 0) {
            *(s32*)(gIdPool + 0x14) = nh;
            *(s32*)(gIdPool + 0x8) = 0;
        }
        s32 lo = *(s32*)(gIdPool + 0x10);
        s32 lo2 = lo;
        s32 ok;
        if (lo < 0 || lo >= *(s32*)(gIdPool + 0x18)) {
            ok = 0;
        } else {
            s32 tail = *(s32*)(gIdPool + 0x14);
            if (tail < lo) {
                if (lo <= tail) lo2 = lo + *(s32*)(gIdPool + 0x18);
                tail += *(s32*)(gIdPool + 0x18);
            }
            ok = 0;
            if (lo <= lo2 && lo2 <= tail) ok = 1;
        }
        slot = ok ? *(u8**)(gIdPool + 0x1C) + lo * 0x88 : NULL;
    }
    u32 handle = lbl_805F969C;
    if (slot != NULL) {
        *(s32*)(slot + 0x4) = 1;
        *(s32*)(slot + 0x70) = 0;
        *(s32*)(slot + 0x8) = 0;
        *(u32*)(slot + 0x74) = *(u32*)(gIdPool + 0x28);
        *(u32*)(gIdPool + 0x28) = *(u32*)(gIdPool + 0x28) + 1;
        *(s32*)(slot + 0x84) = 0;
        if (*(u32*)(gIdPool + 0x28) >= 0x01FFFFFFu) *(u32*)(gIdPool + 0x28) = 0;
        handle = (*(u32*)(slot + 0x74) << 7) | (u32)((s32)(slot - *(u8**)(gIdPool + 0x1C)) / 0x88);
    }
    u8* p;
    if (handle == 0xFFFFFFFFu) {
        p = NULL;
    } else {
        p = *(u8**)(loader + 4) + (handle & 0x7F) * 0x88;
        if ((handle >> 7) != *(u32*)(p + 0x74)) p = NULL;
    }
    if (d == 0) e = 0;
    if (p != NULL) {
        *(s32*)(p + 0x58) = flags;
        *(s32*)(p + 0x0) = 0;
        *(void**)(p + 0x5C) = dst;
        *(u32*)(p + 0x60) = b;
        *(u32*)(p + 0x64) = b;
        *(void**)(p + 0x54) = a;
        *(u32*)(p + 0x80) = *(u32*)(loader + 0x24);
        *(u32*)(p + 0x68) = c;
        *(void**)(p + 0x18) = dst;
        *(s32*)(p + 0x10) = 0;
        *(s32*)(p + 0x1C) = 0;
        *(s32*)(p + 0x20) = 0;
        *(s32*)(p + 0x14) = 0;
        *(void**)(p + 0x2C) = (void*)fn_801E6570;
        *(void**)(p + 0x30) = (void*)fn_801E65A0;
        *(s32*)(p + 0x44) = e;
        *(s32*)(p + 0x48) = f;
        *(s32*)(p + 0x4C) = 0;
        *(s32*)(p + 0x78) = 0;
        if (e == 0) {
            *(s32*)(p + 0x50) = fn_801E4FE0(lbl_8048A8B4, a, dst, (*(u32*)(p + 0x60) + 0x1F) & 0xFFFFFFE0u,
                                            *(s32*)(p + 0x68));
        } else {
            *(u32*)(p + 0x7C) = *(u32*)(loader + 0x20);
            *(s32*)(p + 0x60) = e;
            *(s32*)(p + 0x78) = 0;
        }
    }
    return handle;
}

// 0x801DA34C
// Per-frame step of an animation key track (stored as callback by fn_801DA7C8): a = owner,
// b = track entry. Returns 0 when the entry is not live for this owner, else 1.
s32 fn_801DA34C(u8* a, u8* b) {
    u32 flags = *(u32*)(b + 8);
    if (!(flags & 2)) return 0;
    if (flags & 0x80) return 1;
    if (*(u8**)(*(u8**)(b + 0x58) + *(s32*)(b + 0x50) * 4) != a) return 0;

    f32 w = *(f32*)(b + 0x44);
    f32 zero = lbl_805FFF50;
    s32 hasSub = *(s32*)(a + 0xB0) != 0;
    f32 k0 = *(f32*)(b + 0x38);
    f32 k1 = *(f32*)(b + 0x3C);
    f32 k2 = *(f32*)(b + 0x40);
    f32 k3 = *(f32*)(b + 0x44);
    f32 dist = *(f32*)(a + 0x20);
    if (w <= zero) return 0;

    u8* src = NULL;
    if (*(f32*)(b + 0x28) >= zero) {
        if (flags & 0x40) {
            *(u32*)(b + 8) = flags & ~0x40u;
            *(f32*)(b + 0x1C) = *(f32*)(b + 0x1C) + *(f32*)(a + 0x3C);
            *(f32*)(b + 0x20) = *(f32*)(b + 0x20) + *(f32*)(a + 0x40);
            *(f32*)(b + 0x24) = *(f32*)(b + 0x24) + *(f32*)(a + 0x44);
            *(f32*)(b + 0x28) = *(f32*)(b + 0x28) + *(f32*)(a + 0x48);
        }
        src = b + 0x1C;
    } else if (*(f32*)(b + 0x18) >= zero) {
        src = b + 0xC;
    }
    if (src != NULL && *(f32*)(src + 0xC) > lbl_805FFF50) {
        *(f32*)(a + 0x3C) = *(f32*)(src + 0x0);
        *(f32*)(a + 0x40) = *(f32*)(src + 0x4);
        *(f32*)(a + 0x44) = *(f32*)(src + 0x8);
        *(f32*)(a + 0x48) = *(f32*)(src + 0xC);
    }
    *(f32*)(a + 0x9C) = *(f32*)(b + 0x30);
    *(f32*)(a + 0x4C) = k0;
    *(f32*)(a + 0x50) = k1;
    *(f32*)(a + 0x54) = k2;
    *(f32*)(a + 0x58) = k3;
    *(u32*)(a + 0xA0) = *(u32*)(b + 0x2C);
    if (hasSub) {
        *(s32*)(lbl_8047C640 + 0x80) += 1;
        u8* o = *(u8**)(a + 0x10);
        *(s32*)(lbl_8047C640 + 0x84) += (o != NULL) ? *(s32*)(o + 0x18) : 0;
    } else {
        *(s32*)(lbl_8047C640 + 0x78) += 1;
        u8* o = *(u8**)(a + 0x10);
        *(s32*)(lbl_8047C640 + 0x7C) += (o != NULL) ? *(s32*)(o + 0x18) : 0;
    }
    if (*(f32*)(b + 0x28) >= lbl_805FFF50) *(f32*)(b + 0x28) = lbl_805FFF54;
    if (dist > lbl_805FFF58) fn_801D6780();
    *(u32*)(b + 8) |= 0x10;
    return 1;
}

// 0x8019EB94
// UIText: expands "$NAME" substitutions in the element's text (+0x170, capacity +0x150) from the
// table at lbl_805FB678 (entries of 0x14 bytes: name, callback, arg) and shifts the per-text
// marker offsets (12-byte entries at +0x140). Repeats until nothing is substituted.
// Returns 1 if anything changed (and refreshes the element), else 0.
s32 fn_8019EB94(u8* el) {
    if (*(char**)(el + 0x174) == NULL) return 0;
    char* tmp = (char*)MemAllocSmall(0x1000);
    u32 cap = *(u32*)(el + 0x150);
    char* text = *(char**)(el + 0x170);
    strncpy(text, *(char**)(el + 0x174), cap);
    text[cap] = 0;

    u8* m = *(u8**)(el + 0x140);
    char* c = *(char**)(el + 0x170);
    while ((s8)*c != 0 && m != NULL) {
        if ((s8)*c == 1 && m != NULL) {
            *(s32*)(m + 4) = (s32)(c - *(char**)(el + 0x170));
            m += 0xC;
        }
        c++;
    }

    s32 changed = 0;
    s32 again;
    do {
        char* src = *(char**)(el + 0x170);
        char* out = tmp;
        u8* ent = *(u8**)(el + 0x140);
        s32 shift = 0;
        again = 0;
        do {
            s32 pos = (s32)(src - *(char**)(el + 0x170)) + shift;
            while (ent != NULL && *(s32*)(ent + 4) < pos && *(s32*)ent != -1) ent += 0xC;
            u8 ch = (u8)*src;
            if (ch == 0x24) {   // '$'
                u8* sub = lbl_805FB678;
                src++;
                if (sub != NULL) {
                    for (;;) {
                        char* name = *(char**)sub;
                        if ((s8)*name == 0) { sub = NULL; break; }
                        if (fn_8033B1B8(name, src, (s32)strlen(name)) == 0) break;
                        sub += 0x14;
                    }
                }
                if (sub != NULL) {
                    again = 1;
                    char* val = (*(SubstFn*)(sub + 4))(*(void**)(sub + 8));
                    if (val != NULL) {
                        strcpy(out, val);
                        u32 vlen = (u32)strlen(val);
                        out += vlen;
                        s32 delta = (s32)(strlen(*(char**)sub) - vlen) + 1;
                        shift += delta;
                        u8* e2 = ent;
                        while (e2 != NULL && *(s32*)e2 != -1) {
                            *(s32*)(e2 + 4) -= delta;
                            e2 += 0xC;
                        }
                    }
                    changed = 1;
                    src += strlen(*(char**)sub);
                } else {
                    *out++ = 0x24;
                }
            } else {
                *out++ = (char)ch;
                src++;
            }
        } while ((s8)src[-1] != 0);

        u32 limit = *(u32*)(el + 0x150);
        if (strlen(tmp) > limit) {
            fn_8018D2B8("UIText : substituted text \"%s\" exceeded maximum length %d by %d characters... \n", tmp, limit,
                        strlen(tmp) - limit);
        }
        u32 cap2 = *(u32*)(el + 0x150);
        char* dst = *(char**)(el + 0x170);
        strncpy(dst, tmp, cap2);
        dst[cap2] = 0;
    } while (again != 0);

    if (changed != 0) fn_8019E27C(el);
    MemFreeSmall(tmp);
    return changed;
}

// Record kind: 1 when the high bit of byte 0 is set, else its low nibble.
// 0x801EDA4C
// Writes the orientation quaternion (4 floats at out) described by the packed record a1:
// kind 1 = quaternion stored in an animation record, 2 = euler angles of a keyed record,
// 4 = euler angles (in 2/pi units) of the world-state entry whose index is in a1+2.
// Anything else, or a failed lookup, gives the identity quaternion.
// The reference word is read as a whole so the code does not depend on byte order.
void fn_801EDA4C(u8* out, u8* a1) {
    u32 w = NodeRefWord(a1);
    u32 kind = NodeRefKind(w);
    switch (kind) {
    case 1: {
        s32 v = lbl_805FB6B8;
        if (NodeRefIsId(w)) v = (s32)w;
        s32 sp10 = v;
        u8* r = (u8*)fn_801B7198(&sp10, 0);
        if (r != NULL) {
            u8* q = *(u8**)(r + 4);
            *(f32*)(out + 0x0) = *(f32*)(q + 0xC);
            *(f32*)(out + 0x4) = *(f32*)(q + 0x10);
            *(f32*)(out + 0x8) = *(f32*)(q + 0x14);
            *(f32*)(out + 0xC) = *(f32*)(q + 0x18);
            return;
        }
        break;
    }
    case 2: {
        u8 key[4];
        key[0] = lbl_805FB710[0];
        key[1] = lbl_805FB710[1];
        *(u16*)(key + 2) = *(u16*)(lbl_805FB710 + 2);
        if (NodeRefKind(w) == 2) {
            key[1] = NodeRefByte1(w);
            key[0] = (u8)NodeRefNibble(w);
            *(u16*)(key + 2) = NodeRefHalf2(w);
        }
        u8 key2[4];
        if (fn_801C71EC(key) != 0) {
            key2[0] = key[0];
            key2[1] = key[1];
            *(u16*)(key2 + 2) = *(u16*)(key + 2);
        } else {
            key2[0] = lbl_805FB710[0];
            key2[1] = lbl_805FB710[1];
            *(u16*)(key2 + 2) = *(u16*)(lbl_805FB710 + 2);
        }
        u8* rec = fn_801C6BCC(key2);
        if (rec != NULL) {
            Vec3 e;
            Quat q;
            e.x = *(f32*)(rec + 0x24) * 0.63661975f;
            e.y = *(f32*)(rec + 0x28) * 0.63661975f;
            e.z = *(f32*)(rec + 0x2C) * 0.63661975f;
            EulerToQuat(&q, &e);
            *(f32*)(out + 0x0) = q.x;
            *(f32*)(out + 0x4) = q.y;
            *(f32*)(out + 0x8) = q.z;
            *(f32*)(out + 0xC) = q.w;
            return;
        }
        break;
    }
    case 4: {
        u32 idx = (NodeRefKind(w) == 4) ? (u32)NodeRefHalf2(w) : 0xFFFFFFFFu;
        u8* world = *(u8**)gWorldState;
        u8* rec = NULL;
        if (idx < *(u32*)(world + 0x24)) rec = *(u8**)(world + 0x28) + (idx << 5);
        if (rec != NULL) {
            Vec3 e;
            Quat q;
            e.x = *(f32*)(rec + 0x14) * 0.63661975f;
            e.y = *(f32*)(rec + 0x18) * 0.63661975f;
            e.z = *(f32*)(rec + 0x1C) * 0.63661975f;
            EulerToQuat(&q, &e);
            *(f32*)(out + 0x0) = q.x;
            *(f32*)(out + 0x4) = q.y;
            *(f32*)(out + 0x8) = q.z;
            *(f32*)(out + 0xC) = q.w;
            return;
        }
        break;
    }
    default:
        break;
    }
    *(f32*)(out + 0x8) = 0.0f;
    *(f32*)(out + 0x4) = 0.0f;
    *(f32*)(out + 0x0) = 0.0f;
    *(f32*)(out + 0xC) = 1.0f;
}

// 0x801EFC9C
// Loads the language text table "TEXT/STR_<lang>[_]<suffix>.TXT" (lang 1..4 = FR/GE/IT/SP, else EN)
// into self: +0x10 raw file, +0xC line count, +0x0 line pointers, +0x4 per-line '[' marker tables.
void fn_801EFC9C(u8* self, s32 lang, const char* suffix) {
    const char* tag;
    switch (lang) {
    case 1: tag = "FR"; break;
    case 2: tag = "GE"; break;
    case 3: tag = "IT"; break;
    case 4: tag = "SP"; break;
    default: tag = "EN"; break;
    }
    const char* sep = (*suffix != 0) ? "_" : "";
    char path[0x100];
    s32 size;
    s32 lineCount;
    s32 hex;
    SysSprintf(path, "%s/STR_%s%s%s.TXT", "TEXT", tag, sep, suffix);
    u8* data = fn_801EBEFC(path, &size, lbl_805FB914);
    *(u8**)(self + 0x10) = data;
    if (data == NULL) return;

    u8* p = data;
    u8* end = data + size;
    while (*p != 0xA && p < end) p++;
    if (p < end) {
        *p = 0;
        if (p[-1] == 0xD) p[-1] = 0;
    }
    u8* cur = p + 1;
    fn_8033861C((const char*)data, "%d\t%x", &lineCount, &hex);
    if (lineCount == 0) return;

    *(s32*)(self + 0xC) = lineCount;
    AllocFn alloc = *(AllocFn*)lbl_805FB914;
    u8** lines = (u8**)alloc((u32)lineCount * 4, "LocalStringTable", lbl_805FB914);
    *(u8***)(self + 0x0) = lines;
    if (lines == NULL) return;
    // the original leaves the third argument unset on this call
    u8** tables = (u8**)(*(AllocFn*)lbl_805FB914)(*(u32*)(self + 0xC) * 4, "LocalStringTable", lbl_805FB914);
    *(u8***)(self + 0x4) = tables;
    if (tables == NULL) return;

    for (u32 i = 0; i < *(u32*)(self + 0xC); i++) (*(u8***)(self + 0x0))[i] = lbl_805F96D8;

    u32 n = 0;
    s32 off = 0;
    while (n < *(u32*)(self + 0xC) && cur < end) {
        *(u8**)(*(u8**)(self + 0x0) + off) = cur;
        while (*cur != 0xA && cur < end) cur++;
        if (cur < end) {
            *cur = 0;
            if (cur[-1] == 0xD) cur[-1] = 0;
        }
        cur++;
        fn_801F020C(self, *(u8**)(*(u8**)(self + 0x0) + off));

        s32 brackets = 0;
        u8* t = *(u8**)(*(u8**)(self + 0x0) + off);
        while ((s8)*t != 0) {
            if ((s8)*t == 0x5B) brackets++;   // '['
            t++;
        }
        u8* table = NULL;
        if (brackets != 0) {
            table = (u8*)(*(AllocFn*)lbl_805FB914)((u32)(brackets + 1) * 0xC, "LocalStringTable", lbl_805FB914);
        }
        *(u8**)(*(u8**)(self + 0x4) + off) = table;
        fn_801F00DC(self, *(u8**)(*(u8**)(self + 0x0) + off), *(u8**)(*(u8**)(self + 0x4) + off));
        off += 4;
        n++;
    }
}

// 0x801B468C
// Activates a UI object: when the zone data of its entity (+4) is loaded it queues the zone slot
// (fn_801DA91C), then clears/sets the entity state flags (+0x20), runs fn_801EC080 and fires the
// two trigger actions (+0x34, and the one inside the object at +0x9C) when arg1 is 0.
void fn_801B468C(u8* self, s32 arg1) {
    u8* t = (*(u8* (**)(u8*))(*(u8**)self + 0xC))(self);
    u8* ent = *(u8**)(self + 4);
    if (t != NULL && !(*(u32*)(ent + 0x20) & 0x40)) {
        s32 z = *(u16*)(*(u8**)(self + 4) + 0x30);
        if (z <= *(s32*)(*(u8**)gWorldState + 4) && ZoneIsLoaded(gWorldState, z) &&
            ZoneIsLoaded(gWorldState, *(u16*)(*(u8**)(self + 4) + 0x32))) {
            u8* slot = *(u8**)(*(u8**)(*(u8**)gWorldState + 0x1B0) + *(u16*)(*(u8**)(self + 4) + 0x30) * 4);
            if (slot == NULL) return;
            fn_801DA91C(slot, t);
        } else {
            s32 lo = *(s32*)(GetGlobalSettings() + 0x68) + 1;
            s32 z2 = *(u16*)(*(u8**)(self + 4) + 0x30);
            bool zoned = false;
            if (lo <= z2) {
                s32 sum = *(s32*)(GetGlobalSettings() + 0x68);
                sum += *(s32*)(GetGlobalSettings() + 0x7C);
                sum += *(s32*)(GetGlobalSettings() + 0x6C);
                sum += *(s32*)(GetGlobalSettings() + 0x70);
                sum += *(s32*)(GetGlobalSettings() + 0x74);
                sum += *(s32*)(GetGlobalSettings() + 0x78);
                z2 = *(u16*)(*(u8**)(self + 4) + 0x30);
                if (z2 <= sum && ZoneIsLoaded(gWorldState, z2) &&
                    ZoneIsLoaded(gWorldState, *(u16*)(*(u8**)(self + 4) + 0x32))) {
                    zoned = true;
                }
            }
            if (zoned) {
                u8* slot = *(u8**)(*(u8**)(*(u8**)gWorldState + 0x1B0) + 4);
                if (slot == NULL) return;
                fn_801DA91C(slot, t);
            } else if (!(*(u32*)(*(u8**)(self + 4) + 0x20) & 0x04000000)) {
                u8* slot = **(u8***)(*(u8**)gWorldState + 0x1B0);
                if (slot == NULL) return;
                fn_801DA91C(slot, t);
            }
        }
    }

    u8* e = *(u8**)(self + 4);
    *(u32*)(e + 0x20) &= 0xF7F9F8FFu;
    if ((*(u8* (**)(u8*))(*(u8**)self + 0xC))(self) != NULL) {
        u8* o = (*(u8* (**)(u8*))(*(u8**)self + 0xC))(self);
        *(u32*)(o + 8) |= 2;
    }
    *(u32*)(*(u8**)(self + 4) + 0x20) |= 3;
    *(u32*)(*(u8**)(self + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(self + 4) + 0x20) |= 0x4000;
    fn_801EC080(self + 0x94, self);
    if (arg1 == 0) {
        u8* other = *(u8**)(self + 0x9C);
        if (other != NULL && ((*(u32*)(other + 0x64) >> 27) & 0xF) != 0) {
            TriggerDoAction(other + 0x64, self, NULL);
        }
        if (((*(u32*)(self + 0x34) >> 27) & 0xF) != 0) {
            TriggerDoAction(self + 0x34, self, NULL);
        }
    }
}

// 0x8015DF1C
// Converts one row of n YUYV pixels (2 bytes per pixel: Y, alternating Cb/Cr) to RGB.
// Chroma of the odd pixels is interpolated from the neighbouring pair. fmt 0x13 writes RGB565
// (u16), fmt 0x16 writes 0xFF,R,G,B bytes. The first argument is unused.
void fn_8015DF1C(void* unused, u8* out, const u8* in, s32 n, s32 fmt) {
    u8 buf[0xC00];
    (void)unused;
    s32 last = n - 2;
    u8* o = buf;
    const u8* s = in;
    if (last > 0) {
        u32 pairs = (u32)(last + 1) >> 1;
        for (u32 k = 0; k < pairs; k++) {
            o[0] = s[0];
            o[1] = s[1];
            o[2] = s[3];
            o[3] = s[2];
            o[4] = (u8)((s32)(s[1] + s[5]) >> 1);
            o[5] = (u8)((s32)(s[3] + s[7]) >> 1);
            s += 4;
            o += 6;
        }
    }
    s32 a = last * 2;
    s32 b = (n - 1) * 2;
    s32 oa = last * 3;
    s32 ob = (n - 1) * 3;
    u8 cb = in[a + 1];
    u8 cr = in[b + 1];
    buf[oa + 0] = in[a];
    buf[oa + 1] = cb;
    buf[oa + 2] = cr;
    buf[ob + 0] = in[b];
    buf[ob + 1] = cb;
    buf[ob + 2] = cr;

    const u8* p = buf;
    for (s32 i = 0; i < n; i++) {
        s32 v = (s32)p[2] - 0x80;
        s32 y = ((s32)p[0] - 0x10) * 0x48C;
        s32 u = (s32)p[1] - 0x80;
        s32 r = (y + v * 0x63C + 0x1F4) / 1000;
        s32 g = (((y - v * 0x32D) - u * 0x187) + 0x1F4) / 1000;
        s32 bl = (y + u * 0x7E2 + 0x1F4) / 1000;
        r = r < 0 ? 0 : (r > 0xFF ? 0xFF : r);
        g = g < 0 ? 0 : (g > 0xFF ? 0xFF : g);
        bl = bl < 0 ? 0 : (bl > 0xFF ? 0xFF : bl);
        if (fmt == 0x13) {
            *(u16*)out = (u16)((bl >> 3) | ((r << 8) & 0xF800) | ((g << 3) & 0xFFE0));
            out += 2;
        } else if (fmt == 0x16) {
            out[0] = 0xFF;
            out[1] = (u8)r;
            out[2] = (u8)g;
            out[3] = (u8)bl;
            out += 4;
        }
        p += 3;
    }
}

// 0x8019914C
// UI screen fixup walker: points node's +0x138 at the static table for its kind (+0x60, 0..6;
// kinds 2 and 3 also use the sub-kind at +0x13C), and recurses into child nodes. arg0 is unused.
// Returns -2 for a null node, else 0.
s32 fn_8019914C(s32 unused, u8* node) {
    (void)unused;
    if (node == NULL) return -2;
    switch (*(u32*)(node + 0x60)) {
    case 0:
        *(u8**)(node + 0x138) = lbl_80382C78;
        break;
    case 1:
        *(u8**)(node + 0x138) = lbl_80382DA4;
        break;
    case 2:
        switch (*(s32*)(node + 0x13C)) {
        case 0: *(u8**)(node + 0x138) = lbl_80382E2C; break;
        case 1: *(u8**)(node + 0x138) = lbl_803830B4; break;
        case 2: *(u8**)(node + 0x138) = lbl_80382F1C; break;
        case 3: *(u8**)(node + 0x138) = lbl_803836EC; break;
        case 4: fn_801A1B2C(node); break;
        default: fn_8018D370("UIScreenSet - Fixup unknown UIGraphic type ...\n"); break;
        }
        break;
    case 3: {
        switch (*(s32*)(node + 0x13C)) {
        case 0: *(u8**)(node + 0x138) = lbl_8038336C; break;
        case 1: *(u8**)(node + 0x138) = lbl_803832F4; break;
        case 2: *(u8**)(node + 0x138) = lbl_80383278; break;
        default:
            fn_8018D2B8("UIScreenSet - Fixup unknown UISlider type ...\n");
            *(u8**)(node + 0x138) = lbl_803833E0;
            break;
        }
        u8* list = *(u8**)(node + 0x15C);
        for (s32 i = 0; i < fn_8019555C(list); i++) {
            u8* child = *(u8**)fn_8019552C(list, i);
            if (child != NULL) *(u8**)(child + 0x138) = lbl_80382E2C;
        }
        break;
    }
    case 4: {
        *(u8**)(node + 0x138) = lbl_803835E8;
        fn_8019914C(0, *(u8**)(node + 0x1B4));
        u8* list = *(u8**)(node + 0x1BC);
        for (s32 i = 0; i < fn_8019555C(list); i++) {
            u8* child = *(u8**)fn_8019552C(list, i);
            if (child != NULL) fn_8019914C(0, child);
        }
        break;
    }
    case 5:
        *(u8**)(node + 0x138) = lbl_80383638;
        fn_8019914C(0, *(u8**)(node + 0x190));
        break;
    case 6: {
        *(u8**)(node + 0x138) = lbl_80383790;
        for (u8* c = *(u8**)(node + 0x120); c != NULL; c = *(u8**)(c + 0x124)) fn_8019914C(0, c);
        break;
    }
    default: {
        char buf[0x100];
        SysSprintf(buf, "Need a fixup for UIElement type %d.", *(u32*)(node + 0x60));
        break;
    }
    }
    return 0;
}

// 0x8019C71C
// Composes two 3x4 matrices (rows of 3 floats + translation row): this = other * this.
// Layout: floats 0..8 rotation rows, 9..11 translation.
void fn_8019C71C(u8* arg0, u8* arg1) {
    f32 a0 = *(f32*)(arg0 + 0x00), a1 = *(f32*)(arg0 + 0x04), a2 = *(f32*)(arg0 + 0x08);
    f32 a3 = *(f32*)(arg0 + 0x0C), a4 = *(f32*)(arg0 + 0x10), a5 = *(f32*)(arg0 + 0x14);
    f32 a6 = *(f32*)(arg0 + 0x18), a7 = *(f32*)(arg0 + 0x1C), a8 = *(f32*)(arg0 + 0x20);
    f32 a9 = *(f32*)(arg0 + 0x24), a10 = *(f32*)(arg0 + 0x28), a11 = *(f32*)(arg0 + 0x2C);

    f32 b0 = *(f32*)(arg1 + 0x00), b3 = *(f32*)(arg1 + 0x0C), b1 = *(f32*)(arg1 + 0x04);
    f32 b4 = *(f32*)(arg1 + 0x10), b6 = *(f32*)(arg1 + 0x18), b7 = *(f32*)(arg1 + 0x1C);
    f32 b9 = *(f32*)(arg1 + 0x24), b10 = *(f32*)(arg1 + 0x28);

    f32 n9  = b9 + ((b6 * a11) + ((b0 * a9) + (b3 * a10)));
    f32 n0  = (b6 * a2) + ((b0 * a0) + (b3 * a1));
    f32 n10 = b10 + ((b7 * a11) + ((b1 * a9) + (b4 * a10)));
    f32 n3  = (b6 * a5) + ((b0 * a3) + (b3 * a4));
    f32 n6  = (b6 * a8) + ((b0 * a6) + (b3 * a7));
    f32 n1  = (b7 * a2) + ((b1 * a0) + (b4 * a1));
    f32 n4  = (b7 * a5) + ((b1 * a3) + (b4 * a4));
    f32 n7  = (b7 * a8) + ((b1 * a6) + (b4 * a7));

    f32 b2 = *(f32*)(arg1 + 0x08);
    f32 b5 = *(f32*)(arg1 + 0x14);
    *(f32*)(arg0 + 0x00) = n0;
    f32 b8 = *(f32*)(arg1 + 0x20);
    *(f32*)(arg0 + 0x04) = n1;
    *(f32*)(arg0 + 0x10) = n4;
    *(f32*)(arg0 + 0x18) = n6;
    *(f32*)(arg0 + 0x1C) = n7;
    *(f32*)(arg0 + 0x0C) = n3;
    f32 n2 = (b8 * a2) + ((b2 * a0) + (b5 * a1));
    *(f32*)(arg0 + 0x24) = n9;
    *(f32*)(arg0 + 0x28) = n10;
    *(f32*)(arg0 + 0x08) = n2;
    f32 n5 = (b8 * a5) + ((b2 * a3) + (b5 * a4));
    f32 n11 = *(f32*)(arg1 + 0x2C) + ((b8 * a11) + ((b2 * a9) + (b5 * a10)));
    f32 n8 = (b8 * a8) + ((b2 * a6) + (b5 * a7));
    *(f32*)(arg0 + 0x14) = n5;
    *(f32*)(arg0 + 0x20) = n8;
    *(f32*)(arg0 + 0x2C) = n11;
}

}  // extern "C"
