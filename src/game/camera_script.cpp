// Script functions for animation queries, in-game cutscene (IGC) aborting and the camera:
// anim_done, anim_past_*, should_igc_abort, acquire/release_camera_semaphore, SetCameraMode,
// fixed_camera_return, ...
// Original object: .text 0x80018C04 - 0x800195A4 (auto_03_80018C04_text.s / 80018E00)
#include "game/character.h"
#include "game/game.h"

extern "C" {
s32   RegisterScriptFunc(const char* name, s32 type, void* fn);   // 0x801DC91C
void  RegisterAiStateCallbacks(void);                   // 0x80014350
void  fn_8006F5A0(void);
void  fn_80019CC8(void);
void  fn_800195A4(void);
void  fn_80074D80(void);
void  fn_800197A8(void);
s32   AnimSetFindClip(void* set, s32 anim);             // 0x801AC2FC
u8*   AnimSetGetClip(void* set, s32 index);             // 0x801AC164
f32   AnimGetFrame(void* anim, f32 fps);                // 0x8018C014
f32   AnimGetTime(void* anim);                          // 0x8018BE78
f32   AnimGetLength(void* anim);                        // 0x8018C07C
u32   PadGetButtons(s32 chan);                          // 0x801B9B40
void  PadConsumeButtons(s32 chan, u32 mask);            // 0x801B9BC4
}

extern u8 gScriptCtx[];             // 0x803C4298

// Script argument (hashed name or number) passed to some callbacks.
#define SCRIPT_ARG(ctx) (*(u32*)((u8*)(ctx) + 0x1C))

// Camera controller (gScriptCtx + 4). CodeWarrior vtable slots by byte offset.
struct CameraCtl { void** vtbl; };
#define CAMERA() (*(CameraCtl**)(gScriptCtx + 0x4))
typedef void (*CamSetModeFn)(CameraCtl*, int);
typedef void (*CamFn)(CameraCtl*);
typedef void (*CamSetTimeFn)(CameraCtl*, f32);
#define CAM_SET_MODE(c, m)   ((CamSetModeFn)(c)->vtbl[0xC / 4])((c), (m))
#define CAM_CLEAR_MODE(c)    ((CamFn)(c)->vtbl[0x10 / 4])(c)
#define CAM_RETURN_FIXED(c)  ((CamFn)(c)->vtbl[0x20 / 4])(c)
#define CAM_SET_INTERP(c, t) ((CamSetTimeFn)(c)->vtbl[0x30 / 4])((c), (t))

enum { CAM_MODE_FREE = 5, CAM_MODE_FIXED = 6, CAM_MODE_7 = 7 };

Character*        gCameraOwner;    ORIG(0x805FAF60)
u8                gCameraFlag;     ORIG(0x805FAF64)
s32               gCameraLocks;    ORIG(0x805FAF68)

// Animation component (Character + 0xA0).
struct AnimCtl {
    u8  pad00[0xEC];
    s32 current;     // 0xEC
    u32 flags;       // 0xF0: 0x10 = finished
};
#define ANIM(c) (*(AnimCtl**)((u8*)(c) + 0xA0))
#define MODEL(c) (*(u8**)((u8*)(c) + 0x9C))

BOOL anim_done(ScriptCtx* ctx);
BOOL anim_past_impact_frame(ScriptCtx* ctx);
BOOL anim_past_preload_time(ScriptCtx* ctx);

// 0x80018C04
void RegisterScriptFunctions(void) {
    RegisterScriptFunc("anim_done", 0x1D, (void*)anim_done);
    RegisterScriptFunc("anim_past_impact_frame", 0x1D, (void*)anim_past_impact_frame);
    RegisterScriptFunc("anim_past_preload_time", 0x1D, (void*)anim_past_preload_time);
    RegisterAiStateCallbacks();
    fn_8006F5A0();
    fn_80019CC8();
    fn_800195A4();
    fn_80074D80();
    fn_800197A8();
}

// 0x80018C84
BOOL anim_done(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    return c != NULL && (ANIM(c)->flags & 0x10);
}

