// Scene-graph view (camera frustum) objects.
// Original: 0x80170374 - 0x801704E8 (scene-graph module)
#include <string.h>

#include "engine/engine.h"
#include "engine/view.h"

extern "C" u8 SCGetAspectRatio(void);    // 0x8026A9D8: 1 = 16:9

enum { SG_MEMORY_FRUSTUM = 10 };
enum { SC_ASPECT_RATIO_16x9 = 1 };

// 0x1A4 bytes
struct View {
    f32 rot[3][3];          // 0x000: orientation, identity on creation
    f32 pos[3];             // 0x024
    u8  unk30[0x90];        // 0x030
    u8  planesA[0x60];      // 0x0C0: cleared on creation (frustum planes?)
    u8  planesB[0x60];      // 0x120
    f32 aspect;             // 0x180
    f32 scale;              // 0x184: 1.0
    s32 dirty;              // 0x188: 1
    f32 unk18C;             // 0x18C: 1.0
    f32 defaultAspect;      // 0x190: 4:3 or 16:9 from console settings
    f32 nearPlane;          // 0x194: 0.05
    f32 farPlane;           // 0x198: 10.0
    s32 unk19C;             // 0x19C
    s32 unk1A0;             // 0x1A0
};

extern EngineMemCallbacks* gSgMem;      ORIG(0x805FB608) // set by SgInit

// 0x80170374
extern "C" View* ViewCreate(void) {
    View* v = (View*)gSgMem->alloc(sizeof(View), 16, SG_MEMORY_FRUSTUM, NULL, 0);

    v->unk19C = 0;
    v->unk1A0 = 0;
    v->nearPlane = 0.05f;
    v->farPlane = 10.0f;
    v->dirty = 1;
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++)
            v->rot[r][c] = (r == c) ? 1.0f : 0.0f;
    v->pos[0] = v->pos[1] = v->pos[2] = 0.0f;

    memset(v->planesA, 0, sizeof(v->planesA));
    memset(v->planesB, 0, sizeof(v->planesB));
    v->unk18C = 1.0f;

    v->defaultAspect = (SCGetAspectRatio() == SC_ASPECT_RATIO_16x9) ? 16.0f / 9.0f : 4.0f / 3.0f;
    v->scale = 1.0f;
    v->aspect = v->defaultAspect;
    return v;
}

// 0x801704D8
extern "C" void ViewDestroy(View* v) {
    gSgMem->free(v);
}
