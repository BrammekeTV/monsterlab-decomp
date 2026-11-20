// Miscellaneous functions from the second "missing" draft batch: audio pause, prop state and
// animation, UI screen script callbacks, UIText, effect lookup, camera matrix, small allocator.
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "revolution.h"
#include "engine/math.h"
#include "engine/view.h"
#include "game/character.h"

typedef void (*VFn)(void*);
typedef s32 (*VFnII)(void*, s32);

extern "C" {
// script_callbacks.h is not included: its SetPropState / PlayCharacterAnimation take u32 for pointers.
void  BufferFree(void* p);                                       // 0x801D4980
void  CharacterStartReaction(void* ch);                          // 0x8004E3D0
void* EventPost(s32 id, void* sender);                           // 0x801F3918
void  GameLog(int level, int category, const char* fmt, ...);
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);   // 0x80209E70
s32   PlaySoundAt(void* sound, int a, const void* pos, int b);   // 0x80209ABC
void  PropPlayFx(void* prop, void* fx);                          // 0x801254F8
u32   RandomU32(void);                                           // 0x80143460
void* ScreenSetFindElement(void* set, u32 hash);                 // 0x80198F18
void* ScreenSetFindWidget(void* set, u32 hash);                  // 0x80198E34
void* ScreenSetsCurrent(void* sets);                             // 0x8018D1E4
u32   StringHash(const char* s);                                 // 0x801EB704
int   SysSprintf(char* buf, const char* fmt, ...);               // 0x80142EFC
s32 TriggerDoAction(void* action, void* a1, void* a2);         // 0x801E0054
BOOL  ZoneIsLoaded(void* world, s32 zone);                       // 0x8012F408
s32  fn_8004D3F8(void* c, const char* name, s32 a, s32 b);      // 0x8004D3F8
void  fn_8004F310(void* slot, s32 idx, s32 a, s32 b);            // 0x8004F310
void  fn_800699E4(u8* prop, u32 state, f32 t);                   // 0x800699E4
void  fn_80069EEC(u8* prop);                                     // 0x80069EEC
void  fn_801225F4(u8* prop, u8* data, s32 a, s32 b);             // 0x801225F4
void  fn_8012F33C(u8* p);                                        // 0x8012F33C
void  fn_80144C60(f32* m);                                       // 0x80144C60
void  fn_80154294(void);                                         // 0x80154294
void  fn_80154318(void);                                         // 0x80154318
void  fn_8016E304(void* cam);                                    // 0x8016E304
void  fn_8016FB20(void* cam);                                    // 0x8016FB20
void  fn_80170044(void* cam);                                    // 0x80170044
void  fn_8018A78C(void* p);                                      // 0x8018A78C
void  fn_8018A948(u8* obj, f32 t);                               // 0x8018A948
void  fn_8018C3D0(void* p);                                      // 0x8018C3D0
void  fn_8018D1FC(const char* fmt, ...);                         // 0x8018D1FC (debug log)
void  fn_8018D2B8(const char* fmt, ...);                         // 0x8018D2B8 (debug log)
void  fn_8018D370(const char* fmt, ...);                         // 0x8018D370 (debug log)
void* fn_8018D428(s32 size, const char* file, void* tag, s32 line);   // 0x8018D428 (alloc)
s32 fn_801987BC(u8* arg0, u8* arg1);                      // 0x801987BC
s32   fn_80198A8C(void* set, void* widget);                      // 0x80198A8C
void* fn_80198F74(void* set, u32 hash);                          // 0x80198F74
u32   fn_8019AC00(void* obj, s32 index);                         // 0x8019AC00
void  fn_8019DB40(void* el);                                     // 0x8019DB40
void  fn_8019E27C(void* el);                                     // 0x8019E27C
s32   fn_8019EAA4(const char* text);                             // 0x8019EAA4
s32   fn_8019EB94(void* el);                                     // 0x8019EB94
s32   fn_8019F5A8(void* el);                         // 0x8019F5A8
s32 fn_801AC51C(u8* p, u8* obj, u8* target);                     // 0x801AC51C
void  fn_801ACB98(void* p);                                      // 0x801ACB98
void  fn_801B1668(void* p);                                      // 0x801B1668
void  fn_801C83AC(u8* mgr, u8* item);                            // 0x801C83AC
void  fn_801C849C(u8* mgr, u8* item);                            // 0x801C849C
void* fn_801D40E0(s32 n);                                        // 0x801D40E0
void  fn_801D6A4C(void* cam, const f32* mtx);                    // 0x801D6A4C
void  fn_801E839C(u8* p);                                        // 0x801E839C
const char* fn_801EB708(u32 hash);                               // 0x801EB708
void* fn_801EEB20(u8* pool, u32 size);                           // 0x801EEB20
void  fn_801EF070(u8* pool, void* p);                            // 0x801EF070
void  fn_801ED130(void* p);                                      // 0x801ED130
void  fn_8020A398(s32 handle);                                   // 0x8020A398
u32   fn_8020A9A8(s32 handle);                                   // 0x8020A9A8
void* fn_80209814(void* v);                                      // 0x80209814
BOOL SpawnEffectAtPos(u8* mgr, u8* effect, const f32* pos, u8** owner);   // 0x801C81D8

