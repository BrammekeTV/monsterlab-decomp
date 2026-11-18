// UI element functions in 0x801Axxxx: screen copy pass, slideshow, model and entity elements.
#include "types.h"
#include "revolution.h"

extern "C" {
#include "game/script_callbacks.h"
}

typedef void (*VFnI)(void*, s32);
typedef void (*VFnF)(void*, f32);
typedef void (*VFnIF)(void*, s32, f32);

extern "C" {
s32    UISetTexture(u8* el, s32 tex);                           // 0x801A11FC
void   fn_801719DC(void* p);                                    // 0x801719DC
u8*    fn_8018C8E4(void);                                       // 0x8018C8E4
void   fn_8018D2B8(const char* fmt, ...);                       // 0x8018D2B8 (debug log)
void*  fn_8018D428(s32 size, const char* file, void* tag, s32 line); // 0x8018D428 (alloc)
void   fn_80195A58(void* gx, void* a, u8* b, u8* c);            // 0x80195A58
void   fn_80196678(void* gx, void* p, f32 t);                   // 0x80196678
s32    fn_80196B10(u8* a3);                                     // 0x80196B10 (leaf)
s32 fn_80196B4C(u8* arg0, const f32* arg1);                   // 0x80196B4C
s32    fn_80196C88(u8* a3);                                     // 0x80196C88 (leaf)
s32    fn_8019AD6C(u8* a3);                                     // 0x8019AD6C (leaf)
u32    fn_8019AC00(void* obj, s32 index);                       // 0x8019AC00
void   fn_8019B2F4(void* obj);                                  // 0x8019B2F4 (UI element base init)
s32 fn_8019B6CC(u8* arg0, f32 farg0);                           // 0x8019B6CC (UI element base update)
s32    fn_8019B7D8(void* obj);                                  // 0x8019B7D8
u8*    fn_8019F9B8(s32 kind);                                   // 0x8019F9B8 (UI element factory)
void*  fn_801B7198(void* anim, s32 a);                          // 0x801B7198
void   fn_801B72A0(void* anim, s32 a);                          // 0x801B72A0
void   fn_801A21CC(u8* o);                                      // 0x801A21CC
void   fn_801A31BC(u8* o);                                      // 0x801A31BC

extern const char lbl_80382E90[];   // 0x80382E90 ("UISlideshow.cpp")
extern u8 lbl_80382E2C[];           // 0x80382E2C (vtable)
extern u8 lbl_80382EA0[];           // 0x80382EA0 (vtable)
extern const char lbl_80382F10[];   // 0x80382F10 ("UIModel.cpp")
extern u8 lbl_80382F1C[];           // 0x80382F1C (vtable)
extern const char lbl_80382F80[];   // 0x80382F80 ("UIEntity.cpp")
extern const char lbl_80382F90[];   // 0x80382F90 (log message)
extern u8 lbl_803830B4[];           // 0x803830B4 (vtable)
extern s32 lbl_805F9450;            // 0x805F9450
extern s32 lbl_805F9460;            // 0x805F9460
extern s32 lbl_805F9478;            // 0x805F9478
extern u8 lbl_805F9212;             // 0x805F9212 (copy filter value)
extern u8 lbl_805FB50C;             // 0x805FB50C (GX shadow flags byte)
extern void* lbl_805FB658;          // 0x805FB658 (global render object)

// 0x801A0D04
// Draws the element's model through fn_80195A58 with GX flag bit 3 cleared, bracketed by BP
// register writes. Returns 0.
s32 fn_801A0D04(u8* arg0) {
    lbl_805FB50C = (u8)(lbl_805FB50C & ~8);
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0xFE00C000u;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0;
    fn_8019B7D8(arg0);
    if (*(s32*)(arg0 + 0x144) != 0) {
        fn_80195A58(lbl_805FB658, *(void**)(arg0 + 0x144), arg0 + 0x240, arg0 + 0x1E0);
    } else {
        u8** rec = *(u8***)(arg0 + 0x140);
        if (rec != NULL) {
            fn_80195A58(lbl_805FB658, *rec, arg0 + 0x240, arg0 + 0x1E0);
        } else {
            fn_80195A58(lbl_805FB658, NULL, arg0 + 0x240, arg0 + 0x1E0);
        }
    }
    lbl_805FB50C |= 8;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0xFE00C000u;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = (u32)lbl_805F9212 << 14;
    return 0;
}

// 0x801A1B64
// UISlideshow constructor: two child elements (kind 0) at +0x164/+0x168, crossfade timing
// 3.0s hold / 0.5s fade, first image shown on child 1.
void* fn_801A1B64(void) {
    u8* o = (u8*)fn_8018D428(0x16C, lbl_80382E90, &lbl_805F9450, 0x2F);
    if (o != NULL) {
        *(void**)(o + 0x138) = lbl_80382EA0;
        u8* c = *(u8**)(o + 0x164);
        if (c != NULL) {
            *(void**)(c + 0x138) = lbl_80382E2C;
        }
        c = *(u8**)(o + 0x168);
        if (c != NULL) {
            *(void**)(c + 0x138) = lbl_80382E2C;
        }
    }
    fn_8019B2F4(o);
    *(s32*)(o + 0x60) = 2;
    *(s32*)(o + 0x13C) = 4;
    *(void**)(o + 0x140) = NULL;
    *(s32*)(o + 0x144) = 0;
    *(f32*)(o + 0x154) = 3.0f;
    *(f32*)(o + 0x158) = 0.5f;
    *(s32*)(o + 0x14C) = 0;
    *(s32*)(o + 0x150) = 1;
    for (s32 i = 0; i < 2; i++) {
        u8* ch = fn_8019F9B8(0);
        *(u8**)(o + 0x164 + i * 4) = ch;
        *(f32*)(ch + 0x68) = 0.0f;
        *(f32*)(ch + 0x6C) = 0.0f;
        *(f32*)(ch + 0x70) = (f32)i;
        ch = *(u8**)(o + 0x164 + i * 4);
        *(f32*)(ch + 0x74) = 1.0f;
        *(f32*)(ch + 0x78) = 1.0f;
        *(f32*)(ch + 0x7C) = 1.0f;
        ch = *(u8**)(o + 0x164 + i * 4);
        (*(VFnI*)(*(u8**)(ch + 0x138) + 0x2C))(ch, 8);
    }
    *(s32*)(o + 0x148) = 0;
    *(f32*)(o + 0x15C) = 0.0f;
    *(f32*)(o + 0x160) = -1.0f;
    void* list = *(void**)(o + 0x140);
    s32 tex = list != NULL ? (s32)fn_8019AC00(list, *(s32*)(o + 0x148)) : 0;
    UISetTexture(*(u8**)(o + 0x164 + *(s32*)(o + 0x150) * 4), tex);
    *(f32*)(*(u8**)(o + 0x164 + *(s32*)(o + 0x150) * 4) + 0x8C) = 1.0f;
    UISetTexture(*(u8**)(o + 0x164 + *(s32*)(o + 0x14C) * 4), 0);
    *(f32*)(*(u8**)(o + 0x164 + *(s32*)(o + 0x14C) * 4) + 0x8C) = 0.0f;
    for (s32 i = 0; i < 2; i++) {
        u8* ch = *(u8**)(o + 0x164 + i * 4);
        (*(VFnI*)(*(u8**)(ch + 0x138) + 0x2C))(ch, 0x20);
    }
    return o;
}

// 0x801A1DC8
// Slideshow: sets the image list at +0x140; if it changed, restarts at image 0 with the fade
// timer reset and shows the first image on the current child.
void fn_801A1DC8(u8* arg0, void* arg1) {
    if (*(void**)(arg0 + 0x140) == arg1) {
        return;
    }
    *(void**)(arg0 + 0x140) = arg1;
    *(s32*)(arg0 + 0x148) = 0;
    *(f32*)(arg0 + 0x15C) = 0.0f;
    *(f32*)(arg0 + 0x160) = -1.0f;
    s32 tex = arg1 != NULL ? (s32)fn_8019AC00(arg1, 0) : 0;
    UISetTexture(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x150) * 4), tex);
    *(f32*)(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x150) * 4) + 0x8C) = 1.0f;
    UISetTexture(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x14C) * 4), 0);
    *(f32*)(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x14C) * 4) + 0x8C) = 0.0f;
    for (s32 i = 0; i < 2; i++) {
        u8* ch = *(u8**)(arg0 + 0x164 + i * 4);
        (*(VFnI*)(*(u8**)(ch + 0x138) + 0x2C))(ch, 0x20);
    }
}

