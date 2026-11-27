// PC port: the mod tool's model view (include/port/port_modtool.h, docs/concepts/mod-tools.md).
//
// Finding the meshes of a zone image: a batch (docs/loader.md, "Batches of mesh parts") holds at +0x18 the
// distance to its display list and at +0x24 the list's size; fn_801873C0 calls that list. Every list of the game
// starts with the CP writes of its vertex format and array addresses (as tools/mods/blackhole/build_mod.py writes
// one), so a list is read on its own. The array addresses are physical (0x1xxxxxxx); the zone's Wii address comes
// from the meshes: a mesh part {material, batch} (mesh +0x70) points at a batch, so the address that makes the most
// batch pointers land on batches is the zone's.
//
// The view draws the triangles with the first texture of each batch's material, lit from the camera, without the
// skeleton's matrices: a skinned part shows its pieces in their bones' spaces.
#ifdef _WIN32
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <math.h>
#include <string.h>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

#include "port/port_modtool.h"

#pragma comment(lib, "d3dcompiler.lib")

BOOL PortGxDecodeTexture(const u8* src, u32 w, u32 h, u32 fmt, const u8* tlut, u32 tlutFmt, u32* out);   // port_gfx_d3d11.cpp
u32  PortGxTextureBytes(u32 w, u32 h, u32 fmt);

namespace {

inline u32 Be32(const u8* p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }
inline u16 Be16(const u8* p) { return (u16)(p[0] << 8 | p[1]); }

u32 CompSize(u32 fmt) { return fmt == 0 || fmt == 1 ? 1 : fmt == 2 || fmt == 3 ? 2 : fmt == 4 ? 4 : 0; }
u32 ColorSize(u32 fmt) { static const u8 k[8] = {2, 3, 4, 2, 3, 4, 0, 0}; return k[fmt & 7]; }

f32 Comp(const u8* p, u32 fmt, u32 frac) {
    switch (fmt) {
        case 0: return (f32)p[0] / (f32)(1u << frac);
        case 1: return (f32)(s8)p[0] / (f32)(1u << frac);
        case 2: return (f32)Be16(p) / (f32)(1u << frac);
        case 3: return (f32)(s16)Be16(p) / (f32)(1u << frac);
        case 4: { u32 v = Be32(p); f32 f; memcpy(&f, &v, 4); return f; }
    }
    return 0;
}

u32 Color(const u8* p, u32 fmt) {   // RGBA8 in memory order (r lowest)
    auto rgba = [](u32 r, u32 g, u32 b, u32 a) { return r | g << 8 | b << 16 | a << 24; };
    switch (fmt) {
        case 0: { u32 c = Be16(p); u32 r = c >> 11, g = c >> 5 & 63, b = c & 31; return rgba(r << 3 | r >> 2, g << 2 | g >> 4, b << 3 | b >> 2, 255); }
        case 1: case 2: return rgba(p[0], p[1], p[2], 255);
        case 3: { u32 c = Be16(p); return rgba((c >> 12) * 17, (c >> 8 & 15) * 17, (c >> 4 & 15) * 17, (c & 15) * 17); }
        case 4: { u32 c = (u32)p[0] << 16 | p[1] << 8 | p[2]; auto e = [](u32 v) { return v << 2 | v >> 4; };
                  return rgba(e(c >> 18), e(c >> 12 & 63), e(c >> 6 & 63), e(c & 63)); }
        case 5: return rgba(p[0], p[1], p[2], p[3]);
    }
    return 0xFFFFFFFFu;
}

struct Decoder {
    const u8* z;
    u32 size, base;
    u32 cp[256];

    // The bytes of array `n`'s element `idx`, or NULL outside the zone.
    const u8* Elem(u32 n, u32 idx, u32 bytes) const {
        u32 addr = (cp[0xA0 + n] | 0x80000000u) - base;
        u64 at = (u64)addr + (u64)idx * cp[0xB0 + n];
        return at + bytes <= size ? z + at : NULL;
    }

