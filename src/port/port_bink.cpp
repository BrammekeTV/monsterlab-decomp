// PC replacement for RAD's Bink library on the Wii (0x802B3020-0x802BE300): the entry points the game's
// movie player (MoviePlay) calls, on top of the decoder in port_bink_codec.cpp. No original code.
//
// The names are the Bink API's; the game calls them by the address names (fn_802B....). What each one is
// comes from how MoviePlay uses it and from the asm of BinkGetFrameBuffersInfo / BinkRegisterFrameBuffers /
// BinkClose; the notes are in docs/sdk_libraries.md.
//
// Frames reach the screen the way the game draws them on the console: MoviePlay registers GX texture
// memory for the planes (BinkRegisterFrameBuffers) and draws them through TEV (fn_80205894), which turns
// Y, Cr and Cb into RGB. BinkDoFrame decodes into those planes as plain rows (pitch = width).
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "port/port.h"
#include "port/port_bink_codec.h"
#include "port/port_mods.h"
#include "revolution.h"

extern "C" {
s32  DVDOpen(char* path, u8* info);
void DVDClose(u8* info);
s32  DVDReadAsync(u8* info, void* addr, s32 len, s32 offset, void* cb, s32 prio);
void OSSleepTicks(s32 hi, u32 lo);
void* fn_80330D80(s32 size);   // MSL malloc (port_os.cpp)
void  fn_80330E20(void* p);    // MSL free
}

namespace {

// The game reads the first 0x20 bytes of a Bink handle: size, frame counts, frame rate, read error.
struct BinkHandle {
    u32 width, height;
    u32 frames;
    u32 frameNum;          // 1-based: the frame BinkDoFrame decodes next / the one on show
    u32 lastFrameNum;
    u32 rate, rateDiv;     // frames per second = rate / rateDiv
    u32 readError;
    // ---- private from here on ----
    u8   fileInfo[0x40];
    bink::Header hdr;
    bink::VideoDecoder video;
    bink::AudioDecoder audio;
    bool haveAudio = false;
    int  audioRate = 44100;
    u8*  fb = nullptr;     // the game's BINKFRAMEBUFFERS (BinkRegisterFrameBuffers)
    s64  startTicks = 0;
    bool started = false;
    s32  decoded = -1;     // index (0-based) of the last frame decoded
    std::vector<u8> buf;
};

BinkHandle* gPlaying = nullptr;   // the movie on show (F12 capture)
s32 gTrackId = 0;          // BinkSetSoundTrack: the track (language) the next BinkOpen plays

bool ReadAt(BinkHandle* h, u32 off, void* dst, u32 len) {
    return DVDReadAsync(h->fileInfo, dst, (s32)len, (s32)off, NULL, 2) != 0;
}

// Plane -> the game's buffer: plain rows of 'pitch' bytes. MoviePlay's TEV set-up (indirect lookup textures
// built by fn_80205208) reads these buffers through GX I8 textures and undoes the 8x4 tiling itself.
void CopyPlane(u8* dst, int pitch, const u8* src, int stride, int w, int h) {
    for (int y = 0; y < h; y++) memcpy(dst + (size_t)y * pitch, src + (size_t)y * stride, (size_t)w);
}

s64 FrameTicks(const BinkHandle* h) {
    return (s64)((double)OS_TIMER_CLOCK * h->rateDiv / h->rate);
}

// Decodes frame index n (0-based) into the decoders; returns false on a read or decode error.
bool DecodeFrame(BinkHandle* h, u32 n, bool wantAudio) {
    u32 off = h->hdr.frameOffset[n], size = h->hdr.frameOffset[n + 1] - off;
    if (size < 4 || size > (64u << 20)) return false;
    h->buf.resize(size);
    if (!ReadAt(h, off, h->buf.data(), size)) return false;
    bink::FrameSplit fs;
    if (!bink::SplitFrame(h->hdr, h->buf.data(), size, fs)) return false;
    if (h->haveAudio && wantAudio) {
        auto& a = fs.audio[gTrackId < (s32)fs.audio.size() ? gTrackId : 0];
        if (a.second >= 4) {
            std::vector<float> pcm;
            if (h->audio.DecodeAudio(a.first, a.second, pcm) && !pcm.empty()) {
                const auto& ti = h->hdr.tracks[gTrackId < (s32)fs.audio.size() ? gTrackId : 0];
                if (ti.channels == 2) {
                    PortAudioMoviePush(pcm.data(), (u32)(pcm.size() / 2), ti.sampleRate);
                } else {
                    std::vector<float> st(pcm.size() * 2);
                    for (size_t i = 0; i < pcm.size(); i++) st[2 * i] = st[2 * i + 1] = pcm[i];
                    PortAudioMoviePush(st.data(), (u32)pcm.size(), ti.sampleRate);
                }
            }
        }
    }
    return h->video.Decode(fs.video, fs.videoSize);
}

}  // namespace

