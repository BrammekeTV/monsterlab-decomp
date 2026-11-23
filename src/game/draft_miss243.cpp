// Decompiled functions (draft_miss243): fn_80028A84, ...
#include <math.h>
#include "types.h"

extern "C" {
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
u32  RandomU32(void);                                       // 0x80143460
f32  cosf(f32 x);                                           // 0x8033EC98 (MSL)
f32  sqrtf(f32 x);
void fn_800D28B8(u8* p);                                    // 0x800D28B8
void fn_801F9164(u8* p, f32 v);                             // 0x801F9164
void fn_801F6B78(u8* p, f32* v);                            // 0x801F6B78
u8*  fn_801B3308(void);                                     // 0x801B3308
void fn_801B3310(u8* p, u8* q);                             // 0x801B3310
u8*  GetGlobalSettings(void);                               // 0x801D2BF8
void* MemAllocDefault(u32 size);                            // 0x801D4650
void BufferFree(void* p);                                   // 0x801D4980
s32  fn_801C0830(u8* zone, u8* box, u8* pos);               // 0x801C0830
f64  floor(f64 x);                                    // 0x8033ED6C (floor)
void fn_8008CE28(u8* vec, f32 v);                           // 0x8008CE28
void fn_8008D29C(u8* p);                                    // 0x8008D29C
f32  sinf(f32 x);                                           // 0x8033F0A4 (MSL)
f64  fn_8033F20C(f64 x, f64 y);                             // 0x8033F20C (pow)
f32  fn_8033F200(f32 a, f32 b);                             // 0x8033F200
s32  fn_8002BC20(u8* p);                                    // 0x8002BC20
void fn_8002BDD4(u8* p);                                    // 0x8002BDD4
void fn_8002A028(u8* e, f32 x, f32 y);                      // 0x8002A028
void fn_8002A158(u8* e, f32 v);                             // 0x8002A158
void fn_8002A31C(u8* e, u8* a, u8* b, u8* c, u8* d, f32 dt);  // 0x8002A31C
void fn_8002A8D8(u8* e);                                    // 0x8002A8D8
void fn_8002A9E0(u8* e);                                    // 0x8002A9E0
void fn_8002AA64(u8* a, u8* b);                             // 0x8002AA64
s32  fn_8002AA8C(u8* a, u8* b);                             // 0x8002AA8C
u32  StringHash(const char* s);                             // 0x801EB704
int  SysSprintf(char* buf, const char* fmt, ...);
u8*  fn_801B6E74(u8* a3);                                   // 0x801B6E74
extern u8 gWorldState[];                                    // 0x803E6D00
extern const char* lbl_805F8908;                            // 0x805F8908
extern const char* lbl_805F890C;                            // 0x805F890C
extern const char* lbl_805F8910;                            // 0x805F8910
extern const char* lbl_805F8914;                            // 0x805F8914
extern const char* lbl_805F8918;                            // 0x805F8918
extern const char* lbl_805F891C;                            // 0x805F891C
extern const char* lbl_805F8920;                            // 0x805F8920
extern const char* lbl_805F8924;                            // 0x805F8924
extern const char* lbl_805F8928;                            // 0x805F8928
extern const char* lbl_8035ABB4[3];                         // 0x8035ABB4
}
extern "C++" void* gEventMgr;                               // 0x805FB6C8
extern "C++" s32 gZoneResourceSize;                         // 0x805FB94C