    // One draw's vertices (TRUE) into tri-list form; FALSE when the list does not read as GX commands.
    bool Draw(u32 op, const u8*& p, const u8* end, std::vector<MtVert>* out, u32* skipped) {
        if (p + 2 > end) return false;
        u32 vat = op & 7, prim = op & 0xF8, count = Be16(p);
        p += 2;
        u32 lo = cp[0x50], hi = cp[0x60], a = cp[0x70 + vat], b = cp[0x80 + vat], c = cp[0x90 + vat];
        u32 posFmt = a >> 1 & 7, posCnt = a & 1 ? 3 : 2, posFrac = a >> 4 & 31;
        u32 nrmFmt = a >> 10 & 7, nbt = a >> 9 & 1, nrm3 = a >> 31;
        u32 c0Fmt = a >> 14 & 7, c1Fmt = a >> 18 & 7;
        u32 texCnt[8], texFmt[8], texFrac[8];
        texCnt[0] = a >> 21 & 1; texFmt[0] = a >> 22 & 7; texFrac[0] = a >> 25 & 31;
        texCnt[1] = b & 1; texFmt[1] = b >> 1 & 7; texFrac[1] = b >> 4 & 31;
        texCnt[2] = b >> 9 & 1; texFmt[2] = b >> 10 & 7; texFrac[2] = b >> 13 & 31;
        texCnt[3] = b >> 18 & 1; texFmt[3] = b >> 19 & 7; texFrac[3] = b >> 22 & 31;
        texCnt[4] = b >> 27 & 1; texFmt[4] = b >> 28 & 7; texFrac[4] = c & 31;
        texCnt[5] = c >> 5 & 1; texFmt[5] = c >> 6 & 7; texFrac[5] = c >> 9 & 31;
        texCnt[6] = c >> 14 & 1; texFmt[6] = c >> 15 & 7; texFrac[6] = c >> 18 & 31;
        texCnt[7] = c >> 23 & 1; texFmt[7] = c >> 24 & 7; texFrac[7] = c >> 27 & 31;
        u32 mPos = lo >> 9 & 3, mNrm = lo >> 11 & 3, mC0 = lo >> 13 & 3, mC1 = lo >> 15 & 3;
        std::vector<MtVert> v(count);
        for (u32 i = 0; i < count; i++) {
            MtVert& x = v[i];
            memset(&x, 0, sizeof(x));
            x.rgba = 0xFFFFFFFFu;
            x.n[1] = 1;
            u32 mtx = (lo & 1) + ((lo >> 1 & 1) + (lo >> 2 & 1) + (lo >> 3 & 1) + (lo >> 4 & 1) + (lo >> 5 & 1) + (lo >> 6 & 1) + (lo >> 7 & 1) + (lo >> 8 & 1));
            if (p + mtx > end) return false;
            p += mtx;                                        // matrix indices: the skeleton is not applied
            auto attr = [&](u32 mode, u32 n, u32 bytes, const u8** at) -> bool {
                *at = NULL;
                if (mode == 0) return true;
                if (mode == 1) { if (p + bytes > end) return false; *at = p; p += bytes; return true; }
                u32 isz = mode == 2 ? 1 : 2;
                if (p + isz > end) return false;
                u32 idx = isz == 1 ? p[0] : Be16(p);
                p += isz;
                *at = Elem(n, idx, bytes);
                return true;
            };
            const u8* q;
            if (!attr(mPos, 0, CompSize(posFmt) * posCnt, &q)) return false;
            if (q) for (u32 k = 0; k < posCnt; k++) x.p[k] = Comp(q + k * CompSize(posFmt), posFmt, posFrac);
            if (mNrm) {
                u32 nfr = nrmFmt == 1 ? 6 : nrmFmt == 3 ? 14 : 0, one = CompSize(nrmFmt) * 3;
                u32 bytes = one * (nbt ? 3 : 1);
                if (nbt && nrm3 && mNrm >= 2) {              // three indices, one per vector
                    for (int k = 0; k < 3; k++) { const u8* r; if (!attr(mNrm, 1, one, &r)) return false; if (k == 0) q = r; }
                } else if (!attr(mNrm, 1, bytes, &q)) return false;
                if (q) for (u32 k = 0; k < 3; k++) x.n[k] = Comp(q + k * CompSize(nrmFmt), nrmFmt, nfr);
            }
            if (!attr(mC0, 2, ColorSize(c0Fmt), &q)) return false;
            if (q) x.rgba = Color(q, c0Fmt);
            if (!attr(mC1, 3, ColorSize(c1Fmt), &q)) return false;
            for (u32 t = 0; t < 8; t++) {
                u32 mode = hi >> (2 * t) & 3, n = texCnt[t] ? 2 : 1;
                if (!attr(mode, 4 + t, CompSize(texFmt[t]) * n, &q)) return false;
                if (q && t == 0) for (u32 k = 0; k < n; k++) x.uv[k] = Comp(q + k * CompSize(texFmt[0]), texFmt[0], texFrac[0]);
            }
        }
        switch (prim) {
            case 0x80: for (u32 i = 0; i + 3 < count; i += 4) { const MtVert* q4 = &v[i]; out->insert(out->end(), {q4[0], q4[1], q4[2], q4[0], q4[2], q4[3]}); } break;
            case 0x90: for (u32 i = 0; i + 2 < count; i += 3) out->insert(out->end(), {v[i], v[i + 1], v[i + 2]}); break;
            case 0x98: for (u32 i = 0; i + 2 < count; i++) {
                           if (i & 1) out->insert(out->end(), {v[i + 1], v[i], v[i + 2]});
                           else out->insert(out->end(), {v[i], v[i + 1], v[i + 2]});
                       } break;
            case 0xA0: for (u32 i = 1; i + 1 < count; i++) out->insert(out->end(), {v[0], v[i], v[i + 1]}); break;
            default: (*skipped)++; break;                    // lines, line strips, points
        }
        return true;
    }