// F12 capture: the last decoded frame of the movie on show as a PNG, from the decoder's planes (before any
// upload or draw). BT.601, studio range: for comparing positions of seams and blocks, not exact colours.
BOOL PortBinkSaveFrame(const char* path) {
    BinkHandle* h = gPlaying;
    if (!h || h->decoded < 0) return FALSE;
    bink::Image im = h->video.GetImage();
    u32 w = h->width, hgt = h->height;
    if (!im.plane[0] || !im.plane[1] || !im.plane[2]) return FALSE;
    std::vector<u32> rgba((size_t)w * hgt);
    for (u32 y = 0; y < hgt; y++)
        for (u32 x = 0; x < w; x++) {
            int Y = im.plane[0][(size_t)y * im.stride[0] + x];
            int cb = im.plane[1][(size_t)(y / 2) * im.stride[1] + x / 2] - 128;
            int cr = im.plane[2][(size_t)(y / 2) * im.stride[2] + x / 2] - 128;
            int c = (Y - 16) * 298;
            int r = (c + 409 * cr + 128) >> 8, g = (c - 100 * cb - 208 * cr + 128) >> 8, b = (c + 516 * cb + 128) >> 8;
            r = r < 0 ? 0 : r > 255 ? 255 : r; g = g < 0 ? 0 : g > 255 ? 255 : g; b = b < 0 ? 0 : b > 255 ? 255 : b;
            rgba[(size_t)y * w + x] = (u32)r | (u32)g << 8 | (u32)b << 16 | 0xFF000000u;
        }
    return PortPngWrite(path, rgba.data(), w, hgt);
}

