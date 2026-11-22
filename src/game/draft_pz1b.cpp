// Small UI, texture and list helpers (0x801A*, 0x801B*, 0x801C*, 0x801D*).
#include "types.h"

extern "C" {
void*  MemAllocDefault(u32 size);                              // 0x801D4650
u8* fn_801AA898(u8* arg0, s32 a4, f32 farg0);                                     // 0x801AA898
s32    fn_801AA730(u8* dst, u8* src);                          // 0x801AA730
s32    fn_8019555C(u8* list);                                  // 0x8019555C
void   fn_8018D370(const char* fmt, ...);                      // 0x8018D370 (debug log)
u8*    fn_801EB7A4(void* p, s32 a4, s32 a5, s32 a6, s32 a7);  // 0x801EB7A4
void*  fn_801EB94C(void* p);                                   // 0x801EB94C
void   fn_801EBDF0(u8* list, u8* node);                        // 0x801EBDF0
s32 fn_801EE46C(u8* arg0, const char* name);                    // 0x801EE46C
void fn_801EE4B8(u8* arg0);                      // 0x801EE4B8
s32    fn_801EE504(u8* handle);                                // 0x801EE504
s32    fn_801EE4FC(u8* handle);                                // 0x801EE4FC
s32    fn_8016B5E0(u8* state, u8* data, s32 size);             // 0x8016B5E0
s32    fn_8016B6DC(u8* state);                                 // 0x8016B6DC
void   fn_8016B704(u8* state);                                 // 0x8016B704
u8*    fn_8016B724(u8* state, s32 a, const char* name);        // 0x8016B724
void*  memset(void* p, int c, unsigned long n);
char*  strcpy(char* d, char* s);
char*  strcat(char* d, const char* s);
}

extern u8 lbl_805F9548;   // 0x805F9548 (byte default for the record's short field)
extern f32 lbl_805FFA98;  // 0x805FFA98 (float constant passed to fn_801AA898)

