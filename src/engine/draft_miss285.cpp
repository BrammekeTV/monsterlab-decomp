// Engine functions in 0x801E86FC-0x801EC080: effect-pool list insertion (head / tail) and the
// per-frame matrix + effect-slot update of an attached-effect manager.
#include "types.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  SpawnEffectAt(void* fxMgr, u32 effect, const f32 posAndScale[4], u8** owner);  // 0x801C80E0
void  fn_801B55F4(u8* obj, u8* out, s32 idx);                    // 0x801B55F4
void  fn_801C9DF0(u8* obj, const f32* v, f32* out, f32 s);       // 0x801C9DF0
void  fn_801CA004(u8* obj, const f32* v);                        // 0x801CA004
void* fn_801DA798(void* p);                                      // 0x801DA798
void  fn_801E93EC(u8* list, u8* entry);                          // 0x801E93EC
void* memset(void* p, int c, unsigned long n);
void* memcpy(void* d, const void* s, unsigned long n);
extern u8  lbl_8047C760[];       // 0x8047C760 (effect manager)
extern u8* lbl_805FB8D8;         // 0x805FB8D8 (effect pool: 0xFF entries of 0x80 bytes, free list at +0x7F80, count at +0x837C, generation counter at +0x8380)
}

typedef u8 (*PoolCb)(u8* self, void* arg, f32 dt);
typedef void* (*VirtGetFn)(void*);

// Pool entry for a handle (low byte = slot, high byte = generation), without checking that the
// slot still carries that handle. NULL when the handle is invalid.
static u8* PoolRec(u8* pool, u16 h) {
    u32 idx = h & 0xFF;
    if (idx < 0xFF && (h & 0xFF00) != 0) return pool + (idx << 7);
    return NULL;
}

// Same, but also requires the slot to still carry exactly this handle.
static u8* PoolRecChecked(u8* pool, u16 h) {
    u32 idx = h & 0xFF;
    if (idx < 0xFF && (h & 0xFF00) != 0 && *(u16*)(pool + (idx << 7) + 0x7C) == h) return pool + (idx << 7);
    return NULL;
}

// Returns the pool slot named by e's own handle to the free list (clears the record).
static void PoolFree(u8* pool, u8* e) {
    e[0x7F] = 0x80;
    u16 h = *(u16*)(e + 0x7C);
    if (PoolRecChecked(pool, h) != NULL) {
        u8* t = pool + ((u32)(h & 0xFF) << 7);
        memset(t, 0, 0x64);
        *(s32*)(t + 0x64) = 0;
        *(s32*)(t + 0x68) = 0;
        *(s32*)(t + 0x6C) = 0;
        *(f32*)(t + 0x70) = 0.0f;
        *(u16*)(t + 0x78) = 0xFFFF;
        *(u16*)(t + 0x7A) = 0xFFFF;
        *(u16*)(t + 0x7C) = 0xFFFF;
        *(u8**)(t + 0x74) = NULL;
        t[0x7E] = 0;
        t[0x7F] = 0;
        if (*(s32*)(pool + 0x837C) + 1 != 0xFF) {
            s32 n = *(s32*)(pool + 0x837C) + 1;
            *(s32*)(pool + 0x837C) = n;
            *(s32*)(pool + n * 4 + 0x7F80) = (s32)(u8)h;
        }
    }
}

