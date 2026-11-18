#!/bin/zsh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="${KFX_MAC_DEPS_ROOT:-$ROOT/build-macos-deps}"
TARGET="${MACOSX_DEPLOYMENT_TARGET:-13.0}"
JOBS="${KFX_JOBS:-$(sysctl -n hw.logicalcpu)}"
mkdir -p "$WORK"
fetch() {
  local url="$1" name="$2" sha="$3"
  [[ -f "$WORK/$name" ]] || curl -fL --retry 3 "$url" -o "$WORK/$name"
  echo "$sha  $WORK/$name" | shasum -a 256 -c -
}
fetch https://curl.se/download/curl-8.23.0.tar.xz curl-8.23.0.tar.xz 8dadd7d2536a314f982ad05a651b713f426bf7d02cc2c76cffb9796f2218e6c5
fetch https://github.com/openssl/openssl/releases/download/openssl-3.5.9/openssl-3.5.9.tar.gz openssl-3.5.9.tar.gz 603f5602e2eef00d77fbd429d34dcd5822bb301757a1bc9cdb24c670f1eb859a
[[ -d "$WORK/curl-8.23.0" ]] || tar -xf "$WORK/curl-8.23.0.tar.xz" -C "$WORK"
[[ -d "$WORK/openssl-3.5.9" ]] || tar -xf "$WORK/openssl-3.5.9.tar.gz" -C "$WORK"
for arch in arm64 x86_64; do
  b="$WORK/openssl-$arch-build"
  p="$WORK/openssl-$arch-prefix"
  mkdir -p "$b"
  platform=darwin64-arm64-cc
  [[ "$arch" == x86_64 ]] && platform=darwin64-x86_64-cc
  (cd "$b"
   "$WORK/openssl-3.5.9/Configure" "$platform" no-shared no-tests no-module --prefix="$p" --libdir=lib -mmacosx-version-min="$TARGET"
   make -j "$JOBS" build_libs
   make install_dev)
done
mkdir -p "$WORK/openssl-fat/lib"
cp -R "$WORK/openssl-arm64-prefix/include" "$WORK/openssl-fat/"
# OpenSSL generates architecture-specific configuration headers.
mv "$WORK/openssl-fat/include/openssl/configuration.h" "$WORK/openssl-fat/include/openssl/configuration-arm64.h"
cp "$WORK/openssl-x86_64-prefix/include/openssl/configuration.h" "$WORK/openssl-fat/include/openssl/configuration-x86_64.h"
cat > "$WORK/openssl-fat/include/openssl/configuration.h" <<'HEAD'
#if defined(__arm64__)
#include "configuration-arm64.h"
#else
#include "configuration-x86_64.h"
#endif
HEAD
for lib in ssl crypto; do
  lipo -create "$WORK/openssl-arm64-prefix/lib/lib$lib.a" "$WORK/openssl-x86_64-prefix/lib/lib$lib.a" -output "$WORK/openssl-fat/lib/lib$lib.a"
done
cmake -S "$WORK/curl-8.23.0" -B "$WORK/curl-build" -G Ninja \
  -DCMAKE_IGNORE_PREFIX_PATH="/opt/homebrew;/usr/local" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$WORK/curl-prefix" \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET="$TARGET" \
  -DBUILD_SHARED_LIBS=OFF -DBUILD_CURL_EXE=OFF -DBUILD_TESTING=OFF \
  -DBUILD_LIBCURL_DOCS=OFF -DBUILD_MISC_DOCS=OFF -DCURL_USE_OPENSSL=ON \
  -DOPENSSL_INCLUDE_DIR="$WORK/openssl-fat/include" -DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=ON \
  -DCURL_DISABLE_LDAP=ON -DCURL_DISABLE_LDAPS=ON \
  -DOPENSSL_ROOT_DIR="$WORK/openssl-fat" -DOPENSSL_USE_STATIC_LIBS=ON \
  -DUSE_APPLE_SECTRUST=ON -DCURL_CA_NATIVE=ON -DCURL_DISABLE_WEBSOCKETS=OFF \
  -DCURL_USE_LIBPSL=OFF -DCURL_USE_LIBSSH2=OFF -DUSE_LIBIDN2=OFF \
  -DCURL_BROTLI=OFF -DCURL_ZSTD=OFF -DUSE_NGHTTP2=OFF
cmake --build "$WORK/curl-build" -j "$JOBS"
cmake --install "$WORK/curl-build"
lipo -info "$WORK/curl-prefix/lib/libcurl.a"