// 0x801A1F40
// Slideshow update: advances the hold timer (next image after +0x154 seconds), runs the
// crossfade alpha on the two children, then the base update, virtual notifications and the
// children's own updates. Returns 0.
s32 fn_801A1F40(u8* arg0, f32 farg0) {
    s32 flags = 0;
    if (*(void**)(arg0 + 0x140) != NULL && !(*(u32*)(arg0 + 0x64) & 4)) {
        f32 t = *(f32*)(arg0 + 0x15C) + farg0;
        *(f32*)(arg0 + 0x15C) = t;
        if (t >= *(f32*)(arg0 + 0x154)) {
            fn_801A21CC(arg0);
        }
        f32 fade = *(f32*)(arg0 + 0x160);
        if (fade >= 0.0f) {
            fade -= farg0;
            *(f32*)(arg0 + 0x160) = fade;
            if (fade > 0.0f) {
                f32 a = fade / *(f32*)(arg0 + 0x158);
                *(f32*)(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x14C) * 4) + 0x8C) = a;
                if (*(u32*)(arg0 + 0x64) & 0x20000) {
                    *(f32*)(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x150) * 4) + 0x8C) = 1.0f - a;
                }
            } else {
                *(f32*)(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x14C) * 4) + 0x8C) = 0.0f;
                if (*(u32*)(arg0 + 0x64) & 0x20000) {
                    *(f32*)(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x150) * 4) + 0x8C) = 1.0f;
                }
                *(f32*)(arg0 + 0x160) = -1.0f;
            }
            u8* ch = *(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x14C) * 4);
            (*(VFnI*)(*(u8**)(ch + 0x138) + 0x2C))(ch, 0x20);
            if (*(u32*)(arg0 + 0x64) & 0x20000) {
                ch = *(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x150) * 4);
                (*(VFnI*)(*(u8**)(ch + 0x138) + 0x2C))(ch, 0x20);
            }
        }
    }
    fn_8019B6CC(arg0, farg0);
    if (*(u32*)(arg0 + 0x64) & 0x10) {
        flags = 0x10;
        (*(VFnI*)(*(u8**)(arg0 + 0x138) + 0x30))(arg0, 0x10);
    }
    if (*(u32*)(arg0 + 0x64) & 0x20) {
        flags |= 0x20;
        (*(VFnI*)(*(u8**)(arg0 + 0x138) + 0x30))(arg0, 0x20);
    }
    fn_80196678(lbl_805FB658, arg0 + 0xF0, farg0);
    fn_80196B4C((u8*)lbl_805FB658, (const f32*)(arg0 + 0x80));
    for (s32 i = 0; i < 2; i++) {
        u8* ch;
        if (flags != 0) {
            ch = *(u8**)(arg0 + 0x164 + i * 4);
            (*(VFnI*)(*(u8**)(ch + 0x138) + 0x2C))(ch, flags);
        }
        ch = *(u8**)(arg0 + 0x164 + i * 4);
        if (*(u32*)(ch + 0x64) & 0x38) {
            (*(VFnF*)(*(u8**)(ch + 0x138) + 0x18))(ch, farg0);
            (*(VFnI*)(*(u8**)(arg0 + 0x138) + 0x30))(arg0, 0x38);
        }
    }
    fn_80196B10((u8*)lbl_805FB658);
    fn_80196C88((u8*)lbl_805FB658);
    return 0;
}

