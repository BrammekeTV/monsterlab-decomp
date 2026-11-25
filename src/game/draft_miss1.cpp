// Miscellaneous functions: event queues, heap wrappers, script table, audio voices.
#include "types.h"
#include <stdarg.h>
#include <stdio.h>

extern "C" {
#include "game/script_callbacks.h"
}

struct ScriptFunc { u32 type; void* fn; u32 hash; };   // 0xC bytes (as in script_dispatch.cpp)

extern "C++" void* gScreenSets;        // 0x805FB654
extern "C++" s32   gZoneResourceSize;  // 0x805FB94C

extern "C" {
void   GameLog(int level, int category, const char* fmt, ...);   // 0x8000B2F0
u8*    GetGlobalSettings(void);                                  // 0x801D2BF8
s32 HeapInit(u8* arg0, u8* arg1, u32 arg2);              // 0x801EF8DC
void*  ScreenSetsCurrent(void* sets);                            // 0x8018D1E4
void   SysInit(void);
void*  SysAllocMEM2Block(u32 size);                              // 0x801FAEB0
BOOL   ZoneIsLoaded(void* world, s32 zone);                      // 0x8012F408
u8* fn_8006F61C(u8* p, s32 type, u8* rec, s32 reset);                                       // 0x8006F61C
void   fn_8008CE28(u8* vec, f32 v);                              // 0x8008CE28
void   fn_8018D2B8(const char* fmt, ...);                        // 0x8018D2B8 (debug log)
void   fn_8018D370(const char* fmt, ...);                        // 0x8018D370 (debug log)
s32    fn_801B5A60(u8* p);                                       // 0x801B5A60
s32    fn_801B9F14(u8* sender, u32 id);                          // 0x801B9F14
ScriptValue fn_801E1BD4(u8* a, u8* b, u32* code);               // 0x801E1BD4
const char* FormatHashString(u32 hash);                               // 0x801EB708 (hash -> name for logs)
void*  fn_801EEB20(u8* heap, u32 size);                          // 0x801EEB20 (heap alloc)
void   fn_801EF070(u8* heap, void* p);                           // 0x801EF070 (heap free)
u32 fn_801F2C10(u32 key);                                    // 0x801F2C10
void   fn_801F357C(void);                                        // 0x801F357C
void   fn_801F3BB8(void);                                        // 0x801F3BB8
void   fn_801F401C(void);                                        // 0x801F401C
s32    fn_801F5B44(u8* p, s32 id);                               // 0x801F5B44
void*  fn_801F5BB8(u8* p, s32 id, s32 a, s32 b, s32 c, s32 d);   // 0x801F5BB8
BOOL   fn_801F5220(void);                                        // 0x801F5220
void   fn_801FF2A8(u8* audio, s32 handle);                       // 0x801FF2A8 (audio voice release)
s32    fn_801D4790(void* p);                                     // 0x801D4790
void*  fn_80330D80(s32 size);                                    // 0x80330D80 (malloc)
void*  fn_80198E68(void* set, u32 hash);                         // 0x80198E68
u32    fn_80143A5C(const char* s);                               // 0x80143A5C
void*  ScreenSetFindElement(void* set, u32 hash);                // 0x80198F18
u32    StringHash(const char* s);                                // 0x801EB704
void   operator_delete(void* p);                                 // 0x801D426C
void*  fn_801D40E0(s32 size);                                    // 0x801D40E0

extern const char lbl_803818A8[];    // 0x803818A8 (string)
extern const char lbl_80381AA0[];    // 0x80381AA0 (string)
extern const char lbl_80360278[];    // 0x80360278 (character-not-found log format)
extern u8   gAudio[];
extern u8   gWorldState[];           // 0x803E6D00 (first word: pointer to the world block)
extern ScriptFunc gScriptFuncs[];    // 0x80484230
extern s32  lbl_8055C578[];          // 0x8055C578 (256-entry identity table)
extern u8   lbl_8035A0B4[];          // 0x8035A0B4 (vtable)
extern u8   lbl_80412F90[];          // 0x80412F90 (log text buffer)
extern u8   lbl_8047CAC0[];          // 0x8047CAC0 (main heap; second heap at +0x394)
extern u8   lbl_80559D78[];          // 0x80559D78 (0x28-byte audio voice records, indexed by handle & 0xFFFFF)
extern s32  lbl_80386064[];          // 0x80386064 table (read at idx - 0x5F)
extern s32  lbl_805F9800;            // script function index for the fallback call
extern s32  lbl_805F9650;            // 0x805F9650 (script table capacity)
extern s32  lbl_805FB880;            // 0x805FB880 (script table entry count)
extern u8*  lbl_805FB698;            // 0x805FB698 (anim set table: first word is the count)
extern s32  lbl_805FB6B8;            // 0x805FB6B8
extern u8   lbl_805FB7A8[];          // 0x805FB7A8 (+0 MEM2 heap base, +4 second heap enabled)
extern u32  lbl_805FB7B0;            // 0x805FB7B0 (alloc count, high word)
extern u32  lbl_805FB7B4;            // 0x805FB7B4 (alloc count, low word)
extern u32  lbl_805FB7B8;            // 0x805FB7B8 (free count, high word)
extern u32  lbl_805FB7BC;            // 0x805FB7BC (free count, low word)
extern u32  lbl_805FB7C0;            // 0x805FB7C0 (alloc bytes, high word)
extern u32  lbl_805FB7C4;            // 0x805FB7C4 (alloc bytes, low word)
extern u8*  lbl_805FB888;            // 0x805FB888 (table for zone id 0x1FF)
extern u8*  lbl_805FB95C;            // 0x805FB95C (event pool)
extern void* lbl_805FB964;           // 0x805FB964 (object with a virtual ready check at vtable +0x34)
extern s32  lbl_805FB970;            // 0x805FB970
extern s32  lbl_805FB974;            // 0x805FB974
extern s32  lbl_805FB994;            // 0x805FB994
extern u32  lbl_805FB9A4;            // 0x805FB9A4 (lock flag)
extern u32  lbl_805FB9B0;            // 0x805FB9B0 (current posted size)
extern s32  lbl_805FB9B8;            // 0x805FB9B8 (total posted size)
extern u32  lbl_805FB9C0;            // 0x805FB9C0 (peak posted size)
extern u32  lbl_805FB9C8;            // 0x805FB9C8 (peak posted size, second counter)
extern u8   lbl_805FBBB4[];          // 0x805FBBB4 (listener-position valid flag per id)
}