extern "C++" void* gScreenSets;        // 0x805FB654
extern "C++" s32   gZoneResourceSize;  // 0x805FB94C
extern "C++" View* gMainView;          // 0x805FB75C
extern u8   gWorldState[];             // 0x803E6D00
extern u8   lbl_80355940[];            // 0x80355940 (4 sound handles)
extern const u32 lbl_80387F68[];       // 0x80387F68 (0x92 animation name hashes)
extern const char* const lbl_80359008[];   // 0x80359008 (null-terminated animation name table)
extern const char lbl_80381868[];      // 0x80381868 (UI script message strings)
extern const char lbl_8038191C[];      // 0x8038191C (UIScreen_Pop: no active screen set)
extern const char lbl_80381974[];      // 0x80381974 (UIScreen_Pop trace)
extern const char lbl_80381D68[];      // 0x80381D68 (textureList not found)
extern const char lbl_80381E68[];      // 0x80381E68 (textureList index not found)
extern u8   lbl_8047C760[];            // 0x8047C760 (effect manager)
extern u8   lbl_8047CAC0[];            // 0x8047CAC0 (small-block pool)
extern u8   lbl_8047D318[];            // 0x8047D318 (camera matrix copy, 12 floats)
extern u8   lbl_8051E6A8[];            // 0x8051E6A8 (sound voices, 0x114 each)
extern u8   lbl_80524E28[];            // 0x80524E28 (sound table entries, 0x58 each, 0x60 of them)
extern u8   lbl_805F9212;              // 0x805F9212 (copy filter value)
extern u32  lbl_805F9614;              // 0x805F9614
extern f32  lbl_805F9408;              // 0x805F9408 (45.0f)
extern u8   lbl_805F940C;              // 0x805F940C (allocation tag)
extern u8*  lbl_805FB134;              // 0x805FB134 (experiment object)
extern u8   lbl_805FB50C;              // 0x805FB50C (GX shadow flags byte)
extern void* lbl_805FB5F8;             // 0x805FB5F8
extern s32  lbl_805FB7B0;              // 0x805FB7B0 (alloc count, high word)
extern s32  lbl_805FB7B4;              // 0x805FB7B4 (alloc count, low word)
extern s32  lbl_805FB7B8;              // 0x805FB7B8 (free count, high word)
extern s32  lbl_805FB7BC;              // 0x805FB7BC (free count, low word)
extern s32  lbl_805FB7C0;              // 0x805FB7C0 (allocated bytes, high word)
extern s32  lbl_805FB7C4;              // 0x805FB7C4 (allocated bytes, low word)
extern u8   lbl_805FB7A8[];            // 0x805FB7A8 (flags, second word at +4: spare pool enabled)
extern s32  lbl_805FBABC;              // 0x805FBABC (count of unpaused looping voices)
extern s32  lbl_805FBAE0;              // 0x805FBAE0 (audio busy flag)
extern void* lbl_805FBBAC;             // 0x805FBBAC (voice SFX handle)
extern void* lbl_805FB964;             // 0x805FB964 (movie player)
extern s32  lbl_805FB970;              // 0x805FB970 (movie player active flag)
extern u8   lbl_80412F90[];            // 0x80412F90 (log message buffer)
}

// Sound hashes of the two ambient chatter sounds (names unresolved).
static const u32 kHashChatterA = 0x06112941u;
static const u32 kHashChatterB = 0x488AF532u;

