// Draft fixes (miss59a): robot evolution callbacks, small float helpers, zone and
// section-table lookups, and a few event/effect utilities. Verified against dump.sh.
#include "types.h"
#include <math.h>

extern "C++" u8* lbl_803C6A70[3];          // robot table (key at +4, state at +8)
extern "C++" u8 lbl_8047C760[];            // effect manager
extern "C++" f32 lbl_805FDB08;             // float constant (defined in small_8009.cpp)
extern "C++" f32 lbl_805FC928;             // float constant (0.0 threshold)
extern "C++" f32 lbl_805FE68C;             // float constant
extern "C++" f32 lbl_805FE690;             // float constant
extern "C++" f32 lbl_805FEB0C;             // float constant
extern "C++" f32 lbl_805FEB50;             // float constant
extern "C++" f32 lbl_805FEBAC;             // float constant
extern "C++" f32 lbl_805FEBB0;             // float constant
extern "C++" f32 lbl_805FEBB4;             // float constant
extern "C++" f32 lbl_805FEC10;             // float constant
extern "C++" f32 lbl_805FEC18;             // float constant (distance threshold)
extern "C++" f32 lbl_803E6A48[];           // float table (4 entries)
extern "C++" u8* lbl_805FB240;             // pointer to a table with count at +0xC
extern "C++" u8 lbl_803C4298[];            // table, sub-object at +4
extern "C++" u8 lbl_805FB248[];            // lab books section table pointer
extern "C++" s32 lbl_8036CA10[];           // event id table
extern "C++" const char lbl_805F8DE8[];    // name string
extern "C++" u8 gWorldState[];             // world state (pointer at offset 0)

extern "C" {
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner); // 0x801C80E0
void*   EventPost(s32 id, void* sender);                                          // 0x801F3918
u32     StringHash(const char* s);                                                // 0x801EB704
void    fn_801B6878(u8* obj, void* a1, s32 a2, s32 a3);                          // 0x801B6878
f64     fn_8033E8EC(f64 x);                                                       // 0x8033E8EC
void    fn_80117320(u8* p, s32 idx);                                              // 0x80117320
}

typedef void (*VcallObjArgFlag)(u8*, void*, s32, s32);
typedef void (*VcallObjInt)(u8*, s32);
typedef u8* (*VcallObjHash)(u8*, u32);

// Hash of "creatures" (lab books section object vfunc 0x28 argument).
static const u32 kHashCreatures = 0xE5368B8Bu;

