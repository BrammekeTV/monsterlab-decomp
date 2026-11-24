// third_party/zstd (BSD-3, see its LICENSE), decompression only, built as C++ with the platform layer.
#ifdef _WIN32
#define ZSTD_DISABLE_ASM 1
#include "../../../third_party/zstd/lib/common/fse_decompress.c"
#endif