extern "C" {

// 0x801FF708
// Pauses (arg1 != 0) or resumes every active voice in the sound table. Pausing bumps the pause
// counter at +0x2B0 and stamps it into each voice's +0x110; resuming drops the counter and releases
// voices stamped with a higher value. Looping voices (+0xFC and +0xFD) adjust lbl_805FBABC, and
// the voice state at +8 is remapped (4->3, 5->2, 0->1 when pausing; 1->0, 2->5, 3->4 on resume).
void AudioSetPaused(u8* arg0, s32 arg1) {
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    *(u8*)(arg0 + 0x2B8) = (u8)arg1;
    if (arg1 != 0) {
        *(s32*)(arg0 + 0x2B0) = *(s32*)(arg0 + 0x2B0) + 1;
        u8* entry = lbl_80524E28;
        s32 loops = lbl_805FBABC;
        for (s32 i = 0; i < 0x60; i++, entry += 0x58) {
            if (*(s32*)(entry + 0x0) == -1) continue;
            u8* v = lbl_8051E6A8 + *(s32*)(entry + 0x8) * 0x114;
            if (*(s32*)(v + 0x110) != 0) continue;
            *(s32*)(v + 0x110) = *(s32*)(arg0 + 0x2B0);
            if (*(u8*)(v + 0xFC) != 0 && *(u8*)(v + 0xFD) != 0) {
                loops -= 1;
                *(u8*)(v + 0xFD) = 0;
                lbl_805FBABC = loops;
                if (loops < 0) {
                    lbl_805FBABC = 0;
                    loops = 0;
                }
            }
            s32 state = *(s32*)(v + 0x8);
            if (state == 4)      *(s32*)(v + 0x8) = 3;
            else if (state == 5) *(s32*)(v + 0x8) = 2;
            else if (state == 0) *(s32*)(v + 0x8) = 1;
        }
    } else {
        s32 count = *(s32*)(arg0 + 0x2B0);
        if (count > 0) *(s32*)(arg0 + 0x2B0) = count - 1;
        u8* entry = lbl_80524E28;
        s32 loops = lbl_805FBABC;
        for (s32 i = 0; i < 0x60; i++, entry += 0x58) {
            if (*(s32*)(entry + 0x0) == -1) continue;
            u8* v = lbl_8051E6A8 + *(s32*)(entry + 0x8) * 0x114;
            if (!((u32)*(s32*)(arg0 + 0x2B0) < *(u32*)(v + 0x110))) continue;
            *(s32*)(v + 0x110) = 0;
            if (*(u8*)(v + 0xFC) != 0 && *(u8*)(v + 0xFD) == 0) {
                loops += 1;
                *(u8*)(v + 0xFD) = 1;
                lbl_805FBABC = loops;
            }
            s32 state = *(s32*)(v + 0x8);
            if (state == 1)      *(s32*)(v + 0x8) = 0;
            else if (state == 2) *(s32*)(v + 0x8) = 5;
            else if (state == 3) *(s32*)(v + 0x8) = 4;
        }
    }
    lbl_805FBAE0 = saved;
}

// 0x80069350
// Starts anim `anim` (clamped to >= 0) of the prop's anim def table. a2 != 0 sets prop flag 0x80
// and clears bit 2 of the sub-object flags, otherwise the reverse; a3 != 0 also clears bit 4 of the
// animation object's +0xF0 and calls fn_8018C3D0. Logs (into a stack buffer) when the table or
// the anim is missing.
void PropPlayAnim(u8* prop, s32 anim, s32 a2, s32 a3) {
    char buf[0x58];
    u8* def = *(u8**)(prop + 0x9C);
    u8* table = *(u8**)(def + 0x48);
    if (table == NULL) {
        SysSprintf(buf, "ERROR - attempt to animate prop (%s) without anim def table ...\n", prop + 0x108);
        return;
    }
    if (anim < 0) anim = 0;
    u8* clip = *(u8**)(table + 0xC) + anim * 0x44;
    if (clip == NULL) {
        SysSprintf(buf, "ERROR - unable to find anim [%d] for prop (%s) ...\n", anim, prop + 0x108);
        return;
    }
    *(u8*)(*(u8**)(prop + 0x4) + 0x39) = (u8)anim;
    *(u32*)(prop + 0x15C) &= ~1u;
    fn_801AC51C(clip, *(u8**)(prop + 0xA0), prop);
    if (a2 != 0) {
        *(u32*)(prop + 0x15C) |= 0x80;
        *(u32*)(*(u8**)(prop + 0x4) + 0x2C) &= ~2u;
        *(s32*)(*(u8**)(prop + 0xA0) + 0xF0) = 0;
    } else {
        *(u32*)(prop + 0x15C) &= ~0x80u;
        *(u32*)(*(u8**)(prop + 0x4) + 0x2C) |= 2;
        *(s32*)(*(u8**)(prop + 0xA0) + 0xF0) = 8;
    }
    if (a3 != 0) {
        u8* a = *(u8**)(prop + 0xA0);
        *(u32*)(a + 0xF0) &= ~0x10u;
        fn_8018C3D0(*(void**)(prop + 0xA0));
    }
    fn_8018A948(*(u8**)(prop + 0xA0), 0.0f);   // lbl_805FD4E8
}

// 0x801452A8
// Y axis (second column) of the rotation quaternion. (The original also builds the full matrix
// in dead stack slots.)
Vec3 QuatGetAxisY(const Quat* q) {
    f32 x = q->x, y = q->y, z = q->z, w = q->w;
    f32 x2 = 2.0f * x;
    f32 y2 = 2.0f * y;
    f32 z2 = 2.0f * z;
    f32 xx = x2 * x;
    f32 zz = z2 * z;
    f32 xw = x2 * w;
    f32 zy = z2 * y;
    f32 yx = y2 * x;
    f32 zw = z2 * w;
    Vec3 r;
    r.z = zy + xw;
    r.y = 1.0f - (xx + zz);
    r.x = yx - zw;
    return r;
}

// 0x8018D370
// Debug log (printf style): formats into the shared message buffer lbl_80412F90, then logs it
// with GameLog(4, 1, "%s", buf). The original builds its va_list from the register save area.
void fn_8018D370(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsprintf((char*)lbl_80412F90, fmt, ap);
    va_end(ap);
    GameLog(4, 1, "%s", (const char*)lbl_80412F90);   // lbl_805F9350
}

// 0x801D6814
// Stores the 3x4 camera matrix into lbl_8047D318 and applies it to the main view. Unless
// lbl_805F9614 is set (then fn_801D6A4C does it), the matrix is rearranged (rotation rows swapped
// into fn_80144C60's layout), negated in the 2x3 axis blocks, copied into the view at +0x00..+0x2C
// and fn_8016E304 is run. Afterwards the view is refreshed and two BP register writes are sent.
void CameraSetMatrix(void* camera, const f32 mtx[3][4]) {
    const f32* m = (const f32*)mtx;
    f32 v[12];
    for (s32 i = 0; i < 12; i++) v[i] = m[i];
    f32* dst = (f32*)lbl_8047D318;
    for (s32 i = 0; i < 12; i++) dst[i] = v[i];
    u8* cam = (u8*)gMainView;
    if (lbl_805F9614 != 0) {
        fn_801D6A4C(cam, dst);
    } else {
        f32 t[12];
        t[0] = v[3];  t[1] = v[4];  t[2] = v[5];
        t[3] = v[6];  t[4] = v[7];  t[5] = v[8];
        t[6] = v[0];  t[7] = v[1];  t[8] = v[2];
        t[9] = v[9];  t[10] = v[10]; t[11] = v[11];
        fn_80144C60(t);
        t[0] = -t[0]; t[1] = -t[1]; t[2] = -t[2];
        t[6] = -t[6]; t[7] = -t[7]; t[8] = -t[8];
        f32* out = (f32*)cam;
        for (s32 i = 0; i < 12; i++) out[i] = t[i];
        fn_8016E304(cam);
    }
    if (lbl_805FB5F8 != NULL) fn_80170044(gMainView);
    fn_8016FB20(gMainView);
    fn_80154294();
    fn_80154318();
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0xFE000FE0u;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0x410004A0u;
    lbl_805FB50C |= 8;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = 0xFE00C000u;
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = (u32)lbl_805F9212 << 14;
}

// 0x801D41CC
// Small-block allocator: bumps the 64-bit allocation count and byte total, then allocates from
// the primary pool, falling back to the spare pool (pool + 0x394) when enabled.
void* MemAllocSmall(u32 size) {
    u32 cntLo = (u32)lbl_805FB7B4;
    u32 cntHi = (u32)lbl_805FB7B0;
    u32 newCntLo = cntLo + 1;
    u32 newCntHi = cntHi + (newCntLo < cntLo ? 1u : 0u);
    u32 byteLo = (u32)lbl_805FB7C4;
    u32 byteHi = (u32)lbl_805FB7C0;
    u32 newByteLo = byteLo + size;
    u32 newByteHi = byteHi + (newByteLo < byteLo ? 1u : 0u);
    lbl_805FB7B4 = (s32)newCntLo;
    lbl_805FB7B0 = (s32)newCntHi;
    lbl_805FB7C4 = (s32)newByteLo;
    lbl_805FB7C0 = (s32)newByteHi;
    void* p = fn_801EEB20(lbl_8047CAC0, size);
    if (p == NULL && *(s32*)(lbl_805FB7A8 + 4) != 0) {
        p = fn_801EEB20(lbl_8047CAC0 + 0x394, size);
    }
    return p;
}

// 0x801D4340
// Small-block free: bumps the 64-bit free count, ignores NULL, and releases p to whichever pool
// (primary, or spare at +0x394 when enabled) owns the address range.
void MemFreeSmall(void* p) {
    u32 cntLo = (u32)lbl_805FB7BC;
    u32 cntHi = (u32)lbl_805FB7B8;
    u32 newLo = cntLo + 1;
    lbl_805FB7BC = (s32)newLo;
    lbl_805FB7B8 = (s32)(cntHi + (newLo < cntLo ? 1u : 0u));
    if (p == NULL) return;
    u8* a = (u8*)p;
    if (a >= *(u8**)(lbl_8047CAC0 + 0x37C) - 0x10 && a < *(u8**)(lbl_8047CAC0 + 0x384)) {
        fn_801EF070(lbl_8047CAC0, p);
        return;
    }
    if (*(s32*)(lbl_805FB7A8 + 4) == 0) return;
    if (a >= *(u8**)(lbl_8047CAC0 + 0x710) - 0x10 && a < *(u8**)(lbl_8047CAC0 + 0x718)) {
        fn_801EF070(lbl_8047CAC0 + 0x394, p);
    }
}

// 0x8004D604
// Script callback: plays the animation whose name hash is `anim` on the fighter. A hash found in
// the table lbl_80387F68 (0x92 entries) is started on each of the five limb slots at +0xC50 that
// are set (fn_8004F310 gets the table index) and then the reaction is started; otherwise the
// string table lbl_80359008 is searched by StringHash and the match is played via fn_8004D3F8.
void PlayCharacterAnimation(Character* c, int a, u32 anim, int loop) {
    u8* ch = (u8*)c;
    for (s32 idx = 0; idx < 0x92; idx++) {
        if (anim == lbl_80387F68[idx]) {
            for (s32 i = 0; i < 5; i++) {
                void* slot = *(void**)(ch + 0xC50 + i * 4);
                if (slot != NULL) fn_8004F310(slot, idx, a, loop);
            }
            CharacterStartReaction(ch);
            return;
        }
    }
    for (s32 i = 0; lbl_80359008[i] != NULL; i++) {
        if (anim == StringHash(lbl_80359008[i])) {
            fn_8004D3F8(ch, lbl_80359008[i], a, loop);
            return;
        }
    }
}

// 0x8006BB3C
// Script callback: sets prop state `state` (byte at sub+0x35). Posts event 0x1F unless the sub flag
// 0x04000000 is set. If the state changes: state 0 with a2 == 0 plays the def's fx and sound,
// releases the effect and sound handle; otherwise only releases them. A state below the limit at
// +0x1A4 triggers the enter/leave actions and fn_800699E4 (returns 1); otherwise fn_80069EEC runs.
// Returns the old state when unchanged or out of range.
ScriptValue SetPropState(u8* prop, u32 state, s32 a2) {
    u8* sub = *(u8**)(prop + 0x4);
    if (!(*(u32*)(sub + 0x20) & 0x04000000)) {
        u8* ev = (u8*)EventPost(0x1F, prop);
        if (ev != NULL) {
            *(u32*)(ev + 0x4) = state;
            *(s32*)(ev + 0x8) = a2;
        }
    }
    u8 old = *(u8*)(*(u8**)(prop + 0x4) + 0x35);
    if (old == state) return (ScriptValue)old;

    if (a2 == 0) {
        u8* def = *(u8**)(prop + 0x9C);
        if (def != NULL) {
            void* fx = *(void**)(def + 0xB4);
            if (fx != NULL) PropPlayFx(prop, fx);
            if (lbl_805FBBAC == NULL) {
                void* v = fn_801D40E0(1);
                if (v != NULL) v = fn_80209814(v);
                lbl_805FBBAC = v;
            }
            PlaySoundAt(*(void**)(def + 0xB8), 0, *(u8**)(prop + 0x4), 0);
            s8 volume = (s8)*(u8*)(def + 0xC0);
            if (volume > 0) fn_801225F4(prop, *(u8**)(prop + 0x4), 0, volume);
            u8* item = *(u8**)(prop + 0xA8);
            if (item != NULL) {
                fn_801C849C(lbl_8047C760, item);
                *(void**)(prop + 0xA8) = NULL;
            }
            fn_801ED130(prop + 0x94);
            s32 handle = *(s32*)(prop + 0xAC);
            if (handle != -1) {
                fn_8020A398(handle);
                *(s32*)(prop + 0xAC) = -1;
            }
        }
    } else {
        u8* item = *(u8**)(prop + 0xA8);
        if (item != NULL) {
            fn_801C849C(lbl_8047C760, item);
            *(void**)(prop + 0xA8) = NULL;
        }
        s32 handle = *(s32*)(prop + 0xAC);
        if (handle != -1) {
            fn_8020A398(handle);
            *(s32*)(prop + 0xAC) = -1;
        }
    }

    if (state < *(u32*)(prop + 0x1A4)) {
        f32 t = 0.0f;   // lbl_805FD4E8
        if (*(s32*)(prop + 0xA0) != 0) {
            t = (f32)*(u8*)(*(u8**)(prop + 0x4) + 0x38) / 255.0f;   // lbl_805FD4F0
        }
        u8* def = *(u8**)(prop + 0x9C);
        if (def != NULL && ((*(u32*)(def + 0x94) >> 27) & 0xF)) TriggerDoAction(def + 0x94, prop, NULL);
        if ((*(u32*)(prop + 0x64) >> 27) & 0xF) TriggerDoAction(prop + 0x64, prop, NULL);
        fn_800699E4(prop, state, t);
        def = *(u8**)(prop + 0x9C);
        if (def != NULL && ((*(u32*)(def + 0x98) >> 27) & 0xF)) TriggerDoAction(def + 0x98, prop, NULL);
        if ((*(u32*)(prop + 0x68) >> 27) & 0xF) TriggerDoAction(prop + 0x68, prop, NULL);
        *(f32*)(*(u8**)(prop + 0x4) + 0x24) = *(f32*)(*(u8**)(prop + 0x9C) + 0x24);
        *(f32*)(prop + 0x158) = *(f32*)(*(u8**)(prop + 0x9C) + 0x24);
        return 1;
    }
    fn_80069EEC(prop);
    return (ScriptValue)old;
}

// 0x8018F000
// Script callback: pops the UI screen with hash a0 off the current screen set (logs when there is
// no active screen set; does nothing when the widget is not found).
void UIScreen_PopScreen(u32 a0) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D2B8(lbl_8038191C);
        return;
    }
    fn_8018D1FC(lbl_80381974);
    void* w = ScreenSetFindWidget(set, a0);
    if (w != NULL) fn_80198A8C(set, w);
}