extern "C" {

typedef void (*VCallS32)(u8*, s32);
typedef u8* (*VCallPtr)(u8*);

// Finds the 0x4C-byte record whose hash (at +0x2C) matches; the record table is at zone+0xA0,
// its count (byte) at zone+0xA4.
static u8* FindZoneRecord(u8* zone, u32 hash) {
    u8* recs = *(u8**)(zone + 0xA0);
    s32 n = *(u8*)(zone + 0xA4);
    s32 i;
    for (i = 0; i < n; i++) {
        if (hash == *(u32*)(recs + i * 0x4C + 0x2C)) return recs + i * 0x4C;
    }
    return NULL;
}

// Clears bit 1 of the flag word (+0x20) of the object's secondary data after clearing bit 1 of the
// word at +8 of the object returned by its virtual slot 0xC (when non-null).
static void ClearObjFlag2(u8* o) {
    if ((*(VCallPtr*)(*(u8**)o + 0xC))(o)) {
        u8* r = (*(VCallPtr*)(*(u8**)o + 0xC))(o);
        *(u32*)(r + 0x8) = *(u32*)(r + 0x8) & ~2u;
    }
    u8* d = *(u8**)(o + 0x4);
    *(u32*)(d + 0x20) = *(u32*)(d + 0x20) & ~2u;
}

// Computes the vector from the record's position (+0xC..) to the object's position (obj[1]+0..).
static void ZoneDelta(f32* out, u8* rec, u8* pos) {
    f32 ox = *(f32*)(rec + 0xC);
    f32 oy = *(f32*)(rec + 0x10);
    f32 oz = *(f32*)(rec + 0x14);
    out[0] = *(f32*)(pos + 0x0) - ox;
    out[1] = *(f32*)(pos + 0x4) - oy;
    out[2] = *(f32*)(pos + 0x8) - oz;
}

// 0x80028A84
// Per-frame update of the two-sided element manager: advances the scroll offsets, steps the
// 0x128-stride and 0x12C-stride element lists, tracks the progress meter and fires the
// completion/failure events.
void fn_80028A84(u8* p, f32 dt) {
    f32 sp18 = 0.0f;
    f32 sp1C = 0.0f;
    s32 i;

    if (*(u8*)(p + 0xAC) == 0) {
        u8* r4 = *(u8**)(p + 0xA0);
        if (r4 && *(s32*)(p + 0x120) == 0) fn_8002BC20(r4);
    }
    f32 step = dt * *(f32*)(p + 0xD0);
    *(f32*)(p + 0xB8) = *(f32*)(p + 0xB8) + step;
    *(f32*)(p + 0xBC) = *(f32*)(p + 0xBC) + step;

    if (*(f32*)(p + 0x100) >= 0.0f) {
        f32 c = 0.6f;
        f32 f3 = 1.0f - c * c;
        f32 root = 0.0f;
        if (f3 > 0.0f) root = sqrtf(f3);
        f32 t = 12.566371f * (3.0f - *(f32*)(p + 0x100));
        f32 co = cosf(1.5707964f * (0.63661975f * (root * t)));
        f32 pw = (f32)fn_8033F20C(2.0, (f64)(-c * t));
        f32 amp = pw * co;
        u8* obj = *(u8**)(p + 0x104);
        *(f32*)(p + 0x100) = *(f32*)(p + 0x100) - dt;
        sp18 = 22.0f * amp;
        sp1C = 16.0f * amp;
        if (RandomU32() & 1) sp18 = sp18 * -1.0f;
        if (RandomU32() & 1) sp1C = sp1C * -1.0f;
        *(f32*)(obj + 0x68) = sp18 + *(f32*)(p + 0x108);
        *(f32*)(obj + 0x6C) = sp1C + *(f32*)(p + 0x10C);
        u8* o2 = *(u8**)(p + 0x104);
        (*(VCallS32*)(*(u8**)(o2 + 0x138) + 0x2C))(o2, 0x18);
    }

    // first list (stride 0x128)
    for (i = 0; i < *(s32*)(p + 0xC0); i++) {
        u8* shared = *(u8**)(p + 0xA0);
        u8* e = *(u8**)(p + 0xA4) + i * 0x128;
        fn_8002A31C(e, p + 0xB8, shared + 0x40, shared + 0x30, shared + 0x38, dt);
        e = *(u8**)(p + 0xA4) + i * 0x128;
        s32 type = *(s32*)(e + 0x18);
        if (type == 5) {
            s32 cnt = *(s32*)(p + 0xB4) + 1;
            s32 mx = *(s32*)(p + 0xCC);
            *(s32*)(p + 0xB4) = cnt;
            if (cnt < mx) mx = cnt;
            s32 a = *(s32*)(p + 0xEC);
            s32 b = *(s32*)(p + 0xF0);
            *(s32*)(p + 0xB4) = mx;
            a = a + 1;
            *(s32*)(p + 0xEC) = a;
            e = *(u8**)(p + 0xA4) + i * 0x128;
            fn_8002A158(e, 49.0f - (f32)(a + b) / 10.0f);
            fn_8002A8D8(*(u8**)(p + 0xA4) + i * 0x128);
            fn_8002A9E0(*(u8**)(p + 0xA4) + i * 0x128);
        } else if ((u32)(type - 6) <= 1u) {
            if (*(f32*)(p + 0x100) >= 0.0f) {
                f32 x = *(f32*)(e + 0x2C);
                f32 y = *(f32*)(e + 0x30);
                fn_8002A028(e, x + sp18, y + sp1C);
            }
        }
    }

    // second list (stride 0x12C)
    for (i = 0; i < *(s32*)(p + 0xC4); i++) {
        u8* shared = *(u8**)(p + 0xA0);
        u8* e = *(u8**)(p + 0xA8) + i * 0x12C;
        fn_8002A31C(e, p + 0xBC, shared + 0x40, shared + 0x30, shared + 0x38, dt);
        e = *(u8**)(p + 0xA8) + i * 0x12C;
        if (*(s32*)(e + 0x18) == 5) {
            fn_8002AA64(e, NULL);
            s32 b = *(s32*)(p + 0xF0) + 1;
            s32 a = *(s32*)(p + 0xEC);
            *(s32*)(p + 0xF0) = b;
            e = *(u8**)(p + 0xA8) + i * 0x12C;
            fn_8002A158(e, 49.0f - (f32)(a + b) / 10.0f);
            fn_8002A8D8(*(u8**)(p + 0xA8) + i * 0x12C);
            fn_8002A9E0(*(u8**)(p + 0xA8) + i * 0x12C);
        }
        e = *(u8**)(p + 0xA8) + i * 0x12C;
        if (*(s32*)(e + 0x18) == 7) {
            s32 j;
            for (j = 0; j < *(s32*)(p + 0xC0); j++) {
                u8* ej = *(u8**)(p + 0xA4) + j * 0x128;
                u32 tj = (u32)(*(s32*)(ej + 0x18) - 5);
                if (tj <= 1u) {
                    if (fn_8002AA8C(ej, e)) {
                        fn_8002AA64(*(u8**)(p + 0xA4) + j * 0x128, e);
                        s32 n = *(s32*)(p + 0xB4) - 1;
                        *(s32*)(p + 0xB4) = n > 0 ? n : 0;
                    }
                }
            }
        }
        e = *(u8**)(p + 0xA8) + i * 0x12C;
        if ((u32)(*(s32*)(e + 0x18) - 6) <= 1u) {
            if (*(f32*)(p + 0x100) >= 0.0f) {
                f32 x = *(f32*)(e + 0x2C);
                f32 y = *(f32*)(e + 0x30);
                fn_8002A028(e, x + sp18, y + sp1C);
            }
        }
    }

    f32 timer = *(f32*)(p + 0x6C);
    if (timer < 0.0f) {
        EventSend(gEventMgr, *(void**)(p + 0x90), NULL, 0x117741C3u, 0, 0, 1, 0);
    } else if (*(s32*)(p + 0xB4) >= *(s32*)(p + 0xCC)) {
        if (*(u8*)(p + 0xAC) == 0) {
            *(f32*)(p + 0xB0) = 0.0f;
            *(u8*)(p + 0xAC) = 1;
            for (i = 0; i < *(s32*)(p + 0xC4); i++) {
                u8* e = *(u8**)(p + 0xA8) + i * 0x12C;
                if (*(s32*)(e + 0x18) == 7) {
                    f32 cur = *(f32*)(p + 0xB0);
                    f32 v = *(f32*)(e + 0x10);
                    if (!(v > cur)) v = cur;
                    *(f32*)(p + 0xB0) = v;
                }
            }
        } else {
            f32 left = *(f32*)(p + 0xB0);
            if (left <= 0.0f) {
                EventSend(gEventMgr, *(void**)(p + 0x90), NULL, 0xC608C842u, 0, 0, 1, 0);
            } else {
                *(f32*)(p + 0xB0) = left - dt;
            }
        }
    }

    s32 prog = *(s32*)(p + 0xB4);
    s32 total = *(s32*)(p + 0xCC);
    if (prog < 0) prog = 0;
    f32 limit = *(f32*)(p + 0x9C);
    *(s32*)(p + 0xB4) = prog;
    f32 frac = (limit * (f32)prog) / (f32)total;
    if (frac >= limit) frac = limit;
    u8* bar = *(u8**)(p + 0x98);
    *(f32*)(bar + 0x74) = frac;
    bar = *(u8**)(p + 0x98);
    (*(VCallS32*)(*(u8**)(bar + 0x138) + 0x2C))(bar, 0x1A);

    *(f32*)(p + 0xB8) = fn_8033F200(*(f32*)(p + 0xB8), 1.0f);
    *(f32*)(p + 0xBC) = fn_8033F200(*(f32*)(p + 0xBC), 1.0f);
    u8* shared = *(u8**)(p + 0xA0);
    if (*(s32*)(shared + 0x4C) == 4 && *(s32*)(p + 0xEC) == *(s32*)(p + 0x114) &&
        *(s32*)(p + 0xF0) == *(s32*)(p + 0x118)) {
        fn_8002BDD4(shared);
    }
    typedef void (*VF)(u8*);
    (*(VF*)(*(u8**)(p + 0x4) + 0x6C))(p);
}


// 0x800784EC
// Looks up the zone records and objects named by the two name-pairs (and the 3 x 100 numbered
// "slot" names) and stores the pointers plus the object-to-record offset vectors.
void fn_800784EC(u8* p) {
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
    u8* rec;
    s32 i, j, k;
    u32 hA = StringHash(lbl_805F8918);
    u32 hB = StringHash(lbl_805F891C);
    u32 hKey = StringHash(lbl_805F8914);

    rec = FindZoneRecord(zone, hKey);
    if (rec) *(u8**)(p + 0x10CC) = rec;
    for (i = 0; i < *(s32*)(zone + 0x98); i++) {
        u8* obj = *(u8**)(*(u8**)(zone + 0x12C) + i * 4);
        u32 h = *(u32*)(obj + 0x2C);
        fn_801B6E74(obj);
        if (hB == h) *(u8**)(p + 0x10C8) = *(u8**)(*(u8**)(zone + 0x12C) + i * 4);
        if (hA == h) *(u8**)(p + 0x10C4) = *(u8**)(*(u8**)(zone + 0x12C) + i * 4);
    }
    {
        u8* r = *(u8**)(p + 0x10CC);
        u8* o8 = *(u8**)(p + 0x10C8);
        ZoneDelta((f32*)(p + 0x10DC), *(u8**)r, *(u8**)(o8 + 4));
        u8* o4 = *(u8**)(p + 0x10C4);
        if (o4) ZoneDelta((f32*)(p + 0x10D0), *(u8**)r, *(u8**)(o4 + 4));
    }

    hA = StringHash(lbl_805F8924);
    hB = StringHash(lbl_805F8928);
    hKey = StringHash(lbl_805F8920);
    rec = FindZoneRecord(zone, hKey);
    if (rec) *(u8**)(p + 0x10F0) = rec;
    for (i = 0; i < *(s32*)(zone + 0x98); i++) {
        u8* obj = *(u8**)(*(u8**)(zone + 0x12C) + i * 4);
        u32 h = *(u32*)(obj + 0x2C);
        if (hB == h) *(u8**)(p + 0x10EC) = obj;
        if (hA == h) *(u8**)(p + 0x10E8) = *(u8**)(*(u8**)(zone + 0x12C) + i * 4);
    }
    {
        u8* r = *(u8**)(p + 0x10F0);
        u8* o8 = *(u8**)(p + 0x10EC);
        ZoneDelta((f32*)(p + 0x1100), *(u8**)r, *(u8**)(o8 + 4));
        u8* o4 = *(u8**)(p + 0x10E8);
        if (o4) ZoneDelta((f32*)(p + 0x10F4), *(u8**)r, *(u8**)(o4 + 4));
    }

    // three groups of up to 100 numbered slots
    for (k = 0; k < 3; k++) {
        u8* a = p + k * 0x50;          // pointer arrays (+0x764 / +0x854 / +0x944 / +0xFD4)
        u8* cntp = p + k * 4 + 0x1110; // slot counter
        u8* vecA = p + 0xD04 + k * 0xF0;
        u8* vecB = p + 0xA34 + k * 0xF0;
        *(s32*)cntp = 0;
        for (j = 0; j < 100; j++) {
            char bufA[64];
            char bufB[64];
            char bufC[64];
            s32 q1 = j / 10;
            s32 q2 = j % 10;
            SysSprintf(bufA, lbl_805F8908, q1, q2, lbl_8035ABB4[k]);
            SysSprintf(bufB, lbl_805F8910, q1, q2, lbl_8035ABB4[k]);
            SysSprintf(bufC, lbl_805F890C, q1, q2, lbl_8035ABB4[k]);
            u32 hC = StringHash(bufC);
            u32 hB2 = StringHash(bufB);
            u32 hKey2 = StringHash(bufA);
            s32 c = *(s32*)cntp;
            *(u8**)(a + c * 4 + 0x854) = NULL;
            c = *(s32*)cntp;
            *(u8**)(a + c * 4 + 0x764) = NULL;
            *(u8**)(a + c * 4 + 0x944) = NULL;
            rec = FindZoneRecord(zone, hKey2);
            if (rec) *(u8**)(a + *(s32*)cntp * 4 + 0x944) = rec;
            for (i = 0; i < *(s32*)(zone + 0x98); i++) {
                u8* obj = *(u8**)(*(u8**)(zone + 0x12C) + i * 4);
                u32 h = *(u32*)(obj + 0x2C);
                fn_801B6E74(obj);
                if (hB2 == h) {
                    *(u8**)(a + *(s32*)cntp * 4 + 0x854) = *(u8**)(*(u8**)(zone + 0x12C) + i * 4);
                    ClearObjFlag2(*(u8**)(a + *(s32*)cntp * 4 + 0x854));
                }
                if (hC == h) {
                    *(u8**)(a + *(s32*)cntp * 4 + 0x764) = *(u8**)(*(u8**)(zone + 0x12C) + i * 4);
                    ClearObjFlag2(*(u8**)(a + *(s32*)cntp * 4 + 0x764));
                }
            }
            c = *(s32*)cntp;
            if (*(u8**)(a + c * 4 + 0x944) && *(u8**)(a + c * 4 + 0x854) && *(u8**)(a + c * 4 + 0x764)) {
                *(s32*)(a + c * 4 + 0xFD4) = j;
                c = *(s32*)cntp;
                ZoneDelta((f32*)(vecA + c * 0xC), **(u8***)(a + c * 4 + 0x944),
                          *(u8**)(*(u8**)(a + c * 4 + 0x854) + 4));
                c = *(s32*)cntp;
                u8* o = *(u8**)(a + c * 4 + 0x764);
                if (o) {
                    ZoneDelta((f32*)(vecB + c * 0xC), **(u8***)(a + c * 4 + 0x944), *(u8**)(o + 4));
                }
                *(s32*)cntp = *(s32*)cntp + 1;
            }
        }
    }
}


// Affine concatenation of two 4x3 row-major matrices (3 rotation rows + a translation row):
// out = a * b.
static void ConcatMtx43(f32* out, const f32* a, const f32* b) {
    s32 r, c;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 3; c++) {
            f32 v = a[r * 3 + 0] * b[0 * 3 + c] + a[r * 3 + 1] * b[1 * 3 + c] +
                    a[r * 3 + 2] * b[2 * 3 + c];
            if (r == 3) v = v + b[9 + c];
            out[r * 3 + c] = v;
        }
    }
}

