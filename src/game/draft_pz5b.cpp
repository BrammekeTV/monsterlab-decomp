// Misc helpers: render ring push, UI image ctor, interpolate task init, handle table, pad
// connect callback, model loop, file loader, text tag parser (draft_pz5b).
// Functional rewrites checked against the asm; not byte-matching.
#include <string.h>
#include "types.h"
#include "revolution.h"

typedef u8* (*AllocFn)(s32 size, const char* tag);
typedef void (*VFnObjS32)(u8* self, s32 a);
typedef void (*TagFn)(u8* rec, u8* text);

extern "C" {
void  fn_801469FC(u8* file);                                     // 0x801469FC
s32   fn_80146808(u8* file, const char* name, const char* mode); // 0x80146808
s32   fn_80146930(u8* file, void* buf, s32 size);                // 0x80146930
s32   fn_80146A58(u8* file);                                     // 0x80146A58
void  fn_8018430C(u8* a3, u8* a4);                               // 0x8018430C
void* fn_8018D428(s32 size, const char* file, void* tag, s32 line); // 0x8018D428 (alloc)
void  fn_8019B2F4(void* obj);                                    // 0x8019B2F4 (UI element base init)
u8*   fn_80196C9C(u8* a3);                                       // 0x80196C9C
s32   fn_801A1238(u8* obj, f32* colors);                         // 0x801A1238 (4 vertex colours)
s32   fn_801DA780(u8* a3);                                       // 0x801DA780
u8*   fn_801DA788(u8* a3, s32 i);                                // 0x801DA788
s32   fn_80338A2C(const char* a, const char* b, u32 n);          // 0x80338A2C (strncmp)
s32   fn_802737A8(s32 chan, s32 a, void* cb);                    // 0x802737A8
void  fn_80202E24(s32 chan, s32 result);                         // 0x80202E24
s32   InterpolateQuaternionTask_Update_SLERP(u8* task, void* arg); // 0x801EA7F8
s32   InterpolateVector3Task_Update_LERP(u8* task, void* arg);     // 0x801EA134

extern u8     sRendThread[];            // 0x8040AEA8
extern u8     gScriptFuncs[];           // 0x80484230 (0xC-byte entries: type, fn, hash)
extern u8     lbl_80382E2C[];           // 0x80382E2C (vtable)
extern const char lbl_80382E20[];       // 0x80382E20 ("UIImage.cpp")
extern u8     lbl_80385F58[];           // 0x80385F58 (text tag table, 0x10-byte entries)
extern u8     lbl_8051E680[];           // 0x8051E680 (0x28-byte pad records)
extern u8     lbl_804DD0D4[];           // 0x804DD0D4 (interpolate script ids)
extern u8     lbl_805F9438;             // 0x805F9438 (alloc tag)
extern const char lbl_805F96B0[];       // 0x805F96B0 (file mode string)
extern s32    lbl_805F9738;             // 0x805F9738 (text tag table count)
extern s32    lbl_805FB560;             // 0x805FB560 (render ring write index)
extern void*  lbl_805FB658;             // 0x805FB658 (global render object)
extern s32    lbl_805FB8E4;             // 0x805FB8E4
extern s32    lbl_805FBAC8;             // 0x805FBAC8
extern void*  lbl_805FBACC;             // 0x805FBACC
extern s32    lbl_805FBAD0;             // 0x805FBAD0
}

static const f32 kInv255 = 0.003921569f;

// Clamps to 0..255.
static inline s32 ClampByte(s32 v) {
    if (v < 0) return 0;
    if (v > 0xFF) return 0xFF;
    return v;
}

// Expands a packed colour (byte0 = low byte) into 4 identical vertex colours of 4 floats each.
static void ExpandColor(f32* out, u32 c) {
    f32 r = kInv255 * (f32)(u8)c;
    f32 g = kInv255 * (f32)(u8)(c >> 8);
    f32 b = kInv255 * (f32)(u8)(c >> 16);
    f32 a = kInv255 * (f32)(c >> 24);
    for (int i = 0; i < 4; i++) {
        out[i * 4 + 0] = r;
        out[i * 4 + 1] = g;
        out[i * 4 + 2] = b;
        out[i * 4 + 3] = a;
    }
}

