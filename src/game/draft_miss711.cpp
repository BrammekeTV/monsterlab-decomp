// Small functions 0x80029D08 .. 0x800DEFB8 (draft miss711), verified against the asm.
#include "types.h"
#include "engine/math.h"

extern "C++" void* gEventMgr;
extern "C++" void* gScreenSets;
extern "C++" s32 gZoneResourceSize;
extern u8 gWorldState[];
extern void* lbl_805FBBAC;
extern u8 lbl_805FB1A1;
extern s32 lbl_805FAFC8;
extern u8 lbl_8035AFD0[];
extern u8 lbl_803C4298[];
extern u8 lbl_8047C760[];
extern const char lbl_80357950[];
extern const char lbl_805F83F8[];
extern const char lbl_805F8400[];
extern const char lbl_805F8408[];
extern s32 lbl_80346CE0[];
extern s32 lbl_80346CD0[];

extern f32 lbl_805FCCDC;
extern f32 lbl_805FCCE0;
extern f32 lbl_805FCCE4;
extern f32 lbl_805FCCE8;
extern f32 lbl_805FCD08;
extern f32 lbl_805FD770;
extern f32 lbl_805FDA90;
extern f32 lbl_805FDFC0;
extern f32 lbl_805FDFC4;
extern f32 lbl_805FDFC8;
extern f32 lbl_805FDFCC;
extern f32 lbl_805FDFD4;
extern f32 lbl_805FDFDC;
extern f32 lbl_805FDFE0;
extern f32 lbl_805FDFE4;
extern f32 lbl_805FDFE8;
extern f32 lbl_805FDFEC;
extern f32 lbl_805FDFF0;
extern f32 lbl_805FE7D0;
extern f32 lbl_805FE7D4;
extern f32 lbl_805FE7D8;
extern f32 lbl_805FE7DC;
extern f32 lbl_805FE7E0;

extern "C" {
void operator_delete(void* p);
void TimedRumble(s32 chan, f32 strength, u32 a2);
void StopRumble(s32 chan);
void UIElement_PlayUIAnim(u32 element, u32 anim);
void UIText_SetString_Int(u32 a0, s32 a1);
void UIElement_SetVisibleFlag(u32 a0, u32 a1);
void CatchMinigame_UpdateBase(void* g, f32 dt);
void CatchMinigame_UpdatePointer(void* g);
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);
s32 PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);
void PropPlayAnim(void* prop, int a, int b, BOOL once);
s32 AnimSetFindClip(void* set, s32 anim);
s32 RaySceneTest(void* ray);
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner);
u32 RandomU32(void);
u32 StringHash(const char* s);
int SysSprintf(char* buf, const char* fmt, ...);
void* UIWidgetFindChild(void* widget, u32 hash);
void* ScreenSetFindElement(void* set, u32 hash);
void* ScreenSetsCurrent(void* sets);
void* fn_80070F4C(void);
void fn_80070460(u8* p);
void fn_800223DC(u8* p);
void fn_8002C444(f32* out, u8* p, f32 v);
void fn_8002FE14(u8* g, s32 kind);
void fn_800302B0(u8* elem, void* found);
void fn_8003056C(u8* p);
void fn_800328A0(u8* obj, f32 t);
void fn_800330E0(u8* elem, f32 dt);
void fn_80031AA4(u8* p);
void fn_80032A48(u8* p);
void fn_8006FAF4(u8* p, s32 a);
void fn_80081E84(u8* self);
void* fn_80198F1C(void* set, u32 hash);
s32 fn_8019BBA4(u8* arg0, u8* arg1, s32 arg2);
s32 fn_801AB80C(u8* a3, f32 rate);
void fn_8020A398(s32 handle);
u32 fn_8020A9A8(s32 handle);
u8* fn_801B3308(void);
s32 fn_801B33F4(u8* mgr, u8* key);
void fn_801B3378(u8* p, u8* key);
u32 fn_801EC880(u8* p, u32 hash);
void fn_801EC8C4(u8* p, u32 a, u32 b);
void fn_8020B1D4(void* h, s32 on);
u8* fn_801F7920(u8* p, s32 del);
void fn_801C849C(u8* mgr, u8* item);
void* fn_801D40E0(s32 n);
void* fn_80209814(void* v);
void fn_8008F660(u8* p);
void fn_800A7D88(void* obj, u32 hash);
void fn_800B0CFC(u8* p);
s32 fn_800C180C(s32 a, s32 b);
void fn_801458B0(f32* quat, f32* a, f32* b);
void fn_80145F9C(f32* out, f32* a, f32* b, f32 t);
void fn_80146180(void* quat, const Vec3* axis, f32 angle);
f32 fn_801BFE38(u8* a, f32* b, u8* c, u8* d, f32* outT0, f32* outT1);

