# zstd (decompression only)

zstd 1.6.0, https://github.com/facebook/zstd, BSD-3-Clause (see `LICENSE`).
Source: release tarball `zstd-1.6.0.tar.gz`, sha256 `2ba599ae2a257976278e7e71e998433a387bfd18aa7b78678229fd5d84f5e9b8`.
Unmodified copies of `lib/zstd.h`, `lib/zstd_errors.h`, the decompressor (`lib/decompress/`, without the amd64
assembly) and the common files it needs (`lib/common/`, without the thread pool). Used to read `.rvz` disc images
(`src/port/port_disc.cpp`); built from `src/port/zstd/` with the platform layer, as C++.
