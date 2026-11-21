// Missing functions batch miss322: audio voice start (fn_801FE52C), ...
#include <math.h>
#include <string.h>

#include "types.h"
#include "engine/math.h"

extern "C++" s32 gTextLanguage;   // 0x805F81B4

extern "C" {

BOOL  ZoneIsLoaded(void* world, s32 zone);                       // 0x8012F408
void  SleepMicroseconds(int unused, u32 us);                     // 0x80142818
void* IdPoolNext(void* pool);                                    // 0x801E6EB0
void  fn_801091EC(u8* out, u8* src, s32 lang);                   // 0x801091EC (localised name lookup)
void* fn_80109278(u8* name, void* buf, s32 size);                // 0x80109278 (load named sound, returns id)
u16   fn_801B62C8(u8* a3);                                       // 0x801B62C8
void  fn_80144F8C(u8* m, f32 angle);                              // 0x80144F8C (rotate matrix about X)
void  fn_80145094(u8* m, f32 angle);                              // 0x80145094 (rotate matrix about Y)
void  fn_801451A0(u8* m, f32 angle);                              // 0x801451A0 (rotate matrix about Z)
void  fn_801CAA10(u8* batch, s32 count, u8* p);                   // 0x801CAA10 (submit particle batch)
void  ScalarMul(f32* out, const f32* a, const f32* b);            // 0x80058214
void  ScalarSet(f32* out, f32 v);                                 // 0x80053250
void  ScalarZero(f32* out);                                       // 0x80055E2C
void  Vec3Assign(Vec3* out, const Vec3* v);                       // 0x80053354
void  Vec3Copy(Vec3* out, const Vec3* v);                         // 0x80052640
void  Vec3MulScalar(Vec3* out, const Vec3* v, const f32* s);      // 0x80053224
void  Vec3ScaleBy(Vec3* v, const f32* s);                         // 0x80054A3C
void  Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);           // 0x80053370
void  fn_80052638(Vec3* v);                                       // 0x80052638 (empty leaf)
void  fn_80054484(Vec3* out, const Vec3* a, const Vec3* b);       // 0x80054484 (a + b)
f32   fn_800544B8(f32* out, f32* a, f32* b);                      // 0x800544B8 (out = a - b)
void  fn_800544CC(u8* out);                                       // 0x800544CC (out = 1.0f)
void  fn_80054840(u8* out, u8* in);                               // 0x80054840 (copy float)
void  fn_800636A0(void);                                          // 0x800636A0 (empty leaf)
void  fn_800676E0(Vec3* out, const Vec3* v, f32* len);            // 0x800676E0 (normalize, length out)
u8*   fn_80067844(u8* e);                                         // 0x80067844 (leaf: pointer at +0x5C)
f32   fn_80082BE0(u8* out, u8* v);                                // 0x80082BE0
f32   fn_80082F20(u8* out, u8* v);                                // 0x80082F20 (copy Vec3)
void  fn_800C11A4(Vec3* out, const Vec3* a, const Vec3* b);       // 0x800C11A4 (cross product)
void  fn_80144F78(u8* out, u8* in);                               // 0x80144F78 (fabs)
void  fn_801467D4(void);                                          // 0x801467D4 (empty leaf)
f32   fn_8015EC78(u8* out, u8* in);                               // 0x8015EC78 (copy vec4)
void  fn_8015FDA4(u8* out, u8* a, u8* b);                         // 0x8015FDA4 (vec4 add)
void  fn_8015FDE8(u8* out, u8* v, f32* s);                        // 0x8015FDE8 (vec4 * scalar)
void  fn_8015FED8(u8* out, u8* a, u8* b);                         // 0x8015FED8 (vec4 sub)
void  fn_8015FFEC(u8* a, u8* b);                                  // 0x8015FFEC (a += b, vec4)
void  fn_8016EB24(u8* out, u8* in);                               // 0x8016EB24 (negate float)
void  fn_8016EB60(u8* v, u8* w);                                  // 0x8016EB60 (v.w = *w)
void  fn_8016F6F0(u8* out, u8* v);                                // 0x8016F6F0 (out = v.w)
f32   fn_8016F718(u8* out, u8* v);                                // 0x8016F718 (dot product with itself)
void  fn_801C4F30(u8* a, u8* p0, u8* p1, u8* color, f32 t);       // 0x801C4F30
void  fn_801C85E0(u8* mgr, u8* obj);                              // 0x801C85E0
void  fn_801C908C(u8* track, u8* dst, f32 t);                     // 0x801C908C
void  fn_801C9148(u8* track, u8* dst, f32 t);                     // 0x801C9148
void  fn_801C9C0C(u8* obj, f32 dt);                               // 0x801C9C0C
s32   fn_801CBCF8(u8* p, s32 bit);                                // 0x801CBCF8
u8*   fn_801CBD28(u8* e);                                         // 0x801CBD28 (leaf: e + 0x10)
s32   fn_801CBD38(u8* proto);                                     // 0x801CBD38 (leaf: word at +0x1D4)
u8*   fn_801CC8A4(u8* e);                                         // 0x801CC8A4 (leaf: node list of an emitter)
u8*   fn_801CC8AC(u8* list);                                      // 0x801CC8AC (leaf: first node, or NULL)
s32   fn_801CC8C8(u8* node, s32 bit);                             // 0x801CC8C8 (node flag bit test, flags at +0x98)
u8*   fn_801CC8E0(u8* node);                                      // 0x801CC8E0 (identity: node position)
u8*   fn_801CC8E4(u8* node);                                      // 0x801CC8E4 (leaf: node + 0x10, color)
u8*   fn_801CC8EC(u8* node);                                      // 0x801CC8EC (leaf: node + 0x20, size)
s32   fn_801CC8F4(u8* obj, s32 bit);                              // 0x801CC8F4 (flag bit test, flags at +0x1A8)
u8*   fn_801CC90C(u8* list, u8* node);                            // 0x801CC90C (leaf: next node, or NULL)
void  fn_801D0B48(u8* a, u8* b, u8* c);                           // 0x801D0B48 (kill particle)
void  fn_801CE2BC(u8* a, f32 v);                                  // 0x801CE2BC
f32   fn_801CE2C4(u8* a);                                         // 0x801CE2C4
f32   fn_801CE2CC(u8* proto);                                     // 0x801CE2CC
void  fn_801CE2D4(u8* a, s32 flag);                               // 0x801CE2D4
u32   fn_801CE2FC(u8* a);                                         // 0x801CE2FC
void  fn_801CE308(u8* a, f32 v);                                  // 0x801CE308
f32   fn_801CE310(u8* a);                                         // 0x801CE310
f32   fn_801CE318(u8* proto);                                     // 0x801CE318
f32   fn_801CE320(u8* node);                                      // 0x801CE320 (node life)
u8*   fn_801CE328(u8* node);                                      // 0x801CE328 (pointer at +0x90)
void  fn_801CE330(u8* list, u8* vec);                             // 0x801CE330
u8*   fn_801CE380(u8* node);                                      // 0x801CE380 (value at +0x94)
u8*   fn_801CE388(u8* a);                                         // 0x801CE388 (a + 0x20)
u8*   fn_801CE390(u8* a);                                         // 0x801CE390 (a + 0x30)
f32   fn_801CE398(u8* a);                                         // 0x801CE398
u8*   fn_801CE3A0(u8* proto);                                     // 0x801CE3A0 (proto + 0xBC)
f32   fn_801CE3A8(u8* node);                                      // 0x801CE3A8 (node max life)
void  fn_801CE3B0(u8* node, s32 bit);                             // 0x801CE3B0
u8*   fn_801CE3C8(u8* a);                                         // 0x801CE3C8 (a + 0x40)
void  fn_801CE3D0(u8* a, u8* b);                                  // 0x801CE3D0 (vec4 a *= b)
u8*   fn_801CE414(u8* proto);                                     // 0x801CE414 (proto + 0xFC)
u8*   fn_801CE41C(u8* a);                                         // 0x801CE41C (a + 0x50)
f32   fn_801CE424(u8* obj);                                       // 0x801CE424
f32   fn_801CE478(u8* a);                                         // 0x801CE478
u8*   fn_801CE480(u8* proto);                                     // 0x801CE480 (proto + 0x14C)
void  fn_801CE488(u8* node, u8* v);                               // 0x801CE488
u8*   fn_801CE494(u8* node);                                      // 0x801CE494 (node + 0xA4)
u8*   fn_801CE49C(u8* node);                                      // 0x801CE49C (node + 0xA8)
void  fn_801CE4A4(u8* out, u8* obj);                              // 0x801CE4A4
u8*   fn_801CE53C(u8* a);                                         // 0x801CE53C (a + 0x60)
u8*   fn_801CE544(u8* a);                                         // 0x801CE544 (a + 0x70)
void  fn_801CE54C(u8* out, u8* in, f32* lo, f32* hi);             // 0x801CE54C (per-component clamp)
f32   fn_801CE6D0(u8* proto);                                     // 0x801CE6D0
void  fn_801CE6D8(f32* out, f32* in);                             // 0x801CE6D8 (sin(pi/2 * in))
u8*   fn_801CE718(u8* proto);                                     // 0x801CE718 (proto + 0x1C)
f64   fn_801CE720(f32* out, f32* a, f32* b);                      // 0x801CE720 (min)
f32   fn_801CE740(u8* out, u8* v, u8* w);                         // 0x801CE740
void  fn_801CE764(u8* node, f32 v);                               // 0x801CE764
f64   fn_801CE76C(f32* out, f32* a, f32* b);                      // 0x801CE76C (max)
f32   fn_801CE78C(f32* out, f32* a, f32* b);                      // 0x801CE78C (a + b)
void  fn_801CE7A0(f32* out, f32* in);                             // 0x801CE7A0 (sqrt, 0 for <= 0)
void  fn_801B72A0(u8* p, u8* target);                            // 0x801B72A0
s32   fn_801FE308(u8* slot);                                     // 0x801FE308 (allocate voice for slot)
void  fn_801FF2A8(u8* audio, s32 handle);                        // 0x801FF2A8
void  fn_80202B8C(void);                                          // 0x80202B8C (voice callback)
void  fn_80203340(u8* data, s32 handle, s32 flag);               // 0x80203340
s32   fn_8020390C(u8* audio, u8* name);                          // 0x8020390C
void  fn_802095B0(u8* slot, void* a, void* b, void* c);          // 0x802095B0
s32   fn_80258024(s32 priority, void* cb, void* ctx);            // 0x80258024
s32   fn_80263FEC(u8* p, s32 handle, s32 a);                     // 0x80263FEC
s32   fn_80264020(u8* p);                                        // 0x80264020
void  fn_80264498(u8* p);                                        // 0x80264498
s32   fn_80264524(u8* p);                                        // 0x80264524
s32   fn_80264538(u8* p);                                        // 0x80264538
s32   fn_8026454C(u8* p);                                        // 0x8026454C
s32   fn_80264560(u8* p);                                        // 0x80264560
s32   fn_80264574(u8* p);                                        // 0x80264574
void  fn_80264588(u8* p);                                        // 0x80264588
void  fn_802645A8(u8* p);                                        // 0x802645A8
void  fn_802645BC(u8* p);                                        // 0x802645BC
s32   fn_80264A14(u8* p, u8* q);                                 // 0x80264A14
f64   fn_8033F204(f64 x);                                        // 0x8033F204 (natural log)

