// Skeleton / model matrix updates in 0x8017xxxx: scaled local matrix times the camera, and the
// per-bone parent-chain matrix builders.
#include "types.h"

extern "C" {
void  fn_80155724(u8* m);                          // 0x80155724
void fn_801561C0(f32 scale);                           // 0x801561C0
u8* fn_801704F8(u8* obj, f32* scale);                     // 0x801704F8
void  fn_80173438(u8* res, s32 type, s32 flag, u8* p);  // 0x80173438
void* fn_80183FCC(u8* p, s32 idx);                 // 0x80183FCC
extern u8 lbl_80400FF0[];                          // 0x80400FF0 (renderer matrix block)
extern u8 lbl_804129E8[];                          // 0x804129E8 (counters)
extern u8 lbl_80412B20[];                          // 0x80412B20 (record pool, float range at +0x54/+0x58)
extern void* lbl_805FB5F8;                         // 0x805FB5F8 (current camera)
}

// out = L * P for 4x3 row-vector matrices (3 rotation rows then translation). Operands are
// loaded first, so out may alias L or P.
static void Mat43Mul(f32* out, const f32* L, const f32* P) {
    f32 l[12], p[12];
    for (int i = 0; i < 12; i++) { l[i] = L[i]; p[i] = P[i]; }
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            out[r * 3 + c] = p[6 + c] * l[r * 3 + 2] + (p[c] * l[r * 3] + p[3 + c] * l[r * 3 + 1]);
        }
    }
    for (int c = 0; c < 3; c++) {
        out[9 + c] = p[9 + c] + (p[6 + c] * l[11] + (p[c] * l[9] + p[3 + c] * l[10]));
    }
}

// Local matrix of a bone: rotation from the quaternion at n+0xC, translation from n+0x0.
static void BoneLocalMatrix(f32* L, const f32* n) {
    f32 q0 = n[3], q1 = n[4], q2 = n[5], q3 = n[6];
    f32 a = 2.0f * q0, b = 2.0f * q1, c = 2.0f * q2;
    f32 b11 = b * q1, c22 = c * q2, c3 = c * q3, b0 = b * q0, b3 = b * q3;
    f32 c0 = c * q0, a00 = a * q0, a3 = a * q3, c1 = c * q1;
    L[0] = 1.0f - (b11 + c22);
    L[1] = b0 + c3;
    L[2] = c0 - b3;
    L[3] = b0 - c3;
    L[4] = 1.0f - (a00 + c22);
    L[5] = c1 + a3;
    L[6] = c0 + b3;
    L[7] = c1 - a3;
    L[8] = 1.0f - (a00 + b11);
    L[9] = n[0];
    L[10] = n[1];
    L[11] = n[2];
}

// Fills a pool record (0x24 bytes) from the draw job fields.
static void FillRecord(u8* r, f32 t, u8* res, u8* ent, u8* job, void* mtx, s32 flag, f32 sc, s32 a1, s32 a2) {
    *(u8**)(r + 0x8) = res;
    *(u8**)(r + 0x4) = ent;
    *(f32*)(r + 0x0) = t;
    *(u8**)(r + 0xC) = job;
    *(void**)(r + 0x10) = mtx;
    *(s32*)(r + 0x14) = flag;
    *(f32*)(r + 0x18) = sc;
    *(s32*)(r + 0x1C) = a1;
    *(s32*)(r + 0x20) = a2;
}

