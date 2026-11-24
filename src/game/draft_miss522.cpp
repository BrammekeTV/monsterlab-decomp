// Small functions from the miss522 batch: deleting destructors that call the base dtor
// and free the block when del > 0, a few object teardown helpers and a battle frame update.
#include "types.h"

extern "C" {
u8*  fn_80062CE0(u8* p, s32 del);                  // 0x80062CE0 (base dtor)
u8*  fn_800DE5E4(u8* p, s32 del);                  // 0x800DE5E4 (base dtor)
void fn_80128C20(void* world);                     // 0x80128C20
s32  fn_8003E560(s32 idx);                         // 0x8003E560
void fn_80040E4C(u8* p, f32 dt);                   // 0x80040E4C
void BattleUpdate(u8* b, f32 dt);     // 0x80210060 (battle_object.cpp, C++ linkage)
void operator_delete(void* p);                     // 0x801D426C
extern u8* gBattle;                                // 0x805FBBB8
extern u8* lbl_805FB014;                           // 0x805FB014 (battle camera object)
}

extern "C" {

// 0x800867CC
// Deleting destructor: base dtor on the object, then free if del > 0.
u8* fn_800867CC(u8* p, s32 del) {
    if (p != NULL) {
        fn_80062CE0(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x8008CA44
// Deleting destructor: base dtor on the object, then free if del > 0.
u8* fn_8008CA44(u8* p, s32 del) {
    if (p != NULL) {
        fn_80062CE0(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x8008CC64
// Deleting destructor: base dtor on the object, then free if del > 0.
u8* fn_8008CC64(u8* p, s32 del) {
    if (p != NULL) {
        fn_80062CE0(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x800A65AC
// Deleting destructor: base dtor on the object, then free if del > 0.
u8* fn_800A65AC(u8* p, s32 del) {
    if (p != NULL) {
        fn_80062CE0(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x800ACD80
// Deleting destructor: base dtor on the object, then free if del > 0.
u8* fn_800ACD80(u8* p, s32 del) {
    if (p != NULL) {
        fn_80062CE0(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x800BC9E4
// Deleting destructor: base dtor on the object, then free if del > 0.
u8* fn_800BC9E4(u8* p, s32 del) {
    if (p != NULL) {
        fn_80062CE0(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x800BD8E8
// Deleting destructor: base dtor on the object, then free if del > 0.
u8* fn_800BD8E8(u8* p, s32 del) {
    if (p != NULL) {
        fn_80062CE0(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x800BE344
// Deleting destructor: base dtor on the object, then free if del > 0.
u8* fn_800BE344(u8* p, s32 del) {
    if (p != NULL) {
        fn_80062CE0(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x800BF69C
// Deleting destructor: base dtor on the object, then free if del > 0.
u8* fn_800BF69C(u8* p, s32 del) {
    if (p != NULL) {
        fn_80062CE0(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x800CD944
// Deleting destructor: base dtor on the object, then free if del > 0.
u8* fn_800CD944(u8* p, s32 del) {
    if (p != NULL) {
        fn_80062CE0(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x800DF4F0
// Deleting destructor: base dtor on the subobject at +0xC, then free if del > 0.
u8* fn_800DF4F0(u8* p, s32 del) {
    if (p != NULL) {
        fn_800DE5E4(p + 0xC, -1);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x801292D4
// Teardown: if the flag byte at +0x4 is set, run the cleanup routine, then free if del > 0.
u8* fn_801292D4(u8* p, s32 del) {
    if (p != NULL) {
        if (*(u8*)(p + 0x4) != 0) {
            fn_80128C20(p);
        }
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

// 0x8003F700
// Per-frame battle update: runs the battle update and the two fn_8003E560 calls while a battle
// exists, and the camera update when the battle camera object is set.
// Virtual (CombatContext vtable lbl_803584A0, slot 0xC; called as (context, dt) by fn at draft_miss192.cpp): the
// PowerPC code never reads r3, but the context pointer is an argument all the same.
void fn_8003F700(void* context, f32 farg0) {
    (void)context;
    if (gBattle != NULL) {
        BattleUpdate(gBattle, farg0);
        fn_8003E560(0);
        fn_8003E560(1);
    }
    if (lbl_805FB014 != NULL) {
        fn_80040E4C(lbl_805FB014, farg0);
    }
}

// 0x8004E778
u8* fn_8004E778(u8* p, s32 del) {
    if (p != NULL) {
        fn_80062CE0(p, 0);
        if (del > 0) {
            operator_delete(p);
        }
    }
    return p;
}

}  // extern "C"
