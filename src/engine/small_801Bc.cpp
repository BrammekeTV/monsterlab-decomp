// Small engine functions in 0x801B6Bxx-0x801BA6xx (overlay alpha tweens, entity draw flags,
// event queue, pad button consume, texture/buffer helpers).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  fn_801B737C(void* a, void* b, void* c, s32 d);                  // 0x801B737C
void  fn_801B75AC(void* p);                                           // 0x801B75AC
void  fn_801B7B60(void* anim, s32 mode, f32 rate);                    // 0x801B7B60
u8* fn_801B7D5C(u8* arg0, u32 a3, f32 a1, f32 a2);                // 0x801B7D5C
void  fn_801E8D74(void* out, void* list, void* owner, void* anim);    // 0x801E8D74
s32   fn_801DC9B0(void* p);                                           // 0x801DC9B0
void* EventPost(s32 id, void* sender);                                // 0x801F3918
void  fn_801EB748(void* p);                                           // 0x801EB748
u8*  fn_801EB7A4(void* p, s32 a4, s32 a5, s32 a6, s32 a7);           // 0x801EB7A4
void  fn_801EBA20(void* list, void* item);                            // 0x801EBA20 (asm reads r3 and r4)
void* fn_801E9D18(void* p, u32 hash, s32 size);                       // 0x801E9D18
s32 fn_801E9678(void* list, void* handle);                               // 0x801E9678
void  TextureRelease(void* tex);                                      // 0x80184028
void  BufferFree(void* p);                                            // 0x801D4980
void* MemAllocDefault(u32 size);                                      // 0x801D4650
void  operator_delete(void* p);                                       // 0x801D426C

extern f32 lbl_805FFB44;      // 0x805FFB44 (float, minimum overlay duration)
extern f32 lbl_805FFB2C;      // 0x805FFB2C (float, bounce rate numerator)
extern f32 lbl_805FFBB0;      // 0x805FFBB0 (float, default value for texture slot)
extern u8  lbl_80383ED0[];    // 0x80383ED0 (EventMgr vtable)
extern u8  lbl_80383E38[];    // 0x80383E38 (4 pointer-sized inputs)
extern u8  lbl_80413400[];    // 0x80413400 (4 pointer-sized results)
extern u32 lbl_80413410[];    // 0x80413410 (pad button word, 4 channels)
extern u32 lbl_80413420[];    // 0x80413420 (pad button word, 4 channels)
}

// Calls virtual slot 0x0C of obj (returns a pointer to a sub-object).
typedef void* (*VirtGetFn)(void*);
static void* CallVirtualSlotC(void* obj) {
    return (*(VirtGetFn*)(*(u8**)obj + 0xc))(obj);
}

