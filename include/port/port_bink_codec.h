// Bink 1 (revisions f..k) video and audio decoder for the PC port. Derived from FFmpeg 7.1 (LGPL 2.1+),
// see third_party/bink/README.md. Pure computation: no file or game code in here.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>

namespace bink {

// File header + frame index of a .bik file (the first 'headerBytes' bytes of the file are enough).
struct AudioTrackInfo {
    int sampleRate;
    int channels;     // 1 or 2
    bool dct;         // DCT (true) or RDFT (false) variant
    uint32_t id;      // track id (language) from the header
};

struct Header {
    uint8_t  revision;       // 'i' for Monster Lab
    uint32_t fileSize;
    uint32_t frames;
    uint32_t maxFrameSize;
    uint32_t width, height;
    uint32_t fpsNum, fpsDen;
    uint32_t videoFlags;
    std::vector<AudioTrackInfo> tracks;
    std::vector<uint32_t> frameOffset;  // frames + 1 entries (last = fileSize), key flag removed
    std::vector<uint8_t>  frameKey;
    size_t   headerBytes;               // bytes needed from the file start to parse everything
};

// Returns the number of bytes the header needs (0 if 'n' bytes are too few to know, -1 invalid).
// Call with the first 64 bytes, then with 'headerBytes' bytes via ParseHeader.
int HeaderSize(const uint8_t* data, size_t n);
bool ParseHeader(const uint8_t* data, size_t n, Header& h);

// One frame split into its audio packets and the video packet.
struct FrameSplit {
    std::vector<std::pair<const uint8_t*, uint32_t>> audio;  // per track (size may be < 4: no data)
    const uint8_t* video = nullptr;
    uint32_t videoSize = 0;
};
bool SplitFrame(const Header& h, const uint8_t* frame, uint32_t size, FrameSplit& out);

// Planar YUV 4:2:0 image owned by the decoder; valid until the next Decode.
struct Image {
    const uint8_t* plane[3];
    int stride[3];
    int width, height;     // luma size; chroma is (width+1)/2 x (height+1)/2
};

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();
    bool Open(uint8_t revision, uint32_t videoFlags, int width, int height);
    // Decodes one video packet. Returns false on a corrupt packet.
    bool Decode(const uint8_t* pkt, size_t size);
    Image GetImage() const;
    void Reset() { frameNum = 0; }
private:
    struct Impl;
    Impl* p;
    unsigned frameNum = 0;
};

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();
    bool Open(int sampleRate, int channels, bool dct, uint8_t revision);
    // Decodes one audio packet (including its 4-byte size field). Appends interleaved float samples
    // (-1..1) to 'out'. Returns false on corrupt data.
    bool DecodeAudio(const uint8_t* pkt, size_t size, std::vector<float>& out);
    void Reset();
private:
    struct Impl;
    Impl* p;
};

}  // namespace bink