extern "C" {

// 0x80170C28
// Builds the scaled model matrix (times the camera if flagged), loads it, and dispatches the
// draw entries of arg1.
void fn_80170C28(u8* arg0, u8* arg1, s32 arg2) {
    f32 scale = *(f32*)(arg1 + 0x54);
    u8* var_r31;
    if (*(s32*)(arg0 + 0xB0) != 0) {
        f32 sp8 = scale;
        var_r31 = fn_801704F8(arg0, &sp8);
        fn_80155724(*(u8**)(arg0 + 0x4));
        *(f32*)(lbl_80400FF0 + 0x0) = *(f32*)(arg0 + 0x24);
        *(f32*)(lbl_80400FF0 + 0x4) = *(f32*)(arg0 + 0x28);
        *(f32*)(lbl_80400FF0 + 0x8) = *(f32*)(arg0 + 0x2C);
    } else {
        u8* src = *(u8**)(arg0 + 0x4);
        var_r31 = NULL;
        if (src != NULL) {
            f32 m[12], sp[12];
            s32 flags = *(s32*)(arg0 + 0xC);
            for (int i = 0; i < 9; i++) m[i] = *(f32*)(src + i * 4) * scale;
            m[9] = m[10] = m[11] = 0.0f;
            for (int i = 0; i < 12; i++) sp[i] = m[i];
            if (flags & 2) {
                Mat43Mul(sp, m, (f32*)((u8*)lbl_805FB5F8 + 0x60));
            } else if (flags & 1) {
                Mat43Mul(sp, m, (f32*)((u8*)lbl_805FB5F8 + 0x30));
            }
            sp[9] = *(f32*)(src + 0x24);
            sp[10] = *(f32*)(src + 0x28);
            sp[11] = *(f32*)(src + 0x2C);
            fn_80155724((u8*)sp);
            *(f32*)(lbl_80400FF0 + 0x0) = *(f32*)(arg0 + 0x24);
            *(f32*)(lbl_80400FF0 + 0x4) = *(f32*)(arg0 + 0x28);
            *(f32*)(lbl_80400FF0 + 0x8) = *(f32*)(arg0 + 0x2C);
        } else {
            fn_801561C0(1.0f);  // scale is a leftover f1 in the asm; 1.0 assumed
        }
    }
    fn_80173438(arg1, arg2, (s32)(intptr_t)var_r31, arg0 + 0x3C);  // lint:ok pointer passed as a flag
}

// 0x80171228
// Like fn_80170C28 but writes the scaled/camera matrix into the object's own storage
// (matrix + 0x30) and queues one pool record per draw entry instead of drawing.
void fn_80171228(u8* arg0, s32 arg1, s32 arg2, f32 farg0) {
    u8* res = *(u8**)(arg0 + 0x10);
    if (res == NULL) return;

    u8* var_r31;
    f32 var_f14;
    f32* var_r30;
    if (*(s32*)(arg0 + 0xB0) != 0) {
        f32 sp8 = *(f32*)(res + 0x54);
        var_r30 = *(f32**)(arg0 + 0x4);
        var_r31 = fn_801704F8(arg0, &sp8);
        var_f14 = 1.0f;
    } else {
        u8* src = *(u8**)(arg0 + 0x4);
        var_r31 = NULL;
        if (src != NULL) {
            f32 scale = *(f32*)(res + 0x54);
            s32 flags = *(s32*)(arg0 + 0xC);
            var_r30 = (f32*)(src + 0x30);
            for (int i = 0; i < 9; i++) var_r30[i] = *(f32*)(src + i * 4) * scale;
            if (flags & 2) {
                var_r30[9] = var_r30[10] = var_r30[11] = 0.0f;
                Mat43Mul(var_r30, var_r30, (f32*)((u8*)lbl_805FB5F8 + 0x60));
            } else if (flags & 1) {
                var_r30[9] = var_r30[10] = var_r30[11] = 0.0f;
                Mat43Mul(var_r30, var_r30, (f32*)((u8*)lbl_805FB5F8 + 0x30));
            }
            var_f14 = 1.0f;
            var_r30[9] = *(f32*)(src + 0x24);
            var_r30[10] = *(f32*)(src + 0x28);
            var_r30[11] = *(f32*)(src + 0x2C);
        } else {
            var_f14 = *(f32*)(res + 0x54);
            var_r30 = NULL;
        }
    }

    s32 count = *(s32*)(*(u8**)(arg0 + 0x10) + 0x6C);
    bool notUnit = *(f32*)(arg0 + 0x58) != 1.0f;
    for (s32 i = 0; i < count; i++) {
        u8* ent = *(u8**)(arg0 + 0x10) + i * 8;
        u8* r23 = *(u8**)(ent + 0x70);
        u8* r24 = *(u8**)(ent + 0x74);
        s32 type = *(u8*)(r23 + 0xD);
        if (notUnit || (*(s32*)(r24 + 0xC) & 1)) type = 3;
        u8* rec = (u8*)fn_80183FCC(lbl_80412B20, type);
        if (rec != NULL) FillRecord(rec, farg0, r23, r24, arg0, var_r30, (s32)(intptr_t)var_r31, var_f14, arg1, arg2);  // lint:ok pointer stored as a word
    }
    if (var_r30 == NULL) {
        u8* r23 = *(u8**)(*(u8**)(arg0 + 0x10) + 0x64);
        if (r23 != NULL) {
            f32 d = farg0 - *(f32*)(arg0 + 0x20);
            if (d >= *(f32*)(lbl_80412B20 + 0x54) && d <= *(f32*)(lbl_80412B20 + 0x58)) {
                u8* rec = (u8*)fn_80183FCC(lbl_80412B20, 0);
                if (rec != NULL) FillRecord(rec, farg0, NULL, r23, arg0, NULL, 0, var_f14, 0, 0);
            }
        }
    }
    if (var_r31 != 0) {
        *(s32*)(lbl_804129E8 + 0x10) += 1;
        return;
    }
    *(s32*)(lbl_804129E8 + 0x20) += 1;
}

// 0x80173944
// Walks the bone table: builds each bone's matrix as (local rotation/translation) * (parent
// matrix) into arg0+4, with push/pop of the parent per the bone's op (1 push, 2 pop, 3 set).
void fn_80173944(u8* arg0) {
    f32 root[12];
    f32* stack[65];
    f32** stk;
    f32* parent;
    s32 n;
    if (*(u8**)(arg0 + 0x1C) != NULL) {
        const f32* s = *(f32**)(arg0 + 0x1C);
        for (int i = 0; i < 12; i++) root[i] = s[i];
    } else {
        for (int i = 0; i < 12; i++) root[i] = 0.0f;
        root[0] = root[4] = root[8] = 1.0f;
    }
    parent = root;
    stack[0] = parent;
    stk = stack + 1;
    n = *(s32*)(arg0 + 0x8);
    for (s32 i = 0; i < n; i++) {
        u8* anim = *(u8**)(arg0 + 0x14);
        u8* node = anim + *(s32*)(anim + 0x8) + i * *(s32*)(anim + 0x18);
        f32 L[12];
        f32* out = (f32*)(*(u8**)(arg0 + 0x4) + i * 0x30);
        BoneLocalMatrix(L, (const f32*)node);
        Mat43Mul(out, L, parent);
        switch (*(s32*)(*(u8**)(arg0 + 0xC) + i * 8 + 4)) {
        case 1:
            *stk++ = parent;
            parent = out;
            break;
        case 2:
            parent = *--stk;
            break;
        case 3:
            parent = out;
            break;
        default:
            break;
        }
    }
}

// 0x80173EB0
// Same bone walk as fn_80173944, but bone arg1 uses the supplied matrix arg2 as its local
// matrix, and only bones from arg1 on are written. Stops early after bone arg1 unless its op
// has bit 0 set, or when a pop would go below the mark set at bone arg1.
void fn_80173EB0(u8* arg0, s32 arg1, u8* arg2) {
    f32 root[12];
    f32* stack[65];
    f32** stk;
    f32** mark = NULL;
    f32* parent;
    s32 n;
    if (*(u8**)(arg0 + 0x1C) != NULL) {
        const f32* s = *(f32**)(arg0 + 0x1C);
        for (int i = 0; i < 12; i++) root[i] = s[i];
    } else {
        for (int i = 0; i < 12; i++) root[i] = 0.0f;
        root[0] = root[4] = root[8] = 1.0f;
    }
    parent = root;
    stack[0] = parent;
    stk = stack + 1;
    n = *(s32*)(arg0 + 0x8);
    for (s32 i = 0; i < n; i++) {
        f32 L[12];
        f32* out = (f32*)(*(u8**)(arg0 + 0x4) + i * 0x30);
        s32 op = *(s32*)(*(u8**)(arg0 + 0xC) + i * 8 + 4);
        if (i == arg1) {
            for (int k = 0; k < 12; k++) L[k] = ((const f32*)arg2)[k];
        } else {
            u8* anim = *(u8**)(arg0 + 0x14);
            u8* node = anim + *(s32*)(anim + 0x8) + i * *(s32*)(anim + 0x18);
            BoneLocalMatrix(L, (const f32*)node);
        }
        if (i >= arg1) {
            Mat43Mul(out, L, parent);
            if (i == arg1) {
                if (!(op & 1)) return;
                mark = stk;
            }
        }
        switch (op) {
        case 1:
            *stk++ = parent;
            parent = out;
            break;
        case 2:
            if (stk == mark) return;
            parent = *--stk;
            break;
        case 3:
            parent = out;
            break;
        default:
            break;
        }
    }
}

}  // extern "C"
