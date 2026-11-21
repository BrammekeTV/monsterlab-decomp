// Misc functions batch pz3d: weighted random pick, home-menu loop, grid layout, anim clip start,
// sound volume scale, sir load request, screen-set candidate callback, movie shutdown, ...
#include <math.h>
#include <stdint.h>

#include "types.h"
#include "engine/math.h"
#include "system/sys.h"

extern "C++" void* gScreenSets;        // 0x805FB654
extern "C++" void* lbl_805FB670;
// Defined in C++ files (no extern "C" there).
void fn_8015D530(void);                 // 0x8015D530
void fn_8015DDEC(void);                 // 0x8015DDEC
BOOL fn_801982A8(void* obj, u32 mask);  // 0x801982A8       // 0x805FB670 (screen set id table, see game_ui.cpp)

extern "C" {
void*  memset(void* p, int c, unsigned long n);
char*  strcpy(char* d, const char* s);

f32    AnimGetFrame(void* anim, f32 fps);                         // 0x8018C014
void*  EventPost(s32 id, void* sender);                           // 0x801F3918
s32    RandomRoll(void);                                          // raw 32-bit random (used unsigned)
s32    TriggerDoAction(void* action, void* a1, void* a2);         // 0x801E0054
u8*    fn_80044634(s32 n);                                        // 0x80044634
void   fn_800446BC(void* cam, s32 v);                             // 0x800446BC
void   fn_8010AB3C(void);                                         // 0x8010AB3C
void   fn_80121434(u8* v, f32 a, f32 b);                          // 0x80121434
void   fn_8012145C(u8* v, f32 x, f32 y, f32 z);                   // 0x8012145C
u32    fn_80143A5C(const char* s);                                // 0x80143A5C (name hash)
void   fn_801458B0(f32* quat, f32* a, f32* b);                    // 0x801458B0
void   fn_80146180(void* quat, const Vec3* axis, f32 angle);      // 0x80146180 (rotate quat about axis)
s32    fn_80151644(s32 a, s32 b, u8* quad);                       // 0x80151644
void    fn_80152620(s32 a, s32 b, u8* quad);                       // 0x80152620
void   fn_8015D578(void);                                         // 0x8015D578
void   fn_8015D5A4(void);                                         // 0x8015D5A4
void   fn_8017493C(void* p);                                      // 0x8017493C
void   fn_80174A24(u8* p, u32 color);                             // 0x80174A24
void   fn_80174A70(u8* p, f32 x, f32 y, f32 z);                   // 0x80174A70
void*  fn_8018B78C(u8* b, u8* entry, s32 a, f32 x, f32 y, s32 c, s32 d);  // 0x8018B78C
f32    fn_8018C044(u8* anim, f32 fps);                            // 0x8018C044
u8* fn_8018C48C(u8* base, u32 key);                                        // 0x8018C48C
u32    fn_8018C8CC(u8* anim);                                     // 0x8018C8CC
void   fn_8018D68C(f32* out, void* sets);                         // 0x8018D68C
u32    fn_8019552C(u8* list, s32 index);                          // 0x8019552C (leaf; returns an element pointer)
void   fn_8018C4D8(u8* p);                                        // 0x8018C4D8
s32 fn_801A9EC8(u8* obj, f32 x, f32 y, f32 z);                        // 0x801A9EC8
s32 fn_8019CCAC(u8* self);                                      // 0x8019CCAC
u8*    fn_801B35C8(u8* p, s32 a);                                 // 0x801B35C8
void   fn_801BB338(u8* p, u32 color);                             // 0x801BB338
void fn_801CA7B0(u32 mode);                                     // 0x801CA7B0
u32 fn_801E6934(u8* loader, void* a, void* dst, u32 b, u32 c, s32 d, s32 e, s32 f, s32 flags);  // 0x801E6934
void   fn_801F0668(s32 a);                                        // 0x801F0668
void   fn_801F5E44(void);                                         // 0x801F5E44
void   fn_801F5E48(void* a, s32 b);                               // 0x801F5E48
void   fn_801F5E4C(void* a, s32 b);                               // 0x801F5E4C
void   fn_801FB10C(void);                                         // 0x801FB10C
void   fn_801FB3F8(void);                                         // 0x801FB3F8
void   fn_80203184(void);                                         // 0x80203184
s32    fn_802C8C04(void);                                         // 0x802C8C04
s32    fn_802C8C38(u8* p);                                        // 0x802C8C38
s32    fn_802C8C88(void);                                         // 0x802C8C88
s32    fn_802C8DF4(void);                                         // 0x802C8DF4
s32    fn_802C8E04(void);                                         // 0x802C8E04

extern u8   lbl_80386190[];     // 0x80386190 (vtable)
extern u8   lbl_8047C640[];     // 0x8047C640 (camera)
extern u8   lbl_804DD180[];     // 0x804DD180
extern u8   lbl_804FB5E0[];     // 0x804FB5E0
extern u8   lbl_804FB8A0[];     // 0x804FB8A0
extern u8   lbl_8051E6A8[];     // 0x8051E6A8 (sound voices, 0x114 each)
extern u32  lbl_805F969C;       // 0x805F969C (invalid-handle sentinel)
extern f32  lbl_805F9820;       // 0x805F9820 (countdown float)
extern f32  lbl_805F98C8;       // 0x805F98C8
extern s32  lbl_805F98A8;       // 0x805F98A8
extern void* lbl_805FB964;      // 0x805FB964 movie player
extern void* lbl_805FB968;      // 0x805FB968 (current target)
extern u32  lbl_805FB96C;       // 0x805FB96C
extern s32  lbl_805FB970;       // 0x805FB970 (movie player active flag)
extern s32  lbl_805FB974;       // 0x805FB974
extern s32  lbl_805FB978;       // 0x805FB978
extern s32  lbl_805FB97C;       // 0x805FB97C
extern s32  lbl_805FB994;       // 0x805FB994
extern s32  lbl_805FB9D8;       // 0x805FB9D8
extern s32  lbl_805FB9EC;       // 0x805FB9EC
extern s32  lbl_805FBAE0;       // 0x805FBAE0 (audio busy flag)
extern s32  lbl_805FBB64;       // 0x805FBB64
extern s32  lbl_805FBB68;       // 0x805FBB68
extern s32  lbl_805FBB6C;       // 0x805FBB6C
extern void (*lbl_805FBA20)(void);   // 0x805FBA20 (callback run when the home menu asks to quit)
}