// Voice handle for the sound effect: created on first use (see fn_801D40E0 / fn_80209814).
static void EnsureVoiceHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* h = fn_801D40E0(1);
        if (h != NULL) h = fn_80209814(h);
        lbl_805FBBAC = h;
    }
}

// Zone record of the current world block: *(world + 0x1A4) + gZoneResourceSize.
static u8* ZoneRecord(void) {
    return *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
}

// 0x80029D08
// Advances the timer while in state 3/4, re-seeds the track slots and pushes the position
// values into each track object.
void fn_80029D08(u8* p, f32 dt) {
    s32 st = *(s32*)(p + 0x18);
    if (st == 3 || st == 4) {
        s32 idx = *(s32*)(p + 0x110);
        u8* sub = *(u8**)(p + 0x104 + idx * 4);
        (*(void (**)(void*, s32))(*(u8**)(sub + 0x138) + 0x30))(sub, 2);
        s32 old = *(s32*)(p + 0x10C);
        *(s32*)(p + 0x10C) = old + 1;
        s32 count = (*(s32 (**)(void*, s32))(*(u8**)p + 0x14))(p, old);
        if (*(s32*)(p + 0x10C) >= lbl_805FAFC8 * count) *(s32*)(p + 0x10C) = 0;
        idx = *(s32*)(p + 0x10C) / lbl_805FAFC8;
        *(s32*)(p + 0x110) = idx;
        sub = *(u8**)(p + 0x104 + idx * 4);
        (*(void (**)(void*, s32))(*(u8**)(sub + 0x138) + 0x2C))(sub, 2);

        f32 out[2];
        fn_8002C444(out, p + 0xB8, dt * (*(f32*)(p + 0x4) * (f32)(*(s32*)(p + 0x8))));
        *(f32*)(p + 0x2C) = out[0];
        *(f32*)(p + 0x30) = out[1];

        s32 i = 0;
        while (i < (*(s32 (**)(void*))(*(u8**)p + 0x14))(p)) {
            u8* s = *(u8**)(p + 0x104 + i * 4);
            f32 f1v = *(f32*)(p + 0x30);
            f32 f2v = *(f32*)(s + 0x70);
            f32 f0v = *(f32*)(p + 0x2C);
            *(f32*)(s + 0x68) = f0v;
            *(f32*)(s + 0x6C) = f1v;
            *(f32*)(s + 0x70) = f2v;
            (*(void (**)(void*, s32, f32, f32))(*(u8**)(s + 0x138) + 0x2C))(s, 0x18, f1v, f2v);
            i++;
        }

        u8* last = *(u8**)(p + 0x114);
        f32 f1l = *(f32*)(p + 0x30);
        f32 f2l = *(f32*)(last + 0x70);
        f32 f0l = *(f32*)(p + 0x2C);
        *(f32*)(last + 0x68) = f0l;
        *(f32*)(last + 0x6C) = f1l;
        *(f32*)(last + 0x70) = f2l;
        (*(void (**)(void*, s32, f32, f32))(*(u8**)(last + 0x138) + 0x2C))(last, 0x18, f1l, f2l);

        if (*(f32*)(p + 0xE4) >= *(f32*)(p + 0xE0)) {
            (*(void (**)(void*))(*(u8**)p + 0x1C))(p);
        }
    }
}