extern "C" {

// fn_802B47A0: BinkSetSoundTrack(total, tracks): the next BinkOpen plays this audio track.
s32 fn_802B47A0(s32 total, s32* tracks) {
    gTrackId = (total > 0 && tracks) ? tracks[0] : 0;
    return 1;
}

// fn_802B4B30: BinkOpen(path, flags). NULL when the movie cannot be played.
u8* fn_802B4B30(const char* path, u32 flags) {
    if (gPortOptions.noMovies) return NULL;
    BinkHandle* h = new BinkHandle();
    memset(h->fileInfo, 0, sizeof(h->fileInfo));
    u8 first[64 + 32];
    if (!DVDOpen((char*)path, h->fileInfo)) { delete h; return NULL; }
    bool ok = ReadAt(h, 0, first, 64);
    int need = ok ? bink::HeaderSize(first, 64) : -1;
    std::vector<u8> head;
    if (need > 0) { head.resize((size_t)need + 32); ok = ReadAt(h, 0, head.data(), (u32)need); }
    if (!ok || need <= 0 || !bink::ParseHeader(head.data(), (size_t)need, h->hdr) ||
        !h->video.Open(h->hdr.revision, h->hdr.videoFlags, (int)h->hdr.width, (int)h->hdr.height)) {
        PortLog("Bink: %s: not a playable Bink file", path);
        DVDClose(h->fileInfo);
        delete h;
        return NULL;
    }
    if (gTrackId >= 0 && gTrackId < (s32)h->hdr.tracks.size()) {
        const auto& t = h->hdr.tracks[gTrackId];
        h->haveAudio = h->audio.Open(t.sampleRate, t.channels, t.dct, h->hdr.revision);
    }
    h->width = h->hdr.width; h->height = h->hdr.height; h->frames = h->hdr.frames;
    h->frameNum = 1; h->lastFrameNum = 0;
    h->rate = h->hdr.fpsNum; h->rateDiv = h->hdr.fpsDen;
    h->readError = 0;
    PortAudioMovieStop();
    PortLog("Bink: %s: %ux%u, %u frames, %.2f fps, audio track %d%s", path, h->width, h->height, h->frames,
            (double)h->rate / h->rateDiv, gTrackId, h->haveAudio ? "" : " (none)");
    gPlaying = h;
    return (u8*)h;
}

// fn_802B6C60: BinkSetVolume(bink, track, volume 0..32768).
s32 fn_802B6C60(u8* bink, s32 track, s32 volume) {
    PortAudioMovieVolume((float)volume / 32768.0f);
    return 1;
}

// fn_802B4A1C: BinkGetFrameBuffersInfo(bink, fb): what the game has to allocate (asm of the original).
s32 fn_802B4A1C(u8* bink, u8* p) {
    BinkHandle* h = (BinkHandle*)bink;
    if (!h || !p) return 0;
    u32* w = (u32*)p;
    memset(p, 0, 0x78);
    w[0x14 / 4] = 0;                                  // FrameNum
    w[4 / 4] = (h->width + 7) & ~7u;                  // YABufferWidth
    w[8 / 4] = (h->height + 7) & ~7u;                 // YABufferHeight
    w[0xC / 4] = (((h->width + 1) >> 1) + 7) & ~7u;   // cRcBBufferWidth
    w[0x10 / 4] = (((h->height + 1) >> 1) + 7) & ~7u; // cRcBBufferHeight
    w[0x18 / 4] = 1;                                  // frame set 0: Y, Cr, Cb allocated, no alpha
    w[0x24 / 4] = 1;
    w[0x30 / 4] = 1;
    w[0x48 / 4] = 1;                                  // frame set 1
    w[0x54 / 4] = 1;
    w[0x60 / 4] = 1;
    w[0] = 2;                                         // TotalFrames: two sets
    return 1;
}

// fn_802B4B14: BinkRegisterFrameBuffers(bink, fb).
s32 fn_802B4B14(u8* bink, u8* p) {
    BinkHandle* h = (BinkHandle*)bink;
    if (h && p) h->fb = p;
    return 1;
}

// fn_802B6574: BinkGoto(bink, frame (1-based), flags): decoding resumes at that frame.
s32 fn_802B6574(u8* bink, s32 frame, s32 flags) {
    BinkHandle* h = (BinkHandle*)bink;
    if (!h) return 0;
    if (frame < 1) frame = 1;
    if ((u32)frame > h->frames) frame = (s32)h->frames;
    h->frameNum = (u32)frame;
    h->started = false;
    return 1;
}

// fn_802B5C6C: BinkDoFrame(bink): decodes the current frame into the registered planes.
s32 fn_802B5C6C(u8* bink) {
    BinkHandle* h = (BinkHandle*)bink;
    if (!h || h->readError) return 0;
    u32 n = h->frameNum - 1;
    if (!h->started) {
        // First frame, or a restart after an error: decode from the key frame at or before it.
        u32 k = n;
        while (k > 0 && !h->hdr.frameKey[k]) k--;
        h->video.Reset();
        for (u32 i = k; i < n; i++)
            if (!DecodeFrame(h, i, false)) { h->readError = 1; return 0; }
        h->startTicks = PortTicks() - (s64)n * FrameTicks(h);
        h->started = true;
        h->decoded = (s32)n - 1;
    }
    if (h->decoded != (s32)n) {
        if (!DecodeFrame(h, n, true)) { h->readError = 1; return 0; }
        h->decoded = (s32)n;
    }
    h->lastFrameNum = h->frameNum;
    if (h->fb) {
        u32* fb = (u32*)h->fb;
        u32 idx = (fb[0x14 / 4] + 1) % (fb[0] ? fb[0] : 1);   // the set after the one on show
        u32* set = fb + (0x18 + idx * 0x30) / 4;
        bink::Image im = h->video.GetImage();
        // set[0..2] Y {allocate, buffer, pitch}, set[3..5] Cr, set[6..8] Cb
        if (set[0] && set[1]) CopyPlane((u8*)(uintptr_t)set[1], (int)set[2], im.plane[0], im.stride[0], fb[1], fb[2]);
        if (set[3] && set[4]) CopyPlane((u8*)(uintptr_t)set[4], (int)set[5], im.plane[2], im.stride[2], fb[3], fb[4]);
        if (set[6] && set[7]) CopyPlane((u8*)(uintptr_t)set[7], (int)set[8], im.plane[1], im.stride[1], fb[3], fb[4]);
        fb[0x14 / 4] = idx;
    }
    return 1;
}

// fn_802B61D0: BinkShouldSkip(bink): the movie is never behind on PC, no frame is dropped.
s32 fn_802B61D0(u8* bink) { return 0; }

// fn_802B625C: BinkNextFrame(bink).
s32 fn_802B625C(u8* bink) {
    BinkHandle* h = (BinkHandle*)bink;
    if (!h) return 0;
    if (h->frameNum < h->frames) h->frameNum++;
    return 1;
}

// fn_802B6950: BinkWait(bink): non-zero while the next frame is not due yet.
s32 fn_802B6950(u8* bink) {
    BinkHandle* h = (BinkHandle*)bink;
    if (!h || h->readError || !h->started) return 0;
    s64 due = h->startTicks + (s64)h->frameNum * FrameTicks(h);
    if (PortTicks() >= due) return 0;
    OSSleepTicks(0, (u32)(OS_TIMER_CLOCK / 1000));   // 1 ms; in fast mode the clock jumps ahead
    return PortTicks() < due ? 1 : 0;
}

// fn_802B6798: BinkClose(bink).
s32 fn_802B6798(u8* bink) {
    BinkHandle* h = (BinkHandle*)bink;
    if (!h) return 0;
    if (gPlaying == h) gPlaying = nullptr;
    PortAudioMovieStop();
    DVDClose(h->fileInfo);
    delete h;
    return 1;
}

// fn_802BE160 / fn_802BE220: the library's allocator (size, ?) and its free. They take the game's heap,
// as the original does through MSL malloc: the plane textures must lie in the Wii address space (GX
// reads them by address).
void* fn_802BE160(s32 size, s32 unused) {
    if (size <= 0) return NULL;
    void* p = fn_80330D80(size + 0x40);
    if (p) memset(p, 0, (size_t)size + 0x40);
    return p;
}
void fn_802BE220(void* p) { if (p) fn_80330E20(p); }

}  // extern "C"
BOOL PortBinkPlaying(void) { return gPlaying != nullptr; }

#endif