extern "C" {

// 0x8020DD90
// Weighted random pick: n weights at stride 4 from +0xC (negatives count as 0). Rolls a value in
// [0,1) times the total and returns the first index whose running sum reaches it (0 if none).
// Note: the running sum starts with weight 0 and adds it again in the first step (as the original does).
s32 fn_8020DD90(u8* arg0, s32 n) {
    if (n < 1) return 0;
    f32 total = 0.0f;
    u8* p = arg0;
    for (s32 i = 0; i < n; i++) {
        f32 w = *(f32*)(p + 0xc);
        if (w < 0.0f) w = 0.0f;
        total += w;
        p += 4;
    }
    f32 sum = *(f32*)(arg0 + 0xc);
    f32 target = ((f32)(s32)((u32)RandomRoll() % 1000u) / 1000.0f) * total;
    s32 idx = 0;
    p = arg0;
    for (s32 i = 0; i < n; i++) {
        sum += *(f32*)(p + 0xc);
        if (target <= sum) break;
        p += 4;
        idx++;
    }
    if (idx >= n) return 0;
    return idx;
}

// 0x801F70AC
// Constructor: base init (fn_801B35C8), vtable, direction (0,-1,0) normalised at +0x90, zero +0x9C/+0xA0/+0xA4.
u8* fn_801F70AC(u8* p, s32 a) {
    fn_801B35C8(p, a);
    *(void**)p = (void*)lbl_80386190;
    f32 x = 0.0f, y = -1.0f, z = 0.0f;
    f32 len2 = y * y + x * x + z * z;
    f32 nx, ny, nz;
    if (len2 == 0.0f) {
        nx = ny = nz = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);
        nx = x * inv;
        ny = y * inv;
        nz = z * inv;
    }
    *(f32*)(p + 0x90) = nx;
    *(f32*)(p + 0x94) = ny;
    *(f32*)(p + 0x98) = nz;
    *(s32*)(p + 0x9c) = 0;
    *(f32*)(p + 0xa0) = 0.0f;
    *(u8*)(p + 0xa4)  = 0;
    return p;
}

