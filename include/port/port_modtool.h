// PC port: the mod tool's model view (port_modtool_mesh.cpp, docs/concepts/mod-tools.md). Finds the meshes of a
// zone image (a level zone, a part of shared_parts.sir) and decodes their display lists into triangles, the way the
// GX pipe reads them: a batch's list starts with its own vertex format and array addresses (CP writes), then the
// draws. Read from the big-endian image as it is on the disc; nothing is converted or written.
#ifndef PORT_MODTOOL_H
#define PORT_MODTOOL_H

#include <string>
#include <vector>
#include "types.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11ShaderResourceView;

struct MtVert {
    f32 p[3], n[3], uv[2];
    u32 rgba;                 // vertex colour 0 (white when the format has none)
};

struct MtBatch {
    u32 batch, list, size;    // offsets in the zone: the batch record, its display list and the list's size
    u32 material;             // offset of the material that draws it ({material, batch} of a mesh), or ~0u
    u32 texture;              // offset of the material's first texture, or ~0u
    u32 first, count;         // its triangles' vertices in MtZone::verts
    u32 skipped;              // draws of points and lines (not shown)
};

struct MtDraw { u32 batch, first, count; };   // a batch as a scene item places it (or as it is)

struct MtZone {
    u32 base = 0;             // the zone's Wii address: what the pointers in it are relative to
    std::vector<MtBatch> batches;
    std::vector<MtDraw> draws; // the whole zone: every batch where its scene items put it
    u32 items = 0;            // scene items found
    std::vector<MtVert> verts;
    f32 lo[3] = {0, 0, 0}, hi[3] = {0, 0, 0};
    std::string error;
};

// FALSE (and why in out->error) when the image holds no display list the tool can read.
bool MtZoneScan(const u8* zone, u32 size, MtZone* out);
// The texture a batch's material draws with, as RGBA8 (the renderer's decoder).
bool MtBatchTexture(const u8* zone, u32 size, const MtZone& z, const MtBatch& b, std::vector<u32>* rgba, u32* w, u32* h);

// An off-screen view: orbit camera around the zone's (or one batch's) box.
struct MtView;
MtView* MtViewCreate(ID3D11Device* dev);
void    MtViewDestroy(MtView* v);
struct MtCamera { f32 yaw = 0.6f, pitch = 0.35f, dist = 2.2f; bool textured = true, wire = false; };
// Draws the zone (only == -1) or one batch into a w x h picture; the picture to show, or NULL.
ID3D11ShaderResourceView* MtViewDraw(MtView* v, ID3D11DeviceContext* ctx, const void* key, const u8* zone, u32 size, const MtZone& z,
                                     int only, u32 w, u32 h, const MtCamera& cam);

#endif
