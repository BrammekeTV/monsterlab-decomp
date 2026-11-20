#include "types.h"

// Globals (match the defining files' declarations).
extern "C++" void* gScreenSets;          // 0x805FB654
extern void* lbl_805FB658;               // 0x805FB658 (global render object)
extern void* lbl_805FBBAC;               // 0x805FBBAC (voice SFX handle)
extern f32 lbl_805FF940;                 // 0x805FF940 (float constant)
extern f32 lbl_805FF944;                 // 0x805FF944 (float constant, 1.0f)

// Log strings (0x803829F8 base + offset, see fn_8019BE68).
extern const char lbl_803826A8[];        // 0x803826A8 (log string)
extern const char lbl_803826E8[];        // 0x803826E8 (log string)
extern const char lbl_80382838[];        // 0x80382838 (log string)
extern const char lbl_80382898[];        // 0x80382898 (log string)
extern const char lbl_80382AB4[];        // 0x80382AB4 (log string)
extern const char lbl_80382B1C[];        // 0x80382B1C (lbl_803829F8 + 0x124)
extern const char lbl_80382B2C[];        // 0x80382B2C (lbl_803829F8 + 0x134)
extern const char lbl_80382B40[];        // 0x80382B40 (lbl_803829F8 + 0x148)
extern const char lbl_80382B90[];        // 0x80382B90 (lbl_803829F8 + 0x198)

typedef void (*VFlagFn)(void* self, s32 flag);

extern "C" {
s32 PlaySoundAt(void* sound, int a, const void* pos, int b);  // 0x80209ABC
void* ScreenSetsCurrent(void* sets);                          // 0x8018D1E4
void TriggerDoAction(void* action, void* a4, void* a5);       // 0x801E0054
void UIPlayAnim(void* el, u32 anim);                          // 0x8019BE40
void fn_8018D2B8(const char* fmt, ...);                       // 0x8018D2B8 (debug log)
void fn_8018D370(const char* fmt, ...);                       // 0x8018D370 (debug log)
u32 fn_8019552C(u8* a3, s32 a4);                              // 0x8019552C (leaf)
s32 fn_8019555C(u8* a3);                                      // 0x8019555C (leaf)
void* fn_80198F1C(void* set, u32 hash);                       // 0x80198F1C
f32 fn_801AB7F0(void* h);                                     // 0x801AB7F0
f32 fn_801AB804(void* h);                                     // 0x801AB804
void* fn_801D40E0(s32 n);                                     // 0x801D40E0
const char* fn_801EB708(u32 hash);                            // 0x801EB708
s32 fn_801ED050(u8* p, s32 a, u8* b);                         // 0x801ED050
void* fn_80209814(void* v);                                   // 0x80209814
void fn_8020A398(s32 handle);                                 // 0x8020A398
void fn_8019645C(void* a, s32 b);                             // 0x8019645C
void fn_8019BA18(u8* arg0, void* arg1, s8 arg2, f32 arg3);    // 0x8019BA18
}

