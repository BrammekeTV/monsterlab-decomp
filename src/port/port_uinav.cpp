// Keyboard / D-pad navigation for screens the console drives only with the pointer (the main menu,
// Options): the arrow keys move the emulated pointer to the next item. The items are the widgets
// fn_80199828 would pick: the top screen's tree (children +0x120, siblings +0x124) with a hit
// rectangle (+0x1B0..+0x1D8 in the widget's local space, matrix +0xC0..+0xE8, as fn_8019F810 reads it).
#include <math.h>
#include "types.h"

extern "C++" void* gScreenSets;     // 0x805FB654
extern "C++" s32   gScreenWidth;
extern "C++" s32   gScreenHeight;
extern f32 lbl_805FF95C;

extern "C" s32 fn_8019CCAC(u8* self);

typedef s32 (*PickFn)(u8* self, s32 mask, f32 x, f32 y);

struct UiItem { f32 x, y; bool hovered; };
static UiItem sItems[64];
static f32 gCurX, gCurY;
static int sItemCount;

static void Collect(u8* parent) {
    for (u8* c = *(u8**)(parent + 0x120); c != NULL && sItemCount < 64; c = *(u8**)(c + 0x124)) {
        u32 fl = *(u32*)(c + 0x64);
        f32 lo = *(f32*)(c + 0x1B0), hi = *(f32*)(c + 0x1D4), lo2 = *(f32*)(c + 0x1B4), hi2 = *(f32*)(c + 0x1D8);
        bool visible = (((fl & 2) && *(f32*)(c + 0x8C) > 0.0f) || !(fl & 0x400));
        bool eligible = (fl & 1) && lo != hi && (*(s32*)(c + 0xBC) != 0 || fn_8019CCAC(c) != 0) &&
                        (((fl & 2) && *(f32*)(c + 0x8C) > 0.0f) || !(fl & 0x400));
        if (eligible) {
            f32 w = lbl_805FF95C;
            f32 a = *(f32*)(c + 0xC0), b = *(f32*)(c + 0xCC), cc = *(f32*)(c + 0xC4), d = *(f32*)(c + 0xD0);
            f32 det = a * d - b * cc;
            if (fabsf(det) > 1e-6f) {
                f32 r1 = (lo + hi) * 0.5f - *(f32*)(c + 0xE4) - *(f32*)(c + 0xD8) * w;
                f32 r2 = (lo2 + hi2) * 0.5f - *(f32*)(c + 0xE8) - *(f32*)(c + 0xDC) * w;
                f32 x = (r1 * d - b * r2) / det, y = (a * r2 - cc * r1) / det;
                if ((*(PickFn*)(*(u8**)(c + 0x138) + 0x38))(c, 0, x, y) != 0) {
                    sItems[sItemCount].x = x;
                    sItems[sItemCount].y = y;
                    sItems[sItemCount].hovered = sItems[sItemCount].x == x && gCurX >= 0 && (*(PickFn*)(*(u8**)(c + 0x138) + 0x38))(c, 0, gCurX, gCurY) != 0;
                    sItemCount++;
                }
            }
        }
        else if ((fl & 1) && lo == hi && visible && (*(s32*)(c + 0xBC) != 0 || fn_8019CCAC(c) != 0)) {
            // No usable hit rectangle (the Options buttons): probe the widget's own hit test on a grid
            // and take the middle of the hits.
            f32 sx = 0, sy = 0; int n = 0;
            for (f32 y = 0; y < (f32)gScreenHeight; y += 6.0f)
                for (f32 x = 0; x < (f32)gScreenWidth; x += 6.0f)
                    if ((*(PickFn*)(*(u8**)(c + 0x138) + 0x38))(c, 0, x, y) != 0) { sx += x; sy += y; n++; }
            if (n > 0 && sItemCount < 64) {
                sItems[sItemCount].x = sx / n;
                sItems[sItemCount].y = sy / n;
                sItems[sItemCount].hovered = gCurX >= 0 && (*(PickFn*)(*(u8**)(c + 0x138) + 0x38))(c, 0, gCurX, gCurY) != 0;
                sItemCount++;
            }
        }
        Collect(c);
    }
}

// Direction (dx, dy): -1/+1 (dy +1 = down). Returns TRUE and the KPAD pointer (-1..1) of the item to hover.
extern "C" BOOL PortUiNavigate(int dx, int dy, f32* px, f32* py) {
    u8* sets = (u8*)gScreenSets;
    if (sets == NULL) return FALSE;
    u8* stack = *(u8**)(sets + 0x84);
    u8* cursor = *(u8**)(sets + 0x7C);
    u8* pad = *(u8**)(sets + 0x78);
    if (stack == NULL || cursor == NULL || pad == NULL) return FALSE;
    s32 n = *(s32*)(stack + 0x60);
    if (n < 1) return FALSE;
    u8* top = *(u8**)(stack + n * 4 + 0x3C);
    if (top == NULL || *(u8**)(top + 0x144) != NULL) return FALSE;    // a screen with its own selection grid keeps it
    sItemCount = 0;
    gCurX = *(f32*)(cursor + 0x68); gCurY = *(f32*)(cursor + 0x6C);
    Collect(top);
    if (sItemCount == 0) return FALSE;
    f32 cx = *(f32*)(cursor + 0x68), cy = *(f32*)(cursor + 0x6C);
    int best = -1, wrap = -1;
    f32 bestScore = 1e30f, wrapScore = 1e30f;
    for (int i = 0; i < sItemCount; i++) {
        f32 along = dy ? (sItems[i].y - cy) * dy : (sItems[i].x - cx) * dx;
        f32 across = dy ? fabsf(sItems[i].x - cx) : fabsf(sItems[i].y - cy);
        if (!sItems[i].hovered && along > 3.0f && across <= along && along + 0.5f * across < bestScore) { bestScore = along + 0.5f * across; best = i; }
        f32 far = -along;     // the other end of the screen, for wrapping
        if (far > 3.0f && far - 0.5f * across < wrapScore && far > 0) { wrapScore = far; wrap = i; }
    }
    if (best < 0) {          // none in the straight line: the nearest one in that direction at all
        for (int i = 0; i < sItemCount; i++) {
            f32 along = dy ? (sItems[i].y - cy) * dy : (sItems[i].x - cx) * dx;
            f32 across = dy ? fabsf(sItems[i].x - cx) : fabsf(sItems[i].y - cy);
            if (!sItems[i].hovered && along > 3.0f && along + 0.5f * across < bestScore) { bestScore = along + 0.5f * across; best = i; }
        }
    }
    if (best < 0) {          // none further: the item farthest the other way
        f32 m = -1e30f;
        for (int i = 0; i < sItemCount; i++) {
            f32 along = dy ? (sItems[i].y - cy) * dy : (sItems[i].x - cx) * dx;
            if (-along > m) { m = -along; wrap = i; }
        }
        best = wrap;
    }
    if (best < 0) return FALSE;
    f32 hw = 0.5f * (f32)gScreenWidth, hh = 0.5f * (f32)gScreenHeight;
    f32 sx = *(f32*)(pad + 0xB0), sy = *(f32*)(pad + 0xB4);
    if (sx == 0.0f || sy == 0.0f || hw == 0.0f || hh == 0.0f) return FALSE;
    *px = (sItems[best].x - hw) / (hw * sx);
    *py = (sItems[best].y - hh) / (hh * sy);
    return TRUE;
}
