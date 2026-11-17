// Character movement: response to one collision contact. Handles standing on moving
// platforms, bouncing off pushable entities, sliding along walls, and landing on top of
// boxes (box and oriented-box colliders snap the character onto the top face).
// Original: CharacterOnContact 0x800575E8 (auto_03_80052504_text.s)
#include <stddef.h>

#include "types.h"
#include "engine/math.h"

typedef f32 Scalar;

extern "C" {
void   Vec3Copy(Vec3* out, const Vec3* v);                       // 0x80052640
void   Vec3SetZero(Vec3* out);                                   // 0x800531A8
void   Vec3MulScalar(Vec3* out, const Vec3* v, const Scalar* s); // 0x80053224
void   Vec3Negate(Vec3* out, const Vec3* v);                     // 0x80053324
void   Vec3Assign(Vec3* out, const Vec3* v);                     // 0x80053354
void   Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);         // 0x80053370
void   Vec3AddTo(Vec3* v, const Vec3* d);                        // 0x800544FC
void   Vec3SubFrom(Vec3* v, const Vec3* d);                      // 0x8005461C
void   Vec3SetUnitZ(Vec3* v);                                    // 0x800549A8
void   ScalarZero(Scalar* out);                                  // 0x80055E2C
BOOL   ScalarLess(const Scalar* a, const Scalar* b);             // 0x800533B0
void   Vec3Dot(Scalar* out, const Vec3* a, const Vec3* b);       // 0x800549C0
void   Vec3ScaleBy(Vec3* v, const Scalar* s);                    // 0x80054A3C
f32    Vec3LengthSqOf(const Vec3* v);                            // 0x80054A90
void   Vec3NormalizeTo(Vec3* out, const Vec3* v);                // 0x80055D9C
void   Vec3Length(Scalar* out, const Vec3* v);                   // 0x80057128
void   Vec3ToLocal(Vec3* out, const Vec3* p, const Vec3* origin, const f32 m[9]);   // 0x8005824C
void   Vec3RotateNormalized(Vec3* out, const Vec3* v, const f32 m[9]);              // 0x80058324
void   Vec3Rotate(Vec3* out, const f32 m[9], const Vec3* v);                        // 0x8005844C

void   CharacterGetTotalVelocity(Vec3* out, u8* c);              // 0x800571A0: +0x844 + +0x850
u8*    CharacterGetGround(u8* c);                                // 0x8005A520
void   CharacterLeaveGround(u8* c);                              // 0x8005A420
void   CharacterSetGround(u8* c, u8* entity, u8* collider);      // 0x8005A304
void   CharacterSetFlags(u8* c, u32 flags);                      // 0x800532A0
void   CharacterSetMoved(u8* c);                                 // 0x800533C8
void   CharacterSetRotated(u8* c);                               // 0x800533EC
void   CharacterSetFallTimer(u8* c, f32 t);                      // 0x800584C8
void   CharacterResetGroundState(u8* c, s32 a, s32 b);           // 0x8005BAD4
void*  PhysicsBodyGet(void* handle);                             // 0x801B1840
void   RayQueryInit(u8* q);                                      // 0x801B11DC
void   RayQueryDestroy(u8* q, s32 del);                          // 0x80055C80
void   RayHitInit(u8* hit);                                      // 0x80055CD8
void   RayQuerySetStart(u8* q, const Vec3* p);                   // 0x80055D64
void   RayQuerySetDir(u8* q, const Vec3* d);                     // 0x80055D80
BOOL   RayQueryCast(u8* q, void* world, u32 filter, u8* hit, u8* hit2);   // 0x801B203C
void   EventSend(void* mgr, void* from, void* to, u32 id, s32 a, s32 b, s32 c, s32 d);
}
extern void* gEventMgr;     // 0x805FB6C8

// Contact record.
#define CT_NORMAL(ct)    ((Vec3*)((ct) + 0x0C))
#define CT_DEPTH(ct)     (*(f32*)((ct) + 0x18))
#define CT_ENTITY(ct)    (*(u8**)((ct) + 0x5C))
#define CT_COLLIDER(ct)  (*(u8**)((ct) + 0x68))
// Collider: +0x0C shape, +0x14 centre, +0x20 previous centre, +0x3C rotation (box: extents),
// +0x6C extents (oriented box).
#define COL_SHAPE(col)   (*(s32*)((col) + 0x0C))
#define COL_CENTER(col)  ((Vec3*)((col) + 0x14))
#define COL_PREV(col)    ((Vec3*)((col) + 0x20))
enum { SHAPE_PLANE = 0, SHAPE_BOX = 4, SHAPE_OBB = 7 };

// Character fields.
#define C_POS(c)         (*(Vec3**)((c) + 0x04))
#define C_MASS(c)        (*(f32*)((c) + 0x5F4))
#define C_MOVE_MODE(c)   (*(s32*)((c) + 0x78C))
#define C_GROUND_NORMAL(c) ((Vec3*)((c) + 0x7A4))
#define C_GROUND_COL(c)  (*(u8**)((c) + 0x7B4))
#define C_GROUND_OFS(c)  ((Vec3*)((c) + 0x7B8))
#define C_MOVE_FLAGS(c)  (*(u32*)((c) + 0x838))
#define C_VELOCITY(c)    ((Vec3*)((c) + 0x844))
#define C_PUSH(c)        ((Vec3*)((c) + 0x850))
#define E_BODY(e)        (*(void**)((e) + 0x8C))

