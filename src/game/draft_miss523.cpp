#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

// Defined in ai_states.cpp with C++ linkage.
extern "C++" void* gEventMgr;
// Defined in small_800C.cpp without an extern "C" block, so C++ linkage.
extern "C++" u8* fn_800C5EE4(u8* p, s32 del);  // 0x800C5EE4

extern "C" {
extern u32 lbl_805FDDC8[];  // 0x805FDDC8, prop state table indexed by state id
extern f32 lbl_805FCBE4;    // 0x805FCBE4, float threshold factor
extern f32 lbl_805FE540;    // 0x805FE540, float reset value

void EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);
void fn_80023160(void);
void fn_800698DC(u8* p);
u8* fn_8006D34C(u8* p, s32 arg1);
void fn_8020E864(u8* p, s32 del);
void operator_delete(void* p);
void fn_800A7D88(u8* obj, u32 hash);
s32 fn_800AEFEC(u8* p, u8* sphere, f32* contact);
void fn_800CD010(u8* p);
void* fn_801ACCA0(u8* arg0, s32 arg1);
u8* fn_800DE5E4(u8* p, s32 del);
void fn_80070460(u8* p);
u8* fn_800C210C(u8* p, s32 del);
u8* fn_800C42FC(u8* p, s32 del);
}

extern "C" {

// 0x8009E2AC
// Stores the state index at +0x29C and moves the prop to its table state when it differs.
void fn_8009E2AC(u8* arg0, s32 arg1) {
    u32 temp_r31;

    temp_r31 = lbl_805FDDC8[arg1];
    *(s32*)(arg0 + 0x29C) = arg1;
    if (temp_r31 != (u32)GetPropCurrentState((u32)(uintptr_t)arg0)) {
        SetPropState(arg0, temp_r31, 0);
    }
}

// 0x8009E888
// Deleting destructor: base cleanup on arg0 + 0xC, frees arg0 when arg1 > 0.
u8* fn_8009E888(u8* arg0, s32 arg1) {
    if (arg0 != 0) {
        fn_800DE5E4(arg0 + 0xC, -1);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x800AEEA8
s32 fn_800AEEA8(u8* arg0, u8* sphere) {
    s32 temp_r3;

    temp_r3 = fn_800AEFEC(arg0 + 0x27C, sphere, 0);
    if ((u32)temp_r3 + 0x10000u != 0xFFFFu) {
        fn_800A7D88(arg0, (u32)temp_r3);
        return (s32)0xAA9BD49F;
    }
    return -1;
}

// 0x8002DE24
// Fires fn_80023160 once when the float at +0x6C drops below 0x78 times the threshold.
void fn_8002DE24(u8* arg0) {
    if (*(f32*)(arg0 + 0x6C) < *(f32*)(arg0 + 0x78) * lbl_805FCBE4 && *(u8*)(arg0 + 0xF0) == 0) {
        fn_80023160();
        *(u8*)(arg0 + 0xF0) = 1;
    }
}

// 0x8004E874
// Releases the object at +0x268 (if any), runs fn_800698DC and clears the byte at +0x28C.
void fn_8004E874(u8* arg0) {
    u8* temp_r0;

    temp_r0 = *(u8**)(arg0 + 0x268);
    if (temp_r0 != 0) {
        fn_8020E864(temp_r0, 1);
        *(s32*)(arg0 + 0x268) = 0;
    }
    fn_800698DC(arg0);
    *(u8*)(arg0 + 0x28C) = 0;
}

// 0x8006F71C
// Deleting destructor: fn_8006D34C on arg0, frees arg0 when arg1 > 0.
u8* fn_8006F71C(u8* arg0, s32 arg1) {
    if (arg0 != 0) {
        fn_8006D34C(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8009D6B8
// Deleting destructor: fn_801ACCA0 (mode 0) on arg0, frees arg0 when arg1 > 0.
u8* fn_8009D6B8(u8* arg0, s32 arg1) {
    if (arg0 != 0) {
        fn_801ACCA0(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8009D880
// Deleting destructor: fn_800C5EE4 on arg0, frees arg0 when arg1 > 0.
u8* fn_8009D880(u8* arg0, s32 arg1) {
    if (arg0 != 0) {
        fn_800C5EE4(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8009E1D4
// Deleting destructor: fn_800C5EE4 on arg0, frees arg0 when arg1 > 0.
u8* fn_8009E1D4(u8* arg0, s32 arg1) {
    if (arg0 != 0) {
        fn_800C5EE4(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8009FD74
// Deleting destructor: fn_800C210C on arg0, frees arg0 when arg1 > 0.
u8* fn_8009FD74(u8* arg0, s32 arg1) {
    if (arg0 != 0) {
        fn_800C210C(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x8009FDCC
// Deleting destructor: fn_800C42FC on arg0, frees arg0 when arg1 > 0.
u8* fn_8009FDCC(u8* arg0, s32 arg1) {
    if (arg0 != 0) {
        fn_800C42FC(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x800CDC44
// Walks the count-prefixed pointer list at arg0: resets each object's floats at +0x264..+0x26C
// and clears its list slot, then sets the count to 0.
void fn_800CDC44(s32* arg0) {
    s32 i;

    for (i = 0; i < *arg0; i++) {
        u8* slot = (u8*)arg0 + 4 + i * 4;
        u8* obj = *(u8**)slot;

        *(f32*)(obj + 0x264) = lbl_805FE540;
        *(f32*)(obj + 0x268) = lbl_805FE540;
        *(f32*)(obj + 0x26C) = lbl_805FE540;
        *(s32*)slot = 0;
    }
    *arg0 = 0;
}

// 0x800CE38C
// Deleting destructor: fn_800C210C on arg0, frees arg0 when arg1 > 0.
u8* fn_800CE38C(u8* arg0, s32 arg1) {
    if (arg0 != 0) {
        fn_800C210C(arg0, 0);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x800CF23C
// Runs fn_80070460 on arg0, sends event 0x757B006F to the event manager, then fn_800CD010 on arg0 +0xA4.
void fn_800CF23C(u8* arg0) {
    fn_80070460(arg0);
    EventSendTo(gEventMgr, 0x757B006Fu, 0, 0xCA90298Eu, 0, 0);
    fn_800CD010(*(u8**)(arg0 + 0xA4));
}

}  // extern "C"