// 0x801FB624
// Home-menu loop: runs the frame steps until the menu closes (exit request or a choice), then
// reports the choice through gSysEventCallback (1 -> event 2, 2 -> event 1) or lbl_805FBA20 + fn_80203184.
void fn_801FB624(void) {
    fn_801F0668(1);
    fn_8015D530();
    fn_802C8C04();
    s32 choice;
    s32 exitMode;
    do {
        fn_801FB3F8();
        choice = fn_802C8C38(lbl_804FB8A0);
        fn_802C8E04();
        fn_8015D578();
        fn_801FB10C();
        fn_802C8C88();
        fn_8015D5A4();
        exitMode = (s32)SysGetExitMode();
        if (exitMode != 0 || lbl_805F98A8 == 0) fn_802C8DF4();
    } while (choice == -1 && exitMode == 0 && lbl_805F98A8 != 0);
    lbl_805F98A8 = 1;
    fn_8015DDEC();
    if (exitMode == 0) {
        if (choice == 1) {
            if (gSysEventCallback) gSysEventCallback(2);
        } else if (choice == 2) {
            if (gSysEventCallback) gSysEventCallback(1);
        } else {
            if (lbl_805FBA20) lbl_805FBA20();
            fn_80203184();
        }
    }
    fn_801F0668(0);
}

// 0x801C5B0C
// Draws the 4-vertex quad of a node (stride 0x18 per vertex at +8: u, v, color, x, y, z). The flag at
// +4 bit 0 enables it; the textured path (byte at +6 of node->+0x6C) builds 0x18-byte vertices, the
// plain path 0x10-byte ones.
void fn_801C5B0C(u8* node) {
    if ((*(u32*)(node + 4) & 1) == 0) return;
    u8* def = *(u8**)node;
    fn_801CA7B0(*(u32*)(def + 0x74));
    fn_8010AB3C();
    def = *(u8**)node;
    u8* tex = *(u8**)(def + 0x6c);
    s32 camArg = *(s32*)(def + 0x70);
    if (*(u8*)(tex + 6) != 0) {
        fn_800446BC(lbl_8047C640, camArg);
        u8* quad = fn_80044634(0x60);
        u8* v = quad;
        for (u32 i = 0; i < 4; i++) {
            fn_80121434(v, *(f32*)(node + 0x8), *(f32*)(node + 0xc));
            fn_801BB338(v, *(u32*)(node + 0x10));
            fn_8012145C(v, *(f32*)(node + 0x14), *(f32*)(node + 0x18), *(f32*)(node + 0x1c));
            node += 0x18;
            v += 0x18;
        }
        fn_80151644(0x98, 4, quad);
    } else {
        fn_800446BC(lbl_8047C640, 0);
        u8* quad = fn_80044634(0x40);
        u8* v = quad;
        for (u32 i = 0; i < 4; i++) {
            fn_80174A24(v, *(u32*)(node + 0x10));
            fn_80174A70(v, *(f32*)(node + 0x14), *(f32*)(node + 0x18), *(f32*)(node + 0x1c));
            node += 0x18;
            v += 0x10;
        }
        fn_80152620(0x98, 4, quad);
    }
}