// 0x801A21CC
// Slideshow: advances to the next image (wrapping at the list size), swaps the front/back
// children, starts the fade timer and shows the new image on the new front child.
void fn_801A21CC(u8* arg0) {
    *(f32*)(arg0 + 0x15C) = 0.0f;
    *(f32*)(arg0 + 0x160) = *(f32*)(arg0 + 0x158);
    u8* list = *(u8**)(arg0 + 0x140);
    if (list == NULL) {
        return;
    }
    *(s32*)(arg0 + 0x148) = *(s32*)(arg0 + 0x148) + 1;
    if (*(s32*)(arg0 + 0x148) >= fn_8019AD6C(list)) {
        *(s32*)(arg0 + 0x148) = 0;
    }
    s32 cur = *(s32*)(arg0 + 0x14C);
    s32 next = 1 - cur;
    *(s32*)(arg0 + 0x150) = cur;
    *(s32*)(arg0 + 0x14C) = next;
    *(f32*)(*(u8**)(arg0 + 0x164 + next * 4) + 0x70) = 0.0f;
    *(f32*)(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x150) * 4) + 0x70) = 1.0f;
    void* l2 = *(void**)(arg0 + 0x140);
    s32 tex = l2 != NULL ? (s32)fn_8019AC00(l2, *(s32*)(arg0 + 0x148)) : 0;
    UISetTexture(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x150) * 4), tex);
    *(f32*)(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x14C) * 4) + 0x8C) = 1.0f;
    if (*(u32*)(arg0 + 0x64) & 0x20000) {
        *(f32*)(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x150) * 4) + 0x8C) = 0.0f;
    } else {
        *(f32*)(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x150) * 4) + 0x8C) = 1.0f;
    }
    for (s32 i = 0; i < 2; i++) {
        u8* ch = *(u8**)(arg0 + 0x164 + i * 4);
        (*(VFnI*)(*(u8**)(ch + 0x138) + 0x2C))(ch, 0x28);
    }
}

