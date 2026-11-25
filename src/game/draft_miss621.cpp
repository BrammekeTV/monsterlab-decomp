// Draft fixes (batch miss621): a segment-sweep helper, the part-action menu builder, the
// character teardown, a tree free and an effect-timer tick.
#include "types.h"
#include <math.h>
#include "game/part.h"

extern "C++" void* gEventMgr;   // 0x805FB6C8

extern "C" {
void   EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);   // 0x801BA2FC
s32    BattleListActions(u8* stats, void* out);   // 0x8021415C (asm also sets r5 = 10, r6 = 0)
const char* ReactionName(s32 id);
int    SysSprintf(char* buf, const char* fmt, ...);   // 0x80142EF8
s32    TriggerDoAction(void* action, void* a1, void* a2);   // 0x801E0054
char*  fn_80040EFC(s32 idx, u8* buf);
void   fn_80036B90(u8* self, f32* pos);   // 0x80036B90
void   fn_80062F58(u8* p, s32 mode);   // 0x80062F58
void   fn_800D7394(u8* arg);   // 0x800D7394
void   fn_801B381C(u8* menu, const char* name, void* a, void* b, void* c, void* d, void* e, void* user);   // 0x801B381C
void   fn_801B38BC(u8* p);   // 0x801B38BC
u8*    fn_801B3938(u8* a3);   // 0x801B3938 (asm returns *(a3 + 0x64))
void   fn_801C3E94(u8* mgr, u8* obj);   // 0x801C3E94 (asm ignores r3, ors 0xA into obj + 4)
void   fn_801C83AC(u8* mgr, u8* item);   // 0x801C83AC
void   fn_801E839C(u8* p);   // 0x801E839C
u32    fn_801EC880(u8* p, u32 hash);   // 0x801EC880
void   fn_801ECFA0(u8* p, s32 idx);   // 0x801ECFA0
void   fn_8020A398(s32 handle);   // 0x8020A398
const char* fn_8020E438(s32 idx);   // 0x8020E438
u8     fn_8021EF70(u8* part);   // 0x8021EF70
s32    fn_8003FF7C(u8* a3);   // 0x8003FF7C
void   operator_delete(void* p);   // 0x801D426C
int    strcmp(const char* a, const char* b);
void   fn_8006F794(u8* self, u8* node);   // 0x8006F794

extern u8 gGame[];   // 0x803B53A0
extern u8 gWorldState[];   // 0x803E6D00
extern const char lbl_80358538[];
extern u8 lbl_8047C760[];   // effect manager
extern u8 lbl_8047C7AC[];
extern void* lbl_805FB058;
extern f32 lbl_80347684[];
extern f32 lbl_805FCEF8, lbl_805FD0E0, lbl_805FD300;
extern f32 lbl_805FD318, lbl_805FD368, lbl_805FD378, lbl_805FD37C;
extern f32 lbl_805FD3B0, lbl_805FD3B4, lbl_805FD3B8, lbl_805FD3BC;
}

static const u32 kHashCC70BCDE = 0xCC70BCDEu;   // no name known
static const u32 kHash5F9EE3AD = 0x5F9EE3ADu;   // no name known