// Scales a sound voice's +0x28 from its +0x24 by f (only for voice types other than 2..7).
static void ScaleVoiceVolume(u8* v, f32 f) {
    if (v == NULL) return;
    s32 t = *(s32*)(v + 0x14);
    s32 ok = 1;
    if (t == 2) ok = 0;
    if ((u32)(t - 6) <= 1u) ok = 0;
    if ((u32)(t - 3) <= 2u) ok = 0;
    if (ok == 0) return;
    *(s32*)(v + 0x28) = (s32)((f32)*(s32*)(v + 0x24) * f);
}

// 0x801FFC58
// Sets the global volume scale (clamped to 0..1) and rescales the +0x28 volume of every sound voice
// from index 2 (94 voices, two per 0x228 step) from its +0x24 base; keeps lbl_805FBAE0 set meanwhile.
void fn_801FFC58(f32 v) {
    f32 f = 0.0f;
    if (v >= 0.0f) {
        f = 1.0f;
        if (v <= 1.0f) f = v;
    }
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    lbl_805F98C8 = f;
    u8* p = lbl_8051E6A8 + 0x228;
    for (s32 i = 0; i < 0x2f; i++) {
        ScaleVoiceVolume(*(u8**)(p + 0xc), f);
        ScaleVoiceVolume(*(u8**)(p + 0x120), f);
        p += 0x228;
    }
    lbl_805FBAE0 = saved;
}

// 0x80174AC4
// Initialises a 0x68-byte transform record: identity-like float block, zeroed floats, then
// the Y axis of a freshly built orientation quaternion at +0x68.
void fn_80174AC4(u8* p) {
    Vec3  fwd, up, ax, out;
    Quat  q;

    *(f32*)(p + 0x00) = 0.0f; *(f32*)(p + 0x04) = 0.0f; *(f32*)(p + 0x08) = 0.0f; *(f32*)(p + 0x0c) = 0.0f;
    *(f32*)(p + 0x10) = 1.0f; *(f32*)(p + 0x14) = 1.0f; *(f32*)(p + 0x18) = 1.0f; *(f32*)(p + 0x1c) = 1.0f;
    for (s32 off = 0x20; off <= 0x60; off += 4) *(f32*)(p + off) = 0.0f;
    *(s32*)(p + 0x64) = 0;

    fwd.x = 0.0f; fwd.y = 1.0f; fwd.z = 0.0f;       // stack 0x2C
    up.x  = 0.0f; up.y  = 0.0f; up.z  = 1.0f;       // stack 0x38
    fn_801458B0((f32*)&q, (f32*)&up, (f32*)&fwd);

    ax.x = 1.0f; ax.y = 0.0f; ax.z = 0.0f;
    fn_80146180(&q, &ax, 0.0f);
    ax.x = 0.0f; ax.y = 1.0f; ax.z = 0.0f;
    fn_80146180(&q, &ax, 0.0f);

    out = QuatGetAxisY(&q);
    *(f32*)(p + 0x68) = out.x;
    *(f32*)(p + 0x6c) = out.y;
    *(f32*)(p + 0x70) = out.z;
}

// 0x801ABD84
// Tests whether the anim's current frame lies inside the record's [+8, +0xC) frame window
// (handling a degenerate/zero-length window and wrapped looping anims); gated by an optional trigger.
s32 fn_801ABD84(u8* rec, u8* anim) {
    if (anim == NULL) return 0;
    if (((*(u32*)(rec + 4) >> 27) & 0xf) != 0) {
        if (TriggerDoAction(rec + 4, NULL, NULL) == 0) return 0;
    }
    if (fn_8018C8CC(anim) == 0) return 0;
    f32 cur = AnimGetFrame(anim, 1.0f);
    f32 span = *(f32*)(rec + 0xc) - *(f32*)(rec + 0x8);
    if (span > -0.0001f && span < 0.0001f) {
        f32 end = fn_8018C044(anim, 1.0f);
        u32 fl = *(u32*)(anim + 0xf0);
        u32 loop = fl & 0x10;
        if (loop != 0 && cur < end) {
            f32 s = *(f32*)(rec + 0x8);
            if (s < cur || s > end) return 1;
        }
        if (loop != 0 && (fl & 8) != 0) {
            if (cur <= *(f32*)(rec + 0x8)) return 1;
        }
        f32 s = *(f32*)(rec + 0x8);
        if (s >= end && s < cur) return 1;
        return 0;
    }
    if (cur >= *(f32*)(rec + 0x8) && cur < *(f32*)(rec + 0xc)) return 1;
    return 0;
}

