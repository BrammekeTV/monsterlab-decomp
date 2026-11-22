// PC platform layer: zlib zutil.c from third_party/zlib (zlib 1.3.1, see its LICENSE), compiled as C++ with the
// public names prefixed z_ so the wrappers in port_zlib.cpp can define the names the game links.
#ifdef _WIN32
#define Z_PREFIX 1
#define Z_SOLO 1
#include <string.h>
#define HAVE_MEMCPY 1
#include "../../../third_party/zlib/zutil.c"
#endif
#ifdef _WIN32
#include <stdlib.h>
// Z_SOLO leaves the default allocator to the application.
void* zcalloc(void* opaque, unsigned items, unsigned size) { (void)opaque; return calloc(items, size); }
void zcfree(void* opaque, void* ptr) { (void)opaque; free(ptr); }
#endif