// 0x8002FA68
// Looks up the UI screen-set elements: the two header widgets, the ten per-slot widget
// groups (pointers stored in the five tables at 0xE44..0xEE4) and the four bottom-row widgets.
void fn_8002FA68(u8* p) {
    char buf[0x40];
    void* sets;
    s32 i;
    u8* elA = p + 0x55C;
    u8* elB = p + 0x9BC;
    u8* elC = p + 0x78C;
    u8* elD = p + 0xBEC;
    u8* elE = p + 0x2B4;

    fn_800223DC(p);
    sets = ScreenSetsCurrent(gScreenSets);
    *(void**)(p + 0x27C) = ScreenSetFindElement(sets, 0x22F1B404u);
    void* e278 = ScreenSetFindElement(sets, 0x728D86D2u);
    *(void**)(p + 0x278) = e278;
    UIText_SetString_Int(*(u32*)e278, 0);
    UIElement_SetVisibleFlag(*(u32*)e278, 1);
    fn_8002FE14(p, 0);

    for (i = 0; i < 10; i++) {
        SysSprintf(buf, lbl_80357950 + 0x40, i);
        fn_800302B0(elA, ScreenSetFindElement(sets, StringHash(buf)));
        *(void**)(p + 0xE6C + i * 4) = elA;

        SysSprintf(buf, lbl_80357950 + 0x4C, i);
        fn_800302B0(elB, ScreenSetFindElement(sets, StringHash(buf)));
        *(void**)(p + 0xEBC + i * 4) = elB;

        SysSprintf(buf, lbl_805F83F8, i);
        fn_800302B0(elC, ScreenSetFindElement(sets, StringHash(buf)));
        *(void**)(p + 0xE94 + i * 4) = elC;

        SysSprintf(buf, lbl_80357950 + 0x58, i);
        fn_800302B0(elD, ScreenSetFindElement(sets, StringHash(buf)));
        *(void**)(p + 0xEE4 + i * 4) = elD;

        SysSprintf(buf, lbl_805F8400, i);
        fn_800302B0(elE, ScreenSetFindElement(sets, StringHash(buf)));
        *(void**)(p + 0xE44 + i * 4) = elE;

        elA += 0x38;
        elB += 0x38;
        elC += 0x38;
        elD += 0x3C;
        elE += 0x44;
    }

    *(void**)(p + 0x294) = ScreenSetFindElement(sets, StringHash(lbl_805F8408));
    *(void**)(p + 0x298) = ScreenSetFindElement(sets, StringHash(lbl_80357950 + 0x64));
    *(void**)(p + 0x29C) = ScreenSetFindElement(sets, StringHash(lbl_80357950 + 0x70));
    *(void**)(p + 0x2A0) = ScreenSetFindElement(sets, StringHash(lbl_80357950 + 0x7C));
}

// 0x80030CC8
// Per-state tick for the rumble/sound/UI-anim sequence; state 0 is idle.
void fn_80030CC8(u8* p) {
    fn_8003056C(p);
    s32 st = *(s32*)(p + 0x38);
    if (st == 0) return;

    if (st == 3) {
        if (*(s32*)(p + 0x3C) != 0) {
            TimedRumble(*(s32*)(lbl_803C4298 + 0x38), lbl_805FCCDC, 4);
        }
        if (*(s32*)(p + 0x40) > 0 && fn_8020A9A8(*(s32*)(p + 0x40)) != 0) return;
        *(s32*)(p + 0x40) = PlaySound(0xBE77A95Eu, 0, 0, 0, 0, 0);
        return;
    }
    if (st == 1) {
        *(s32*)(p + 0x38) = 2;
        u8* q = *(u8**)(p + 0x4);
        *(f32*)(q + 0x70) = lbl_805FCCE0;
        (*(void (**)(void*, s32))(*(u8**)(q + 0x138) + 0x2C))(q, 0x18);
        UIElement_PlayUIAnim(*(u32*)q, 0xD3601402u);
        PlaySound(0x741322E8u, 0, 0, 0, 0, 0);
        return;
    }
    if (st == 2) {
        u8* q = *(u8**)(p + 0x4);
        if (*(s32*)(q + 0x10) == 0) {
            *(s32*)(p + 0x38) = 3;
            *(f32*)(p + 0x18) = lbl_805FCCE4;
            *(s32*)(p + 0x40) = PlaySound(0xBE77A95Eu, 0, 0, 0, 0, 0);
        }
        return;
    }
    if (st == 4) {
        *(s32*)(p + 0x38) = 5;
        u32 r = RandomU32();
        *(f32*)(p + 0x18) = (f32)(r % 5u + 8u);
        void* sets = ScreenSetsCurrent(gScreenSets);
        u8* e = (u8*)fn_80198F1C(sets, 0x1FC61193u);
        if (e != NULL) {
            fn_801AB80C(e, lbl_805FCCE8);
            fn_8019BBA4(*(u8**)(p + 0x4), e, 10);
        }
        if (*(s32*)(p + 0x40) > 0) fn_8020A398(*(s32*)(p + 0x40));
        return;
    }
    if (st == 5) {
        u8* q = *(u8**)(p + 0x4);
        if (*(s32*)(q + 0x10) == 0) {
            (*(void (**)(void*))(*(u8**)p + 0x14))(p);
        }
    }
}