extern "C" {

// 0x801B6BDC

// Script callback StartBouncingOverlayAlpha: starts a bouncing alpha tween on the
// overlay. The duration is clamped to at least lbl_805FFB44 and turned into a rate.
void StartBouncingOverlayAlpha(void* a0, f32 a1) {
    if (a0 == NULL) return;
    f32 duration = a1;
    if (duration <= lbl_805FFB44) duration = lbl_805FFB44;
    f32 rate = lbl_805FFB2C / duration;
    u8 tween[8];
    u8 anim[0x10];
    fn_801B7B60(anim, 1, rate);
    fn_801E8D74(tween, (u8*)a0 + 0xfc, a0, anim);
}

// 0x801B6CC8

// Script callback FadeToOverlayAlpha: starts a fade of the overlay alpha.
void FadeToOverlayAlpha(void* a0, f32 a1, f32 a2, u32 a3) {
    if (a0 == NULL) return;
    u8 tween[8];
    u8 anim[0x10];
    fn_801B7D5C(anim, a3, a1, a2);
    fn_801E8D74(tween, (u8*)a0 + 0xfc, a0, anim);
}

// 0x801B6D14

// Script callback SetOverlayAlpha: sets the alpha (+0x30) of the sub-object
// returned by the object's slot-0x0C getter.
void SetOverlayAlpha(void* a0, f32 a1) {
    if (a0 == NULL) return;
    void* sub = CallVirtualSlotC(a0);
    if (sub == NULL) return;
    void* target = CallVirtualSlotC(a0);
    *(f32*)((u8*)target + 0x30) = a1;
}

// 0x801B6E94

// Script callback WasEntityDrawnLastFrame: refreshes the entity via slot 0x0C,
// then returns bit 5 of the flag word at +0x8 of the returned sub-object.
u32 WasEntityDrawnLastFrame(void* a0) {
    void* sub = CallVirtualSlotC(a0);
    return (*(u32*)((u8*)sub + 0x8) >> 5) & 1u;
}

// 0x801B6EC8

// Posts event 0x62 for obj unless flag 0x4 of obj's sub-object (+0x4) is set,
// and stores value in the posted event (+0x4) and in the sub-object (+0x28).
void fn_801B6EC8(void* self, s32 value) {
    u8* s = (u8*)self;
    if ((*(u32*)(*(u8**)(s + 0x4) + 0x20) & 0x4) == 0) {
        u8* ev = (u8*)EventPost(0x62, self);
        if (ev != NULL) *(s32*)(ev + 0x4) = value;
    }
    u8* sub = *(u8**)(s + 0x4);
    if (sub != NULL) *(s32*)(sub + 0x28) = value;
}

// 0x801B7128

// Sets flag 0x80 in the flag word (+0x8) of the slot-0x0C sub-object.
void fn_801B7128(void* self) {
    void* sub = CallVirtualSlotC(self);
    *(u32*)((u8*)sub + 0x8) |= 0x80u;
}

// 0x801B7160

// Clears flag 0x80 in the flag word (+0x8) of the slot-0x0C sub-object.
void fn_801B7160(void* self) {
    void* sub = CallVirtualSlotC(self);
    *(u32*)((u8*)sub + 0x8) &= ~0x80u;
}

// 0x801B7410

// Initialises an object from a position (3 floats) and an integer, then runs
// the common setup. Does nothing when p4 is NULL.
void fn_801B7410(void* self, void* p4, void* p5, s32 p6, f32* pos) {
    u8* s = (u8*)self;
    if (p4 == NULL) return;
    fn_801B737C(self, p4, p5, 0);
    *(f32*)(s + 0x10) = pos[0];
    *(f32*)(s + 0x14) = pos[1];
    *(f32*)(s + 0x18) = pos[2];
    *(s32*)(s + 0xc) = p6;
    fn_801B75AC(self);
}

// 0x801B7A00

// Frees the buffer at +0x0 (if any) and clears the three words at +0x4..+0xC.
void fn_801B7A00(void* p) {
    u8* s = (u8*)p;
    void* buf = *(void**)s;
    if (buf != NULL) BufferFree(buf);
    *(u32*)(s + 0x4) = 0;
    *(u32*)(s + 0x8) = 0;
    *(u32*)(s + 0xc) = 0;
}

// 0x801B8550

// Fills the 4-entry table lbl_80413400 with fn_801DC9B0 applied to lbl_80383E38.
void fn_801B8550(void) {
    for (s32 i = 0; i < 4; i++) {
        void* in = *(void**)(lbl_80383E38 + i * 4);
        *(s32*)(lbl_80413400 + i * 4) = fn_801DC9B0(in);
    }
}

// 0x801B8A04

// Script callback StopFollowingWaypointPath: looks up the path entry with hash
// 0x80B4120F, copies its 16-bit value at +0x7C into fn_801E9678, and sets byte +0x3A of the
// owner's sub-object (+0x4) to 0xFF.
void StopFollowingWaypointPath(void* a0) {
    u8* o = (u8*)a0;
    u8* entry = (u8*)fn_801E9D18(o + 0xfc, 0x80B4120Fu, 0x70);
    if (entry != NULL) {
        u16 v = *(u16*)(entry + 0x7c);
        fn_801E9678(o + 0xfc, &v);
        u8* sub = *(u8**)(o + 0x4);
        sub[0x3a] = 0xff;
    }
}

// 0x801B9BC4

// Clears the given bits (mask) in the pad button words of channel chan.
// Channels outside 0..3 are ignored.
void PadConsumeButtons(s32 chan, u32 mask) {
    if (chan < 0 || chan >= 4) return;
    u32 keep = ~mask;
    lbl_80413420[chan] &= keep;
    lbl_80413410[chan] &= keep;
}

// 0x801B9C0C

// Sets the EventMgr vtable and clears its queue fields.
void EventMgrCtor(void* mgr) {
    u8* m = (u8*)mgr;
    *(void**)m = lbl_80383ED0;
    *(u32*)(m + 0xc) = 0;
    *(u32*)(m + 0x8) = 0;
    *(u32*)(m + 0x4) = 0;
    *(u32*)(m + 0x418) = 0;
    *(u32*)(m + 0x414) = 0;
    *(u32*)(m + 0x410) = 0;
    *(u32*)(m + 0x824) = 0;
    *(u32*)(m + 0x820) = 0;
    *(u32*)(m + 0x81c) = 0;
    *(u32*)(m + 0xc28) = 0;
}

// 0x801B9C48

// EventMgr deleting destructor: resets the vtable, frees the object when del > 0,
// and returns the pointer.
void* fn_801B9C48(void* p, s32 del) {
    if (p != NULL) {
        *(void**)p = lbl_80383ED0;
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x801B9EAC

// Clears every queued entry equal to value in the event ring: 64 entries of
// 16 bytes at +0x10, head index at +0x4, tail index at +0x8. Walks from head to tail.
void fn_801B9EAC(void* p, u32 value) {
    u8* q = (u8*)p;
    u32 head = *(u32*)(q + 0x4);
    u32 tail = *(u32*)(q + 0x8);
    s32 count;
    if (head == tail) count = 0;
    else if (head > tail) count = (s32)(0x40 - head + tail);
    else count = (s32)(tail - head);
    u32 idx = head;
    for (s32 i = 0; i < count; i++) {
        u8* entry = q + idx * 16;
        idx++;
        if (idx == 0x40) idx = 0;
        if (*(u32*)(entry + 0x10) == value) *(u32*)(entry + 0x10) = 0;
    }
}

// 0x801BA3C4

// Initialises an object: clears the buffer pointer at +0x0, allocates 0x2C bytes
// for it and sets up that buffer with fn_801EB7A4. Returns the object.
void* fn_801BA3C4(void* obj) {
    u8* o = (u8*)obj;
    *(void**)o = NULL;
    void* buf = MemAllocDefault(0x2c);
    *(void**)o = buf;
    if (buf != NULL) fn_801EB7A4(buf, 0x1c, 0x10, 0x20, 0);
    return obj;
}

// 0x801BA424

// Releases the buffer at +0x0 of an object (through fn_801EB748) and clears it.
void fn_801BA424(void* obj) {
    u8* o = (u8*)obj;
    if (*(void**)o != NULL) {
        fn_801EB748(*(void**)o);
        BufferFree(*(void**)o);
        *(void**)o = NULL;
    }
}

// 0x801BA470

// List visitor callback: removes item (second argument) from the list given as
// the first argument via fn_801EBA20, then returns 1 to keep iterating.
s32 fn_801BA470(void* a, void* b) {
    fn_801EBA20(b, a);
    return 1;
}

// 0x801BA638

// Initialises a texture slot: sets the float at +0x0 to lbl_805FFBB0 and clears
// the pointer at +0x4 and the words at +0x8 and +0xC.
// The asm has an unreachable tail at 0x801BA660..0x801BA678 (a texture-release fragment that
// nothing branches to); it is not implemented.
void fn_801BA638(void* p) {
    u8* o = (u8*)p;
    *(void**)(o + 0x4) = NULL;
    *(f32*)o = lbl_805FFBB0;
    *(u32*)(o + 0x8) = 0;
    *(u32*)(o + 0xc) = 0;
}

// 0x801BA68C

// Drops one reference on the texture at +0x4 of an object; releases the texture
// when the count reaches zero, then clears the pointer.
void fn_801BA68C(void* p) {
    u8* o = (u8*)p;
    u8* tex = *(u8**)(o + 0x4);
    if (tex != NULL) {
        s32 refs = *(s32*)(tex + 0x24) - 1;
        *(s32*)(tex + 0x24) = refs;
        if (refs == 0) TextureRelease(tex);
        *(void**)(o + 0x4) = NULL;
    }
}

}  // extern "C"
