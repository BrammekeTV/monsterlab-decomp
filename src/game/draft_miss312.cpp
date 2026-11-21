// Game start-up / shutdown of the world systems (0x8000xxxx) and the skull puzzle texture table lookup.
// fn_80009BE0 (inlined GX FIFO writes plus dead code) was not decompiled.
#include "types.h"

extern "C" {
void  BufferFree(void* p);                                    // 0x801D4980
s32   LoadingScreenTick(int, f32);                            // 0x8000E254
void* MemAllocDefault(u32 size);                              // 0x801D4650
void* memset(void* p, int c, unsigned long n);
int   strcmp(const char* a, const char* b);

void  fn_80006F40(void* g);                                   // 0x80006F40
void  fn_80007C20(u8* p);                                     // 0x80007C20 (event callback)
void  fn_80052384(void);                                      // 0x80052384
void  fn_800DB9A4(u8* p);                                     // 0x800DB9A4
void  fn_80109A20(void);                                      // 0x80109A20
void  fn_80109AE0(void);                                      // 0x80109AE0
void  fn_80122220(void);                                      // 0x80122220
void  fn_80122B98(u8* p);                                     // 0x80122B98
void  fn_8018A41C(u8* p, s32 del);                            // 0x8018A41C
void  fn_801B27F4(void);                                      // 0x801B27F4
void  fn_801B28C4(void);                                      // 0x801B28C4
void  fn_801B313C(void);                                      // 0x801B313C
void  fn_801B31F0(void);                                      // 0x801B31F0
void* fn_801BA3C4(void* obj);                                 // 0x801BA3C4
void  fn_801BA424(void* obj);                                 // 0x801BA424
void  fn_801BC870(void);                                      // 0x801BC870
void  fn_801BD97C(u8* p);                                     // 0x801BD97C
void  fn_801C383C(u8* p);                                     // 0x801C383C
void  fn_801C767C(u8* p);                                     // 0x801C767C
void  fn_801C76FC(u8* p, void* a, u32 aSize, void* b, u32 bSize, void* c, u32 cSize);  // 0x801C76FC
void  fn_801C7874(u8* p);                                     // 0x801C7874
void  fn_801E27D0(u8* p);                                     // 0x801E27D0
void  fn_801E74AC(u8* pool);                                  // 0x801E74AC
void  fn_801E7574(u8* pool);                                  // 0x801E7574
void  fn_801EB6E0(void);                                      // 0x801EB6E0
void  fn_801EB908(u8* p, u8* src);                            // 0x801EB908
void  fn_801EBC74(u8* a, void* fn, u8* entry);                // 0x801EBC74

extern u8   gGame[];                // 0x803B53A0
extern u8   lbl_803C2A00[];         // 0x803C2A00
extern u8*  lbl_803C4000[16];       // 0x803C4000 (per-slot record pointers)
extern s32  lbl_803C4040[16];       // 0x803C4040 (per-slot "in use" flags)
extern u8   lbl_803C4298[];         // 0x803C4298 (+4 and +8: objects with vtables)
extern u8   lbl_8047C760[];         // 0x8047C760 (effect manager)
extern u8   lbl_8047C790[];         // 0x8047C790
extern u8   lbl_8047C7AC[];         // 0x8047C7AC
extern u8   lbl_80355D58[];         // 0x80355D58 (puzzle script name table)
extern s32  lbl_805F95F8;           // 0x805F95F8
extern u8   lbl_805FB8A8[];         // 0x805FB8A8
extern void* lbl_805FB6D0;          // 0x805FB6D0
extern u8*  lbl_805FB8D8;           // 0x805FB8D8 (effect pool)
extern f32  lbl_805FC630;           // 0x805FC630 (0.0f)
extern f32  lbl_805FC634;           // 0x805FC634 (-1.0f)
extern f32  lbl_805FC638;           // 0x805FC638 (0.1f)
}

extern "C++" u8 lbl_8047C730[];     // 0x8047C730 (combat results state)

typedef void (*VFn)(u8*);