// 0x80018CB0
// TRUE once the current animation has passed its "impact" frame (at 30 fps).
BOOL anim_past_impact_frame(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL) return FALSE;
    u8* model = MODEL(c);
    if (model == NULL) return FALSE;
    void* set = *(void**)(model + 0x48);
    if (set == NULL) return FALSE;

    s32 clip = AnimSetFindClip(set, ANIM(c)->current);
    if (clip < 0) return FALSE;
    s32 impact = *(s32*)(AnimSetGetClip(*(void**)(model + 0x48), clip) + 0x28);
    return AnimGetFrame(ANIM(c), 30.0f) >= (f32)impact;
}

// 0x80018D64
// TRUE in the last second of the current animation (time to preload the next one).
BOOL anim_past_preload_time(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c == NULL || ANIM(c) == NULL) return FALSE;
    f32 t = AnimGetTime(ANIM(c));
    return AnimGetLength(ANIM(c)) - t <= 1.0f;
}

// 0x80018E00
// The player can skip an in-game cutscene with button 0x80.
BOOL should_igc_abort(ScriptCtx* ctx) {
    (void)ctx;
    s32 chan = *(s32*)(gScriptCtx + 0x38);
    if (!(PadGetButtons(chan) & 0x80)) return FALSE;
    PadConsumeButtons(chan, 5);
    return TRUE;
}

// 0x80018E54
void acquire_camera_semaphore(ScriptCtx* ctx) {
    gCameraOwner = ctx->chr;
    gCameraLocks++;
}

// 0x80018E6C
void release_camera_semaphore(ScriptCtx* ctx) {
    if (gCameraOwner == ctx->chr) gCameraOwner = NULL;
    if (--gCameraLocks < 0) gCameraLocks = 0;
    if (gCameraLocks <= 0) gCameraFlag = 0;
}

// 0x80018EB4
void SetCameraMode(ScriptCtx* ctx) {
    CameraCtl* cam = CAMERA();
    if (cam == NULL) return;
    switch (SCRIPT_ARG(ctx)) {
    case 0x8A54CB00:            CAM_SET_MODE(cam, CAM_MODE_7); break;
    case 0x7BBB2C7C /* FREE */: CAM_SET_MODE(cam, CAM_MODE_FREE); break;
    case 0x699D0C82 /* FIXED */:CAM_SET_MODE(cam, CAM_MODE_FIXED); break;
    default:                    CAM_CLEAR_MODE(cam); break;
    }
}

// 0x80018F40
// Camera sensors (trigger volumes) select the mode from their type.
void set_camera_mode_from_sensor_type(ScriptCtx* ctx) {
    CameraCtl* cam = CAMERA();
    if (cam == NULL || ctx->chr == NULL) return;
    u32 type = *(u32*)(MODEL(ctx->chr) + 0x8);
    if (type == 0x4BA45FB7)      CAM_SET_MODE(cam, CAM_MODE_FIXED);
    else if (type == 0x5C1F86EE) CAM_SET_MODE(cam, CAM_MODE_7);
}

// 0x80018FB4
void clear_camera_mode(ScriptCtx* ctx) {
    (void)ctx;
    if (gCameraLocks > 1) return;
    CameraCtl* cam = CAMERA();
    if (cam != NULL) CAM_CLEAR_MODE(cam);
}

// 0x80018FE8
void fn_80018FE8(void) {
}

// 0x80018FEC
void set_interp_timer(ScriptCtx* ctx) {
    CAM_SET_INTERP(CAMERA(), (f32)SCRIPT_ARG(ctx));
}

// 0x80019040
void fn_80019040(void) {
}

// 0x80019044
void fixed_camera_return(ScriptCtx* ctx) {
    Character* c = ctx->chr;
    if (c != NULL && (c->def->flags & 0x100)) {
        if (gCameraLocks != 1 || *(s32*)((u8*)c + 0x174) == 0) return;
        CAM_RETURN_FIXED(CAMERA());
        return;
    }
    if (gCameraLocks != 1) return;
    CAM_RETURN_FIXED(CAMERA());
}

// 0x800190C4
void fn_800190C4(void) {
}

// 0x800190C8
void force_fixed_camera_return(ScriptCtx* ctx) {
    (void)ctx;
    CAM_RETURN_FIXED(CAMERA());
}
