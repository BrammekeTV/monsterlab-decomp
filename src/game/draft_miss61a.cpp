#include "types.h"
#include "game/part.h"  // GameDb, gGameDb, TextIdFromHash

struct StringTable;

// Save block pointer: the global holds a pointer to the save block (read as *(u8**)lbl_805FBBE0).
extern "C++" u8 lbl_805FBBE0[];
extern "C++" u8 lbl_8036D314[];           // 0x8036D314, address stored into object vtable slot
extern "C++" const char lbl_805F8200[];   // 0x805F8200, empty fallback string
extern "C++" const char lbl_80354800[];   // 0x80354800, text for TextId -1
extern "C++" const char lbl_805F8F94[];   // 0x805F8F94, "%d/%d" format
extern "C++" const char lbl_80376980[];   // 0x80376980, file version error format
extern "C++" StringTable* gStringTable;   // 0x805FB910
extern "C++" StringTable* gStringTable2;  // 0x805FB90C

extern "C" {
s32  fn_80222994(u8* s);     // 0x80222994
s32  fn_80226C24(u8* s);     // 0x80226C24
s32  fn_80226C2C(u8* s);     // 0x80226C2C
u8*  fn_80222704(u8* s, s32 a, s32 b);  // 0x80222704
void fn_80116BE8(u8* p);     // 0x80116BE8
void fn_80116E54(u8* p, s32 a, s32 b);  // 0x80116E54
void fn_80116EF0(u8* p, s32 a);         // 0x80116EF0
s32  fn_8014ACCC(s32, s32, s32, s32, s32*, s32*, s32, s32);  // 0x8014ACCC
void fn_8015B19C(void);      // 0x8015B19C
void fn_8016964C(u8* mgr, u8* tex);     // 0x8016964C
u8*  fn_801840E4(u8* a, u8* b, s32 c);  // 0x801840E4
void TextureRelease(void* tex);         // 0x80184028
s32  SysSprintf(char* buf, const char* fmt, ...);  // 0x80142EFC
}

typedef u8* (*VGetObjFn)(u8* self);
typedef void (*VSetupFn)(u8* self, u8* a, u8* b, u8* c);

// Text lookup for a TextIdFromHash result (shared by fn_8011EEF4 and fn_8011F32C).
// -1 gives lbl_80354800; an id with bit 31 set indexes gStringTable2 (bit cleared), else gStringTable.
static const char* TextLookupById(s32 id) {
    if (id == -1) return lbl_80354800;
    if ((id & 0x80000000) && gStringTable2 != NULL) {
        u8* st = (u8*)gStringTable2;
        const char** strings = *(const char***)st;
        if (strings != NULL) {
            u32 idx = (u32)id & 0x7FFFFFFF;
            if (idx < *(u32*)(st + 0xC)) return strings[idx];
        }
        return lbl_805F8200;
    }
    u8* st = (u8*)gStringTable;
    const char** strings = *(const char***)st;
    if (strings != NULL && (u32)id < *(u32*)(st + 0xC)) return strings[id];
    return lbl_805F8200;
}

// List record lookup shared by fn_8011EFFC, fn_8011F080 and fn_8011F104. NULL when index >= count (+0x2C).
static u8* GetListRecord(u8* arg0, s32 arg1) {
    if (arg1 >= *(s32*)(arg0 + 0x2C)) return NULL;
    u8* save = *(u8**)lbl_805FBBE0;
    if (*(u8*)(arg0 + 0x2B) != 0) return fn_80222704(save, arg1 + 1, 0);
    return fn_80222704(save, 0, *(s32*)(arg0 + 0x2C) - arg1 - 1);
}

