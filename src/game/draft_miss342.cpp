// Small functions batch 342: id pool table, sound load, script dispatch, lab text lookups, test-failed report.
#include <string.h>
#include "types.h"
#include "revolution.h"
#include "game/part.h"

extern "C++" u8 gFlag805FB948;
extern "C++" u8 gIdPool[];
extern "C++" MEMAllocator gSysAllocator;
extern "C++" u8* gWorldState;
extern "C++" s32 gZoneResourceSize;
struct StringTable;
extern "C++" StringTable* gStringTable;  // 0x805FB910
extern "C++" StringTable* gStringTable2;  // 0x805FB90C
extern "C++" u8* lbl_805FBBE0;

typedef void (*VFn)(void* self, ...);
typedef u8* (*VFnPtr)(void* self, ...);

extern "C" {
void*  IdPoolNext(void* pool);                                 // 0x801E6EB0
u32    StringHash(const char* s);                              // 0x801EB704
void   LabReleasePart(u8* inv, PartInstance* p);
int    SysSprintf(char* buf, const char* fmt, ...);
void*  SirLoad(void* loader, const char* path, void* dst, u32 size, s32 flags);
s32    fn_801DC9B0(void* p);                                   // 0x801DC9B0
s32    fn_801E5F48(u8* pool, const char* name, u32 hash);
void fn_801F2EE4(void);                              // 0x801F2EE4
void   fn_801F5414(u8* p);                                     // 0x801F5414
void   fn_801F5270(s32 a3, s32 a4, s32 a5, s32 a6, s32 a7, s32 a8, s32 a9, s32 a10);  // 0x801F5270
void   fn_80333B34(char* dst, char* src);                      // 0x80333B34
void*  fn_80333D50(const char* path, const char* mode);        // 0x80333D50
void   fn_80333B60(void* obj);                                 // 0x80333B60
s32    fn_8010F880(u32 sig, s32 func, u32* args);              // 0x8010F880
BOOL   ScriptDispatch(s32 func, u32 sig, u32* args, u32* result);
void   fn_8012189C(void);                                      // 0x8012189C
u32*   GetGlobalSettings(void);                                // 0x801D2BF8

extern u8  lbl_803C4298[];
extern u8  lbl_803C75C8[];
extern u8  lbl_803F22D8[];
extern u8  lbl_803F2088[];
extern s32 lbl_805FB22C;
extern s32 lbl_805FB230;
extern s32 lbl_805FB234;
extern u8  lbl_805FB228;
extern u8* lbl_805FB220;
extern u8* lbl_805FB224;
extern s32 lbl_805FBB38;
extern const char lbl_805F8200[];
extern const char lbl_805F8EB8[];
extern const char lbl_805F8EC0[];
extern const char lbl_805F8EC8[];
extern const char lbl_805F8ECC[];
extern const char lbl_805F8ED0[];
extern const char lbl_805F8ED4[];
extern const char lbl_805F8ED8[];
extern const char lbl_805F8EDC[];
extern const char lbl_805F9178[];
extern const char lbl_8036C8B8[];
extern const char lbl_8036C8C8[];
extern const char lbl_80375E18[];
extern const char lbl_80354800[];
extern const char lbl_8035480C[];

// Fills the 0x7D0-entry id table (0x40 bytes each) and resets the table flags.
// arg5 is the caller's r5 (not set in these functions), passed on to SysSprintf.
static void ResetIdTable(s32 arg5) {
    u8* rec = lbl_803C75C8;
    s32 i = 0;

    lbl_805FB230 = 1;
    lbl_805FB22C = 1;
    lbl_805FB234 = 0;
    lbl_805FB228 = 0;
    do {
        *(void**)(rec + 0x0) = IdPoolNext(gIdPool);
        *(s32*)(rec + 0x4) = 0;
        *(s32*)(rec + 0x38) = 0;
        SysSprintf((char*)(rec + 0x8), lbl_805F8EDC, arg5);
        i += 1;
        *(s32*)(rec + 0x3C) = 0;
        rec += 0x40;
    } while (i < 0x7D0);
}

// Shared tail of the lab text getters: id -1 is "not found"; ids with bit 31 come from the second table.
static const char* LabTextFromId(s32 id) {
    u32 uid = (u32)id;
    u8* strs;
    u32 idx;

    if (id == -1) {
        return lbl_80354800;
    }
    if ((uid & 0x80000000u) != 0 && gStringTable2 != NULL) {
        strs = *(u8**)((u8*)gStringTable2 + 0x0);
        if (strs != NULL) {
            idx = uid & 0x7FFFFFFFu;
            if (idx < (u32)*(s32*)((u8*)gStringTable2 + 0xC)) {
                return *(const char**)(strs + idx * 4);
            }
        }
        return lbl_805F8200;
    }
    strs = *(u8**)((u8*)gStringTable + 0x0);
    if (strs != NULL && uid < (u32)*(s32*)((u8*)gStringTable + 0xC)) {
        return *(const char**)(strs + uid * 4);
    }
    return lbl_805F8200;
}

// 0x801091EC
// Formats a value into buf with one of the 0x805F8Exx formats, selected by kind (default: ED8).
s32 fn_801091EC(u8* buf, s32 val, s32 kind) {
    switch (kind) {
    case 1:
        return SysSprintf((char*)buf, lbl_805F8EC0, val, lbl_805F8EC8);
    case 2:
        return SysSprintf((char*)buf, lbl_805F8EC0, val, lbl_805F8ED0);
    case 3:
        return SysSprintf((char*)buf, lbl_805F8EC0, val, lbl_805F8ECC);
    case 4:
        return SysSprintf((char*)buf, lbl_805F8EC0, val, lbl_805F8ED4);
    default:
        return SysSprintf((char*)buf, lbl_805F8ED8, val);
    }
}

// 0x80109278
// Loads a named shared voice file into dst; returns the pool id when the name is not registered.
void* fn_80109278(u8* name, void* dst, s32 size) {
    char buf[0x80];
    u32 hash;
    s32 found;

    IdPoolNext(gIdPool);
    hash = StringHash((const char*)name);
    found = fn_801E5F48(gIdPool, lbl_8036C8C8, hash);
    if (found == 0) {
        return IdPoolNext(gIdPool);
    }
    SysSprintf(buf, lbl_805F8EB8, lbl_8036C8B8);
    return SirLoad(gIdPool, buf, dst, (u32)size, (s32)hash);
}

// 0x80109A20
// Resets the voice id table and allocates the 0xC0000-byte block on first use.
void fn_80109A20(s32 arg5) {
    ResetIdTable(arg5);
    if (lbl_805FB220 == NULL) {
        u8* p = (u8*)MEMAllocFromAllocator(&gSysAllocator, 0xC0000);
        lbl_805FB224 = p;
        lbl_805FB220 = p;
    }
}

// 0x80109AE0
// Resets the voice id table and frees the block allocated by fn_80109A20.
void fn_80109AE0(s32 arg5) {
    ResetIdTable(arg5);
    if (lbl_805FB220 != NULL) {
        MEMFreeToAllocator(&gSysAllocator, lbl_805FB224);
        lbl_805FB220 = NULL;
        lbl_805FB224 = NULL;
    }
}

// 0x8010A28C
// Sets the start-up flags and registers the eight hashed script names with fn_801F5270.
void fn_8010A28C(void) {
    s32 t25;
    s32 t26;
    s32 t27;
    s32 t28;
    s32 t29;
    s32 t30;
    s32 t31;
    s32 t3;

    gFlag805FB948 = 1;
    lbl_805FBB38 = 0x524D3745;
    fn_801F2EE4();
    fn_801F5414((u8*)1);
    t25 = fn_801DC9B0((u8*)0xF200EC1E);
    t26 = fn_801DC9B0((u8*)0x5D8630F8);
    t27 = fn_801DC9B0((u8*)0x2BFC9A28);
    t28 = fn_801DC9B0((u8*)0xBCF079A0);
    t29 = fn_801DC9B0((u8*)0xCDB02413);
    t30 = fn_801DC9B0((u8*)0x3220DC3F);
    t31 = fn_801DC9B0((u8*)0x9C194337);
    t3 = fn_801DC9B0((u8*)0x8AECDF1F);
    fn_801F5270(t3, t31, t30, t29, t28, t27, t26, t25);
}

// 0x8010B50C
// Looks up entry arg1->idx in the ring at +0xB0C (start/end window at +0xB00/+0xB04, count at +0xB08)
// and writes its position (3 floats at +0xC..+0x14) to out. Entry is NULL when out of range.
void fn_8010B50C(u8* out, u8* arg1) {
    s32 idx = *(s32*)(arg1 + 0x4);
    s32 pos = idx;
    s32 n = 0;
    s32 ok = 0;
    s32 lo;
    s32 hi;
    u8* p = NULL;

    if (idx >= 0) {
        n = *(s32*)(arg1 + 0xB08);
        if (idx < n) {
            hi = *(s32*)(arg1 + 0xB04);
            lo = *(s32*)(arg1 + 0xB00);
            if (hi < lo) {
                if (idx <= hi) {
                    pos = idx + n;
                }
                hi += n;
            }
            ok = (lo <= pos && pos <= hi) ? 1 : 0;
        }
    }
    if (ok != 0) {
        p = *(u8**)(arg1 + 0xB0C) + idx * 0x1C;
    }
    *(f32*)(out + 0x0) = *(f32*)(p + 0xC);
    *(f32*)(out + 0x4) = *(f32*)(p + 0x10);
    *(f32*)(out + 0x8) = *(f32*)(p + 0x14);
}

// 0x8010F7F8
// Dispatches script function func with signature sig when flag is set and the signature check passes.
u32 fn_8010F7F8(s32 func, u32 sig, u32* args, s32 flag) {
    u32 result = 0;

    if (flag == 0) {
        return 0;
    }
    if (fn_8010F880(sig, func, args + 1) != 0) {
        ScriptDispatch(func, sig, args, &result);
    }
    return result;
}

// 0x8011BF34
// Returns the text for the current lab entry (table at +0x25A8) or the "Not found" string.
const char* fn_8011BF34(u8* arg0) {
    s32 idx = *(s32*)(arg0 + 0x259C);

    if (idx < *(s32*)(arg0 + 0x2CA8)) {
        return *(const char**)(*(u8**)(arg0 + idx * 4 + 0x25A8) + 0x2C);
    }
    return LabTextFromId(TextIdFromHash(0x557U));
}

// 0x8011C018
// Same as fn_8011BF34 but for the entry at (arg1 + base index at +0x25A0).
const char* fn_8011C018(u8* arg0, s32 arg1) {
    s32 idx = arg1 + *(s32*)(arg0 + 0x25A0) * 4;

    if (idx >= 0 && idx < *(s32*)(arg0 + 0x2CA8)) {
        return *(const char**)(*(u8**)(arg0 + idx * 4 + 0x25A8) + 0x2C);
    }
    return LabTextFromId(TextIdFromHash(0x557U));
}

// 0x8011C22C
// Releases the current lab part and calls the object's vtable slots 0x10 and 0xC when the index is valid.
u8* fn_8011C22C(u8* arg0) {
    s32 idx = *(s32*)(arg0 + 0x259C);
    u8* ret = arg0;

    if (idx < *(s32*)(arg0 + 0x2CA8)) {
        u8* base = lbl_805FBBE0;
        u8* el = *(u8**)(arg0 + idx * 4 + 0x25A8);
        s32 k = *(s32*)(el + 0x54);

        LabReleasePart(base + 0xA30, *(PartInstance**)(base + k * 4 + 0xA58));
        (*(VFn*)(*(u8**)arg0 + 0x10))(arg0);
        ret = (*(VFnPtr*)(*(u8**)arg0 + 0xC))(arg0);
    }
    return ret;
}

// 0x8011D79C
// Returns the text for the record selected by the save block index (0x128) in the game db table.
const char* fn_8011D79C(void) {
    u8* data = *(u8**)((u8*)gGameDb + 0xC);
    u8* table = *(u8**)(data + 0x63C8);
    s32 idx = *(s32*)(lbl_805FBBE0 + 0x128);
    u8* rec = table + idx * 16;
    u32 hash = *(u32*)(rec + 0x4);

    return LabTextFromId(TextIdFromHash(hash));
}

// 0x801214E0
// Tail-calls fn_8012189C.
void fn_801214E0(void) {
    fn_8012189C();
}

// 0x80128318
// Calls the object's vtable slot 0x40 and returns 1.
s32 fn_80128318(void* ent, s32 zone, s32 arg) {
    (*(VFn*)(*(u8**)ent + 0x40))(ent);
    return 1;
}

// 0x80133D74
// Returns 1 when arg1's record (+0x18) is the current zone's record or is found in the zone's 0x54-byte list.
s32 fn_80133D74(u8* arg0, u8* arg1) {
    s32 found = 0;
    u8* zone = *(u8**)(arg0 + 0xE0);
    u8* t = *(u8**)(*(u8**)(lbl_803C4298 + 0x8) + 0x2C0);

    if (t != NULL) {
        u8* ent = *(u8**)(t + 0x4);
        u32 zi = *(u16*)(ent + 0x30);
        zone = *(u8**)(gWorldState + 0x1A4) + zi * gZoneResourceSize;
    }
    if (zone != NULL && *(s32*)(zone + 0x18) == *(s32*)(arg1 + 0x18)) {
        found = 1;
    } else if (zone != NULL) {
        s32 i = 0;
        s32 off = 0;
        while (i < *(s32*)((u8*)GetGlobalSettings() + 0x34)) {
            s32 list = *(s32*)(zone + 0x114);
            if (*(s32*)(arg1 + 0x18) == *(s32*)(list + off) && (*(u32*)(list + off + 0x4) & 1) != 0) {
                found = 1;
                break;
            }
            off += 0x54;
            i += 1;
        }
    }
    return found;
}

// 0x8013E3BC
// Copies up to 15 characters of src into the shared name buffer and returns it.
char* fn_8013E3BC(const char* src) {
    return strncpy((char*)lbl_803F2088, src, 15);
}

// 0x80142FBC
// Reports the failed test: formats the message buffer at lbl_803F22D8+0x204 and reopens the report handle.
void TestFailed(void) {
    char buf[0x100];
    const char* s = lbl_80375E18;
    const char* p1;
    const char* p2;

    if (*(void**)lbl_803F22D8 != NULL) {
        fn_80333B60(*(void**)lbl_803F22D8);
        *(void**)lbl_803F22D8 = NULL;
        strcpy(buf, (const char*)(lbl_803F22D8 + 0x204));
        p1 = s + 0xC;
        if (*(u8*)(lbl_803F22D8 + 0x4) != 0) {
            p1 = (const char*)(lbl_803F22D8 + 0x4);
        }
        p2 = s + 0x34;
        if (*(u8*)(lbl_803F22D8 + 0x104) != 0) {
            p2 = (const char*)(lbl_803F22D8 + 0x104);
        }
        SysSprintf((char*)(lbl_803F22D8 + 0x204), s + 0x20, (char*)(lbl_803F22D8 + 0x458), p1, p2, lbl_803F22D8);
        fn_80333B34(buf, (char*)(lbl_803F22D8 + 0x204));
        *(void**)lbl_803F22D8 = fn_80333D50((const char*)(lbl_803F22D8 + 0x204), lbl_805F9178);
    }
}

}  // extern "C"