extern "C" {

// 0x80037058
// Steps along the segment from `from` to `to` in units of half the object's radius (table
// lbl_80347684, indexed by obj + 0x820), calling fn_80036B90 on each sample point, far end first.
// Does nothing unless to.x > obj.x.
void fn_80037058(u8* obj, f32* to, f32* from) {
    fn_80036B90(obj, from);
    if (to[0] > *(f32*)(obj + 0x0)) {
        f32 dx = to[0] - from[0];
        f32 dy = to[1] - from[1];
        f32 dz = to[2] - from[2];
        f32 len2 = dz * dz + (dx * dx + dy * dy);
        f32 len = 0.0f;
        if (!(len2 <= 0.0f)) len = sqrtf(len2);   // original: frsqrte with two Newton steps
        s32 n = (s32)(len / (lbl_805FCEF8 * lbl_80347684[*(s32*)(obj + 0x820)]));
        for (; n >= 0; n--) {
            f32 p[3];
            if (len2 == 0.0f) {
                p[0] = 0.0f;
                p[1] = 0.0f;
                p[2] = 0.0f;
            } else {
                f32 inv = 1.0f / sqrtf(len2);
                p[0] = dx * inv;
                p[1] = dy * inv;
                p[2] = dz * inv;
            }
            p[0] = (f32)n * p[0];
            p[1] = (f32)n * p[1];
            p[2] = (f32)n * p[2];
            p[0] = p[0] + to[0];
            p[1] = p[1] + to[1];
            p[2] = p[2] + to[2];
            fn_80036B90(obj, p);
        }
    }
}

// 0x8003FC6C
// Rebuilds the action menu (menu object at self + 0x84): a title, the fighter's name line, then
// one entry per usable (limb, action) pair of fighter `arg1` with the action name, the limb part
// name and its two stats (scaled by the part's tier when the limb has a part). Each entry's user
// data is a 0xC record in self + 4.. (limb, action). If the previously selected entry is no longer
// present the selection resets to the first list node.
void fn_8003FC6C(u8* self, u8* arg1) {
    char nameBuf[0x40];
    char title[0x100];
    char line[0x100];
    char limbName[0x80];
    char actName[0x80];
    u8 opts[0x50];
    PartDef def;
    u8* sel;
    s32 found = 0;

    fn_801B38BC(*(u8**)(self + 0x84));
    fn_801B381C(*(u8**)(self + 0x84), lbl_80358538 + 0x18, NULL, NULL, NULL, NULL,
                *(void**)((u8*)lbl_805FB058 + 0x260), self);
    fn_80040EFC(*(s32*)(arg1 + 0x44), (u8*)nameBuf);   // result unused, the text is in nameBuf
    SysSprintf(title, lbl_80358538 + 0x28, nameBuf);
    fn_801B381C(*(u8**)(self + 0x84), title, NULL, NULL, NULL, NULL, NULL, self);
    s32 n = BattleListActions(arg1 + 0x20, opts);
    u8* list = *(u8**)(*(u8**)(*(u8**)(self + 0x84) + 0x60) + 0x14);
    sel = list != NULL ? list + 8 : NULL;
    u8* ent = opts;
    u8* rec = self;
    for (s32 i = 0; i < n; i++) {
        u8** limb = *(u8***)ent;
        s32 defIdx = **(s32**)limb;
        PartDbGetDef(&def, gGameDb, defIdx);
        const u8* row = *(const u8**)(gGameDb->data + 0x430) + *(s32*)(ent + 4) * 0x14;
        s32 type = *(const s32*)(row + 4);
        u32 w0c = *(const u32*)(row + 0xC);
        u32 w10 = *(const u32*)(row + 0x10);
        SysSprintf(actName, "%s", ReactionName(type));
        SysSprintf(limbName, "%s", fn_8020E438(defIdx));
        s32 s1 = (s8)(u8)(w10 >> 24);
        s32 s2 = (s16)(w0c & 0xFFFF);
        if (*limb != NULL) {
            u8 tier = fn_8021EF70(*(u8**)limb);
            s1 = (s32)((f32)(s1 * tier) / lbl_805FD0E0);
            s2 = (s32)((f32)(s2 * tier) / lbl_805FD0E0);
        }
        SysSprintf(line, lbl_80358538 + 0x4C, limbName, actName, s1, s2);
        *(void**)(rec + 0x4) = *(void**)ent;
        *(s32*)(rec + 0x8) = *(s32*)(ent + 4);
        fn_801B381C(*(u8**)(self + 0x84), line, NULL, NULL, NULL, (void*)fn_8003FF7C, NULL, rec);
        if (strcmp((const char*)(fn_801B3938(*(u8**)(self + 0x84)) + 0x60), line) == 0) found = 1;
        ent += 8;
        rec += 0xC;
    }
    if (found == 0) *(u8**)(*(u8**)(self + 0x84) + 0x64) = sel;
}

// 0x8005382C
// Character teardown: removes `p` from the character table (gGame + 0x140), releases its effect,
// runs the exit actions, and for mode != 0 clears its slots and handle; mode 2 also fires the
// death-style actions, the 0x5F9EE3AD event (when the object has no pending state) and sets the
// finished flag.
void fn_8005382C(u8* p, s32 mode) {
    u8** slots = (u8**)(gGame + 0x140);
    for (;;) {
        u8 cnt = *(u8*)(gGame + 0x13C);
        s32 i = 0;
        s32 idx = -1;
        for (; (u8)i < cnt; i++) {
            if (slots[(u8)i] == p) {
                idx = (u8)i;
                break;
            }
        }
        if (idx == -1) break;
        slots[idx] = NULL;
        if (idx == *(u8*)(gGame + 0x13C)) *(u8*)(gGame + 0x13C) = (u8)(idx - 1);
    }
    u8* inner = *(u8**)(p + 0x4);
    if (inner == NULL) return;
    if (*(u32*)(inner + 0x20) & 0x20000) return;
    u8* item = *(u8**)(p + 0x15C);
    if (item != NULL) {
        fn_801C83AC(lbl_8047C760, item);
        *(u8**)(p + 0x15C) = NULL;
    }
    s32 hi = (s32)fn_801EC880(p + 0x94, kHashCC70BCDE);
    if (hi >= 0) fn_801ECFA0(p + 0x94, hi);
    u8* owner = *(u8**)(p + 0x9C);
    if (owner != NULL && ((*(u32*)(owner + 0x84) >> 27) & 0xF)) TriggerDoAction(owner + 0x84, p, NULL);
    if ((*(u32*)(p + 0x54) >> 27) & 0xF) TriggerDoAction(p + 0x54, p, NULL);
    fn_80062F58(p, mode);
    if (mode == 0) return;
    u8* owner2 = *(u8**)(p + 0x9C);
    for (s32 i = 0; i < *(s32*)(owner2 + 0x114); i++) {
        u8* slot = *(u8**)(p + 0x81C + i * 4);
        if (slot != NULL) {
            fn_801C3E94(lbl_8047C7AC, slot);
            *(u8**)(p + 0x81C + i * 4) = NULL;
        }
    }
    s32 handle = *(s32*)(p + 0x830);
    if (handle != -1) {
        fn_8020A398(handle);
        *(s32*)(p + 0x830) = -1;
    }
    *(f32*)(p + 0x834) = lbl_805FD300;
    if (mode != 2) return;
    inner = *(u8**)(p + 0x4);
    if (!(*(u32*)(inner + 0x20) & 0x08000000)) {
        u8* o = *(u8**)(p + 0x9C);
        if (o != NULL && ((*(u32*)(o + 0x78) >> 27) & 0xF)) TriggerDoAction(o + 0x78, p, NULL);
        if ((*(u32*)(p + 0x48) >> 27) & 0xF) TriggerDoAction(p + 0x48, p, NULL);
        *(u32*)(*(u8**)(p + 0x4) + 0x20) |= 0x08000000;
    }
    fn_801E839C(p + 0xFC);
    fn_800D7394(*(u8**)(p + 0x2C));
    inner = *(u8**)(p + 0x4);
    s32 st = *(s32*)(inner + 0x28);
    if (st == 0) {
        s32 ev = *(s32*)(p + 0xF8);
        if (ev != 0) EventSendTo(gEventMgr, (u32)ev, p, kHash5F9EE3AD, 0, 0);
        *(u32*)(p + 0x838) |= 0x80;
        return;
    }
    if (st != -1) {
        *(u32*)(p + 0x838) |= 0x80;
        *(u32*)(inner + 0x20) |= 0x20000;
    }
}

// 0x80059EB0
// Per-frame tick of an effect timer: counts the countdown at +0x6D0 down by the step at +0x24, and
// when the value at +0x7D8 is inside the open range (lbl_805FD37C, lbl_805FD378) and the countdown
// has run out, re-arms it and picks a rate from the mode nibble at +0x748 (some modes scale it
// from the sub-mode, mode 7 from +0x5F4). A second nibble scales the vector at +0x810. If the
// resulting duration, relative to the world's +0x5F4, falls outside the range, virtual slot 0x50
// (restart) is called.
void fn_80059EB0(u8* p) {
    f32 cd = *(f32*)(p + 0x6D0);
    if (cd > lbl_805FD300) *(f32*)(p + 0x6D0) = cd - *(f32*)(p + 0x24);
    f32 v = *(f32*)(p + 0x7D8);
    if (!(v > lbl_805FD37C && v < lbl_805FD378)) return;
    f32 cd2 = *(f32*)(p + 0x6D0);
    if (!(cd2 <= lbl_805FD300)) return;
    u32 flags = *(u32*)(p + 0x748);
    *(f32*)(p + 0x6D0) = cd2 + lbl_805FD3B0;
    u32 mode = (flags >> 4) & 0xF;
    u32 sub = (flags >> 2) & 3;
    f32 rate = lbl_805FD300;
    switch (mode) {
    case 1:
    case 2:
    case 3:
    case 5:
    case 6:
        rate = lbl_805FD318 + lbl_805FD3B4 * (f32)(sub / 3);
        break;
    case 7:
        rate = lbl_805FD3B8 * *(f32*)(p + 0x5F4);
        break;
    default:
        break;
    }
    u32 scaleMode = (*(u32*)(p + 0x748) >> 12) & 0xF;
    if (scaleMode == 1) {
        *(f32*)(p + 0x810) = *(f32*)(p + 0x810) * lbl_805FD368;
        *(f32*)(p + 0x814) = *(f32*)(p + 0x814) * lbl_805FD368;
        *(f32*)(p + 0x818) = *(f32*)(p + 0x818) * lbl_805FD368;
    } else if (scaleMode == 3) {
        *(f32*)(p + 0x810) = *(f32*)(p + 0x810) * lbl_805FD3B0;
        *(f32*)(p + 0x814) = *(f32*)(p + 0x814) * lbl_805FD3B0;
        *(f32*)(p + 0x818) = *(f32*)(p + 0x818) * lbl_805FD3B0;
    }
    u8* world = *(u8**)(*(u8**)gWorldState + 0x8);
    f32 dur = rate * lbl_805FD3BC * (*(f32*)(p + 0x5F4) / *(f32*)(world + 0x5F4));
    if (!(dur > lbl_805FD37C && dur < lbl_805FD378)) {
        typedef void (*RestartFn)(u8*, s32, s32, s32, s32);
        // r5 is a leftover register in the original; passed as 0 here
        (*(RestartFn*)(*(u8**)p + 0x50))(p, 0, 0, 0, 0);
    }
}

// Helper (inlined in the original): frees the subtree levels below `n`; nodes `depth` levels down
// are released by recursing into fn_8006F794. Each node is two child pointers at +0 and +4.
static void FreeTree(u8* self, u8* n, int depth) {
    for (int k = 0; k < 8; k += 4) {
        u8* c = *(u8**)(n + k);
        if (c == NULL) continue;
        if (depth == 0) {
            fn_8006F794(self, c);
        } else {
            FreeTree(self, c, depth - 1);
            operator_delete(c);
        }
    }
}

// 0x8006F794
// Frees the tree node `node` together with its subtree (three levels freed inline, the fourth
// level through recursion).
void fn_8006F794(u8* self, u8* node) {
    FreeTree(self, node, 3);
    operator_delete(node);
}

}  // extern "C"