extern u8   gWorldState[];          // 0x803E6D00
extern u8   gIdPool[];              // 0x8048A8F0
extern u8   lbl_80478570[];         // 0x80478570 (camera data: axes at +0xC/+0x18/+0x30, pos +0x3C, particle batch +0x48)
extern void* lbl_805FB5F8;          // 0x805FB5F8 (current camera)
extern f32  lbl_805F95C8;           // 0x805F95C8 (150.0f)
extern u8   lbl_804FDAA0[];         // 0x804FDAA0
extern u8   lbl_8051DAD0[];         // 0x8051DAD0 (audio state block)
extern u8   lbl_8051E6A8[];         // 0x8051E6A8 (sound voices, 0x114 each)
extern u8   lbl_80524E28[];         // 0x80524E28 (sound table entries, 0x58 each, 0x60 of them)
extern f32  lbl_805F98C4;           // 0x805F98C4
extern f32  lbl_805F98C8;           // 0x805F98C8
extern f32  lbl_805F98D0;           // 0x805F98D0
extern f32  lbl_805F98D4;           // 0x805F98D4
extern f32  lbl_805F98D8;           // 0x805F98D8
extern f32  lbl_805F98DC;           // 0x805F98DC
extern s32  lbl_805FB6B8;           // 0x805FB6B8
extern s32  lbl_805FBA8C;           // 0x805FBA8C
extern void* lbl_805FBA90;          // 0x805FBA90
extern void* lbl_805FBA94;          // 0x805FBA94 (id from gIdPool)
extern s32  lbl_805FBA98;           // 0x805FBA98
extern u8   lbl_805FBAA4;           // 0x805FBAA4
extern s32  lbl_805FBAAC;           // 0x805FBAAC
extern u8   lbl_805FBAD4;           // 0x805FBAD4
extern s32  lbl_805FBAE0;           // 0x805FBAE0 (audio busy flag)
extern s32  lbl_805FBAE4;           // 0x805FBAE4 (slot serial)

