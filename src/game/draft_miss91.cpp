// Small functions in 0x80143Axx-0x8018Dxxx (renderer/debug table helpers, list append, video mode select).
#include <string.h>
#include "types.h"
#include "engine/rend.h"

extern "C" {
void     MemFreeSmall(void* p);                                                        // 0x801D4340
void     operator_delete(void* p);                                                     // 0x801D426C
u32      fn_80143A5C(const char* s);                                                   // 0x80143A5C
void     fn_80149B4C(void* a, void* b, void* c, s32 w, s32 h, const char* name);       // 0x80149B4C
void     fn_80187B68(void* p, s32 a, s32 b);                                           // 0x80187B68
s32      fn_80173400(void* a, s32 b);                                                  // 0x80173400
void     fn_8017514C(s32 a);                                                           // 0x8017514C
void     fn_80170C28(void* a, void* b, s32 c);                                         // 0x80170C28
void fn_80175C48(void);                                                           // 0x80175C48
void     fn_801731C8(void* p);                                                         // 0x801731C8
void     fn_8018A1FC(void* p);                                                         // 0x8018A1FC
s32      fn_8015BA30(void* p);                                                         // 0x8015BA30
void     fn_8014DE68(void);                                                            // 0x8014DE68
void     fn_80182A1C(void);                                                            // 0x80182A1C
void     fn_8017A56C(void);                                                            // 0x8017A56C
void     fn_8017A614(void);                                                            // 0x8017A614
void     fn_80182AB8(void);                                                            // 0x80182AB8
void     fn_80187E90(void* a, void* b, void* c, s32 idx);                              // 0x80187E90
void     fn_80188768(void* a, void* b, void* c, s32 idx);                              // 0x80188768
void     fn_80188FC0(void);                                                            // 0x80188FC0
void     fn_80189410(void);                                                            // 0x80189410

extern u8        lbl_8037617C[];     // 0x8037617C (vtable of the base object)
extern u8        lbl_80377938[];     // 0x80377938 (static debug buffer block)
extern u8        lbl_80412B80[];     // 0x80412B80 (static record)
extern u8        lbl_80412BC0[];     // 0x80412BC0 (static table)
extern const char lbl_803812E0[];    // 0x803812E0 (debug name string)
extern const char lbl_803812EC[];    // 0x803812EC (debug name string)
extern u8        sRendDLs[];         // 0x80401108 (display-list records, 0x94 bytes each)
extern s32       gViewportIndex;     // 0x805FB4BC
extern s32       lbl_805FB4C0;       // 0x805FB4C0
extern s32       gFrameBudget;       // 0x805FB4C8
extern u8*       lbl_805FB620[];     // 0x805FB620 (per-mode record pointers, index 0..1)
extern s32       lbl_805FB628;       // 0x805FB628
extern s32       lbl_805FB62C;       // 0x805FB62C
extern s32       lbl_805FB630;       // 0x805FB630
extern s32       lbl_805FB634;       // 0x805FB634 (current mode index)
extern u8        lbl_805FB638[];     // 0x805FB638 (video mode selector, read as s32)
extern s32       lbl_805FB63C;       // 0x805FB63C
extern s32       lbl_805FB640;       // 0x805FB640
extern s32       lbl_805FB644;       // 0x805FB644
}

