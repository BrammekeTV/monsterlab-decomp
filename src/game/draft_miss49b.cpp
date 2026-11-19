#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C++" {
extern u8 gGame[];
extern u8 gWorldState[];
extern u8 gIdPool[];
extern void* gScreenSets;
extern void* gEventMgr;
extern u8* gBattle;
extern u8 lbl_803C2A20[];
extern u8 lbl_803C4298[];
extern u8 lbl_8040E260[];
extern u8 lbl_8040E5F0[];
extern u8 lbl_8047C790[];
extern u8 lbl_805FB1A4[];
extern u8 lbl_805FB030[];
extern u8 lbl_805FB89C[];
extern u8* lbl_805FB2E8;
extern s32 lbl_805FB034;
extern s32 lbl_805FB038;
extern s32 lbl_803C4E28[];
extern void* lbl_805FB010;
extern void* lbl_805FB6D0;
extern u8 lbl_803584A0[];
extern s32 lbl_803582E0[];
extern void* lbl_80358470[];
extern u32 lbl_80358488[];
extern void* lbl_803C4F60[];
extern f32 lbl_80357748[];
extern u8 lbl_805F83E0[];
extern const char lbl_80381868[];
}

extern "C" {
void* ScreenSetsCurrent(void* sets);                  // 0x8018D1E4
void* ScreenSetFindElement(void* set, u32 hash);      // 0x80198F18
void* EventPost(s32 id, void* sender);
void* CharacterFind(s32 id);
void* fn_801D40E0(s32 n);
u8* BattleCtor(u8* b);
void* WorldFindLocator(void* world, u32 hash);
s32 WorldGetPhase(u8* world);
void fn_8018D2B8(const char* fmt, ...);               // 0x8018D2B8 (debug log)
void fn_8018D370(const char* fmt, ...);               // 0x8018D370 (debug log)
void fn_80199C30(void* obj);
void fn_801ECD1C(u8* p, s32 idx, s32 immediate);
void fn_801ED788(f32* out, u8* a1);
BOOL fn_801F56FC(s32 flags);
u8* fn_800E4D7C(u32 a0);
u32 fn_800E5684(void);
void* fn_800E60BC(void);
void fn_8012649C(void);
s32 fn_8016195C(void* p, s32 a, s32 b, void* c);
void fn_8004D760(u8* c, u8* monster);
void* fn_8004DAF8(void* c, s32 a, s32 b);
s32 fn_801A3010(void* obj, void* data);
void fn_8004CEE8(void*);
void fn_800D8114(u8* world);
u8* fn_800D85CC(u8* arg0, s32 arg1);
void fn_800E69EC(s32 flag);
void fn_801214E4(void* p);
s32 fn_801B9E7C(u8* a3);
void fn_801BA4A0(u8* a3);
void fn_801BDA0C(void* a0);
void fn_801E839C(u8* p);
void fn_800E6A84(s32);
s32 fn_80160A28(u8* p);
void fn_80162A70(u8* p);
void fn_801BB478(u8* p, s32 a1);
s32 fn_802728A4(s32 chan, u32* out);
s32 fn_80272B64(s32 chan, u8* out);
void operator_delete(void* p);
void fn_8020A6C8(void* a0);
void fn_801D5D14(u8 a, f32 b);
s32 PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);
void FaderSetTargetColour(f32 a0, f32 a1, f32 a2);
void StickRead(s32 channel, f32* x, f32* y);
void TriggerDoAction(void* action, void* a4, void* a5);
void* IdPoolNext(void* pool);

// 0x80195064
void UITopScreen_StopLoopingSfx(void) {
    u8* sets;
    s32 idx;
    u8* screen;

    sets = (u8*)ScreenSetsCurrent(gScreenSets);
    if (sets == NULL) {
        fn_8018D2B8(lbl_80381868 + 0x40);
        screen = NULL;
    } else {
        idx = *(s32*)(sets + 0x60);
        if (idx <= 0) {
            fn_8018D2B8(lbl_80381868 + 0x14C);
            screen = NULL;
        } else {
            screen = *(u8**)(sets + idx * 4 + 0x3C);
            if (screen == NULL) {
                fn_8018D370(lbl_80381868 + 0x170);
            }
        }
    }
    if (screen != NULL) {
        fn_80199C30(screen);
    }
}

