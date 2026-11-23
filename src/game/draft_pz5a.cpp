// UI system batch: menu list navigation, text colour runs, pad word builder, GX overlay setup,
// UI event dispatcher, hit tests between part lists.
#include "types.h"
#include "revolution.h"

struct ScriptFunc { u32 type; void* fn; u32 hash; };   // 0xC bytes (as in script_dispatch.cpp)

extern "C++" void* gEventMgr;            // 0x805FB6C8
extern "C++" u8 sRendDLs[];              // 0x80401108 (display-list records, 0x94 bytes each)

extern "C" {

void*  ObjAlloc(u32 size);                                         // 0x801D4414
BOOL   PadButtonHeld(s32 button);                                  // 0x801474EC
u32    PadGetButtons(s32 chan);                                    // 0x801B9B40
u32    PadGetHeld(s32 chan);                                       // 0x801B9B98
void   StickRead(s32 channel, f32* x, f32* y);                     // 0x80147704
BOOL   fn_80147554(s32 button);                                    // 0x80147554 (button pressed)
s32    fn_8014762C(s32 button);                                    // 0x8014762C (button released)
s32    fn_801481FC(s32 chan);                                      // 0x801481FC
void   fn_8014DE68(void);                                          // 0x8014DE68
void    fn_80151644(s32 a, s32 b, u8* quad);                        // 0x80151644
void   fn_80154318(void);                                          // 0x80154318
extern "C++" void fn_80154358(void);                                          // 0x80154358
s32    fn_8015BA30(void* dl);                                      // 0x8015BA30
s32    fn_8018BDE0(u8* p, s32 key);                                // 0x8018BDE0
u8*    fn_80196C9C(u8* a3);                                        // 0x80196C9C
void   fn_8019DB40(void* el);                                      // 0x8019DB40
void   fn_8019E024(f32* c);                                        // 0x8019E024 (RGBA -> HSVA in place)
void   fn_8019E140(f32* c);                                        // 0x8019E140 (HSVA -> RGBA in place)
void*  fn_801B7198(void* anim, s32 a);                             // 0x801B7198
void   fn_801B7484(u8* self, u8* a, u8* b, s32 idx, f32* d, f32* e);  // 0x801B7484
void   fn_801BF4B8(u8* node);                                      // 0x801BF4B8
u8*    fn_801C6BCC(u8* key);                                       // 0x801C6BCC
void   fn_801CA7B0(u32 mode);                                      // 0x801CA7B0
s32    fn_801E9678(void* list, void* handle);                      // 0x801E9678
void*  fn_801EB94C(void* p);                                       // 0x801EB94C
void*  fn_801F4B6C(s32 id);                                        // 0x801F4B6C
void*  EventPost(s32 id, void* sender);                            // 0x801F3918
void   EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
u8*    AnimSetGetClip(void* set, s32 index);                       // 0x801AC164
s32    fn_801AC51C(u8* p, u8* obj, u8* target);                    // 0x801AC51C
void   fn_801B1778(void* p, s32 a);                                // 0x801B1778
void   fn_801B6878(u8* obj, void* a1, s32 a2, s32 a3);             // 0x801B6878
void   fn_801B69F8(u8* p);                                         // 0x801B69F8

extern ScriptFunc gScriptFuncs[];   // 0x80484230
extern u8   lbl_80348950[];         // 0x80348950 (0x20-byte records, u32 at +0)
extern u8   lbl_80348A10[];         // 0x80348A10 (0x20-byte records, s32 at +0)
extern u32  lbl_80348AD0;           // 0x80348AD0
extern s32  lbl_8040ADF0[8];        // 0x8040ADF0
extern s32  lbl_8040AE10[8];        // 0x8040AE10
extern u32  lbl_80413410[];         // 0x80413410 (3 x 4 pad words: held, pressed, released)
extern u8   lbl_805F9212;           // 0x805F9212 (copy filter value)
extern u32  lbl_805FAEB8;           // 0x805FAEB8 (pad channel)
extern s32  lbl_805FB4B0;           // 0x805FB4B0
extern s32  lbl_805FB4BC;           // 0x805FB4BC (current display list)
extern s32  lbl_805FB4C0;           // 0x805FB4C0
extern s32  lbl_805FB4C8;           // 0x805FB4C8
extern u8   lbl_805FB50C;           // 0x805FB50C (GX shadow flags byte)
extern void* lbl_805FB658;          // 0x805FB658 (global render object)
extern u32  lbl_805FB7D0;           // 0x805FB7D0
extern u32  lbl_805FB7D4;           // 0x805FB7D4
extern u32  lbl_805FB7D8;           // 0x805FB7D8
extern u32  lbl_805FB7DC;           // 0x805FB7DC

// One BP register write (opcode 0x61 then the 32-bit word) through the write-gather pipe.
static void GxBp(u32 v) {
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = v;
}

// 0x801B8474
// Task update for a "play effect" event record (UI event block, byte +0x7F == 0x20). Fields of the
// record: +0 u16 handle, +4 u32 anim id, +8 u8 key0, +9 u8 key1, +A u16 key2, +C s32 script index.
// Looks up the anim object, runs the script callback gScriptFuncs[idx].fn(rec, anim, ev, entry)
// when the key resolves and the index is not -1, then adds the handle to the anim's list at +0xFC.
s32 fn_801B8474(u8* arg0) {
    if (*(u8*)(arg0 + 0x7F) != 0x20) {
        return 1;
    }
    u32 animId = *(u32*)(arg0 + 4);
    u8* anim = (u8*)fn_801B7198(&animId, 0);
    if (anim != NULL) {
        u8 key[4];
        key[0] = *(u8*)(arg0 + 8);
        key[1] = *(u8*)(arg0 + 9);
        *(u16*)(key + 2) = *(u16*)(arg0 + 0xA);
        u8* rec = fn_801C6BCC(key);
        if (rec != NULL) {
            s32 idx = *(s32*)(arg0 + 0xC);
            if (idx != -1) {
                ScriptFunc* ent = &gScriptFuncs[idx];
                typedef void (*ScriptCb)(u8*, u8*, u8*, ScriptFunc*);
                ((ScriptCb)ent->fn)(rec, anim, arg0, ent);
            }
        }
        u16 handle = *(u16*)(arg0 + 0);
        fn_801E9678(anim + 0xFC, &handle);
    }
    return 3;
}

// 0x8019F4C8
// Text element line wrap: walks the string at +0x170, counting lines in +0x1EC. A 0x0A (soft break)
// becomes 0x0D once the current line holds the per-line limit (+0x164 / float +0x78, at least 1).
// Then calls fn_8019DB40(el).
void fn_8019F4C8(u8* el) {
    s32 col = 1;
    u8* s = *(u8**)(el + 0x170);
    *(s32*)(el + 0x1EC) = 1;
    s32 limit = (s32)((f32)*(s32*)(el + 0x164) / *(f32*)(el + 0x78));
    if (limit <= 0) {
        limit = 1;
    }
    while (s != NULL && *(s8*)s != 0) {
        s8 c = *(s8*)s;
        if (c == 0xA) {
            if (col >= limit) {
                *s = 0xD;
                col = 1;
                *(s32*)(el + 0x1EC) = *(s32*)(el + 0x1EC) + 1;
            } else {
                col += 1;
                s += 1;
            }
        } else if (c == 0xD) {
            col = 1;
            *(s32*)(el + 0x1EC) = *(s32*)(el + 0x1EC) + 1;
        }
        s += 1;
    }
    fn_8019DB40(el);
}

// Packs four 0..1 floats into 0xAABBGGRR (each *255, truncated and clamped to 0..255).
static u32 PackColor(f32 r, f32 g, f32 b, f32 a) {
    s32 c0 = (s32)(255.0f * r);
    s32 c1 = (s32)(255.0f * g);
    s32 c2 = (s32)(255.0f * b);
    s32 c3 = (s32)(255.0f * a);
    u32 v0 = c0 < 0 ? 0 : (c0 > 0xFF ? 0xFF : (u32)c0);
    u32 v1 = c1 < 0 ? 0 : (c1 > 0xFF ? 0xFF : (u32)c1);
    u32 v2 = c2 < 0 ? 0 : (c2 > 0xFF ? 0xFF : (u32)c2);
    u32 v3 = c3 < 0 ? 0 : (c3 > 0xFF ? 0xFF : (u32)c3);
    return v0 | (v1 << 8) | (v2 << 16) | (v3 << 24);
}

// 0x8019E27C
// Text element colour pass: builds the per-character colour array (u32* at +0x178) for the string
// at +0x170. Base colour is the float RGBA at +0x80..+0x8C (scaled by the global tint from
// fn_80196C9C when present). Colour runs at +0x140 (0xC bytes each: +0 skip flag, +4 start offset,
// +8 value) switch the colour: 0x1010101 resets to the base colour, 0x1010102 rotates the hue by
// 180 degrees and brightens, any other value is a literal colour. Finally sets byte +0x148 and
// calls the function stored at (*(el+0x138) + 0x30)(el, 0x20).
void fn_8019E27C(u8* el) {
    u8* cur = *(u8**)(el + 0x170);
    u32* out = *(u32**)(el + 0x178);
    u8* run = *(u8**)(el + 0x140);
    f32 base[4];
    u8* tint = fn_80196C9C((u8*)lbl_805FB658);
    if (tint != NULL) {
        base[0] = *(f32*)(el + 0x80) * *(f32*)(tint + 0);
        base[1] = *(f32*)(el + 0x84) * *(f32*)(tint + 4);
        base[2] = *(f32*)(el + 0x88) * *(f32*)(tint + 8);
        base[3] = *(f32*)(el + 0x8C) * *(f32*)(tint + 0xC);
    } else {
        base[0] = *(f32*)(el + 0x80);
        base[1] = *(f32*)(el + 0x84);
        base[2] = *(f32*)(el + 0x88);
        base[3] = *(f32*)(el + 0x8C);
    }
    u32 color = PackColor(base[0], base[1], base[2], base[3]);
    while (*(s8*)cur != 0) {
        if (run != NULL) {
            s32 off = (s32)(cur - *(u8**)(el + 0x170));
            if (off >= *(s32*)(run + 4)) {
                if (*(s32*)(run + 0) == 0) {
                    u32 tag = *(u32*)(run + 8);
                    if (tag == 0x1010101u) {
                        // reset to the (untinted) base colour
                        color = PackColor(*(f32*)(el + 0x80), *(f32*)(el + 0x84),
                                          *(f32*)(el + 0x88), *(f32*)(el + 0x8C));
                    } else if (tag == 0x1010102u) {
                        f32 hsv[4];
                        hsv[0] = 0.003921569f * (f32)(color & 0xFF);
                        hsv[1] = 0.003921569f * (f32)((color >> 8) & 0xFF);
                        hsv[2] = 0.003921569f * (f32)((color >> 16) & 0xFF);
                        hsv[3] = 0.003921569f * (f32)(color >> 24);
                        fn_8019E024(hsv);
                        hsv[0] = hsv[0] + 180.0f;
                        if (hsv[0] >= 360.0f) {
                            hsv[0] = hsv[0] - 360.0f;
                        }
                        hsv[2] = (f32)((f64)hsv[2] + 0.25);
                        if (hsv[2] > 1.0f) {
                            hsv[2] = 1.0f;
                        }
                        fn_8019E140(hsv);
                        color = PackColor(hsv[0], hsv[1], hsv[2], hsv[3]);
                    } else {
                        color = tag;
                    }
                }
                run += 0xC;
            }
        }
        *out = color;
        out += 1;
        cur += 1;
    }
    *(u8*)(el + 0x148) = 1;
    typedef void (*ElFn)(u8*, s32);
    (*(ElFn*)(*(u8**)(el + 0x138) + 0x30))(el, 0x20);
}

// 0x801B5420
// Tries to start a path/probe effect for object a: the object's vfunc 0x10 yields a lookup source;
// fn_8018BDE0(src, c) must be >= 0. Allocates a 0x2C-byte record (flag byte +0x1C = f), then
// fn_801B7484 fills it in. Returns 1 when started, 0 otherwise.
s32 fn_801B5420(u8* a, u8* b, s32 c, f32* d, f32* e, s32 f) {
    typedef u8* (*SrcFn)(u8*);
    SrcFn src = *(SrcFn*)(*(u8**)a + 0x10);
    if (src(a) == NULL) {
        return 0;
    }
    if (fn_8018BDE0(src(a), c) < 0) {
        return 0;
    }
    u8* rec = (u8*)ObjAlloc(0x2C);
    if (rec == NULL) {
        return 0;
    }
    *(u8*)(rec + 0x1C) = (u8)f;
    fn_801B7484(rec, a, b, fn_8018BDE0(src(a), c), d, e);
    return 1;
}

// 0x801B9278
// Per-frame pad state update for a UI input block (dt in f1 is clamped to [0, 0.0333]).
// Layout: +0 u32 button mask (this block's pad), +4/+8 f32 stick scale x/y, +0xC u32 buttons now,
// +0x10 u32 buttons previous, +0x14 f32 idle time, +0x18 s32 pad index, +0x24 + 0xC*i (i<=64)
// entries whose bit 0 is cleared each frame, +0x328/+0x32C f32 stick x/y, +0x330 u8 disable flag.
void fn_801B9278(u8* p, f32 dt) {
    f32 step = 0.0f;
    if (dt >= 0.0f) {
        step = 0.0333f;
        if (dt <= 0.0333f) {
            step = dt;
        }
    }
    s32 idx = *(s32*)(p + 0x18);
    *(u32*)(p + 0x10) = *(u32*)(p + 0xC);
    u32 padMask;
    if (idx < 0 || idx >= 4) {
        padMask = 0;
    } else {
        padMask = lbl_80413410[idx];
    }
    *(u32*)(p + 0xC) = padMask & *(u32*)(p + 0);
    StickRead(*(s32*)(p + 0x18) * 0x10 + 0xE, (f32*)(p + 0x328), (f32*)(p + 0x32C));
    *(f32*)(p + 0x328) = *(f32*)(p + 0x328) * *(f32*)(p + 4);
    *(f32*)(p + 0x32C) = *(f32*)(p + 0x32C) * *(f32*)(p + 8);
    for (s32 i = 0; i <= 0x40; i++) {
        *(u32*)(p + 0x24 + i * 0xC) &= 0xFFFFFFFEu;
    }
    s32 xActive = 1;
    s32 active = 1;
    if (*(u32*)(p + 0xC) == 0) {
        f32 x = *(f32*)(p + 0x328);
        if (x > -0.0001f && x < 0.0001f) {
            xActive = 0;
        }
    }
    if (xActive == 0) {
        f32 y = *(f32*)(p + 0x32C);
        if (y > -0.0001f && y < 0.0001f) {
            active = 0;
        }
    }
    if (active != 0) {
        *(f32*)(p + 0x14) = 0.0f;
    } else {
        *(f32*)(p + 0x14) = *(f32*)(p + 0x14) + step;
    }
    if (*(u8*)(p + 0x330) != 0) {
        *(u32*)(p + 0xC) = 0;
        *(f32*)(p + 0x328) = 0.0f;
        *(f32*)(p + 0x32C) = 0.0f;
    }
}

// Menu list helpers. Items are intrusive: the link at (item - 4) points at the previous item's
// link, the link at (item - 8) at the next item's link; item = link + 8. The list owner at +0x60
// of the menu has the tail link at +0x14.
static u8* MenuPrev(u8* item) {
    u8* l = *(u8**)(item - 4);
    return l != NULL ? l + 8 : NULL;
}
static u8* MenuNext(u8* item) {
    u8* l = *(u8**)(item - 8);
    return l != NULL ? l + 8 : NULL;
}
static u8* MenuTail(u8* menu) {
    u8* l = *(u8**)(*(u8**)(menu + 0x60) + 0x14);
    return l != NULL ? l + 8 : NULL;
}
// Repeat count for held modifier buttons (D-pad style accelerators).
static s16 MenuStepCount(u32 held) {
    s16 n = 1;
    if (held & 0x1000) n = 3;
    if (held & 0x2000) n = (s16)(n + 4);
    if (held & 0x10) n = (s16)(n + 8);
    if (held & 0x20) n = (s16)(n + 0x10);
    return n;
}

// 0x801B3C20
// Menu input: arg0 = menu (+0x60 list owner, +0x64 selected item, +0x68 s16 visible rows,
// +0x6A s16 scroll top, +0x6C s16 selected index), arg1 = result slot (+0 u8 set, +4 value ptr,
// +8 list). Buttons: 1 = up, 2 = down (repeated by modifiers), 8/4 = item vfuncs at +0xC8/+0xC4,
// 0x40 = select (item +0xCC callback, +0xD0 value). Item callbacks take the item. Finally every
// item's +0xC0 callback is run (first to last) and the four alloc/free counters are cleared.
void fn_801B3C20(u8* arg0, u8* arg1) {
    typedef void (*ItemFn)(u8*);
    u32 pressed = PadGetButtons(lbl_805FAEB8);
    u32 held = PadGetHeld(lbl_805FAEB8);

    if ((pressed & 1) || ((held & 1) && (held & 0x100))) {
        s16 n = MenuStepCount(held);
        for (s16 i = 0; i < n; i++) {
            u8* prev = MenuPrev(*(u8**)(arg0 + 0x64));
            *(s16*)(arg0 + 0x6C) = (s16)(*(s16*)(arg0 + 0x6C) - 1);
            if (prev == NULL) {
                // wrap to the last item and recount the index
                prev = MenuTail(arg0);
                *(s16*)(arg0 + 0x6C) = -1;
                u8* t = prev;
                while (t != NULL) {
                    t = MenuPrev(t);
                    *(s16*)(arg0 + 0x6C) = (s16)(*(s16*)(arg0 + 0x6C) + 1);
                }
                s16 top = (s16)((*(s16*)(arg0 + 0x6C) - *(s16*)(arg0 + 0x68)) + 2);
                *(s16*)(arg0 + 0x6A) = top;
                if (top < 0) {
                    *(s16*)(arg0 + 0x6A) = 0;
                }
            }
            if (prev != NULL) {
                s16 lim = (s16)(*(s16*)(arg0 + 0x6A) - 1);
                *(u8**)(arg0 + 0x64) = prev;
                if (*(s16*)(arg0 + 0x6C) < lim) {
                    *(s16*)(arg0 + 0x6A) = lim;
                }
            }
        }
    }
    if ((pressed & 2) || ((held & 2) && (held & 0x100))) {
        s16 n = MenuStepCount(held);
        for (s16 i = 0; i < n; i++) {
            u8* next = MenuNext(*(u8**)(arg0 + 0x64));
            s16 idx = (s16)(*(s16*)(arg0 + 0x6C) + 1);
            *(s16*)(arg0 + 0x6C) = idx;
            if (next == NULL) {
                // wrap to the first item
                *(s16*)(arg0 + 0x6C) = 0;
                *(s16*)(arg0 + 0x6A) = 0;
                u8* last = MenuTail(arg0);
                u8* t = last;
                while (t != NULL) {
                    t = MenuPrev(t);
                    if (t != NULL) last = t;
                }
                *(u8**)(arg0 + 0x64) = last;
            } else {
                s16 top = *(s16*)(arg0 + 0x6A);
                s32 lim = top + *(s16*)(arg0 + 0x68) - 1;
                *(u8**)(arg0 + 0x64) = next;
                if (idx >= lim) {
                    *(s16*)(arg0 + 0x6A) = (s16)(top + 1);
                }
            }
        }
    }
    if ((pressed & 8) || ((held & 8) && (held & 0x100))) {
        u8* item = *(u8**)(arg0 + 0x64);
        ItemFn fn = *(ItemFn*)(item + 0xC8);
        if (fn != NULL) fn(item);
    }
    if ((pressed & 4) || ((held & 4) && (held & 0x100))) {
        u8* item = *(u8**)(arg0 + 0x64);
        ItemFn fn = *(ItemFn*)(item + 0xC4);
        if (fn != NULL) fn(item);
    }
    if (pressed & 0x40) {
        u8* item = *(u8**)(arg0 + 0x64);
        ItemFn fn = *(ItemFn*)(item + 0xCC);
        if (fn != NULL) fn(item);
        u32 value = *(u32*)(*(u8**)(arg0 + 0x64) + 0xD0);
        if (value != 0) {
            u32* slot = (u32*)fn_801EB94C(*(void**)(arg1 + 8));
            if (slot != NULL) {
                *slot = value;
                *(u8**)(arg1 + 4) = (u8*)slot;
                *(u8*)(arg1 + 0) = 1;
            }
        }
    }
    u8* it = MenuTail(arg0);
    u8* first = it;
    while (it != NULL) {
        first = it;
        it = MenuPrev(it);
    }
    while (first != NULL) {
        ItemFn upd = *(ItemFn*)(first + 0xC0);
        if (upd != NULL) upd(first);
        first = MenuNext(first);
    }
    lbl_805FB7D4 = 0;
    lbl_805FB7D0 = 0;
    lbl_805FB7DC = 0;
    lbl_805FB7D8 = 0;
}

// 0x801B19C4
// Hit test between two named parts lists. a/b: +0xC array of record pointers, +0x10 count (u32),
// record +8 holds its name hash; the chosen records have +0x1C (enabled byte), +0x20 flags (bit 1 =
// write result), +0x24 part pointer array, +0x28 part count, +4 owner record whose +8 is an id.
// Each enabled part (+0x38 byte != 0) of a is tested against each of b with the part's vfunc 0x14
// (part, &hitA, other, &hitB, c, d, e). The 0x6C-byte hit record layout (outA/outB and locals):
// +0..+0x18 seven f32 (+0x18 = distance), +0x1C..+0x3C nine f32, +0x50 s32, +0x54 s32 = 7,
// +0x58 part, +0x5C owner id, +0x60 owner, +0x64 list record, +0x68 other part.
// Returns 1 on the first pair that collides (results written to outA/outB), else 0.
s32 fn_801B19C4(u8* a, u32 hashA, u8* b, u32 hashB, u8* outA, u8* outB, s32 c, s32 d, u8 e) {
    u8* ra = NULL;
    for (u32 i = 0; i < *(u32*)(a + 0x10); i++) {
        u8* rec = *(u8**)(*(u8**)(a + 0xC) + i * 4);
        if (rec != NULL && hashA == *(u32*)(rec + 8)) {
            ra = rec;
            break;
        }
    }
    if (ra == NULL || *(u8*)(ra + 0x1C) == 0) {
        return 0;
    }
    u8* rb = NULL;
    for (u32 i = 0; i < *(u32*)(b + 0x10); i++) {
        u8* rec = *(u8**)(*(u8**)(b + 0xC) + i * 4);
        if (rec != NULL && hashB == *(u32*)(rec + 8)) {
            rb = rec;
            break;
        }
    }
    if (rb == NULL || ra == rb || *(u8*)(rb + 0x1C) == 0) {
        return 0;
    }
    u32 countA = *(u32*)(ra + 0x28);
    u32 countB = *(u32*)(rb + 0x28);
    s32 hit = 0;
    for (u32 ia = 0; ia < countA; ia++) {
        u8* pa = ia >= *(u32*)(ra + 0x28) ? NULL : *(u8**)(*(u8**)(ra + 0x24) + ia * 4);
        if (pa == NULL || *(u8*)(pa + 0x38) == 0) {
            continue;
        }
        for (u32 ib = 0; ib < countB; ib++) {
            u8* pb = ib >= *(u32*)(rb + 0x28) ? NULL : *(u8**)(*(u8**)(rb + 0x24) + ib * 4);
            if (pb == NULL || *(u8*)(pb + 0x38) == 0) {
                continue;
            }
            u32 hitA[0x6C / 4];
            u32 hitB[0x6C / 4];
            u8* hA = (u8*)hitA;
            u8* hB = (u8*)hitB;
            for (s32 k = 0; k <= 0x18; k += 4) {
                *(f32*)(hA + k) = 0.0f;
                *(f32*)(hB + k) = 0.0f;
            }
            for (s32 k = 0x1C; k <= 0x3C; k += 4) {
                *(f32*)(hA + k) = 0.0f;
                *(f32*)(hB + k) = 0.0f;
            }
            *(s32*)(hA + 0x50) = 0;
            *(s32*)(hA + 0x54) = 7;
            *(s32*)(hB + 0x50) = 0;
            *(s32*)(hB + 0x54) = 7;
            for (s32 k = 0x58; k <= 0x68; k += 4) {
                *(s32*)(hA + k) = 0;
                *(s32*)(hB + k) = 0;
            }
            typedef s32 (*TestFn)(u8*, u8*, u8*, u8*, s32, s32, u8);
            TestFn test = *(TestFn*)(*(u8**)pa + 0x14);
            if (test(pa, hA, pb, hB, c, d, e) == 0) {
                continue;
            }
            if ((*(u32*)(ra + 0x20) & 2) && (hit == 0 || *(f32*)(hA + 0x18) < *(f32*)(outA + 0x18))) {
                *(u32*)(outA + 0x5C) = *(u32*)(*(u8**)(rb + 4) + 8);
                *(u8**)(outA + 0x58) = pa;
                *(u8**)(outA + 0x60) = *(u8**)(rb + 4);
                *(u8**)(outA + 0x64) = rb;
                *(u8**)(outA + 0x68) = pb;
                for (s32 k = 0; k <= 0x18; k += 4) {
                    *(f32*)(outA + k) = *(f32*)(hA + k);
                }
            }
            if ((*(u32*)(rb + 0x20) & 2) && (hit == 0 || *(f32*)(hB + 0x18) < *(f32*)(outB + 0x18))) {
                *(u32*)(outB + 0x5C) = *(u32*)(*(u8**)(ra + 4) + 8);
                *(u8**)(outB + 0x58) = pb;
                *(u8**)(outB + 0x60) = *(u8**)(ra + 4);
                *(u8**)(outB + 0x64) = ra;
                *(u8**)(outB + 0x68) = pa;
                for (s32 k = 0; k <= 0x18; k += 4) {
                    *(f32*)(outB + k) = *(f32*)(hB + k);
                }
            }
            hit = 1;
            break;
        }
        if (hit != 0) {
            break;
        }
    }
    return hit;
}

// Builds a pad bit mask for one 16-button group starting at button index b: for each of the 14
// button slots (offsets 0xA, 0xB, 7, 5, 4, 6, 9, 8, 2, 1, 3, 0, 0xC, 0xD) calls test(slot) and sets
// the matching mask bit (0x10, 0x20, 0x200, 0x80, 0x40, 0x100, 0x800, 0x400, 1, 8, 2, 4, 0x1000,
// 0x2000).
static u32 PadMask(s32 (*test)(s32), s32 b) {
    u32 m = 0;
    if (test(b + 0xA)) m |= 0x10;
    if (test(b + 0xB)) m |= 0x20;
    if (test(b + 7)) m |= 0x200;
    if (test(b + 5)) m |= 0x80;
    if (test(b + 4)) m |= 0x40;
    if (test(b + 6)) m |= 0x100;
    if (test(b + 9)) m |= 0x800;
    if (test(b + 8)) m |= 0x400;
    if (test(b + 2)) m |= 0x1;
    if (test(b + 1)) m |= 0x8;
    if (test(b + 3)) m |= 0x2;
    if (test(b + 0)) m |= 0x4;
    if (test(b + 0xC)) m |= 0x1000;
    if (test(b + 0xD)) m |= 0x2000;
    return m;
}
static s32 PadTestHeld(s32 b) { return PadButtonHeld(b); }
static s32 PadTestPressed(s32 b) { return fn_80147554(b); }

// 0x801B95EC
// Rebuilds the pad word arrays lbl_80413410[chan] (held), [4 + chan] (pressed), [8 + chan]
// (released) for the four pad channels from the per-button queries; a channel that
// fn_801481FC reports as unavailable gets zeros. Each channel owns 16 button indices.
void fn_801B95EC(void) {
    for (s32 chan = 0; chan < 4; chan++) {
        s32 b = chan * 0x10;
        if (fn_801481FC(chan) == 0) {
            lbl_80413410[chan] = 0;
            lbl_80413410[4 + chan] = 0;
            lbl_80413410[8 + chan] = 0;
        } else {
            lbl_80413410[chan] = PadMask(PadTestHeld, b);
            lbl_80413410[4 + chan] = PadMask(PadTestPressed, b);
            lbl_80413410[8 + chan] = PadMask(fn_8014762C, b);
        }
    }
}

// 0x801BF714
// Draws a screen-space UI node through GX: ensures display-list space, resets the copy/scissor
// state, picks the node's texture record (+0x1FC, else *(*(node)+0x28), only used when
// *(*(*node + 0x64) + 3) is set), programs the TEV/texture/BP registers on a change, then submits
// a quad via fn_80151644 (0xB0 when *(*(*node + 0x64) + 1) is set, else 0x98). Fields: +0 owner
// (+0x28 default texture record, +0x2C mode, +0x64 settings), +0x38 u32 vertex count (must be >= 3),
// +0x48 quad data. Texture record: +8/+A u16 width/height, +0x11 u8 lod, +0x13 u8 format,
// +0x14 u8 wrap, +0x15 u8, +0x16 u8, +0x17 u8 index into the 0x20-byte tables at lbl_80348950/A10,
// +0x1C/+0x20 data addresses.
void fn_801BF714(u8* node) {
    if (*(u32*)(node + 0x38) < 3) {
        return;
    }
    u8* dl = sRendDLs + lbl_805FB4BC * 0x94;
    if (fn_8015BA30(dl) + 0x1176 > lbl_805FB4C8) {
        fn_8014DE68();
        fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94);
    }
    fn_801BF4B8(node);
    fn_801CA7B0(*(u32*)(*(u8**)node + 0x2C));
    lbl_805FB50C &= 0xF7;
    GxBp(0xFE00C000);
    GxBp(0);
    GxBp(0xFE00000F);
    GxBp(0x80000001);

    u8* tex = *(u8**)(node + 0x1FC);
    if (tex == NULL) {
        tex = *(u8**)(*(u8**)node + 0x28);
    }
    if (*(u8*)(*(u8**)(*(u8**)node + 0x64) + 3) == 0) {
        tex = NULL;
    }
    if (tex != NULL) {
        if (lbl_805FB4C0 != 0) {
            if (fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x10C0 > lbl_805FB4C8) {
                fn_8014DE68();
            }
        }
        if (!(lbl_805FB50C & 2)) {
            lbl_805FB50C |= 2;
            if (lbl_805FB50C & 1) {
                GxBp(0xFE00000F);
                GxBp(1);
                GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x103F; GXWGFifo._u32 = 1;
                GxBp(0xFE003C00);
                GxBp(0x400);
                GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x1010; GXWGFifo._u32 = 0x441;
                GxBp(0x283C0000);
                GxBp(0xC008F2A4);
                GxBp(0xC108E6F0);
                GxBp(0xC208F80F);
                GxBp(0xC308F070);
            } else {
                GxBp(0xFE00000F);
                GxBp(1);
                GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x103F; GXWGFifo._u32 = 1;
                GxBp(0xFE003C00);
                GxBp(0);
                GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x100E; GXWGFifo._u32 = 0x442;
                GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x1010; GXWGFifo._u32 = 0x442;
                GxBp(0x28380040);
                GxBp(0xC008F8AF);
                GxBp(0xC108F2F0);
            }
        }
        GxBp(0xFE000060);
        GxBp(((u32)*(u8*)(tex + 0x16) << 5) | 0x80000000u);
        if ((u32)lbl_8040ADF0[0] != *(u32*)(tex + 0x1C) || lbl_8040AE10[0] != 0) {
            lbl_805FB4B0 = lbl_805FB4B0 + 1;
            lbl_8040ADF0[0] = *(s32*)(tex + 0x1C);
            lbl_8040AE10[0] = 0;
            u32 cfg = lbl_80348AD0;
            GxBp(((((u32)*(u8*)(tex + 0x13) - 1) << 12) & 0xF000u) | 0x84000000u);
            u32 idx = *(u8*)(tex + 0x17);
            u32 w0 = *(u32*)(lbl_80348950 + idx * 0x20);
            s32 w1 = *(s32*)(lbl_80348A10 + idx * 0x20);
            GxBp((u32)(*(u16*)(tex + 8) - 1) | ((u32)(*(u16*)(tex + 0xA) - 1) << 10) |
                 ((u32)*(u8*)(tex + 0x14) << 20) | 0x88000000u);
            GxBp((w0 >> 5) | 0x8C0D0000u | 0x8000u);
            if (w1 >= 0) {
                GxBp(((u32)w1 >> 5) | 0x900D0000u | 0x8000u);
            } else {
                GxBp(0x90000000u);
            }
            GxBp(((*(u32*)(tex + 0x1C) >> 5) & 0xFFFFFFu) | 0x94000000u);
            if (*(u8*)(tex + 0x11) != 0) {
                u32 m = (cfg >> 9) & 0x3FF;
                u32 r98 = ((m | 0x98000000u) & ~0x3FC00u) | (((u32)*(u8*)(tex + 0x15) << 10) & 0x3FC00u);
                GxBp(r98);
                GxBp(0xFEFFFF00u);
                GxBp(0x0F000000u);
                GxBp(((*(u32*)(tex + 0x20) >> 5) & 0xFFFFFFu) | 0x64000000u);
                u32 r65 = ((m | 0x65000000u) & ~0x1FC00u) |
                          ((((u32)*(u8*)(tex + 0x11) >> 1) << 10) & 0x1FC00u);
                GxBp(r65);
                GxBp(0xFEFFFF00u);
                GxBp(0x0F000000u);
            }
        }
        GxBp((u32)(*(u16*)(tex + 8) - 1) | 0x30000000u);
        GxBp((u32)(*(u16*)(tex + 0xA) - 1) | 0x31000000u);
    } else if (lbl_805FB50C & 2) {
        u8 fl = lbl_805FB50C;
        lbl_805FB50C = fl & 0xFD;
        if (fl & 1) {
            GxBp(0xFE00000F);
            GxBp(0);
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x103F; GXWGFifo._u32 = 0;
            GxBp(0xFE003C00);
            GxBp(0);
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x1010; GXWGFifo._u32 = 0x441;
            GxBp(0x28380000);
            GxBp(0xC008F2A4);
            GxBp(0xC108E6F0);
        } else {
            GxBp(0xFE00000F);
            GxBp(0);
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x103F; GXWGFifo._u32 = 0;
            GxBp(0xFE003C00);
            GxBp(0);
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x100E; GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x1010; GXWGFifo._u32 = 0x442;
            GxBp(0x28380000);
            GxBp(0xC008FFFA);
            GxBp(0xC108FFD0);
        }
    }
    fn_80154358();
    if (*(u8*)(*(u8**)(*(u8**)node + 0x64) + 1) != 0) {
        fn_80151644(0xB0, *(s32*)(node + 0x38) * 2, *(u8**)(node + 0x48));
    } else {
        fn_80151644(0x98, *(s32*)(node + 0x38) * 2, *(u8**)(node + 0x48));
    }
    if (fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x10E8 > lbl_805FB4C8) {
        fn_8014DE68();
        fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94);
    }
    fn_80154318();
    GxBp(0xFE000FE0);
    GxBp(0x410004A0);
    lbl_805FB50C |= 8;
    GxBp(0xFE00C000);
    GxBp((u32)lbl_805F9212 << 14);
    GxBp(0xFE00000F);
    GxBp(0x80000005);
}