// 0x80143CA8
// Builds the rotation matrix Rx * Ry * Rz from three locator rotation values (each is a
// multiple of a quarter turn: angle = pi/2 * value).
void MtxFromLocatorRotation(f32 out[12], const void* rot) {
    const f32* r = (const f32*)rot;
    f32 mx[12] = { 1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 0.0f };
    f32 my[12] = { 1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 0.0f };
    f32 mz[12] = { 1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 0.0f };
    f32 tmp[12];

    f32 a = 1.5707964f * r[0];
    f32 c = cosf(a);
    f32 sn = sinf(a);
    mx[4] = c;
    mx[5] = sn;
    mx[7] = -sn;
    mx[8] = c;

    a = 1.5707964f * r[1];
    c = cosf(a);
    sn = sinf(a);
    my[0] = c;
    my[6] = sn;
    my[2] = -sn;
    my[8] = c;

    a = 1.5707964f * r[2];
    c = cosf(a);
    sn = sinf(a);
    mz[0] = c;
    mz[4] = c;
    mz[1] = sn;
    mz[3] = -sn;

    ConcatMtx43(tmp, my, mz);
    ConcatMtx43(out, mx, tmp);
}


// Slot ring (6 vectors of 12 bytes at +0x188): true when the vector in slot [+0x180] is
// near-orthogonal (dot within +-0.0001) to the vector in slot [+0x184].
static bool RingSlotsOrtho(u8* p) {
    u8* a = p + *(s32*)(p + 0x180) * 0xC;
    u8* b = p + *(s32*)(p + 0x184) * 0xC;
    f32 d = (*(f32*)(b + 0x188) * *(f32*)(a + 0x188)) + (*(f32*)(b + 0x18C) * *(f32*)(a + 0x18C));
    d = (*(f32*)(b + 0x190) * *(f32*)(a + 0x190)) + d;
    return d > -0.0001f && d < 0.0001f;
}

