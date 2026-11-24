// Misc functions (batch pz5c): sound-effect/anim helpers, bounding box, text markup, camera/music setup.
#include <math.h>
#include <string.h>
#include "types.h"

struct ScriptFunc { u32 type; void* fn; u32 hash; };   // 0xC bytes (as in script_dispatch.cpp)
extern ScriptFunc gScriptFuncs[];                       // 0x80484230

extern "C" {
void   GameLog(int level, int category, const char* fmt, ...);           // 0x8000B2F0
void   UIEvent_SendToAllScreens(u32 a0);
void*  MemAllocDefault(u32 size);                                         // 0x801D4650
void   BufferFree(void* p);                                               // 0x801D4980
s32    fn_801F5294(s32 idx);                                              // 0x801F5294
void   fn_801F4DFC(u8* entry);                                            // 0x801F4DFC
void   fn_801DFAC0(u8* a3, s32 a4, s32 a5);                               // 0x801DFAC0 (r6-r8 are ignored by the callee)
void   fn_801DFB58(u8* cmd, s32 mode);                                    // 0x801DFB58
void   fn_801DFE7C(s32 a);                                                // 0x801DFE7C (tail-jumps through a hook pointer)
void   fn_801EA3C8(u8* entry, f32 v);                                     // 0x801EA3C8
s32    fn_801F0448(char* text, const char* pattern, const char* replacement);  // 0x801F0448
void   fn_8016A3F8(u8* p);                                                // 0x8016A3F8
void   fn_8016A43C(u8* p);                                                // 0x8016A43C
void   fn_8016A64C(u8* p, f32 dt);                                        // 0x8016A64C
void   fn_8016A788(u8* p);                                                // 0x8016A788
s32    fn_8016AFE8(u8* a, u8* b, u8* c, f32 t);                           // 0x8016AFE8 (tail-calls fn_8016ACB8)
void   fn_80173944(u8* obj);                                              // 0x80173944
u8*    fn_80196C9C(void* p);                                              // 0x80196C9C
s32    fn_801A1238(u8* obj, f32* colors);                                 // 0x801A1238
void   fn_801FCD9C(void* data, u8* ctx);                                               // 0x801FCD9C (callback)
void   fn_801FCDCC(void* data, u8* ctx);                                               // 0x801FCDCC (callback)
void   fn_801FD730(u8* audio);                                            // 0x801FD730
s32    fn_801FD2D4(u8* audio, const char* name, s32 flag);                // 0x801FD2D4
s32    fn_801FD420(u8* audio, s32 a, s32 b);                              // 0x801FD420
void   fn_801FFC04(u8* audio, f32 v);                                     // 0x801FFC04
u8*    fn_80208CE8(void);                                                 // 0x80208CE8
void   AXRegisterAuxACallback(void* cb, void* p);                                    // 0x80258904
void   AXRegisterAuxBCallback(void* cb, void* p);                                    // 0x80258968
void   fn_8025D5A4(u8* p);                                                // 0x8025D5A4
void   fn_8025D724(u8* p);                                                // 0x8025D724
void   fn_8025DE60(u8* p);                                                // 0x8025DE60
void   fn_8025DF84(u8* p);                                                // 0x8025DF84
void   fn_8025E640(u8* p);                                                // 0x8025E640
void   fn_8025E768(u8* p);                                                // 0x8025E768
void   fn_8025F0CC(u8* p);                                                // 0x8025F0CC
void   fn_8025F1F4(u8* p);                                                // 0x8025F1F4

extern u8   gAudio[];                // audio manager object
extern u8   lbl_804DD0C8[];          // 0x804DD0C8 (s32 table of script function indices)
extern u8   lbl_804FDAA0[];          // 0x804FDAA0
extern u8   lbl_804FB5A8[];          // 0x804FB5A8
extern u8   lbl_804FB5C4[];          // 0x804FB5C4
extern char lbl_80559CF8[];          // 0x80559CF8 (0x80-byte music name buffer)
extern char lbl_803875D0[];          // 0x803875D0 (log strings)
extern s32  lbl_80386064[];          // 0x80386064
extern void* lbl_805FB658;           // 0x805FB658
extern s32  lbl_805FB8E0;            // 0x805FB8E0
extern void* lbl_805FB968;           // 0x805FB968
extern u32  lbl_805FB9B4;            // 0x805FB9B4
extern u32  lbl_805FB9BC;            // 0x805FB9BC
extern u32  lbl_805FB9C4;            // 0x805FB9C4
extern u32  lbl_805FB9CC;            // 0x805FB9CC
extern s32  lbl_805F9804;            // 0x805F9804
extern s32  lbl_805F9808;            // 0x805F9808
extern f32  lbl_805F9970;            // 0x805F9970
extern f32  lbl_805F9974;            // 0x805F9974
extern f32  lbl_805F9978;            // 0x805F9978
extern f32  lbl_805F997C;            // 0x805F997C
extern u8   lbl_805FBA9C;            // 0x805FBA9C (platform/mode, 2 = alternate)
extern u8   lbl_805FBBA8;            // 0x805FBBA8
extern u8   lbl_805FBBB3;            // 0x805FBBB3 (music loaded flag)
}