// 64-bit counter kept as two big-endian words (hi, lo): *hi:*lo += v.
static inline void Add64(u32* hi, u32* lo, u32 v) {
    u32 old = *lo;
    u32 nw = old + v;
    *lo = nw;
    *hi += (nw < old);
}

// Appends a 16-byte slot to a 0x40-entry ring buffer in q (head index at headOff, tail index at
// tailOff, slots at dataOff). Advances the tail when the head catches up. Returns the new slot.
static inline u8* RingPush(u8* q, u32 headOff, u32 tailOff, u32 dataOff) {
    u32 h = *(u32*)(q + headOff);
    u32 nh = h + 1;
    *(u32*)(q + headOff) = nh;
    u8* e = q + h * 0x10 + dataOff;
    if (nh == 0x40) *(u32*)(q + headOff) = 0;
    u32 t = *(u32*)(q + tailOff);
    if (*(u32*)(q + headOff) == t) {
        u32 nt = t + 1;
        *(u32*)(q + tailOff) = nt;
        if (nt == 0x40) *(u32*)(q + tailOff) = 0;
    }
    return e;
}

// True when p lies inside the heap block described at heap+base (start-0x10 <= p < end).
static inline BOOL InHeap(u8* heap, u32 startOff, u32 endOff, u8* p) {
    return p >= *(u8**)(heap + startOff) - 0x10 && p < *(u8**)(heap + endOff);
}