// 0x800E9B8C
void UnspawnAttachedParticleAtChild(void* a0, u32 a1) {
    u8* p = (u8*)(uintptr_t)a1;
    s32 idx;
    u8* ev;

    if (a0 == NULL) {
        return;
    }
    if (*(u16*)p & 2) {
        idx = *(u8*)(p + 2);
    } else {
        idx = -1;
    }
    if (idx >= 0) {
        ev = (u8*)EventPost(0x1A, a0);
        if (ev != NULL) {
            *(s32*)(ev + 0x4) = 0;
            *(s32*)(ev + 0x8) = 3;
            *(s32*)(ev + 0xC) = *(s32*)p;
        }
        fn_801ECD1C((u8*)a0 + 0x94, idx, 0);
    }
}

// 0x800F64B4
void VariableToNormalCamera(f32 a0) {
    u8* obj;

    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        *(s32*)(*(u8**)(lbl_803C4298 + 0x8) + 0x2C0) = *(s32*)(gGame + 0xD614);
        obj = *(u8**)(lbl_803C4298 + 0x4);
        ((void (*)(void*))(*(void**)(*(u8**)obj + 0x10)))(obj);
        obj = *(u8**)(lbl_803C4298 + 0x4);
        ((void (*)(void*, f32))(*(void**)(*(u8**)obj + 0x30)))(obj, a0);
        return;
    }
    if (WorldGetPhase(gGame + 0xD5F0) == 1) {
        fn_8012649C();
    }
}

// 0x8013FE60
ScriptValue WasPacketDelivered(void) {
    return (ScriptValue)(fn_801F56FC(1) == 1);
}

// 0x8012642C
void WiiSelectorSetUpCameraView(void) {
    u8* cam;
    u8* obj;
    u8* view;

    if (lbl_805FB2E8 != NULL) {
        cam = (u8*)fn_800E60BC();
        if (cam != NULL) {
            *(u8*)(cam + 0x54) = 1;
            view = fn_800E4D7C(fn_800E5684());
            if (view != NULL) {
                *(u8*)(view + 0x54) = 0;
            }
            obj = *(u8**)(lbl_803C4298 + 0x4);
            ((void (*)(void*))(*(void**)(*(u8**)obj + 0x10)))(obj);
        }
    }
}

// 0x800F7D98
void Wii_StartConversationFX(BOOL a0) {
    f32 v1[7];
    f32 v2[6];

    if (a0 == 0) {
        v1[0] = 0.0f;
        v1[1] = 0.0f;
        v1[2] = 0.0f;
        v1[3] = 1.0f;
        v1[4] = 1.0f;
        v1[5] = 0.0f;
        v1[6] = 20.01f;
        fn_8016195C(lbl_8040E5F0, 4, 0, v1);
        v2[0] = 1.0f;
        v2[1] = 0.041666668f;
        v2[2] = 1.0f;
        v2[3] = 1.0f;
        v2[4] = 1.0f;
        v2[5] = 0.8f;
        fn_8016195C(lbl_8040E5F0, 0x10, 0, v2);
    }
}

// 0x800ECA30
s32 WithinRangeOf(void* a0, u32 a1, f32 a2) {
    f32 v[3];
    u8* pos;
    f32 dx;
    f32 dy;
    f32 dz;

    fn_801ED788(v, (u8*)(uintptr_t)a1);
    pos = *(u8**)((u8*)a0 + 0x4);
    dx = *(f32*)(pos + 0x0) - v[0];
    dy = *(f32*)(pos + 0x4) - v[1];
    dz = *(f32*)(pos + 0x8) - v[2];
    return ((dx * dx + dy * dy) + dz * dz) <= (a2 * a2);
}

// 0x801F2B40
void ZoneSlotInit(void* arg0) {
    u8* p = (u8*)arg0;
    u8* owner = *(u8**)p;

    if (owner == NULL || !(*(u32*)(owner + 0x4) & 1)) {
        *(s32*)(p + 0x4) = 0;
        *(s32*)(p + 0x8) = -2;
        *(s32*)(p + 0x10) = -2;
        *(s32*)(p + 0x28) = 0;
        *(s32*)(p + 0x2C) = 0;
        *(f32*)(p + 0x14) = 0.0f;
        *(u16*)(p + 0x18) = 0;
        *(s32*)(p + 0x1C) = 0;
        *(void**)(p + 0x20) = IdPoolNext(gIdPool);
        *(s32*)(p + 0x24) = -1;
    }
    *(s32*)(p + 0xC) = 0x10;
}