// 0x8018EEB4
// Script callback: pushes the UI screen with hash a0 onto the current screen set. Logs when there
// is no active screen set, or the widget does not exist (with its name).
void UIScreen_Push(u32 a0) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D2B8(lbl_80381868);
        return;
    }
    void* w;
    void* set2 = ScreenSetsCurrent(gScreenSets);
    if (set2 == NULL) {
        fn_8018D2B8(lbl_80381868 + 0x40);
        w = NULL;
    } else {
        w = ScreenSetFindWidget(set2, a0);
        if (w == NULL) {
            fn_8018D370(lbl_80381868 + 0x68, fn_801EB708(a0));
            w = NULL;
        }
    }
    if (w != NULL) {
        fn_8018D1FC(lbl_80381868 + 0x94, a0);
        fn_801987BC((u8*)set, (u8*)w);
    }
}

// 0x8019EFC8
// Sets the text of a UIText element (el + 0x170 buffer, length at +0x150, capacity at +0x168).
// Skips work when the same string is set again on an element with flag 0x10000 and a == 0.
// Allocates the buffers on first use (capacity = length + 1, max 0x1000; 0x1E for NULL text),
// chops over-long strings with a log, copies the text, then sends the layout message (vtable
// slot 0x2C or 0x30 of the object at +0x138) and refreshes the screen set. (The asm leaves 0 in
// r3 on both exits; every caller treats it as void.)
void UISetText(void* elp, const char* text, s32 a) {
    u8* el = (u8*)elp;
    *(s32*)(el + 0x1E8) = 0;
    if (*(u32*)(el + 0x64) & 0x10000) {
        *(f32*)(el + 0x14C) = 0.0f;   // lbl_805FF958
    } else {
        *(f32*)(el + 0x14C) = 1.0f + ((f32)*(s32*)(el + 0x150) / lbl_805F9408);   // lbl_805FF95C
    }
    if (a == 0 && *(const char**)(el + 0x174) == text && (*(u32*)(el + 0x64) & 0x10000)) {
        if (*(u32*)(el + 0x1EC) > 1) fn_8019DB40(el);
        return;
    }
    char* buf = *(char**)(el + 0x170);
    *(const char**)(el + 0x174) = text;
    if (buf == NULL) {
        if (text != NULL) {
            s32 len = fn_8019EAA4(text);
            *(s32*)(el + 0x150) = len;
            if (len >= 0x1000) *(s32*)(el + 0x150) = 0xFFF;
            *(s32*)(el + 0x168) = *(s32*)(el + 0x150) + 1;
            char* b = (char*)fn_8018D428(*(s32*)(el + 0x150) + 1, "UIText.cpp", &lbl_805F940C, 0x4E7);
            *(char**)(el + 0x170) = b;
            memset(b, 0, *(s32*)(el + 0x168));
            *(void**)(el + 0x178) = fn_8018D428(*(s32*)(el + 0x168) * 4, "UIText.cpp", &lbl_805F940C, 0x4E9);
        } else {
            *(s32*)(el + 0x150) = 0x1D;
            *(s32*)(el + 0x168) = 0x1E;
            char* b = (char*)fn_8018D428(0x1E, "UIText.cpp", &lbl_805F940C, 0x4F0);
            *(char**)(el + 0x170) = b;
            memset(b, 0, *(s32*)(el + 0x168));
            *(void**)(el + 0x178) = fn_8018D428(*(s32*)(el + 0x168) * 4, "UIText.cpp", &lbl_805F940C, 0x4F2);
        }
        if (*(void**)(el + 0x1E0) == NULL) {
            *(void**)(el + 0x1E0) = fn_8018D428(0x200, "UIText.cpp", &lbl_805F940C, 0x4F7);
        }
    } else if (text == NULL) {
        *buf = 0;
        *(s32*)(el + 0x150) = 0;
        *(s32*)(el + 0x1EC) = 1;
    } else {
        s32 len = fn_8019EAA4(text);
        if (len > *(s32*)(el + 0x168) - 1) {
            fn_8018D2B8("UIText : set string chopped \"%s\" to %d chars due to buffer overflow ...\n", text);
            *(s32*)(el + 0x150) = *(s32*)(el + 0x168) - 1;
        } else {
            *(s32*)(el + 0x150) = len;
        }
    }
    if (text != NULL) {
        s32 cap = *(s32*)(el + 0x168);
        char* b = *(char**)(el + 0x170);
        strncpy(b, text, cap - 1);
        b[cap - 1] = 0;
        *(u8*)(el + 0x1F0) = 1;
        fn_8019E27C(el);
    } else {
        **(char**)(el + 0x170) = 0;
    }
    if (fn_8019EB94(el) != 0) {
        (*(VFnII*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x20000);
    } else {
        (*(VFnII*)(*(u8**)(el + 0x138) + 0x30))(el, 0x20000);
    }
    *(s32*)((u8*)gScreenSets + 0x6C) = 1;
    fn_8019F5A8(el);
}

// 0x80194944
// Script callback: texture `index` of the textureList widget `hash` in the current screen set.
// Logs and returns 0 when the list or the texture does not exist.
ScriptValue UITextureList_GetTexture(u32 hash, s32 index) {
    void* list = fn_80198F74(ScreenSetsCurrent(gScreenSets), hash);
    if (list == NULL) {
        fn_8018D370(lbl_80381D68, hash);
        return 0;
    }
    u32 tex = fn_8019AC00(list, index);
    if (tex == 0) {
        fn_8018D370(lbl_80381E68, index);
        return 0;
    }
    return (ScriptValue)tex;
}

// Shared test of fn_80022E10 / fn_80022F28: the experiment object (only if its tag at +0 is 0)
// must have the float at +0x6C >= 2.0 and none of the four sound handles may still be playing.
// (The original reads +0x6C even through a NULL object; NULL is treated as "not ready" here.)
static BOOL ChatterReady(void) {
    u8* obj = lbl_805FB134;
    if (obj != NULL && *(s32*)obj != 0) obj = NULL;
    if (obj == NULL || !(*(f32*)(obj + 0x6C) >= 2.0f)) return 0;   // lbl_805FC908
    for (s32 i = 0; i < 4; i++) {
        if (fn_8020A9A8(*(s32*)(lbl_80355940 + i * 4)) != 0) return 0;
    }
    return 1;
}

// 0x80022E10
// Occasionally (67% chance per call) starts ambient chatter sound A into handle slot 0.
void fn_80022E10(void) {
    if (ChatterReady()) {
        u32 r = RandomU32();
        if ((f32)(s32)(r % 100 + 1) < 67.0f) {   // lbl_805FC918
            *(s32*)(lbl_80355940 + 0x0) = PlaySound(kHashChatterA, 0, 0, 0, 0, 0);
        }
    }
}

// 0x80022F28
// Same as fn_80022E10 with chatter sound B into handle slot 1.
void fn_80022F28(void) {
    if (ChatterReady()) {
        u32 r = RandomU32();
        if ((f32)(s32)(r % 100 + 1) < 67.0f) {   // lbl_805FC918
            *(s32*)(lbl_80355940 + 0x4) = PlaySound(kHashChatterB, 0, 0, 0, 0, 0);
        }
    }
}

// 0x80116D40
// Reads the positions of two UI elements (hashes arg1, arg2) from the current screen set into the
// record at arg0: first element's x/y/z at +0/+4/+8 (y moved to the element's lower edge
// y + h * (1 - anchor)), second element's at +0xC/+0x10/+0x14 (y at y - h * anchor), and the
// vertical distance between the two y values at +0x28.
void fn_80116D40(u8* arg0, u32 arg1, u32 arg2) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    u8* e1 = (u8*)ScreenSetFindElement(set, arg1);
    u8* e2 = (u8*)ScreenSetFindElement(set, arg2);
    if (e1 != NULL) {
        *(f32*)(arg0 + 0x0) = *(f32*)(e1 + 0x68);
        *(f32*)(arg0 + 0x4) = *(f32*)(e1 + 0x6C);
        *(f32*)(arg0 + 0x8) = *(f32*)(e1 + 0x70);
        *(f32*)(arg0 + 0x4) = *(f32*)(e1 + 0x78) * (1.0f - *(f32*)(e1 + 0xA0)) + *(f32*)(e1 + 0x6C);   // lbl_805FECD4 = 1.0f
    }
    if (e2 != NULL) {
        *(f32*)(arg0 + 0xC)  = *(f32*)(e2 + 0x68);
        *(f32*)(arg0 + 0x10) = *(f32*)(e2 + 0x6C);
        *(f32*)(arg0 + 0x14) = *(f32*)(e2 + 0x70);
        *(f32*)(arg0 + 0x10) = *(f32*)(e2 + 0x6C) - *(f32*)(e2 + 0x78) * *(f32*)(e2 + 0xA0);
    }
    *(f32*)(arg0 + 0x28) = *(f32*)(arg0 + 0x10) - *(f32*)(arg0 + 0x4);
}