    bool List(u32 off, u32 bytes, std::vector<MtVert>* out, u32* skipped) {
        memset(cp, 0, sizeof(cp));
        const u8* p = z + off;
        const u8* end = p + bytes;
        while (p < end) {
            u32 op = *p++;
            if (op == 0x00 || op == 0x48) continue;                         // NOP, invalidate vertex cache
            if (op == 0x08) { if (p + 5 > end) return false; cp[p[0]] = Be32(p + 1); p += 5; continue; }
            if (op == 0x10) { if (p + 4 > end) return false; u32 n = Be16(p) + 1u; p += 4 + 4 * n; continue; }
            if (op == 0x20 || op == 0x28 || op == 0x30 || op == 0x38 || op == 0x61) { p += 4; continue; }
            if (op == 0x40) { p += 8; continue; }
            if (op >= 0x80 && op < 0xC0) { if (!Draw(op, p, end, out, skipped)) return false; continue; }
            return false;
        }
        return true;
    }
};

bool LooksLikeList(const u8* z, u32 size, u32 l) {
    if (l + 24 > size) return false;
    for (int k = 0; k < 4; k++)
        if (z[l + k * 6] != 0x08 || (z[l + k * 6 + 1] >> 4) < 5 || (z[l + k * 6 + 1] >> 4) > 0xB) return false;
    return true;
}

}  // namespace