// Sum of the global settings counters used to bound the zone id.
static inline s32 GlobalSetting(u32 off) { return *(s32*)(GetGlobalSettings() + off); }

extern "C" {

// 0x801BA190
// Pushes an event (sender, id, two args) on the queue in q, unless dedupe is set and the
// duplicate check finds it. Then offers it to the filter callback at +0xC28 and, when accepted,
// pushes it on the second queue as well.
void EventSend(u8* q, u8* sender, void* unused, u32 id, s32 a4, s32 a5, s32 dedupe, s32 unused2) {
    (void)unused;
    (void)unused2;
    s32 dup = 0;
    if (dedupe != 0) dup = fn_801B9F14(sender, id);
    if (dup != 0) return;
    u8* e = RingPush(q, 0x8, 0x4, 0x10);
    if (e != NULL) {
        *(u8**)(e + 0x0) = sender;
        *(u32*)(e + 0x4) = id;
        *(s32*)(e + 0x8) = a4;
        *(s32*)(e + 0xC) = a5;
    }
    if (*(void**)(q + 0xC28) != NULL) {
        struct { u8* sender; u32 id; s32 a4; s32 a5; } ev;
        ev.sender = sender;
        ev.id = id;
        ev.a4 = a4;
        ev.a5 = a5;
        if ((*(s32 (**)(void*))(q + 0xC28))(&ev) != 0) {
            u8* e2 = RingPush(q, 0x820, 0x81C, 0x828);
            if (e2 != NULL) {
                *(u8**)(e2 + 0x0) = sender;
                *(u32*)(e2 + 0x4) = id;
                *(s32*)(e2 + 0x8) = a4;
                *(s32*)(e2 + 0xC) = a5;
            }
            *(u32*)(q + 0x824) = *(u32*)(q + 0x820);
        }
    }
}

// 0x801BA2FC
// Pushes an event on the third queue in q when the session is active (or the id is 0x6C938971)
// and a sender is given.
void EventSendTo(u8* q, u8* sender, void* unused, u32 id, s32 a4, s32 a5) {
    (void)unused;
    if ((fn_801F5220() != 0 || id == 0x6C938971u) && sender != NULL) {
        u8* e = RingPush(q, 0x414, 0x410, 0x41C);
        if (e != NULL) {
            *(u8**)(e + 0x0) = sender;
            *(u32*)(e + 0x4) = id;
            *(s32*)(e + 0x8) = a4;
            *(s32*)(e + 0xC) = a5;
        }
    }
}

// 0x801D40E0
// Allocates size bytes: before the heaps exist it sets up the MEM2 heap and falls back to malloc;
// otherwise counts the allocation and allocates from the main heap, then the second heap.
void* fn_801D40E0(s32 size) {
    if (*(void**)lbl_805FB7A8 == NULL) {
        SysInit();
        if (*(void**)lbl_805FB7A8 == NULL) {
            void* base = SysAllocMEM2Block(0x03380000);
            *(void**)lbl_805FB7A8 = base;
            HeapInit((u8*)lbl_8047CAC0, (u8*)base, (u32)((u8*)0x90400000 - (u8*)base));
        }
        return fn_80330D80(size);
    }
    Add64(&lbl_805FB7B0, &lbl_805FB7B4, 1);
    Add64(&lbl_805FB7C0, &lbl_805FB7C4, (u32)size);
    void* r = fn_801EEB20(lbl_8047CAC0, (u32)size);
    if (r == NULL && *(s32*)(lbl_805FB7A8 + 4) != 0) r = fn_801EEB20(lbl_8047CAC0 + 0x394, (u32)size);
    return r;
}

// 0x801D4980
// Frees a heap block: counts the free, then releases it to whichever heap contains it.
void BufferFree(void* p) {
    u8* b = (u8*)p;
    if (b == NULL || fn_801D4790(b) != 0) return;
    Add64(&lbl_805FB7B8, &lbl_805FB7BC, 1);
    if (InHeap(lbl_8047CAC0, 0x37C, 0x384, b)) {
        fn_801EF070(lbl_8047CAC0, b);
        return;
    }
    if (*(s32*)(lbl_805FB7A8 + 4) != 0 && InHeap(lbl_8047CAC0, 0x710, 0x718, b)) {
        fn_801EF070(lbl_8047CAC0 + 0x394, b);
    }
}

// 0x800690D4
// Constructor (returns p in r3, not used by callers): runs the base constructor, installs the vtable, flags the child at +4 (bit 0x40,
// state 6), and resets the child's fields from this object when both extra arguments are set.
void fn_800690D4(u8* p, s32 type, u8* a2, s32 a3) {
    fn_8006F61C(p, type, a2, a3);
    u8* child = *(u8**)(p + 0x4);
    *(void**)(p + 0x0) = lbl_8035A0B4;
    if (child != NULL) {
        *(u32*)(child + 0x1C) |= 0x40;
        *(u8*)(*(u8**)(p + 0x4) + 0x34) = 6;
    }
    if (a2 != NULL && a3 != 0) {
        *(f32*)(*(u8**)(p + 0x4) + 0x24) = *(f32*)(p + 0x158);
        *(u8*)(*(u8**)(p + 0x4) + 0x35) = 0;
        *(u8*)(*(u8**)(p + 0x4) + 0x36) = 0;
        *(s32*)(p + 0x1A8) = 0;
    }
}

// 0x801DC91C
// Adds a script function to the table (hash of its name, signature type, code pointer).
// Returns its index, or -1 when the table is full; 0 when no name is given.
s32 RegisterScriptFunc(const char* name, s32 type, void* fn) {
    if (name == NULL) return 0;
    u32 hash = StringHash(name);
    s32 idx = -1;
    if ((u32)lbl_805FB880 < (u32)lbl_805F9650) {
        idx = lbl_805FB880;
        lbl_805FB880++;
    }
    if (idx != -1) {
        ScriptFunc* f = &gScriptFuncs[idx];
        f->hash = hash;
        f->type = (u32)type;
        f->fn = fn;
    }
    return idx;
}

// 0x8018F618
// Shows (on != 0) or hides the UI element with the given hash in the current screen set, by
// calling virtual slot 0x2C / 0x30 of the element's object at +0x138 with arg 2.
void UIElement_SetVisibleFlag(u32 a0, u32 a1) {
    u8* el;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_803818A8);
        el = NULL;
    } else {
        el = (u8*)ScreenSetFindElement(set, a0);
        if (el == NULL) {
            fn_8018D2B8(lbl_80381AA0, FormatHashString(a0));
            el = NULL;
        }
    }
    if (el != NULL) {
        if (a1 != 0) {
            (*(void (**)(u8*, s32))(*(u8**)(el + 0x138) + 0x2C))(el, 2);
        } else {
            (*(void (**)(u8*, s32))(*(u8**)(el + 0x138) + 0x30))(el, 2);
        }
    }
}

