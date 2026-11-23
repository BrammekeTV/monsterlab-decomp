#include "types.h"
#include <string.h>

// Callees and globals used by the functions below. Addresses in the comments.
extern "C" {
void     fn_8010FD14(u8* a3);                       // 0x8010FD14 (base constructor)
void     fn_80116F08(u8* self);                         // 0x80116F08
void     fn_80116EF0(u8* p, s32 a);                 // 0x80116EF0
void*    fn_801D40E0(s32 n);                        // 0x801D40E0 (allocator)
u8*      fn_80117090(u8* p);                        // 0x80117090
u8*      fn_80117AE4(u8* p);                        // 0x80117AE4
u8*      fn_801196F4(u8* p);                        // 0x801196F4
u8*      fn_8011A778(u8* p);                        // 0x8011A778
u8*      fn_8011C460(u8* p);                        // 0x8011C460
u8*      fn_8011D5C4(u8* p);                        // 0x8011D5C4
u8*      fn_8011EA20(u8* p);                        // 0x8011EA20
u8*      fn_8011F410(u8* p);                        // 0x8011F410
void*    fn_801B7198(void* anim, s32 a);            // 0x801B7198
s32      fn_8004E300(void* a, s32 b);               // 0x8004E300
void*    TextureLoadShared(const char* name);             // 0x80109330 (texture lookup by name)
s32      LightningTowerGetOnOperatingTable(void);
void     SetMonsterInventoryPos(s32 a3);
int      SysSprintf(char* buf, const char* fmt, ...);
}

extern u8 lbl_805FBBE0[];         // 0x805FBBE0 (save block pointer)
extern u8 lbl_805F8F20[];         // 0x805F8F20 (selected entry pointer, -1 when none)
extern u8 lbl_805FB290[];         // 0x805FB290 (byte flag)
extern const char* lbl_805F8F10;  // 0x805F8F10 (name prefix string pointer)
extern f32 lbl_8034602C[];        // 0x8034602C (3 floats, position offset)
extern u8 lbl_8036D158[];         // 0x8036D158 (vtable)
extern u8 lbl_8036D258[];         // 0x8036D258 (vtable)

struct GameDb;
extern GameDb* gGameDb;           // 0x805FBBC0