// 0x801E1BBC
void fSetResultRegister(f32 a0) {
    *(f32*)lbl_805FB89C = a0;
}

// 0x80009748
void fn_80009748(void* arg0, u8 arg1) {
    u32 flags;

    (void)arg0;
    (void)arg1;
    fn_8004CEE8(0);
    app_StopMusic();
    fn_8020A6C8(NULL);  // r3 is left over from app_StopMusic
    fn_800D8114(gGame + 0xD5F0);
    fn_800D85CC(gGame + 0xD5F0, 0);
    fn_800E69EC(0);
    fn_801214E4(lbl_803C2A20 + 0x10);
    fn_801B9E7C((u8*)gEventMgr);
    fn_801BA4A0((u8*)lbl_805FB6D0);
    fn_801BDA0C(lbl_8047C790);
    fn_801E839C((u8*)arg0 + 0x40);
    fn_800E6A84(1);
    fn_80160A28(lbl_8040E260);
    fn_80162A70(lbl_8040E5F0);
    flags = *(u32*)(lbl_803C4298 + 0x3C);
    if ((flags >> 8) & 1) {
        TriggerDoAction(gWorldState + 0xA0, NULL, NULL);
        *(u32*)(lbl_803C4298 + 0x3C) = *(u32*)(lbl_803C4298 + 0x3C) & 0xFFFFFEFFu;
    }
}

// 0x8000C314
void fn_8000C314(u8* arg0) {
    u32 sp8;
    u8 sp10[0x10];

    sp8 = 0;
    if (fn_802728A4(0, &sp8) == 0 && fn_80272B64(0, sp10) == 0 && *(u32*)(sp10 + 0xC) != 0) {
        *(s32*)(arg0 + 0x154) = 1;
    }
}

// 0x8001B164
void fn_8001B164(u8* arg0) {
    s32 count;
    u8* table;

    count = *(s32*)(arg0 + 0x0);
    if (count == 0) {
        *(u8*)(arg0 + 0x430) = 0;
        return;
    }
    count -= 1;
    *(s32*)(arg0 + 0x0) = count;
    table = *(u8**)(arg0 + 0x24);
    fn_801BB478(*(u8**)table, 0);
    *(void**)table = *(void**)(arg0 + count * 4 + 0x4);
    *(void**)(arg0 + count * 4 + 0x4) = NULL;
    *(u8*)(arg0 + 0x430) = 0;
}

// 0x8001BFC0
s32 fn_8001BFC0(void) {
    s32 result = 4;
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 ax;
    f32 ay;

    StickRead(*(s32*)(lbl_803C4298 + 0x38) * 16 + 0xE, &x, &y);
    ax = x < 0.0f ? -x : x;
    ay = y < 0.0f ? -y : y;
    if (ax > ay) {
        if (x > 0.2f) {
            result = 2;
        } else if (x < -0.2f) {
            result = 3;
        }
    } else if (y > 0.2f) {
        result = 0;
    } else if (y < -0.2f) {
        result = 1;
    }
    return result;
}

// 0x8001F6A4
void fn_8001F6A4(u8* arg0) {
    u8* temp;
    u8* sub;

    *(void**)(arg0 + 0x8) = ScreenSetFindElement(ScreenSetsCurrent(gScreenSets), 0x2C7D4AEAu);
    lbl_803C4E28[0] = 0;
    lbl_803C4E28[1] = 0;
    lbl_803C4E28[2] = 0;
    lbl_803C4E28[3] = 0;
    lbl_803C4E28[4] = 0;
    lbl_805FB038 = 5;
    lbl_805FB034 = 0;
    if (*(void**)(arg0 + 0xC) == NULL) {
        temp = (u8*)CharacterFind((s32)0x4CF9FE5B);
        *(u8**)(arg0 + 0xC) = temp;
        if (temp != NULL) {
            if (*(u8**)(arg0 + 0x4) != NULL) {
                fn_8004D760(temp, *(u8**)(arg0 + 0x4));
                fn_8004DAF8(*(void**)(arg0 + 0xC), 1, 0);
                lbl_805FB038 = 5;
            }
            sub = *(u8**)(arg0 + 0x8);
            if (sub != NULL) {
                fn_801A3010(sub, *(void**)(arg0 + 0xC));
                sub = *(u8**)(arg0 + 0x8);
                ((void (*)(void*, s32))(*(void**)(*(u8**)(sub + 0x138) + 0x2C)))(sub, 0x1A);
            }
        }
        *(s32*)(arg0 + 0x20) = 1;
    }
}