extern "C" {

// 0x8015A210
u8* fn_8015A210(u8* arg0, s32 arg1, s32* arg2, s32 arg3) {
    s32 sp8;
    s32 total = 0;
    s32 count = arg1;
    *(u8*)(arg0 + 0xB) = (u8)(*(u8*)(arg0 + 0xB) | 2);
    if ((arg3 & 6) != 6) fn_8015B19C();
    u16 w = *(u16*)(arg0 + 0x4);
    u16 h = *(u16*)(arg0 + 0x6);
    u8* acc = *(u8**)(arg0 + 0x18);
    for (;;) {
        sp8 = 0;
        fn_8014ACCC(4, *(u8*)(arg0 + 0x9), (s32)w, (s32)h, &sp8, &total, 0, 0);
        count -= 1;
        if (count < 0) break;
        s32 halfW = (s32)w >> 1;
        w = 1;
        acc += sp8;
        if (halfW >= 1) w = (u16)halfW;
        s32 halfH = (s32)h >> 1;
        h = 1;
        if (halfH >= 1) h = (u16)halfH;
    }
    *arg2 = total;
    return acc;
}

// 0x8016B5E0
s32 fn_8016B5E0(u8* arg0, u8* arg1, s32 arg2) {
    u32 tagA = *(u32*)arg1;
    u32 tagB = *(u32*)(arg1 + 0x4);
    *(u8**)(arg0 + 0x4) = arg1;
    *(u8**)(arg0 + 0x0) = arg1;
    *(s32*)(arg0 + 0x8) = arg2;
    *(u8**)(arg0 + 0x4) = arg1 + 8;
    if (tagA != 0x53473231u || tagB != 0x57493030u) {  // "SG21", "WI00"
        char buf[0x10C];
        s32 a0 = (s8)(u8)(tagA >> 24);
        s32 a1 = (s8)(u8)(tagA >> 16);
        s32 a2 = (s8)(u8)(tagA >> 8);
        s32 a3 = (s8)(u8)tagA;
        s32 b0 = (s8)(u8)(tagB >> 24);
        s32 b1 = (s8)(u8)(tagB >> 16);
        s32 b2 = (s8)(u8)(tagB >> 8);
        s32 b3 = (s8)(u8)tagB;
        SysSprintf(buf, lbl_80376980, a0, a1, a2, a3, b0, b1, b2, b3,
                   'S', 'G', '2', '1', 'W', 'I', '0', '0');
        return 0;
    }
    return 1;
}

// 0x8016B724
u8* fn_8016B724(u8* arg0, u8* arg1, s32 arg2) {
    *(s32*)(arg0 + 0x4) += 8;
    u8* tex = fn_801840E4(arg1, arg0 + 0x4, arg2);
    if (tex != NULL && arg1 != NULL) {
        fn_8016964C(arg1, tex);
        s32 refs = *(s32*)(tex + 0x24) - 1;
        *(s32*)(tex + 0x24) = refs;
        if (refs == 0) TextureRelease(tex);
    }
    return tex;
}

// 0x801B4EA4
void fn_801B4EA4(u8* arg0, u8* arg1) {
    u8* obj = (*(VGetObjFn*)(*(u8**)arg0 + 0xC))(arg0);
    u8* sub = *(u8**)(arg0 + 0x4);
    *(u32*)(sub + 0x20) &= 0xFFFF7FFFu;
    if (arg1 == NULL) {
        if (obj != NULL) {
            u8* pa = *(u8**)(arg0 + 0x4);
            (*(VSetupFn*)(*(u8**)obj + 0x14))(obj, pa, arg0 + 0x8, pa + 0xC);
        }
    } else if (obj != NULL) {
        u8* pa = *(u8**)(arg0 + 0x4);
        (*(VSetupFn*)(*(u8**)obj + 0x14))(obj, pa, arg1, pa + 0xC);
    }
}

// 0x8011D894
s32 fn_8011D894(u8* arg0) {
    u8* db = gGameDb->data;
    u8* save = *(u8**)lbl_805FBBE0;
    u8* table = *(u8**)(db + 0x63C8);
    u8* ent = table + *(s32*)(save + 0x128) * 0x10;
    u8* extra = *(u8**)(db + 0x99D0);
    f32 scale = *(f32*)(extra + 0x194);
    f32 v = (f32)(s16)(*(s32*)(ent + 0xC) >> 16) * scale;
    return (s32)(u32)v;
}

// 0x8011D930
s32 fn_8011D930(u8* arg0) {
    u8* db = gGameDb->data;
    u8* save = *(u8**)lbl_805FBBE0;
    u8* table = *(u8**)(db + 0x63C8);
    u8* ent = table + *(s32*)(save + 0x128) * 0x10;
    return (s32)(s16)(*(s32*)(ent + 0xC) >> 16);
}

// 0x8011E9B8
void fn_8011E9B8(u8* arg0) {
    fn_80116E54(arg0 + 0x24, 0xA, 0xC);
    fn_80116EF0(arg0 + 0x24, *(s32*)(arg0 + 0x20) * 0xA);
}

// 0x8011EA20
u8* fn_8011EA20(u8* arg0) {
    fn_80116BE8(arg0);
    *(u8*)(arg0 + 0x2B) = 0;
    *(u8**)(arg0 + 0x0) = lbl_8036D314;
    *(s32*)(arg0 + 0x2C) = 0;
    *(s32*)(arg0 + 0x30) = 0;
    return arg0;
}

// 0x8011EDAC
s32 fn_8011EDAC(u8* arg0) {
    s32 id = fn_80222994(*(u8**)lbl_805FBBE0);
    return id != -1;
}

// 0x8011EEF4
const char* fn_8011EEF4(u8* arg0) {
    return TextLookupById(TextIdFromHash(fn_80226C24(*(u8**)lbl_805FBBE0)));
}

// 0x8011EFB8
s32 fn_8011EFB8(u8* arg0) {
    fn_80222994(*(u8**)lbl_805FBBE0);
    return 0;
}

// 0x8011EFFC
s32 fn_8011EFFC(u8* arg0, s32 arg1) {
    u8* rec = GetListRecord(arg0, arg1);
    if (rec != NULL && *(s32*)(rec + 0x8) == 2) return 1;
    return 0;
}

// 0x8011F080
s32 fn_8011F080(u8* arg0, s32 arg1) {
    u8* rec = GetListRecord(arg0, arg1);
    if (rec != NULL && *(s32*)(rec + 0x10) > 0) return 1;
    return 0;
}

// 0x8011F104
char* fn_8011F104(u8* arg0, s32 arg1) {
    u8* rec = GetListRecord(arg0, arg1);
    if (rec == NULL) return NULL;
    s32 total = *(s32*)(rec + 0x10);
    if (total <= 0) return NULL;
    char* buf = (char*)(arg0 + arg1 * 5 + 0x1C);
    SysSprintf(buf, lbl_805F8F94, *(s32*)(rec + 0xC), total);
    return buf;
}

// 0x8011F2FC
s32 fn_8011F2FC(u8* arg0) {
    return fn_80226C2C(*(u8**)lbl_805FBBE0) != 0;
}

// 0x8011F32C
const char* fn_8011F32C(u8* arg0) {
    return TextLookupById(TextIdFromHash(fn_80226C2C(*(u8**)lbl_805FBBE0)));
}

}  // extern "C"