extern "C" {

// 0x801E5F48
// Looks up a named entry: hashes the first 0x37 chars of name, then scans the 0x18-byte entries of the
// table at obj+0x7B4 (base at +0, count at +4) for one whose key matches and whose hash matches. Returns its value at +8.
s32 fn_801E5F48(u8* obj, const char* name, u32 key) {
    char buf[0x38];
    const char* src = name;
    u32 n = 0;
    memset(buf, 0, 0x38);
    while (*src != '\0' && n < 0x38) {
        buf[n] = *src;
        src++;
        n++;
    }
    buf[0x37] = '\0';
    u32 hash = fn_80143A5C(buf);

    u8* tbl = *(u8**)(obj + 0x7B4);
    u8* base = *(u8**)tbl;
    u32 count = *(u32*)(tbl + 4);
    u8* found = NULL;
    for (u32 i = 0; i < count; i++) {
        u8* e = base + i * 0x18;
        if (key == *(u32*)e && hash == *(u32*)(e + 0xC)) {
            found = e;
            break;
        }
    }
    if (found == NULL) {
        return 0;
    }
    return *(s32*)(found + 0x8);
}

// 0x801483FC
// Base destructor: restores the vtable, frees the two owned buffers (+4, +8) and clears their slots;
// deletes the object when del > 0. Returns obj.
u8* fn_801483FC(u8* obj, s32 del) {
    if (obj != NULL) {
        u8* buf = *(u8**)(obj + 0x4);
        *(void**)obj = lbl_8037617C;
        if (buf != NULL) {
            MemFreeSmall(buf);
            *(void**)(obj + 0x4) = NULL;
        }
        u8* buf2 = *(u8**)(obj + 0x8);
        if (buf2 != NULL) {
            MemFreeSmall(buf2);
            *(void**)(obj + 0x8) = NULL;
        }
        *(s32*)(obj + 0xC) = 0;
        if (del > 0) {
            operator_delete(obj);
        }
    }
    return obj;
}

// Advances a 0x20-byte ring cursor (cursor at flagOff-4) when the s16 flag at flagOff is set:
// clears the flag, stores the previous ring index at flagOff+2 and sets the ring index at base+0x380 to the new cursor & 0x1F.
static void StepRingCursor(u8* rec, u8* base, s32 flagOff) {
    if (*(s16*)(rec + flagOff) != 0) {
        s32 cur = *(s32*)(rec + flagOff - 4) + 0x20;
        *(s32*)(rec + flagOff - 4) = cur;
        *(s16*)(rec + flagOff) = 0;
        *(s16*)(rec + flagOff + 2) = (s16)*(s32*)(base + 0x380);
        *(s32*)(base + 0x380) = cur & 0x1F;
    }
}

// 0x80160AFC
// Runs StepRingCursor over the eight 0x70-byte records at arg0 (four cursors each).
void fn_80160AFC(u8* arg0) {
    u8* rec = arg0;
    for (s32 i = 0; i < 8; i++) {
        StepRingCursor(rec, arg0, 0x4);
        StepRingCursor(rec, arg0, 0x20);
        StepRingCursor(rec, arg0, 0x3C);
        StepRingCursor(rec, arg0, 0x58);
        rec += 0x70;
    }
}

// 0x801719DC
// Calls the hook at +0x30 (or treats it as passed when none), then for slots 1..3 that report busy at +0x10
// runs the three per-slot helpers; finally signals fn_8014DE68 when the render budget is exceeded.
void fn_801719DC(u8* arg0) {
    s32 ok;
    void* hook = *(void**)(arg0 + 0x30);
    if (hook != NULL) {
        ok = ((s32 (*)(u8*, s32))hook)(arg0, *(s32*)(arg0 + 0x34));
    } else {
        ok = 1;
    }
    if (ok != 0 && *(void**)(arg0 + 0x10) != NULL) {
        for (s32 slot = 1; slot <= 3; slot++) {
            if (fn_80173400(*(void**)(arg0 + 0x10), slot) != 0) {
                fn_8017514C(slot);
                fn_80170C28(arg0, *(void**)(arg0 + 0x10), slot);
                fn_80175C48();
            }
        }
        if (lbl_805FB4C0 != 0 && fn_8015BA30(sRendDLs + gViewportIndex * 0x94) + 0x10C0 > gFrameBudget) {
            fn_8014DE68();
        }
    }
}

// 0x8017323C
// Appends node to the list at arg0 (head at +0, tail at +4, count at +8) unless node already has an owner (+8).
void fn_8017323C(u8* list, u8* node) {
    if (*(void**)(node + 0x8) != NULL) {
        return;
    }
    *(u8**)(node + 0x8) = list;
    u8* tail = *(u8**)(list + 0x4);
    *(s32*)(list + 0x8) = *(s32*)(list + 0x8) + 1;
    if (tail != NULL) {
        *(u8**)tail = node;
        *(u8**)(node + 0x4) = tail;
        *(u8**)(list + 0x4) = node;
        return;
    }
    *(u8**)list = node;
    *(u8**)(list + 0x4) = node;
}

// 0x801762E8
// Runs the two teardown helpers for the 0x8018xxxx subsystem.
void fn_801762E8(void) {
    fn_80182A1C();
    fn_8017A56C();
}

// 0x8017630C
// Runs the two teardown helpers for the 0x8017Axxx / 0x80182Axx subsystem.
void fn_8017630C(void) {
    fn_8017A614();
    fn_80182AB8();
}

// 0x801842E0
// Walks the linked list starting at obj+4 (next pointer at +0); for each entry with flag bit 2 at -0xAC
// stores arg1/arg2 into its -0x88/-0x84 fields.
void fn_801842E0(void* arg0, s32 arg1, s32 arg2) {
    u8* p = *(u8**)((u8*)arg0 + 0x4);
    while (p != NULL) {
        if (*(s32*)(p - 0xAC) & 0x4) {
            *(s32*)(p - 0x88) = arg1;
            *(s32*)(p - 0x84) = arg2;
        }
        p = *(u8**)p;
    }
}

// 0x80184314
// Membership update of node+0xB8 in the list at arg0+4. The current owner (node+0xC0 - 4) is detached first when it
// is the list being updated; bit 2 of node+0xC mirrors whether the node is in the list (flag arg2 selects it).
void fn_80184314(void* arg0, void* arg1, s32 arg2) {
    u8* node = (u8*)arg1;
    u8* owner = *(u8**)(node + 0xC0);
    u8* prev = (owner != NULL) ? owner - 0x4 : NULL;
    s32 inList = (*(s32*)(node + 0xC) & 0x4) != 0;

    if (prev == (u8*)arg0 && ((arg2 != 0) == inList)) {
        return;
    }
    if (prev != NULL) {
        fn_801731C8(node + 0xB8);
        if (*(s32*)(node + 0xC) & 0x4) {
            *(s32*)(prev + 0x38) = *(s32*)(prev + 0x38) - 1;
        }
    }
    fn_8017323C((u8*)arg0 + 0x4, node + 0xB8);
    if (arg2 != 0) {
        *(s32*)(node + 0xC) = *(s32*)(node + 0xC) | 0x4;
    } else {
        *(s32*)(node + 0xC) = *(s32*)(node + 0xC) & ~0x4;
    }
}

// 0x80184490
// Same as fn_80184314 for the second list: owner at node+0xC (owner - 0x10), flag halfword at +0x12, node at +4, list at arg0+0x10.
void fn_80184490(void* arg0, void* arg1, s32 arg2) {
    u8* node = (u8*)arg1;
    u8* owner = *(u8**)(node + 0xC);
    u8* prev = (owner != NULL) ? owner - 0x10 : NULL;
    s32 inList = (*(s16*)(node + 0x12) & 0x4) != 0;

    if (prev == (u8*)arg0 && ((arg2 != 0) == inList)) {
        return;
    }
    if (prev != NULL) {
        fn_801731C8(node + 0x4);
        if (*(s16*)(node + 0x12) & 0x4) {
            *(s32*)(prev + 0x3C) = *(s32*)(prev + 0x3C) - 1;
        }
    }
    fn_8017323C((u8*)arg0 + 0x10, node + 0x4);
    if (arg2 != 0) {
        *(s16*)(node + 0x12) = (s16)(*(s16*)(node + 0x12) | 0x4);
    } else {
        *(s16*)(node + 0x12) = (s16)(*(s16*)(node + 0x12) & ~0x4);
    }
}

// 0x80187DB8
// Walks the entry table at arg0+4 (count at +0, 0xC-byte entries). Entries with a non-zero second word are
// dispatched to fn_80187E90 (first word set) or fn_80188768; the first live entry calls fn_80188FC0 when arg2 is NULL,
// and fn_80189410 runs at the end in that case.
void fn_80187DB8(void* arg0, void* arg1, void* arg2) {
    u8* a = (u8*)arg0;
    s32 seen = 0;
    s32 off = 0;
    for (s32 i = 0; i < *(s32*)a; i++) {
        u8* tbl = *(u8**)(a + 0x4);
        if (*(s32*)(tbl + off + 0x4) != 0) {
            if (seen == 0) {
                seen = 1;
                if (arg2 == NULL) {
                    fn_80188FC0();
                }
            }
            if (*(s32*)(*(u8**)(a + 0x4) + off) != 0) {
                fn_80187E90(arg0, arg1, arg2, i);
            } else {
                fn_80188768(arg0, arg1, arg2, i);
            }
        }
        off += 0xC;
    }
    if (seen != 0 && arg2 == NULL) {
        fn_80189410();
    }
}

// 0x80188C2C
// Selects the active video mode record: mode 0 is chosen by a height check (>= 0x2D0), modes 2 and 3 always pick record 1.
// On change it refreshes the static table at lbl_80412BC0 and the three derived globals from the record.
void fn_80188C2C(void) {
    s32 sel = *(s32*)lbl_805FB638;
    s32 want = 0;
    if (sel == 0) {
        if (VideoGetHeight(&gVideo) >= 0x2D0) {
            want = 1;
        }
    } else if (sel == 2 || sel == 3) {
        want = 1;
    }
    if (lbl_805FB634 != want) {
        lbl_805FB634 = want;
        u8* rec = lbl_805FB620[want];
        if (*(u8**)(lbl_80412BC0 + 0x10) != rec) {
            *(u8**)(lbl_80412BC0 + 0x10) = rec;
            *(s16*)(lbl_80412BC0 + 0x16) = -1;
            *(s16*)(lbl_80412BC0 + 0x14) = -1;
            *(s16*)(lbl_80412BC0 + 0x1A) = 0;
            *(s16*)(lbl_80412BC0 + 0x18) = 0;
        }
        s32 isFirst = ((want - 2) == 0) ? 1 : 0;
        lbl_805FB63C = (s32)(*(s8*)(rec + 0x2)) - (isFirst + 1);
        lbl_805FB640 = (s32)(*(s8*)(rec + 0x1));
        lbl_805FB644 = (s32)(*(s8*)(rec + 0x2));
    }
}

// 0x80188D24
// Initialises the two static debug records at lbl_80412B80 and lbl_80377938, clears the mode globals and refreshes the mode.
void fn_80188D24(void) {
    lbl_805FB620[0] = lbl_80377938 + 0x2040;
    fn_80149B4C(lbl_80412B80, lbl_80377938 + 0x40, lbl_80377938, 0x80, 0x80, lbl_803812E0);
    *(u8**)(lbl_805FB620[0] + 0x8C) = lbl_80412B80;
    lbl_805FB620[1] = lbl_80377938 + 0x7B98;
    fn_80149B4C(lbl_80412B80 + 0x20, lbl_80377938 + 0x3B98, lbl_80377938 + 0x3B58, 0x100, 0x80, lbl_803812EC);
    *(u8**)(lbl_805FB620[1] + 0x8C) = lbl_80412B80 + 0x20;
    fn_80187B68(lbl_80412B80 + 0x40, 2, 0x2000);
    fn_8018A1FC(lbl_80412B80 + 0x60);
    lbl_805FB628 = 0;
    lbl_805FB62C = 0;
    lbl_805FB630 = -1;
    lbl_805FB634 = -1;
    *(s32*)lbl_805FB638 = 0;
    fn_80188C2C();
}

// 0x8018CD34
// If bit 1 (0x2) of the flags at +0x64 is set, calls the function pointer at +0x1C of the object at +0x138, passing this.
s32 fn_8018CD34(u8* arg0) {
    if (*(s32*)(arg0 + 0x64) & 0x2) {
        u8* sub = *(u8**)(arg0 + 0x138);
        ((void (*)(u8*))*(void**)(sub + 0x1C))(arg0);
    }
    return 0;
}

// 0x8018CE64
// Copies float blocks into the object: 3 floats from a to +0x20, 4 floats from b to +0x2C, 12 floats from c to +0x3C.
// Each source is optional (NULL skips it).
void fn_8018CE64(void* arg0, void* arg1, void* arg2, void* arg3) {
    u8* obj = (u8*)arg0;
    s32 i;
    if (arg1 != NULL) {
        for (i = 0; i < 3; i++) {
            *(f32*)(obj + 0x20 + i * 4) = *(f32*)((u8*)arg1 + i * 4);
        }
    }
    if (arg2 != NULL) {
        for (i = 0; i < 4; i++) {
            *(f32*)(obj + 0x2C + i * 4) = *(f32*)((u8*)arg2 + i * 4);
        }
    }
    if (arg3 != NULL) {
        for (i = 0; i < 12; i++) {
            *(f32*)(obj + 0x3C + i * 4) = *(f32*)((u8*)arg3 + i * 4);
        }
    }
}

}  // extern "C"
