// PC platform layer: reads a DDS replacement texture (port_mods.cpp, docs/modding.md): a 2D texture in
// BC1 (DXT1), BC3 (DXT5) or BC7, with its mip levels. The blocks are not decoded: the renderer hands
// them to the graphics card as they are.
#ifdef _WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port/port_mods.h"

static u32 Le32(const u8* p) { return p[0] | p[1] << 8 | p[2] << 16 | (u32)p[3] << 24; }

u32 PortModLevelBytes(u32 dxgi, u32 w, u32 h, u32 level, u32* pitch) {
    u32 lw = w >> level, lh = h >> level;
    if (!lw) lw = 1;
    if (!lh) lh = 1;
    if (!dxgi) { if (pitch) *pitch = lw * 4; return lw * lh * 4; }
    u32 row = (lw + 3) / 4 * (dxgi == PORT_DXGI_BC1 ? 8 : 16);
    if (pitch) *pitch = row;
    return row * ((lh + 3) / 4);
}

BOOL PortDdsRead(const char* path, PortModImage* out, const char** why) {
    memset(out, 0, sizeof(*out));
    *why = "cannot be opened";
    FILE* f = fopen(path, "rb");
    if (!f) return FALSE;
    u8 head[148];
    size_t got = fread(head, 1, sizeof(head), f);
    *why = "is not a DDS file";
    if (got < 128 || memcmp(head, "DDS ", 4) || Le32(head + 4) != 124 || Le32(head + 76) != 32) { fclose(f); return FALSE; }
    u32 h = Le32(head + 12), w = Le32(head + 16), levels = Le32(head + 28), fourFlags = Le32(head + 80), caps2 = Le32(head + 112);
    u32 dxgi = 0, at = 128;
    if ((fourFlags & 4) && !memcmp(head + 84, "DXT1", 4)) dxgi = PORT_DXGI_BC1;
    else if ((fourFlags & 4) && !memcmp(head + 84, "DXT5", 4)) dxgi = PORT_DXGI_BC3;
    else if ((fourFlags & 4) && !memcmp(head + 84, "DX10", 4) && got >= 148) {
        u32 d = Le32(head + 128);                            // the sRGB kinds are taken as the plain ones: the game works in gamma space
        if (d >= 70 && d <= 72) dxgi = PORT_DXGI_BC1;
        else if (d >= 76 && d <= 78) dxgi = PORT_DXGI_BC3;
        else if (d >= 97 && d <= 99) dxgi = PORT_DXGI_BC7;
        if (Le32(head + 132) != 3 || (Le32(head + 136) & 4) || Le32(head + 140) > 1) dxgi = 0;
        at = 148;
    }
    *why = "is not a 2D texture in BC1 (DXT1), BC3 (DXT5) or BC7";
    if (!dxgi || (caps2 & 0x200200)) { fclose(f); return FALSE; }     // cube map or volume
    *why = "has a size that is not supported (width and height: multiples of 4, up to 8192)";
    if (!w || !h || w > 8192 || h > 8192 || (w & 3) || (h & 3)) { fclose(f); return FALSE; }
    u32 most = 1;
    while ((w >> most) || (h >> most)) most++;
    if (!levels) levels = 1;
    if (levels > most) levels = most;
    u32 total = 0;
    for (u32 i = 0; i < levels; i++) total += PortModLevelBytes(dxgi, w, h, i, NULL);
    u8* data = (u8*)malloc(total);
    *why = "is too large";
    if (!data) { fclose(f); return FALSE; }
    *why = "is cut short";
    if (fseek(f, (long)at, SEEK_SET) || fread(data, 1, total, f) != total) { free(data); fclose(f); return FALSE; }
    fclose(f);
    out->w = w; out->h = h; out->levels = levels; out->dxgi = dxgi; out->data = data;
    *why = NULL;
    return TRUE;
}
#endif
