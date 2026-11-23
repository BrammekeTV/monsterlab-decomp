// Tiny functions (<= 0x40 bytes) from the miss551 batch. Written by hand from the asm.
#include "types.h"

extern f32 lbl_805FC6C4;
extern f32 lbl_805FC6C8;
extern f32 lbl_805FD300;
extern f32 lbl_805FD304;
extern f32 lbl_805FD934;
extern f32 lbl_805FD940;
extern f32 lbl_805FDC1C;
extern f32 lbl_805FDF1C;
extern f32 lbl_805FDF60;
extern f32 lbl_805FE258;
extern "C++" void* gEventMgr;
extern "C" u8* fn_800C52F0(u8* p, s32 del);  // 0x800C52F0 (C++ linkage in small_800C.cpp)

extern "C" {
extern u8 lbl_803C49A8[];
extern u8 lbl_803C529C[];
extern u8 lbl_803C54D4[];
extern u8 lbl_803C5498[];
extern u8 lbl_805FAEF0[];
extern u8 lbl_80347440[];
extern u8 lbl_80381370[];

u8* fn_80142500(void* p);                                    // 0x80142500
void fn_8032F1E8(u8* obj, void (*cb)(void), s32 a, s32 b, s32 c);  // 0x8032F1E8
void fn_80052638(void);                                   // 0x80052638
void StopExperimentSound(void* obj, u32 id);                // (see src/leaf/auto_03_80099AD8_text.cpp)
void GameFlowSignal(s32 id, s32 arg1, s32 arg2);            // 0x8000C7F4
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
u32 fn_800C2618(u8* a3, s32 a4);                            // 0x800C2618
u8* fn_800C2028(u8* p);                                     // 0x800C2028 (defined in draft_800Cd.cpp)
void operator_delete(void* p);                              // 0x801D426C
void fn_8018D4CC(void* p);                                  // 0x8018D4CC (free)
void fn_8018D1FC(const char* fmt, ...);                     // 0x8018D1FC
u8* fn_800CD7D8(u8* p, u8* q, s32 limit);                   // 0x800CD7D8

// 0x80010FF4
// Sets the 3-float state block lbl_803C49A8 (c1, c2, c1), then tail-calls fn_80142500 on lbl_805FAEF0.
u8* fn_80010FF4(void) {
    *(f32*)(lbl_803C49A8 + 0x0) = lbl_805FC6C4;
    *(f32*)(lbl_803C49A8 + 0x4) = lbl_805FC6C8;
    *(f32*)(lbl_803C49A8 + 0x8) = lbl_805FC6C4;
    return fn_80142500(lbl_805FAEF0);
}

// 0x80046F30
// Copies the 12-byte UI state block lbl_803C529C into p (s32, u8, s32 at +0/+4/+8).
void fn_80046F30(u8* p) {
    *(s32*)(p + 0x0) = *(s32*)(lbl_803C529C + 0x0);
    *(u8*)(p + 0x4) = *(u8*)(lbl_803C529C + 0x4);
    *(s32*)(p + 0x8) = *(s32*)(lbl_803C529C + 0x8);
}

// 0x8005CA4C
// Sets three floats in lbl_803C54D4, then tail-calls fn_8032F1E8 on lbl_803C5498 with fn_80052638.
void fn_8005CA4C(void) {
    *(f32*)(lbl_803C54D4 + 0x0) = lbl_805FD300;
    *(f32*)(lbl_803C54D4 + 0x4) = lbl_805FD304;
    *(f32*)(lbl_803C54D4 + 0x8) = lbl_805FD300;
    fn_8032F1E8(lbl_803C5498, (void (*)(void))fn_80052638, 0, 0xC, 4);
}

// 0x8007E880
// Clears the two experiment fields in the sub-object at +0x1f4; if the sound flag at +0x394 was set,
// clears it and stops the experiment sound.
void fn_8007E880(u8* p) {
    u8* sub = *(u8**)(p + 0x1f4);
    *(s32*)(sub + 0x264) = 0;
    *(s32*)(sub + 0x268) = -1;
    if (*(u8*)(p + 0x394) == 0) return;
    *(u8*)(p + 0x394) = 0;
    StopExperimentSound(*(void**)(p + 0x1f4), 0x8BF23844u);
}

// 0x80087EB8
// Adds dt to the timer at +0x8c; when it reaches lbl_805FD934, signals the game flow.
void fn_80087EB8(u8* p, f32 dt) {
    f32 v = *(f32*)(p + 0x8c) + dt;
    *(f32*)(p + 0x8c) = v;
    if (v >= lbl_805FD934) GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x80087EFC
// Adds dt to the timer at +0x8c; when it reaches lbl_805FD940, signals the game flow.
void fn_80087EFC(u8* p, f32 dt) {
    f32 v = *(f32*)(p + 0x8c) + dt;
    *(f32*)(p + 0x8c) = v;
    if (v >= lbl_805FD940) GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x8008CAC0
// If v differs from the stored value at +0x268, stores it and sends event 0xFCEB7453 to the event manager.
void fn_8008CAC0(u8* p, f32 v) {
    if (v == *(f32*)(p + 0x268)) return;
    *(f32*)(p + 0x268) = v;
    EventSend(gEventMgr, p, NULL, 0xFCEB7453u, 0, 0, 1, 0);
}

// 0x80098D60
// Returns the flag byte at +0x2f8; when set and the value at +0x2fc is above lbl_805FDC1C, copies it to +0x270.
u8 fn_80098D60(u8* p) {
    if (*(u8*)(p + 0x2f8) != 0) {
        f32 t = *(f32*)(p + 0x2fc);
        if (t > lbl_805FDC1C) *(f32*)(p + 0x270) = t;
    }
    return *(u8*)(p + 0x2f8);
}

// 0x8009B780
// Looks up the entry at index 1 via fn_800C2618 and passes it to fn_800C2028.
void fn_8009B780(u8* p) {
    fn_800C2028((u8*)(uintptr_t)fn_800C2618(p, 1));
}


// 0x800A9658
// Adds dt to the timer at +0x8c; when it reaches lbl_805FDF1C, signals the game flow.
void fn_800A9658(u8* p, f32 dt) {
    f32 v = *(f32*)(p + 0x8c) + dt;
    *(f32*)(p + 0x8c) = v;
    if (v >= lbl_805FDF1C) GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x800A97C8
// Adds dt to the timer at +0x8c; when it reaches lbl_805FDF60, signals the game flow.
void fn_800A97C8(u8* p, f32 dt) {
    f32 v = *(f32*)(p + 0x8c) + dt;
    *(f32*)(p + 0x8c) = v;
    if (v >= lbl_805FDF60) GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x800BB5E8
// Adds dt to the timer at +0x8c; when it reaches lbl_805FE258, signals the game flow.
void fn_800BB5E8(u8* p, f32 dt) {
    f32 v = *(f32*)(p + 0x8c) + dt;
    *(f32*)(p + 0x8c) = v;
    if (v >= lbl_805FE258) GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x800CD788
// Dispatch: if the table entry for p's index is above q's count, forwards to fn_800CD7D8; otherwise
// calls slot 0x18 of the object at p+0xc. UNCERTAIN: return type and slot signature are guessed.
s32 fn_800CD788(u8* p, u8* q) {
    s32 idx = *(s32*)(p + 0x14);
    s32 limit = *(s32*)(q + 0x8) - 1;
    if (limit < *(s32*)(lbl_80347440 + idx * 4)) return (s32)(intptr_t)fn_800CD7D8(p, q, limit); // lint:ok (returns a pointer in the original)
    return ((s32(*)(u8*, u8*, s32))(*(void**)(*(u8**)(p + 0xc) + 0x18)))(p, q, limit);
}

// 0x8018D4EC
// Stores the entry at index idx (obj+0x88 + idx*4) into obj+0x84; if non-null, prints its s32 value
// with the format string lbl_80381370.
void fn_8018D4EC(u8* obj, s32 idx) {
    u8* node = *(u8**)(obj + idx * 4 + 0x88);
    if (node == NULL) return;
    *(u8**)(obj + 0x84) = node;
    fn_8018D1FC((const char*)lbl_80381370, *(s32*)node, idx);
}

// 0x8019CD68
// Frees p via fn_8018D4CC and returns 0.
s32 fn_8019CD68(u8* p) {
    fn_8018D4CC(p);
    return 0;
}

}  // extern "C"