// Advances the slot index [+0x180] (wrapping at 6) up to three times until the slots are
// near-orthogonal, then feeds that slot's vector to the members at +0x150C and +0x2908; when none
// qualifies, falls back to fn_800D28B8.
static void RingAdvanceAndFeed(u8* p) {
    s32 n;
    for (n = 0; n < 3; n++) {
        s32 i = *(s32*)(p + 0x180) + 1;
        *(s32*)(p + 0x180) = i;
        if (i >= 6) *(s32*)(p + 0x180) = 0;
        if (RingSlotsOrtho(p)) {
            u8* slot = p + *(s32*)(p + 0x180) * 0xC;
            f32 v[3];
            v[0] = *(f32*)(slot + 0x188);
            v[1] = *(f32*)(slot + 0x18C);
            v[2] = *(f32*)(slot + 0x190);
            fn_801F6B78(p + 0x150C, v);
            fn_801F6B78(p + 0x2908, v);
            return;
        }
    }
    fn_800D28B8(p);
}

typedef void (*VCallVec)(u8*, f32*);

// 0x800D0DC0
// Resets the member records (flags to 1/0), steps the current-direction index
// [+0x184], finds a new orthogonal slot for the +0x180 index, broadcasts the current direction to
// the members at +0x1464/+0x150C/+0x2860/+0x2908 and re-registers all 23 members in the list.
void fn_800D0DC0(u8* p) {
    f32 v[3];
    u8* cur;

    *(s32*)(p + 0x268) = 1;
    *(s32*)(p + 0x368) = 1;
    *(s32*)(p + 0x41C) = 1;
    *(s32*)(p + 0x4DC) = 1;
    *(s32*)(p + 0x960) = 1;
    *(s32*)(p + 0xDE4) = 1;
    *(s32*)(p + 0x1268) = 1;
    *(s32*)(p + 0x138C) = 1;
    *(s32*)(p + 0x1440) = 1;
    *(s32*)(p + 0x1500) = 1;
    *(s32*)(p + 0x15A8) = 1;
    *(s32*)(p + 0x1664) = 0;
    *(s32*)(p + 0x1764) = 0;
    *(s32*)(p + 0x1818) = 0;
    *(s32*)(p + 0x18D8) = 0;
    *(s32*)(p + 0x1D5C) = 0;
    fn_801F9164(p + 0x1CC4, 0.25f);
    s32 idx = *(s32*)(p + 0x184) + 1;
    *(s32*)(p + 0x21E0) = 0;
    *(s32*)(p + 0x2664) = 0;
    *(s32*)(p + 0x2788) = 0;
    *(s32*)(p + 0x283C) = 0;
    *(s32*)(p + 0x28FC) = 0;
    *(s32*)(p + 0x29A4) = 0;
    *(s32*)(p + 0x184) = idx;
    if (idx >= 6) *(s32*)(p + 0x184) = 0;

    if (!RingSlotsOrtho(p)) RingAdvanceAndFeed(p);

    cur = p + *(s32*)(p + 0x184) * 0xC;
    v[0] = *(f32*)(cur + 0x188);
    v[1] = *(f32*)(cur + 0x18C);
    v[2] = *(f32*)(cur + 0x190);
    (*(VCallVec*)(*(u8**)(p + 0x1464) + 0x10))(p + 0x1464, v);
    (*(VCallVec*)(*(u8**)(p + 0x150C) + 0x10))(p + 0x150C, v);
    (*(VCallVec*)(*(u8**)(p + 0x2860) + 0x10))(p + 0x2860, v);
    (*(VCallVec*)(*(u8**)(p + 0x2908) + 0x10))(p + 0x2908, v);

    RingAdvanceAndFeed(p);

    fn_801B3310(fn_801B3308(), p + 0x1D0);
    fn_801B3310(fn_801B3308(), p + 0x2D0);
    fn_801B3310(fn_801B3308(), p + 0x384);
    fn_801B3310(fn_801B3308(), p + 0x444);
    fn_801B3310(fn_801B3308(), p + 0x8C8);
    fn_801B3310(fn_801B3308(), p + 0xD4C);
    fn_801B3310(fn_801B3308(), p + 0x11D0);
    fn_801B3310(fn_801B3308(), p + 0x12F4);
    fn_801B3310(fn_801B3308(), p + 0x13A8);
    fn_801B3310(fn_801B3308(), p + 0x1464);
    fn_801B3310(fn_801B3308(), p + 0x150C);
    fn_801B3310(fn_801B3308(), p + 0x15CC);
    fn_801B3310(fn_801B3308(), p + 0x16CC);
    fn_801B3310(fn_801B3308(), p + 0x1780);
    fn_801B3310(fn_801B3308(), p + 0x1840);
    fn_801B3310(fn_801B3308(), p + 0x1CC4);
    fn_801B3310(fn_801B3308(), p + 0x2148);
    fn_801B3310(fn_801B3308(), p + 0x25CC);
    fn_801B3310(fn_801B3308(), p + 0x26F0);
    fn_801B3310(fn_801B3308(), p + 0x27A4);
    fn_801B3310(fn_801B3308(), p + 0x2860);
    fn_801B3310(fn_801B3308(), p + 0x2908);
    fn_801B3310(fn_801B3308(), p + 0x29C8);
}