typedef s32 (*PickFn)(u8* self, BOOL mask, f32 x, f32 y);

// 0x80199828
// Screen-set candidate callback: keeps in *best the element (self or the earlier pick) that is
// nearest/topmost for the point from fn_8018D68C, subject to flag checks on self and its +0x128 chain.
// Returns 0.
s32 fn_80199828(u8* self, u8** best) {
    f32 pt[2];
    u8* cur = *best;
    fn_8018D68C(pt, gScreenSets);
    if (cur == NULL || *(f32*)(cur + 0x70) > *(f32*)(self + 0x70)) {
        if (*(s32*)(self + 0xbc) != 0 || fn_8019CCAC(self) != 0) {
            u32 fl = *(u32*)(self + 0x64);
            bool go;
            if ((fl & 2) != 0 && *(f32*)(self + 0x8c) > 0.0f) go = true;
            else go = (fl & 0x400) == 0;
            if (go) {
                u8* n = *(u8**)(self + 0x128);
                s32 all = 1;
                while (n != NULL) {
                    u32 nf = *(u32*)(n + 0x64);
                    if ((nf & 2) != 2) { all = 0; break; }
                    if ((nf & 1) != 1) { all = 0; break; }
                    f32 w = *(f32*)(n + 0x8c);
                    s32 tiny = (w > -0.0001f && w < 0.0001f) ? 1 : 0;
                    if (tiny != 0 || *(f32*)(n + 0x8c) < 0.0f) { all = 0; break; }
                    n = *(u8**)(n + 0x128);
                }
                if (all != 0 || (fl & 0x400) == 0) {
                    BOOL mask = fn_801982A8(lbl_805FB670, 0x40);
                    if ((*(PickFn*)(*(u8**)(self + 0x138) + 0x38))(self, mask, pt[0], pt[1]) != 0) cur = self;
                }
            }
        }
    }
    *best = cur;
    return 0;
}

