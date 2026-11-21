// Save-block bookkeeping / battle part helpers (0x8020xxxx, 0x8022xxxx).
#include "types.h"
#include "game/part.h"

extern "C" {
u64   __shl2i(s32, s32, s32);                                   // 0x8032F9A0
u64   __shr2u(s32, s32, s32);                                   // 0x8032F9C4
void  NoticePosted(u8* p, s32 idx);                             // 0x8022DF3C

extern const s32 lbl_80349D00[3];                               // 0x80349D00 (flag ids)
extern const s32 lbl_80349D0C[3];                               // 0x80349D0C (flag ids)

u8*   fn_80208CE8(void);                                        // 0x80208CE8
void  fn_801FFC04(u8* audio, f32 v);                            // 0x801FFC04
s32   fn_8020AB5C(const char* name, s32 flag);                  // 0x8020AB5C
void  fn_801FF2A8(u8* audio, s32 handle);                       // 0x801FF2A8 (audio voice release)
void  fn_8020B1E0(u8* voice, u8* arg);                          // 0x8020B1E0
void  fn_8020B548(void);                                        // 0x8020B548 (sort comparator)
void  fn_80336FE0(void* base, s32 n, s32 size, void* cmp);      // 0x80336FE0 (sort)
f64   fn_8033F20C(f64 x, f64 y);                                // 0x8033F20C (pow)
u32   RandomU32(void);                                          // 0x80143460
BOOL  ZoneIsLoaded(void* world, s32 zone);                      // 0x8012F408
void* fn_801B7198(void* anim, s32 a);                           // 0x801B7198
s32   fn_801FF154(u8* audio, void* desc);                       // 0x801FF154
s32   fn_801FF090(u8* audio, void* desc);                       // 0x801FF090
s32   fn_801FE52C(u8* audio, void* desc, s32 c);                // 0x801FE52C
void  fn_801FF6A4(u8* audio, s32 handle, s32 v);                // 0x801FF6A4
void  fn_801FD95C(u8* audio, f32 a, f32 b);                     // 0x801FD95C

struct ChoicePair {
    u8* p;
    u32 v;
};
s32   FighterStatsGet(u8* stats, s32 i);                        // 0x80214950
s32   PartGetBattleEnergy(const PartInstance* torso, u8** limbs);   // 0x8021ED90
s32   fn_80220974(f32 r);                                       // 0x80220974
s32   fn_8020D0E8(u8* obj);                                     // 0x8020D0E8
s32   fn_8020D788(u8* obj, s32 a);                              // 0x8020D788
s32   fn_8020DD90(u8* obj, s32 a);                              // 0x8020DD90
ChoicePair fn_8020E04C(u8* obj, s32 a);                         // 0x8020E04C
extern f32  lbl_8060055C;                                       // 0x8060055C (0.999999)

extern u8   gAudio[];                                           // audio manager
extern u8   gWorldState[];                                      // 0x803E6D00
extern u8   lbl_80559CF8[];                                     // 0x80559CF8 (music name buffer)
extern u8   lbl_80559D78[];                                     // 0x80559D78 (0x28-byte audio voice records)
extern s32  lbl_8055C578[];                                     // 0x8055C578 (voice order table, 256 entries)
extern f32  lbl_805F9970;                                       // 0x805F9970
extern f32  lbl_805F9974;                                       // 0x805F9974
extern f32  lbl_805F9978;                                       // 0x805F9978
extern f32  lbl_805F997C;                                       // 0x805F997C
extern s32  lbl_805FB6B8;                                       // 0x805FB6B8
extern u8   lbl_805FBBA8;                                       // 0x805FBBA8
extern u8   lbl_805FBBB1;                                       // 0x805FBBB1
extern u8   lbl_805FBBB3;                                       // 0x805FBBB3 (music enabled flag)
}
extern "C++" u8* gSaveSystem;                                   // 0x805FBBE0

// Bit `id` (0..255) of the 256-bit flag set at p+0x20: four big-endian 64-bit groups, the highest
// ids first (ids 0..63 at +0x38, 64..127 at +0x30, 128..191 at +0x28, 192..255 at +0x20).
static inline u8* FlagGroup(u8* p, u32 id, u32* shift) {
    if (id < 0x40) { *shift = id;        return p + 0x38; }
    if (id < 0x80) { *shift = id - 0x40; return p + 0x30; }
    if (id < 0xC0) { *shift = id - 0x80; return p + 0x28; }
    *shift = id - 0xC0;
    return p + 0x20;
}