bool MtZoneScan(const u8* z, u32 size, MtZone* out) {
    *out = MtZone();
    // Batches: a record whose +0x18 leads to a list of the zone and +0x24 is its size (32-byte multiples, as GX wants).
    std::unordered_map<u32, std::pair<u32, u32>> at;          // list -> batch, size
    for (u32 b = 0; b + 0x28 <= size; b += 4) {
        u32 d = Be32(z + b + 0x18), n = Be32(z + b + 0x24);
        if (!d || d >= size - b || ((b + d) & 31) || !n || (n & 31) || n > size - (b + d)) continue;
        if (!at.count(b + d) && LooksLikeList(z, size, b + d)) at[b + d] = {b, n};
    }
    if (at.empty()) { out->error = "no display list found"; return false; }
    // The zone's address: the one that makes mesh parts point at batches.
    std::unordered_map<u32, std::vector<u32>> words;           // pointer-looking word -> where it stands
    for (u32 p = 0; p + 4 <= size; p += 4) { u32 w = Be32(z + p); if (w >= 0x90000000u && w < 0x94000000u) words[w].push_back(p); }
    u32 first = at.begin()->first, arr = 0;
    for (u32 k = 0; k < 0x60 && first + k + 6 <= size; k += 6)
        if (z[first + k] == 0x08 && z[first + k + 1] == 0xA0) { arr = Be32(z + first + k + 2) | 0x80000000u; break; }
    // Candidates: a pointer word minus a batch's offset (for a few batches), with the first list's position array
    // inside the zone; the one that the most batches agree with wins.
    std::vector<u32> batchOffs;
    for (auto& kv : at) batchOffs.push_back(kv.second.first);
    std::sort(batchOffs.begin(), batchOffs.end());
    std::unordered_set<u32> cands;
    for (size_t i = 0; i < batchOffs.size() && i < 4; i++)
        for (auto& kv : words) {
            u32 c = kv.first - batchOffs[i];
            if (arr && c <= arr && arr - c < size && !(c & 0x1F)) cands.insert(c);
        }
    int best = -1;
    for (u32 c : cands) {
        int score = 0;
        for (size_t i = 0; i < batchOffs.size() && i < 256; i++) {
            auto it = words.find(c + batchOffs[i]);
            if (it == words.end()) continue;
            score++;
            for (u32 p : it->second) {                       // a mesh part {material, batch}: the material is marked 0x53
                u32 m = p >= 4 ? Be32(z + p - 4) - c : ~0u;
                if (m + 4 <= size && Be32(z + m) == 0x53) { score += 2; break; }
            }
        }
        if (score > best || (score == best && c < out->base)) { best = score; out->base = c; }
    }
    if (best <= 0) { out->error = "the zone's address was not found (no mesh points at a batch)"; return false; }
    // Materials: the word before a batch pointer in a mesh's part list.
    std::unordered_map<u32, u32> material;                     // batch -> material
    std::vector<u32> partAt;                                    // where a mesh part {material, batch} stands
    for (u32 p = 4; p + 4 <= size; p += 4) {
        u32 w = Be32(z + p) - out->base, m = Be32(z + p - 4) - out->base;
        if (w < size && m + 0x40 <= size && Be32(z + m) == 0x53 && at.count(w + Be32(z + w + 0x18))) { material[w] = m; partAt.push_back(p - 4); }
    }
    Decoder dec;
    dec.z = z; dec.size = size; dec.base = out->base;
    std::vector<std::pair<u32, std::pair<u32, u32>>> lists(at.begin(), at.end());
    std::sort(lists.begin(), lists.end());
    for (auto& kv : lists) {
        MtBatch b = {};
        b.list = kv.first; b.batch = kv.second.first; b.size = kv.second.second;
        auto m = material.find(b.batch);
        b.material = m != material.end() ? m->second : ~0u;
        b.texture = ~0u;
        if (b.material != ~0u) {
            u32 t = Be32(z + b.material + 0x1C) - out->base;
            if (t + 0x24 <= size) b.texture = t;
        }
        b.first = (u32)out->verts.size();
        if (!dec.List(b.list, b.size, &out->verts, &b.skipped)) out->verts.resize(b.first);   // not a list after all
        b.count = (u32)out->verts.size() - b.first;
        if (b.count) out->batches.push_back(b);
    }
    if (out->batches.empty()) { out->error = "no display list could be read"; return false; }
    // Scene items: an item points at its mesh (+0x10) and its matrix (+0x04: nine floats of rotation and scale, then
    // the position; fn_80171228); the mesh lists its parts {material, batch} (+0x6C count, +0x70) and a scale (+0x54).
    std::unordered_map<u32, size_t> batchIndex;
    for (size_t i = 0; i < out->batches.size(); i++) batchIndex[out->batches[i].batch] = i;
    std::unordered_set<u32> meshes;
    for (u32 e : partAt)
        for (u32 i = 0; i < 64 && e >= 0x70 + 8 * i; i++) {
            u32 m = e - 0x70 - 8 * i, n = Be32(z + m + 0x6C);
            if (n <= i || n > 256 || m + 0x70 + 8 * n > size) continue;
            bool all = true;
            for (u32 k = 0; k < n && all; k++) all = batchIndex.count(Be32(z + m + 0x74 + 8 * k) - out->base) != 0;
            if (all) { meshes.insert(m); break; }
        }
    u32 local = (u32)out->verts.size();
    std::vector<bool> placed(out->batches.size(), false);
    for (u32 q = 0x10; q + 4 <= size; q += 4) {
        u32 m = Be32(z + q) - out->base;
        if (!meshes.count(m)) continue;
        u32 item = q - 0x10, mtx = Be32(z + item + 4) - out->base;
        if (mtx + 48 > size) continue;
        f32 r[12];
        for (int k = 0; k < 12; k++) { u32 v = Be32(z + mtx + 4 * k); memcpy(&r[k], &v, 4); }
        f32 scale;
        { u32 v = Be32(z + m + 0x54); memcpy(&scale, &v, 4); }
        if (!(scale > 0 && scale < 1000)) scale = 1;
        bool sane = true;
        for (f32 x : r) sane = sane && x == x && x > -1e6f && x < 1e6f;
        if (!sane) continue;
        u32 n = Be32(z + m + 0x6C);
        for (u32 k = 0; k < n; k++) {
            size_t bi = batchIndex[Be32(z + m + 0x74 + 8 * k) - out->base];
            const MtBatch& b = out->batches[bi];
            MtDraw dr = {(u32)bi, (u32)out->verts.size(), b.count};
            for (u32 j = b.first; j < b.first + b.count; j++) {
                MtVert v = out->verts[j];
                f32 x = v.p[0] * scale, y = v.p[1] * scale, zz = v.p[2] * scale;
                v.p[0] = x * r[0] + y * r[3] + zz * r[6] + r[9];
                v.p[1] = x * r[1] + y * r[4] + zz * r[7] + r[10];
                v.p[2] = x * r[2] + y * r[5] + zz * r[8] + r[11];
                f32 nx = v.n[0], ny = v.n[1], nz = v.n[2];
                v.n[0] = nx * r[0] + ny * r[3] + nz * r[6];
                v.n[1] = nx * r[1] + ny * r[4] + nz * r[7];
                v.n[2] = nx * r[2] + ny * r[5] + nz * r[8];
                out->verts.push_back(v);
            }
            out->draws.push_back(dr);
            placed[bi] = true;
        }
        out->items++;
    }
    for (size_t i = 0; i < out->batches.size(); i++)          // not placed by an item: as it is
        if (!placed[i]) out->draws.push_back(MtDraw{(u32)i, out->batches[i].first, out->batches[i].count});
    (void)local;
    for (int k = 0; k < 3; k++) { out->lo[k] = 1e30f; out->hi[k] = -1e30f; }
    for (const MtDraw& dr : out->draws)
        for (u32 j = dr.first; j < dr.first + dr.count; j++) {
            const MtVert& v = out->verts[j];
        for (int k = 0; k < 3; k++) { if (v.p[k] < out->lo[k]) out->lo[k] = v.p[k]; if (v.p[k] > out->hi[k]) out->hi[k] = v.p[k]; }
        }
    return true;
}