// 0x801B82A8
// Sets the target's position (a4->+4 transform at +0/+4/+8) from the anim state: while the timer at
// +0x70 is still running (or +0x30 is set) it counts down by 4*+0x24 and offsets +0..+8 by the +0x18..
// vector scaled by sin(1.5708 * timer * +0x28); otherwise it uses the vector at +0xC. Sets the
// 0x10008000/0x4000 flag bits in the target (+0x20) and returns 3 (done) or the value at +0x2C.
s32 fn_801B82A8(u8* p, u8* a4) {
    s32 done = 0;
    f32 v0 = 0.0f, v1 = 0.0f, v2 = 0.0f;
    if (*(s32*)(p + 0x30) != 0 || !(*(f32*)(p + 0x70) >= *(f32*)(p + 0x24))) {
        f32 step = 4.0f * *(f32*)(p + 0x24);
        f32 t = *(f32*)(p + 0x70);
        if (t > step) *(f32*)(p + 0x70) = t - step;
        f32 s = sinf(1.5707964f * (*(f32*)(p + 0x70) * *(f32*)(p + 0x28)));
        f32 dx = *(f32*)(p + 0x18) * s;
        f32 dy = *(f32*)(p + 0x1c) * s;
        f32 dz = *(f32*)(p + 0x20) * s;
        v0 = *(f32*)(p + 0x0) + dx;
        v1 = *(f32*)(p + 0x4) + dy;
        v2 = *(f32*)(p + 0x8) + dz;
    } else {
        v0 = *(f32*)(p + 0xc);
        v1 = *(f32*)(p + 0x10);
        v2 = *(f32*)(p + 0x14);
        done = 1;
    }
    u8* t = *(u8**)(a4 + 4);
    *(f32*)(t + 0) = v0;
    *(f32*)(t + 4) = v1;
    *(f32*)(t + 8) = v2;
    *(u32*)(*(u8**)(a4 + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(a4 + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(a4 + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(a4 + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(a4 + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(a4 + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(a4 + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(a4 + 4) + 0x20) |= 0x10000000;
    if (done != 0) return 3;
    return *(s32*)(p + 0x2c);
}

typedef void (*DtorFn)(void* obj, s32 del);
typedef void (*StopFn)(void* obj);

// 0x801F31A8
// Movie shutdown: if the movie flag (lbl_805FB970) is set, clears the three pending bytes per 0x1E0
// record of lbl_804DD180 (decrementing the counter at +0x3C0 and flooring it at 0), resets the
// movie state globals. Then, if a movie player exists, stops it and deletes it unless a lock is held.
void fn_801F31A8(void) {
    if (lbl_805FB970 != 0) {
        fn_801F5E4C(lbl_804DD180 + 0x3c4, 0);
        u8* rec = lbl_804DD180;
        for (s32 i = 0; i < 2; i++) {
            if (*(u8*)(rec + 0x94) != 0) {
                *(u8*)(rec + 0x94) = 0;
                s32 c = *(s32*)(lbl_804DD180 + 0x3c0) - 1;
                *(s32*)(lbl_804DD180 + 0x3c0) = c;
                if (c < 0) *(s32*)(lbl_804DD180 + 0x3c0) = 0;
            }
            if (*(u8*)(rec + 0x134) != 0) {
                *(u8*)(rec + 0x134) = 0;
                s32 c = *(s32*)(lbl_804DD180 + 0x3c0) - 1;
                *(s32*)(lbl_804DD180 + 0x3c0) = c;
                if (c < 0) *(s32*)(lbl_804DD180 + 0x3c0) = 0;
            }
            if (*(u8*)(rec + 0x1d4) != 0) {
                *(u8*)(rec + 0x1d4) = 0;
                s32 c = *(s32*)(lbl_804DD180 + 0x3c0) - 1;
                *(s32*)(lbl_804DD180 + 0x3c0) = c;
                if (c < 0) *(s32*)(lbl_804DD180 + 0x3c0) = 0;
            }
            rec += 0x1e0;
        }
        fn_801F5E48(lbl_804DD180 + 0x3c4, 0);
        fn_801F5E44();
        if (lbl_805FB970 != 0) {
            if (lbl_805FB9D8 == 0) fn_801F5E4C(lbl_804FB5E0, 6);
            s32 locked = lbl_805FB9D8;
            lbl_805FB968 = NULL;
            if (locked == 0) fn_801F5E48(lbl_804FB5E0, 6);
        }
        fn_801F5E44();
        lbl_805FB974 = 0;
        lbl_805FB970 = 0;
        lbl_805FB978 = 0;
        lbl_805FB97C = 0;
        lbl_805FB96C = 0;
        lbl_805F9820 = -1.0f;
        lbl_805FB994 = 0;
    }
    void* mv = lbl_805FB964;
    if (mv != NULL) {
        lbl_805FBB6C = 0;
        if (lbl_805FBB64 == 0 && lbl_805FBB68 == 0) {
            (*(StopFn*)(*(u8**)mv + 0x14))(mv);
            mv = lbl_805FB964;
            if (mv != NULL) (*(DtorFn*)(*(u8**)mv + 0x8))(mv, 1);
            lbl_805FB964 = NULL;
            lbl_805FB9EC = 0;
        } else {
            lbl_805FB9EC = 1;
        }
    }
}

// 0x801A6B9C
// Lays out the grid cells: divides the +0x180/+0x184 cell steps by the scale (+0x74/+0x78) of the
// grid and each parent in its +0x128 chain, then for every row/column cell (rows +0x13C, cols +0x140;
// cells list +0x1BC, per-row x offsets +0x1B8) calls the cell's vtable +0x2C(0x18) and positions it with
// fn_801A9EC8; if a is set it also stores the position at +0x68/+0x6C and the depth +0x70 on the cell.
s32 fn_801A6B9C(u8* grid, s32 a) {
    f32 depth = *(f32*)(grid + 0x70);
    f32 stepX = *(f32*)(grid + 0x180);
    f32 stepY = *(f32*)(grid + 0x184);
    for (u8* n = grid; n != NULL; n = *(u8**)(n + 0x128)) {
        f32 sx = *(f32*)(n + 0x74);
        f32 sy = *(f32*)(n + 0x78);
        stepX = stepX / sx;
        stepY = stepY / sy;
    }
    for (s32 i = 0; i < *(s32*)(grid + 0x13c); i++) {
        for (s32 j = 0; j < *(s32*)(grid + 0x140); j++) {
            u8* slot = (u8*)(uintptr_t)fn_8019552C(*(u8**)(grid + 0x1bc), i * *(s32*)(grid + 0x140) + j);
            u8* cell = slot != NULL ? *(u8**)slot : NULL;
            if (cell == NULL) continue;
            (*(void (**)(u8*, s32))(*(u8**)(cell + 0x138) + 0x2c))(cell, 0x18);
            f32* rowX = (f32*)(uintptr_t)fn_8019552C(*(u8**)(grid + 0x1b8), i);
            f32 x = *rowX + stepX * (f32)(*(s32*)(grid + 0x160) - 1);
            f32 y = 0.0f + stepY * (f32)(*(s32*)(grid + 0x15c) - 1);
            fn_801A9EC8(cell, x, y, 0.0f /* leftover f3 */);
            if (a != 0) {
                *(f32*)(cell + 0x68) = x;
                *(f32*)(cell + 0x6c) = y;
                *(f32*)(cell + 0x70) = depth;
            }
        }
    }
    return 0;
}

// 0x801AC944
// Starts (clip != NULL) or clears an anim clip slot: looks up the clip's track table entry for the
// obj's key (u16 at obj->+4->+0x32) and index clip+0x38, starts it with fn_8018B78C on b and stores
// the clip's +0 as the started object's +0x1C; or (clip == NULL) frees b's entry via fn_8018C48C.
// Posts event 0x7C (unless obj->+4->+0x20 bit 0x04000000 is set) carrying the clip id (+0x3C, or -1) and a.
// Returns 1 if a clip was started.
s32 fn_801AC944(u8* clip, s32 a, u8* b, s32 c, u8* obj) {
    s32 started = 0;
    if (clip != NULL) {
        u16 key = *(u16*)(*(u8**)(obj + 4) + 0x32);
        u8* tbl = *(u8**)(clip + 0x34);
        u8* ent = NULL;
        for (s32 k = 0; k < 2; k++) {
            if ((s32)key == (s32)*(u16*)(tbl + 0x18 + k * 2)) { ent = *(u8**)(tbl + 0x10 + k * 4); break; }
        }
        if (ent == NULL) key = 0;
        s32 idx = *(s32*)(clip + 0x38);
        ent = NULL;
        for (s32 k = 0; k < 2; k++) {
            if ((s32)key == (s32)*(u16*)(tbl + 0x18 + k * 2)) { ent = *(u8**)(tbl + 0x10 + k * 4); break; }
        }
        if (ent != NULL && idx < *(s32*)(tbl + 0x1c) && idx >= 0) ent = *(u8**)(ent + idx * 4);
        else ent = NULL;
        if (ent != NULL) {
            u8* r = (u8*)fn_8018B78C(b, ent, *(s32*)(clip + 0xc), *(f32*)(clip + 0x1c), *(f32*)(clip + 0x20), a, c);
            if (r != NULL) {
                started = 1;
                *(f32*)(r + 0x1c) = *(f32*)clip;
            }
        }
        if ((*(u32*)(*(u8**)(obj + 4) + 0x20) & 0x04000000) == 0) {
            u8* ev = (u8*)EventPost(0x7c, obj);
            if (ev != NULL) {
                *(s32*)(ev + 4) = *(s32*)(clip + 0x3c);
                *(s32*)(ev + 8) = a;
            }
        }
    } else {
        u8* e = fn_8018C48C(b, 0 /* leftover r4 key */);
        if (e != NULL) {
            void* payload = *(void**)(e + 4);
            if (payload != NULL) fn_8017493C(payload);
            *(void**)(e + 4) = NULL;
            *(s32*)(e + 0) = -1;
            fn_8018C4D8(e);
        }
        if ((*(u32*)(*(u8**)(obj + 4) + 0x20) & 0x04000000) == 0) {
            u8* ev = (u8*)EventPost(0x7c, obj);
            if (ev != NULL) {
                *(s32*)(ev + 4) = -1;
                *(s32*)(ev + 8) = a;
            }
        }
    }
    return started;
}

// 0x801E6374
// Queues an async sir load in the first free of the 8 request slots of the loader pool (+0x46C is
// the per-slot size, 0 = free): stores the two names (0x40-byte slots at +0x2C / +0x22C), hash/flags
// (+0x42C), dst (+0x44C) and size, hashes the name, finds its table entry and starts the load with
// fn_801E6934, keeping the returned handle in the table at +0x48C. Returns the handle, or
// lbl_805F969C (invalid) when all slots are busy.
s32 fn_801E6374(u8* pool, u8* name, u8* name2, u32 dst, u32 size, s32 hash) {
    s32 slot = 0;
    for (s32 n = 0; n < 8; n++) {
        if (*(s32*)(pool + 0x46c + n * 4) == 0) {
            u32 nameOff = (u32)slot << 6;
            strcpy((char*)(pool + nameOff + 0x2c), (const char*)name);
            strcpy((char*)(pool + nameOff + 0x22c), (const char*)name2);
            u32 off4 = (u32)slot * 4;
            *(s32*)(pool + off4 + 0x42c) = hash;
            *(u32*)(pool + off4 + 0x44c) = dst;
            *(u32*)(pool + off4 + 0x46c) = size;
            u8* tbl = *(u8**)(pool + 0x7b4);
            char buf[0x38];
            memset(buf, 0, sizeof(buf));
            u32 k = 0;
            u8* src = pool + nameOff + 0x2c;
            while ((s8)*src != 0 && k < 0x38) {
                buf[k] = (char)*src;
                src++;
                k++;
            }
            buf[0x37] = 0;
            u32 h = fn_80143A5C(buf);
            s32 tblIdx = 0;
            u8* ent = NULL;
            u32 count = *(u32*)(tbl + 4);
            for (u32 i = 0; i < count; i++) {
                u8* e = *(u8**)tbl + i * 0x18;
                if ((u32)hash == *(u32*)e && h == *(u32*)(e + 0xc)) {
                    u32 cnt2 = *(u32*)(tbl + 0x10);
                    tblIdx = (s32)(cnt2 + 1);
                    for (u32 j = 0; j < cnt2; j++) {
                        if (h == *(u32*)(*(u8**)(tbl + 0xc) + j * 0x40)) {
                            tblIdx = (s32)j;
                            break;
                        }
                    }
                    ent = e;
                    break;
                }
            }
            s32 csize = *(s32*)(ent + 0x10);
            if (csize == 0) csize = *(s32*)(ent + 0x8);
            void* handle = (void*)(uintptr_t)fn_801E6934(pool, *(void**)(*(u8**)(pool + 0x7a8) + tblIdx * 4), (void*)(uintptr_t)dst,
                                       (u32)csize, *(u32*)(ent + 4) >> 11, 0, 0, 0, hash);
            *(u32*)(*(u8**)(pool + 0x48c) + off4) = (u32)(uintptr_t)handle;  // lint:ok 4-byte handle slot
            return *(s32*)(*(u8**)(pool + 0x48c) + off4);
        }
        slot++;
    }
    return (s32)lbl_805F969C;
}

}  // extern "C"