static inline bool FlagTest(u8* p, u32 id) {
    u32 sh;
    u8* g = FlagGroup(p, id, &sh);
    return (__shr2u(*(s32*)g, *(s32*)(g + 4), (s32)sh) & 1) != 0;
}

static inline void FlagSet(u8* p, u32 id) {
    u32 sh;
    u8* g = FlagGroup(p, id, &sh);
    u64 m = __shl2i(0, 1, (s32)sh);
    *(s32*)g = (s32)(*(u32*)g | (u32)(m >> 32));
    *(s32*)(g + 4) = (s32)(*(u32*)(g + 4) | (u32)m);
}

// Queues a notice record (up to 15) of the given type and id, then calls NoticePosted.
static inline void PostNotice(u8* p, s32 type, s32 id) {
    s32 n = *(s32*)(p + 0x8E8);
    if (n < 0xF) {
        u8* rec = p + n * 0x1C;
        *(s32*)(rec + 0x744) = type;
        *(s32*)(rec + 0x748) = id;
        *(s32*)(rec + 0x74C) = 0;
        *(s32*)(rec + 0x750) = 0;
        *(s32*)(rec + 0x754) = 0;
        *(s32*)(rec + 0x758) = -1;
        s32 serial = *(s32*)(p + 0x910) + 1;
        *(s32*)(p + 0x910) = serial;
        *(s32*)(rec + 0x75C) = serial;
        *(s32*)(p + 0x8E8) = *(s32*)(p + 0x8E8) + 1;
        NoticePosted(p, n);
    }
}

static const f32 kU32ToUnit = 2.3283064e-10f;   // 1 / 2^32

// Sound description handed to the audio manager.
struct VoiceDesc {
    s32 id;           // 0x00
    u8* obj;          // 0x04
    void* owner;      // 0x08
    f32 pitch;        // 0x0C
    f32 vol;          // 0x10
    s32 arg3;         // 0x14
    s32 flags;        // 0x18
    f32 f24;          // 0x1C
    u8 b28;           // 0x20
    u8 b29;           // 0x21
    f32 minDist;      // 0x24
    f32 maxDist;      // 0x28
};

// Frees the voice (if started) and clears the 0x28-byte voice record `v`.
static inline void VoiceClear(u8* v) {
    s32 h = *(s32*)(v + 0x14);
    if (h != -1) {
        fn_801FF2A8(gAudio, h);
        *(s32*)(v + 0x14) = -1;
    }
    *(s32*)(v + 0x0) = 0;
    *(s32*)(v + 0x4) = lbl_805FB6B8;
    *(f32*)(v + 0x8) = 0.0f;
    *(f32*)(v + 0xC) = 0.0f;
    *(f32*)(v + 0x10) = 0.0f;
    *(s32*)(v + 0x14) = -1;
    *(f32*)(v + 0x18) = 0.0f;
    *(s32*)(v + 0x24) = 0;
    *(s32*)(v + 0x20) = 0;
    *(s32*)(v + 0x1C) = -1;
}