// 0x801B4520
// Tears down an object's owned resources: base cleanup of the sub-record at +0xFC, then free the
// buffers at +0x8C, +0x90 (virtual slot 0x10 first), +0xA4, release the item at +0xA8 and the
// handle at +0xAC (-1 when none), destroy and free every node of the list at +0xB0 (node +4 is
// the object, virtual slot 0x40, node +8 the next), run fn_8012F33C, free +0xA0 and clear +4.
void fn_801B4520(u8* arg0) {
    fn_801E839C(arg0 + 0xFC);
    if (*(void**)(arg0 + 0x8C) != NULL) {
        fn_801B1668(*(void**)(arg0 + 0x8C));
        BufferFree(*(void**)(arg0 + 0x8C));
        *(void**)(arg0 + 0x8C) = NULL;
    }
    void* obj = *(void**)(arg0 + 0x90);
    if (obj != NULL) {
        (*(VFn*)(*(u8**)obj + 0x10))(obj);
        BufferFree(*(void**)(arg0 + 0x90));
        *(void**)(arg0 + 0x90) = NULL;
    }
    if (*(void**)(arg0 + 0xA4) != NULL) {
        fn_801ACB98(*(void**)(arg0 + 0xA4));
        BufferFree(*(void**)(arg0 + 0xA4));
        *(void**)(arg0 + 0xA4) = NULL;
    }
    u8* item = *(u8**)(arg0 + 0xA8);
    if (item != NULL) {
        fn_801C83AC(lbl_8047C760, item);
        *(void**)(arg0 + 0xA8) = NULL;
    }
    s32 handle = *(s32*)(arg0 + 0xAC);
    if (handle != -1) {
        fn_8020A398(handle);
        *(s32*)(arg0 + 0xAC) = -1;
    }
    u8* node = *(u8**)(arg0 + 0xB0);
    while (node != NULL) {
        u8* cur = node;
        node = *(u8**)(cur + 0x8);
        void* o = *(void**)(cur + 0x4);
        (*(VFn*)(*(u8**)o + 0x40))(o);
        BufferFree(o);
        BufferFree(cur);
    }
    *(void**)(arg0 + 0xB0) = NULL;
    fn_8012F33C(arg0);
    if (*(void**)(arg0 + 0xA0) != NULL) {
        fn_8018A78C(*(void**)(arg0 + 0xA0));
        BufferFree(*(void**)(arg0 + 0xA0));
        *(void**)(arg0 + 0xA0) = NULL;
    }
    if (*(s32*)(arg0 + 0x4) != 0) *(s32*)(arg0 + 0x4) = 0;
}