// Level (x10) clamped to s16 and moved to the high half-word, as the voice params want it.
static inline s32 LevelParam(f32 level) {
    s32 t = (s32)(10.0f * level);
    s32 c;
    if (t < -0x8000) {
        c = -0x8000;
    } else if (t > 0x7FFF) {
        c = 0x7FFF;
    } else {
        c = t;
    }
    return (s32)((u32)c << 16);
}

// Linear gain -> dB-like level (30 * log2(gain)); near-zero gain gives -1000.
static inline f32 GainToLevel(f32 g) {
    if (g > -0.0001f && g < 0.0001f) return -1000.0f;
    f32 a = (f32)fn_8033F204(2.0);
    f32 b = (f32)fn_8033F204(10.0) / a;
    f32 c = (f32)fn_8033F204(2.0);
    f32 d = (f32)fn_8033F204((f64)g) / c;
    return (30.0f * d) / b;
}

// 0x801FE52C
// Starts a sound described by desc (info at +0, pos/object at +4/+8, volume +0xC, ...): validates the zone,
// queues the request if the sound data is not loaded yet (returns -1), otherwise allocates a slot in the
// sound table and a voice, configures its parameters and returns the slot id (or -1).
s32 fn_801FE52C(u8* audio, u8* desc, s32 mode) {
    u8* state = lbl_804FDAA0 + 0x20000;
    u8* info = *(u8**)(desc + 0x0);
    u8 nameBuf[0x100];
    s32 sp14, sp10, spC;
    s32 sp8;

    if (info == NULL || ZoneIsLoaded(gWorldState, (s32) * (s16*)(info + 0xE)) == 0) return -1;
    if (*(u8*)(audio + 0x2B9) != 0) return -1;
    if (*(s32*)(audio + 0x2B0) > 0 && (*(s32*)(desc + 0x8) != 0 || *(void**)(desc + 0x4) != NULL)) return -1;

    info = *(u8**)(desc + 0x0);
    if (*(void**)(info + 0x4) == NULL && *(s32*)(info + 0x0) == 0) {
        memset(nameBuf, 0, 0x100);
        fn_801091EC(nameBuf, *(u8**)(desc + 0x0) + 0x18, gTextLanguage);
        if (lbl_805FBAA4 != 0) {
            if (fn_8020390C(audio, nameBuf) == 0 && fn_8020390C(audio, *(u8**)(desc + 0x0) + 0x18) == 0) return -1;
            while (*(s32*)(lbl_8051DAD0 + 0x174) != 0) {
                SleepMicroseconds(0, 100);
            }
        } else {
            lbl_805FBA94 = fn_80109278(nameBuf, lbl_805FBA90, 0x60000);
            if (lbl_805FBA94 == IdPoolNext(gIdPool)) {
                lbl_805FBA94 = fn_80109278(*(u8**)(desc + 0x0) + 0x18, lbl_805FBA90, 0x60000);
            }
            if (lbl_805FBA94 == IdPoolNext(gIdPool)) return -1;
            if (lbl_805FBA8C != 0) {
                // Release the slot that was waiting for this load.
                s32 saved = lbl_805FBAE0;
                lbl_805FBAE0 = 1;
                for (s32 i = 0; i < 0x60; i++) {
                    if (*(u32*)(lbl_80524E28 + i * 0x58 + 0xC) == (u32)lbl_805FBA8C) {
                        fn_801FF2A8(audio, *(s32*)(lbl_80524E28 + i * 0x58));
                        break;
                    }
                }
                lbl_805FBAE0 = saved;
                lbl_805FBA8C = 0;
            }
            // Remember the request so it can be replayed once the data is loaded.
            *(void**)(state + 0x0) = *(void**)(desc + 0x0);
            *(void**)(state + 0x4) = *(void**)(desc + 0x4);
            *(s32*)(state + 0x8) = *(s32*)(desc + 0x8);
            *(f32*)(state + 0xC) = *(f32*)(desc + 0xC);
            *(f32*)(state + 0x10) = *(f32*)(desc + 0x10);
            *(s32*)(state + 0x14) = *(s32*)(desc + 0x14);
            *(s32*)(state + 0x18) = *(s32*)(desc + 0x18);
            *(f32*)(state + 0x1C) = *(f32*)(desc + 0x1C);
            *(u8*)(state + 0x20) = *(u8*)(desc + 0x20);
            *(u8*)(state + 0x21) = *(u8*)(desc + 0x21);
            *(f32*)(state + 0x24) = *(f32*)(desc + 0x24);
            *(f32*)(state + 0x28) = *(f32*)(desc + 0x28);
            lbl_805FBA98 = mode;
            return -1;
        }
    }

    s32 savedBusy = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    if (lbl_805FBAA4 != 0 && *(s32*)(desc + 0x14) == 2) {
        *(void**)(*(u8**)(desc + 0x0) + 0x4) = *(void**)(lbl_8051DAD0 + 0x154);
    }
    if (*(void**)(*(u8**)(desc + 0x0) + 0x4) == NULL) {
        lbl_805FBAE0 = savedBusy;
        return -1;
    }

    f32 scale;
    s32 kind = *(s32*)(desc + 0x14);
    if (kind == 2) {
        scale = lbl_805F98D0;
    } else if (kind < 2) {
        if (kind < 0) {
            lbl_805FBAE0 = savedBusy;
            return -1;
        }
        scale = lbl_805F98C8;
    } else if (kind >= 8) {
        lbl_805FBAE0 = savedBusy;
        return -1;
    } else if (kind >= 6) {
        scale = lbl_805F98D4;
    } else {
        scale = lbl_805F98C4;
    }

    // Find a free slot in the sound table and reset it.
    u8* slot = NULL;
    for (s32 i = 0; i < 0x60; i++) {
        u8* ent = lbl_80524E28 + i * 0x58;
        if (*(s32*)(ent + 0x10) == 0) {
            s32 serial = lbl_805FBAE4;
            slot = ent;
            *(s32*)(slot + 0x0) = -1;
            *(s32*)(slot + 0x4) = -1;
            *(s32*)(slot + 0x8) = -1;
            *(s32*)(slot + 0xC) = 0;
            *(s32*)(slot + 0x10) = 0;
            *(s32*)(slot + 0x14) = -1;
            *(s32*)(slot + 0x44) = lbl_805FB6B8;
            *(s32*)(slot + 0x4C) = 0;
            *(s32*)(slot + 0x50) = 0;
            *(s32*)(slot + 0x30) = 0;
            *(f32*)(slot + 0x34) = 0.0f;
            *(s32*)(slot + 0x54) = 0;
            *(s32*)(slot + 0x0) = (s32)(((u32)i << 16) | (u32)serial);
            lbl_805FBAE4 = serial + 1;
            if (serial > 0xFFDC) lbl_805FBAE4 = 0;
            break;
        }
    }
    if (slot == NULL) {
        lbl_805FBAE0 = savedBusy;
        return -1;
    }

    *(void**)(slot + 0xC) = *(void**)(desc + 0x0);
    s32 vol = (s32)(32768.0f * *(f32*)(desc + 0xC));
    *(s32*)(slot + 0x24) = vol;
    *(s32*)(slot + 0x28) = (s32)((f32)vol * scale);
    *(s32*)(slot + 0x8) = -1;
    *(f32*)(slot + 0x38) = *(f32*)(desc + 0x10);
    *(s32*)(slot + 0x30) = *(s32*)(desc + 0x18);
    *(f32*)(slot + 0x34) = *(f32*)(desc + 0x1C);
    if (mode == 0) {
        switch (*(s32*)(desc + 0x14)) {
        case 6: *(s32*)(slot + 0x10) = 4; break;
        case 7: *(s32*)(slot + 0x10) = 5; break;
        case 4: *(s32*)(slot + 0x10) = 6; break;
        case 5: *(s32*)(slot + 0x10) = 7; break;
        default: *(s32*)(slot + 0x10) = 1; break;
        }
    } else {
        *(s32*)(slot + 0x10) = mode;
        if (mode == 3) {
            sp8 = lbl_805FB6B8;
            fn_801B72A0((u8*)&sp8, *(u8**)(desc + 0x8));
            *(s32*)(slot + 0x44) = sp8;
            *(s32*)(slot + 0x48) = fn_801B62C8(*(u8**)(desc + 0x8));
            *(f32*)(slot + 0x3C) = *(f32*)(desc + 0x24);
            *(f32*)(slot + 0x40) = *(f32*)(desc + 0x28);
        } else if (mode == 2) {
            *(s32*)(slot + 0x54) = 0;
            f32* pos = *(f32**)(desc + 0x4);
            *(f32*)(slot + 0x18) = pos[0];
            *(f32*)(slot + 0x1C) = pos[1];
            *(f32*)(slot + 0x20) = pos[2];
            *(f32*)(slot + 0x3C) = *(f32*)(desc + 0x24);
            *(f32*)(slot + 0x40) = *(f32*)(desc + 0x28);
        }
    }
    *(s32*)(slot + 0x14) = *(s32*)(desc + 0x14);
    *(s32*)(slot + 0x4C) = 0;
    *(s32*)(slot + 0x50) = 0;
    if (*(u16*)(*(u8**)(desc + 0x0) + 0xC) != 0) {
        *(u32*)(slot + 0x50) |= 0x200;
    }

    s32 vidx = fn_801FE308(slot);
    if (vidx == -1) {
        *(s32*)(slot + 0x10) = 0;
        lbl_805FBAE0 = savedBusy;
        return -1;
    }
    u8* voice = lbl_8051E6A8 + vidx * 0x114;
    *(u8*)(voice + 0xFE) = 0;
    *(u8*)(voice + 0x105) = 0;
    *(u8*)(voice + 0xFC) = (u8)(*(u8*)(desc + 0x20) != 0);
    *(s32*)(voice + 0x110) = 0;
    *(s32*)(voice + 0x100) = 0x10;
    if ((u32)(*(s32*)(slot + 0x10) - 4) <= 1u) {
        *(s32*)(voice + 0x100) = 0x1F;
    } else if (*(s32*)(desc + 0x14) == 1) {
        *(s32*)(voice + 0x100) = 1;
    }
    s32 h = fn_80258024(*(s32*)(voice + 0x100), (void*)fn_80202B8C, voice);
    *(s32*)(voice + 0x0) = h;
    if (h == 0) {
        lbl_805FBAE0 = savedBusy;
        return -1;
    }
    *(s32*)(slot + 0x8) = vidx;
    *(u8**)(voice + 0xC) = slot;
    if (lbl_805FBAA4 != 0 && *(s32*)(desc + 0x14) == 2) {
        *(s32*)(lbl_8051DAD0 + 0x14C) = *(s32*)(voice + 0x0);
    } else {
        fn_80203340(*(u8**)(*(u8**)(desc + 0x0) + 0x4), *(s32*)(voice + 0x0), (s32)((*(u32*)(slot + 0x50) >> 9) & 1));
    }
    fn_80263FEC(voice + 0x10, *(s32*)(voice + 0x0), *(s32*)(*(u8**)(*(u8**)(desc + 0x0) + 0x4) + 0x8));
    fn_80264498(voice + 0x24);
    fn_80264A14(voice + 0x10, voice + 0x24);
    *(f32*)(voice + 0x2C) = 0.0f;
    *(f32*)(voice + 0x30) = 0.0f;
    *(f32*)(voice + 0x34) = 0.0f;
    *(f32*)(voice + 0x38) = 0.0f;
    *(s32*)(voice + 0x3C) = 1;
    fn_802095B0(slot, &sp14, &sp10, &spC);
    fn_80264538(voice + 0x68);
    fn_80264A14(voice + 0x10, voice + 0x68);
    fn_80264524(voice + 0x5C);
    fn_80264A14(voice + 0x10, voice + 0x5C);
    *(u8*)(voice + 0x104) = *(u8*)(desc + 0x21);
    fn_8026454C(voice + 0x88);
    fn_80264A14(voice + 0x10, voice + 0x88);
    fn_80264560(voice + 0x94);
    fn_80264A14(voice + 0x10, voice + 0x94);
    fn_80264574(voice + 0xA0);
    fn_80264A14(voice + 0x10, voice + 0xA0);
    if ((*(u8*)(desc + 0x21) & 1) || lbl_805FBAD4 != 0) {
        *(s32*)(voice + 0x90) = LevelParam(GainToLevel(lbl_805F98D8));
    } else {
        *(s32*)(voice + 0x90) = LevelParam(-1000.0f);
    }
    if ((*(u8*)(desc + 0x21) & 2) || lbl_805FBAD4 != 0) {
        *(s32*)(voice + 0x9C) = LevelParam(GainToLevel(lbl_805F98DC));
    } else {
        *(s32*)(voice + 0x9C) = LevelParam(-1000.0f);
    }
    *(s32*)(voice + 0xA8) = LevelParam(-1000.0f);
    fn_80264588(voice + 0x74);
    fn_80264A14(voice + 0x10, voice + 0x74);
    if (*(u8*)(desc + 0x20) != 0) {
        fn_802645A8(voice + 0xAC);
        fn_80264A14(voice + 0x10, voice + 0xAC);
        *(s16*)(voice + 0xB4) = 1;
        *(u8*)(voice + 0xFC) = 1;
        fn_802645BC(voice + 0xB8);
        fn_80264A14(voice + 0x10, voice + 0xB8);
        *(s32*)(voice + 0xC0) = LevelParam(-1000.0f);
        *(u8*)(voice + 0xFD) = 0;
    }
    f32 pitch = 10.0f * (*(f32*)(slot + 0x38) - 1.0f);
    s32 pitchParam = (s32)(1200.0f * pitch);
    *(s32*)(voice + 0x64) = (s32)((u32)pitchParam << 16);
    *(s32*)(voice + 0x80) = 0;
    *(s32*)(voice + 0x84) = 1;
    *(s32*)(voice + 0x70) = LevelParam(-1000.0f);
    *(u8*)(voice + 0xFE) = 1;
    fn_80264020(voice + 0x10);
    if (lbl_805FBAA4 != 0 && *(s32*)(desc + 0x14) == 2) {
        lbl_805FBAAC = 1;
    } else {
        *(s32*)(voice + 0x4) = 1;
    }
    *(s32*)(voice + 0x8) = 0;
    info = *(u8**)(desc + 0x0);
    if (*(s32*)(info + 0x0) == 0) {
        *(void**)(info + 0x4) = NULL;
    }
    lbl_805FBAE0 = savedBusy;
    return *(s32*)(slot + 0x0);
}