static inline f32 Clamp01(f32 v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

extern "C++" u8* gBattle;                                       // 0x805FBBB8

// Kind (+8 of the action type) of action `id`.
static inline s32 ActionKind(s32 id) {
    u8* data = gGameDb->data;
    u8* act = *(u8**)(data + 0x430) + id * 0x14;
    return *(s32*)(*(u8**)(data + 0x8) + *(s32*)(act + 4) * 0x24 + 0x8);
}

// Energy band 0..4 of a fighter: stat 3 over its torso battle energy, classified by the tuning block.
static s32 FighterEnergyBand(u8* fighter) {
    u8* torso = *(u8**)(fighter + 0x10);
    f32 ratio = (f32)FighterStatsGet(fighter + 0x20, 3);
    ratio = ratio / (f32)PartGetBattleEnergy(*(PartInstance**)torso, (u8**)fighter);
    if (fn_80220974(ratio) != 0) return 0;
    u8* tune = *(u8**)(gGameDb->data + 0x99D0);
    if (ratio < *(f32*)(tune + 0xB4)) return 1;
    if (ratio < *(f32*)(tune + 0xAC)) return 2;
    if (ratio < lbl_8060055C) return 3;
    return 4;
}

// Entry i of the candidate list at +0x34 (8 bytes each: object pointer, action id).
static inline ChoicePair ChoiceAt(u8* p, s32 i) {
    ChoicePair r;
    r.p = *(u8**)(p + 0x34 + i * 8);
    r.v = *(u32*)(p + 0x38 + i * 8);
    return r;
}

extern "C" {

// 0x8020C614
// Battle-AI pick of an entry from the candidate list (count at +0x84, entries at +0x34):
// mode 0 asks fn_8020D788/fn_8020E04C; mode 1 scores each candidate's float slot (+0xC) from the
// energy band of the opponent; mode 2 scores them from the band of the own fighter; mode 3
// returns the first candidate whose action kind is 5; anything else returns entry 0.
ChoicePair fn_8020C614(u8* arg0, s32 arg1) {
    s32 i;
    s32 n;
    switch (arg1) {
    case 0: {
        s32 pick = fn_8020D0E8(arg0);
        for (i = 0xC; i <= 0x30; i += 4) *(f32*)(arg0 + i) = 0.0f;
        if (fn_8020D788(arg0, pick) == -1) {
            ChoicePair r;
            r.p = *(u8**)(*(u8**)(gBattle + 0xA04) + 0x10);
            r.v = 0x4F;
            return r;
        }
        return fn_8020E04C(arg0, pick);
    }
    case 1: {
        s32 cntKind2 = 0;
        s32 cntKind1 = 0;
        u8* opp = *(u8**)(gBattle + 0xA00 + (1 - *(s32*)(*(u8**)arg0 + 0x44)) * 4);
        s32 band = FighterEnergyBand(opp);
        s32 sel = *(s32*)(arg0 + 0xE4 + band * 4);
        n = *(s32*)(arg0 + 0x84);
        for (i = 0; i < n; i++) {
            s32 kind = ActionKind(*(s32*)(arg0 + 0x38 + i * 8));
            *(f32*)(arg0 + 0xC + i * 4) = 0.0f;
            if (kind == 2) cntKind2++;
            else if (kind == 1) cntKind1++;
        }
        for (i = 0; i < n; i++) {
            s32 kind = ActionKind(*(s32*)(arg0 + 0x38 + i * 8));
            f32* slot = (f32*)(arg0 + 0xC + i * 4);
            if (kind == 2) {
                u8* tbl = *(u8**)(gGameDb->data + 0x18F8);
                *slot = (f32)*(s16*)(tbl + *(s32*)(arg0 + 0x8) * 0x1C + 0x16);
                *slot = *slot + (f32)*(s16*)(tbl + sel * 0x1C + 0x16);
                u8* tune = *(u8**)(gGameDb->data + 0x99D0);
                if ((f32)*(s32*)(gBattle + 0xA10) > *(f32*)(tune + 0x1D4)) {
                    *slot = *slot + (f32)*(s16*)(*(u8**)(gGameDb->data + 0x18F8) + 0x24C + 0x16);
                }
                *slot = *slot / (f32)cntKind2;
            } else if (kind == 1) {
                u8* tbl = *(u8**)(gGameDb->data + 0x18F8);
                *slot = (f32)*(s16*)(tbl + *(s32*)(arg0 + 0x8) * 0x1C + 0x18);
                *slot = *slot + (f32)*(s16*)(tbl + sel * 0x1C + 0x18);
                u8* tune = *(u8**)(gGameDb->data + 0x99D0);
                if ((f32)*(s32*)(gBattle + 0xA10) > *(f32*)(tune + 0x1D4)) {
                    *slot = *slot + (f32)*(s16*)(*(u8**)(gGameDb->data + 0x18F8) + 0x24C + 0x18);
                }
                *slot = *slot / (f32)cntKind1;
            }
        }
        return ChoiceAt(arg0, fn_8020DD90(arg0, *(s32*)(arg0 + 0x84)));
    }
    case 2: {
        s32 band = FighterEnergyBand(*(u8**)arg0);
        s32 sel = *(s32*)(arg0 + 0xF8 + band * 4);
        n = *(s32*)(arg0 + 0x84);
        for (i = 0; i < n; i++) {
            s32 kind = ActionKind(*(s32*)(arg0 + 0x38 + i * 8));
            f32* slot = (f32*)(arg0 + 0xC + i * 4);
            if (kind == 2) {
                u8* e = *(u8**)(gGameDb->data + 0x2570) + sel * 8;
                *slot = (f32)*(s8*)(e + 4);
                e = *(u8**)(gGameDb->data + 0x2570) + sel * 8;
                *slot = *slot + (f32)((s32)*(s8*)(e + 6) / 2);
            } else if (kind == 3) {
                u8* e = *(u8**)(gGameDb->data + 0x2570) + sel * 8;
                *slot = (f32)*(s8*)(e + 5);
                e = *(u8**)(gGameDb->data + 0x2570) + sel * 8;
                *slot = *slot + (f32)*(s8*)(e + 6) * 0.5f;
            } else {
                *slot = 0.0f;
            }
        }
        return ChoiceAt(arg0, fn_8020DD90(arg0, *(s32*)(arg0 + 0x84)));
    }
    case 3:
        n = *(s32*)(arg0 + 0x84);
        for (i = 0; i < n; i++) {
            if (ActionKind(*(s32*)(arg0 + 0x38 + i * 8)) == 5) return ChoiceAt(arg0, i);
        }
        return ChoiceAt(arg0, 0);
    default:
        return ChoiceAt(arg0, 0);
    }
}

// 0x8020B5E8
// Per-frame audio update: fades the music volume, updates/clears the 256 voice records (records
// bound to an unloaded zone are dropped), sorts the voice order, starts up to 64 of the active
// voices that have no handle yet (random sound variant, volume and pitch jitter), pushes their
// volumes, frees every voice past the last active one, then ticks the audio manager.
void fn_8020B5E8(u8* arg0, f32 farg0, f32 farg1) {
    s32 i;
    s32 last = 0;
    u8* obj = fn_80208CE8();
    if (*(s32*)(obj + 0x8) != 0) {
        u8* o = fn_80208CE8();
        (*(void (**)(u8*))(o + 0x8))(o);
    }
    if (lbl_805F9974 >= 0.0f) {
        f32 t = lbl_805F9974 - farg1;
        lbl_805F9974 = t;
        if (t >= 0.0f) {
            lbl_805F9978 = Clamp01(lbl_805F997C * (t / lbl_805F9970));
            fn_801FFC04(gAudio, lbl_805F9978);
        } else {
            lbl_805FBBB3 = 0;
            fn_8020AB5C((const char*)lbl_80559CF8, lbl_805FBBA8);
            lbl_805F9978 = Clamp01(lbl_805F997C);
            fn_801FFC04(gAudio, lbl_805F9978);
        }
    }
    for (i = 0; i < 0x100; i++) {
        u8* v = lbl_80559D78 + i * 0x28;
        if (*(s32*)v != 0) {
            if ((*(s32*)(v + 0x24) & 2) && lbl_805FBBB1 == 0) {
                u32 w = *(u32*)(v + 0x4);
                u32 zone = (w >> 16) & 3;
                if (zone == 3) zone = (w >> 7) & 0x1FF;
                if (ZoneIsLoaded(gWorldState, (s32)zone) == 0) {
                    VoiceClear(lbl_80559D78 + ((i | 0x0AB00000) & 0xFFFFF) * 0x28);
                    continue;
                }
            }
            fn_8020B1E0(v, arg0);
        }
    }
    fn_80336FE0(lbl_8055C578, 0x100, 4, (void*)fn_8020B548);
    for (i = 0; i < 0x40; i++) {
        s32 idx = lbl_8055C578[i];
        u8* v = lbl_80559D78 + idx * 0x28;
        if (*(f32*)(v + 0x18) != 0.0f) {
            last = i;
            if (*(s32*)(v + 0x14) == -1) {
                u8* snd = *(u8**)v;
                if (snd != NULL) {
                    f32 base = (f32)*(u8*)(snd + 6);
                    f32 jitter = (f32)*(u8*)(snd + 7);
                    u32 pick = (u32)(0.0f + (((f32)*(u8*)(snd + 4) - 0.0f) * (kU32ToUnit * (f32)RandomU32())));
                    u8* ent = snd + pick * 0x30;
                    f32 e = 2.0f + 12.0f * (base / 255.0f);
                    if (e < 2.0f) e = 2.0f;
                    else if (e > 14.0f) e = 14.0f;
                    f32 j = jitter / 255.0f;
                    f32 pw = (f32)fn_8033F20C(2.0, (f64)e) / 4096.0f;
                    f32 nj = -j;
                    s32 flags = *(s32*)(v + 0x24);
                    VoiceDesc d;
                    d.id = 0;
                    d.obj = NULL;
                    d.owner = NULL;
                    d.pitch = 1.0f;
                    d.vol = 1.0f;
                    d.arg3 = 0;
                    d.flags = 0;
                    d.f24 = 0.0f;
                    d.b28 = 0;
                    d.b29 = 0;
                    d.minDist = 5.0f;
                    d.maxDist = 30.0f;
                    d.id = *(s32*)ent;
                    d.minDist = (f32)*(u8*)(ent + 0x15);
                    d.maxDist = (f32)*(u8*)(ent + 0x16);
                    d.pitch = *(f32*)(ent + 8);
                    d.vol = pw + (nj + ((j - nj) * (kU32ToUnit * (f32)RandomU32())));
                    d.arg3 = *(s32*)(v + 0x1C);
                    d.b28 = *(u8*)(ent + 0x2D);
                    d.flags = (s32)*(u8*)(ent + 0x14);
                    d.f24 = *(f32*)(ent + 0x10);
                    d.b29 = *(u8*)(ent + 0x2E);
                    if (flags & 2) {
                        s32 key = *(s32*)(v + 0x4);
                        d.owner = fn_801B7198(&key, 0);
                        if (d.owner != NULL) {
                            *(s32*)(v + 0x14) = fn_801FF154(gAudio, &d);
                        } else {
                            VoiceClear(lbl_80559D78 + ((idx | 0x0AB00000) & 0xFFFFF) * 0x28);
                        }
                    } else if (flags & 1) {
                        d.obj = v + 8;
                        *(s32*)(v + 0x14) = fn_801FF090(gAudio, &d);
                    } else {
                        *(s32*)(v + 0x14) = fn_801FE52C(gAudio, &d, 0);
                    }
                }
            }
            s32 h = *(s32*)(v + 0x14);
            if (h != -1) fn_801FF6A4(gAudio, h, *(s32*)(v + 0x20));
        }
    }
    for (i = last + 1; i < 0x100; i++) {
        u8* v = lbl_80559D78 + lbl_8055C578[i] * 0x28;
        s32 h = *(s32*)(v + 0x14);
        if (h != -1) {
            fn_801FF2A8(gAudio, h);
            *(s32*)(v + 0x14) = -1;
        }
    }
    fn_801FD95C(gAudio, farg0, farg1);
}

// 0x80228AFC
// Adds `delta` to the progress counter at +0x9D4, then for each of three (flag id, threshold index)
// pairs: when the counter reaches the threshold and the flag is not yet set, sets it, posts a
// type-4 notice, adds the reward amount to the save block's total (capped at 99999999) and
// advances the save block's milestone index past every milestone the total now reaches.
void fn_80228AFC(u8* arg0, s32 delta) {
    s32 ids[3];
    s32 thr[3];
    s32 i;
    s32 progress = *(s32*)(arg0 + 0x9D4) + delta;
    *(s32*)(arg0 + 0x9D4) = progress;
    for (i = 0; i < 3; i++) { thr[i] = lbl_80349D00[i]; ids[i] = lbl_80349D0C[i]; }
    for (i = 2; i >= 0; i--) {
        s32 id = ids[i];
        if (FlagTest(arg0, (u8)id)) continue;
        u8* limit = *(u8**)(gGameDb->data + 0x99D0) + thr[i] * 8;
        f32 lim = *(f32*)(limit + 4);
        if (progress < (s32)lim || id == -1) continue;
        if (FlagTest(arg0, (u8)id)) continue;
        *(s32*)(arg0 + 0x120) = *(s32*)(arg0 + 0x120) + 1;
        FlagSet(arg0, (u8)id);
        PostNotice(arg0, 4, id);

        u8* reward = *(u8**)(gGameDb->data + 0x10A8) + id * 0x18;
        u8* save = gSaveSystem;
        u32 total = *(u32*)(save + 0x12C) + (u32)*(f32*)(reward + 0x10);
        *(u32*)(save + 0x12C) = total;
        if (total > 99999999u) *(u32*)(save + 0x12C) = 99999999u;
        s32 cur = *(s32*)(save + 0x128);
        if (cur < 0x31) {
            u32 n = cur + 1;
            bool advanced = false;
            u8* tbl = *(u8**)(gGameDb->data + 0x63C8);
            f32 mile = *(f32*)(tbl + n * 0x10 + 8);
            while (mile < (f32)*(u32*)(save + 0x12C) && n <= 0x31) {
                *(s32*)(save + 0x128) = (s32)n;
                n++;
                advanced = true;
                if (n <= 0x31) mile = *(f32*)(*(u8**)(gGameDb->data + 0x63C8) + n * 0x10 + 8);
            }
            if (advanced) PostNotice(save, 1, *(s32*)(save + 0x128));
        }
    }
}

}  // extern "C"