// 0x80031C68
// Per-frame update of the minigame object: refreshes the base/pointer state, fires the
// events for the current keyframe entries and advances the timer.
void fn_80031C68(u8* p, f32 dt) {
    s32 i;
    CatchMinigame_UpdatePointer(p);
    CatchMinigame_UpdateBase(p, dt);
    fn_800328A0(*(u8**)(p + 0xCC), dt);
    if (*(s32*)(*(u8**)(p + 0xCC) + 0x44) == 5) {
        EventSend(gEventMgr, *(void**)(p + 0x90), NULL, 0x3C98014Au, 0, 0, 1, 0);
    }

    for (i = 0; i < *(s32*)(p + 0xA8); i++) {
        u8* elem = *(u8**)(p + 0xC0) + i * 0x60;
        fn_800330E0(elem, dt);
        s32 t = *(s32*)(elem + 0x8);
        if (t == 2) {
            *(s32*)(p + 0xE0) = 0;
            *(u8*)(p + 0xC4) = 1;
        } else if (t == 5) {
            *(s32*)(p + 0xE0) = *(s32*)(p + 0xE0) + 1;
            if (*(void**)(*(u8**)(p + 0xCC) + 0x14) == elem) {
                *(s32*)(p + 0xE4) = 0;
            }
        }
    }

    f32 timer = *(f32*)(p + 0xD4) + dt;
    *(f32*)(p + 0xD4) = timer;
    if (timer >= *(f32*)(p + 0xD8)) fn_80031AA4(p);

    fn_80032A48(*(u8**)(p + 0xCC));
    if (*(s32*)(p + 0xD0) >= *(s32*)(p + 0x9C)) {
        EventSend(gEventMgr, *(void**)(p + 0x90), NULL, 0xC608C842u, 0, 0, 1, 0);
    }
    if (*(f32*)(p + 0x6C) < lbl_805FCD08) {
        StopRumble(*(s32*)(lbl_803C4298 + 0x38));
        EventSend(gEventMgr, *(void**)(p + 0x90), NULL, 0x117741C3u, 0, 0, 1, 0);
    }

    // Three hooks stored directly in the object at +0x64, +0x60, +0x6C (called with p).
    {
        u8* q = *(u8**)(p + 0x4);
        void (*hook)(void*) = (void (*)(void*))(*(void**)(q + 0x64));
        hook(p);
        q = *(u8**)(p + 0x4);
        hook = (void (*)(void*))(*(void**)(q + 0x60));
        hook(p);
        q = *(u8**)(p + 0x4);
        hook = (void (*)(void*))(*(void**)(q + 0x6C));
        hook(p);
    }
}

// 0x8007D620
// Teardown of the effect/animation object: clears the sub-object links, spawns or removes the
// per-slot effects, then frees the object when del > 0. Returns p.
u8* fn_8007D620(u8* p, s32 del) {
    s32 i;
    if (p == NULL) return p;
    *(void**)(p + 0x4) = lbl_8035AFD0;
    if (fn_801B33F4(fn_801B3308(), p + 0xC8)) {
        fn_801B3378(fn_801B3308(), p + 0xC8);
    }
    if (*(void**)(p + 0x2A8) != NULL) {
        u8* o = *(u8**)(p + 0x2A8);
        (*(void (**)(void*, s32))(*(u8**)o + 0x8))(o, 1);
    }
    *(void**)(p + 0x2A8) = NULL;
    {
        u8* o = *(u8**)(p + 0x29C);
        (*(void (**)(void*))(*(u8**)o + 0xC0))(o);
        o = *(u8**)(p + 0x2A0);
        (*(void (**)(void*))(*(u8**)o + 0xC0))(o);
    }
    for (i = 0; i < *(s32*)(p + 0x298); i++) {
        fn_80081E84(*(u8**)(p + 0x1F8 + i * 4));
    }

    *(f32*)(p + 0x324) = lbl_805FD770;
    *(u8*)(p + 0x384) = 0;
    for (i = 0; i < 6; i++) {
        u8** slot = (u8**)(p + 0x36C + i * 4);
        if ((*(u8*)(p + 0x384) >> i) & 1) {
            u8* obj = *(u8**)(p + 0x354 + i * 4);
            f32 pos[4];
            pos[0] = *(f32*)(obj + 0x8);
            pos[1] = *(f32*)(obj + 0xC);
            pos[2] = *(f32*)(obj + 0x10);
            pos[3] = lbl_805FD770;
            SpawnEffectAt(lbl_8047C760, 0xFDB35BFEu, pos, slot);
            EnsureVoiceHandle();
            PlaySound(0x6158D3FDu, 0, 0, 0, 0, 1);
        } else if (*slot != NULL) {
            fn_801C849C(lbl_8047C760, *slot);
            *slot = NULL;
        }
    }

    fn_801B3378(fn_801B3308(), p + 0xC8);
    EnsureVoiceHandle();
    fn_8020B1D4(lbl_805FBBAC, 0);
    fn_801F7920(p + 0xC8, -1);
    fn_8006FAF4(p, 0);
    if (del > 0) operator_delete(p);
    return p;
}