extern "C" {

// 0x8019FA78
// Allocates and builds a UIImage (0x294 bytes) at (x, y) with a packed colour; applies the
// colour (scaled by the global render object's colour, if any) to all 4 vertices.
u8* fn_8019FA78(s32 x, s32 y, u32 color) {
    f32 col[16];
    u8* o = (u8*)fn_8018D428(0x294, lbl_80382E20, &lbl_805F9438, 0x2A);
    if (o != NULL) {
        *(u8**)(o + 0x138) = lbl_80382E2C;
    }
    fn_8019B2F4(o);
    *(s32*)(o + 0x60) = 2;
    *(s32*)(o + 0x13C) = 0;
    *(f32*)(o + 0x148) = (f32)x;
    *(f32*)(o + 0x14C) = (f32)y;
    // 3x4 identity-like block at +0x240
    *(f32*)(o + 0x240) = 1.0f;
    *(f32*)(o + 0x244) = 0.0f;
    *(f32*)(o + 0x248) = 0.0f;
    *(f32*)(o + 0x24C) = 0.0f;
    *(f32*)(o + 0x250) = 1.0f;
    *(f32*)(o + 0x254) = 0.0f;
    *(f32*)(o + 0x258) = 0.0f;
    *(f32*)(o + 0x25C) = 0.0f;
    *(f32*)(o + 0x260) = 1.0f;
    *(f32*)(o + 0x264) = 0.0f;
    *(f32*)(o + 0x268) = 0.0f;
    *(f32*)(o + 0x26C) = 0.0f;
    *(f32*)(o + 0x270) = 1.0f;
    *(f32*)(o + 0x274) = 1.0f;
    *(f32*)(o + 0x278) = 1.0f;
    *(f32*)(o + 0x27C) = 1.0f;
    ExpandColor(col, color);
    fn_801A1238(o, col);
    *(f32*)(o + 0x180) = 0.0f;
    *(f32*)(o + 0x184) = 0.0f;
    *(f32*)(o + 0x188) = 1.0f;
    *(f32*)(o + 0x18C) = 0.0f;
    *(f32*)(o + 0x190) = 0.0f;
    *(f32*)(o + 0x194) = 1.0f;
    *(f32*)(o + 0x198) = 1.0f;
    *(f32*)(o + 0x19C) = 1.0f;
    u8* m = fn_80196C9C((u8*)lbl_805FB658);
    if (m != NULL) {
        *(f32*)(o + 0x270) = *(f32*)(o + 0x80) * *(f32*)(m + 0x0);
        *(f32*)(o + 0x274) = *(f32*)(o + 0x84) * *(f32*)(m + 0x4);
        *(f32*)(o + 0x278) = *(f32*)(o + 0x88) * *(f32*)(m + 0x8);
        *(f32*)(o + 0x27C) = *(f32*)(o + 0x8C) * *(f32*)(m + 0xC);
    } else {
        *(f32*)(o + 0x270) = *(f32*)(o + 0x80);
        *(f32*)(o + 0x274) = *(f32*)(o + 0x84);
        *(f32*)(o + 0x278) = *(f32*)(o + 0x88);
        *(f32*)(o + 0x27C) = *(f32*)(o + 0x8C);
    }
    s32 r = ClampByte((s32)(255.0f * *(f32*)(o + 0x270)));
    s32 g = ClampByte((s32)(255.0f * *(f32*)(o + 0x274)));
    s32 b = ClampByte((s32)(255.0f * *(f32*)(o + 0x278)));
    s32 a = ClampByte((s32)(255.0f * *(f32*)(o + 0x27C)));
    u32 packed = (u32)r | ((u32)g << 8) | ((u32)b << 16) | ((u32)a << 24);
    ExpandColor(col, packed);
    fn_801A1238(o, col);
    (*(VFnObjS32*)(*(u8**)(o + 0x138) + 0x30))(o, 0x20);
    *(s32*)(o + 0x144) = 0;
    *(s32*)(o + 0x140) = 0;
    *(s32*)(o + 0x280) = 0;
    *(s32*)(o + 0x284) = 0;
    *(s32*)(o + 0x288) = 0;
    *(f32*)(o + 0x28C) = 0.0f;
    *(f32*)(o + 0x290) = 0.0f;
    return o;
}

// 0x8015E7C8
// Pushes a type-4 record (value, value) into the 0x40-entry render request ring. Only r7
// (value) is an input; r3..r6 are overwritten before use (same shape as fn_8015E6D0).
void fn_8015E7C8(void* unusedR3, s32 unusedR4, s32 unusedR5, s32 unusedR6, s32 value) {
    OSWaitSemaphore((OSSemaphore*)(sRendThread + 0x2618));
    u8* ring = sRendThread + 0x2318;
    s32 idx = lbl_805FB560;
    s32 next = idx + 1;
    lbl_805FB560 = next;
    u8* rec = ring + idx * 0xC;
    *(s32*)rec = 4;
    *(s32*)(rec + 0x4) = value;
    *(s32*)(rec + 0x8) = value;
    if (next >= 0x40) {
        lbl_805FB560 = 0;
    }
    OSSignalSemaphore((OSSemaphore*)(sRendThread + 0x2624));
}

// 0x801EA59C
// Initialises an interpolate task for quaternions (SLERP update callback at +0x6C).
u8* fn_801EA59C(u8* task) {
    *(u16*)(task + 0x78) = 0xFFFF;
    *(u16*)(task + 0x7A) = 0xFFFF;
    *(u16*)(task + 0x7C) = 0xFFFF;
    memset(task, 0, 0x64);
    *(s32*)(task + 0x64) = 0;
    *(s32*)(task + 0x68) = 0;
    *(f32*)(task + 0x70) = 0.0f;
    *(u16*)(task + 0x78) = 0xFFFF;
    *(u16*)(task + 0x7A) = 0xFFFF;
    *(u16*)(task + 0x7C) = 0xFFFF;
    *(s32*)(task + 0x74) = 0;
    *(u8*)(task + 0x7E) = 0;
    *(u8*)(task + 0x7F) = 0;
    *(void**)(task + 0x6C) = (void*)InterpolateQuaternionTask_Update_SLERP;
    return task;
}

// 0x801E9E50
// Initialises an interpolate task for vector3 (LERP update callback at +0x6C), clears +0x3C.
u8* fn_801E9E50(u8* task) {
    *(u16*)(task + 0x78) = 0xFFFF;
    *(u16*)(task + 0x7A) = 0xFFFF;
    *(u16*)(task + 0x7C) = 0xFFFF;
    memset(task, 0, 0x64);
    *(s32*)(task + 0x64) = 0;
    *(s32*)(task + 0x68) = 0;
    *(f32*)(task + 0x70) = 0.0f;
    *(u16*)(task + 0x78) = 0xFFFF;
    *(u16*)(task + 0x7A) = 0xFFFF;
    *(u16*)(task + 0x7C) = 0xFFFF;
    *(s32*)(task + 0x74) = 0;
    *(u8*)(task + 0x7E) = 0;
    *(u8*)(task + 0x7F) = 0;
    *(void**)(task + 0x6C) = (void*)InterpolateVector3Task_Update_LERP;
    *(s32*)(task + 0x3C) = 0;
    return task;
}

// 0x8016056C
// Looks up a handle in a 32-slot table (0x1C-byte entries, slot = handle & 0x1F, entry word 0 must
// equal the handle). For a type-1 entry, copies the 3 floats of v to +0x10 and sets the dirty flag
// +0xC when they differ. Returns the entry or NULL.
u8* fn_8016056C(u8* table, s32 handle, f32* v) {
    u8* e;
    if (handle == 0) {
        e = NULL;
    } else {
        e = table + ((u32)handle & 0x1F) * 0x1C;
        if (*(u32*)e != (u32)handle) {
            e = NULL;
        }
    }
    if (e == NULL) return e;
    if (*(s16*)(e + 0x4) != 1) return e;
    if (*(f32*)(e + 0x10) == v[0] && *(f32*)(e + 0x14) == v[1] && *(f32*)(e + 0x18) == v[2]) {
        return e;
    }
    f32 x = v[0];
    f32 y = v[1];
    f32 z = v[2];
    *(f32*)(e + 0x10) = x;
    *(f32*)(e + 0x14) = y;
    *(f32*)(e + 0x18) = z;
    *(s32*)(e + 0xC) = 1;
    return e;
}

// 0x80202EFC
// Pad connect result callback: result 0 resets the channel record (state 1 -> 2) and requests the
// next step via fn_802737A8; any other result except -1 queues a deferred retry (lbl_805FBAC8..D0).
void fn_80202EFC(s32 chan, s32 result) {
    if (result == 0) {
        u8* rec = lbl_8051E680 + chan * 0x28;
        if (*(s32*)(rec + 0x24) == 1) {
            *(u8*)rec = 1;
            *(s32*)(rec + 0x24) = 2;
            memset(rec + 1, 0, 0x20);
            fn_802737A8(chan, 4, (void*)fn_80202E24);
        }
    } else if (result != -1) {
        lbl_805FBAC8 = 5;
        lbl_805FBACC = (void*)fn_80202EFC;
        lbl_805FBAD0 = 1;
    }
}

// 0x801DA91C
// For a model-like obj (type at +4 == 3) with slot+0x4C set, calls fn_8018430C(slot+0x4C, item)
// for every item 0..fn_801DA780(obj)-1.
void fn_801DA91C(u8* slot, u8* obj) {
    if (*(s32*)(obj + 0x4) == 3 && *(s32*)(slot + 0x4C) != 0 && obj != NULL && fn_801DA780(obj) > 0) {
        for (s32 i = 0; i < fn_801DA780(obj); i++) {
            u8* item = fn_801DA788(obj, i);
            u8* h = *(u8**)(slot + 0x4C);
            if (h != NULL) {
                fn_8018430C(h, item);
            }
        }
    }
}

// 0x801EBEFC
// Loads a whole text file: opens it ("rt"), stores the size in *outSize, allocates size+1 bytes
// through alloc (first word = alloc fn (size, path)), reads, NUL-terminates. Returns the buffer
// or NULL.
u8* fn_801EBEFC(const char* path, s32* outSize, void* alloc) {
    u8 file[0x68];
    u8* buf = NULL;
    if (fn_80146808(file, path, lbl_805F96B0) == 0) {
        return NULL;
    }
    s32 size = fn_80146A58(file);
    *outSize = size;
    if (size != 0) {
        buf = (*(AllocFn*)alloc)(size + 1, path);
        fn_80146930(file, buf, *outSize);
        buf[*outSize] = 0;
    }
    fn_801469FC(file);
    return buf;
}

// 0x801F00DC
// Parses a text line in place: "[Tag...]" sequences matching the tag table (lbl_80385F58, 0x10-byte
// entries: name, id, parse fn, skip) are removed from the text and recorded in the 0xC-byte records
// at table (id, offset into the stripped text, parse fn output); the list ends with id -1.
void fn_801F00DC(u8* self, u8* line, u8* table) {
    u8* rec = table;
    u8* rd = line;
    u8* wr = line;
    if (rec == NULL) return;
    while ((s8)*rd != 0) {
        u8 c = *rd;
        if ((s8)c == 0x5B) {
            u8* ent = lbl_80385F58;
            s32 off = (s32)(wr - line);
            rd += 1;
            for (s32 i = 0; i < lbl_805F9738; i++) {
                const char* name = *(const char**)ent;
                u32 n = (u32)strlen(name);
                if (fn_80338A2C((const char*)rd, name, n) == 0) {
                    rd = rd + n + *(s32*)(ent + 0xC);
                    (*(TagFn*)(ent + 0x8))(rec, rd);
                    *(s32*)(rec + 0x0) = *(s32*)(ent + 0x4);
                    *(s32*)(rec + 0x4) = off;
                    rec += 0xC;
                    while (*rd != 0x5D) {
                        rd += 1;
                    }
                    rd += 1;
                }
                ent += 0x10;
            }
        } else {
            if (rd > wr) {
                *wr = c;
            }
            wr += 1;
            rd += 1;
        }
    }
    *(s32*)(rec + 0x4) = (s32)(wr - line);
    *(s32*)(rec + 0x0) = -1;
    *wr = 0;
}

// 0x801EA62C
// Sets up a 2-keyframe interpolation task: copies 2 pairs of 4-float values (NULL -> 0,0,0,1) to
// +0x0..0x3F, mode (0 or 3 -> 2) at +0x44, 1/t at +0x40, update fn from the script table at +0x6C,
// ids at +0x48 / +0x50 (validated by script type 0x8A / 0xF), and t1/(1/t) at +0x70.
void fn_801EA62C(u8* task, u8** pts, s32 mode, s32 idA, s32 idB, f32 t, f32 t1) {
    u8* dst = task;
    *(s32*)(task + 0x4C) = 5;
    for (s32 k = 0; k < 2; k++) {
        u8* a = pts[k * 2 + 0];
        if (a != NULL) {
            *(f32*)(dst + 0x0) = *(f32*)(a + 0x0);
            *(f32*)(dst + 0x4) = *(f32*)(a + 0x4);
            *(f32*)(dst + 0x8) = *(f32*)(a + 0x8);
            *(f32*)(dst + 0xC) = *(f32*)(a + 0xC);
        } else {
            *(f32*)(dst + 0x0) = 0.0f;
            *(f32*)(dst + 0x4) = 0.0f;
            *(f32*)(dst + 0x8) = 0.0f;
            *(f32*)(dst + 0xC) = 1.0f;
        }
        u8* b = pts[k * 2 + 1];
        if (b != NULL) {
            *(f32*)(dst + 0x10) = *(f32*)(b + 0x0);
            *(f32*)(dst + 0x14) = *(f32*)(b + 0x4);
            *(f32*)(dst + 0x18) = *(f32*)(b + 0x8);
            *(f32*)(dst + 0x1C) = *(f32*)(b + 0xC);
        } else {
            *(f32*)(dst + 0x10) = 0.0f;
            *(f32*)(dst + 0x14) = 0.0f;
            *(f32*)(dst + 0x18) = 0.0f;
            *(f32*)(dst + 0x1C) = 1.0f;
        }
        dst += 0x20;
    }
    if (mode == 0 || mode == 3) {
        mode = 2;
    }
    *(s32*)(task + 0x44) = mode;
    if (t > -0.0001f && t < 0.0001f) {
        t = 0.0001f;
    }
    *(f32*)(task + 0x40) = 1.0f / t;
    s32 fnIdx = *(s32*)(lbl_804DD0D4 + mode * 4);
    *(void**)(task + 0x6C) = *(void**)(gScriptFuncs + fnIdx * 0xC + 0x4);
    u32 typeA = (idA == -1) ? 0u : *(u32*)(gScriptFuncs + idA * 0xC);
    if (typeA != 0x8A) {
        idA = lbl_805FB8E4;
    }
    *(s32*)(task + 0x48) = idA;
    if (idB != -1) {
        u32 typeB = *(u32*)(gScriptFuncs + idB * 0xC);
        if (typeB != 0xF) {
            idB = -1;
        }
    }
    *(s32*)(task + 0x50) = idB;
    *(f32*)(task + 0x70) = t1 / *(f32*)(task + 0x40);
}

}  // extern "C"