// 0x801B7198
// Resolves a packed 20-bit handle (word at arg0) to a pointer in the world's zone resources:
// bit 19 = valid, bit 18 selects the table (+0x94 count / +0x120 array, or +0x98 / +0x12C),
// the zone index is bits 16-17 (or bits 7-15 when those are 3) and the entry index bits 2-15
// (or bits 2-6). Returns NULL when invalid, the zone is not loaded or the index is out of range.
void* fn_801B7198(void* arg0, s32 arg1) {
    u32 w = *(u32*)arg0;
    if (!((w >> 19) & 1)) return NULL;
    u32 mode = (w >> 16) & 3;
    u32 zone = mode;
    if (mode == 3) zone = (w >> 7) & 0x1FF;
    s32 index = (w >> 2) & 0x3FFF;
    if (mode == 3) index = (w >> 2) & 0x1F;
    if (*(u8*)(gWorldState + 4) != 0 && ZoneIsLoaded(gWorldState, (s32)zone) != 0) {
        u8* z = *(u8**)(gWorldState + 0x1A4) + zone * gZoneResourceSize;
        if ((w >> 18) & 1) {
            if (index < *(s32*)(z + 0x94)) return *(void**)(*(u8**)(z + 0x120) + index * 4);
        } else {
            if (index < *(s32*)(z + 0x98)) return *(void**)(*(u8**)(z + 0x12C) + index * 4);
        }
    }
    return NULL;
}