// Sum of the s32 settings at offsets first..last (step 4) of the global settings block.
static s32 SettingsSum(s32 first, s32 last) {
    s32 sum = 0;
    s32 off;
    for (off = first; off <= last; off += 4) sum += *(s32*)(GetGlobalSettings() + off);
    return sum;
}

// 0x80132458
// Returns the zone id of the zone that contains the entity's position (entity->+4 holds the
// position and, at +0x30, a preferred zone id). Tries the preferred zone first (when it is a
// valid world zone or lies in one of the settings-defined id ranges), then falls back to testing
// every zone and picking the one with the smallest squared distance to its origin. -1 if none.
s32 WorldGetEntityZone(void* world, void* entity) {
    u8* ent = (u8*)entity;
    s32 result = -1;

    if (ent == NULL || *(u8**)(ent + 4) == NULL) return -1;
    if (*(u16*)(*(u8**)(ent + 4) + 0x30) == 0) *(u16*)(*(u8**)(ent + 4) + 0x30) = 1;
    {
        u32 id = *(u16*)(*(u8**)(ent + 4) + 0x30);
        if (id == 0xFF) result = (s32)id;
    }
    if (result == -1) {
        s32 z = *(u16*)(*(u8**)(ent + 4) + 0x30);
        u8* zone;
        if (z > 0 && z <= *(s32*)(*(u8**)world + 4)) {
            zone = *(u8**)(*(u8**)world + 0x1A4) + z * gZoneResourceSize;
        } else if ((SettingsSum(0x68, 0x68) + 1 <= z && z <= SettingsSum(0x68, 0x7C)) ||
                   (SettingsSum(0x68, 0x80) + 1 <= z && z <= SettingsSum(0x68, 0x84)) ||
                   (SettingsSum(0x68, 0x84) + 1 <= z && z <= SettingsSum(0x68, 0x88)) ||
                   (SettingsSum(0x68, 0x7C) + 1 <= z && z <= SettingsSum(0x68, 0x80))) {
            zone = *(u8**)(*(u8**)world + 0x1A4) + z * gZoneResourceSize;
        } else {
            zone = NULL;
        }
        if (zone != NULL && fn_801C0830(zone, zone + 0xC, *(u8**)(ent + 4)) != 0) {
            result = *(s32*)(zone + 0x18);
        }
    }

    if (result == -1) {
        s32 capacity = SettingsSum(0x34, 0x64);
        u8** hits = (u8**)MemAllocDefault((u32)capacity * 4);   // fixed list of candidate zones
        s32 count = 0;
        u8 any = 0;
        s32 i;
        for (i = 0; i < SettingsSum(0x34, 0x64); i++) hits[i] = NULL;
        i = 0;
        while (i <= *(s32*)(*(u8**)gWorldState + 4)) {
            u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + i * gZoneResourceSize;
            s32 more = 1;
            if (fn_801C0830(zone, zone + 0xC, *(u8**)(ent + 4)) != 0) {
                u8* pos = *(u8**)(ent + 4);
                f32 dx = *(f32*)(pos + 0) - *(f32*)(zone + 0);
                f32 dy = *(f32*)(pos + 4) - *(f32*)(zone + 4);
                f32 dz = *(f32*)(pos + 8) - *(f32*)(zone + 8);
                hits[count] = zone;
                count = count + 1;
                any = 1;
                *(f32*)(zone + 0x1C) = dz * dz + (dx * dx + dy * dy);
                if (!(count < SettingsSum(0x34, 0x64))) more = 0;
            }
            if (!more) break;
            i++;
        }
        if (any != 0) {
            f32 best = -1.0f;
            s32 bestIdx = -1;
            for (i = 0; i < count; i++) {
                if (best == -1.0f || *(f32*)(hits[i] + 0x1C) < best) {
                    bestIdx = i;
                    best = *(f32*)(hits[i] + 0x1C);
                }
            }
            if (bestIdx != -1) result = *(s32*)(hits[bestIdx] + 0x18);
        }
        if (hits != NULL) BufferFree(hits);
    }
    return result;
}