// 0x801A23F8
// Slideshow: restarts at image 0 (shows it on the current child, clears the other), then
// runs the element's own slot 0x2C notification with 4.
void fn_801A23F8(u8* arg0) {
    void* list = *(void**)(arg0 + 0x140);
    *(s32*)(arg0 + 0x148) = 0;
    *(f32*)(arg0 + 0x15C) = 0.0f;
    *(f32*)(arg0 + 0x160) = -1.0f;
    s32 tex = list != NULL ? (s32)fn_8019AC00(list, 0) : 0;
    UISetTexture(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x150) * 4), tex);
    *(f32*)(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x150) * 4) + 0x8C) = 1.0f;
    UISetTexture(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x14C) * 4), 0);
    *(f32*)(*(u8**)(arg0 + 0x164 + *(s32*)(arg0 + 0x14C) * 4) + 0x8C) = 0.0f;
    for (s32 i = 0; i < 2; i++) {
        u8* ch = *(u8**)(arg0 + 0x164 + i * 4);
        (*(VFnI*)(*(u8**)(ch + 0x138) + 0x2C))(ch, 0x20);
    }
    (*(VFnI*)(*(u8**)(arg0 + 0x138) + 0x2C))(arg0, 4);
}

// 0x801A2588
// UIModel constructor: identity-style defaults (0/1 float blocks at +0x148..+0x184).
void* fn_801A2588(void) {
    u8* o = (u8*)fn_8018D428(0x188, lbl_80382F10, &lbl_805F9460, 0x26);
    if (o != NULL) {
        *(void**)(o + 0x138) = lbl_80382F1C;
    }
    fn_8019B2F4(o);
    *(s32*)(o + 0x60) = 2;
    *(s32*)(o + 0x13C) = 2;
    *(s32*)(o + 0x140) = 0;
    *(s32*)(o + 0x144) = -1;
    *(f32*)(o + 0x148) = 1.0f;
    *(f32*)(o + 0x14C) = 0.0f;
    *(f32*)(o + 0x150) = 0.0f;
    *(f32*)(o + 0x154) = 0.0f;
    *(f32*)(o + 0x158) = 1.0f;
    *(f32*)(o + 0x15C) = 0.0f;
    *(f32*)(o + 0x160) = 0.0f;
    *(f32*)(o + 0x164) = 0.0f;
    *(f32*)(o + 0x168) = 1.0f;
    *(f32*)(o + 0x16C) = 0.0f;
    *(f32*)(o + 0x170) = 0.0f;
    *(f32*)(o + 0x174) = 0.0f;
    *(f32*)(o + 0x178) = 1.0f;
    *(f32*)(o + 0x17C) = 1.0f;
    *(f32*)(o + 0x180) = 1.0f;
    *(f32*)(o + 0x184) = 1.0f;
    return o;
}