extern "C" {

// Sets the animation window: stores the target count at +0x24 and the step at +0x1C
// from the ratio of arg1 to arg2, then refreshes the view.
// 0x80116E54
void fn_80116E54(u8* arg0, s32 arg1, s32 arg2) {
    f32 cur = *(f32*)(arg0 + 0x28);
    *(f32*)(arg0 + 0x24) = cur;
    if (cur > 0.0f && arg2 > 0 && arg2 > arg1) {
        *(s32*)(arg0 + 0x1C) = ((arg2 - 1) / arg1) * arg1;
        *(f32*)(arg0 + 0x24) = cur * ((f32)arg1 / (f32)arg2);
    } else {
        *(s32*)(arg0 + 0x1C) = 0;
    }
    fn_80116F08(arg0);
}

// Advances the selected slot by arg1, wrapping it into [0, +0x34) and, when the
// slot changed, loads its value from the save block table and resets +0x38/+0x3C.
// 0x8011726C
void fn_8011726C(u8* arg0, s32 arg1) {
    s32 old = *(s32*)(arg0 + 0x2C);
    s32 limit = *(s32*)(arg0 + 0x34);
    s32 cur = old + arg1;
    *(s32*)(arg0 + 0x2C) = cur;
    if (cur >= limit) {
        *(s32*)(arg0 + 0x2C) = cur % limit;
    }
    while (*(s32*)(arg0 + 0x2C) < 0) {
        *(s32*)(arg0 + 0x2C) += *(s32*)(arg0 + 0x34);
    }
    s32 idx = *(s32*)(arg0 + 0x2C);
    if (idx != old) {
        *(s32*)(arg0 + 0x20) = *(s32*)(*(u8**)lbl_805FBBE0 + idx * 4 + 0xA30);
        *(s32*)(arg0 + 0x4) = idx + 1;
        if (LightningTowerGetOnOperatingTable() > 0) {
            SetMonsterInventoryPos(*(s32*)(arg0 + 0x2C));
        }
        *(s32*)(arg0 + 0x38) = 0;
        *(s32*)(arg0 + 0x3C) = 0;
    }
}

// Moves the attached object (+0x24) to the position stored at +0x14..+0x1C,
// shifted by the table offset when its state is 0xD, then re-positions its UI node.
// 0x801179C8
void fn_801179C8(u8* arg0) {
    u8* e = *(u8**)(arg0 + 0x24);
    if (e == NULL) return;
    f32 p0 = *(f32*)(e + 0x14);
    f32 p1 = *(f32*)(e + 0x18);
    f32 p2 = *(f32*)(e + 0x1C);
    u8* anim = (u8*)fn_801B7198(e + 0x140, 0);
    if (anim != NULL && fn_8004E300(anim, 1) == 0xD) {
        p0 += lbl_8034602C[0];
        p1 += lbl_8034602C[1];
        p2 += lbl_8034602C[2];
    }
    *(f32*)(e + 0x68) = p0;
    *(f32*)(e + 0x6C) = p1;
    *(f32*)(e + 0x70) = p2;
    (*(void (**)(u8*, s32, f32, f32))(*(u8**)(e + 0x138) + 0x2C))(e, 0x18, p1, p0);
}

// Builds the inventory icon name for table entry arg0 ("%sinventory_icon_sec_%03d") and
// looks it up, falling back to the placeholder icon.
// 0x80118A6C
void* PartFormatSecondaryIconName(s32 arg0) {
    u8* data = *(u8**)((u8*)gGameDb + 0xC);
    u8* e = *(u8**)(data + 0x5328) + arg0 * 0x18;
    char buf[0x80];
    SysSprintf(buf, "%sinventory_icon_sec_%03d", lbl_805F8F10, (s32)*(s8*)(e + 0x12));
    void* found = TextureLoadShared(buf);
    if (found == NULL) {
        SysSprintf(buf, "%sinventory_icon_placeholder", lbl_805F8F10);
        found = TextureLoadShared(buf);
    }
    return found;
}

// Moves the window at +0x30 by 3 * arg1 entries, clamped to the list, updates the scroll
// position at +0x2C to match, then forwards the new window to the child at +0x14AC (tail call).
// 0x801190E8
void fn_801190E8(u8* arg0, s32 arg1) {
    s32 count = *(s32*)(arg0 + 0xD58);
    s32 n = count / 4;
    if (count % 4 > 0) n += 1;
    if (n <= 0) return;
    s32 cur = *(s32*)(arg0 + 0x30);
    s32 next = cur + arg1 * 3;
    if (n > next) {
        *(s32*)(arg0 + 0x30) = (next < 0) ? 0 : next;
    }
    s32 now = *(s32*)(arg0 + 0x30);
    if (now != cur) {
        *(s32*)(arg0 + 0x2C) += (now - cur) * 4;
    }
    fn_80116EF0(arg0 + 0x14AC, now);
}

// Ring-buffer scroll of the list at +0x2C by arg1 steps (modulo +0xD58), then keeps the
// visible window +0x30 in step with the new position (one slot per 4 units).
// 0x8011954C
void fn_8011954C(u8* arg0, s32 arg1) {
    s32 count = *(s32*)(arg0 + 0xD58);
    if (count <= 0) return;
    *(s32*)(arg0 + 0x2C) += arg1;
    while (*(s32*)(arg0 + 0x2C) < 0) {
        *(s32*)(arg0 + 0x2C) += count;
    }
    s32 pos = *(s32*)(arg0 + 0x2C);
    if (pos >= count) {
        pos = pos % count;
        *(s32*)(arg0 + 0x2C) = pos;
    }
    s32 idx = pos / 4;
    s32 cur = *(s32*)(arg0 + 0x30);
    if (idx < cur) {
        *(s32*)(arg0 + 0x30) = idx;
        return;
    }
    if (idx < cur + 3) return;
    while (idx >= *(s32*)(arg0 + 0x30) + 3) {
        *(s32*)(arg0 + 0x30) += 1;
    }
}

// 1 when the selected-entry pointer is not the -1 sentinel.
// 0x80119B78
s32 fn_80119B78(void) {
    return *(s32*)lbl_805F8F20 != -1;
}

// 1 when the byte flag is not 1.
// 0x80119B90
s32 fn_80119B90(void) {
    return *(u8*)lbl_805FB290 != 1;
}

// Constructor: sets the vtable and clears the four child slots, then allocates and
// initialises the child objects (each one only if its allocation succeeded).
// 0x8011A3BC
u8* fn_8011A3BC(u8* arg0) {
    fn_8010FD14(arg0);
    *(u8**)arg0 = lbl_8036D158;
    *(s32*)(arg0 + 0x18) = -1;
    *(u8**)(arg0 + 0x8) = NULL;
    *(u8**)(arg0 + 0xC) = NULL;
    *(u8**)(arg0 + 0x10) = NULL;
    *(u8**)(arg0 + 0x14) = NULL;
    u8* p = (u8*)fn_801D40E0(0x54);
    if (p != NULL) p = fn_80117090(p);
    *(u8**)(arg0 + 0x8) = p;
    p = (u8*)fn_801D40E0(0x14E0);
    if (p != NULL) p = fn_80117AE4(p);
    *(u8**)(arg0 + 0xC) = p;
    p = (u8*)fn_801D40E0(0x1C);
    if (p != NULL) p = fn_801196F4(p);
    *(u8**)(arg0 + 0x10) = p;
    p = (u8*)fn_801D40E0(0x2DE8);
    if (p != NULL) p = fn_8011A778(p);
    *(u8**)(arg0 + 0x14) = p;
    return arg0;
}

// Sets the slot at +0x2CB0 to arg1 steps further, wrapped into 0..5 (with a skip over 3
// in the direction of the step), and derives the displayed index at +4 from it.
// 0x8011BCF8
void fn_8011BCF8(u8* arg0, s32 arg1) {
    s32 v = *(s32*)(arg0 + 0x2CB0) + arg1;
    s32 dir = (arg1 >= 0) ? 1 : -1;
    v = ((v % 6) + 6) % 6;
    if (v == 3) v += dir;
    *(s32*)(arg0 + 0x2CB0) = v;
    if (v == 5) {
        *(s32*)(arg0 + 0x4) = 1;
    } else if (v >= 3) {
        *(s32*)(arg0 + 0x4) = v + 1;
    } else {
        *(s32*)(arg0 + 0x4) = v + 2;
    }
    *(s32*)(arg0 + 0x25A0) = 0;
}

// Moves the window at +0x25A0 by 3 * arg1 entries, clamped to the list, then forwards the
// new position to the child at +0x2CB4 (tail call).
// 0x8011BDA8
void fn_8011BDA8(u8* arg0, s32 arg1) {
    s32 count = *(s32*)(arg0 + 0x2CA8);
    s32 n = count / 4;
    if (count % 4 > 0) n += 1;
    if (n <= 0) return;
    s32 next = *(s32*)(arg0 + 0x25A0) + arg1 * 3;
    if (n > next) {
        *(s32*)(arg0 + 0x25A0) = (next < 0) ? 0 : next;
    }
    fn_80116EF0(arg0 + 0x2CB4, *(s32*)(arg0 + 0x25A0));
}

// Rebuilds the badge list at +0x1088 from the rows at +0x1C (stride 0x1C) whose kind
// matches +0x12F4 (or is 3), then re-lays out the view at +0x1318.
// 0x8011CD18
void fn_8011CD18(u8* arg0) {
    *(s32*)(arg0 + 0x12EC) = 0;
    memset(arg0 + 0x1088, 0, 0x264);
    *(s32*)(arg0 + 0x1084) = 0;
    u8* row = arg0 + 0x1C;
    s32 n = *(s32*)(arg0 + 0x12F0);
    for (s32 i = 0; i < n; ++i, row += 0x1C) {
        s32 kind = *(s32*)(arg0 + 0x12F4);
        if (kind == *(s32*)(row + 0x10) || kind == 3) {
            s32 cnt = *(s32*)(arg0 + 0x1084);
            *(u8**)(arg0 + 0x1088 + cnt * 4) = row;
            *(s32*)(arg0 + 0x1084) = cnt + 1;
        }
    }
    fn_80116E54(arg0 + 0x1318, 3, *(s32*)(arg0 + 0x1084));
    fn_80116EF0(arg0 + 0x1318, *(s32*)(arg0 + 0x12EC));
}

// Constructor: sets the vtable and clears the child slots, then allocates and initialises
// the four child objects (each one only if its allocation succeeded).
// 0x8011D1C0
u8* fn_8011D1C0(u8* arg0) {
    fn_8010FD14(arg0);
    *(u8**)arg0 = lbl_8036D258;
    *(s32*)(arg0 + 0x18) = -1;
    *(u8*)(arg0 + 0x1C) = 0;
    *(u8**)(arg0 + 0x8) = NULL;
    *(u8**)(arg0 + 0xC) = NULL;
    *(u8**)(arg0 + 0x10) = NULL;
    *(u8**)(arg0 + 0x14) = NULL;
    u8* p = (u8*)fn_801D40E0(0x134C);
    if (p != NULL) p = fn_8011C460(p);
    *(u8**)(arg0 + 0x8) = p;
    p = (u8*)fn_801D40E0(0x50);
    if (p != NULL) p = fn_8011D5C4(p);
    *(u8**)(arg0 + 0xC) = p;
    p = (u8*)fn_801D40E0(0x34);
    if (p != NULL) p = fn_8011EA20(p);
    *(u8**)(arg0 + 0x10) = p;
    p = (u8*)fn_801D40E0(0xA68C);
    if (p != NULL) p = fn_8011F410(p);
    *(u8**)(arg0 + 0x14) = p;
    return arg0;
}

// Advances the counter at +0x1C by arg1 steps, wrapped into 0..3, and sets +4 to it + 1.
// 0x8011D69C
void fn_8011D69C(u8* arg0, s32 arg1) {
    *(s32*)(arg0 + 0x1C) += arg1;
    while (*(s32*)(arg0 + 0x1C) < 0) {
        *(s32*)(arg0 + 0x1C) += 4;
    }
    if (*(s32*)(arg0 + 0x1C) >= 4) {
        *(s32*)(arg0 + 0x1C) = *(s32*)(arg0 + 0x1C) % 4;
    }
    *(s32*)(arg0 + 0x4) = *(s32*)(arg0 + 0x1C) + 1;
}

// Returns the float stored at +8 of the table entry after the current one (0 for the last
// entry), converted to an integer.
// 0x8011D728
s32 fn_8011D728(u8* arg0) {
    (void)arg0;
    s32 next = *(s32*)(*(u8**)lbl_805FBBE0 + 0x128) + 1;
    if ((u32)next == 0x32) return 0;
    u8* data = *(u8**)((u8*)gGameDb + 0xC);
    u8* e = *(u8**)(data + 0x63C8) + next * 16;
    return (s32)(u32)*(f32*)(e + 0x8);
}

}  // extern "C"