extern "C" {

// 0x801A23E4
// UI element: calls slot 0x30 of the second vtable (at +0x138) with argument 4 (hide).
void fn_801A23E4(u8* el) {
    u8* vt = *(u8**)(el + 0x138);
    ((void (*)(u8*, s32))(*(void**)(vt + 0x30)))(el, 4);
}

// 0x801A2510
// UI element: calls slot 0x2C of the second vtable (at +0x138) with argument 4 (show).
void fn_801A2510(u8* el) {
    u8* vt = *(u8**)(el + 0x138);
    ((void (*)(u8*, s32))(*(void**)(vt + 0x2C)))(el, 4);
}

// 0x801A2524
// UI element: calls slot 0x30 of the second vtable (at +0x138) with argument 4 (hide).
void fn_801A2524(u8* el) {
    u8* vt = *(u8**)(el + 0x138);
    ((void (*)(u8*, s32))(*(void**)(vt + 0x30)))(el, 4);
}

// 0x801AB950
// Five UI animation slots: for each slot with a non-zero word, look up the matching
// animation track (offsets 0x68/0x74/0x80/0x90/0x9C of arg1) and feed it to fn_801AA730.
void fn_801AB950(u8* arg0, u8* arg1) {
    for (s32 i = 0; i < 5; i++) {
        if (*(s32*)(arg0 + i * 4 + 4) != 0) {
            u8* temp = fn_801AA898(*(u8**)(arg0 + i * 4 + 4), 0, lbl_805FFA98);
            u8* dst;
            switch (i) {
            case 0:
                dst = arg1 + 0x68;
                break;
            case 1:
                dst = arg1 + 0x74;
                break;
            case 2:
                dst = arg1 + 0x80;
                break;
            case 3:
                dst = arg1 + 0x90;
                break;
            case 4:
                dst = arg1 + 0x9C;
                break;
            default:
                fn_8018D370("UIAnim : how did you get here ...\n");
                dst = NULL;
                break;
            }
            if (temp != NULL) {
                fn_801AA730(temp, dst);
            }
        }
    }
}

// 0x801B373C
// Constructor for the 0x60-byte record: allocates a 0x2C-byte sub-object at +0x60 (set up
// by fn_801EB7A4), clears the record and sets the 16-bit fields at +0x68/+0x6A/+0x6C.
u8* fn_801B373C(u8* obj) {
    u8* mem = (u8*)MemAllocDefault(0x2C);
    *(u8**)(obj + 0x60) = mem;
    if (mem != NULL) {
        fn_801EB7A4(mem, 0xD8, 1, 0x10, 1);
    }
    *(s32*)(obj + 0x64) = 0;
    memset(obj, 0, 0x60);
    *(s16*)(obj + 0x68) = (s16)lbl_805F9548;
    *(s16*)(obj + 0x6A) = 0;
    *(s16*)(obj + 0x6C) = 0;
    return obj;
}

// 0x801B381C
// Adds a named entry: allocates a node from the sub-object at +0x60, makes it the head
// at +0x64 if none is set, copies the name twice (at node and node+0x60) and stores six
// pointers at node+0xC0..0xD4.
void fn_801B381C(u8* obj, const char* name, u8* a2, u8* a3, u8* a4, u8* a5, u8* a6, u8* a7) {
    u8* node = (u8*)fn_801EB94C(*(void**)(obj + 0x60));
    if (node != NULL) {
        if (*(void**)(obj + 0x64) == NULL) {
            *(void**)(obj + 0x64) = node;
        }
        strcpy((char*)node, (char*)name);
        strcpy((char*)(node + 0x60), (char*)name);
        *(void**)(node + 0xC0) = a2;
        *(void**)(node + 0xC4) = a3;
        *(void**)(node + 0xC8) = a4;
        *(void**)(node + 0xCC) = a5;
        *(void**)(node + 0xD0) = a6;
        *(void**)(node + 0xD4) = a7;
    }
}

// 0x801C9178
// Removes a node from a list. obj holds a count at +0, a table of list pointers at +0xC and
// a parallel slot table at +8. If list is found in the table, its slot is cleared and every
// node of list is removed via fn_801EBDF0 (walking the next pointer at node+4).
void fn_801C9178(u8* obj, u8* list) {
    s32 count = *(s32*)(obj + 0x0);
    s32 found = -1;
    s32 i;
    for (i = 0; i < count; i++) {
        if (((u8**)*(u8**)(obj + 0xC))[i] == list) {
            found = i;
            break;
        }
    }
    if (found != -1) {
        ((s32*)*(u8**)(obj + 0x8))[found] = 0;
        u8* cur = NULL;
        if (*(u8**)list != NULL) {
            cur = *(u8**)list + 8;
        }
        while (*(u8**)list != NULL) {
            u8* nextNode = *(u8**)(cur - 4);
            u8* nextCur = NULL;
            if (nextNode != NULL) {
                nextCur = nextNode + 8;
            }
            fn_801EBDF0(list, cur);
            cur = nextCur;
        }
    }
}

// 0x801D8D80
// Loads a .texture.wii file named after name (opened through fn_801EE46C), then polls the
// texture loader until it reports done (state 3, returns the texture via fn_8016B724) or
// fails (returns NULL). fallbackName is passed to the loader as the texture name.
u8* fn_801D8D80(char* name, char* fallbackName) {
    char path[0x400];
    u8 handle[0x10];
    u8 state[0x10];
    s32 size;
    u8* tex;

    strcpy(path, name);
    strcat(path, ".texture.wii");
    if (fn_801EE46C(handle, path) == 0) {
        return NULL;
    }
    size = fn_801EE504(handle);
    if (fn_8016B5E0(state, (u8*)fn_801EE4FC(handle), size) == 0) {
        return NULL;
    }
    for (;;) {
        if (fn_8016B6DC(state) == 3) {
            tex = fn_8016B724(state, 0, fallbackName);
            fn_801EE4B8(handle);
            return tex;
        }
        if (fn_8016B6DC(state) != 0) {
            fn_801EE4B8(handle);
            return NULL;
        }
        fn_8016B704(state);
    }
}

}  // extern "C"