bool MtBatchTexture(const u8* z, u32 size, const MtZone& zn, const MtBatch& b, std::vector<u32>* rgba, u32* w, u32* h) {
    if (b.texture == ~0u) return false;
    const u8* t = z + b.texture;                                // a texture header (docs/loader.md, SirTexture)
    u32 tw = Be16(t + 8), th = Be16(t + 10), fmt = t[0x14], tl = t[0x15] & 3;
    u32 data = Be32(t + 0x1C) - zn.base, pal = Be32(t + 0x20);
    u32 bytes = PortGxTextureBytes(tw, th, fmt);
    if (!tw || !th || tw > 4096 || th > 4096 || data >= size || bytes > size - data) return false;
    const u8* tlut = NULL;
    std::vector<u8> padded;
    if (fmt == 8 || fmt == 9 || fmt == 10) {
        u32 po = pal - zn.base, pb = fmt == 8 ? 32 : fmt == 9 ? 512 : 0x8000;
        if (!pal || po >= size) return false;
        padded.assign(z + po, z + po + (pb < size - po ? pb : size - po));
        padded.resize(pb, 0);
        tlut = padded.data();
    }
    rgba->assign((size_t)tw * th, 0);
    if (!PortGxDecodeTexture(z + data, tw, th, fmt, tlut, tl, rgba->data())) return false;
    *w = tw; *h = th;
    return true;
}