typedef void (*ScriptCbV)(void*);
typedef s32 (*ScriptCbP)(void*);

extern "C" {

// 0x801FB254
// D-pad direction bits (0xC003) nudge a 2D stick vector by 1/28.4 (cardinal) or by the diagonal
// step (0.7071 of it); the result is clamped to [-1, 1]. Returns 0 if no direction is held.
s32 fn_801FB254(u32 dirs, f32* v) {
    const f32 kStep = 0.03515625f;
    const f32 kDiag = 0.024858983f;
    switch (dirs & 0xC003) {
    case 0x1:    v[1] = v[1] - kStep; break;
    case 0x2:    v[0] = v[0] - kStep; break;
    case 0x4000: v[1] = v[1] + kStep; break;
    case 0x8000: v[0] = v[0] + kStep; break;
    case 0x3:    v[1] = v[1] - kDiag; v[0] = v[0] - kDiag; break;
    case 0x8001: v[1] = v[1] - kDiag; v[0] = v[0] + kDiag; break;
    case 0x4002: v[1] = v[1] + kDiag; v[0] = v[0] - kDiag; break;
    case 0xC000: v[1] = v[1] + kDiag; v[0] = v[0] + kDiag; break;
    default: return 0;
    }
    f32 x = v[0];
    f32 cx = 1.0f;
    if (!(x > 1.0f)) { cx = -1.0f; if (!(x < -1.0f)) cx = x; }
    f32 y = v[1];
    f32 cy = 1.0f;
    v[0] = cx;
    if (!(y > 1.0f)) { cy = -1.0f; if (!(y < -1.0f)) cy = y; }
    v[1] = cy;
    return 1;
}

// 0x801F020C
// Text markup: replaces the button tags ([XB], [CB], [TB], [SB], [UP], [DN], [LE], [RI], [ST],
// [SE], [HO], [L1], [R1], [NBSP], $nonbreakingspace) with their font glyph characters and the
// literal "\n" with a newline, repeating each replacement until none is left. self is unused.
void fn_801F020C(u8* self, char* line) {
    static const char* const kTags[13] = {
        "[XB]", "[CB]", "[TB]", "[SB]", "[UP]", "[DN]", "[LE]", "[RI]", "[ST]", "[SE]", "[HO]", "[L1]", "[R1]"
    };
    static const char* const kGlyphs[13] = {
        "\xA4", "\xBA", "\x89", "\x7F", "\x86", "\x87", "\xAB", "\xBB", "\xA7", "\xB1", "\xB6", "\xF7", "\xAC"
    };
    for (s32 i = 0; i < 13; i++) {
        while (fn_801F0448(line, kTags[i], kGlyphs[i]) != 0) {}
    }
    while (fn_801F0448(line, "[NBSP]", "\xA0") != 0) {}
    while (fn_801F0448(line, "$nonbreakingspace", "\xA0") != 0) {}
    while (fn_801F0448(line, "\\n", "\n") != 0) {}
}

// 0x801E9EE4
// Sets up an interpolation task: copies up to four vec3 control points (null = zero) into
// +4..+0x30, picks the mode (a, 1 if not 1..2), the script function indices at +0x38/+0x40, the
// rate at +0 (1/t0, or whatever fn_801EA3C8 computes when c is set) and the end rate at +0x70.
void fn_801E9EE4(u8* task, u8** pts, s32 a, s32 b, s32 c, s32 d, s32 e, f32 t0, f32 t1) {
    *(s32*)(task + 0x3c) = 5;
    for (s32 i = 0; i < 4; i++) {
        f32* dst = (f32*)(task + (i < 2 ? 0x4 + i * 0xc : 0x1c + (i - 2) * 0xc));
        f32* src = (f32*)pts[i];
        if (src != NULL) {
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
        } else {
            dst[0] = 0.0f;
            dst[1] = 0.0f;
            dst[2] = 0.0f;
        }
    }
    if (a <= 0 || a >= 3) a = 1;
    u32 type = (b == -1) ? 0 : gScriptFuncs[b].type;
    if (type != 0x90) b = lbl_805FB8E0;
    *(s32*)(task + 0x38) = b;
    *(s32*)(task + 0x34) = a;
    if (c != 0) {
        fn_801EA3C8(task, t0);
    } else {
        if (t0 > -0.0001f && t0 < 0.0001f) t0 = 0.0001f;
        *(f32*)(task + 0x0) = 1.0f / t0;
    }
    s32 idx = *(s32*)(lbl_804DD0C8 + a * 4);
    *(void**)(task + 0x6c) = gScriptFuncs[idx].fn;
    if (e != -1) {
        if (gScriptFuncs[e].type != 0xF) e = -1;
    }
    *(s32*)(task + 0x40) = e;
    *(f32*)(task + 0x70) = t1 / *(f32*)(task + 0x0);
    if (d != 0) *(u32*)(task + 0x3c) |= 2;
}

// 0x801F1A48
// Bounding box of four points (x,y,z triples at +0x1C, +0x28, +0x34, +0x40): writes the center to
// center and the half extent (max - center) to half.
void fn_801F1A48(u8* p, f32* center, f32* half) {
    f32 mn[3], mx[3];
    for (s32 i = 0; i < 3; i++) {
        f32 a = *(f32*)(p + 0x1c + i * 4);
        f32 b = *(f32*)(p + 0x28 + i * 4);
        f32 c = *(f32*)(p + 0x34 + i * 4);
        f32 d = *(f32*)(p + 0x40 + i * 4);
        f32 lo = (a < b) ? a : b;
        lo = (lo < c) ? lo : c;
        lo = (lo < d) ? lo : d;
        f32 hi = (a > b) ? a : b;
        hi = (hi > c) ? hi : c;
        hi = (hi > d) ? hi : d;
        mn[i] = lo;
        mx[i] = hi;
    }
    for (s32 i = 0; i < 3; i++) {
        f32 ctr = (mx[i] + mn[i]) * 0.5f;
        center[i] = ctr;
        half[i] = mx[i] - ctr;
    }
}

// 0x80200F34
// Sets the 3D audio/camera parameters of record slot idx (record picked by lbl_805FBA9C == 2 and
// idx) for preset a (1..5; others leave the values alone), then applies them through the
// platform setup call (a != 0 with the callback, a == 0 with none). p is unused.
void fn_80200F34(void* p, s32 a, s32 idx) {
    u8* r;
    u32 mode = lbl_805FBA9C;
    u8* base = lbl_804FDAA0 + 0x20000;
    if (mode == 2) {
        r = (idx == 0) ? base + 0x1b0 : base + 0x440;
    } else {
        r = base + 0x910;
        if (idx == 0) r = base + 0x6d0;
    }
    f32 va = 0.0f, vb = 0.0f, vc = 0.0f, vd = 0.0f;
    bool set = true;
    switch (a) {
    case 1: va = 80.0f;   vb = 100.0f;  vc = 0.6f; vd = 1.0f; break;
    case 2: va = 40.0f;   vb = 100.0f;  vc = 0.5f; vd = 0.4f; break;
    case 3: va = 1500.0f; vb = 1500.0f; vc = 0.5f; vd = 0.5f; break;
    case 4: va = 0.1f;    vb = 100.0f;  vc = 0.6f; vd = 1.0f; break;
    case 5: va = 0.5f;    vb = 100.0f;  vc = 0.6f; vd = 0.7f; break;
    default: set = false; break;
    }
    if (set) {
        *(f32*)(r + 0x3c) = va;
        *(f32*)(r + 0x38) = vb;
        *(f32*)(r + 0x40) = vc;
        *(f32*)(r + 0x44) = vd;
        *(s32*)(r + 0x48) = 0;
        *(s32*)(r + 0x4c) = 0;
        *(f32*)(r + 0x50) = 1.0f;
        *(f32*)(r + 0x54) = 0.0f;
    }
    if (a != 0) {
        if (mode == 2) fn_8025DE60(r); else fn_8025D5A4(r);
        if (idx == 0) AXRegisterAuxACallback((void*)fn_801FCD9C, r);
        else          AXRegisterAuxBCallback((void*)fn_801FCD9C, r);
    } else {
        if (mode == 2) fn_8025DF84(r); else fn_8025D724(r);
        if (idx == 0) AXRegisterAuxACallback(NULL, NULL);
        else          AXRegisterAuxBCallback(NULL, NULL);
    }
}

// 0x80201190
// Same as fn_80200F34 for the second record set (+0x80..+0x9C of each record): presets 1..9 fill
// six floats; preset 0 and anything above 9 leave the record alone. p is unused.
void fn_80201190(void* p, s32 a, s32 idx) {
    u8* r;
    u32 mode = lbl_805FBA9C;
    u8* base = lbl_804FDAA0 + 0x20000;
    if (mode == 2) {
        r = (idx == 0) ? base + 0x388 : base + 0x618;
    } else {
        r = base + 0xab0;
        if (idx == 0) r = base + 0x870;
    }
    if ((u32)a <= 9 && a != 0) {
        static const f32 kPresets[10][6] = {
            { 0.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
            { 20.0f, 0.6f, 0.1f, 0.0f, 1.0f, 0.0f },
            { 30.0f, 0.3f, 0.5f, 0.1f, 1.0f, 0.0f },
            { 40.0f, 0.1f, 1.5f, 0.0f, 1.0f, 0.0f },
            { 0.8f,  0.9f, 0.5f, 0.9f, 1.0f, 0.0f },
            { 0.3f,  0.9f, 2.0f, 0.8f, 1.0f, 0.0f },
            { 30.0f, 0.1f, 0.5f, 0.7f, 1.0f, 0.0f },
            { 40.0f, 0.8f, 1.0f, 0.8f, 1.0f, 0.0f },
            { 40.0f, 0.8f, 1.5f, 0.0f, 1.0f, 0.0f },
            { 50.0f, 1.0f, 2.0f, 0.3f, 1.0f, 0.0f },
        };
        const f32* k = kPresets[a];
        *(f32*)(r + 0x80) = k[0];
        *(f32*)(r + 0x84) = k[1];
        *(f32*)(r + 0x88) = k[2];
        *(f32*)(r + 0x8c) = k[3];
        *(s32*)(r + 0x90) = 0;
        *(s32*)(r + 0x94) = 0;
        *(f32*)(r + 0x98) = k[4];
        *(f32*)(r + 0x9c) = k[5];
    }
    if (a != 0) {
        if (mode == 2) fn_8025F0CC(r); else fn_8025E640(r);
        if (idx == 0) AXRegisterAuxACallback((void*)fn_801FCDCC, r);
        else          AXRegisterAuxBCallback((void*)fn_801FCDCC, r);
    } else {
        if (mode == 2) fn_8025F1F4(r); else fn_8025E768(r);
        if (idx == 0) AXRegisterAuxACallback(NULL, NULL);
        else          AXRegisterAuxBCallback(NULL, NULL);
    }
}

// 0x801F4E8C
// Processes one script/stream command word: bits 5-11 hold an id; its size (table lookup for ids
// 0x5F-0x7F, else a script function) is added to the running counters lbl_805FB9B4/BC and the
// peak counters B9C4/B9CC. Then ids 0x65/0x66/0x6D-0x70 handle the two stream buffers
// (lbl_804FB5C4 / lbl_804FB5A8: start, append, finish) and any other id goes to the generic hook.
void fn_801F4E8C(u8* p) {
    if (lbl_805F9808 != -1) ((ScriptCbV)gScriptFuncs[lbl_805F9808].fn)(p);
    u32 id = (*(u32*)p >> 20) & 0x7f;
    u32 size = (u32)fn_801F5294((s32)id);
    u32 total = lbl_805FB9B4 + size;
    u32 other = lbl_805FB9BC + size;
    lbl_805FB9B4 = total;
    lbl_805FB9BC = other;
    if (total > lbl_805FB9C4) lbl_805FB9C4 = total;
    if (total > lbl_805FB9CC) lbl_805FB9CC = total;
    switch (id) {
    case 0x70: {   // start buffer: header (id, total size, callbacks) then allocate storage
        s32 w4 = *(s32*)(p + 0x4);
        s32 w8 = *(s32*)(p + 0x8);
        s32 wc = *(s32*)(p + 0xc);
        s32 w10 = *(s32*)(p + 0x10);
        u8* b = lbl_804FB5C4;
        if (*(s32*)b == 0) {
            u8* rec = (u8*)MemAllocDefault(0xc);
            *(u8**)(b + 0xc) = rec;
            *(s32*)(rec + 0x4) = w8;
            *(void**)(rec + 0x8) = MemAllocDefault((u32)w8);
            *(s32*)(*(u8**)(b + 0xc) + 0x0) = w4;
            *(s32*)(b + 0x14) = wc;
            *(s32*)(b + 0x18) = w10;
            *(s32*)(b + 0x8) = 0;
            *(s32*)(b + 0x0) = 1;
            *(s32*)(b + 0x10) = 0;
        }
        break;
    }
    case 0x6F: {   // append a chunk of w4 bytes; flag completion when the buffer is full
        s32 len = *(s32*)(p + 0x4);
        u8* b = lbl_804FB5C4;
        if (*(s32*)b != 0) {
            u8* rec = *(u8**)(b + 0xc);
            memcpy(*(u8**)(rec + 0x8) + *(s32*)(b + 0x8), p + 0x8, (size_t)len);
            u32 fill = (u32)*(s32*)(b + 0x8) + (u32)len;
            *(u32*)(b + 0x8) = fill;
            if (fill == *(u32*)(*(u8**)(b + 0xc) + 0x4)) *(s32*)(b + 0x10) = 1;
        }
        break;
    }
    case 0x6D: {   // free the buffers: first one when w4 == 1, second one when w4 == 0
        u8* b = lbl_804FB5C4;
        if (*(s32*)b != 0 && *(s32*)(p + 0x4) == 1) {
            BufferFree(*(void**)(*(u8**)(b + 0xc) + 0x8));
            BufferFree(*(void**)(b + 0xc));
            *(void**)(b + 0xc) = NULL;
            *(s32*)b = 0;
        }
        u8* b2 = lbl_804FB5A8;
        if (*(s32*)b2 != 0 && *(s32*)(p + 0x4) == 0) {
            BufferFree(*(void**)(*(u8**)(b2 + 0xc) + 0x8));
            BufferFree(*(void**)(b2 + 0xc));
            *(void**)(b2 + 0xc) = NULL;
            *(s32*)b2 = 0;
        }
        break;
    }
    case 0x6E: {   // finish the second buffer: run its callback (if any), then free it
        u8* b = lbl_804FB5A8;
        if (*(s32*)b != 0) {
            s32 cb = *(s32*)(b + 0x14);
            if (cb != -1) ((ScriptCbV)gScriptFuncs[cb].fn)(*(void**)(b + 0xc));
            BufferFree(*(void**)(*(u8**)(b + 0xc) + 0x8));
            BufferFree(*(void**)(b + 0xc));
            *(void**)(b + 0xc) = NULL;
            *(s32*)b = 0;
        }
        break;
    }
    case 0x66: {
        u8 cmd[0x18];
        fn_801DFAC0(cmd, *(s32*)(p + 0x4), *(s32*)(p + 0x8));
        fn_801DFB58(cmd, 4);
        break;
    }
    case 0x65:
        fn_801DFE7C(1);
        break;
    case 0x60: case 0x61: case 0x64: case 0x67: case 0x68: case 0x69: case 0x6A:
    case 0x74: case 0x7E:
        break;
    default: {
        s32 r = ((ScriptCbP)gScriptFuncs[lbl_805F9804].fn)(p);
        if (r == 0 && (intptr_t)lbl_805FB968 == 3) fn_801F4DFC(p);
        break;
    }
    }
}

// 0x801A0E10
// Sets the fill box of a UI element: stores the rect corners (x0,y0,x1,y1 from box) at +0x180,
// scales the color at +0x80 by the render object's current tint, converts it to RGBA bytes and
// back (clamped to 0..255), sets all four corner colors and hides the element's child (0x20).
s32 fn_801A0E10(u8* obj, f32* box) {
    f32 x0 = box[0], y0 = box[1], z = box[2], w = box[3];
    *(f32*)(obj + 0x180) = y0;
    *(f32*)(obj + 0x184) = x0;
    *(f32*)(obj + 0x188) = w;
    *(f32*)(obj + 0x18c) = x0;
    *(f32*)(obj + 0x190) = y0;
    *(f32*)(obj + 0x194) = z;
    *(f32*)(obj + 0x198) = w;
    *(f32*)(obj + 0x19c) = z;
    f32* tint = (f32*)fn_80196C9C(lbl_805FB658);
    if (tint != NULL) {
        *(f32*)(obj + 0x270) = *(f32*)(obj + 0x80) * tint[0];
        *(f32*)(obj + 0x274) = *(f32*)(obj + 0x84) * tint[1];
        *(f32*)(obj + 0x278) = *(f32*)(obj + 0x88) * tint[2];
        *(f32*)(obj + 0x27c) = *(f32*)(obj + 0x8c) * tint[3];
    } else {
        *(f32*)(obj + 0x270) = *(f32*)(obj + 0x80);
        *(f32*)(obj + 0x274) = *(f32*)(obj + 0x84);
        *(f32*)(obj + 0x278) = *(f32*)(obj + 0x88);
        *(f32*)(obj + 0x27c) = *(f32*)(obj + 0x8c);
    }
    s32 c[4];
    for (s32 i = 0; i < 4; i++) {
        s32 v = (s32)(255.0f * *(f32*)(obj + 0x270 + i * 4));
        if (v < 0) v = 0;
        else if (v > 0xff) v = 0xff;
        c[i] = v;
    }
    u32 packed = (u32)c[0] | ((u32)c[1] << 8) | ((u32)c[2] << 16) | ((u32)c[3] << 24);
    f32 k = 0.003921569f;   // 1/255
    f32 rgba[4];
    rgba[0] = k * (f32)(s32)(packed & 0xff);
    rgba[1] = k * (f32)(s32)((packed >> 8) & 0xff);
    rgba[2] = k * (f32)(s32)((packed >> 16) & 0xff);
    rgba[3] = k * (f32)(s32)(packed >> 24);
    f32 colors[16];
    for (s32 i = 0; i < 16; i++) colors[i] = rgba[i & 3];
    fn_801A1238(obj, colors);
    typedef void (*Fn)(u8*, s32);
    (*(Fn*)(*(u8**)(obj + 0x138) + 0x30))(obj, 0x20);
    return 0;
}

// 0x8018C508
// Per-frame update of a fading sound/effect object: advances its animation (owner +4), restarts
// it when the object enters its fade phase (+8 = remaining fade time, +0xC fade-out time,
// +0x10 fade-in time) and blends the owner's animation weights with a cosine curve.
// Flag bits at +0x14: 0x10 started, 0x400 anim restarted, 0x20000 finished, 0x40000 fading out.
void fn_8018C508(u8* o, f32 dt) {
    u32 flags = *(u32*)(o + 0x14);
    if (flags & 0x20000) return;
    if (dt != 0.0f) *(u32*)(o + 0x14) = flags & ~0x10u;
    if (!(*(u32*)(o + 0x14) & 0x400)) {
        u8* anim = *(u8**)(*(u8**)(o + 0x4) + 0x14);
        if (anim != NULL) {
            f32 step = dt * *(f32*)(o + 0x1c);
            if (step > 0.0f) {
                fn_8016A64C(anim, step);
                anim = *(u8**)(*(u8**)(o + 0x4) + 0x14);
                if (*(s32*)(anim + 0x1c) != 0) {
                    s32 idx = *(s32*)(o + 0x20);
                    if (idx != -1) {
                        ((ScriptCbV)gScriptFuncs[idx].fn)(anim);
                        if (*(u8**)(o + 0x4) == NULL) return;
                    }
                    u32 fl = *(u32*)(o + 0x14) | 0x10;
                    *(u32*)(o + 0x14) = fl;
                    if (fl & 0x8) {
                        if (!(fl & 0x80000)) {
                            f32 fade = *(f32*)(o + 0xc);
                            *(s32*)(o + 0x20) = -1;
                            if (fade > -0.0001f && fade < 0.0001f) {
                                u8* owner = *(u8**)(o + 0x4);
                                if (owner != NULL) *(s32*)(*(u8**)(owner + 0x14) + 0x1c) = 0;
                                *(s32*)(o + 0x20) = -1;
                                *(u32*)(o + 0x14) |= 0x20000;
                            } else {
                                *(u32*)(o + 0x14) |= 0x40000;
                                *(f32*)(o + 0x8) = *(f32*)(o + 0xc);
                            }
                        }
                        *(u32*)(o + 0x14) |= 0x400;
                        u8* a = *(u8**)(*(u8**)(o + 0x4) + 0x14);
                        f32 prev = *(f32*)(a + 0x24);
                        f32 t = *(f32*)(*(u8**)(a + 0x14) + 0x20);
                        if (t < 0.0f) {
                            u8* q = *(u8**)(a + 0x14);
                            while (t < 0.0f) t += *(f32*)(q + 0x20);
                        }
                        u8* q = *(u8**)(a + 0x14);
                        while (t > *(f32*)(q + 0x20)) t -= *(f32*)(q + 0x20);
                        *(f32*)(a + 0x24) = t;
                        *(s32*)(a + 0x1c) = 0;
                        if (t < prev) fn_8016A3F8(a);
                        fn_8016A43C(a);
                        fn_8016A788(a);
                    }
                    *(u32*)(o + 0x14) |= 0x10;
                    *(s32*)(*(u8**)(*(u8**)(o + 0x4) + 0x14) + 0x1c) = 0;
                }
            }
        }
    }
    if (*(f32*)(o + 0x8) > 0.0f) {
        f32 w;
        if (*(u32*)(o + 0x14) & 0x40000) {
            f32 d = *(f32*)(o + 0xc);
            if (d > -0.0001f && d < 0.0001f) {
                w = 0.0f;
            } else {
                f32 s = *(f32*)(o + 0x8) / d;
                f32 e = 0.5f * (1.0f + (f32)cosf(1.5707964f * (2.0f * s)));
                w = 1.0f * (1.0f - e) + 0.0f * e;
            }
        } else {
            f32 d = *(f32*)(o + 0x10);
            if (d > -0.0001f && d < 0.0001f) {
                w = 1.0f;
            } else {
                f32 s = *(f32*)(o + 0x8) / d;
                f32 e = 0.5f * (1.0f + (f32)cosf(1.5707964f * (2.0f * s)));
                w = 0.0f * (1.0f - e) + 1.0f * e;
            }
        }
        u8* a = *(u8**)(*(u8**)(o + 0x4) + 0x14);
        fn_8016AFE8(a, *(u8**)(o + 0x18), a, w);
        *(f32*)(o + 0x8) = *(f32*)(o + 0x8) - dt;
    } else if (*(u32*)(o + 0x14) & 0x40000) {
        u8* a = *(u8**)(*(u8**)(o + 0x4) + 0x14);
        fn_8016AFE8(a, *(u8**)(o + 0x18), a, 0.0f);
        *(u32*)(o + 0x14) &= ~0x40000u;
        u8* owner = *(u8**)(o + 0x4);
        if (owner != NULL) *(s32*)(*(u8**)(owner + 0x14) + 0x1c) = 0;
        *(s32*)(o + 0x20) = -1;
        *(u32*)(o + 0x14) |= 0x20000;
    }
    fn_80173944(*(u8**)(o + 0x4));
}

// 0x8020AB5C
// Starts music by name: flag == 0 loads one voice from `name`, flag != 0 loads the stereo pair
// `name_L` / `name_R`. If a track is already playing it is only queued (name copied to
// lbl_80559CF8, fade state reset). Returns 1 on success or when queued, 0 on failure.
s32 fn_8020AB5C(const char* name, s32 flag) {
    if (lbl_805F9970 >= 0.0f) {
        f32 v = lbl_805F997C;
        f32 c = 0.0f;
        if (v >= 0.0f) {
            c = 1.0f;
            if (v <= 1.0f) c = v;
        }
        lbl_805F9978 = c;
        fn_801FFC04(gAudio, c);
        lbl_805F9974 = -1.0f;
        lbl_805F9970 = -1.0f;
    }
    fn_801FD730(gAudio);
    lbl_805FBBB3 = 0;
    lbl_805FBBA8 = (u8)flag;
    if (name == NULL || *(s32*)fn_80208CE8() == 0) return 0;
    if (lbl_805FBBB3 != 0) {
        // (The original also has a recursive call here for lbl_805FBBB3 == 0; it is unreachable.)
        if (lbl_805F9974 >= 0.0f) {
            strncpy(lbl_80559CF8, name, 0x7f);
            lbl_80559CF8[0x7f] = 0;
        } else {
            lbl_805F9970 = 1.0f;
            lbl_805F9974 = 1.0f;
            strncpy(lbl_80559CF8, name, 0x7f);
            lbl_80559CF8[0x7f] = 0;
            lbl_805F997C = lbl_805F9978;
        }
        return 1;
    }
    const u32 kMusicLoadedEvent = 0xB83DCF32u;   // hash not resolved by tools/hashes.py
    if (flag == 0) {
        char buf[0x80];
        strncpy(buf, name, 0x7b);
        buf[0x7b] = 0;
        if (fn_801FD2D4(gAudio, buf, 0) == 0) {
            fn_801FD420(gAudio, 0, 0x40);
            GameLog(1, 2, lbl_803875D0 + 0x20, name);
            UIEvent_SendToAllScreens(kMusicLoadedEvent);
            lbl_805FBBB3 = 1;
            lbl_805F9970 = -1.0f;
            lbl_805F9974 = -1.0f;
            return 1;
        }
        GameLog(4, 2, lbl_803875D0 + 0x38, name);
        return 0;
    }
    char buf[0x80];
    strncpy(buf, name, 0x79);
    buf[0x79] = 0;
    strcat(buf, "_L");
    if (fn_801FD2D4(gAudio, buf, 0) != 0) {
        GameLog(4, 2, lbl_803875D0 + 0x38, buf);
        return 0;
    }
    GameLog(1, 2, lbl_803875D0 + 0x58, buf);
    strncpy(buf, name, 0x79);
    buf[0x79] = 0;
    strcat(buf, "_R");
    if (fn_801FD2D4(gAudio, buf, 1) == 0) {
        fn_801FD420(gAudio, 0, 0);
        fn_801FD420(gAudio, 1, 0x7f);
        UIEvent_SendToAllScreens(kMusicLoadedEvent);
        lbl_805FBBB3 = 1;
        lbl_805F9970 = -1.0f;
        lbl_805F9974 = -1.0f;
        return 1;
    }
    GameLog(4, 2, lbl_803875D0 + 0x38, buf);
    return 0;
}

}