// 0x8002D508
void fn_8002D508(s32 arg0, f32 farg0, u32 arg1) {
    lbl_805F83E0[arg0] = (u8)arg1;
    lbl_80357748[arg0] = farg0;
}

// 0x8002D524
f32 fn_8002D524(s32 arg0) {
    return lbl_80357748[arg0];
}

// 0x8003E354
void fn_8003E354(void) {
    void* obj = lbl_805FB010;

    if (obj != NULL) {
        ((void (*)(void*, s32))(*(void**)(*(u8**)obj + 0x8)))(obj, 1);
        lbl_805FB010 = NULL;
    }
}

// 0x8003F858
u32 fn_8003F858(void) {
    u32 result = 0;
    s32 i;

    if (gBattle == NULL) {
        return 0;
    }
    if (lbl_805FB030[0] != 0) {
        for (i = 0; i < 6; i++) {
            if ((s32)lbl_803C4298[0x1C] == lbl_803582E0[i]) {
                result = lbl_80358488[i];
                break;
            }
        }
    }
    if (result == 0) {
        return 0x38EE8A9Cu;
    }
    return result;
}

// 0x8003F7DC
void* fn_8003F7DC(void) {
    void* result = NULL;
    s32 i;

    if (gBattle == NULL) {
        return NULL;
    }
    if (lbl_805FB030[0] != 0) {
        for (i = 0; i < 6; i++) {
            if ((s32)lbl_803C4298[0x1C] == lbl_803582E0[i]) {
                result = lbl_80358470[i];
                break;
            }
        }
    }
    if (result == NULL) {
        return (void*)(uintptr_t)0x0B8025EFu;
    }
    return result;
}

// 0x8003D4B0
void fn_8003D4B0(void) {
    u8* temp;
    u8* var;
    u8* old;

    lbl_805FB1A4[1] = 1;
    if (lbl_803C4298[0x1C] != 0x1C) {
        temp = (u8*)fn_801D40E0(4);
        if (temp != NULL) {
            *(void**)temp = (void*)lbl_803584A0;
            old = (u8*)lbl_805FB010;
            if (old != NULL) {
                ((void (*)(void*, s32))(*(void**)(*(u8**)old + 0x8)))(old, 1);
                lbl_805FB010 = NULL;
            }
            var = (u8*)fn_801D40E0(0xB04);
            if (var != NULL) {
                var = BattleCtor(var);
            }
            gBattle = var;
            lbl_805FB010 = temp;
        }
    }
    WorldFindLocator(gWorldState, 0x834C7963u);
    WorldFindLocator(gWorldState, 0xAAE1A370u);
}

// 0x80042B30
void fn_80042B30(void) {
    s32 i;
    u8* obj;

    FaderSetTargetColour(1.0f, 1.0f, 1.0f);
    fn_801D5D14(1, 0.08f);
    PlaySound(0x4AE3C760u, 0, 0, 0, 0, 0);
    for (i = 0; i < 0x96; i++) {
        obj = (u8*)lbl_803C4F60[i];
        if (obj != NULL) {
            ((void (*)(void*, s32))(*(void**)(*(u8**)(obj + 0x138) + 0x2C)))(obj, 2);
            lbl_803C4F60[i] = NULL;
        }
    }
}

// 0x80031090
void fn_80031090(u8* arg0, s32 arg1) {
    u8* inner;

    if (arg0 != NULL) {
        *(s32*)(arg0 + 0x4) = 0;
        inner = *(u8**)arg0;
        if (inner != NULL) {
            operator_delete(inner);
        }
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
}

}  // extern "C"