// ---------------------------------------------------------------------------------------- view
struct MtView {
    ID3D11Device* dev = NULL;
    ID3D11VertexShader* vs = NULL;
    ID3D11PixelShader* ps = NULL;
    ID3D11InputLayout* layout = NULL;
    ID3D11Buffer* cb = NULL;
    ID3D11Buffer* vb = NULL;
    u32 vbCount = 0;
    const void* vbKey = NULL;
    int vbOnly = -2;
    ID3D11RasterizerState* solid = NULL;
    ID3D11RasterizerState* wire = NULL;
    ID3D11DepthStencilState* depth = NULL;
    ID3D11SamplerState* sampler = NULL;
    ID3D11Texture2D* color = NULL;
    ID3D11RenderTargetView* rtv = NULL;
    ID3D11ShaderResourceView* out = NULL;
    ID3D11Texture2D* ztex = NULL;
    ID3D11DepthStencilView* dsv = NULL;
    u32 w = 0, h = 0;
    ID3D11ShaderResourceView* white = NULL;
    std::unordered_map<u32, ID3D11ShaderResourceView*> textures;   // texture offset -> picture (for vbKey)
};

namespace {

const char* kShader = R"(
cbuffer C : register(b0) { row_major float4x4 mvp; float4 eye; float4 flags; };
Texture2D tex : register(t0);
SamplerState smp : register(s0);
struct VI { float3 p : POSITION; float3 n : NORMAL; float2 uv : TEXCOORD; float4 c : COLOR; };
struct VO { float4 p : SV_Position; float3 n : NORMAL; float2 uv : TEXCOORD; float4 c : COLOR; float3 w : POS; };
VO vs(VI i) { VO o; o.p = mul(float4(i.p, 1), mvp); o.n = i.n; o.uv = i.uv; o.c = i.c; o.w = i.p; return o; }
float4 ps(VO i) : SV_Target {
    float3 l = normalize(eye.xyz - i.w);
    float lit = 0.35 + 0.65 * abs(dot(normalize(i.n), l));
    float4 t = flags.x > 0.5 ? tex.Sample(smp, i.uv) : float4(1, 1, 1, 1);
    if (flags.y > 0.5) return float4(0.9, 0.9, 0.95, 1);
    return float4(t.rgb * i.c.rgb * lit, 1);
}
)";

struct Consts { f32 mvp[16]; f32 eye[4]; f32 flags[4]; };

void Release(IUnknown* p) { if (p) p->Release(); }

void Mul(const f32* a, const f32* b, f32* o) {   // row vectors: o = a * b
    for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) {
        f32 s = 0;
        for (int k = 0; k < 4; k++) s += a[r * 4 + k] * b[k * 4 + c];
        o[r * 4 + c] = s;
    }
}

}  // namespace

MtView* MtViewCreate(ID3D11Device* dev) {
    MtView* v = new MtView;
    v->dev = dev;
    ID3DBlob* code = NULL;
    ID3DBlob* err = NULL;
    if (SUCCEEDED(D3DCompile(kShader, strlen(kShader), NULL, NULL, NULL, "vs", "vs_4_0", 0, 0, &code, &err))) {
        dev->CreateVertexShader(code->GetBufferPointer(), code->GetBufferSize(), NULL, &v->vs);
        D3D11_INPUT_ELEMENT_DESC el[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0}};
        dev->CreateInputLayout(el, 4, code->GetBufferPointer(), code->GetBufferSize(), &v->layout);
        code->Release();
    }
    Release(err); err = NULL;
    if (SUCCEEDED(D3DCompile(kShader, strlen(kShader), NULL, NULL, NULL, "ps", "ps_4_0", 0, 0, &code, &err))) {
        dev->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), NULL, &v->ps);
        code->Release();
    }
    Release(err);
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(Consts); bd.Usage = D3D11_USAGE_DYNAMIC; bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    dev->CreateBuffer(&bd, NULL, &v->cb);
    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID; rd.CullMode = D3D11_CULL_NONE; rd.DepthClipEnable = TRUE;
    dev->CreateRasterizerState(&rd, &v->solid);
    rd.FillMode = D3D11_FILL_WIREFRAME;
    dev->CreateRasterizerState(&rd, &v->wire);
    D3D11_DEPTH_STENCIL_DESC dd = {};
    dd.DepthEnable = TRUE; dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; dd.DepthFunc = D3D11_COMPARISON_LESS;
    dev->CreateDepthStencilState(&dd, &v->depth);
    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; sd.AddressU = sd.AddressV = sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP; sd.MaxLOD = D3D11_FLOAT32_MAX;
    dev->CreateSamplerState(&sd, &v->sampler);
    return v;
}

