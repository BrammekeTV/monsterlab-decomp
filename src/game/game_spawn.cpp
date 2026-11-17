// Small entity helpers: spawning the shared "attached" helper entity and target prediction.
// Original object: .text 0x8000C41C - 0x8000C7A8 (auto_03_8000C41C_text.s)
#include "engine/math.h"
#include "game/game.h"

extern "C" {
void  fn_80121498(void* member, s32 flags);
void  operator_delete(void* p);                         // 0x801D426C
void* HashTableFind(void* table, u32 hash);             // 0x801DB32C
BOOL  EntityAttach(void* e, void* a, void* b, int, int, int, int, Vec3 scale);   // 0x80065200
}

extern u8 gWorldState[];   // 0x803E6D00

#define OBJ_TYPE_OWNER   0x37645F52u   // type id an owner must have
#define HASH_HELPER_TMPL 0xBE8520ABu   // template looked up in zone 0

struct SpawnedEntity {
    u8    pad00[8];
    Vec3  scale;        // 0x08
    u8    pad14[0x88];
    void* owner9C;      // 0x9C
    u8    padA0[0x18];
    u32   flags;        // 0xB8
    u8    padBC[0x16C];
    void* owner;        // 0x228
    u8    pad22C[0x1C];
    s32   arg;          // 0x248
};

// 0x8000C41C
void* fn_8000C41C(void* self, s32 flags) {
    if (self != NULL) {
        fn_80121498((u8*)self + 0x10, -1);
        if (flags > 0) operator_delete(self);
    }
    return self;
}

static inline SpawnedEntity* FindHelper(void* owner, s32 arg) {
    if (owner == NULL || *(u32*)owner != OBJ_TYPE_OWNER) return NULL;
    u8* zone0 = *(u8**)(*(u8**)gWorldState + 0x1A4);
    SpawnedEntity* e = (SpawnedEntity*)HashTableFind(zone0 + 0xE4, HASH_HELPER_TMPL);
    if (e != NULL) {
        e->flags &= ~2u;
        e->owner = owner;
        e->owner9C = owner;
        e->arg = arg;
    }
    return e;
}

// 0x8000C478
void* SpawnHelper(void* owner, s32 arg) {
    return FindHelper(owner, arg);
}

// 0x8000C508
void* SpawnHelperAttached(void* a, void* owner, void* b) {
    SpawnedEntity* e = FindHelper(owner, 0);
    if (e == NULL) return NULL;
    e->scale.x = e->scale.y = e->scale.z = 1.0f;
    Vec3 one = { 1.0f, 1.0f, 1.0f };
    if (!EntityAttach(e, a, b, 0, 0, 0, 0, one)) return NULL;
    return e;
}

struct Mover {
    void** vtbl;
    u8*    def;         // 0x04: +0x1C flags
    u8     pad08[0x1C];
    f32    speed;       // 0x24
    u8     pad28[0x748];
    Vec3   velocity;    // 0x770
};
struct Seeker { u8 pad[0x18]; Vec3 pos; };

// 0x8000C604
// Aim point for `seeker` chasing `target`: the target's closest point, led by its velocity.
void PredictTargetPos(u8* self, Seeker* seeker, Mover* target, Vec3* out, f32 dt) {
    if (seeker == NULL || target == NULL) return;

    typedef const Vec3* (*ClosestFn)(Mover*, const Vec3*, Seeker*);
    const Vec3* p = ((ClosestFn)target->vtbl[0x44 / 4])(target, &seeker->pos, seeker);
    *out = *p;

    Vec3 d = { out->x - seeker->pos.x, out->y - seeker->pos.y, out->z - seeker->pos.z };
    if (d.x * d.x + d.y * d.y + d.z * d.z < 4.0f) return;
    if (!(*(u32*)(target->def + 0x1C) & 0x10)) return;

    f32 speed = target->speed > 0.032f ? target->speed : 0.032f;
    f32 inv = 1.0f / speed;
    Vec3 v = { target->velocity.x * inv, target->velocity.y * inv, target->velocity.z * inv };

    f32 selfSpeed = *(f32*)(self + 0xB8);
    if (selfSpeed == 0.0f) return;
    f32 k = 10.0f * dt / selfSpeed;
    out->x += v.x * k;
    out->y += v.y * k;
    out->z += v.z * k;
}

// 0x8000C7A0
void* fn_8000C7A0(u8* obj) {
    return obj + 0x18;
}