// 0x8008F918
// Plays the entry animation and cue sounds on the prop, then walks the zone object table and
// fires the matching script event for each flagged object.
void fn_8008F918(u8* p) {
    s32 i;
    fn_80070460(p);
    *(f32*)(p + 0x8C) = lbl_805FDA90;

    u8* a = *(u8**)(p + 0xA4);
    u8* b = *(u8**)(a + 0x9C);
    if (b != NULL && *(void**)(b + 0x48) != NULL) {
        s32 clip = AnimSetFindClip(*(void**)(b + 0x48), 0x4FB7031E);
        PropPlayAnim(*(void**)(p + 0xA4), clip, 0, 1);
        EnsureVoiceHandle();
        PlaySound(0xCADC05A2u, 0, 0, 0, 0, 0);
        u8* sub = *(u8**)(p + 0xA4) + 0x94;
        u32 h = fn_801EC880(sub, 0xF3C07FC1u);
        fn_801EC8C4(sub, h, 0xAF0DCAD7u);
    }
    EnsureVoiceHandle();
    PlaySound(0xD8D839DEu, 0, 0, 0, 0, 0);
    fn_8008F660(p);

    *(s32*)(p + 0x810) = 0;
    *(s32*)(p + 0x80C) = 0;
    *(s32*)(p + 0x814) = 0;

    u8* zone0 = ZoneRecord();
    s32 total = *(s32*)(zone0 + 0x94) + *(s32*)(zone0 + 0x98);
    s32 off = 0;
    s32 off4 = 0;
    for (i = 0; i < total; i++, off += 0x40, off4 += 4) {
        u8* z = ZoneRecord();
        u8* e;
        u8* obj;
        if (i < *(s32*)(z + 0x94)) {
            obj = *(u8**)(*(u8**)(z + 0x120) + off4);
            e = *(u8**)(z + 0x8C) + off;
        } else {
            s32 j = i - *(s32*)(z + 0x94);
            obj = *(u8**)(*(u8**)(z + 0x12C) + j * 4);
            e = *(u8**)(z + 0x90) + j * 0x40;
        }
        if ((*(u32*)(e + 0x1C) & 0x40) == 0) continue;
        if (obj == NULL) continue;

        u8* o9c = *(u8**)(obj + 0x9C);
        s32 v = (o9c != NULL) ? *(s32*)(o9c + 0x8) : 0;
        if (v == 0x5DB90DD2 || (v < 0x5DB90DD2 && v == 0x0C898A89)) {
            EventSend(gEventMgr, obj, NULL, 0xCA90298Eu, 0, 0, 1, 0);
        }
    }
}