static void DropTarget(MtView* v) {
    Release(v->out); Release(v->rtv); Release(v->color); Release(v->dsv); Release(v->ztex);
    v->out = NULL; v->rtv = NULL; v->color = NULL; v->dsv = NULL; v->ztex = NULL;
    v->w = v->h = 0;
}

static void DropTextures(MtView* v) {
    for (auto& kv : v->textures) Release(kv.second);
    v->textures.clear();
}

void MtViewDestroy(MtView* v) {
    if (!v) return;
    DropTarget(v);
    DropTextures(v);
    Release(v->vs); Release(v->ps); Release(v->layout); Release(v->cb); Release(v->vb);
    Release(v->solid); Release(v->wire); Release(v->depth); Release(v->sampler);
    delete v;
}

static ID3D11ShaderResourceView* Picture(MtView* v, const u8* z, u32 size, const MtZone& zn, const MtBatch& b) {
    auto it = v->textures.find(b.texture);
    if (it != v->textures.end()) return it->second;
    std::vector<u32> rgba;
    u32 w, h;
    ID3D11ShaderResourceView* srv = NULL;
    if (MtBatchTexture(z, size, zn, b, &rgba, &w, &h)) {
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1; td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_IMMUTABLE; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA sd = {rgba.data(), w * 4, 0};
        ID3D11Texture2D* t = NULL;
        if (SUCCEEDED(v->dev->CreateTexture2D(&td, &sd, &t))) { v->dev->CreateShaderResourceView(t, NULL, &srv); t->Release(); }
    }
    v->textures[b.texture] = srv;
    return srv;
}