// 0x801CBD40
// Builds one 3x4 matrix + one color/uv vec4 per live particle of the effect and submits them in
// batches of 0x100 (fn_801CAA10). Particle orientation: camera aligned (flag bit 2), velocity
// aligned (bit 5) or rotated by the particle's own angles.
void fn_801CBD40(u8* arg0) {
    u8* cam = lbl_80478570;
    u8* proto = *(u8**)(arg0 + 0x5C);
    u8* link = *(u8**)(proto + 0x1D8);
    if (link == NULL || lbl_805FB5F8 == NULL) return;
    u8* batch = *(u8**)(link + 0x10);
    if (batch == NULL || *(s32*)(batch + 0x60) != 3) return;
    f32 scale = *(f32*)(batch + 0x54);
    if (scale <= 0.0001f && scale >= -0.0001f) return;
    u8* listHdr = *(u8**)(arg0 + 0x60);
    if (*(u8**)listHdr == NULL) return;

    // Direction from the camera to the effect, used to push particles along the view ray.
    f32 viewDir[3];
    f32 bias = *(f32*)(proto + 0x1B0);
    if (!(bias > -0.0001f && bias < 0.0001f)) {
        f32 dx = *(f32*)(arg0 + 0x10) - *(f32*)(cam + 0x3C);
        f32 dy = *(f32*)(arg0 + 0x14) - *(f32*)(cam + 0x40);
        f32 dz = *(f32*)(arg0 + 0x18) - *(f32*)(cam + 0x44);
        f32 len2 = dz * dz + (dx * dx + dy * dy);
        f32 inv = 1.0f / sqrtf(len2);   // original: frsqrte with two Newton steps
        viewDir[0] = dx * inv;
        viewDir[1] = dy * inv;
        viewDir[2] = dz * inv;
    } else {
        viewDir[0] = 0.0f;
        viewDir[1] = 0.0f;
        viewDir[2] = 0.0f;
    }

    u8* mtx = cam + 0x48;      // matrix batch (0x30 bytes each)
    u8* col = cam + 0x3048;    // vec4 batch (0x10 bytes each)
    s32 count = 0;
    const f32 minScale = 0.0001f;

    if (*(s32*)(arg0 + 0x74) != 0) {
        // Plane normal = axisA x axisB (the normalized copy of it is never used).
        f32 a0 = *(f32*)(cam + 0x18), a1 = *(f32*)(cam + 0x1C), a2 = *(f32*)(cam + 0x20);
        f32 b0 = *(f32*)(cam + 0x0C), b1 = *(f32*)(cam + 0x10), b2 = *(f32*)(cam + 0x14);
        *(f32*)(cam + 0x24) = a1 * b2 - a2 * b1;
        *(f32*)(cam + 0x28) = a2 * b0 - a0 * b2;
        *(f32*)(cam + 0x2C) = a0 * b1 - a1 * b0;
    }

    u8* node = *(u8**)listHdr + 8;
    while (node != NULL) {
        if (((*(u32*)(node + 0x98) >> 3) & 1) == 0) {
            u32 flags = *(u32*)(*(u8**)(arg0 + 0x5C) + 0x1A8);
            f32 px = *(f32*)(node + 0x0);
            f32 py = *(f32*)(node + 0x4);
            f32 pz = *(f32*)(node + 0x8);
            if (flags & 1) {
                // Particle positions are relative to the effect.
                px = px + *(f32*)(arg0 + 0x10);
                py = py + *(f32*)(arg0 + 0x14);
                pz = pz + *(f32*)(arg0 + 0x18);
            }
            if (*(s32*)(arg0 + 0x74) != 0) {
                // Flatten the particle onto the plane through the effect position.
                f32 dx = px - *(f32*)(arg0 + 0x10);
                f32 dy = py - *(f32*)(arg0 + 0x14);
                f32 dz = pz - *(f32*)(arg0 + 0x18);
                f32 n0 = *(f32*)(cam + 0x24), n1 = *(f32*)(cam + 0x28), n2 = *(f32*)(cam + 0x2C);
                f32 t = n2 * dz + (n0 * dx + n1 * dy);
                px = px - n0 * t;
                py = py - n1 * t;
                pz = pz - n2 * t;
            }
            if ((flags >> 2) & 1) {
                // Camera aligned: the camera axes scaled by the effect scale, then the position.
                *(f32*)(mtx + 0x00) = *(f32*)(cam + 0x0C) * scale;
                *(f32*)(mtx + 0x04) = *(f32*)(cam + 0x10) * scale;
                *(f32*)(mtx + 0x08) = *(f32*)(cam + 0x14) * scale;
                *(f32*)(mtx + 0x0C) = *(f32*)(cam + 0x30) * scale;
                *(f32*)(mtx + 0x10) = *(f32*)(cam + 0x34) * scale;
                *(f32*)(mtx + 0x14) = *(f32*)(cam + 0x38) * scale;
                *(f32*)(mtx + 0x18) = *(f32*)(cam + 0x18) * scale;
                *(f32*)(mtx + 0x1C) = *(f32*)(cam + 0x1C) * scale;
                *(f32*)(mtx + 0x20) = *(f32*)(cam + 0x20) * scale;
                *(f32*)(mtx + 0x24) = px;
                *(f32*)(mtx + 0x28) = py;
                *(f32*)(mtx + 0x2C) = pz;
            } else {
                // Diagonal scale matrix from the particle size (clamped to a minimum).
                f32 sx = *(f32*)(node + 0x20) * scale;
                f32 sy = *(f32*)(node + 0x24) * scale;
                f32 sz = *(f32*)(node + 0x28) * scale;
                if (!(sx > minScale)) sx = minScale;
                if (!(sy > minScale)) sy = minScale;
                if (!(sz > minScale)) sz = minScale;
                *(f32*)(mtx + 0x00) = sx;
                *(f32*)(mtx + 0x04) = 0.0f;
                *(f32*)(mtx + 0x08) = 0.0f;
                *(f32*)(mtx + 0x0C) = 0.0f;
                *(f32*)(mtx + 0x10) = sy;
                *(f32*)(mtx + 0x14) = 0.0f;
                *(f32*)(mtx + 0x18) = 0.0f;
                *(f32*)(mtx + 0x1C) = 0.0f;
                *(f32*)(mtx + 0x20) = sz;
                *(f32*)(mtx + 0x24) = 0.0f;
                *(f32*)(mtx + 0x28) = 0.0f;
                *(f32*)(mtx + 0x2C) = 0.0f;
                if ((*(u32*)(*(u8**)(arg0 + 0x5C) + 0x1A8) >> 5) & 1) {
                    // Velocity aligned: basis (a, v, c) built around the velocity direction v,
                    // multiplied into the scale matrix (the translation row is rewritten below).
                    f32 vx = *(f32*)(node + 0x30), vy = *(f32*)(node + 0x34), vz = *(f32*)(node + 0x38);
                    f32 vlen2 = vz * vz + (vx * vx + vy * vy);
                    f32 v0, v1, v2;
                    if (vlen2 == 0.0f) {
                        v0 = 0.0f;
                        v1 = 0.0f;
                        v2 = 0.0f;
                    } else {
                        f32 vinv = 1.0f / sqrtf(vlen2);   // original: frsqrte with two Newton steps
                        v0 = vx * vinv;
                        v1 = vy * vinv;
                        v2 = vz * vinv;
                    }
                    // a = v x (0,0,1), c = a x v
                    f32 a0 = v1 * 1.0f - v2 * 0.0f;
                    f32 a1 = v2 * 0.0f - v0 * 1.0f;
                    f32 a2 = v0 * 0.0f - v1 * 0.0f;
                    f32 c0 = a1 * v2 - a2 * v1;
                    f32 c1 = a2 * v0 - a0 * v2;
                    f32 c2 = a0 * v1 - a1 * v0;
                    f32 s0 = *(f32*)(mtx + 0x00), s1 = *(f32*)(mtx + 0x04), s2 = *(f32*)(mtx + 0x08);
                    f32 s3 = *(f32*)(mtx + 0x0C), s4 = *(f32*)(mtx + 0x10), s5 = *(f32*)(mtx + 0x14);
                    f32 s6 = *(f32*)(mtx + 0x18), s7 = *(f32*)(mtx + 0x1C), s8 = *(f32*)(mtx + 0x20);
                    *(f32*)(mtx + 0x00) = c0 * s2 + (a0 * s0 + v0 * s1);
                    *(f32*)(mtx + 0x04) = c1 * s2 + (a1 * s0 + v1 * s1);
                    *(f32*)(mtx + 0x08) = c2 * s2 + (a2 * s0 + v2 * s1);
                    *(f32*)(mtx + 0x0C) = c0 * s5 + (a0 * s3 + v0 * s4);
                    *(f32*)(mtx + 0x10) = c1 * s5 + (a1 * s3 + v1 * s4);
                    *(f32*)(mtx + 0x14) = c2 * s5 + (a2 * s3 + v2 * s4);
                    *(f32*)(mtx + 0x18) = c0 * s8 + (a0 * s6 + v0 * s7);
                    *(f32*)(mtx + 0x1C) = c1 * s8 + (a1 * s6 + v1 * s7);
                    *(f32*)(mtx + 0x20) = c2 * s8 + (a2 * s6 + v2 * s7);
                } else {
                    // Rotate by the particle's own angles (scaled by 2/pi).
                    fn_80144F8C(mtx, 0.63661975f * *(f32*)(node + 0x60));
                    fn_80145094(mtx, 0.63661975f * *(f32*)(node + 0x64));
                    fn_801451A0(mtx, 0.63661975f * *(f32*)(node + 0x68));
                }
                f32 bias2 = *(f32*)(*(u8**)(arg0 + 0x5C) + 0x1B0);
                if (!(bias2 > -0.0001f && bias2 < 0.0001f)) {
                    px = px + viewDir[0] * bias2;
                    py = py + viewDir[1] * bias2;
                    pz = pz + viewDir[2] * bias2;
                }
                *(f32*)(mtx + 0x24) = px;
                *(f32*)(mtx + 0x28) = py;
                *(f32*)(mtx + 0x2C) = pz;
            }
            count++;
            mtx += 0x30;
            *(f32*)(col + 0x0) = *(f32*)(node + 0x10);
            *(f32*)(col + 0x4) = *(f32*)(node + 0x14);
            *(f32*)(col + 0x8) = *(f32*)(node + 0x18);
            *(f32*)(col + 0xC) = *(f32*)(node + 0x1C);
            col += 0x10;
            if (count >= 0x100) {
                fn_801CAA10(batch, count, link + 0x3C);
                mtx = cam + 0x48;
                col = cam + 0x3048;
                count = 0;
            }
        }
        u8* next = *(u8**)(node - 4);
        node = next != NULL ? next + 8 : NULL;
    }
    if (count > 0) {
        fn_801CAA10(batch, count, link + 0x3C);
    }
}