extern "C" {

// 0x80109A0C
// Returns 1 when arg is greater than 2000 (unsigned), else 0.
s32 fn_80109A0C(s32 arg0) {
    return (u32)arg0 > 2000u ? 1 : 0;
}

// 0x80093580
// Looks up the robot whose key (+0x10 of arg0) matches; returns 1 if its state is 4 or 5.
s32 RobotEvolutionExperiment_RobotGameOver(u8* arg0) {
    u32 key = *(u32*)(arg0 + 0x10);
    u8* e = 0;
    for (s32 i = 0; i < 3; i++) {
        if (*(u32*)(lbl_803C6A70[i] + 4) == key) {
            e = lbl_803C6A70[i];
            break;
        }
    }
    if (e == 0) {
        return 0;
    }
    s32 state = *(s32*)(e + 8);
    return (state == 4 || state == 5) ? 1 : 0;
}

// 0x80092EA0
// Spawns the landing effect at the robot's position (robot's +0x18 sub-object, +8..+0x10).
void RobotEvolutionExperiment_RobotLanded(u8* arg0) {
    u32 key = *(u32*)(arg0 + 0x10);
    u8* e = 0;
    for (s32 i = 0; i < 3; i++) {
        if (*(u32*)(lbl_803C6A70[i] + 4) == key) {
            e = lbl_803C6A70[i];
            break;
        }
    }
    if (e != 0) {
        u8* fx = *(u8**)(e + 0x18);
        f32 pos[4];
        pos[0] = *(f32*)(fx + 0x8);
        pos[1] = *(f32*)(fx + 0xC);
        pos[2] = *(f32*)(fx + 0x10);
        pos[3] = lbl_805FDB08;
        SpawnEffectAt(lbl_8047C760, 0xF5BB8364u, pos, 0);
    }
}

// 0x80092F50
// Sets the robot's state to 1 (ready) unless it is already in state 4 or 5.
void RobotEvolutionExperiment_RobotReadyToPlay(u8* arg0) {
    u32 key = *(u32*)(arg0 + 0x10);
    u8* e = 0;
    for (s32 i = 0; i < 3; i++) {
        if (*(u32*)(lbl_803C6A70[i] + 4) == key) {
            e = lbl_803C6A70[i];
            break;
        }
    }
    if (e == 0) {
        return;
    }
    s32 state = *(s32*)(e + 8);
    if (state == 4 || state == 5) {
        return;
    }
    *(s32*)(e + 8) = 1;
}

// 0x80024E90
// Clears the flag at +0xFD and forwards to the vfunc at 0x30 of the sub-object's vtable (arg 0x1A).
void fn_80024E90(u8* arg0) {
    *(u8*)(arg0 + 0xFD) = 0;
    u8* o = *(u8**)(arg0 + 0x100);
    VcallObjInt fn = *(VcallObjInt*)(*(u8**)(o + 0x138) + 0x30);
    fn(o, 0x1A);
}

// 0x80024EB0
// Classifies the value at +0x104 against zero: 0 if negative, 1 if positive, 2 if zero.
s32 fn_80024EB0(u8* arg0) {
    f32 v = *(f32*)(arg0 + 0x104);
    if (v < lbl_805FC928) {
        return 0;
    }
    if (v > lbl_805FC928) {
        return 1;
    }
    return 2;
}

// 0x800D8724
// Searches the two per-zone tables of the current zone set for an entry whose id (+0x2C)
// equals arg1; returns the first match, or NULL. arg0 is unused (r3 is clobbered by the code).
u8* fn_800D8724(u8* arg0, u32 arg1) {
    u8* ws = *(u8**)gWorldState;
    u8* base = *(u8**)(ws + 0x1A4);

    s32 n1 = *(s32*)(base + 0x94);
    u8* arr1 = *(u8**)(base + 0x120);
    for (s32 i = 0; i < n1; i++) {
        u8* e = *(u8**)(arr1 + i * 4);
        if (*(u32*)(e + 0x2C) == arg1) {
            return e;
        }
    }

    s32 n2 = *(s32*)(base + 0x98);
    u8* arr2 = *(u8**)(base + 0x12C);
    for (s32 i = 0; i < n2; i++) {
        u8* e = *(u8**)(arr2 + i * 4);
        if (*(u32*)(e + 0x2C) == arg1) {
            return e;
        }
    }
    return 0;
}

// 0x800D8CB0
// Splits the vector at arg0->[1] into four outputs: x+c1, x-c1 (out 3), z-c2, z+c2.
void fn_800D8CB0(u8* arg0, f32* arg1, f32* arg2, f32* arg3, f32* arg4) {
    f32* v = *(f32**)(arg0 + 0x4);
    f32 x = v[0];
    f32 z = v[2];
    *arg1 = lbl_805FE68C + x;
    *arg2 = z - lbl_805FE690;
    *arg3 = x - lbl_805FE68C;
    *arg4 = lbl_805FE690 + z;
}

// 0x800EF70C
// Returns a state code for the object's kind byte (+0x7F): 2 for kinds 0x08/0x10/0x40, else 3.
// Kind 0x20 first calls a vfunc (or fn_801B6878 when the name hash at +4 matches).
s32 fn_800EF70C(u8* arg0) {
    u8 kind = *(u8*)(arg0 + 0x7F);
    if (kind == 0x20) {
        u8* obj = *(u8**)(arg0 + 0x68);
        if (*(u32*)(arg0 + 4) == StringHash(lbl_805F8DE8)) {
            fn_801B6878(obj, *(void**)arg0, 0, 0);
        } else {
            VcallObjArgFlag vf = *(VcallObjArgFlag*)(*(u8**)obj + 0xA4);
            vf(obj, *(void**)arg0, 0, 0);
        }
        return 3;
    }
    if (kind == 0x08 || kind == 0x10 || kind == 0x40) {
        return 2;
    }
    return 3;
}

// 0x800FB5B8
// Angle-like helper: y = f1, x = f2. Returns a value in a fixed set of constants or
// a value derived from fn_8033E8EC(y / x) when both magnitudes are large.
f32 fn_800FB5B8(f32 y, f32 x) {
    f32 r;
    if (fabsf(x) >= lbl_805FEBAC) {
        if (fabsf(y) >= lbl_805FEBAC) {
            f32 q = (f32)fn_8033E8EC((f64)(y / x));
            if (y > lbl_805FEB0C) {
                r = -q;
            } else {
                r = lbl_805FEBB0 - q;
            }
        } else {
            r = (x > lbl_805FEB0C) ? lbl_805FEBB4 : lbl_805FEB50;
        }
    } else {
        r = (y > lbl_805FEB0C) ? lbl_805FEB0C : lbl_805FEBB0;
    }
    return r;
}

// 0x8010A578
// Posts event lbl_8036CA10[idx]; returns the payload + 4 when idx is 0..3 and the event
// returned a payload, else NULL.
void* fn_8010A578(s32 arg0) {
    u8* e = (u8*)EventPost(lbl_8036CA10[arg0], 0);
    if (e == 0) {
        return 0;
    }
    if (arg0 >= 0 && arg0 <= 3) {
        return e + 4;
    }
    return 0;
}

// 0x8010A678
// Adds the offset table lbl_803E6A48 to the vector at (lbl_803C4298[1]+4) and writes the
// result (4 floats, last one plus lbl_805FEC10) into +0x10..+0x1C of each non-null entry
// of the list at lbl_805FB240 (count at +0xC, pointers from +0x14).
void fn_8010A678(void) {
    u8* list = lbl_805FB240;
    if (list == 0) {
        return;
    }
    u8* a = *(u8**)(lbl_803C4298 + 4);
    if (a == 0) {
        return;
    }
    f32* v = *(f32**)(a + 4);
    f32 x = lbl_803E6A48[0] + v[0];
    f32 y = lbl_803E6A48[1] + v[1];
    f32 z = lbl_803E6A48[2] + v[2];
    f32 w = lbl_803E6A48[3] + lbl_805FEC10;

    s32 count = *(s32*)(list + 0xC);
    for (s32 i = 0; i < count; i++) {
        u8* e = *(u8**)(list + 0x14 + i * 4);
        if (e != 0) {
            *(f32*)(e + 0x10) = x;
            *(f32*)(e + 0x14) = y;
            *(f32*)(e + 0x18) = z;
            *(f32*)(e + 0x1C) = w;
        }
    }
}

// 0x8010B05C
// Distance between control point arg1 and the next one (or the previous one, for the last
// index) in the list at arg0[0] (stride 0x28, count at +0x24). Returns sqrt of the squared
// distance when it is greater than lbl_805FEC18, else lbl_805FEC18.
f32 fn_8010B05C(u8* arg0, s32 arg1) {
    s32 count = *(s32*)(arg0 + 0x24);
    s32 j = (arg1 >= count - 1 || arg1 < 0) ? -1 : arg1 + 1;
    u8* base = *(u8**)arg0;
    u8* a = base + arg1 * 0x28;
    u8* b = base + j * 0x28;

    f32 dx = *(f32*)(b + 0xC) - *(f32*)(a + 0xC);
    f32 dy = *(f32*)(b + 0x10) - *(f32*)(a + 0x10);
    f32 dz = *(f32*)(b + 0x14) - *(f32*)(a + 0x14);
    f32 d2 = dz * dz + (dx * dx + dy * dy);

    if (!(d2 <= lbl_805FEC18)) {
        return sqrtf(d2);
    }
    return lbl_805FEC18;
}

// 0x8010B200
// Walks the segment list at arg1 (base +0, count +4, total +8) with distance farg0 scaled by
// the total, and writes the interpolated point into arg0 (stride 0x1C, duration at +0x18).
void fn_8010B200(f32* arg0, u8* arg1, f32 farg0) {
    f32 total = *(f32*)(arg1 + 0x8);
    s32 count = *(s32*)(arg1 + 0x4);
    u8* base = *(u8**)arg1;

    f32 t = farg0 * total;
    s32 n = 0;
    for (s32 i = 0; i < count; i++) {
        f32 d = *(f32*)(base + i * 0x1C + 0x18);
        if (t < d) {
            break;
        }
        t -= d;
        n++;
    }
    if (n == count) {
        n--;
        t = *(f32*)(base + n * 0x1C + 0x18);
    }

    u8* e = base + n * 0x1C;
    f32 dur = *(f32*)(e + 0x18);
    f32 f = t / dur;
    arg0[0] = *(f32*)(e + 0x0) + *(f32*)(e + 0xC) * f;
    arg0[1] = *(f32*)(e + 0x4) + *(f32*)(e + 0x10) * f;
    arg0[2] = *(f32*)(e + 0x8) + *(f32*)(e + 0x14) * f;
}

// 0x801132BC
// Looks up the creatures page object in the lab books section table, calls its vfunc 0x28
// with the creatures hash, and forwards arg0 to fn_80117320 when a page is found.
// Returns 0 (the asm leaves r3 as whatever fn_80117320 left, which callers never read).
s32 fn_801132BC(s32 arg0) {
    u8* top = *(u8**)lbl_805FB248;
    u8* o = (top != 0) ? *(u8**)(top + 4) : 0;
    if (o != 0) {
        VcallObjHash fn = *(VcallObjHash*)(*(u8**)o + 0x28);
        o = fn(o, kHashCreatures);
    }
    if (o != 0) {
        fn_80117320(o, arg0);
    }
    return 0;
}

}  // extern "C"
