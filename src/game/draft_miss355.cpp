// Vertex-strip builders (spline / linear interpolation of control vertices) and UI element
// position / size setters.
#include "types.h"

typedef void (*ElemVecFn)(void* el, s32 code, const f32* v);

extern "C" {
void*       ScreenSetFindElement(void* set, u32 hash);   // 0x80198F18
void*       ScreenSetsCurrent(void* sets);               // 0x8018D1E4
void        fn_8018D2B8(const char* fmt, ...);           // 0x8018D2B8 (debug log)
void        fn_8018D370(const char* fmt, ...);           // 0x8018D370 (debug log)
const char* fn_801EB708(u32 hash);                       // 0x801EB708 (hash -> name for logs)
extern const char lbl_80381868[];                        // 0x80381868 (string block)
extern f32 lbl_803483C0[];                               // 0x803483C0 (spline weights, 4 per row)
extern f32 lbl_80348650[];                               // 0x80348650 (cubic weights, 4 per row)
extern f32 lbl_803487E0[];                               // 0x803487E0 (quadratic weights, 4 per row, 3 used)
}
extern "C++" void* gScreenSets;                          // 0x805FB654

// Control vertex as stored in the source strip (stride 0x30):
// +0 pos xyz, +0xC packed color, +0x10 uv.
struct CtrlVert {
    f32 pos[3];
    f32 col[4];  // normalised 0..1, component 0 = low byte
    f32 uv[2];
};

static void LoadCtrl(CtrlVert* c, const u8* p) {
    u32 rgba = *(u32*)(p + 0xc);
    c->pos[0] = *(f32*)(p + 0x0);
    c->pos[1] = *(f32*)(p + 0x4);
    c->pos[2] = *(f32*)(p + 0x8);
    c->col[0] = 0.003921569f * (f32)(u8)rgba;
    c->col[1] = 0.003921569f * (f32)(u8)(rgba >> 8);
    c->col[2] = 0.003921569f * (f32)(u8)(rgba >> 16);
    c->col[3] = 0.003921569f * (f32)(rgba >> 24);
    c->uv[0]  = *(f32*)(p + 0x10);
    c->uv[1]  = *(f32*)(p + 0x14);
}

static s32 ClampByte(s32 v) {
    if (v < 0) return 0;
    if (v > 0xff) return 0xff;
    return v;
}

// Blends n control vertices with weights w[0..n-1] (left to right sum) and writes one output
// vertex: +0 uv, +8 packed color (component 0 in the top byte), +0xC pos xyz.
static void EmitVert(u8* out, const CtrlVert* c, const f32* w, s32 n) {
    f32 pos[3], col[4], uv[2];
    s32 i, j;
    for (j = 0; j < 3; j++) pos[j] = c[0].pos[j] * w[0];
    for (j = 0; j < 4; j++) col[j] = c[0].col[j] * w[0];
    for (j = 0; j < 2; j++) uv[j] = c[0].uv[j] * w[0];
    for (i = 1; i < n; i++) {
        for (j = 0; j < 3; j++) pos[j] = pos[j] + c[i].pos[j] * w[i];
        for (j = 0; j < 4; j++) col[j] = col[j] + c[i].col[j] * w[i];
        for (j = 0; j < 2; j++) uv[j] = uv[j] + c[i].uv[j] * w[i];
    }
    s32 c0 = ClampByte((s32)(255.0f * col[0]));
    s32 c1 = ClampByte((s32)(255.0f * col[1]));
    s32 c2 = ClampByte((s32)(255.0f * col[2]));
    s32 c3 = ClampByte((s32)(255.0f * col[3]));
    *(f32*)(out + 0xc)  = pos[0];
    *(f32*)(out + 0x10) = pos[1];
    *(f32*)(out + 0x14) = pos[2];
    *(u32*)(out + 0x8)  = ((u32)c0 << 24) | ((u32)c1 << 16) | ((u32)c2 << 8) | (u32)c3;
    *(f32*)(out + 0x0)  = uv[0];
    *(f32*)(out + 0x4)  = uv[1];
}

