// Test tool for the Bink decoder (src/port/port_bink_codec.cpp). Needs a .bik file (game data, not committed).
//   bink_test <file.bik> <audioTrack> <outDir> <frame> [<frame>...]
// Writes <outDir>/frame_<n>.yuv (planar 4:2:0) for each listed frame and <outDir>/audio.f32 (interleaved
// float samples of the first 10 s of the track). tools/bink_check.py compares them with FFmpeg's output.
#include "port/port_bink_codec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <set>

static std::vector<uint8_t> ReadAll(const char* path) {
    std::vector<uint8_t> v;
    FILE* f = fopen(path, "rb");
    if (!f) return v;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    v.resize(n);
    fread(v.data(), 1, n, f);
    fclose(f);
    return v;
}

// "all" mode: decode every frame and every audio track; report errors and whether each track's sound length
// matches the picture's.
static int DecodeAll(const char* path) {
    std::vector<uint8_t> file = ReadAll(path);
    bink::Header h;
    if (!bink::ParseHeader(file.data(), file.size(), h)) { printf("%s: bad header\n", path); return 1; }
    bink::VideoDecoder vd;
    if (!vd.Open(h.revision, h.videoFlags, h.width, h.height)) { printf("%s: video init failed\n", path); return 1; }
    std::vector<bink::AudioDecoder> ad(h.tracks.size());
    std::vector<double> samples(h.tracks.size(), 0);
    for (size_t t = 0; t < h.tracks.size(); t++)
        if (!ad[t].Open(h.tracks[t].sampleRate, h.tracks[t].channels, h.tracks[t].dct, h.revision)) { printf("%s: audio init failed\n", path); return 1; }
    for (uint32_t n = 0; n < h.frames; n++) {
        uint32_t off = h.frameOffset[n], size = h.frameOffset[n + 1] - off;
        bink::FrameSplit fs;
        if (off + size > file.size() || !bink::SplitFrame(h, file.data() + off, size, fs)) { printf("%s: frame %u: bad split\n", path, n); return 1; }
        for (size_t t = 0; t < h.tracks.size(); t++) {
            if (fs.audio[t].second < 4) continue;
            std::vector<float> pcm;
            if (!ad[t].DecodeAudio(fs.audio[t].first, fs.audio[t].second, pcm)) { printf("%s: frame %u track %zu: audio error\n", path, n, t); return 1; }
            samples[t] += (double)pcm.size() / h.tracks[t].channels;
        }
        if (!vd.Decode(fs.video, fs.videoSize)) { printf("%s: frame %u: video error\n", path, n); return 1; }
    }
    double secs = (double)h.frames * h.fpsDen / h.fpsNum, worst = 0;
    for (size_t t = 0; t < h.tracks.size(); t++) {
        double d = samples[t] / h.tracks[t].sampleRate - secs;
        if (d < 0) d = -d;
        if (d > worst) worst = d;
    }
    printf("%s: ok, %u frames (%.1f s), %zu tracks, worst sound/picture length difference %.2f s\n", path, h.frames, secs, h.tracks.size(), worst);
    return worst > 1.0 ? 1 : 0;
}

int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "all") return DecodeAll(argv[2]);
    if (argc < 5) { printf("usage: bink_test file.bik track outDir frame...\n"); return 2; }
    std::vector<uint8_t> file = ReadAll(argv[1]);
    int track = atoi(argv[2]);
    std::string out = argv[3];
    std::set<int> want;
    for (int i = 4; i < argc; i++) want.insert(atoi(argv[i]));
    bink::Header h;
    if (!bink::ParseHeader(file.data(), file.size(), h)) { printf("bad header\n"); return 1; }
    printf("rev %c %ux%u frames %u fps %u/%u tracks %zu\n", h.revision, h.width, h.height, h.frames, h.fpsNum,
           h.fpsDen, h.tracks.size());
    bink::VideoDecoder vd;
    if (!vd.Open(h.revision, h.videoFlags, h.width, h.height)) { printf("video init failed\n"); return 1; }
    bink::AudioDecoder ad;
    bool haveAudio = track >= 0 && track < (int)h.tracks.size();
    if (haveAudio && !ad.Open(h.tracks[track].sampleRate, h.tracks[track].channels, h.tracks[track].dct, h.revision)) {
        printf("audio init failed\n");
        return 1;
    }
    std::vector<float> audio;
    int last = *want.rbegin();
    double audioLimit = 10.0 * h.tracks[track].sampleRate * h.tracks[track].channels;
    for (int n = 0; n < (int)h.frames; n++) {
        uint32_t off = h.frameOffset[n], size = h.frameOffset[n + 1] - off;
        bink::FrameSplit fs;
        if (off + size > file.size() || !bink::SplitFrame(h, file.data() + off, size, fs)) { printf("frame %d: bad split\n", n); return 1; }
        if (haveAudio && audio.size() < audioLimit && fs.audio[track].second >= 4)
            if (!ad.DecodeAudio(fs.audio[track].first, fs.audio[track].second, audio)) { printf("frame %d: audio error\n", n); return 1; }
        if (n > last && audio.size() >= audioLimit) break;
        if (n <= last) {
            if (!vd.Decode(fs.video, fs.videoSize)) { printf("frame %d: video error\n", n); return 1; }
            if (want.count(n)) {
                bink::Image im = vd.GetImage();
                std::string p = out + "/frame_" + std::to_string(n) + ".yuv";
                FILE* f = fopen(p.c_str(), "wb");
                for (int pl = 0; pl < 3; pl++) {
                    int w = pl ? (im.width + 1) / 2 : im.width, hh = pl ? (im.height + 1) / 2 : im.height;
                    for (int y = 0; y < hh; y++) fwrite(im.plane[pl] + (size_t)y * im.stride[pl], 1, w, f);
                }
                fclose(f);
            }
        }
    }
    if (haveAudio) {
        std::string p = out + "/audio.f32";
        FILE* f = fopen(p.c_str(), "wb");
        fwrite(audio.data(), 4, audio.size(), f);
        fclose(f);
    }
    printf("ok\n");
    return 0;
}
