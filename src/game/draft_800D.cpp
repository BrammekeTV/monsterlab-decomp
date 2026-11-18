// Functions in 0x800Dxxxx: object database (gObjDb) and its owner object.
#include "types.h"
#include "engine/math.h"

extern "C" {
int   SysSprintf(char* buf, const char* fmt, ...);                 // 0x80142EF8
void  fn_8006FAF4(u8* p, s32 a);                                   // 0x8006FAF4
void  fn_801455E0(f32* out, u8* quat, u8* arg);                    // 0x801455E0
u8*   fn_801B3308(void);                                           // 0x801B3308
void  fn_801B3378(u8* p, u8* key);                                 // 0x801B3378
void  fn_801C83AC(u8* mgr, u8* item);                              // 0x801C83AC
void* fn_801D40E0(s32 n);                                          // 0x801D40E0
void  operator_delete(void* p);                                    // 0x801D426C
u8*   fn_801F6238(u8* p, s32 del);                                 // 0x801F6238
u8*   fn_801F858C(u8* p, s32 del);                                 // 0x801F858C
u8*   fn_801F69B8(u8* p, s32 del);                                 // 0x801F69B8
u8*   fn_801F71E8(u8* p, s32 del);                                 // 0x801F71E8
u8*   fn_801F7920(u8* p, s32 del);                                 // 0x801F7920
u8*   fn_801F81E0(u8* p, s32 del);                                 // 0x801F81E0
u8*   fn_801F94EC(u8* p, s32 del);                                 // 0x801F94EC
u8*   fn_801F9598(u8* p, s32 del);                                 // 0x801F9598
u8*   fn_801F9C78(u8* p, s32 del);                                 // 0x801F9C78
u8*   fn_801FA048(u8* p, s32 del);                                 // 0x801FA048
u8*   fn_801FA8C8(u8* p, s32 del);                                 // 0x801FA8C8
u8*   fn_801FAB3C(u8* p, s32 del);                                 // 0x801FAB3C
void  fn_801F6B78(u8* p, f32* v);                                  // 0x801F6B78
void  fn_801F916C(f32* out, u8* p, s32 a);                         // 0x801F916C
const char* fn_800D2D38(u8* p, f32* v);                            // 0x800D2D38
u8*   fn_800D4B68(u8* p);                                          // 0x800D4B68
u8*   fn_800D4B74(u8* p, s32 del);                                          // 0x800D4B74
u8*   fn_800D4BB4(u8* p);                                          // 0x800D4BB4
u8*   fn_800D4BE8(u8* p, s32 del);                                          // 0x800D4BE8
void  fn_800D4C64(u8* p);                                          // 0x800D4C64
void  fn_800D50C4(u8* p);                                          // 0x800D50C4
void  fn_800D5110(u8* holder, u8* obj, u8* data);                  // 0x800D5110
void  fn_800D5174(u8* p, f32 dt);                                  // 0x800D5174
void  fn_8032F1E8(void* arr, void (*ctor)(void), void (*dtor)(void), s32 size, s32 count); // 0x8032F1E8
void  fn_800D14F0(u8* p);

extern void* gObjDb;
extern u8  lbl_8035FB30[];          // 0x8035FB30 (vtable)
extern u8  lbl_8035FC08[];          // 0x8035FC08 (object db vtable)
extern u8  lbl_803C4298[];          // 0x803C4298
extern u8  lbl_8047C760[];          // 0x8047C760 (effect manager)
extern f32 lbl_805FE5B8;            // 0x805FE5B8 (0.0)
extern f32 lbl_805FE5DC;            // 0x805FE5DC
extern f32 lbl_805FE5E0;            // 0x805FE5E0
extern f32 lbl_805FE5F4;            // 0x805FE5F4
extern f32 lbl_805FE5F8;            // 0x805FE5F8 (initial best distance)

typedef void (*DbDtorFn)(void* self, s32 del);

// Object db layout: entries of 0x1AC bytes at +0x13284, count at +0x2DCD8, vtable at +0x36458.
// Entry pointer with the index clamped to [0, count]; an index outside that range gives entry 0.
static inline u8* ObjDbEntry(u8* db, s32 idx) {
    s32 count = *(s32*)(db + 0x2DCD8);
    s32 c = idx < 0 ? 0 : (idx > count ? count : idx);
    return idx != c ? db + 0x13284 : db + 0x13284 + idx * 0x1AC;
}

// Releases every entry's +0x50 item to the effect manager, then deletes the db through its vtable.
static inline void ObjDbDestroy(void) {
    u8* db = (u8*)gObjDb;
    if (db != NULL) {
        for (s32 i = *(s32*)(db + 0x2DCD8) - 1; i > -1; i--) {
            u8* e = ObjDbEntry((u8*)gObjDb, i);
            if (*(u8**)(e + 0x50) != NULL) {
                fn_801C83AC(lbl_8047C760, *(u8**)(ObjDbEntry((u8*)gObjDb, i) + 0x50));
                *(u8**)(ObjDbEntry((u8*)gObjDb, i) + 0x50) = NULL;
            }
        }
        if (gObjDb != NULL) {
            u8* d = (u8*)gObjDb;
            (*(DbDtorFn*)(*(u8**)(d + 0x36458) + 0x8))(d, 1);
        }
    }
    gObjDb = NULL;
}

// 0x800D01D8
// Writes the position (+0x58) and the Y axis of the orientation (+0x64) to the out vectors, and
// stores the transformed result into the global at lbl_803C4298+4 (flag at +0xFC, vector at +0xF0).
s32 fn_800D01D8(u8* arg0, f32* arg1, f32* arg2) {
    Vec3 axis;
    f32 sp8[3];

    arg1[0] = *(f32*)(arg0 + 0x58);
    arg1[1] = *(f32*)(arg0 + 0x5C);
    arg1[2] = *(f32*)(arg0 + 0x60);
    axis = QuatGetAxisY((const Quat*)(arg0 + 0x64));
    arg2[0] = axis.x;
    arg2[1] = axis.y;
    arg2[2] = axis.z;
    u8* g = *(u8**)(lbl_803C4298 + 4);
    *(u8*)(g + 0xFC) = 1;
    fn_801455E0(sp8, arg0 + 0x64, g);
    g = *(u8**)(lbl_803C4298 + 4);
    *(f32*)(g + 0xF0) = sp8[0];
    *(f32*)(g + 0xF4) = sp8[1];
    *(f32*)(g + 0xF8) = sp8[2];
    return 1;
}

// 0x800D0C40
// Deleting destructor: sets vtable, unregisters (fn_800D14F0), destroys the 23 embedded members
// back to front, runs the base cleanup, frees if del > 0.
u8* fn_800D0C40(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(u8**)(arg0 + 0x4) = lbl_8035FB30;
        fn_800D14F0(arg0);
        fn_801F6238(arg0 + 0x29C8, -1);
        fn_801F69B8(arg0 + 0x2908, -1);
        fn_801F71E8(arg0 + 0x2860, -1);
        fn_801F9C78(arg0 + 0x27A4, -1);
        fn_801F9598(arg0 + 0x26F0, -1);
        fn_801F7920(arg0 + 0x25CC, -1);
        fn_801F94EC(arg0 + 0x2148, -1);
        fn_801F81E0(arg0 + 0x1CC4, -1);
        fn_801F858C(arg0 + 0x1840, -1);
        fn_801FAB3C(arg0 + 0x1780, -1);
        fn_801FA8C8(arg0 + 0x16CC, -1);
        fn_801FA048(arg0 + 0x15CC, -1);
        fn_801F69B8(arg0 + 0x150C, -1);
        fn_801F71E8(arg0 + 0x1464, -1);
        fn_801F9C78(arg0 + 0x13A8, -1);
        fn_801F9598(arg0 + 0x12F4, -1);
        fn_801F7920(arg0 + 0x11D0, -1);
        fn_801F94EC(arg0 + 0xD4C, -1);
        fn_801F81E0(arg0 + 0x8C8, -1);
        fn_801F858C(arg0 + 0x444, -1);
        fn_801FAB3C(arg0 + 0x384, -1);
        fn_801FA8C8(arg0 + 0x2D0, -1);
        fn_801FA048(arg0 + 0x1D0, -1);
        fn_8006FAF4(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x800D14F0
// Removes the 23 embedded members from the global list (fn_801B3308) in ascending offset order.
void fn_800D14F0(u8* arg0) {
    fn_801B3378(fn_801B3308(), arg0 + 0x1D0);
    fn_801B3378(fn_801B3308(), arg0 + 0x2D0);
    fn_801B3378(fn_801B3308(), arg0 + 0x384);
    fn_801B3378(fn_801B3308(), arg0 + 0x444);
    fn_801B3378(fn_801B3308(), arg0 + 0x8C8);
    fn_801B3378(fn_801B3308(), arg0 + 0xD4C);
    fn_801B3378(fn_801B3308(), arg0 + 0x11D0);
    fn_801B3378(fn_801B3308(), arg0 + 0x12F4);
    fn_801B3378(fn_801B3308(), arg0 + 0x13A8);
    fn_801B3378(fn_801B3308(), arg0 + 0x1464);
    fn_801B3378(fn_801B3308(), arg0 + 0x150C);
    fn_801B3378(fn_801B3308(), arg0 + 0x15CC);
    fn_801B3378(fn_801B3308(), arg0 + 0x16CC);
    fn_801B3378(fn_801B3308(), arg0 + 0x1780);
    fn_801B3378(fn_801B3308(), arg0 + 0x1840);
    fn_801B3378(fn_801B3308(), arg0 + 0x1CC4);
    fn_801B3378(fn_801B3308(), arg0 + 0x2148);
    fn_801B3378(fn_801B3308(), arg0 + 0x25CC);
    fn_801B3378(fn_801B3308(), arg0 + 0x26F0);
    fn_801B3378(fn_801B3308(), arg0 + 0x27A4);
    fn_801B3378(fn_801B3308(), arg0 + 0x2860);
    fn_801B3378(fn_801B3308(), arg0 + 0x2908);
    fn_801B3378(fn_801B3308(), arg0 + 0x29C8);
}

// Dot of the vectors in ring-buffer slots (+0x180, +0x184) of 12-byte vectors at +0x188 is
// close to zero.
static inline u8* RingSlotZeroDot(u8* arg0) {
    s32 i = *(s32*)(arg0 + 0x180) + 1;
    *(s32*)(arg0 + 0x180) = i;
    if (i >= 6) {
        *(s32*)(arg0 + 0x180) = 0;
    }
    u8* a = arg0 + *(s32*)(arg0 + 0x180) * 0xC;
    u8* b = arg0 + *(s32*)(arg0 + 0x184) * 0xC;
    f32 d = (*(f32*)(b + 0x188) * *(f32*)(a + 0x188)) + (*(f32*)(b + 0x18C) * *(f32*)(a + 0x18C));
    d = (*(f32*)(b + 0x190) * *(f32*)(a + 0x190)) + d;
    if (d > lbl_805FE5DC && d < lbl_805FE5E0) {
        return a;
    }
    return NULL;
}

// 0x800D28B8
// Advances the ring index up to three times until the new slot's vector is near-orthogonal to the
// slot at +0x184, then feeds that vector to the members at +0x150C and +0x2908; recurses if none.
void fn_800D28B8(u8* arg0) {
    f32 v[3];
    u8* slot = NULL;

    for (s32 n = 0; n < 3; n++) {
        slot = RingSlotZeroDot(arg0);
        if (slot != NULL) {
            v[0] = *(f32*)(slot + 0x188);
            v[1] = *(f32*)(slot + 0x18C);
            v[2] = *(f32*)(slot + 0x190);
            fn_801F6B78(arg0 + 0x150C, v);
            fn_801F6B78(arg0 + 0x2908, v);
            return;
        }
    }
    fn_800D28B8(arg0);
}

// 0x800D2B1C
// Debug text: tracks how long the vector from fn_801F916C stays unchanged (+0x300 time, +0x304
// last-change time, +0x308 vector, +0x314 count) and formats it into the buffer at arg0.
void fn_800D2B1C(u8* arg0, u8* arg1, f32 farg0) {
    f32 a[3];
    f32 b[3];
    f32 c[3];
    f32 z[3];

    if (*(u8*)(arg1 + 0x90) != 0) {
        f32 f2 = *(f32*)(arg1 + 0x94);
        if (f2 > lbl_805FE5B8) {
            *(f32*)(arg1 + 0x8) = f2;
        }
    }
    if (*(u8*)(arg1 + 0x90) != 0) {
        fn_801F916C(a, arg1, 0);
        s32 same = 0;
        if (*(f32*)(arg0 + 0x308) == a[0] && *(f32*)(arg0 + 0x30C) == a[1] && *(f32*)(arg0 + 0x310) == a[2]) {
            same = 1;
        }
        if (same != 0) {
            f32 t = *(f32*)(arg0 + 0x300) + farg0;
            *(s32*)(arg0 + 0x314) = *(s32*)(arg0 + 0x314) + 1;
            *(f32*)(arg0 + 0x300) = t;
        } else {
            *(s32*)(arg0 + 0x314) = 1;
            *(f32*)(arg0 + 0x300) = lbl_805FE5B8;
        }
        fn_801F916C(b, arg1, 0);
        *(f32*)(arg0 + 0x308) = b[0];
        *(f32*)(arg0 + 0x30C) = b[1];
        *(f32*)(arg0 + 0x310) = b[2];
        *(f32*)(arg0 + 0x304) = *(f32*)(arg0 + 0x300);
        c[0] = b[0];
        c[1] = *(f32*)(arg0 + 0x30C);
        c[2] = *(f32*)(arg0 + 0x310);
        SysSprintf((char*)arg0, "%s: %s %d %0.03f %0.03f", (char*)(arg0 + 0x100), fn_800D2D38(arg0, c),
                   *(s32*)(arg0 + 0x314), *(f32*)(arg0 + 0x300), *(f32*)(arg0 + 0x304));
        return;
    }
    f32 last = *(f32*)(arg0 + 0x304);
    f32 t = *(f32*)(arg0 + 0x300) + farg0;
    f32 diff = t - last;
    *(f32*)(arg0 + 0x300) = t;
    if (diff > lbl_805FE5F4) {
        SysSprintf((char*)arg0, "%s: None", (char*)(arg0 + 0x100));
        f32 zero = lbl_805FE5B8;
        *(s32*)(arg0 + 0x314) = 0;
        *(f32*)(arg0 + 0x304) = zero;
        z[2] = zero;
        z[1] = zero;
        z[0] = zero;
        *(f32*)(arg0 + 0x308) = zero;
        *(f32*)(arg0 + 0x30C) = zero;
        *(f32*)(arg0 + 0x310) = zero;
        return;
    }
    c[0] = *(f32*)(arg0 + 0x308);
    c[1] = *(f32*)(arg0 + 0x30C);
    c[2] = *(f32*)(arg0 + 0x310);
    SysSprintf((char*)arg0, "%s: %s %d %0.03f %0.03f", (char*)(arg0 + 0x100), fn_800D2D38(arg0, c),
               *(s32*)(arg0 + 0x314), *(f32*)(arg0 + 0x300), *(f32*)(arg0 + 0x304));
}

// 0x800D3428
// Recreates the global object db: destroys the old one, allocates 0x3645C bytes and constructs the
// two arrays (20 entries of 0x428 at +0x428 with a busy-flag byte per entry at +0x5748, and 0xFF
// entries of 0xDC at +0x575C), initialises every 0x1AC entry at +0x13284, and clears the count.
void fn_800D3428(void) {
    ObjDbDestroy();
    u8* p = (u8*)fn_801D40E0(0x3645C);
    if (p != NULL) {
        *(u8**)(p + 0x36458) = lbl_8035FC08;
        fn_800D50C4(p);
        u8* arrA = p + 0x428;
        fn_8032F1E8(arrA, (void (*)(void))fn_800D4BB4, (void (*)(void))fn_800D4BE8, 0x428, 0x14);
        for (s32 i = 0; i < 0x14; i++) {
            *(u8*)(arrA + 0x5320 + i) = 0;
        }
        u8* arrB = p + 0x575C;
        fn_8032F1E8(arrB, (void (*)(void))fn_800D4B68, (void (*)(void))fn_800D4B74, 0xDC, 0xFF);
        *(s32*)(arrB + 0xDB24) = 0;
        for (u8* e = p + 0x13284; e < p + 0x2DCD8; e += 0x1AC) {
            fn_800D4C64(e);
        }
        *(s32*)(p + 0x2DCD8) = 0;
        *(s32*)(p + 0x36454) = 0;
    }
    gObjDb = p;
}

// 0x800D36A0
// Destroys the global object db (see ObjDbDestroy) and clears gObjDb.
void fn_800D36A0(void) {
    ObjDbDestroy();
}

// 0x800D3818
// Updates every busy slot (flag byte at +0x5748+i) of the 20 0x428-byte entries at +0x428 with dt.
void fn_800D3818(u8* arg0, f32 farg0) {
    for (s32 i = 0; i < 0x14; i++) {
        if (*(u8*)(arg0 + 0x5748 + i) != 0) {
            fn_800D5174(arg0 + 0x428 + i * 0x428 + 0x24, farg0);
        }
    }
}

// 0x800D39BC
// Finds the entry whose first word equals the id (searching from the last entry) and binds it to
// the holder at +0x404 via fn_800D5110. Returns the db.
u8* ObjDbLookup(void* db_, s32 arg1) {
    u8* db = (u8*)db_;
    s32 i = *(s32*)(db + 0x2DCD8) - 1;
    while (i > -1) {
        if ((u32)arg1 == *(u32*)ObjDbEntry(db, i)) {
            break;
        }
        i--;
    }
    if (i >= 0) {
        fn_800D5110(db, db + 0x404, ObjDbEntry(db, i));
    }
    return db;
}

// 0x800D3AC4
// Finds the entry nearest (squared distance, entry +0x34 vs pos) to pos, claims a free slot of the
// 20 busy-flagged ones, binds it to that entry and returns slot+0x24, or NULL.
u8* fn_800D3AC4(u8* arg0, f32* arg1) {
    f32 d[3];
    f32 best = lbl_805FE5F8;
    u8* found = NULL;

    for (s32 i = *(s32*)(arg0 + 0x2DCD8) - 1; i > -1; i--) {
        u8* e = ObjDbEntry(arg0, i);
        f32 dx = *(f32*)(e + 0x34) - arg1[0];
        f32 dy = *(f32*)(e + 0x38) - arg1[1];
        f32 dz = *(f32*)(e + 0x3C) - arg1[2];
        d[0] = dx;
        d[1] = dy;
        d[2] = dz;
        f32 dist = (dz * dz) + ((dx * dx) + (dy * dy));
        if (dist < best) {
            found = e;
            best = dist;
        }
    }
    if (found == NULL) {
        return NULL;
    }
    u8* slot = NULL;
    for (s32 i = 0; i < 0x14; i++) {
        if (*(u8*)(arg0 + 0x5748 + i) == 0) {
            *(u8*)(arg0 + 0x5748 + i) = 1;
            slot = arg0 + 0x428 + i * 0x428;
            break;
        }
    }
    if (slot == NULL) {
        return NULL;
    }
    fn_800D5110(slot + 0x24, slot, found);
    return slot + 0x24;
}

}  // extern "C"