// 0x801A2D10
// UIModel pre-draw: after the base step, if the model record (+0x140 -> first word) exists,
// copies the colour at +0x178..+0x184 into it (+0x3C..+0x48) and calls fn_801719DC on it.
s32 fn_801A2D10(u8* arg0) {
    fn_8019B7D8(arg0);
    u8** rec = *(u8***)(arg0 + 0x140);
    if (rec != NULL) {
        u8* model = *rec;
        if (model != NULL) {
            f32 r = *(f32*)(arg0 + 0x178);
            f32 g = *(f32*)(arg0 + 0x17C);
            f32 b = *(f32*)(arg0 + 0x180);
            f32 a = *(f32*)(arg0 + 0x184);
            *(f32*)(model + 0x3C) = r;
            *(f32*)(model + 0x40) = g;
            *(f32*)(model + 0x44) = b;
            *(f32*)(model + 0x48) = a;
            fn_801719DC(**(u8***)(arg0 + 0x140));
        }
    }
    return 0;
}

// 0x801A2DDC
// UIEntity constructor: flag 1 at +0x13C, anim handle at +0x140 initialised to 0.
void* fn_801A2DDC(void) {
    u8* o = (u8*)fn_8018D428(0x148, lbl_80382F80, &lbl_805F9478, 0x24);
    if (o != NULL) {
        *(void**)(o + 0x138) = lbl_803830B4;
    }
    fn_8019B2F4(o);
    *(s32*)(o + 0x60) = 2;
    *(s32*)(o + 0x13C) = 1;
    fn_801B72A0(o + 0x140, 0);
    *(s32*)(o + 0x144) = 0;
    return o;
}

// 0x801A2E94
// UIEntity update: if the anim handle is empty, logs (when flag 2 is set) and returns;
// otherwise base update, copies the scale (+0x74..+0x7C) into the anim, runs its slot 0x20
// virtual and notifies fn_801A31BC when the global byte is set. Returns 0.
s32 fn_801A2E94(u8* arg0, f32 farg0) {
    if (fn_801B7198(arg0 + 0x140, 0) == NULL) {
        if (*(u32*)(arg0 + 0x64) & 2) {
            fn_8018D2B8(lbl_80382F90);
        }
        return 0;
    }
    fn_8019B6CC(arg0, farg0);
    u8* a = (u8*)fn_801B7198(arg0 + 0x140, 0);
    *(f32*)(a + 0x8)  = *(f32*)(arg0 + 0x74);
    *(f32*)(a + 0xC)  = *(f32*)(arg0 + 0x78);
    *(f32*)(a + 0x10) = *(f32*)(arg0 + 0x7C);
    void* anim = fn_801B7198(arg0 + 0x140, 0);
    (*(VFnIF*)(*(u8**)anim + 0x20))(anim, 0, farg0);
    if (*fn_8018C8E4() != 0) {
        fn_801A31BC(arg0);
    }
    return 0;
}

}  // extern "C"