// Tables at 0x803468D8 / 0x803467F8 / 0x8034685C / 0x80346848 / 0x80346834 / 0x80346958.
static const f32 kTbl803468D8[4] = { 3.0f, 2.0f, 2.0f, 1.0f };
static const f32 kTbl803467F8[5] = { 0.5f, 4.0f, 1.0f, 1.0f, 1.0f };
static const f32 kTbl8034685C[5] = { 0.25f, 0.25f, 0.25f, 0.25f, 0.25f };
static const f32 kTbl80346848[5] = { 0.5f, 1.0f, 1.0f, 1.5f, 1.5f };
static const f32 kTbl80346834[5] = { 0.2f, 1.0f, 0.5f, 1.0f, 1.0f };
static const f32 kTbl80346958[4] = { 15.0f, 5.0f, 7.5f, 8.5f };

// Table index from the state pair (+0x268 kind, +0x270 flag): 4 when the flag is set, else
// kind 2/3/4/0 -> 0/1/2/3 and anything else -> 0.
static s32 StateTableIndex(u8* p) {
    s32 idx = 4;
    if (*(s32*)(p + 0x270) == 0) {
        switch (*(s32*)(p + 0x268)) {
        case 2: idx = 0; break;
        case 3: idx = 1; break;
        case 4: idx = 2; break;
        case 0: idx = 3; break;
        default: idx = 0; break;
        }
    }
    return idx;
}