// 0x801B5D94
// UI object event dispatcher: obj (+4 body: +0..8 position, +0xC..0x18 orientation, +0x20 flags,
// +0x24 f32, +0x28 value, +0x39 byte; +0x8C child ctl, +0x9C owner, +0xB0 child list with next
// at +8 and the child object at +4), ev (+0 u32, type = bits 20..26 i.e. (ev0 >> 20) & 0x7F).
// Types: 0x62 set value, 0x63/0x7A vfunc 0x18, 0x6B/0x6C send event, 0x71 fn_801B69F8, 0x72
// fn_801B6878, 0x73 vfunc 0xA4, 0x75/0x76 set transform, 0x77 spawn, 0x78 set flag 0x200,
// 0x79 vfunc 0x30, 0x7B activate/deactivate, 0x7D play clip.
void fn_801B5D94(u8* obj, u8* ev) {
    u32 type = (*(u32*)ev >> 20) & 0x7F;
    switch (type) {
    case 0x78: {
        u8* body = *(u8**)(obj + 4);
        u32 fl = *(u32*)(body + 0x20);
        if (fl & 0x100) {
            *(u32*)(body + 0x20) = fl | 0x200;
        }
        break;
    }
    case 0x6B: {
        u32 id = *(u32*)(ev + 8);
        void* found = fn_801B7198(&id, 1);
        EventSend(gEventMgr, obj, found, *(u32*)(ev + 4), *(s32*)(ev + 0xC), *(s32*)(ev + 0x10), 1, 1);
        break;
    }
    case 0x6C:
        EventSend(gEventMgr, obj, obj, *(u32*)(ev + 4), 0, 0, 1, 0);
        break;
    case 0x75:
    case 0x76: {
        f32 x = *(f32*)(ev + 4);
        f32 y = *(f32*)(ev + 8);
        f32 z = *(f32*)(ev + 0xC);
        u8* body = *(u8**)(obj + 4);
        *(f32*)(body + 0) = x;
        *(f32*)(body + 4) = y;
        *(f32*)(body + 8) = z;
        f32 q3 = *(f32*)(ev + 0x1C);
        f32 q2 = *(f32*)(ev + 0x18);
        f32 q1 = *(f32*)(ev + 0x14);
        f32 q0 = *(f32*)(ev + 0x10);
        body = *(u8**)(obj + 4);
        *(f32*)(body + 0xC) = q0;
        *(f32*)(body + 0x10) = q1;
        *(f32*)(body + 0x14) = q2;
        *(f32*)(body + 0x18) = q3;
        *(u32*)(*(u8**)(obj + 4) + 0x20) |= 0x8000;
        *(u32*)(*(u8**)(obj + 4) + 0x20) |= 0x10000000;
        *(u32*)(*(u8**)(obj + 4) + 0x20) |= 0x4000;
        *(u32*)(*(u8**)(obj + 4) + 0x20) |= 0x10000000;
        break;
    }
    case 0x7D: {
        u8* set = *(u8**)(*(u8**)(obj + 0x9C) + 0x48);
        if (set != NULL) {
            typedef u8* (*GetFn)(u8*);
            u8* who = (*(GetFn*)(*(u8**)obj + 0x10))(obj);
            u8* clip = AnimSetGetClip(set, *(s32*)(ev + 4));
            fn_801AC51C(clip, who, obj);
            *(u8*)(*(u8**)(obj + 4) + 0x39) = (u8)*(u32*)(ev + 4);
        }
        break;
    }
    case 0x77: {
        f32 pos[3];
        pos[0] = *(f32*)(ev + 4);
        pos[1] = *(f32*)(ev + 8);
        pos[2] = *(f32*)(ev + 0xC);
        void* found = NULL;
        if (*(u32*)(ev + 0x20) & 0x80000) {
            u32 id = *(u32*)(ev + 0x20);
            found = fn_801B7198(&id, 1);
        }
        if (found == NULL) {
            found = fn_801F4B6C(*(s32*)(ev + 0x24));
        }
        u8* body = *(u8**)(obj + 4);
        if (*(u32*)(body + 0x20) & 0x4000000) {
            *(f32*)(body + 0x24) = *(f32*)(ev + 0x1C);
        }
        typedef void (*SpawnFn)(u8*, void*, u32, u32, f32*, f32);
        (*(SpawnFn*)(*(u8**)obj + 0x50))(obj, found, *(u32*)(ev + 0x10), *(u32*)(ev + 0x14), pos,
                                          *(f32*)(ev + 0x18));
        break;
    }
    case 0x79: {
        typedef void (*Fn30)(u8*, s32, s32);
        (*(Fn30*)(*(u8**)obj + 0x30))(obj, 0, *(u32*)(ev + 4) != 0);
        break;
    }
    case 0x7B: {
        typedef u8* (*GetCtl)(u8*);
        typedef void (*CtlFn)(u8*, u8*, u8*, u8*);
        typedef void (*SetFn)(u8*, s32);
        if (obj != NULL) {
            u8* body = *(u8**)(obj + 4);
            if (*(u32*)(body + 0x20) & 0x8000) {
                u8* ctl = (*(GetCtl*)(*(u8**)obj + 0xC))(obj);
                body = *(u8**)(obj + 4);
                *(u32*)(body + 0x20) &= ~0x8000u;
                if (ctl != NULL) {
                    (*(CtlFn*)(*(u8**)ctl + 0x14))(ctl, body, obj + 8, body + 0xC);
                }
            }
            u8* c = *(u8**)(obj + 0x8C);
            if (c != NULL && (*(u32*)(*(u8**)(obj + 4) + 0x20) & 0x4000)) {
                fn_801B1778(c, 0);
            }
            for (u8* ch = *(u8**)(obj + 0xB0); ch != NULL; ch = *(u8**)(ch + 8)) {
                u8* co = *(u8**)(ch + 4);
                if (co != NULL) {
                    u8* cc = *(u8**)(co + 0x8C);
                    if (cc != NULL && (*(u32*)(*(u8**)(co + 4) + 0x20) & 0x4000)) {
                        fn_801B1778(cc, 0);
                    }
                }
            }
        }
        if (*(u32*)(ev + 4) != 0) {
            (*(SetFn*)(*(u8**)obj + 0x14))(obj, 1);
        } else {
            (*(SetFn*)(*(u8**)obj + 0x14))(obj, 0);
        }
        if (obj != NULL) {
            u8* body = *(u8**)(obj + 4);
            if (*(u32*)(body + 0x20) & 0x8000) {
                u8* ctl = (*(GetCtl*)(*(u8**)obj + 0xC))(obj);
                body = *(u8**)(obj + 4);
                *(u32*)(body + 0x20) &= ~0x8000u;
                if (ctl != NULL) {
                    (*(CtlFn*)(*(u8**)ctl + 0x14))(ctl, body, obj + 8, body + 0xC);
                }
            }
            u8* c = *(u8**)(obj + 0x8C);
            if (c != NULL && (*(u32*)(*(u8**)(obj + 4) + 0x20) & 0x4000)) {
                fn_801B1778(c, 0);
            }
            for (u8* ch = *(u8**)(obj + 0xB0); ch != NULL; ch = *(u8**)(ch + 8)) {
                u8* co = *(u8**)(ch + 4);
                if (co != NULL) {
                    u8* cc = *(u8**)(co + 0x8C);
                    if (cc != NULL && (*(u32*)(*(u8**)(co + 4) + 0x20) & 0x4000)) {
                        fn_801B1778(cc, 0);
                    }
                }
            }
        }
        break;
    }
    case 0x63:
    case 0x7A: {
        typedef void (*Fn18)(u8*, u32);
        (*(Fn18*)(*(u8**)obj + 0x18))(obj, *(u32*)(ev + 4));
        break;
    }
    case 0x73: {
        typedef void (*FnA4)(u8*, u32, u32);
        (*(FnA4*)(*(u8**)obj + 0xA4))(obj, *(u32*)(ev + 8), *(u32*)(ev + 0xC));
        break;
    }
    case 0x72:
        fn_801B6878(obj, *(void**)(ev + 8), 0, 0);
        break;
    case 0x71:
        fn_801B69F8(obj);
        break;
    case 0x62: {
        u8* body = *(u8**)(obj + 4);
        u32 val = *(u32*)(ev + 4);
        if (!(*(u32*)(body + 0x20) & 0x4000000)) {
            u8* p = (u8*)EventPost(0x62, obj);
            if (p != NULL) {
                *(u32*)(p + 4) = val;
            }
        }
        body = *(u8**)(obj + 4);
        if (body != NULL) {
            *(u32*)(body + 0x28) = val;
        }
        break;
    }
    default:
        break;
    }
}

}  // extern "C"