// Pool is full: scans all in-use entries and frees the stale ones (handle index does not match its
// slot, no owner list and not freshly allocated, or no longer linked in its owner list).
static void PoolReclaim(u8* pool) {
    u8* e = pool;
    for (s32 i = 0; i < 0xFF; i++) {
        u8 st = e[0x7F];
        if (st != 0) {
            u16 h = *(u16*)(e + 0x7C);
            s32 stale;
            if (i != (s32)(u8)h) {
                stale = 1;
            } else {
                u16* owner = *(u16**)(e + 0x74);
                if (owner == NULL) {
                    stale = (st != 1) ? 1 : 0;
                } else {
                    u8* self = PoolRec(lbl_805FB8D8, h);
                    s32 found = 0;
                    if (self != NULL) {
                        u8* cur = PoolRec(lbl_805FB8D8, *owner);
                        while (cur != NULL) {
                            if (cur == self) {
                                found = 1;
                                break;
                            }
                            cur = PoolRecChecked(lbl_805FB8D8, *(u16*)(cur + 0x78));
                        }
                    }
                    stale = (found == 0) ? 1 : 0;
                }
            }
            if (stale != 0) PoolFree(pool, e);
        }
        e += 0x80;
    }
}

// Takes a slot off the free list and returns its fresh handle (generation 1..0xFF in the high byte).
static u16 PoolPop(u8* pool) {
    s32 gen = *(s32*)(pool + 0x8380);
    u8* top = pool + *(s32*)(pool + 0x837C) * 4;
    s32 idx = *(s32*)(top + 0x7F80);
    *(s32*)(top + 0x7F80) = -1;
    s32 g2 = *(s32*)(pool + 0x8380) + 1;
    *(s32*)(pool + 0x8380) = g2;
    *(s32*)(pool + 0x837C) = *(s32*)(pool + 0x837C) - 1;
    if (g2 > 0xFF) *(s32*)(pool + 0x8380) = 1;
    u16 h = (u16)(((gen & 0xFF) << 8) | (idx & 0xFF));
    u8* e = pool + (idx << 7);
    *(u16*)(e + 0x7C) = h;
    e[0x7F] = 1;
    return h;
}

// Shared body of fn_801E86FC / fn_801E8D74: allocates a pool entry from the template `tmpl`
// (an effect record: 0x64 bytes of data, owner at +0x68, callback at +0x6C, time at +0x70),
// links it into the list at the head or the tail, runs its callback once, and writes the handle
// to *out (0xFFFF when no entry could be allocated).
static void PoolSpawn(u16* out, u8* list, void* owner, u8* tmpl, s32 atTail) {
    *(s32*)(list + 8) = *(s32*)(list + 8) + 1;
    u16 h = 0xFFFF;
    if (*(PoolCb*)(tmpl + 0x6C) != NULL) {
        u8* pool = lbl_805FB8D8;
        if (*(s32*)(pool + 0x837C) < 0) {
            PoolReclaim(pool);
        }
        if (*(s32*)(pool + 0x837C) >= 0) {
            h = PoolPop(pool);
        } else {
            h = 0xFFFF;
        }
    }

    u8* e = PoolRec(lbl_805FB8D8, h);
    if (e != NULL) {
        memcpy(e, tmpl, 0x64);
        *(u32*)(e + 0x64) = *(u32*)(tmpl + 0x64);
        *(u32*)(e + 0x68) = *(u32*)(tmpl + 0x68);
        *(u32*)(e + 0x6C) = *(u32*)(tmpl + 0x6C);
        *(f32*)(e + 0x70) = *(f32*)(tmpl + 0x70);
        e[0x7E] = tmpl[0x7E];
        e[0x7F] = tmpl[0x7F];
        *(u8**)(e + 0x74) = list;
    }
    if (e == NULL) {
        *out = 0xFFFF;
        *(s32*)(list + 8) = *(s32*)(list + 8) - 1;
        return;
    }

    *(void**)(e + 0x68) = owner;
    u16 link = *(u16*)(list + (atTail ? 2 : 0));
    if (PoolRec(lbl_805FB8D8, link) == NULL) {
        // list was empty
        u16 nh = *(u16*)(e + 0x7C);
        *(u16*)(list + 2) = nh;
        *(u16*)(list + 0) = nh;
    } else {
        u8* r = PoolRec(lbl_805FB8D8, link);
        if (atTail) {
            *(u16*)(r + 0x78) = *(u16*)(e + 0x7C);
            *(u16*)(e + 0x7A) = *(u16*)(r + 0x7C);
            *(u16*)(list + 2) = *(u16*)(e + 0x7C);
        } else {
            *(u16*)(r + 0x7A) = *(u16*)(e + 0x7C);
            *(u16*)(e + 0x78) = *(u16*)(r + 0x7C);
            *(u16*)(list + 0) = *(u16*)(e + 0x7C);
        }
    }
    *(u16*)(list + 4) = (u16)(*(u16*)(list + 4) + 1);
    e[0x7F] = 8;
    s32 res = 3;
    if (*(PoolCb*)(e + 0x6C) != NULL) {
        *(f32*)(e + 0x70) = *(f32*)(e + 0x70) + 0.0f;
        res = (*(PoolCb*)(e + 0x6C))(e, *(void**)(e + 0x68), 0.0f);
        e[0x7E] = (u8)res;
    }
    if (res == 3) {
        if (e[0x7F] != 0x80) {
            e[0x7F] = 0x40;
            if (*(PoolCb*)(e + 0x6C) != NULL) {
                *(f32*)(e + 0x70) = *(f32*)(e + 0x70) + 0.0f;
                e[0x7E] = (*(PoolCb*)(e + 0x6C))(e, *(void**)(e + 0x68), 0.0f);
            }
            e[0x7F] = 0x80;
        }
        if (*(u16*)(list + 6) & 1) {
            *(u16*)(list + 6) = *(u16*)(list + 6) | 2;
        } else {
            fn_801E93EC(list, e);
        }
    } else {
        e[0x7F] = 0x10;
    }
    *out = *(u16*)(e + 0x7C);
    *(s32*)(list + 8) = *(s32*)(list + 8) - 1;
}