// 0x801F3918
// Posts event id for the sender (target) to the event pool. Does nothing unless the session is
// ready; updates the posted-size counters from the script function size callback and returns the
// pool's new event.
void* EventPost(s32 id, void* sender) {
    u8* target = (u8*)sender;
    s32 ready;
    if (lbl_805FB970 == 0 || lbl_805FB974 == 0) return NULL;
    ready = 0;
    if (lbl_805FB964 != NULL &&
        (*(s32 (**)(void*))(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964) != 0) {
        ready = 1;
    }
    if (ready == 0) return NULL;
    if (lbl_805FB95C == NULL) return NULL;
    if (lbl_805FB9A4 != 0) return NULL;
    if (lbl_805FB994 != 0) {
        ready = 0;
        if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != NULL &&
            (*(s32 (**)(void*))(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964) != 0) {
            ready = 1;
        }
        if (ready == 0) lbl_805FB994 = 0;
    }
    s32 var_r28 = 0;
    u16 var_r29 = 0;
    s32 var_r30 = 0;
    s32 var_r31 = 0;
    if (target != NULL) {
        u8* child = *(u8**)(target + 0x4);
        if (child != NULL && ((((*(u32*)(target + 0xB8)) >> 2) & 1) ^ 1) != 0) {
            var_r31 = 1;
            var_r29 = *(u16*)(child + 0x32);
            var_r30 = (*(u32*)(*(u8**)(target + 0x4) + 0x1C) >> 3) & 1;
            if (lbl_805FB970 != 0 && lbl_805FB974 != 0) {
                var_r28 = fn_801B5A60(target);
            } else {
                var_r28 = 0;
            }
        }
    }
    if (fn_801F5B44(lbl_805FB95C, id) == 0) {
        fn_801F3BB8();
        fn_801F401C();
        fn_801F357C();
        if (lbl_805FB95C == NULL) {
            fn_801F2C10(0xFFFA0400);
            return NULL;
        }
    }
    if (lbl_805FB95C == NULL) return NULL;
    s32 size;
    if ((u32)id >= 0x5F && (u32)id <= 0x7F) {
        size = lbl_80386064[id - 0x5F];
    } else {
        ScriptFunc* f = &gScriptFuncs[lbl_805F9800];
        size = ((s32 (*)(s32, ScriptFunc*))f->fn)(id, f);
    }
    u32 cur = lbl_805FB9B0 + (u32)size;
    lbl_805FB9B0 = cur;
    lbl_805FB9B8 += size;
    if (cur > lbl_805FB9C0) lbl_805FB9C0 = cur;
    if (cur > lbl_805FB9C8) lbl_805FB9C8 = cur;
    return fn_801F5BB8(lbl_805FB95C, id, var_r31, var_r30, var_r29, var_r28);
}

// 0x801D4650
// Allocates size bytes from the main heap (then the second heap), counts the allocation, and
// fills the block with the 0x7F800001 debug pattern.
void* MemAllocDefault(u32 size) {
    void* r = fn_801EEB20(lbl_8047CAC0, size);
    if (r == NULL && *(s32*)(lbl_805FB7A8 + 4) != 0) r = fn_801EEB20(lbl_8047CAC0 + 0x394, size);
    Add64(&lbl_805FB7B0, &lbl_805FB7B4, 1);
    Add64(&lbl_805FB7C0, &lbl_805FB7C4, size);
    if (r != NULL && size >= 4) {
        u32 n = size >> 2;
        for (u32 i = 0; i < n; i++) ((u32*)r)[i] = 0x7F800001;
    }
    return r;
}

// 0x80209814
// Fills the 256-entry identity table, then resets three vectors and the valid flag for each of
// the four listener slots. Returns its argument.
u8* fn_80209814(u8* p) {
    for (s32 i = 0; i < 256; i++) lbl_8055C578[i] = i;
    for (s32 i = 0; i < 4; i++) {
        f32 a[3], b[3], c[3];
        fn_8008CE28((u8*)a, 0.0f);
        fn_8008CE28((u8*)b, 0.0f);
        fn_8008CE28((u8*)c, 0.0f);
        lbl_805FBBB4[i] = 0;
    }
    return p;
}

// 0x8018D2B8
// printf-style debug log: formats into the shared buffer and logs it as "%s" (level 3, category 1).
void fn_8018D2B8(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsprintf((char*)lbl_80412F90, fmt, ap);
    va_end(ap);
    GameLog(3, 1, "%s", (const char*)lbl_80412F90);
}

// 0x80142EF8
// sprintf (formats into buf, returns the length).
int SysSprintf(char* buf, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vsprintf(buf, fmt, ap);
    va_end(ap);
    return r;
}

// 0x8020A398
// Releases an audio voice handle. Handles tagged 0x0AB00000 index a voice record: the record's
// voice (+0x14) is released and the record is reset. Other non-negative handles are released directly.
void fn_8020A398(s32 handle) {
    if ((handle & 0xFFF00000) == 0x0AB00000) {
        u32 off = (u32)(handle & 0xFFFFF) * 0x28;
        u8* rec = lbl_80559D78 + off;
        if (*(s32*)(rec + 0x14) != -1) {
            fn_801FF2A8(gAudio, *(s32*)(rec + 0x14));
            *(s32*)(rec + 0x14) = -1;
        }
        *(s32*)(rec + 0x0) = 0;
        *(s32*)(rec + 0x4) = lbl_805FB6B8;
        *(f32*)(rec + 0x8) = 0.0f;
        *(f32*)(rec + 0xC) = 0.0f;
        *(f32*)(rec + 0x10) = 0.0f;
        *(s32*)(rec + 0x14) = -1;
        *(f32*)(rec + 0x18) = 0.0f;
        *(s32*)(rec + 0x24) = 0;
        *(s32*)(rec + 0x20) = 0;
        *(s32*)(rec + 0x1C) = -1;
        return;
    }
    if (handle >= 0) fn_801FF2A8(gAudio, handle);
}

// 0x801AC2FC
// Finds the index of the anim_def entry (0x44 bytes each at +0xC) whose handle (+0x2C) equals
// the key. The entry count is the sum of the u16 counts at +4 over the anim sets. Returns -1 and
// logs when not found.
s32 AnimSetFindClip(u8* set, u32 key) {
    s32 total = 0;
    s32 n = *(s32*)lbl_805FB698;
    if (n > 0) {
        u16* counts = *(u16**)(set + 0x4);
        for (s32 i = 0; i < n; i++) total += counts[i];
    }
    s32 idx = 0;
    u32 off = 0;
    for (s32 left = total; left > 0; left--) {
        if (key == *(u32*)(*(u8**)(set + 0xC) + off + 0x2C)) return idx;
        off += 0x44;
        idx++;
    }
    GameLog(4, 0, "Could not locate index of anim_def entry with handle %s\n", FormatHashString(key));
    return -1;
}

// 0x800E876C
// Looks up a character by handle in the loaded zones (the two zone slots, each with two entry
// lists at +0x120 / +0x12C). Logs when it is not found.
u8* CharacterFind(u32 id) {
    s32 slot = 0;
    u8* found = NULL;
    u8* world = *(u8**)gWorldState;
    while (slot < 2 && found == NULL) {
        u8* zone = *(u8**)(world + 0x1A4) + slot * gZoneResourceSize;
        s32 i = 0;
        while (i < *(s32*)(zone + 0x94)) {
            found = *(u8**)(*(u8**)(zone + 0x120) + i * 4);
            if (found != NULL && *(u32*)(found + 0x2C) == id) break;
            found = NULL;
            i++;
        }
        if (found == NULL) {
            i = 0;
            while (i < *(s32*)(zone + 0x98)) {
                found = *(u8**)(*(u8**)(zone + 0x12C) + i * 4);
                if (found != NULL && *(u32*)(found + 0x2C) == id) break;
                found = NULL;
                i++;
            }
        }
        slot++;
    }
    if (found == NULL) GameLog(3, 0, "Could not find character %s\n", FormatHashString(id));
    return found;
}

// 0x801E0054
// Runs a trigger action word: bits 1-4 give the kind, the low 18 bits the index. Kind 2 runs the
// action from a zone's action table (zone id in bits 5-13); kind 4 calls a registered script
// function (types 0x16 / 0x1D return a value, 0x2F / 0x4E do not). The result (r3) is not used by callers.
s32 TriggerDoAction(void* action, void* a1, void* a2) {
    u8* obj = (u8*)a1;
    ScriptValue result = 0;
    u32 w = *(u32*)action;
    u32 kind = (w >> 27) & 0xF;
    u32 idx = w & 0x3FFFF;
    if (kind == 2) {
        s32 zone = (s32)((w >> 18) & 0x1FF);
        if (ZoneIsLoaded(gWorldState, zone) != 0) {
            u8* tbl = NULL;
            if (zone == 0x1FF) {
                tbl = lbl_805FB888;
            } else if (zone >= 0) {
                s32 sum = GlobalSetting(0x80);
                sum += GlobalSetting(0x68);
                sum += GlobalSetting(0x84);
                sum += GlobalSetting(0x88);
                sum += GlobalSetting(0x8C);
                sum += GlobalSetting(0x90);
                sum += GlobalSetting(0x94);
                sum += GlobalSetting(0x98);
                sum += GlobalSetting(0x6C);
                sum += GlobalSetting(0x70);
                sum += GlobalSetting(0x74);
                sum += GlobalSetting(0x78);
                sum += GlobalSetting(0x7C);
                if (zone < sum + 1) {
                    tbl = *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
                    tbl = *(u8**)(tbl + 0xD4);
                }
            }
            if (tbl != NULL && idx < *(u32*)tbl) {
                result = fn_801E1BD4(obj, (u8*)a2, *(u32**)(*(u8**)(tbl + 0x4) + idx * 4));
            }
        }
    } else if (kind == 4) {
        // The index can only be -1 here if the mask were wider; kept as in the original.
        u32 type = (idx == 0xFFFFFFFFu) ? 0 : gScriptFuncs[idx].type;
        ScriptFunc* f = &gScriptFuncs[idx];
        typedef ScriptValue (*ScriptCb)(void*, ScriptFunc*);
        switch (type) {
        case 0x2F:
            if (obj != NULL) ((ScriptCb)f->fn)(obj, f);
            break;
        case 0x16:
            if (obj != NULL) result = ((ScriptCb)f->fn)(obj, f);
            break;
        case 0x4E:
            if (obj != NULL) {
                u8** slot = *(u8***)(obj + 0xA4);
                if (slot != NULL && *slot != NULL) ((ScriptCb)f->fn)(*slot, f);
            }
            break;
        case 0x1D:
            if (obj != NULL) {
                u8** slot = *(u8***)(obj + 0xA4);
                if (slot != NULL && *slot != NULL) result = ((ScriptCb)f->fn)(*slot, f);
            }
            break;
        }
    }
    return (s32)result;
}

// 0x801EC880
// Finds the index of the 0xC8-byte entry (array at +4, count at +0) whose first word equals key.
// Returns -1 when none matches.
s32 fn_801EC880(u8* tbl, u32 key) {
    s32 n = *(s32*)tbl;
    u32 off = 0;
    s32 idx = 0;
    for (; n > 0; n--) {
        if (key == *(u32*)(*(u8**)(tbl + 0x4) + off)) return idx;
        off += 0xC8;
        idx++;
    }
    return -1;
}

// 0x80198F18
// Tail call to the screen set element lookup.
void* ScreenSetFindElement(void* set, u32 hash) {
    return fn_80198E68(set, hash);
}

// 0x801D426C
// operator delete: counts the free and releases the block to whichever heap contains it.
void operator_delete(void* p) {
    u8* b = (u8*)p;
    Add64(&lbl_805FB7B8, &lbl_805FB7BC, 1);
    if (b == NULL) return;
    if (InHeap(lbl_8047CAC0, 0x37C, 0x384, b)) {
        fn_801EF070(lbl_8047CAC0, b);
        return;
    }
    if (*(s32*)(lbl_805FB7A8 + 4) == 0) return;
    if (InHeap(lbl_8047CAC0, 0x710, 0x718, b)) fn_801EF070(lbl_8047CAC0 + 0x394, b);
}

// 0x801EB704
// Tail call to the string hash function.
u32 StringHash(const char* s) {
    return fn_80143A5C(s);
}

}  // extern "C"