// 0x801C80E0
// Looks up the effect record with id `effect` in the manager's effect table (manager +0x1C ->
// table +0x10, count +0xC, 0xEC bytes per record, id in the first word; binary search when the
// table byte +0x14 says it is sorted, else linear) and spawns it at pos. Does nothing when the
// id is not found. (The asm returns SpawnEffectAtPos' pointer, or 0; declared void like its callers.)
void SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], int a) {
    if (effect == 0) return;
    u8* db = *(u8**)((u8*)fxMgr + 0x1C);
    s32 count = *(s32*)(db + 0xC);
    s32 found;
    if (*(u8*)(db + 0x14) != 0) {
        s32 lo = -1;
        s32 hi = count;
        while (hi - lo > 1) {
            s32 mid = (hi + lo) / 2;
            u32 key = *(u32*)(*(u8**)(db + 0x10) + mid * 0xEC);
            if (effect <= key) hi = mid;
            if (effect > key) lo = mid;
        }
        // The original reads the record at `hi` even when hi == count; that is guarded here.
        if (hi < count && effect == *(u32*)(*(u8**)(db + 0x10) + hi * 0xEC)) found = hi;
        else found = -1;
    } else {
        found = -1;
        for (s32 i = 0; i < count; i++) {
            if (effect == *(u32*)(*(u8**)(db + 0x10) + i * 0xEC)) {
                found = i;
                break;
            }
        }
    }
    u8* rec = NULL;
    if (found >= 0) rec = *(u8**)(db + 0x10) + found * 0xEC;
    if (rec == NULL) return;
    SpawnEffectAtPos((u8*)fxMgr, (u8*)rec, pos, (u8**)a);   // lint:ok tail call; the original returns its result (callers ignore it)
}

// 0x801F5220
// While the movie player is active, returns its virtual slot 0x38 (called on the player);
// otherwise 1.
BOOL fn_801F5220(void) {
    if (lbl_805FB970 != 0) {
        return (*(BOOL (**)(void*))(*(u8**)lbl_805FB964 + 0x38))(lbl_805FB964);
    }
    return 1;
}

}  // extern "C"
