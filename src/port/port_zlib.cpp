// PC platform layer: the zlib the game links (inflateInit_, inflate, inflateEnd). The game's own copy
// is SDK code (0x8022FEC4-0x80232AF8, not decompiled); these call real zlib (third_party/zlib, names
// prefixed z_) on the game's z_stream, which has the same 0x38-byte layout on a 32-bit build.
// A packed .sir section is inflated through here, so this is also where it gets converted:
// port_files.cpp is told when a stream starts on the bytes of a packed read and when it ends.
// No original code. Design: docs/port_runtime.md.
#ifdef _WIN32
#include <string.h>

#include "port/port.h"

extern "C" {
int z_inflateInit_(void* strm, const char* version, int size);
int z_inflate(void* strm, int flush);
int z_inflateEnd(void* strm);
}

void* zcalloc(void* opaque, unsigned items, unsigned size);   // zl_zutil.cpp
void  zcfree(void* opaque, void* ptr);

namespace {
struct ZStreamHead { const u8* nextIn; u32 availIn, totalIn; u8* nextOut; u32 availOut, totalOut; };   // z_stream, first six fields
}

extern "C" {

s32 inflateInit_(void* strm, const char* version, s32 size) {
    PortFileInflateDrop(strm);
    // The game leaves zalloc/zfree/opaque at 0 (zlib's default allocator); built with Z_SOLO the caller supplies it.
    u32* w = (u32*)strm;
    if (w[8] == 0) { w[8] = (u32)(uintptr_t)zcalloc; w[9] = (u32)(uintptr_t)zcfree; w[10] = 0; }
    return z_inflateInit_(strm, version, size);
}

s32 inflate(void* strm, s32 flush) {
    ZStreamHead* z = (ZStreamHead*)strm;
    if (z->totalIn == 0) PortFileInflateBegin(strm, z->nextIn, z->nextOut);
    s32 r = z_inflate(strm, flush);
    if (r == 1) PortFileInflateEnd(strm, z->totalOut);        // Z_STREAM_END
    return r;
}

s32 inflateEnd(void* strm) {
    PortFileInflateDrop(strm);
    return z_inflateEnd(strm);
}

}  // extern "C"

// A zlib stream split across calls, and one given all its input at once, must both come out right.
int PortSelfTestZlib(void) {
    static const u8 packed[38] = {0x78,0xDA,0xF3,0xCD,0xCF,0x2B,0x2E,0x49,0x2D,0x52,0xF0,0x49,0x4C,0x52,0xA8,0xCA,0xC9,0x4C,0x52,0x28,0x4E,0xCD,0x49,0xD3,
                                  0x2D,0x49,0x2D,0x2E,0x51,0xF0,0x1D,0xCA,0x52,0x00,0x20,0x43,0x4D,0xF9};
    static const char text[] = "Monster Lab zlib self-test ";
    int fails = 0;
    for (int chunk = 1; chunk <= 38; chunk += 37) {
        u32 zs[14] = {0};                                    // a z_stream
        ZStreamHead* z = (ZStreamHead*)zs;
        static u8 out[300];
        memset(out, 0, sizeof(out));
        if (inflateInit_(zs, "1.3.1", 0x38) != 0) { PortLog("selftest zlib: inflateInit_ failed"); return 1; }
        z->nextOut = out; z->availOut = sizeof(out);
        u32 pos = 0;
        s32 r = 0;
        while (r == 0 && pos < 38) {
            u32 n = 38 - pos < (u32)chunk ? 38 - pos : (u32)chunk;
            z->nextIn = packed + pos; z->availIn = n; pos += n;
            r = inflate(zs, 0);
        }
        bool ok = r == 1 && z->totalOut == 216;
        for (u32 i = 0; ok && i < 216; i++) ok = out[i] == (u8)text[i % 27];
        if (!ok) { PortLog("selftest zlib: %d-byte chunks: result %d, %u bytes out", chunk, r, z->totalOut); fails++; }
        inflateEnd(zs);
    }
    return fails;
}
#endif