// 0x800A8ED4
// Refreshes the two header widgets, then for each flagged zone object whose type code matches
// one of the known values sends the event (bit 0 of its flags is toggled around the send).
void fn_800A8ED4(u8* p) {
    void* r18 = fn_80070F4C();
    if (r18 != NULL) {
        *(void**)(p + 0x6A8) = UIWidgetFindChild(r18, 0x13902A64u);
        *(void**)(p + 0x6A4) = UIWidgetFindChild(r18, 0x7D3AC4FAu);
    }

    u8* zone0 = ZoneRecord();
    s32 total = *(s32*)(zone0 + 0x94) + *(s32*)(zone0 + 0x98);
    s32 off = 0;
    s32 off4 = 0;
    for (s32 i = 0; i < total; i++, off += 0x40, off4 += 4) {
        u8* z = ZoneRecord();
        u8* e;
        u8* obj;
        if (i < *(s32*)(z + 0x94)) {
            obj = *(u8**)(*(u8**)(z + 0x120) + off4);
            e = *(u8**)(z + 0x8C) + off;
        } else {
            s32 j = i - *(s32*)(z + 0x94);
            obj = *(u8**)(*(u8**)(z + 0x12C) + j * 4);
            e = *(u8**)(z + 0x90) + j * 0x40;
        }
        if ((*(u32*)(e + 0x1C) & 0x40) == 0) continue;
        if (obj == NULL) continue;

        // Type codes are compared as signed s32, as the asm does with cmpw.
        const s32 kA = 0x0BE4259C;
        const s32 kB = (s32)0xBAC5C3F2u;
        const s32 kC = (s32)0x861FD133u;
        const s32 kD = (s32)0xDCF3BBECu;
        const s32 kE = 0x7EABE956;
        const s32 kF = 0x6D3BDFAA;
        u8* o9c = *(u8**)(obj + 0x9C);
        s32 t = (o9c != NULL) ? *(s32*)(o9c + 0x8) : 0;
        bool hit;
        if (t == kA) {
            hit = true;
        } else if (t > kA) {
            hit = (t == kE) || (t < kE && t == kF);
        } else if (t == kB) {
            hit = true;
        } else if (t > kB) {
            hit = (t == kD);
        } else {
            hit = (t == kC);
        }
        if (!hit) continue;

        u8* q = *(u8**)(obj + 0x4);
        if ((*(u32*)(q + 0x20) & 1) == 0) {
            *(u32*)(q + 0x20) |= 1u;
            EventSend(gEventMgr, obj, NULL, 0xF15918A6u, 0, 0, 1, 0);
            u8* q2 = *(u8**)(obj + 0x4);
            *(u32*)(q2 + 0x20) &= ~1u;
        } else {
            EventSend(gEventMgr, obj, NULL, 0xF15918A6u, 0, 0, 1, 0);
        }
    }
    fn_800B0CFC(*(u8**)(p + 0xB4));
}

// 0x800AC390
// Picks a random pose from the two weight tables for the hit object, updates the timing
// fields and the orientation quaternion, and reseeds the 0x288..0x2A0 vectors.
void fn_800AC390(u8* p, f32* a, f32 dt) {
    s32 idx = RaySceneTest(p);
    s32 w1 = lbl_80346CE0[idx];
    s32 w2 = lbl_80346CD0[idx];
    *(s32*)(p + 0x268) = 0;
    s32 sum = w2 + w1;
    if (sum != 0) {
        u32 rnd = RandomU32();
        if ((s32)(rnd % (u32)sum) < w1) {
            *(s32*)(p + 0x268) = 1;
            u8* o = *(u8**)(p + 0x90);
            *(f32*)(o + 0x38) = lbl_805FDFC4;
            *(f32*)(o + 0x3C) = lbl_805FDFC0;
            *(f32*)(o + 0x40) = lbl_805FDFC0;
            *(f32*)(o + 0x44) = lbl_805FDFC4;
        }
    }

    if (*(s32*)(p + 0x268) == 0) {
        f32 c = lbl_805FDFDC;
        *(f32*)(p + 0x26C) = c + c * dt;
    } else if (*(s32*)(p + 0x268) == 1) {
        *(f32*)(p + 0x26C) = lbl_805FDFE0 + dt;
    }

    f32 c0 = lbl_805FDFC0;
    f32 c1 = lbl_805FDFC4;
    *(f32*)(p + 0x270) = c0;
    *(f32*)(p + 0x274) = c1 + lbl_805FDFE4 * dt;
    if (RandomU32() & 1) {
        *(f32*)(p + 0x274) = *(f32*)(p + 0x274) * lbl_805FDFE8;
    }

    f32 va[3] = { c1, c0, c0 };
    f32 vb[3] = { c0, c0, c1 };
    fn_801458B0((f32*)(p + 0x278), va, vb);

    u32 rnd2 = RandomU32();
    f32 angle = c0 + lbl_805FDFF0 * (lbl_805FDFEC * (f32)rnd2);
    Vec3 axis = { c0, c0, c1 };
    fn_80146180(p + 0x278, &axis, angle);

    f32 a0 = a[0];
    f32 a1 = a[1];
    f32 a2 = a[2];
    *(f32*)(p + 0x288) = a0;
    *(f32*)(p + 0x28C) = a1;
    *(f32*)(p + 0x290) = a2;
    *(s32*)(p + 0x264) = 1;
    u8* q = *(u8**)(p + 0x4);
    *(f32*)(q + 0x0) = a0;
    *(f32*)(q + 0x4) = a1;
    *(f32*)(q + 0x8) = lbl_805FDFC8;
    *(u32*)(q + 0x20) |= 0x1000C000u;
    *(f32*)(p + 0x294) = a0;
    *(f32*)(p + 0x298) = a1;
    *(f32*)(p + 0x29C) = lbl_805FDFC8;
    fn_800A7D88(p, 0x027B184Eu);
}