extern "C" {

// 0x801DA798
void* fn_801DA798(void* p) {
    u8* obj = (u8*)p;
    u8* tbl = *(u8**)(obj + 0x5C);
    if (tbl == NULL) {
        return NULL;
    }
    return *(void**)(tbl + *(s32*)(obj + 0x50) * 4);
}

// 0x8020E668
void fn_8020E668(u8* arg0, s32 arg1, s32 arg2) {
    u8* a = *(u8**)(arg0 + arg2 * 4);
    u8* b = *(u8**)(arg0 + arg1 * 4);
    u8* slot;
    u8* cb;
    u8* cc;

    if (a == NULL || b == NULL) {
        return;
    }
    slot = b + arg2 * 4;
    cb = *(u8**)(a + 0x8);
    cc = *(u8**)(slot + 0xC);
    if (cc != NULL) {
        (*(VFlagFn*)(*(u8**)cc + 0x8))(cc, 1);
    }
    if (cb != NULL) {
        (*(VFlagFn*)(*(u8**)cb + 0x8))(cb, 1);
    }
    *(u8**)(a + 0x8) = NULL;
    *(u8**)(slot + 0xC) = NULL;
}

// 0x8004EA38
s32 fn_8004EA38(u8* arg0, s32 arg1, u8* arg2) {
    s32 r = fn_801ED050(arg0 + 0x94, arg1, arg2);
    *(u8*)(arg0 + 0x38E) = 0;
    return r;
}

// 0x80199B68
// Stops the looping SFX in slot 0x14C, then starts arg1 as a looping sound if its loop flag is set.
void fn_80199B68(u8* arg0, u8* arg1) {
    s32 handle = *(s32*)(arg0 + 0x14C);
    void* v;

    if (handle != -1) {
        fn_8020A398(handle);
        *(s32*)(arg0 + 0x14C) = -1;
    }
    if (arg1 == NULL) {
        fn_8018D370(lbl_803826A8);
        return;
    }
    if (*(u8*)(arg1 + 0x5) & 1) {
        if (lbl_805FBBAC == NULL) {
            v = fn_801D40E0(1);
            if (v != NULL) {
                v = fn_80209814(v);
            }
            lbl_805FBBAC = v;
        }
        *(s32*)(arg0 + 0x14C) = PlaySoundAt(arg1, 0, NULL, 0);
        return;
    }
    fn_8018D370(lbl_803826E8);
}

// 0x80199DD4
s32 fn_80199DD4(void* obj) {
    u32 w;
    u32 sp;

    if (obj == NULL) {
        return -2;
    }
    w = *(u32*)((u8*)obj + 0xA8);
    if ((w >> 27) & 0xF) {
        sp = w;
        TriggerDoAction(&sp, NULL, NULL);
    }
    UIPlayAnim(obj, 0x7F2E277Du);
    return 0;
}

// 0x8019A9EC
void fn_8019A9EC(u8* arg0, s32 arg1) {
    s32 d = *(s32*)(arg0 + 0x8);
    if (d != 0) {
        *(s32*)(arg0 + 0xC) = arg1 / d;
        *(s32*)(arg0 + 0x10) = arg1 % d;
    }
}

// 0x8019AC78
// Returns the first word of the list entry whose key equals arg1 (or the entry after it, wrapping),
// or the entry at index 0 when arg1 is NULL. Returns NULL and logs the name if not found.
void* fn_8019AC78(u8* arg0, u8* arg1) {
    u8* list = *(u8**)(arg0 + 0x4);
    u8* ent = NULL;

    if (arg1 == NULL) {
        ent = (u8*)(uintptr_t)fn_8019552C(list, 0);
    } else {
        s32 i = 0;
        while (i < fn_8019555C(list)) {
            u8* cur = (u8*)(uintptr_t)fn_8019552C(list, i);
            if (*(u8**)cur == arg1) {
                s32 last = fn_8019555C(list) - 1;
                s32 next = (i == last) ? 0 : i + 1;
                ent = (u8*)(uintptr_t)fn_8019552C(list, next);
                break;
            }
            i++;
        }
        if (ent == NULL) {
            fn_8018D2B8(lbl_80382838, (const char*)(arg1 + 0x35));
            return NULL;
        }
    }
    return *(void**)ent;
}

// 0x8019ADFC
// Index of the hash arg1 in the sound list at arg0+0x4; logs the hash name and returns -1 if absent.
s32 fn_8019ADFC(u8* arg0, u32 arg1) {
    u8* list = *(u8**)(arg0 + 0x4);
    s32 i = 0;

    while (i < fn_8019555C(list)) {
        u8* ent = (u8*)(uintptr_t)fn_8019552C(list, i);
        if (*(u32*)ent == arg1) {
            return i;
        }
        i++;
    }
    fn_8018D370(lbl_80382898, fn_801EB708(arg1));
    return -1;
}

// 0x8019B7D8
s32 fn_8019B7D8(u8* arg0) {
    fn_8019645C(lbl_805FB658, *(s32*)(arg0 + 0x58));
    return 0;
}

// 0x8019B808
// Multiplies the scale of each node in the chain at arg1+0x128 into the vector arg0 (starting at 1,1,1).
void fn_8019B808(u8* arg0, u8* arg1) {
    u8* node;

    *(f32*)(arg0 + 0x8) = lbl_805FF944;
    *(f32*)(arg0 + 0x4) = lbl_805FF944;
    *(f32*)(arg0 + 0x0) = lbl_805FF944;
    node = *(u8**)(arg1 + 0x128);
    while (node != NULL) {
        *(f32*)(arg0 + 0x0) = *(f32*)(arg0 + 0x0) * *(f32*)(node + 0x74);
        *(f32*)(arg0 + 0x4) = *(f32*)(arg0 + 0x4) * *(f32*)(node + 0x78);
        *(f32*)(arg0 + 0x8) = *(f32*)(arg0 + 0x8) * *(f32*)(node + 0x7C);
        node = *(u8**)(node + 0x128);
    }
}

// 0x8019B888
void fn_8019B888(u8* arg0, u32 arg1, s32 arg2) {
    f32 var = lbl_805FF940;
    u8* p = *(u8**)(arg0 + 0x10);
    void* desc;

    if (p != NULL) {
        f32 d = fn_801AB7F0(p);
        var = fn_801AB804(p) / d;
    }
    desc = fn_80198F1C(ScreenSetsCurrent(gScreenSets), arg1);
    if (desc == NULL) {
        fn_8018D370(lbl_80382AB4, arg1);
    }
    fn_8019BA18(arg0, desc, (s8)arg2, var);
}

// 0x8019B94C
void fn_8019B94C(u8* arg0, u32 arg1, s32 arg2) {
    f32 var = lbl_805FF940;
    u8* p = *(u8**)(arg0 + 0x10);
    void* desc;

    if (p != NULL) {
        f32 d = fn_801AB7F0(p);
        var = lbl_805FF944 - (fn_801AB804(p) / d);
    }
    desc = fn_80198F1C(ScreenSetsCurrent(gScreenSets), arg1);
    if (desc == NULL) {
        fn_8018D370(lbl_80382AB4, arg1);
    }
    fn_8019BA18(arg0, desc, (s8)arg2, var);
}

// 0x8019BE68
// Removes arg1 from the singly linked list at arg0+0x120 (next pointer at +0x124) and clears arg1's links.
void fn_8019BE68(u8* arg0, u8* arg1) {
    u8* head = *(u8**)(arg0 + 0x120);
    u8* cur;
    u8* prev;

    if (head != NULL) {
        cur = head;
        prev = NULL;
        while (cur != NULL && cur != arg1) {
            prev = cur;
            cur = *(u8**)(cur + 0x124);
        }
        if (cur != NULL) {
            if (prev != NULL) {
                if (*(u8**)(prev + 0x124) != cur) {
                    fn_8018D370(lbl_80382B1C);
                }
                *(u8**)(prev + 0x124) = *(u8**)(cur + 0x124);
            } else {
                if (cur != head) {
                    fn_8018D370(lbl_80382B2C);
                }
                *(u8**)(arg0 + 0x120) = *(u8**)(head + 0x124);
            }
        } else {
            fn_8018D370(lbl_80382B40);
        }
    } else {
        fn_8018D370(lbl_80382B90);
    }
    *(u8**)(arg1 + 0x124) = NULL;
    *(u8**)(arg1 + 0x128) = NULL;
}

// 0x8019C6DC
// Component-wise scales the three vector slots of arg0 by the float triple arg1.
void fn_8019C6DC(u8* arg0, const f32* arg1) {
    *(f32*)(arg0 + 0x0) = *(f32*)(arg0 + 0x0) * arg1[0];
    *(f32*)(arg0 + 0x10) = *(f32*)(arg0 + 0x10) * arg1[1];
    *(f32*)(arg0 + 0x20) = *(f32*)(arg0 + 0x20) * arg1[2];
}

}  // extern "C"
