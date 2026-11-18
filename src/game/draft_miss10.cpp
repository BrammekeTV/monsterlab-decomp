// Missing functions batch 10: UI graphic/anim helpers, effect-item lists, fader, sorted queue, sound slot free.
#include <string.h>

#include "types.h"

extern "C" {
void*       EventPost(s32 id, void* sender);                                    // 0x801F3918
void        SpawnEffectAt(void* fxMgr, u32 effect, const f32 posAndScale[4], int a); // 0x801C80E0
int         SysSprintf(char* buf, const char* fmt, ...);
void        fn_801609CC(u8* p, s32 h);                                          // 0x801609CC
void        fn_8016E304(void* cam);                                             // 0x8016E304
void        fn_8016FB20(void*);                                                 // 0x8016FB20
void        fn_80170044(void*);                                                 // 0x80170044
void        fn_8018B2CC(u8* obj, u8* entry, s32 a, s32 b, f32 c, f32 d);       // 0x8018B2CC
void        fn_8018D370(const char* fmt, ...);                                  // 0x8018D370
u8*         fn_8019FA78(s32 a, s32 b, s32 c);                                   // 0x8019FA78
u8*         fn_801A1B64(void);                                                  // 0x801A1B64
u8*         fn_801A2588(void);                                                  // 0x801A2588
u8*         fn_801A2DDC(void);                                                  // 0x801A2DDC
u8*         fn_801A84F8(void);                                                  // 0x801A84F8
void        fn_801A904C(u8* p);                                                 // 0x801A904C
void        fn_801A9174(u8* p, f32* pos, f32* out);                             // 0x801A9174
s32         fn_801B5A60(u8* p);                                                 // 0x801B5A60
void*       fn_801B7198(void* anim, s32 a);                                     // 0x801B7198
void        fn_801B75AC(void* p);                                               // 0x801B75AC
void*       fn_801BA9C4(u8* a0, s32 a1, u8* a2, s32 a3, u32 a4, u32 a5, u32 a6, u32 a7,
                        f32 f1, f32 f2, f32 f3, f32 f4, u32 a8, s32 a9);        // 0x801BA9C4
void        fn_801C83AC(u8* mgr, u8* item);                                     // 0x801C83AC
void        fn_801C9178(u8* a, u8* b);                                          // 0x801C9178
void        fn_801D0B48(u8* a, u8* b, u8* c);                                   // 0x801D0B48
const char* fn_801EB708(u32 hash);                                              // 0x801EB708
void        fn_801EBA20(void* list, void* item);                                // 0x801EBA20
u32         fn_801F38F8(u8* a3);                                                // 0x801F38F8
void        fn_801F5E48(void* a, s32 b);                                        // 0x801F5E48
void        fn_801F5E4C(void* a, s32 b);                                        // 0x801F5E4C
void        fn_8020A398(s32 handle);                                            // 0x8020A398

extern s32 gScreenHeight;           // 0x805F938C
extern s32 gScreenWidth;            // 0x805F9388
extern s32 lbl_8040E260;            // 0x8040E260
extern u8  lbl_8047C760[];          // 0x8047C760 (effect manager)
extern u8  lbl_8047D248[];          // 0x8047D248 (fader state)
extern u32 lbl_804DD140[];          // 0x804DD140 (sorted queue, 10 entries)
extern u8  lbl_804DD168[];          // 0x804DD168 (queue lock object)
extern u8  lbl_8051E6A8[];          // 0x8051E6A8 (sound voices, 0x114 each)
extern u8  lbl_80524E28[];          // 0x80524E28 (sound table entries, 0x58 each, 0x60 of them)
extern u8  lbl_805F9608;            // 0x805F9608 (fader mode)
extern f32 lbl_805F960C;            // 0x805F960C (fader duration)
extern void* lbl_805FB5F8;          // 0x805FB5F8
extern s32 lbl_805FB6B8;            // 0x805FB6B8
extern u8* lbl_805FB75C;            // 0x805FB75C (main view)
extern u8  lbl_805FB7F0;            // 0x805FB7F0 (fader initialized)
extern f32 lbl_805FB7F4;            // 0x805FB7F4 (fader elapsed)
extern u8  lbl_805FB7FC;            // 0x805FB7FC
extern u8* lbl_805FB800;            // 0x805FB800 (pointer to saved aspect pair)
extern f32 lbl_805FB804;            // 0x805FB804
extern f32 lbl_805FB808;            // 0x805FB808
extern s32 lbl_805FB950;            // 0x805FB950 (queue count)
extern s32 lbl_805FB954;            // 0x805FB954
extern s32 lbl_805FB958;            // 0x805FB958 (queue serial)
extern s32 lbl_805FB9A8;            // 0x805FB9A8
extern u32 lbl_805FB9F0;            // 0x805FB9F0
extern s32 lbl_805FBAE0;            // 0x805FBAE0 (audio busy flag)

typedef void* (*SpawnFn)(void*, u32, const f32*, void*);
typedef s32 (*VFn)(u8*, s32);

// 0x8019DB40
// Copies the text line number *(p+0x1E8) of the CR-separated buffer at +0x170 into the line cache at +0x1E0.
void fn_8019DB40(u8* p) {
    if (*(u32*)(p + 0x1ec) > 1u) {
        u8* dst = *(u8**)(p + 0x1e0);
        if (dst == NULL) return;
        memset(dst, 0, 0x200);
        char* base = *(char**)(p + 0x170);
        u32 n = *(u32*)(p + 0x1e8);
        char* cur = base;
        for (u32 i = 0; i < n; i++) {
            cur = strchr(cur, 0xd) + 1;
        }
        char* end = strchr(cur, 0xd);
        u32 len;
        if (end != NULL) {
            len = (u32)(end - cur);
        } else {
            len = (u32)strlen(cur);
        }
        *(s32*)(p + 0x1e4) = *(s32*)(p + 0x178) + (s32)((cur - base) * 4);
        memcpy(*(void**)(p + 0x1e0), cur, len);
    }
}

// 0x8019F9B8
// UIGraphic factory: builds the graphic of the given type (0..4) and stores the type at +0x13C.
u8* fn_8019F9B8(s32 type) {
    u8* g = NULL;
    switch (type) {
    case 0:
        g = fn_8019FA78(0x20, 0x20, -0xf01);
        break;
    case 1:
        g = fn_801A2DDC();
        break;
    case 2:
        g = fn_801A2588();
        break;
    case 3:
        g = fn_801A84F8();
        break;
    case 4:
        g = fn_801A1B64();
        break;
    default:
        fn_8018D370("UIGraphic : invalid type %d ...\n");
        break;
    }
    *(s32*)(g + 0x13c) = type;
    return g;
}

// 0x801A8654
// Shows/hides the graphic: on first show spawns the effect item, then toggles the visible flag on every part.
void fn_801A8654(u8* p, s32 on) {
    f32 pos[3];
    f32 out[3];
    f32 args[4];

    *(u8*)(p + 0x188) = (u8)on;
    if (on != 0 && *(s32*)(p + 0x178) == 0) {
        fn_801A9174(p, pos, out);
        args[0] = pos[0];
        args[1] = pos[1];
        args[2] = pos[2];
        args[3] = 0.0f;
        ((SpawnFn)SpawnEffectAt)(lbl_8047C760, *(u32*)(p + 0x140), args, p + 0x178);
        u8* item = *(u8**)(p + 0x178);
        if (item != NULL) {
            *(u8*)(item + 8) = 0;
            u8* e = item;
            for (s32 i = 0; i < *(s32*)(item + 0xc); i++) {
                u32* part = *(u32**)(e + 0x14);
                if (part != NULL) {
                    *part &= 0xfffffffb;
                }
                e += 4;
            }
            s32 off = 0;
            for (s32 i = 0; i < *(s32*)(*(u8**)(p + 0x178) + 0xc); i++) {
                u8* part = *(u8**)(*(u8**)(p + 0x178) + off + 0x14);
                if (part != NULL) {
                    *(u8**)(part + 0x74) = p;
                    *(s32*)(*(u8**)(*(u8**)(p + 0x178) + off + 0x14) + 0x7c) = *(s32*)(lbl_8047C760 + 0x24);
                }
                off += 4;
            }
        }
        (*(VFn*)(*(u8**)(p + 0x138) + 0x2c))(p, 0x10);
        (*(VFn*)(*(u8**)(p + 0x138) + 0x2c))(p, 2);
    }
    u8* item = *(u8**)(p + 0x178);
    if (item != NULL) {
        u8* e = item;
        for (s32 i = 0; i < *(s32*)(item + 0xc); i++) {
            u8* part = *(u8**)(e + 0x14);
            if (part != NULL) {
                if (on != 0) {
                    *(u32*)part |= 2;
                } else {
                    *(u32*)part &= 0xfffffffd;
                }
                if (on != 0) {
                    part = *(u8**)(e + 0x14);
                    *(f32*)(part + 0x20) = *(f32*)(part + 0x10);
                    *(f32*)(part + 0x24) = *(f32*)(part + 0x14);
                    *(f32*)(part + 0x28) = *(f32*)(part + 0x18);
                    *(f32*)(part + 0x2c) = *(f32*)(part + 0x1c);
                }
            }
            e += 4;
        }
    }
}

// 0x801A8AFC
// Releases the graphic's effect item: clears the owner back-pointer on each part and frees the item.
s32 fn_801A8AFC(u8* p) {
    fn_801A904C(p);
    if (*(u8**)(p + 0x178) != NULL) {
        s32 off = 0;
        for (s32 i = 0; i < *(s32*)(*(u8**)(p + 0x178) + 0xc); i++) {
            u8* part = *(u8**)(*(u8**)(p + 0x178) + off + 0x14);
            if (part != NULL) {
                *(s32*)(part + 0x74) = 0;
            }
            off += 4;
        }
    }
    fn_801C83AC(lbl_8047C760, *(u8**)(p + 0x178));
    *(u8**)(p + 0x178) = NULL;
    return 0;
}

// 0x801AB748
// Stores a float at +0x24, clears two counters on each of five child records, and sets +8 of the one at +0x18 to -1.
s32 fn_801AB748(u8* p, f32 v) {
    *(f32*)(p + 0x24) = v;
    u8* c = *(u8**)(p + 0x4);
    if (c != NULL) {
        *(s32*)(c + 0xc) = 0;
        *(s32*)(c + 0x10) = 0;
    }
    c = *(u8**)(p + 0x8);
    if (c != NULL) {
        *(s32*)(c + 0xc) = 0;
        *(s32*)(c + 0x10) = 0;
    }
    c = *(u8**)(p + 0xc);
    u8* q = p + 8;
    if (c != NULL) {
        *(s32*)(c + 0xc) = 0;
        *(s32*)(c + 0x10) = 0;
    }
    c = *(u8**)(q + 0x8);
    if (c != NULL) {
        *(s32*)(c + 0xc) = 0;
        *(s32*)(c + 0x10) = 0;
    }
    c = *(u8**)(q + 0xc);
    if (c != NULL) {
        *(s32*)(c + 0xc) = 0;
        *(s32*)(c + 0x10) = 0;
    }
    c = *(u8**)(p + 0x18);
    if (c != NULL) {
        *(s32*)(c + 0x8) = -1;
    }
    return 0;
}

// 0x801AC51C
// Looks up the anim entry for the sub-animation index at +0x38 (table chosen by the target's type key);
// binds it to the target object, or logs "Anim not found". Returns 1 on success.
s32 fn_801AC51C(u8* p, u8* obj, u8* target) {
    char buf[0x80];

    if (obj == NULL) return 0;
    if (target == NULL) return 0;
    u8* t = *(u8**)(p + 0x34);
    u16 key = *(u16*)(*(u8**)(target + 0x4) + 0x32);
    u8* tbl = NULL;
    for (s32 i = 0; i < 2; i++) {
        if ((s32)key == (s32)*(u16*)(t + 0x18 + i * 2)) {
            tbl = *(u8**)(t + 0x10 + i * 4);
            break;
        }
    }
    if (tbl == NULL) key = 0;
    s32 idx = *(s32*)(p + 0x38);
    u8* tbl2 = NULL;
    for (s32 i = 0; i < 2; i++) {
        if ((s32)key == (s32)*(u16*)(t + 0x18 + i * 2)) {
            tbl2 = *(u8**)(t + 0x10 + i * 4);
            break;
        }
    }
    u8* entry;
    if (tbl2 != NULL && idx < *(s32*)(t + 0x1c) && idx >= 0) {
        entry = *(u8**)(tbl2 + idx * 4);
    } else {
        entry = NULL;
    }
    if (entry != NULL) {
        fn_8018B2CC(obj, entry, *(s32*)(p + 0xc), *(s32*)(p + 0x2c), *(f32*)(p + 0x1c), *(f32*)(p + 0x20));
        *(f32*)(obj + 0xb0) = *(f32*)(p + 0x0);
        *(f32*)(obj + 0xb4) = *(f32*)(p + 0x0);
        *(f32*)(obj + 0xe8) = *(f32*)(p + 0x8);
        *(u32*)(obj + 0xf0) = *(u32*)(obj + 0xf0) | *(u32*)(p + 0xc);
        *(u8**)(target + 0x84) = p;
        return 1;
    }
    SysSprintf(buf, "Anim not found: %s\n", fn_801EB708(*(u32*)(p + 0x30)));
    return 0;
}

// 0x801B69F8
// Releases the object's sound handle (+0xAC); posts event 0x71 first unless the type flag 0x04000000 is set.
void fn_801B69F8(u8* p) {
    if (*(s32*)(p + 0xac) != -1) {
        if (!(*(u32*)(*(u8**)(p + 0x4) + 0x20) & 0x04000000)) {
            if (fn_801F38F8(p) != 0) {
                u8* ev = (u8*)EventPost(0x71, p);
                if (ev != NULL) {
                    *(s32*)(ev + 0x4) = -1;
                }
            } else if ((u32)(*(s32*)(p + 0x30) + 0x10000) != 0xffffu) {
                u8* ev = (u8*)EventPost(0x71, NULL);
                if (ev != NULL) {
                    *(s32*)(ev + 0x4) = *(s32*)(p + 0x30);
                }
            }
        }
        fn_8020A398(*(s32*)(p + 0xac));
        *(s32*)(p + 0xac) = -1;
    }
}

// 0x801B72A0
// Packs the target's anim info into the flag word at *p: bit 19 set, bit 18 from type flag, type key, anim index.
void fn_801B72A0(u8* p, u8* target) {
    u32* w = (u32*)p;
    if (target != NULL) {
        *w = (*w & 0xfff7ffffu) | 0x80000u;
        u32 key = *(u16*)(*(u8**)(target + 0x4) + 0x32);
        u32 bit = (*(u32*)(*(u8**)(target + 0x4) + 0x1c) >> 3) & 1;
        s32 idx = fn_801B5A60(target);
        u32 v = (*w & 0xfffbffffu) | (bit << 18);
        *w = v;
        if (key >= 3u) {
            *w = ((((((v & 0xfffcffffu) | 0x30000u) & 0xffff007fu) | (key << 7)) & 0xffffff83u) | ((u32)idx << 2));
            return;
        }
        *w = ((((v & 0xfffcffffu) | (key << 16)) & 0xffff0003u) | ((u32)idx << 2));
        return;
    }
    *w = *w & 0xfff7ffffu;
}

// 0x801B737C
// Links a child node into its parent's child list (+0xB0/+0x8 chain), zeroes its vector, optionally initializes it.
void fn_801B737C(u8* node, u8* parent, u8* obj, s32 init) {
    *(s32*)(node + 0xc) = -1;
    *(u8*)(node + 0x1d) = 0;
    *(u8**)(node + 0x0) = parent;
    *(u8**)(node + 0x4) = obj;
    *(u8**)(obj + 0xb4) = node;
    u8* last = *(u8**)(parent + 0xb0);
    if (last == NULL) {
        *(u8**)(parent + 0xb0) = node;
    } else {
        while (*(u8**)(last + 0x8) != NULL) {
            last = *(u8**)(last + 0x8);
        }
        *(u8**)(last + 0x8) = node;
    }
    *(s32*)(node + 0x8) = 0;
    *(f32*)(node + 0x10) = 0.0f;
    *(f32*)(node + 0x14) = 0.0f;
    *(f32*)(node + 0x18) = 0.0f;
    if (init != 0) {
        fn_801B75AC(node);
    }
}

// 0x801BA930
// Wrapper: forwards the string (with its length) and fixed defaults (0,0,0,1.0) to fn_801BA9C4.
void* fn_801BA930(u8* a0, u8* str, u32 a2, u32 a3, u32 a4, u32 a5) {
    u32 len = (u32)strlen((const char*)str);
    return fn_801BA9C4(a0, 1, str, 0, len, a2, a3, a4, 0.0f, 0.0f, 0.0f, 1.0f, a5, 0);
}

// 0x801C7E9C
// Detaches an effect item from its manager: removes all part nodes, unlinks it, and frees its group when empty.
void fn_801C7E9C(u8* item, u8* mgr) {
    *(s32*)(item + 0x74) = 0;
    u8* list = *(u8**)(item + 0x60);
    u8* first = *(u8**)list;
    u8* cur;
    if (first != NULL) {
        cur = first + 8;
    } else {
        cur = NULL;
    }
    for (;;) {
        list = *(u8**)(item + 0x60);
        first = *(u8**)list;
        u8* head = (first == NULL) ? NULL : first + 8;
        if (head == NULL) break;
        u8* next = *(u8**)(cur - 4);
        u8* nextItem = (next != NULL) ? next + 8 : NULL;
        fn_801D0B48(item, cur, mgr);
        cur = nextItem;
    }
    fn_801C9178(*(u8**)(mgr + 0x10), list);
    u8* owner = *(u8**)(mgr + 0x18);
    *(s32*)(mgr + 0x4) = *(s32*)(mgr + 0x4) - 1;
    u8* ownerFirst = *(u8**)(owner + 0x14);
    if (ownerFirst != NULL) {
        fn_801EBA20(owner, item);
        u8* group = *(u8**)(item + 0x68);
        if (group != NULL) {
            *(s32*)(group + 0x10) = *(s32*)(group + 0x10) - 1;
            **(s32**)(item + 0x6c) = 0;
            *(u8**)(item + 0x6c) = NULL;
            u8* g = *(u8**)(item + 0x68);
            if (*(s32*)(g + 0x10) == 0 && *(u8*)(g + 0x8) == 0) {
                u8* e = g;
                for (s32 i = 0; i < *(s32*)(g + 0xc); i++) {
                    u8* part = *(u8**)(e + 0x14);
                    if (part != NULL && ((*(u32*)(*(u8**)(part + 0x5c) + 0x1a8) >> 8) & 1)) {
                        fn_801C7E9C(part, mgr);
                    }
                    e += 4;
                }
                if (*(u8*)(g + 0x8) == 0) {
                    s32* slot = *(s32**)(g + 0x0);
                    if (slot != NULL) {
                        *slot = 0;
                        *(u8**)(g + 0x0) = NULL;
                    }
                    e = g;
                    for (s32 i = 0; i < *(s32*)(g + 0xc); i++) {
                        u8* part = *(u8**)(e + 0x14);
                        if (part != NULL) {
                            *(s32*)(part + 0x68) = 0;
                        }
                        e += 4;
                    }
                    e = g;
                    *(u8*)(g + 0x8) = 1;
                    for (s32 i = 0; i < *(s32*)(g + 0xc); i++) {
                        u32* part = *(u32**)(e + 0x14);
                        if (part != NULL) {
                            *part |= 4;
                        }
                        e += 4;
                    }
                    fn_801EBA20(*(u8**)(mgr + 0xc), g);
                    *(s32*)(mgr + 0x0) = *(s32*)(mgr + 0x0) - 1;
                }
            }
        }
    }
    if (*(s32*)(item + 0x78) != 0) {
        fn_801609CC((u8*)&lbl_8040E260, *(s32*)(item + 0x78));
        *(s32*)(item + 0x78) = 0;
    }
}

// 0x801C9D74
// Detaches every effect item in the manager's list (+0x18 owner, first node at +0x14).
void fn_801C9D74(u8* mgr) {
    u8* first = *(u8**)(*(u8**)(mgr + 0x18) + 0x14);
    u8* cur;
    if (first != NULL) {
        cur = first + 8;
    } else {
        cur = NULL;
    }
    while (cur != NULL) {
        u8* next = *(u8**)(cur - 4);
        u8* nextItem;
        if (next != NULL) {
            nextItem = next + 8;
        } else {
            nextItem = NULL;
        }
        fn_801C7E9C(cur, mgr);
        cur = nextItem;
    }
}

// 0x801D5D14
// Starts a screen fade (mode 0 = out, 1 = in, duration f): resets the fader quad and computes the initial alpha.
void fn_801D5D14(u8 mode, f32 dur) {
    if (lbl_805FB7F0 == 0 || lbl_805F9608 != mode) {
        u8* g = lbl_8047D248;
        u8* q = g + 0x10;
        u8* r = g + 0x70;
        f32 w = (f32)gScreenWidth;
        f32 h = (f32)gScreenHeight;
        *(f32*)(r + 0x8) = 0.0f;
        *(f32*)(g + 0x70) = 1.0f;
        *(f32*)(r + 0xc) = 0.0f;
        *(f32*)(r + 0x4) = 0.0f;
        *(f32*)(r + 0x10) = 1.0f;
        *(f32*)(r + 0x14) = 0.0f;
        *(f32*)(r + 0x18) = 0.0f;
        lbl_805FB7F0 = 1;
        lbl_805F9608 = mode;
        lbl_805F960C = dur;
        lbl_805FB7F4 = 0.0f;
        *(f32*)(g + 0x10) = 0.0f;
        *(f32*)(q + 0x4) = 0.0f;
        *(f32*)(q + 0xc) = 0.0f;
        *(f32*)(q + 0x10) = 0.0f;
        *(f32*)(q + 0x14) = 1.0f;
        *(f32*)(q + 0x18) = 1.0f;
        *(f32*)(q + 0x1c) = 0.0f;
        *(f32*)(q + 0x24) = 1.0f;
        *(f32*)(q + 0x28) = 0.0f;
        *(f32*)(q + 0x2c) = 1.0f;
        *(f32*)(q + 0x30) = 0.0f;
        *(f32*)(q + 0x34) = 1.0f;
        *(f32*)(q + 0x3c) = 0.0f;
        *(f32*)(q + 0x40) = 1.0f;
        *(f32*)(q + 0x44) = 1.0f;
        *(f32*)(q + 0x48) = 1.0f;
        *(f32*)(q + 0x4c) = 1.0f;
        *(f32*)(q + 0x54) = 1.0f;
        *(f32*)(q + 0x58) = 1.0f;
        *(f32*)(q + 0x5c) = 1.0f;
        *(f32*)(r + 0x1c) = 0.0f;
        *(f32*)(r + 0x20) = 1.0f;
        *(f32*)(r + 0x24) = 0.0f;
        *(f32*)(r + 0x28) = 0.0f;
        *(f32*)(r + 0x2c) = 0.0f;
        *(f32*)(g + 0x70) = 1.0f * w;
        *(f32*)(r + 0x4) = 0.0f * h;
        *(f32*)(r + 0x8) = 0.0f * 1.0f;
        *(f32*)(r + 0xc) = 0.0f * w;
        *(f32*)(r + 0x10) = 1.0f * h;
        *(f32*)(r + 0x14) = 0.0f * 1.0f;
        *(f32*)(r + 0x18) = 0.0f * w;
        *(f32*)(r + 0x1c) = 0.0f * h;
        *(f32*)(r + 0x20) = 1.0f * 1.0f;
        *(f32*)(r + 0x24) = 0.0f * w + 0.0f;
        *(f32*)(r + 0x28) = 0.0f * h + 0.0f;
        *(f32*)(r + 0x2c) = 0.0f * 1.0f + 0.0f;
        if (lbl_805FB7FC != 1) {
            f32 t = 0.0f + 0.0f;
            f32 d = (f32)dur;
            lbl_805FB7F4 = t;
            if (t > d) {
                t = d;
                lbl_805FB7F4 = d;
            }
            f32 total = (f32)dur;
            f32 alpha = 1.0f;
            if (total > 0.0001f) {
                f32 frac = (f32)t / total;
                f32 clamped = 0.0f;
                if (!(frac < 0.0f)) {
                    if (!(frac > 1.0f)) {
                        alpha = frac;
                    }
                    clamped = alpha;
                }
                alpha = clamped;
            }
            if (mode == 1) {
                alpha = 1.0f - alpha;
            }
            *(f32*)(g + 0xc) = alpha;
        }
    }
}

// 0x801D66C4
// Sets the main view aspect pair (a, b); re-initializes the camera if it changed and saves the pair.
void fn_801D66C4(f32 a, f32 b) {
    if (lbl_805FB804 != a || lbl_805FB808 != b) {
        s32 redo = 0;
        lbl_805FB804 = a;
        lbl_805FB808 = b;
        if (lbl_805FB5F8 != NULL) {
            redo = 1;
            fn_80170044(lbl_805FB75C);
        }
        *(f32*)(lbl_805FB75C + 0x194) = a;
        *(f32*)(lbl_805FB75C + 0x198) = b;
        fn_8016E304(lbl_805FB75C);
        if (redo != 0) {
            fn_8016FB20(lbl_805FB75C);
        }
        *(f32*)(lbl_805FB800 + 0x0) = a;
        *(f32*)(lbl_805FB800 + 0x4) = b;
    }
}

// 0x801DFF00
// Tests bit n (< 0x3FF) of the bitset starting at +4.
s32 fn_801DFF00(u8* p, u32 n) {
    if (n < 0x3ffu) {
        u32 w = *(u32*)(p + ((n >> 3) & 0x1ffffffcu) + 4);
        return ((1u << (n & 0x1f)) & w) != 0;
    }
    return 0;
}

// 0x801DFF9C
// Returns float slot n (< 0x20) of the array at +4, else 0.
f32 fn_801DFF9C(u8* p, u32 n) {
    if (n < 0x20u) {
        return *(f32*)(p + n * 4 + 4);
    }
    return 0.0f;
}

// 0x801ED130
// Frees the effect item in each of the 8 slots of every 0xC8-byte row, clearing the slot fields.
void fn_801ED130(u8* p) {
    s32 row = 0;
    s32 rowOff = 0;
    while (row < *(s32*)(p + 0x0)) {
        if (row >= 0 && row < *(s32*)(p + 0x0)) {
            s32 col = 0;
            s32 colOff = 0;
            do {
                u8* item = *(u8**)(*(u8**)(p + 0x4) + (colOff + rowOff) + 0x88);
                if (item != NULL) {
                    fn_801C83AC(lbl_8047C760, item);
                    s32 off = colOff + rowOff;
                    *(s32*)(*(u8**)(p + 0x4) + off + 0x88) = 0;
                    *(s32*)(*(u8**)(p + 0x4) + off + 0x68) = 0;
                    *(s32*)(*(u8**)(p + 0x4) + off + 0xa8) = 0;
                }
                col++;
                colOff += 4;
            } while (col < 8);
        }
        rowOff += 0xc8;
        row++;
    }
}

// 0x801EF894
// Clears the 0x394-byte record unless its tag at +0x390 is already "init" (0x696E6974); returns the record.
u8* fn_801EF894(u8* p) {
    if (*(u32*)(p + 0x390) != 0x696e6974u) {
        memset(p, 0, 0x394);
    }
    return p;
}

// 0x801F2C10
// Inserts a key into the sorted queue (max 10 entries, ordered by byte 2 of the key, then tagged with a serial in
// the low byte). Returns the stored entry, or lbl_805FB9F0 if the queue is full.
u32 fn_801F2C10(u32 key) {
    fn_801F5E4C(lbl_804DD168, 0);
    s32 n = lbl_805FB950;
    if ((u32)n == 0xa) {
        fn_801F5E48(lbl_804DD168, 0);
        return lbl_805FB9F0;
    }
    s32 idx = n;
    u32 prio = (key >> 16) & 0xff;
    if (n > 0) {
        for (s32 i = 0; i < n; i++) {
            u32 e = lbl_804DD140[i];
            u32 k = 0xffffffffu;
            if ((u32)(e + 0x10000) != 0xffffu) {
                k = (e >> 16) & 0xff;
            }
            if (k < prio) {
                idx = i;
                break;
            }
        }
    }
    for (s32 i = n; i > idx; i--) {
        lbl_804DD140[i] = lbl_804DD140[i - 1];
    }
    u32 serial = (u32)(lbl_805FB958 + 1) & 0xff;
    lbl_805FB958 = (s32)serial;
    lbl_804DD140[idx] = key | serial;
    if (idx == 0) {
        lbl_805FB954 = 1;
    }
    u32 e = lbl_804DD140[idx];
    u32 k = 0xffffffffu;
    lbl_805FB950 = lbl_805FB950 + 1;
    if ((u32)(e + 0x10000) != 0xffffu) {
        k = (e >> 8) & 0xff;
    }
    if (k & 4) {
        lbl_805FB9A8 = 1;
    }
    fn_801F5E48(lbl_804DD168, 0);
    return lbl_804DD140[idx];
}

// 0x801FF2A8
// Frees a sound slot by handle (index in the high half, serial in the low half): detaches the owner handle,
// stops the voice, and resets the slot record.
void fn_801FF2A8(u8* self, s32 handle) {
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    if (handle == -1) {
        lbl_805FBAE0 = saved;
        return;
    }
    u8* ent;
    s32 hi = handle >> 16;
    if (hi < 0 || hi >= 0x60) {
        ent = NULL;
    } else {
        ent = lbl_80524E28 + hi * 0x58;
        if ((s32)(u16)handle == (s32)(u16) * (u32*)ent) {
        } else {
            ent = NULL;
        }
    }
    if (ent == NULL) {
        lbl_805FBAE0 = saved;
        return;
    }
    s32 tmp = *(s32*)(ent + 0x44);
    u8* owner = (u8*)fn_801B7198(&tmp, 0);
    if (owner != NULL && *(s32*)(owner + 0xac) == handle) {
        *(s32*)(owner + 0xac) = -1;
    }
    s32 voice = *(s32*)(ent + 0x8);
    if (voice != -1) {
        *(s32*)(lbl_8051E6A8 + voice * 0x114 + 0x4) = 4;
    } else {
        s32 saved2 = lbl_805FBAE0;
        lbl_805FBAE0 = 1;
        if (ent == NULL || *(s32*)(ent + 0x0) == -1) {
            lbl_805FBAE0 = saved2;
        } else {
            s32 v2 = *(s32*)(ent + 0x8);
            if (v2 != -1) {
                *(s32*)(lbl_8051E6A8 + v2 * 0x114 + 0xc) = 0;
            }
            s32 tmp2 = *(s32*)(ent + 0x44);
            u8* owner2 = (u8*)fn_801B7198(&tmp2, 0);
            if (owner2 != NULL && *(s32*)(owner2 + 0xac) == *(s32*)(ent + 0x0)) {
                *(s32*)(owner2 + 0xac) = -1;
            }
            *(s32*)(ent + 0x0) = -1;
            *(s32*)(ent + 0x4) = -1;
            *(s32*)(ent + 0x8) = -1;
            *(s32*)(ent + 0xc) = 0;
            *(s32*)(ent + 0x10) = 0;
            *(s32*)(ent + 0x14) = -1;
            *(s32*)(ent + 0x44) = lbl_805FB6B8;
            *(s32*)(ent + 0x4c) = 0;
            *(s32*)(ent + 0x50) = 0;
            *(s32*)(ent + 0x30) = 0;
            *(f32*)(ent + 0x34) = 0.0f;
            *(s32*)(ent + 0x54) = 0;
            lbl_805FBAE0 = saved2;
        }
    }
    lbl_805FBAE0 = saved;
}

}  // extern "C"