// 0x800ACAAC
// Hit test of the ray against the node chain at +0x8C. Returns 1 and copies the object
// position into out when the hit lies inside the radius, else 0 and zeroes out.
s32 fn_800ACAAC(u8* p, f32* a, f32* out) {
    // Node chain: p+0x8C -> type-1 node -> sub-node -> child at +0x24 (must have +0xC == 0).
    u8* node = NULL;
    u8* r6 = *(u8**)(p + 0x8C);
    if (r6 != NULL && *(s32*)(r6 + 0x10) == 1) {
        u8* x = *(u8**)(*(u8**)(r6 + 0xC));
        if (x != NULL && *(s32*)(x + 0x28) == 1) {
            node = *(u8**)(*(u8**)(x + 0x24));
            if (node == NULL || *(s32*)(node + 0xC) != 0) node = NULL;
        }
    }

    u8* q = *(u8**)(p + 0x4);
    f32 d[3];
    d[0] = a[5] - a[8];
    d[1] = a[6] - a[9];
    d[2] = a[7] - a[10];

    f32 q2 = *(f32*)(q + 0x8);
    f32 tt = q2 / lbl_805FDFD4;
    if (tt < lbl_805FDFC0) {
        tt = lbl_805FDFC0;
    } else if (tt > lbl_805FDFC4) {
        tt = lbl_805FDFC4;
    }

    // The asm reads the node radius unconditionally; a NULL node is read as 0 here.
    f32 nodeR = (node != NULL) ? *(f32*)(node + 0x3C) : 0.0f;
    f32 f30 = nodeR + tt;
    f32 f31 = a[15];

    f32 vA[3];
    vA[0] = *(f32*)(p + 0x288);
    vA[1] = *(f32*)(p + 0x28C);
    vA[2] = q2;

    f32 vB[3];
    vB[0] = (*(f32*)(p + 0x288) + lbl_805FDFC0) - *(f32*)(p + 0x288);
    vB[1] = (*(f32*)(p + 0x28C) + lbl_805FDFC0) - *(f32*)(p + 0x28C);
    f32 k26C = *(f32*)(p + 0x26C) * lbl_805FDFCC;
    f32 k270 = (lbl_805FDFCC * *(f32*)(p + 0x270)) * lbl_805FDFCC;
    f32 w4 = k26C - k270;
    vB[2] = (q2 + w4) - q2;

    f32 ret = fn_801BFE38((u8*)vA, vB, (u8*)(a + 8), (u8*)d, NULL, NULL);
    f32 sum = f31 + f30;
    if (ret <= sum * sum) {
        if (out != NULL) {
            out[0] = *(f32*)(q + 0x0);
            out[1] = *(f32*)(q + 0x4);
            out[2] = *(f32*)(q + 0x8);
        }
        return 1;
    }
    if (out != NULL) {
        out[0] = lbl_805FDFC0;
        out[1] = lbl_805FDFC0;
        out[2] = lbl_805FDFC0;
    }
    return 0;
}

