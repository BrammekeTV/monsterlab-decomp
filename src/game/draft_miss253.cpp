// Decompiled from m2c drafts, verified against asm: 0x8005CC30, 0x8005D3C4.
#include "types.h"
#include <math.h>

extern "C++" void* gScreenSets;     // 0x805FB654
extern void* lbl_805FAFB8;          // 0x805FAFB8 (puzzle object: +0xB8/+0xB9 mirror flags, +0xB4 cursor)
extern s32   lbl_805FAFBC;          // 0x805FAFBC
extern s32   lbl_805FAFA4;          // 0x805FAFA4 (cursor element hash)
extern u8    lbl_805F8310;          // 0x805F8310
extern f32   lbl_805FC99C;          // 0x805FC99C (0.5f)

extern "C" {
#include "game/script_callbacks.h"
}

extern u8 gGame[];                  // 0x803B53A0

extern "C" {
void* memset(void* p, int c, unsigned long n);
u8*   GetGestureDetector(void* player, int idx);     // 0x8004CED8
void  WorldSendEvent(void* world, u32 event, s32 from, s32 arg, BOOL queue);   // 0x800D87EC
void  fn_801F916C(f32* out, u8* p, s32 a);           // 0x801F916C
BOOL  fn_80147554(s32 button);                       // 0x80147554
void* ScreenSetsCurrent(void* sets);                 // 0x8018D1E4
void* ScreenSetFindElement(void* set, u32 hash);     // 0x80198F18
u32   StringHash(const char* s);
int   SysSprintf(char* buf, const char* fmt, ...);   // 0x80142EF8
u32   RandomU32(void);
s32   UISetTexture(u8* el, void* tex);               // 0x801A11FC
void* TextureLoadShared(const char* name);                 // 0x80109330 (texture lookup by name)
const char* fn_80027E50(u8* p, s32 idx, const char* name);   // 0x80027E50
void* memcpy(void* d, const void* s, unsigned long n);
u8*  fn_80052530(u8* p, s32 a, u8* b, s32 c);   // 0x80052530
void fn_800526E4(u8* p, s32 a, s32 b);          // 0x800526E4
void fn_8005B648(u8* p, f32 v);                 // 0x8005B648
void fn_8005E1B8(u8* p);                        // 0x8005E1B8
void fn_8005FD4C(u8* p, u8* entry);             // 0x8005FD4C

extern u8  lbl_80359AE8[];          // 0x80359AE8 (vtable)
extern f32 lbl_805FD3F0;            // 0x805FD3F0 (float constant)
extern f32 lbl_805FD3F8;            // 0x805FD3F8 (float constant)
extern f32 lbl_805FD3FC;            // 0x805FD3FC (float constant)
extern f32 lbl_805FD400;            // 0x805FD400 (float constant)

typedef u8* (*GetFn)(u8*);
typedef s32 (*UIVFn)(u8*, s32);

// Resets one 0x20-byte colour-entry slot (+8 = 0xFF, rest of the colour block zeroed).
static inline void ResetColorSlot(u8* e) {
    *(u8*)(e + 0x8)  = 0xff;
    *(u8*)(e + 0x9)  = 0;
    *(u16*)(e + 0xa) = 0;
    *(u16*)(e + 0xe) = 0xffff;
    *(u8*)(e + 0xd)  = 0;
    *(u8*)(e + 0xc)  = 0;
}

// Resets one 4-byte colour slot (0xFF, 0, 0).
static inline void ResetColor4(u8* e) {
    *(u8*)(e + 0x0)  = 0xff;
    *(u8*)(e + 0x1)  = 0;
    *(u16*)(e + 0x2) = 0;
}

// 0x8005CC30
// Constructor: base ctor, set vtable, reset the slot tables at +0xA04 (8 x 0x20), +0xB04 (4 x 0x20)
// and the colour tables at +0xBA0 (11 x 4), then flag the linked object at +4.
u8* fn_8005CC30(u8* p, s32 a, u8* b, s32 c) {
    fn_80052530(p, a, b, c);
    *(void**)p = lbl_80359AE8;
    for (int i = 0; i < 8; i++) ResetColorSlot(p + 0xa04 + i * 0x20);
    for (int i = 0; i < 4; i++) ResetColorSlot(p + 0xb04 + i * 0x20);
    for (int i = 0; i < 11; i++) ResetColor4(p + 0xba0 + i * 4);
    *(u8*)(p + 0xbd5) = 0;
    *(u16*)(p + 0xbd6) = 0xffff;
    *(u8*)(p + 0xbd4) = 0;
    u8* link = *(u8**)(p + 4);
    if (link) {
        *(u32*)(link + 0x1c) |= 0x20;
        *(u8*)(*(u8**)(p + 4) + 0x34) = 5;
    }
    *(s32*)(p + 0xc28) = 0;
    return p;
}

// Inserts a prototype entry into the 4-slot priority list at +0xB04 (sorted by the float at +0x1C),
// unless an entry with the same key (word 0) is already present with a priority >= val.
static void InsertEntry(u8* p, u32 key, f32 val) {
    u8 e[0x20];
    memset(e, 0, 0x20);
    *(u8*)(e + 0x8)  = 0xff;
    *(u8*)(e + 0x9)  = 0;
    *(u16*)(e + 0xa) = 0;
    *(u16*)(e + 0xe) = 0xffff;
    *(u8*)(e + 0xd)  = 0;
    *(u8*)(e + 0xc)  = 0;
    *(u32*)(e + 0x0) = key;
    *(f32*)(e + 0x1c) = val;
    u8* slot = p;
    for (int i = 0; i < 4; i++, slot += 0x20) {
        if (*(f32*)(slot + 0xb20) < val) {
            // shift lower-priority entries down one slot
            for (int j = 2; j >= i; j--) memcpy(p + 0xb04 + (j + 1) * 0x20, p + 0xb04 + j * 0x20, 0x20);
            memcpy(p + 0xb04 + i * 0x20, e, 0x20);
            break;
        } else if (*(u32*)(slot + 0xb04) == key) {
            break;
        }
    }
}

// Finds the first free (or matching) 8-byte slot in the table at +0xBE0 and claims it with a 0x32 value.
static void ClaimSlot(u8* p, u32 key) {
    u8* s = p;
    for (int i = 0; i < 8; i++, s += 8) {
        u32 v = *(u32*)(s + 0xbe0);
        if (v == 0 || v == key) {
            *(u32*)(p + i * 8 + 0xbe0) = key;
            *(u8*)(p + i * 8 + 0xbe4) = 0x32;
            break;
        }
    }
}

// 0x8005D3C4
// Resets the effect state (colours, timers, flags), seeds two default priority entries and slot
// keys, activates entries with positive priority, then continues in the base init.
void fn_8005D3C4(u8* p, s32 a, s32 b) {
    f32 zero = lbl_805FD3F0;
    *(f32*)(p + 0xbd8) = zero;
    *(u8*)(p + 0xc20) = 0;
    *(u8*)(p + 0xba4) = 0xff;
    *(u8*)(p + 0xba0) = 0xff;
    *(u8*)(p + 0xba8) = 0xff;
    *(s32*)(p + 0xc2c) = 0;
    *(s32*)(p + 0xc34) = 0;
    *(s32*)(p + 0xc30) = 0;
    *(s32*)(p + 0xc38) = 0;
    *(u16*)(p + 0xbd6) = 0xffff;
    *(u8*)(p + 0xb84) = 0;
    *(u8*)(p + 0xb85) = 0;
    *(u8*)(p + 0xb86) = 0;
    *(u8*)(p + 0xbdc) = 0;
    *(u8*)(p + 0xbdd) = 0;
    *(u32*)(p + 0xa08) &= 0xffffffc7u;
    *(u8*)(p + 0xbac) = 0xff;
    *(u8*)(p + 0xbbc) = 0xff;
    *(u8*)(p + 0xbb0) = 0xff;
    *(u8*)(p + 0xbc0) = 0xff;
    *(u8*)(p + 0xbb4) = 0xff;
    *(u8*)(p + 0xbc4) = 0xff;
    *(u8*)(p + 0xbb8) = 0xff;
    *(u8*)(p + 0xbc8) = 0xff;
    *(s32*)(p + 0xbcc) = 0;
    *(u16*)(p + 0xbd2) = 0xffff;
    *(u8*)(p + 0xbd0) = 0;
    *(u8*)(p + 0xbd1) = 0;
    fn_8005B648(p, lbl_805FD3F8);
    zero = lbl_805FD3F0;
    *(f32*)(p + 0xb94) = zero;
    *(f32*)(p + 0xb98) = zero;
    *(f32*)(p + 0xb9c) = zero;
    u8* target = (*(GetFn*)(*(u8**)p + 0xc))(p);
    if (target) {
        target = (*(GetFn*)(*(u8**)p + 0xc))(p);
        *(f32*)(target + 0x30) = lbl_805FD3F0;
    }
    memset(p + 0xbe0, 0, 0x40);
    ClaimSlot(p, 0x43c53070u);
    ClaimSlot(p, 0x8ad3ca22u);
    memset(p + 0xa04, 0, 0x100);
    memset(p + 0xb04, 0, 0x80);
    InsertEntry(p, 0x7c498ed2u, lbl_805FD3FC);
    InsertEntry(p, 0x183384ffu, lbl_805FD400);
    f32 z = lbl_805FD3F0;
    for (int i = 0; i < 4; i++) {
        if (*(f32*)(p + i * 0x20 + 0xb20) > z) fn_8005FD4C(p, p + 0xb04 + i * 0x20);
    }
    if (*(s32*)(p + 0xa04) == 0x14db00b3) *(u8*)(p + 0xbdc) = 1;
    fn_8005E1B8(p);
    fn_800526E4(p, a, b);
}

// Flips the x / y scale of a UI element when the puzzle mirror flags are set and refreshes it
// (virtual slot 0x2C, code 0x18).
static void MirrorScale(u8* el, bool flipX, bool flipY) {
    if (flipX) {
        *(f32*)(el + 0x74) = -1.0f * *(f32*)(el + 0x74);
        (*(UIVFn*)(*(u8**)(el + 0x138) + 0x2c))(el, 0x18);
    }
    if (flipY) {
        *(f32*)(el + 0x78) = -1.0f * *(f32*)(el + 0x78);
        (*(UIVFn*)(*(u8**)(el + 0x138) + 0x2c))(el, 0x18);
    }
}

// 0x80026C88
// Skull puzzle setup: picks a random puzzle (1-6) and a mirror flag, finds the UI elements for the
// picture, background, 8 shuffled piece slots (7 real pieces + 1 decoy from another puzzle), the
// cursor and 5 grass elements, loads their textures, and mirrors them when the flip flags are set.
void fn_80026C88(u8* p) {
    char name[6] = {'p', 'a', 'r', 't', '0', 0};
    char buf[0x80];
    u8* set;

    *(u8*)(p + 0xb8) = (u8)((RandomU32() & 1) ^ 1);
    *(s32*)(p + 0xbc) = (s32)(RandomU32() % 6) + 1;
    set = (u8*)ScreenSetsCurrent(gScreenSets);
    for (int k = 0; k < 8; k++) *(void**)(*(u8**)(p + 0xb0) + k * 0x48) = NULL;

    SysSprintf(buf, "Skull_%d_complete", *(s32*)(p + 0xbc));
    *(void**)(p + 0xc0) = ScreenSetFindElement(set, StringHash(buf));
    if (*(u8**)(p + 0xc0)) {
        void* tex = TextureLoadShared(fn_80027E50(p, *(s32*)(p + 0xbc), buf));
        if (tex) {
            *(void**)(p + 0x1f4) = tex;
            UISetTexture(*(u8**)(p + 0xc0), tex);
        }
    }
    SysSprintf(buf, "Skull_%d_background", *(s32*)(p + 0xbc));
    *(void**)(p + 0xc4) = ScreenSetFindElement(set, StringHash(buf));
    if (*(u8**)(p + 0xc4)) {
        void* tex = TextureLoadShared(fn_80027E50(p, *(s32*)(p + 0xbc), buf));
        if (tex) {
            *(void**)(p + 0x1f8) = tex;
            UISetTexture(*(u8**)(p + 0xc4), tex);
        }
    }
    if (*(u8*)((u8*)lbl_805FAFB8 + 0xb8)) {
        u8* el = *(u8**)(p + 0xc0);
        *(f32*)(el + 0x74) = -1.0f * *(f32*)(el + 0x74);
        (*(UIVFn*)(*(u8**)(el + 0x138) + 0x2c))(el, 0x18);
        el = *(u8**)(p + 0xc4);
        *(f32*)(el + 0x74) = -1.0f * *(f32*)(el + 0x74);
        (*(UIVFn*)(*(u8**)(el + 0x138) + 0x2c))(el, 0x18);
    }
    if (*(u8*)((u8*)lbl_805FAFB8 + 0xb9)) {
        u8* el = *(u8**)(p + 0xc0);
        *(f32*)(el + 0x78) = -1.0f * *(f32*)(el + 0x78);
        (*(UIVFn*)(*(u8**)(el + 0x138) + 0x2c))(el, 0x18);
        el = *(u8**)(p + 0xc4);
        *(f32*)(el + 0x78) = -1.0f * *(f32*)(el + 0x78);
        (*(UIVFn*)(*(u8**)(el + 0x138) + 0x2c))(el, 0x1a);
    }
    UIElement_SetVisibleFlag(**(u32**)(p + 0xc4), TRUE);
    SysSprintf(buf, "test_particle");
    *(void**)(p + 0xc8) = ScreenSetFindElement(set, StringHash(buf));

    // Place the 8 piece slots in random free positions: 7 real pieces, then 1 decoy.
    for (int i = 0; i < 8; i++) {
        s32 slot = -1;
        while (slot < 0) {
            slot = (s32)(RandomU32() & 7);
            if (*(s32*)(*(u8**)(p + 0xb0) + slot * 0x48) != 0) slot = -1;
        }
        s32 off = slot * 0x48;
        if (i < 7) {
            name[4] = (char)(i + 0x31);
            SysSprintf(buf, "Skull_%d_%s", *(s32*)(p + 0xbc), name);
            *(void**)(*(u8**)(p + 0xb0) + off) = ScreenSetFindElement(set, StringHash(buf));
            if (*(void**)(*(u8**)(p + 0xb0) + off)) {
                void* tex = TextureLoadShared(fn_80027E50(p, *(s32*)(p + 0xbc), buf));
                if (tex) {
                    *(void**)(p + i * 4 + 0x1fc) = tex;
                    UISetTexture(*(u8**)(*(u8**)(p + 0xb0) + off), tex);
                }
            }
            *(u8*)(*(u8**)(p + 0xb0) + off + 0x44) = 0;
        } else {
            s32 other;
            do {
                other = (s32)(RandomU32() % 5) + 1;
            } while (other == *(s32*)(p + 0xbc));
            name[4] = (char)(RandomU32() % 7 + 0x31);
            SysSprintf(buf, "Skull_%d_%s", other, name);
            *(void**)(*(u8**)(p + 0xb0) + off) = ScreenSetFindElement(set, StringHash(buf));
            if (*(void**)(*(u8**)(p + 0xb0) + off)) {
                void* tex = TextureLoadShared(fn_80027E50(p, other, buf));
                if (tex) {
                    *(void**)(p + 0x22c) = tex;
                    UISetTexture(*(u8**)(*(u8**)(p + 0xb0) + off), tex);
                }
            }
            *(u8*)(*(u8**)(p + 0xb0) + off + 0x44) = 1;
        }
        u8* ent = *(u8**)(p + 0xb0) + off;
        u8* el = *(u8**)ent;
        *(f32*)(ent + 0x4) = *(f32*)(el + 0x68);
        *(f32*)(ent + 0x8) = *(f32*)(el + 0x6c);
        *(f32*)(ent + 0xc) = *(f32*)(el + 0x70);
        if (*(u8*)((u8*)lbl_805FAFB8 + 0xb8)) {
            el = *(u8**)(*(u8**)(p + 0xb0) + off);
            *(f32*)(el + 0x74) = -1.0f * *(f32*)(el + 0x74);
            // mirror the stored x position about the picture element's x
            u8* pic = *(u8**)(p + 0xc0);
            ent = *(u8**)(p + 0xb0) + off;
            f32 dx = *(f32*)(pic + 0x68) - *(f32*)(ent + 0x4);
            *(f32*)(ent + 0x4) = *(f32*)(pic + 0x68) + dx;
        }
        if (*(u8*)((u8*)lbl_805FAFB8 + 0xb9)) {
            el = *(u8**)(*(u8**)(p + 0xb0) + off);
            *(f32*)(el + 0x78) = -1.0f * *(f32*)(el + 0x78);
            u8* pic = *(u8**)(p + 0xc0);
            ent = *(u8**)(p + 0xb0) + off;
            f32 dy = *(f32*)(pic + 0x6c) - *(f32*)(ent + 0x8);
            *(f32*)(ent + 0x8) = *(f32*)(pic + 0x6c) + dy;
        }
    }

    *(void**)(p + 0x188) = ScreenSetFindElement(set, StringHash("InvisibleCursor"));
    lbl_805F8310 = 1;
    lbl_805FAFBC = 0;
    u32 fingerHash = StringHash("finger1");
    f32* oldPos = NULL;
    if (*(u8**)((u8*)lbl_805FAFB8 + 0xb4)) {
        u8* f = *(u8**)((u8*)lbl_805FAFB8 + 0xb4);
        (*(UIVFn*)(*(u8**)(f + 0x138) + 0x30))(f, 2);
        oldPos = (f32*)(*(u8**)((u8*)lbl_805FAFB8 + 0xb4) + 0x68);
    }
    lbl_805FAFA4 = (s32)fingerHash;
    *(void**)((u8*)lbl_805FAFB8 + 0xb4) = ScreenSetFindElement(ScreenSetsCurrent(gScreenSets), fingerHash);
    if (oldPos) {
        f32 x = oldPos[0], y = oldPos[1], z = oldPos[2];
        u8* f = *(u8**)((u8*)lbl_805FAFB8 + 0xb4);
        *(f32*)(f + 0x68) = x;
        *(f32*)(f + 0x6c) = y;
        *(f32*)(f + 0x70) = z;
    }
    *(f32*)(*(u8**)((u8*)lbl_805FAFB8 + 0xb4) + 0x70) = lbl_805FC99C;
    {
        u8* f = *(u8**)((u8*)lbl_805FAFB8 + 0xb4);
        (*(UIVFn*)(*(u8**)(f + 0x138) + 0x2c))(f, 0x18);
    }

    if (set) {
        for (int k = 1; k < 6; k++) {
            u8* slotp = p + 4 + (k - 1) * 4;
            char gbuf[0x80];
            SysSprintf(gbuf, "Skull_%d_Grass%d", *(s32*)(p + 0xbc), k);
            *(void**)(slotp + 0x1d8) = ScreenSetFindElement(set, StringHash(gbuf));
            if (*(void**)(slotp + 0x1d8)) {
                void* tex = TextureLoadShared(fn_80027E50(p, *(s32*)(p + 0xbc), gbuf));
                if (tex) {
                    *(void**)(slotp + 0x214) = tex;
                    UISetTexture(*(u8**)(slotp + 0x1d8), tex);
                }
            }
            u8* el = *(u8**)(slotp + 0x1d8);
            if (el && (*(u32*)(el + 0x64) & 1)) {
                (*(UIVFn*)(*(u8**)(el + 0x138) + 0x2c))(el, 2);
                MirrorScale(el, *(u8*)((u8*)lbl_805FAFB8 + 0xb8) != 0, *(u8*)((u8*)lbl_805FAFB8 + 0xb9) != 0);
            }
        }
    }
}

static const u32 kHashWiimoteUp    = 0x8D6D65B1u;   // "Wiimote_up"
static const u32 kHashWiimoteDown  = 0x1A9EE60Fu;   // "Wiimote_down"
static const u32 kHashWiimoteLeft  = 0x7C06915Cu;   // "Wiimote_left"
static const u32 kHashWiimoteRight = 0xAE16C01Fu;   // "Wiimote_right"
static const u32 kHashRight        = 0x439EB7A4u;   // "right"
static const u32 kHashDown         = 0x2A6A7D8Fu;   // "Down"
static const u32 kHashLeft         = 0x4CF20ADCu;   // "left"
static const u32 kHashButtonA      = 0x8FA00AE4u;   // "buttona"

// True when the gesture direction v equals the normalized direction (x, y, z).
static bool DirMatches(const f32* v, f32 x, f32 y, f32 z) {
    f32 len2 = z * z + (x * x + y * y);
    f32 nx, ny, nz;
    if (len2 == 0.0f) {
        nx = ny = nz = 0.0f;
    } else {
        f32 s = (f32)(1.0 / sqrt((double)len2));
        nx = x * s;
        ny = y * s;
        nz = z * s;
    }
    return v[0] == nx && v[1] == ny && v[2] == nz;
}

// 0x800F6E68
// Wii remote tutorial prompt check: reads the swipe direction (gesture detector 0) or any D-pad
// press, highlights the arrow/button element matching the requested input hash (a0), and returns 1
// if that input was performed. When any input happened, hides all prompt elements and plays the
// success / failure animation (failure also sends a world event).
u32 /*0x06*/ CheckWiiInput(u32 a0) {
    u32 result = 0;
    s32 anyInput = 0;
    void* mgr = *(void**)(gGame + 0xd614);
    f32 dir[3] = {0.0f, 0.0f, 0.0f};

    if (GetGestureDetector(mgr, 0) != NULL) {
        u8* g = GetGestureDetector(mgr, 0);
        if (*(u8*)(g + 0x90) != 0) {
            f32 f = *(f32*)(g + 0x94);
            if (f > 0.0f) *(f32*)(g + 0x8) = f;
        }
        if (*(u8*)(g + 0x90) != 0) {
            f32 tmp[3];
            fn_801F916C(tmp, GetGestureDetector(mgr, 0), 0);
            anyInput = 1;
            dir[0] = tmp[0];
            dir[1] = tmp[1];
            dir[2] = tmp[2];
        } else {
            goto buttons;
        }
    } else {
buttons:
        if (fn_80147554(4) || fn_80147554(6) || fn_80147554(5) || fn_80147554(7)) anyInput = 1;
    }

    switch (a0) {
    case 0xED7EEA1Au:   // up
        UIElement_SetColor_Floats(kHashWiimoteUp, 1.0f, 1.0f, 1.0f, 1.0f);
        if (DirMatches(dir, 0.0f, 1.0f, 0.0f)) result = 1;
        break;
    case kHashRight:
        UIElement_SetColor_Floats(kHashWiimoteRight, 1.0f, 1.0f, 1.0f, 1.0f);
        if (DirMatches(dir, -1.0f, 0.0f, 0.0f)) result = 1;
        break;
    case kHashDown:
        UIElement_SetColor_Floats(kHashWiimoteDown, 1.0f, 1.0f, 1.0f, 1.0f);
        if (DirMatches(dir, 0.0f, -1.0f, 0.0f)) result = 1;
        break;
    case kHashLeft:
        UIElement_SetColor_Floats(kHashWiimoteLeft, 1.0f, 1.0f, 1.0f, 1.0f);
        if (DirMatches(dir, 1.0f, 0.0f, 0.0f)) result = 1;
        break;
    case kHashButtonA:
        UIElement_SetColor_Floats(0xA070273Fu, 1.0f, 1.0f, 1.0f, 1.0f);
        if (WasButtonPressed(1)) result = 1;
        break;
    case 0x16A95B5Eu:
        UIElement_SetColor_Floats(0x39797685u, 1.0f, 1.0f, 1.0f, 1.0f);
        if (WasButtonPressed(2)) result = 1;
        break;
    }

    if (anyInput) {
        UIElement_SetColor_Floats(kHashWiimoteUp, 1.0f, 1.0f, 1.0f, 0.0f);
        UIElement_SetColor_Floats(kHashWiimoteDown, 1.0f, 1.0f, 1.0f, 0.0f);
        UIElement_SetColor_Floats(kHashWiimoteLeft, 1.0f, 1.0f, 1.0f, 0.0f);
        UIElement_SetColor_Floats(kHashWiimoteRight, 1.0f, 1.0f, 1.0f, 0.0f);
        UIElement_SetColor_Floats(0xA070273Fu, 1.0f, 1.0f, 1.0f, 0.0f);
        UIElement_SetColor_Floats(0x39797685u, 1.0f, 1.0f, 1.0f, 0.0f);
        if (result) {
            UIElement_PlayUIAnim(0x3EF26639u, 0x35231DF0u);
        } else {
            UIElement_PlayUIAnim(0x3EF26639u, 0x476CDEB7u);
            WorldSendEvent(gGame + 0xd5f0, 0xADCA036Fu, 0, 0, 0);
        }
    }
    return result;
}

}