ID3D11ShaderResourceView* MtViewDraw(MtView* v, ID3D11DeviceContext* ctx, const void* key, const u8* z, u32 size, const MtZone& zn,
                                     int only, u32 w, u32 h, const MtCamera& cam) {
    if (!v || !v->vs || !v->ps || !v->layout || zn.verts.empty() || !w || !h) return NULL;
    if (v->w != w || v->h != h) {
        DropTarget(v);
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = w; td.Height = h; td.MipLevels = 1; td.ArraySize = 1; td.Format = DXGI_FORMAT_R8G8B8A8_UNORM; td.SampleDesc.Count = 1;
        td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(v->dev->CreateTexture2D(&td, NULL, &v->color))) return NULL;
        v->dev->CreateRenderTargetView(v->color, NULL, &v->rtv);
        v->dev->CreateShaderResourceView(v->color, NULL, &v->out);
        td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        if (FAILED(v->dev->CreateTexture2D(&td, NULL, &v->ztex))) return NULL;
        v->dev->CreateDepthStencilView(v->ztex, NULL, &v->dsv);
        v->w = w; v->h = h;
    }
    if (v->vbKey != key) { DropTextures(v); v->vbOnly = -2; }
    if (v->vbKey != key || v->vbOnly != only) {          // the triangles of the zone, once
        Release(v->vb); v->vb = NULL;
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = (UINT)(zn.verts.size() * sizeof(MtVert)); bd.Usage = D3D11_USAGE_IMMUTABLE; bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA sd = {zn.verts.data(), 0, 0};
        if (FAILED(v->dev->CreateBuffer(&bd, &sd, &v->vb))) return NULL;
        v->vbKey = key;
        v->vbOnly = only;
    }
    // Camera: around the middle of the box of what is shown.
    f32 lo[3], hi[3];
    for (int k = 0; k < 3; k++) { lo[k] = 1e30f; hi[k] = -1e30f; }
    // What is drawn: the whole zone as its items place it, or one batch as it is in its list.
    std::vector<MtDraw> what;
    if (only >= 0 && only < (int)zn.batches.size()) what.push_back(MtDraw{(u32)only, zn.batches[only].first, zn.batches[only].count});
    else what = zn.draws;
    for (const MtDraw& b : what) {
        for (u32 j = b.first; j < b.first + b.count; j++)
            for (int k = 0; k < 3; k++) { f32 c = zn.verts[j].p[k]; if (c < lo[k]) lo[k] = c; if (c > hi[k]) hi[k] = c; }
    }
    f32 mid[3], r = 0;
    for (int k = 0; k < 3; k++) { mid[k] = (lo[k] + hi[k]) * 0.5f; r += (hi[k] - lo[k]) * (hi[k] - lo[k]); }
    r = sqrtf(r) * 0.5f;
    if (r < 1e-4f) r = 1;
    f32 d = r * cam.dist;
    f32 eye[3] = {mid[0] + d * cosf(cam.pitch) * sinf(cam.yaw), mid[1] + d * sinf(cam.pitch), mid[2] + d * cosf(cam.pitch) * cosf(cam.yaw)};
    f32 fz[3] = {mid[0] - eye[0], mid[1] - eye[1], mid[2] - eye[2]};
    f32 l = sqrtf(fz[0] * fz[0] + fz[1] * fz[1] + fz[2] * fz[2]);
    for (f32& c : fz) c /= l;
    f32 up[3] = {0, 1, 0};
    f32 xs[3] = {up[1] * fz[2] - up[2] * fz[1], up[2] * fz[0] - up[0] * fz[2], up[0] * fz[1] - up[1] * fz[0]};
    l = sqrtf(xs[0] * xs[0] + xs[1] * xs[1] + xs[2] * xs[2]);
    for (f32& c : xs) c /= l;
    f32 ys[3] = {fz[1] * xs[2] - fz[2] * xs[1], fz[2] * xs[0] - fz[0] * xs[2], fz[0] * xs[1] - fz[1] * xs[0]};
    f32 view[16] = {xs[0], ys[0], fz[0], 0, xs[1], ys[1], fz[1], 0, xs[2], ys[2], fz[2], 0,
                    -(xs[0] * eye[0] + xs[1] * eye[1] + xs[2] * eye[2]), -(ys[0] * eye[0] + ys[1] * eye[1] + ys[2] * eye[2]),
                    -(fz[0] * eye[0] + fz[1] * eye[1] + fz[2] * eye[2]), 1};
    f32 n = d - r * 1.5f > r * 0.01f ? d - r * 1.5f : r * 0.01f, f = d + r * 1.5f, t = 1.0f / tanf(0.4f), a = (f32)w / h;
    f32 proj[16] = {t / a, 0, 0, 0, 0, t, 0, 0, 0, 0, f / (f - n), 1, 0, 0, -n * f / (f - n), 0};
    Consts c = {};
    Mul(view, proj, c.mvp);
    for (int k = 0; k < 3; k++) c.eye[k] = eye[k];
    D3D11_MAPPED_SUBRESOURCE ms;
    f32 clear[4] = {0.11f, 0.12f, 0.14f, 1};
    ctx->ClearRenderTargetView(v->rtv, clear);
    ctx->ClearDepthStencilView(v->dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
    ctx->OMSetRenderTargets(1, &v->rtv, v->dsv);
    D3D11_VIEWPORT vp = {0, 0, (f32)w, (f32)h, 0, 1};
    ctx->RSSetViewports(1, &vp);
    ctx->RSSetState(cam.wire ? v->wire : v->solid);
    ctx->OMSetDepthStencilState(v->depth, 0);
    ctx->OMSetBlendState(NULL, NULL, 0xFFFFFFFFu);
    ctx->IASetInputLayout(v->layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = sizeof(MtVert), off = 0;
    ctx->IASetVertexBuffers(0, 1, &v->vb, &stride, &off);
    ctx->VSSetShader(v->vs, NULL, 0);
    ctx->PSSetShader(v->ps, NULL, 0);
    ctx->VSSetConstantBuffers(0, 1, &v->cb);
    ctx->PSSetConstantBuffers(0, 1, &v->cb);
    ctx->PSSetSamplers(0, 1, &v->sampler);
    for (const MtDraw& b : what) {
        ID3D11ShaderResourceView* pic = cam.textured ? Picture(v, z, size, zn, zn.batches[b.batch]) : NULL;
        c.flags[0] = pic ? 1.0f : 0.0f;
        c.flags[1] = cam.wire ? 1.0f : 0.0f;
        if (SUCCEEDED(ctx->Map(v->cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) { memcpy(ms.pData, &c, sizeof(c)); ctx->Unmap(v->cb, 0); }
        ctx->PSSetShaderResources(0, 1, &pic);
        ctx->Draw(b.count, b.first);
    }
    ID3D11ShaderResourceView* none = NULL;
    ctx->PSSetShaderResources(0, 1, &none);
    ctx->OMSetRenderTargets(0, NULL, NULL);
    return v->out;
}
#endif