// 0x800C3668
// Advances the ring of slots at +0x78 after the callback at +0x60 reports a count, updates the
// head/tail indices and notifies the object at +0xD4.
void fn_800C3668(u8* p) {
    u8* obj = *(u8**)(p + 0x60);
    s32 cnt = (*(s32 (**)(void*, void*))(*(u8**)(obj + 0xC) + 0x1C))(obj, p);
    s32 r29 = 0;
    s32 r5 = *(s32*)(p + 0x6C);

    if (!(r5 < 0 || *(s32*)(p + 0x70) < 0)) {
        s32 size = *(s32*)(p + 0x74);
        s32 ok = 0;
        if (r5 < size) {
            s32 hi = *(s32*)(p + 0x70);
            if (hi < r5) hi += size;
            ok = (r5 <= hi);
        }
        u8* slot = ok ? (*(u8**)(p + 0x78) + r5 * 4) : NULL;
        *(void**)(p + 0x7C) = slot;
        if (slot != NULL) r29 = *(s32*)slot;
    }

    fn_800C180C(cnt, r29);
    if (cnt != 0) {
        s32 size = *(s32*)(p + 0x74);
        s32 cur = *(s32*)(p + 0x6C);
        s32 rem = (*(s32*)(p + 0x70) + 1) % size;
        u8* sl = NULL;
        if (cur != rem) {
            s32 prev = cur - 1;
            if (prev < 0) prev = size - 1;
            *(s32*)(p + 0x6C) = prev;
            if (*(s32*)(p + 0x70) < 0) {
                *(s32*)(p + 0x70) = prev;
                *(s32*)(p + 0x64) = 0;
            }
            s32 c5 = *(s32*)(p + 0x6C);
            if (c5 >= 0 && c5 < *(s32*)(p + 0x74)) {
                s32 adj = *(s32*)(p + 0x70);
                if (adj < c5) adj += size;
                if (c5 <= adj) sl = *(u8**)(p + 0x78) + c5 * 4;
            }
        }
        *(void**)(p + 0x7C) = sl;
        if (sl != NULL) *(s32*)sl = cnt;
    }

    *(s32*)(p + 0x4) = cnt;
    *(s32*)(p + 0x8) = *(s32*)(p + 0x8) + 1;
    (*(void (**)(void*, s32, s32))(*(u8**)(p + 0xD4) + 0x34))(p, cnt, r29);
}

// 0x800DEFB8
// Timed blend of two vectors: while the timer in +0x5C runs, blends the stored vectors with
// a cosine ease and renormalises the quaternion; afterwards copies the rest pose into a and b.
s32 fn_800DEFB8(u8* p, f32* a, f32* b, f32 dt) {
    if (lbl_805FB1A1 != 0) {
        f32 rem = *(f32*)(p + 0x5C) - dt;
        *(f32*)(p + 0x5C) = rem;
        if (rem > lbl_805FE7D0) {
            f32 t = rem / *(f32*)(p + 0x60);
            f32 cs = cosf(lbl_805FE7D4 * (lbl_805FE7D8 * t));
            f32 s = lbl_805FE7DC * (lbl_805FE7E0 + cs);
            f32 oneMinusS = lbl_805FE7E0 - s;
            f32 W = lbl_805FE7D0 * oneMinusS + s;
            f32 Q = lbl_805FE7E0 - W;

            f32 v9 = *(f32*)(p + 0xC) * W;
            f32 v8 = *(f32*)(p + 0x10) * W;
            f32 v6 = *(f32*)(p + 0x34) * Q;
            f32 v5 = *(f32*)(p + 0x38) * Q;
            f32 v7 = *(f32*)(p + 0x14) * W;
            f32 v4 = *(f32*)(p + 0x3C) * Q;
            a[0] = v6 + v9;
            a[1] = v5 + v8;
            a[2] = v4 + v7;

            f32 qv[4];
            fn_80145F9C(qv, (f32*)(p + 0x4C), (f32*)(p + 0x24), W);
            f32 n2 = qv[0] * qv[0] + qv[1] * qv[1] + qv[2] * qv[2] + qv[3] * qv[3];
            f32 inv = 1.0f / sqrtf(n2);
            for (s32 k = 0; k < 4; k++) qv[k] = qv[k] * inv;

            Vec3 ax = QuatGetAxisY((const Quat*)qv);
            b[0] = ax.x;
            b[1] = ax.y;
            b[2] = ax.z;
        } else {
            lbl_805FB1A1 = 0;
        }
    }
    if (lbl_805FB1A1 == 0) {
        a[0] = *(f32*)(p + 0xC);
        a[1] = *(f32*)(p + 0x10);
        a[2] = *(f32*)(p + 0x14);
        b[0] = *(f32*)(p + 0x18);
        b[1] = *(f32*)(p + 0x1C);
        b[2] = *(f32*)(p + 0x20);
    }
    return 1;
}
}