static inline BOOL EntityHasFlag(u8* e, s32 bit) { return (*(u32*)(*(u8**)(e + 4) + 0x1C) & (1u << bit)) != 0; }
static inline BOOL EntityStateHas(u8* e, u32 mask) { return (*(u32*)(*(u8**)(e + 4) + 0x20) & mask) != 0; }
static inline u32  EntityFlags2(u8* e) { return *(u32*)(e + 0x15C); }

typedef void (*PushFn)(u8*, u8*, s32, s32, s32, f32);
typedef BOOL (*BoolFn)(u8*);
#define VT(o, off) (*(void**)(*(u8**)(o) + (off)))

// Clamp v into [-e, e] with the original's comparison order (works for negative extents too).
static inline f32 ClampExtent(f32 v, f32 e) {
    if (-e < e) {
        if (v < -e) return -e;
        if (v > e) return e;
        return v;
    }
    if (v < e) return e;
    if (v > -e) return -e;
    return v;
}

// Move the body attached to the character's physics handle along with it.
static inline void NudgeBody(u8* c, const Vec3* d) {
    void* body = E_BODY(c);
    if (body != NULL && PhysicsBodyGet(body) != NULL) {
        Vec3AddTo((Vec3*)((u8*)PhysicsBodyGet(body) + 0x14), d);
    }
}

