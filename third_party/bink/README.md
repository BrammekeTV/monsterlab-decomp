# Bink 1 decoder (derived from FFmpeg)

`binkdata.h` is copied unchanged from FFmpeg 7.1 (`libavcodec/binkdata.h`). The decoder in
`src/port/port_bink_codec.cpp` is a standalone port of FFmpeg's `libavcodec/bink.c`, `binkdsp.c`,
`binkaudio.c` and `libavformat/bink.c` (Konstantin Shishkov, Peter Ross and others), so it is also
licensed under the **GNU LGPL 2.1 or later** (`COPYING.LGPLv2.1`). The rest of the repository is
unaffected: the sources of the whole port are public, so anyone can rebuild the exe with a modified
decoder, which is what the LGPL asks for statically linked code.

Source: https://ffmpeg.org/releases/ffmpeg-7.1.tar.xz . No RAD Game Tools code is used and no game
data is included.