extern "C" {

// 0x8014B428
// Open spline through arg2 + 5 control vertices (stride 0x30), 4-point window: 17 output
// vertices for the first segment, 8 for each middle segment, 16 for the last one.
void fn_8014B428(u8* arg0, u8* arg1, s32 arg2) {
    CtrlVert win[4];
    const f32* table = lbl_803483C0;
    s32 i, k;

    for (i = 0; i < 4; i++) LoadCtrl(&win[i], arg1 + i * 0x30);
    for (k = 0; k < 0x11; k++) {
        EmitVert(arg0, win, table + k * 4, 4);
        arg0 += 0x30;
    }

    // slide the window: points 1..4
    win[0] = win[1];
    win[1] = win[2];
    win[2] = win[3];
    LoadCtrl(&win[3], arg1 + 0xc0);

    const u8* next = arg1 + 0xf0;
    for (i = 0; i < arg2; i++) {
        for (k = 0; k < 8; k++) {
            EmitVert(arg0, win, table + 0x84 + k * 4, 4);  // +0x210 bytes
            arg0 += 0x30;
        }
        win[0] = win[1];
        win[1] = win[2];
        win[2] = win[3];
        LoadCtrl(&win[3], next);
        next += 0x30;
    }

    for (k = 0; k < 0x10; k++) {
        EmitVert(arg0, win, table + 0x44 + k * 4, 4);  // +0x110 bytes
        arg0 += 0x30;
    }
}

// 0x8014C8F0
// Cubic curve through 4 control vertices: 25 output vertices.
void fn_8014C8F0(u8* arg0, u8* arg1) {
    CtrlVert c[4];
    s32 i, k;

    for (i = 0; i < 4; i++) LoadCtrl(&c[i], arg1 + i * 0x30);
    for (k = 0; k < 0x19; k++) {
        EmitVert(arg0, c, lbl_80348650 + k * 4, 4);
        arg0 += 0x30;
    }
}

// 0x8014D190
// Quadratic curve through 3 control vertices: 17 output vertices.
void fn_8014D190(u8* arg0, u8* arg1) {
    CtrlVert c[3];
    s32 i, k;

    for (i = 0; i < 3; i++) LoadCtrl(&c[i], arg1 + i * 0x30);
    for (k = 0; k < 0x11; k++) {
        EmitVert(arg0, c, lbl_803487E0 + k * 4, 3);
        arg0 += 0x30;
    }
}

// 0x8014D834
// Copies 2 control vertices into output vertices (color round-tripped through float).
void fn_8014D834(u8* arg0, u8* arg1) {
    static const f32 kOne = 1.0f;
    CtrlVert c;
    s32 i;

    for (i = 0; i < 2; i++) {
        LoadCtrl(&c, arg1 + i * 0x30);
        EmitVert(arg0, &c, &kOne, 1);
        arg0 += 0x30;
    }
}

// 0x8018F840
// Sets the element position (+0x68..) from a vec3 and notifies it via vtable slot 0x2C (code 0x18).
void UIElement_SetPos(u32 a0, f32* a1) {
    f32 v[3];
    u8* el = NULL;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_80381868 + 0x40);
    } else {
        el = (u8*)ScreenSetFindElement(set, a0);
        if (el == NULL) fn_8018D2B8(lbl_80381868 + 0x238, fn_801EB708(a0));
    }
    if (el != NULL) {
        if (a1 == NULL) {
            fn_8018D370(lbl_80381868 + 0x268);
            return;
        }
        f32 x = a1[0], y = a1[1], z = a1[2];
        v[0] = x;
        *(f32*)(el + 0x68) = x;
        *(f32*)(el + 0x6c) = y;
        *(f32*)(el + 0x70) = z;
        v[1] = y;
        v[2] = z;
        (*(ElemVecFn*)(*(u8**)(el + 0x138) + 0x2c))(el, 0x18, v);
    }
}

// 0x8018FC68
// Sets the element size (+0x74..) from a vec3 and notifies it via vtable slot 0x2C (code 0x18).
void UIElement_SetSize(u32 a0, f32* a1) {
    f32 v[3];
    u8* el = NULL;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_80381868 + 0x40);
    } else {
        el = (u8*)ScreenSetFindElement(set, a0);
        if (el == NULL) fn_8018D2B8(lbl_80381868 + 0x238, fn_801EB708(a0));
    }
    if (el != NULL) {
        if (a1 == NULL) {
            fn_8018D370(lbl_80381868 + 0x290);
            return;
        }
        f32 x = a1[0], y = a1[1], z = a1[2];
        v[0] = x;
        *(f32*)(el + 0x74) = x;
        *(f32*)(el + 0x78) = y;
        *(f32*)(el + 0x7c) = z;
        v[1] = y;
        v[2] = z;
        (*(ElemVecFn*)(*(u8**)(el + 0x138) + 0x2c))(el, 0x18, v);
    }
}

}  // extern "C"