// 0x801CD790
// Per-frame update of one particle emitter (obj) owned by the manager mgr: ages the emitter, then walks its
// particle list, removing dead particles, integrating position/velocity/color/size/spin with the
// tweened keyframes, applying drag and the swirl force, and finally publishes the maximum extent
// of the particles into the emitter (obj+0x1C).
void fn_801CD790(u8* obj, u8* mgr, f32 dt) {
    f32 zero74;       // running maximum of the squared extent (starts at 0)
    f32 dtScalar;     // dt wrapped in a scalar for the vector helpers
    u8* node;

    ScalarZero(&zero74);
    fn_801CE2BC(obj, fn_801CE2C4(obj) - dt);
    if (fn_801CE2CC(*(u8**)(obj + 0x5C)) > 0.0f && fn_801CE2C4(obj) < 0.0f) {
        fn_801CE2D4(obj, 1);
    }
    if (fn_801CE2FC(obj) != 0 && fn_801CC8AC(fn_801CC8A4(obj)) == NULL) {
        fn_801C85E0(mgr, obj);
        return;
    }
    fn_801CE308(obj, fn_801CE310(obj) - dt);
    if (fn_801CE2FC(obj) == 0 && (fn_801CBCF8(obj, 0) != 0 || fn_801CC8F4(*(u8**)(obj + 0x5C), 7) != 0)) {
        fn_801C9C0C(obj, dt);
    }
    ScalarSet(&dtScalar, dt);
    node = fn_801CC8AC(fn_801CC8A4(obj));
    while (node != NULL) {
        // Drop particles whose life ran out (only when the emitter has a life limit).
        if (fn_801CE318(fn_80067844(obj)) > 0.0f) {
            while (node != NULL && fn_801CE320(node) < 0.0f) {
                u8* next = fn_801CC90C(fn_801CC8A4(obj), node);
                fn_801D0B48(obj, node, mgr);
                node = next;
            }
        }
        if (node == NULL) break;

        // Particle attached to child objects: move their color slot to the particle position.
        if (fn_801CE328(node) != NULL) {
            if (fn_801CC8F4(*(u8**)(obj + 0x5C), 0) != 0) {
                f32 sp240[4];
                u8* p = fn_801CC8E0(node);
                fn_8015FDA4((u8*)sp240, fn_801CBD28(obj), p);
                fn_801CE330(fn_801CE328(node), (u8*)sp240);
            } else {
                u8* p = fn_801CC8E0(node);
                fn_801CE330(fn_801CE328(node), p);
            }
        }
        // Particle with a trail: hand its two end points and color to the trail builder.
        if (fn_801CE380(node) != NULL) {
            f32 sp2A8[4];
            f32 sp298[4];
            fn_801467D4();
            fn_801467D4();
            if (fn_801CC8F4(*(u8**)(obj + 0x5C), 0) != 0) {
                f32 sp230[4], sp220[4], sp210[4];
                u8* p = fn_801CC8E0(node);
                fn_8015FDA4((u8*)sp230, fn_801CBD28(obj), p);
                fn_8015EC78((u8*)sp2A8, (u8*)sp230);
                u8* v = fn_801CE390(node);
                u8* w = fn_801CE388(obj);
                fn_8015FED8((u8*)sp210, fn_801CBD28(obj), w);
                fn_8015FDA4((u8*)sp220, (u8*)sp210, v);
                fn_8015EC78((u8*)sp298, (u8*)sp220);
            } else {
                fn_8015EC78((u8*)sp2A8, fn_801CC8E0(node));
                fn_8015EC78((u8*)sp298, fn_801CE390(node));
            }
            f32 trailT = *(f32*)(*(u8**)(obj + 0x5C) + 0x1F0);
            u8* color = fn_801CC8E4(node);
            fn_801C4F30(fn_801CE380(node), (u8*)sp2A8, (u8*)sp298, color, trailT);
        }

        // Keyframe tweening: when the particle's life crosses the next key time of the color (bit 0),
        // size (bit 1) and spin (bit 2) tracks, step the track forward.
        if (fn_801CC8C8(node, 0) == 0) {
            f32 t = (1.0f - fn_801CE398(fn_801CE3A0(*(u8**)(obj + 0x5C)))) * fn_801CE3A8(node);
            if (fn_801CE320(node) < t) {
                fn_801CE3B0(node, 0);
                f32 life = fn_801CE3A8(node);
                u8* dst = fn_801CE3C8(node);
                fn_801C908C(fn_801CE3A0(*(u8**)(obj + 0x5C)), dst, life);
                fn_801CE3D0(fn_801CE3C8(node), *(u8**)(obj + 0x70) + 4);
            }
        }
        if (fn_801CC8C8(node, 1) == 0) {
            f32 t = (1.0f - fn_801CE398(fn_801CE414(*(u8**)(obj + 0x5C)))) * fn_801CE3A8(node);
            if (fn_801CE320(node) < t) {
                f32 sp6C, sp50;
                fn_801CE3B0(node, 1);
                f32 life = fn_801CE3A8(node);
                u8* dst = fn_801CE41C(node);
                fn_801C908C(fn_801CE414(*(u8**)(obj + 0x5C)), dst, life);
                fn_8016F6F0((u8*)&sp6C, fn_801CE41C(node));
                ScalarSet(&sp50, fn_801CE424(obj));
                Vec3ScaleBy((Vec3*)fn_801CE41C(node), &sp50);
                fn_8016EB60(fn_801CE41C(node), (u8*)&sp6C);
            }
        }
        if (fn_801CC8C8(node, 2) == 0) {
            f32 t = (1.0f - fn_801CE478(fn_801CE480(*(u8**)(obj + 0x5C)))) * fn_801CE3A8(node);
            if (fn_801CE320(node) < t) {
                f32 sp68;
                fn_801CE3B0(node, 2);
                f32 life = fn_801CE3A8(node);
                fn_801C9148(fn_801CE480(*(u8**)(obj + 0x5C)), (u8*)&sp68, life);
                fn_801CE488(node, (u8*)&sp68);
            }
        }

        // Integrate: color += colorRate*dt, size += sizeRate*dt, spin, velocity += gravity*dt, drag.
        {
            f32 sp200[4], sp1F0[4];
            fn_8015FDE8((u8*)sp200, fn_801CE3C8(node), &dtScalar);
            fn_8015FFEC(fn_801CC8E4(node), (u8*)sp200);
            fn_8015FDE8((u8*)sp1F0, fn_801CE41C(node), &dtScalar);
            fn_8015FFEC(fn_801CC8EC(node), (u8*)sp1F0);
        }
        {
            f32 spinStep = dt * *(f32*)fn_801CE49C(node);
            f32* spin = (f32*)fn_801CE494(node);
            *spin = *spin + spinStep;
        }
        {
            f32 sp1D0[4], sp1E0[4], sp48, sp4C;
            ScalarSet(&sp48, fn_801CE424(obj));
            ScalarMul(&sp4C, &sp48, &dtScalar);
            fn_801CE4A4((u8*)sp1D0, obj);
            fn_8015FDE8((u8*)sp1E0, (u8*)sp1D0, &sp4C);
            fn_8015FFEC(fn_801CE390(node), (u8*)sp1E0);
        }
        if (fn_801CBD38(*(u8**)(obj + 0x5C)) == 0) {
            f32 sp1C0[4];
            fn_8015FDE8((u8*)sp1C0, fn_801CE544(node), &dtScalar);
            fn_8015FFEC(fn_801CE53C(node), (u8*)sp1C0);
        }
        {
            f32 sp40, sp44, sp1B0[4];
            ScalarSet(&sp40, 1.0f);
            ScalarSet(&sp44, 0.0f);
            fn_801CE54C((u8*)sp1B0, fn_801CC8E4(node), &sp44, &sp40);   // clamp the color to 0..1
            fn_8015EC78(fn_801CC8E4(node), (u8*)sp1B0);
        }

        // Swirl force around the emitter axis, applied to the velocity.
        if (fn_801CE6D0(*(u8**)(obj + 0x5C)) != 0.0f) {
            f32 sp64, sp38, sp3C, sp34, sp2C, sp30, sp28;
            Vec3 sp28C, sp280, sp274;
            ScalarSet(&sp64, 0.63661975f * (0.005f * fn_801CE6D0(*(u8**)(obj + 0x5C))));
            fn_801CE6D8(&sp38, &sp64);
            fn_80144F78((u8*)&sp3C, (u8*)&sp38);
            fn_80054840((u8*)&sp64, (u8*)&sp3C);
            fn_80052638(&sp28C);
            fn_80052638(&sp280);
            fn_80052638(&sp274);
            if (fn_801CE6D0(*(u8**)(obj + 0x5C)) < 0.0f) {
                fn_8016EB24((u8*)&sp34, (u8*)&sp64);
                fn_80054840((u8*)&sp64, (u8*)&sp34);
            }
            if (fn_801CC8F4(fn_80067844(obj), 0) != 0) {
                Vec3 sp1A4;
                fn_80082F20((u8*)&sp1A4, fn_801CC8E0(node));
                Vec3Copy(&sp28C, &sp1A4);
            } else {
                Vec3 sp188, sp198;
                u8* origin = fn_801CBD28(obj);
                fn_8015FED8((u8*)&sp188, fn_801CC8E0(node), origin);
                fn_80082F20((u8*)&sp198, (u8*)&sp188);
                Vec3Copy(&sp28C, &sp198);
            }
            if (fn_801CC8F4(*(u8**)(obj + 0x5C), 3) != 0) {
                Vec3 sp170, sp17C;
                fn_80082F20((u8*)&sp170, obj + 0x30);
                fn_800C11A4(&sp17C, &sp170, &sp28C);
                Vec3Copy(&sp280, &sp17C);
            } else {
                Vec3 sp158, sp164;
                fn_80082F20((u8*)&sp158, fn_801CE718(*(u8**)(obj + 0x5C)));
                fn_800C11A4(&sp164, &sp158, &sp28C);
                Vec3Copy(&sp280, &sp164);
            }
            {
                Vec3 sp14C, sp128, sp134, sp140, spF0, spFC, sp108;
                f32 sp118[4];
                fn_800C11A4(&sp14C, (Vec3*)fn_801CE718(*(u8**)(obj + 0x5C)), &sp280);
                Vec3Copy(&sp28C, &sp14C);
                fn_800544CC((u8*)&sp2C);
                fn_800544B8(&sp30, &sp2C, &sp64);
                Vec3MulScalar(&sp128, &sp280, &sp30);
                Vec3MulScalar(&sp134, &sp28C, &sp64);
                fn_80054484(&sp140, &sp134, &sp128);
                Vec3Copy(&sp274, &sp140);
                ScalarSet(&sp28, lbl_805F95C8);
                Vec3MulScalar(&spF0, &sp274, &sp64);
                Vec3MulScalar(&spFC, &spF0, &sp28);
                Vec3MulScalar(&sp108, &spFC, &dtScalar);
                fn_80082BE0((u8*)sp118, (u8*)&sp108);
                fn_8015FFEC(fn_801CE390(node), (u8*)sp118);
            }
        }
        {
            f32 spE0[4];
            fn_8015FDE8((u8*)spE0, fn_801CE390(node), &dtScalar);
            fn_8015FFEC(fn_801CC8E0(node), (u8*)spE0);
        }

        // Drag: remove up to (drag * dt * |v|^2) from the speed along the velocity direction.
        if (*(f32*)fn_801CE494(node) > 0.0f) {
            f32 sp60, sp5C, sp1C, sp20, sp24, sp58, sp18, spA8[4];
            Vec3 sp268, sp25C, spD0, spB8, spC4;
            fn_8016F6F0((u8*)&sp60, fn_801CE390(node));
            fn_80082F20((u8*)&sp268, fn_801CE390(node));
            Vec3Assign(&sp25C, &sp268);
            fn_800636A0();
            fn_800676E0(&spD0, &sp25C, &sp5C);
            Vec3Copy(&sp25C, &spD0);
            fn_8016F718((u8*)&sp1C, (u8*)&sp268);
            ScalarSet(&sp20, *(f32*)fn_801CE494(node));
            ScalarMul(&sp24, &sp20, &dtScalar);
            ScalarMul(&sp58, &sp24, &sp1C);
            fn_801CE720(&sp18, &sp58, &sp5C);
            fn_80054840((u8*)&sp58, (u8*)&sp18);
            Vec3MulScalar(&spB8, &sp25C, &sp58);
            Vec3Sub(&spC4, &sp268, &spB8);
            Vec3Copy(&sp268, &spC4);
            fn_801CE740((u8*)spA8, (u8*)&sp268, (u8*)&sp60);
            fn_8015EC78(fn_801CE390(node), (u8*)spA8);
        }

        // Age the particle and track the maximum squared distance from the emitter.
        if (fn_801CE318(fn_80067844(obj)) > 0.0f) {
            fn_801CE764(node, fn_801CE320(node) - dt);
        }
        {
            Vec3 sp250;
            f32 sp54, sp14;
            fn_80052638(&sp250);
            if (fn_801CC8F4(*(u8**)(obj + 0x5C), 0) != 0) {
                Vec3 sp9C;
                fn_80082F20((u8*)&sp9C, node);
                Vec3Copy(&sp250, &sp9C);
            } else {
                Vec3 sp78, sp84, sp90;
                fn_80082F20((u8*)&sp78, obj + 0x10);
                fn_80082F20((u8*)&sp84, node);
                Vec3Sub(&sp90, &sp84, &sp78);
                Vec3Copy(&sp250, &sp90);
            }
            fn_8016F718((u8*)&sp54, (u8*)&sp250);
            fn_801CE76C(&sp14, &zero74, &sp54);
            fn_80054840((u8*)&zero74, (u8*)&sp14);
        }
        node = fn_801CC90C(fn_801CC8A4(obj), node);
        *(s32*)(mgr + 0x8) = *(s32*)(mgr + 0x8) + 1;
    }
    {
        f32 sp8, spC, sp10;
        ScalarSet(&sp8, *(f32*)(*(u8**)(obj + 0x5C) + 0x1E4));
        fn_801CE7A0(&spC, &zero74);
        fn_801CE78C(&sp10, &spC, &sp8);
        fn_8016EB60(obj + 0x10, (u8*)&sp10);
    }
}

}  // extern "C"