// 0x800575E8
BOOL CharacterOnContact(u8* c, u8* ct) {
    Vec3 vel;
    CharacterGetTotalVelocity(&vel, c);
    u8* other = CT_ENTITY(ct);
    if (other == NULL) return TRUE;

    // Still standing on the same collider of the same entity: ignore while it moves.
    u8* ground = CharacterGetGround(c);
    if (ground == NULL || E_BODY(ground) == NULL) {
        CharacterLeaveGround(c);
        ground = NULL;
    }
    if (ground != NULL && ground == other && C_GROUND_COL(c) == CT_COLLIDER(ct)) {
        Vec3 d;
        Vec3Sub(&d, COL_CENTER(C_GROUND_COL(c)), C_GROUND_OFS(c));
        if (*(s32*)(other + 0xA0) != 0 && (d.z != 0.0f || d.x != 0.0f || d.y != 0.0f)) return TRUE;
    }

    // Walking into a pushable entity.
    if (C_MOVE_MODE(c) == 1 && (C_MOVE_FLAGS(c) & 0x800) && !EntityStateHas(c, 0x04000000) &&
        EntityHasFlag(other, 6)) {
        if (!(EntityFlags2(other) & 0x20) && *(s32*)(other + 0x18C) == 0)
            ((PushFn)VT(other, 0x50))(other, c, 0, 0, 0, 0.1f * C_MASS(c));
        if (!((BoolFn)VT(other, 0x4C))(other)) {
            EventSend(gEventMgr, c, c, 0x1BE2FFCB, 0, 0, 1, 0);
        } else {
            Scalar half = -0.5f;
            Vec3ScaleBy(C_VELOCITY(c), &half);
            Vec3ScaleBy(C_PUSH(c), &half);
        }
    }

    // Push out along the contact normal.
    Scalar depth = CT_DEPTH(ct), margin = 1.001f;
    Vec3 out, step;
    Vec3MulScalar(&out, CT_NORMAL(ct), &depth);
    Vec3MulScalar(&step, &out, &margin);
    Vec3AddTo(C_POS(c), &step);
    if (E_BODY(c) != NULL && PhysicsBodyGet(E_BODY(c)) != NULL) {
        Vec3MulScalar(&step, &out, &margin);
        NudgeBody(c, &step);
    }
    CharacterSetMoved(c);
    CharacterSetRotated(c);

    // Carried along by a moving solid entity.
    if (CharacterGetGround(c) != other) {
        Vec3 v;
        CharacterGetTotalVelocity(&v, c);
        u8* col = CT_COLLIDER(ct);
        if (Vec3LengthSqOf(&v) < 64.0f && C_GROUND_COL(c) != col && EntityHasFlag(other, 6) &&
            (EntityFlags2(other) & 0x40000)) {
            Vec3 move;
            Vec3Sub(&move, COL_CENTER(col), COL_PREV(col));
            if (Vec3LengthSqOf(&move) > 0.010000001f) {
                Vec3 away, dir;
                Vec3Sub(&away, C_POS(c), COL_CENTER(CT_COLLIDER(ct)));
                away.z = 0.0f;
                Vec3NormalizeTo(&dir, &away);
                Vec3Copy(&away, &dir);
                away.z = 0.1f;
                Scalar len, six = 6.0f, push;
                Vec3Length(&len, &move);
                push = len * six;
                Vec3ScaleBy(&away, &push);
                Vec3AddTo(C_VELOCITY(c), &away);
            }
        }
    }

    BOOL solid = EntityHasFlag(other, 6) && (EntityFlags2(other) & 0x40000);
    if (!solid) CharacterSetFlags(other, 8);

    // Landing on top of something: cast down to check there is floor under the character.
    if (EntityStateHas(other, 1) && EntityStateHas(other, 8) && CT_NORMAL(ct)->z > 0.7071f && vel.z < 0.0f) {
        Vec3 savedVel;
        Vec3SetZero(&savedVel);
        u8 query[0x6C], hit[0x6C], hit2[0x6C];
        RayQueryInit(query);
        RayHitInit(hit);
        RayHitInit(hit2);
        RayQuerySetStart(query, C_POS(c));
        Vec3 down, up;
        Vec3SetUnitZ(&up);
        Vec3Negate(&down, &up);
        RayQuerySetDir(query, &down);

        BOOL landed = TRUE;
        if (!RayQueryCast(query, *(void**)(other + 0x8C), 0xF62113D5, hit, hit2) || *(f32*)(hit + 0x18) > 0.1f) {
            landed = FALSE;
            u8* col = CT_COLLIDER(ct);
            Vec3 rel, clamped;
            Vec3Sub(&rel, C_POS(c), COL_CENTER(col));
            Vec3SetZero(&clamped);
            switch (COL_SHAPE(col)) {
            case SHAPE_PLANE: {
                Vec3 n;
                rel.z = 0.0f;
                Vec3NormalizeTo(&n, &rel);
                Vec3Copy(&rel, &n);
                break;
            }
            case SHAPE_BOX: {
                // Push away from the nearest side face of an axis-aligned box.
                const Vec3* ext = (const Vec3*)(col + 0x3C);
                Vec3Copy(&clamped, &rel);
                clamped.x = ClampExtent(clamped.x, ext->x);
                clamped.y = ClampExtent(clamped.y, ext->y);
                clamped.z = ext->z;
                f32 ry = fabsf(rel.y / ext->y);
                if (fabsf(rel.x / ext->x) > ry) rel.y = 0.0f;
                else                             rel.x = 0.0f;
                rel.z = 0.0f;
                Vec3 n;
                Vec3NormalizeTo(&n, &rel);
                Vec3Copy(&rel, &n);
                break;
            }
            case SHAPE_OBB: {
                const f32* rot = (const f32*)(col + 0x3C);
                const Vec3* ext = (const Vec3*)(col + 0x6C);
                Vec3 local;
                Vec3ToLocal(&local, C_POS(c), COL_CENTER(col), rot);
                Vec3Copy(&rel, &local);
                Vec3Copy(&clamped, &rel);
                clamped.x = ClampExtent(clamped.x, ext->x);
                clamped.y = ClampExtent(clamped.y, ext->y);
                clamped.z = ext->z;
                f32 ry = fabsf(rel.y / ext->y);
                if (fabsf(rel.x / ext->x) > ry) rel.y = 0.0f;
                else                             rel.x = 0.0f;
                rel.z = 0.0f;
                Vec3 world;
                Vec3RotateNormalized(&world, &rel, rot);
                Vec3Copy(&rel, &world);
                Vec3Rotate(&world, rot, &clamped);
                Vec3Copy(&clamped, &world);
                break;
            }
            }
            Scalar k = 0.3f;
            Vec3 kick;
            Vec3MulScalar(&kick, &rel, &k);
            Vec3AddTo(C_VELOCITY(c), &kick);
            CharacterSetFallTimer(c, -1.0f);
        }

        if (landed) {
            Vec3Copy(C_VELOCITY(c), &savedVel);
            CharacterSetGround(c, other, CT_COLLIDER(ct));
            Scalar k = 0.05f;
            Vec3 lift;
            Vec3MulScalar(&lift, CT_NORMAL(ct), &k);
            Vec3AddTo(C_POS(c), &lift);
            if (E_BODY(c) != NULL && PhysicsBodyGet(E_BODY(c)) != NULL) {
                Vec3MulScalar(&lift, CT_NORMAL(ct), &k);
                NudgeBody(c, &lift);
            }
            u8* gc = C_GROUND_COL(c);
            Vec3Copy(COL_PREV(gc), COL_CENTER(gc));
            CharacterResetGroundState(c, 0, 0);
            Vec3Copy(C_GROUND_NORMAL(c), CT_NORMAL(ct));
        }
        RayQueryDestroy(query, -1);
    } else {
        // Slide: remove the velocity component going into the surface.
        Scalar into, zero;
        Vec3Dot(&into, C_VELOCITY(c), CT_NORMAL(ct));
        ScalarZero(&zero);
        if (ScalarLess(&into, &zero)) {
            Vec3 n, cut;
            Vec3Assign(&n, CT_NORMAL(ct));
            n.z = 0.0f;
            Vec3MulScalar(&cut, &n, &into);
            Vec3SubFrom(C_VELOCITY(c), &cut);
        }
    }
    return TRUE;
}