extern "C" {

// 0x80007D68
// Start of a game session: clears the loading state, allocates the pools and manager objects
// (ticking the loading screen between steps), links the screen-set / effect lists back to their
// owners and drains the event queue. Returns 0 if already started, else 1.
s32 fn_80007D68(void* p) {
    u8* a = (u8*)p;
    u8* g = lbl_803C2A00;
    s32 flags = *(s32*)(a + 0x3C);
    if (flags & 1) return 0;
    *(s32*)(a + 0x3C) = flags & ~0x182;
    LoadingScreenTick(0, lbl_805FC638);

    *(f32*)(a + 0x140) = lbl_805FC630;
    *(s32*)(a + 0x148) = 0;
    *(s32*)(a + 0x14C) = 0;
    *(s32*)(a + 0x150) = 0;
    *(s32*)(a + 0x154) = 0;
    *(s32*)(a + 0x158) = 0;
    *(s32*)(a + 0x15C) = 0;
    *(f32*)(a + 0x160) = lbl_805FC634;
    *(f32*)(a + 0x164) = lbl_805FC630;
    LoadingScreenTick(0, lbl_805FC638);
    LoadingScreenTick(0, lbl_805FC638);
    LoadingScreenTick(0, lbl_805FC638);
    fn_80109A20();

    // 16 records of 0xF4 bytes: pointer table at +0x1600, in-use flags at +0x1640
    *(u8**)(g + 0x1600) = (u8*)MemAllocDefault(0xF40);
    for (s32 i = 0; i < 16; i++) {
        *(s32*)(g + 0x1640 + i * 4) = 1;
        *(u8**)(g + 0x1600 + i * 4) = *(u8**)(g + 0x1600) + i * 0xF4;
    }
    LoadingScreenTick(0, lbl_805FC638);

    u8* obj = (u8*)MemAllocDefault(0x2C4);
    *(u8**)(lbl_803C4298 + 0x8) = obj;
    if (obj != NULL) fn_800DB9A4(obj);
    *(u8**)(lbl_803C4298 + 0x4) = *(u8**)(lbl_803C4298 + 0x8);

    void* ev = MemAllocDefault(4);
    lbl_805FB6D0 = ev;
    if (ev != NULL) fn_801BA3C4(ev);
    LoadingScreenTick(0, lbl_805FC638);
    fn_80122220();
    LoadingScreenTick(0, lbl_805FC638);
    fn_801E27D0(lbl_805FB8A8);
    LoadingScreenTick(0, lbl_805FC638);
    LoadingScreenTick(0, lbl_805FC634);
    fn_801C767C(lbl_8047C730);
    LoadingScreenTick(0, lbl_805FC638);

    *(void**)(a + 0xC)  = MemAllocDefault(0x158000);
    *(void**)(a + 0x10) = MemAllocDefault(0x20000);
    *(void**)(a + 0x14) = MemAllocDefault(0x6800);
    LoadingScreenTick(0, lbl_805FC638);
    fn_801C76FC(lbl_8047C760, *(void**)(a + 0xC), 0x158000, *(void**)(a + 0x10), 0x20000,
                *(void**)(a + 0x14), 0x6800);
    *(u8**)(lbl_8047C760 + 0x1C) = lbl_8047C730;
    LoadingScreenTick(0, lbl_805FC638);
    fn_801BD97C(lbl_8047C790);
    fn_801C383C(lbl_8047C7AC);
    LoadingScreenTick(0, lbl_805FC638);

    u8* src = g + 0x20;
    *(s32*)(lbl_8047C790 + 0x10) = *(s32*)(src + 0x830);
    *(s32*)(lbl_8047C790 + 0xC)  = *(s32*)(src + 0x82C);
    *(u8*)(lbl_8047C790 + 0x14)  = *(u8*)(src + 0x834);
    *(s32*)(lbl_8047C7AC + 0x1C) = *(s32*)(src + 0x83C);
    *(s32*)(lbl_8047C7AC + 0x18) = *(s32*)(src + 0x838);
    *(u8*)(lbl_8047C7AC + 0x20)  = *(u8*)(src + 0x840);

    // point each 0x80-byte entry back at its owner and register its two resources
    for (s32 i = 0; i < *(s32*)(lbl_8047C7AC + 0x18); i++) {
        u8* e = *(u8**)(lbl_8047C7AC + 0x1C) + i * 0x80;
        *(u8**)(e + 0x6C) = lbl_8047C7AC;
        if (*(u8**)(e + 0x7C) != NULL) fn_801EB908(*(u8**)(e + 0x7C), *(u8**)(*(u8**)(e + 0x6C) + 0x14));
        if (*(u8**)(e + 0x78) != NULL) fn_801EB908(*(u8**)(e + 0x78), *(u8**)(*(u8**)(e + 0x6C) + 0x10));
    }
    // same for the 0x70-byte entries of the second manager
    for (s32 i = 0; i < *(s32*)(lbl_8047C790 + 0xC); i++) {
        u8* e = *(u8**)(lbl_8047C790 + 0x10) + i * 0x70;
        *(u8**)(e + 0x64) = lbl_8047C790;
        if (*(u8**)(e + 0x68) != NULL) fn_801EB908(*(u8**)(e + 0x68), *(u8**)(*(u8**)(e + 0x64) + 0x4));
    }
    // 0x1F4-byte result entries point back at the results state
    for (s32 i = 0; i < *(s32*)lbl_8047C730; i++) {
        *(u8**)(*(u8**)(lbl_8047C730 + 0x4) + i * 0x1F4 + 0x1DC) = lbl_8047C730;
    }
    LoadingScreenTick(0, lbl_805FC638);
    LoadingScreenTick(0, lbl_805FC638);
    fn_801BC870();
    fn_801EBC74(*(u8**)(lbl_8047C790 + 0x4), (void*)fn_80007C20, NULL);

    // drain the ring queue (head +0x5C, tail +0x60, capacity +0x64)
    *(s32*)(a + 0x50) = 0;
    while (!(*(s32*)(a + 0x5C) < 0 || *(s32*)(a + 0x60) < 0)) {
        s32 head = *(s32*)(a + 0x5C);
        if (*(s32*)(a + 0x60) == head) {
            *(s32*)(a + 0x60) = -1;
            *(s32*)(a + 0x5C) = -1;
        } else {
            *(s32*)(a + 0x5C) = (head + 1) % *(s32*)(a + 0x64);
        }
    }
    fn_80122B98(a + 0x20);
    *(u32*)(a + 0x3C) = (*(u32*)(a + 0x3C) & ~8u) | 1;
    LoadingScreenTick(0, lbl_805FC638);
    fn_801EB6E0();
    *(u8*)(a + 0x1C) = *(u8*)(a + 0x1D);
    *(u8*)(a + 0x1D) = 0;
    *(s32*)(a + 0x18) = 1;
    LoadingScreenTick(0, lbl_805FC638);

    lbl_805F95F8 = -1;
    lbl_805FB8D8 = (u8*)MemAllocDefault(0x8384);
    fn_801E74AC(lbl_805FB8D8);
    fn_801B27F4();
    fn_801B313C();
    fn_80052384();
    return 1;
}

// 0x80008298
// Shuts the game session down: frees the effect pool, drains the event queue, destroys the
// per-slot records, the manager objects and the three big buffers, then clears the state flags.
void fn_80008298(void* p) {
    u8* a = (u8*)p;
    if (lbl_805FB8D8 != NULL) {
        fn_801E7574(lbl_805FB8D8);
        BufferFree(lbl_805FB8D8);
        lbl_805FB8D8 = NULL;
    }
    *(s32*)(a + 0x50) = 0;
    while (!(*(s32*)(a + 0x5C) < 0 || *(s32*)(a + 0x60) < 0)) {
        s32 head = *(s32*)(a + 0x5C);
        if (*(s32*)(a + 0x60) == head) {
            *(s32*)(a + 0x60) = -1;
            *(s32*)(a + 0x5C) = -1;
        } else {
            *(s32*)(a + 0x5C) = (head + 1) % *(s32*)(a + 0x64);
        }
    }
    fn_80109AE0();
    for (s32 i = 0; i < 16; i++) {
        if (lbl_803C4040[i] == 0) fn_8018A41C(lbl_803C4000[i], -1);
        lbl_803C4040[i] = 0;
    }
    BufferFree(lbl_803C4000[0]);
    memset(lbl_803C4000, 0, 0x40);

    u8* obj = *(u8**)(lbl_803C4298 + 0x8);
    if (obj != NULL) {
        (*(VFn*)(*(u8**)obj + 0x90))(obj);
        BufferFree(*(u8**)(lbl_803C4298 + 0x8));
        *(u8**)(lbl_803C4298 + 0x8) = NULL;
    }
    *(u8**)(lbl_803C4298 + 0x4) = NULL;
    if (lbl_805FB6D0 != NULL) {
        fn_801BA424(lbl_805FB6D0);
        BufferFree(lbl_805FB6D0);
        lbl_805FB6D0 = NULL;
    }
    fn_801C7874(lbl_8047C760);
    for (s32 off = 0xC; off <= 0x14; off += 4) {
        if (*(void**)(a + off) != NULL) {
            BufferFree(*(void**)(a + off));
            *(void**)(a + off) = NULL;
        }
    }
    fn_801B28C4();
    fn_801B31F0();
    fn_80006F40(gGame);
    *(s32*)(a + 0x3C) = 0;
    *(u8*)(a + 0x1C) = 0;
    *(u8*)(a + 0x1D) = 0;
    *(u8*)(a + 0x1E) = 0;
}

// 0x80027E50
// Looks up the texture name for puzzle piece `name` (script element name, at +8) of skull puzzle
// `idx`: row of 14 name pointers per puzzle, column picked by the piece name.
const char* fn_80027E50(u8* p, s32 idx, const char* name) {
    const char* n = name + 8;
    s32 col;
    if (strcmp(n, (const char*)(lbl_80355D58 + 0x1698)) == 0)       col = 0;    // "complete"
    else if (strcmp(n, (const char*)(lbl_80355D58 + 0x16A4)) == 0)  col = 1;    // "background"
    else if (strcmp(n, "part1") == 0)  col = 2;
    else if (strcmp(n, "part2") == 0)  col = 3;
    else if (strcmp(n, "part3") == 0)  col = 4;
    else if (strcmp(n, "part4") == 0)  col = 5;
    else if (strcmp(n, "part5") == 0)  col = 6;
    else if (strcmp(n, "part6") == 0)  col = 7;
    else if (strcmp(n, "part7") == 0)  col = 8;
    else if (strcmp(n, "Grass1") == 0) col = 9;
    else if (strcmp(n, "Grass2") == 0) col = 0xA;
    else if (strcmp(n, "Grass3") == 0) col = 0xB;
    else col = (strcmp(n, "Grass4") == 0) ? 0xC : 0xD;
    return *(const char**)(lbl_80355D58 + 0x1400 + idx * 0x38 + col * 4);
}

}  // extern "C"