// 0x8008D7C8
// Per-frame motion step of a bouncing entity (state at +0x26C): moves its position (entity->+4)
// by the current speeds, updates the bounce timers and random re-launch, and sends the
// out-of-range event when it passes z = 25.
void fn_8008D7C8(u8* p, f32 dt) {
    f32 x, y, z;
    f32 vz, vx;
    u8* pos;

    *(s32*)(p + 0x264) = 3;
    pos = *(u8**)(p + 0x4);
    x = *(f32*)(pos + 0x0);
    y = *(f32*)(pos + 0x4);
    z = *(f32*)(pos + 0x8);
    *(f32*)(p + 0x308) = *(f32*)(p + 0x308) - dt;

    if (*(u8*)(p + 0x274) != 0) {
        f32 t = *(f32*)(p + 0x278) - dt;
        *(f32*)(p + 0x278) = t;
        if (t <= 0.0f) {
            *(f32*)(p + 0x2A0) = 0.0f;
            *(f32*)(p + 0x2B0) = 0.0f;
            fn_8008D29C(p);
        }
    }

    // grid cell of the position
    *(f32*)(p + 0x2F8) = (f32)(s32)(f32)floor((f64)(x / 3.5f));
    {
        f32 cz = (f32)floor((f64)(z / 3.5f));
        y = *(f32*)(p + 0x2BC);
        *(f32*)(p + 0x300) = (f32)(s32)cz;
    }

    // ramp the scale (+0x2D4) up to 0.85 and copy it to the scale vector at +8
    {
        f32 sc = *(f32*)(p + 0x2D4);
        if (sc < 0.85f) {
            f32 n = sc + 0.1f;
            *(f32*)(p + 0x2D4) = n;
            if (n >= 0.85f) *(f32*)(p + 0x2D4) = 0.85f;
            sc = *(f32*)(p + 0x2D4);
            if (sc < 0.0f) sc = 0.0f;
            if (sc > 1.0f) sc = 1.0f;
            *(f32*)(p + 0x2D4) = sc;
            *(f32*)(p + 0x8) = sc;
            *(f32*)(p + 0xC) = sc;
            *(f32*)(p + 0x10) = sc;
        }
    }

    if (*(s32*)(p + 0x26C) == 3) {
        f32 tmp[3];
        f32 v;
        if (*(f32*)(p + 0x2E8) > 0.05f) *(f32*)(p + 0x2E8) = 0.05f;
        if (*(f32*)(p + 0x2E0) > 4.0f) *(f32*)(p + 0x2E0) = 4.0f;
        {
            f32 a = *(f32*)(p + 0x2A4) + *(f32*)(p + 0x2E8);
            f32 b = *(f32*)(p + 0x2B4) + *(f32*)(p + 0x2E0);
            *(f32*)(p + 0x2A4) = a;
            *(f32*)(p + 0x2B4) = b;
        }
        *(f32*)(p + 0x2AC) = 1.0f;
        fn_8008CE28((u8*)tmp, 0.0f);
        *(f32*)(p + 0x2E0) = tmp[0];
        *(f32*)(p + 0x2E4) = tmp[1];
        *(f32*)(p + 0x2E8) = tmp[2];
        v = *(f32*)(p + 0x2F4);
        if (*(f32*)(p + 0x2A4) < v) *(f32*)(p + 0x2A4) = v;
        v = *(f32*)(p + 0x2EC);
        if (fabsf(*(f32*)(p + 0x2B4)) < fabsf(v)) *(f32*)(p + 0x2B4) = v;
        if (z >= 2.0f && *(f32*)(p + 0x2A0) <= kTbl803468D8[*(s32*)(p + 0x2DC)]) {
            f32 t = *(f32*)(p + 0x2A0) + dt;
            f32 step = 3.0f * dt;
            *(f32*)(p + 0x2A0) = t;
            *(f32*)(p + 0x2A4) = *(f32*)(p + 0x2A4) - step;
            fn_8008CE28((u8*)tmp, 0.0f);
            *(f32*)(p + 0x2EC) = tmp[0];
            *(f32*)(p + 0x2F0) = tmp[1];
            *(f32*)(p + 0x2F4) = tmp[2];
        } else {
            *(f32*)(p + 0x2A4) = *(f32*)(p + 0x2A4) + 3.0f * dt;
        }
        v = *(f32*)(p + 0x2A4);
        if (v < 1.0f) *(f32*)(p + 0x2A4) = 1.0f;
        else if (v > 10.0f) *(f32*)(p + 0x2A4) = 10.0f;
        v = *(f32*)(p + 0x2B4);
        if (v < -7.0f) *(f32*)(p + 0x2B4) = -7.0f;
        else if (v > 7.0f) *(f32*)(p + 0x2B4) = 7.0f;
    }

    vz = *(f32*)(p + 0x2A4) * dt;
    if (*(s32*)(p + 0x26C) != 3) {
        s32 idx = StateTableIndex(p);
        f32 t = *(f32*)(p + 0x2A0);
        f32 w = t * kTbl803467F8[idx];
        *(f32*)(p + 0x2A0) = t + dt;
        vz = vz + dt * w;
    }
    vx = dt * (*(f32*)(p + 0x2AC) * *(f32*)(p + 0x2B4));
    if (*(s32*)(p + 0x26C) != 3 && *(f32*)(p + 0x2B0) > 0.0f) {
        s32 idx = StateTableIndex(p);
        if (*(f32*)(p + 0x2B0) < kTbl8034685C[idx]) {
            idx = StateTableIndex(p);
            f32 r = *(f32*)(p + 0x2B0) / kTbl8034685C[idx];
            vx = vx * (r * r);
        }
    }
    {
        s32 state = *(s32*)(p + 0x26C);
        f32 t2 = *(f32*)(p + 0x2B0) - dt;
        z = vz + z;
        x = vx + x;
        *(f32*)(p + 0x2B0) = t2;
        if (state == 1 && t2 <= 0.0f) {
            // re-launch: random duration between the two table values, flip direction
            s32 i1 = StateTableIndex(p);
            s32 i2 = StateTableIndex(p);
            f32 lo = kTbl80346834[i1];
            f32 hi = kTbl80346848[i2];
            u32 rnd = RandomU32();
            f32 rf = 2.3283064e-10f * (f32)rnd;
            *(f32*)(p + 0x2AC) = *(f32*)(p + 0x2AC) * -1.0f;
            *(f32*)(p + 0x2B0) = (lo + (hi - lo) * rf) * (*(f32*)(p + 0x2A8) / *(f32*)(p + 0x2B4));
        } else if (state == 2) {
            f32 dir = 0.0f;
            f32 dx = *(f32*)(*(u8**)(*(u8**)(p + 0x27C) + 0x4)) - *(f32*)(*(u8**)(p + 0x4));
            if (dir != dx) {
                if (dx < dir) dir = -1.0f;
                else dir = 1.0f;
            }
            f32 old = *(f32*)(p + 0x2B0);
            *(f32*)(p + 0x2B0) = old + dt;
            *(f32*)(p + 0x2B4) = *(f32*)(p + 0x2B4) + (dir * -old) * kTbl80346958[*(s32*)(p + 0x2DC)];
        }
    }

    if (z >= 25.0f) {
        EventSend(gEventMgr, p, NULL, 0xB1108090u, 0, 0, 1, 0);
        return;
    }
    pos = *(u8**)(p + 0x4);
    *(f32*)(pos + 0x0) = x;
    *(f32*)(pos + 0x4) = y;
    *(f32*)(pos + 0x8) = z;
    *(u32*)(*(u8**)(p + 0x4) + 0x20) |= 0x8000u;
    *(u32*)(*(u8**)(p + 0x4) + 0x20) |= 0x10000000u;
    *(u32*)(*(u8**)(p + 0x4) + 0x20) |= 0x4000u;
    *(u32*)(*(u8**)(p + 0x4) + 0x20) |= 0x10000000u;
}

}  // extern "C"