// Result = B * M for 3x3 + translation matrices stored as 4 rows of 3 floats (M = parent/bone
// matrix, B = local matrix); translation row also adds M's translation.
static void MtxConcat43(const f32* m, const f32* b, f32* r) {
    for (s32 i = 0; i < 4; i++) {
        for (s32 j = 0; j < 3; j++) {
            f32 v = m[6 + j] * b[3 * i + 2] + (m[j] * b[3 * i] + m[3 + j] * b[3 * i + 1]);
            if (i == 3) v = m[9 + j] + v;
            r[3 * i + j] = v;
        }
    }
}

extern "C" {

// 0x801E86FC
// Allocates an effect-pool entry from the template `tmpl` and links it at the HEAD of the entry
// list `list` (head at +0, tail at +2, count at +4, flags at +6, busy counter at +8), runs its
// callback once and stores the new handle in *out (0xFFFF on failure).
void fn_801E86FC(void* out, void* list, void* owner, void* tmpl) {
    PoolSpawn((u16*)out, (u8*)list, owner, (u8*)tmpl, 0);
}

// 0x801E8D74
// Same as fn_801E86FC but appends the new entry at the TAIL of the list.
void fn_801E8D74(void* out, void* list, void* owner, void* tmpl) {
    PoolSpawn((u16*)out, (u8*)list, owner, (u8*)tmpl, 1);
}

// 0x801EC080
// Per-frame update of the attached effects of an object: arg0 = manager (count at +0, array of
// 0xC8-byte records at +4), arg1 = the owning object. For every record it computes the world
// matrix (record local matrix at +8 times the bone matrix, written to +0x38), starts pending
// effects in the 8 slots (ids at +0x68, instances at +0x88, started flags at +0xA8), and moves
// each instance's emitters to the record position and orientation.
void fn_801EC080(u8* arg0, u8* arg1) {
    for (s32 n = 0, off = 0; n < *(s32*)arg0; n++, off += 0xC8) {
        u8* rec = *(u8**)(arg0 + 4) + off;
        s32 bone = *(s32*)(rec + 4);
        if (bone >= 0) {
            f32 loc[12];
            fn_801B55F4(arg1, (u8*)loc, bone);
            MtxConcat43(loc, (const f32*)(rec + 8), (f32*)(rec + 0x38));
        } else {
            const f32* m = (const f32*)fn_801DA798((*(VirtGetFn*)(*(u8**)arg1 + 0xC))(arg1));
            MtxConcat43(m, (const f32*)(rec + 8), (f32*)(rec + 0x38));
        }

        for (s32 k = 0, ko = 0; k < 8; k++, ko += 4) {
            rec = *(u8**)(arg0 + 4) + off;
            u8* slot = rec + ko;
            if (*(u32*)(slot + 0x68) != 0 && *(u8**)(slot + 0x88) == NULL && *(s32*)(slot + 0xA8) == 0) {
                f32 pos[4];
                pos[0] = *(f32*)(rec + 0x5C);
                pos[1] = *(f32*)(rec + 0x60);
                pos[2] = *(f32*)(rec + 0x64);
                pos[3] = 0.0f;
                SpawnEffectAt(lbl_8047C760, *(u32*)(slot + 0x68), pos, (u8**)(slot + 0x88));
                *(s32*)(*(u8**)(arg0 + 4) + (ko + off) + 0xA8) = 1;
            }

            rec = *(u8**)(arg0 + 4) + off;
            u8* eff = *(u8**)(rec + ko + 0x88);
            if (eff != NULL) {
                f32 px = *(f32*)(rec + 0x5C);
                f32 py = *(f32*)(rec + 0x60);
                f32 pz = *(f32*)(rec + 0x64);
                f32 pw = 0.0f;
                u8* it = eff;
                for (s32 i = 0; i < *(s32*)(eff + 0xC); i++, it += 4) {
                    u8* child = *(u8**)(it + 0x14);
                    if (child != NULL) {
                        *(f32*)(child + 0x10) = px;
                        *(f32*)(child + 0x14) = py;
                        *(f32*)(child + 0x18) = pz;
                        *(f32*)(child + 0x1C) = pw;
                    }
                }

                rec = *(u8**)(arg0 + 4) + off;
                f32 q[4];
                f32 out[4];
                q[0] = *(f32*)(rec + 0x44);
                q[1] = *(f32*)(rec + 0x48);
                q[2] = *(f32*)(rec + 0x4C);
                q[3] = 0.0f;
                fn_801C9DF0(*(u8**)(*(u8**)(*(u8**)(arg0 + 4) + (off + ko) + 0x88) + 0x14), q, out, pz);

                rec = *(u8**)(arg0 + 4) + off;
                f32 a = *(f32*)(rec + 0x44);
                eff = *(u8**)(rec + ko + 0x88);
                f32 b = *(f32*)(rec + 0x48);
                f32 c = *(f32*)(rec + 0x4C);
                f32 v[4];
                v[0] = a;
                v[1] = b;
                v[2] = c;
                v[3] = 0.0f;
                s32 nearZero = 0;
                if (a <= 0.0001f && a >= -0.0001f && b <= 0.0001f && b >= -0.0001f &&
                    c <= 0.0001f && c >= -0.0001f && 0.0f <= 0.0001f && 0.0f >= -0.0001f) {
                    nearZero = 1;
                }
                if (nearZero != 0) {
                    // No orientation: use the +Z axis.
                    it = eff;
                    for (s32 i = 0; i < *(s32*)(eff + 0xC); i++, it += 4) {
                        if (*(u8**)(it + 0x14) != NULL) {
                            f32 axis[4];
                            axis[0] = 0.0f;
                            axis[1] = 0.0f;
                            axis[2] = 1.0f;
                            axis[3] = 0.0f;
                            fn_801CA004(*(u8**)(it + 0x14), axis);
                        }
                    }
                } else {
                    it = eff;
                    for (s32 i = 0; i < *(s32*)(eff + 0xC); i++, it += 4) {
                        u8* child = *(u8**)(it + 0x14);
                        if (child != NULL) fn_801CA004(child, v);
                    }
                }
            }
        }
    }
}

}  // extern "C"
