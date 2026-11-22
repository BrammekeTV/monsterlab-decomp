// PC platform layer: zlib adler32.c from third_party/zlib (zlib 1.3.1, see its LICENSE), compiled as C++ with the
// public names prefixed z_ so the wrappers in port_zlib.cpp can define the names the game links.
#ifdef _WIN32
#define Z_PREFIX 1
#define Z_SOLO 1
#include <string.h>
#define HAVE_MEMCPY 1
#include "../../../third_party/zlib/adler32.c"
#endif
