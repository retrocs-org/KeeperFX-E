#!/bin/zsh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORK="${KFX_MAC_DEPS_ROOT:-$ROOT/build-macos-deps}"
TARGET="${MACOSX_DEPLOYMENT_TARGET:-13.0}"
SDK="$(xcrun --sdk macosx --show-sdk-path)"
JOBS="${KFX_JOBS:-$(sysctl -n hw.logicalcpu)}"
mkdir -p "$WORK"

need() { command -v "$1" >/dev/null || { echo "Missing tool: $1" >&2; exit 1; }; }
for t in cmake ninja git curl tar shasum clang clang++ ar ranlib libtool lipo xcrun; do need "$t"; done

fetch_tar() {
  local url="$1" archive="$2" sha="$3"
  if [[ ! -f "$WORK/$archive" ]]; then curl -L --fail --retry 3 "$url" -o "$WORK/$archive"; fi
  echo "$sha  $WORK/$archive" | shasum -a 256 -c -
}

clone_at() {
  local name="$1" url="$2" rev="$3"; shift 3
  if [[ ! -d "$WORK/$name/.git" ]]; then git clone "$@" "$url" "$WORK/$name"; fi
  git -C "$WORK/$name" fetch origin "$rev" --depth 1 --quiet || git -C "$WORK/$name" fetch --tags --quiet
  git -C "$WORK/$name" checkout --detach "$rev"
  git -C "$WORK/$name" submodule update --init --recursive
}

cmake_fat() {
  cmake "$@" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$TARGET"
}

# Exact source revisions used by the validated 15 Sep 2026 release build.
clone_at SDL-3.4.16 https://github.com/libsdl-org/SDL.git fa2c02bb6e21974a89ea9824bc53c9932abe5f9c --depth 1
clone_at SDL_image-3.4.6 https://github.com/libsdl-org/SDL_image.git f661fa1ad24ab1b81e43662532f9a6a9fcf67ea6 --depth 1
clone_at SDL_mixer-3.2.4 https://github.com/libsdl-org/SDL_mixer.git 72a81869b45e249e8e67102db4e98dd2441f05a1 --depth 1
clone_at openal-soft-src https://github.com/kcat/openal-soft.git b2c48f7718ef3fcf67921a8b6534c4914e328970 --depth 1
clone_at libnatpmp-git https://github.com/miniupnp/libnatpmp.git 134fc89e2781e154e40042641f4d8bcbe42579f1 --depth 1

fetch_tar https://ffmpeg.org/releases/ffmpeg-9.0.1.tar.xz ffmpeg-9.0.1.tar.xz cf38e0e28c7e5605942c4a77755349b0145804a397af37eb1fb4c77cb237f635
fetch_tar https://www.lua.org/ftp/lua-5.4.8.tar.gz lua-5.4.8.tar.gz 4f18ddae154e793e46eeab727c59ef1c0c0c2b744e7b94219710d76f530629ae
fetch_tar https://github.com/randy408/libspng/archive/refs/tags/v0.7.4.tar.gz libspng-0.7.4.tar.gz 47ec02be6c0a6323044600a9221b049f63e1953faf816903e7383d4dc4234487
fetch_tar https://miniupnp.tuxfamily.org/files/miniupnpc-2.3.3.tar.gz miniupnpc-2.3.3.tar.gz d52a0afa614ad6c088cc9ddff1ae7d29c8c595ac5fdd321170a05f41e634bd1a
fetch_tar https://zlib.net/fossils/zlib-1.3.2.tar.gz zlib-1.3.2.tar.gz bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16

for pair in 'ffmpeg-9.0.1.tar.xz:ffmpeg-9.0.1' 'lua-5.4.8.tar.gz:lua-5.4.8' 'libspng-0.7.4.tar.gz:libspng-0.7.4' 'miniupnpc-2.3.3.tar.gz:miniupnpc-2.3.3' 'zlib-1.3.2.tar.gz:zlib-1.3.2'; do
  a=${pair%%:*}; d=${pair##*:}; [[ -d "$WORK/$d" ]] || tar -xf "$WORK/$a" -C "$WORK"
done

# SDL3
rm -rf "$WORK/sdl-build" "$WORK/sdl-prefix"
cmake_fat -S "$WORK/SDL-3.4.16" -B "$WORK/sdl-build" \
  -DCMAKE_INSTALL_PREFIX="$WORK/sdl-prefix" -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TESTS=OFF
cmake --build "$WORK/sdl-build" -j "$JOBS"
cmake --install "$WORK/sdl-build"

# SDL3_image: only native ImageIO PNG/BMP support is required by KeeperFX.
rm -rf "$WORK/sdlimg-build" "$WORK/sdlimg-prefix"
cmake_fat -S "$WORK/SDL_image-3.4.6" -B "$WORK/sdlimg-build" \
  -DCMAKE_INSTALL_PREFIX="$WORK/sdlimg-prefix" -DCMAKE_PREFIX_PATH="$WORK/sdl-prefix" \
  -DBUILD_SHARED_LIBS=OFF -DSDLIMAGE_VENDORED=OFF -DSDLIMAGE_DEPS_SHARED=OFF -DSDLIMAGE_BACKEND_IMAGEIO=ON \
  -DSDLIMAGE_BACKEND_STB=OFF -DSDLIMAGE_PNG=ON -DSDLIMAGE_PNG_LIBPNG=OFF -DSDLIMAGE_PNG_SAVE=ON \
  -DSDLIMAGE_BMP=ON -DSDLIMAGE_BMP_SAVE=ON -DSDLIMAGE_JPG=OFF -DSDLIMAGE_GIF=OFF \
  -DSDLIMAGE_TIF=OFF -DSDLIMAGE_WEBP=OFF -DSDLIMAGE_JXL=OFF -DSDLIMAGE_AVIF=OFF \
  -DSDLIMAGE_SVG=OFF -DSDLIMAGE_QOI=OFF -DSDLIMAGE_SAMPLES=OFF -DSDLIMAGE_TESTS=OFF
cmake --build "$WORK/sdlimg-build" -j "$JOBS"
cmake --install "$WORK/sdlimg-build"

# SDL3_mixer: built-in decoders keep the release independent of external codec dylibs.
rm -rf "$WORK/sdlmix-build" "$WORK/sdlmix-prefix"
cmake_fat -S "$WORK/SDL_mixer-3.2.4" -B "$WORK/sdlmix-build" \
  -DCMAKE_INSTALL_PREFIX="$WORK/sdlmix-prefix" -DCMAKE_PREFIX_PATH="$WORK/sdl-prefix" \
  -DBUILD_SHARED_LIBS=OFF -DSDLMIXER_VENDORED=OFF -DSDLMIXER_DEPS_SHARED=OFF -DSDLMIXER_TESTS=OFF -DSDLMIXER_EXAMPLES=OFF \
  -DSDLMIXER_MP3=ON -DSDLMIXER_MP3_DRMP3=ON -DSDLMIXER_MP3_MPG123=OFF \
  -DSDLMIXER_FLAC=ON -DSDLMIXER_FLAC_DRFLAC=ON -DSDLMIXER_FLAC_LIBFLAC=OFF \
  -DSDLMIXER_VORBIS_STB=ON -DSDLMIXER_VORBIS_VORBISFILE=OFF -DSDLMIXER_VORBIS_TREMOR=OFF \
  -DSDLMIXER_MIDI=ON -DSDLMIXER_MIDI_TIMIDITY=ON -DSDLMIXER_MIDI_FLUIDSYNTH=OFF \
  -DSDLMIXER_MOD=OFF -DSDLMIXER_OPUS=OFF -DSDLMIXER_GME=OFF -DSDLMIXER_WAVPACK=OFF
cmake --build "$WORK/sdlmix-build" -j "$JOBS"
cmake --install "$WORK/sdlmix-build"

# libspng against the macOS SDK zlib.
rm -rf "$WORK/spng-build" "$WORK/spng-prefix"
cmake_fat -S "$WORK/libspng-0.7.4" -B "$WORK/spng-build" \
  -DCMAKE_INSTALL_PREFIX="$WORK/spng-prefix" -DSPNG_SHARED=OFF -DSPNG_STATIC=ON -DBUILD_EXAMPLES=OFF
cmake --build "$WORK/spng-build" -j "$JOBS"
cmake --install "$WORK/spng-build"

# miniupnpc static Universal.
rm -rf "$WORK/miniupnpc-build" "$WORK/miniupnpc-prefix"
cmake_fat -S "$WORK/miniupnpc-2.3.3" -B "$WORK/miniupnpc-build" \
  -DCMAKE_INSTALL_PREFIX="$WORK/miniupnpc-prefix" -DUPNPC_BUILD_SHARED=OFF -DUPNPC_BUILD_STATIC=ON \
  -DUPNPC_BUILD_TESTS=OFF -DUPNPC_BUILD_SAMPLE=OFF
cmake --build "$WORK/miniupnpc-build" -j "$JOBS"
cmake --install "$WORK/miniupnpc-build"

# OpenAL Soft static Universal, CoreAudio only. Xcode 26's function-effects warning is
# incompatible with OpenAL Soft 1.25.2 callbacks, so keep that probe disabled.
rm -rf "$WORK/openal-build" "$WORK/openal-prefix"
cmake_fat -S "$WORK/openal-soft-src" -B "$WORK/openal-build" \
  -DCMAKE_INSTALL_PREFIX="$WORK/openal-prefix" -DLIBTYPE=STATIC -DALSOFT_TESTS=OFF -DALSOFT_EXAMPLES=OFF \
  -DALSOFT_UTILS=OFF -DALSOFT_BACKEND_PORTAUDIO=OFF -DALSOFT_BACKEND_JACK=OFF \
  -DALSOFT_BACKEND_SDL2=OFF -DALSOFT_BACKEND_SDL3=OFF -DALSOFT_REQUIRE_COREAUDIO=ON \
  -DHAVE_WFUNCTION_EFFECTS=FALSE
cmake --build "$WORK/openal-build" -j "$JOBS"
cmake --install "$WORK/openal-build"

# Lua 5.4: build each architecture separately, then lipo the archives.
rm -rf "$WORK/lua-fat"; mkdir -p "$WORK/lua-fat/include" "$WORK/lua-fat/lib"
cp "$WORK/lua-5.4.8/src/"{lua.h,lauxlib.h,lualib.h,luaconf.h,lua.hpp} "$WORK/lua-fat/include/"
for arch in arm64 x86_64; do
  obj="$WORK/lua-$arch-obj"; rm -rf "$obj"; mkdir -p "$obj"
  for c in "$WORK/lua-5.4.8/src/"*.c; do
    b=${c:t}; [[ "$b" == lua.c || "$b" == luac.c || "$b" == onelua.c ]] && continue
    clang -std=gnu99 -O2 -DLUA_USE_MACOSX -arch "$arch" -isysroot "$SDK" -mmacosx-version-min="$TARGET" -c "$c" -o "$obj/${b%.c}.o"
  done
  libtool -static -o "$WORK/lua-$arch.a" "$obj"/*.o
 done
lipo -create "$WORK/lua-arm64.a" "$WORK/lua-x86_64.a" -output "$WORK/lua-fat/lib/liblua.a"

# zlib contrib/minizip: KeeperFX expects the classic minizip API and header layout.
rm -rf "$WORK/minizip-fat"; mkdir -p "$WORK/minizip-fat/include/minizip" "$WORK/minizip-fat/lib"
cp "$WORK/zlib-1.3.2/contrib/minizip/"{ioapi.h,unzip.h,zip.h,ints.h} "$WORK/minizip-fat/include/minizip/"
for arch in arm64 x86_64; do
  obj="$WORK/minizip-$arch-obj"; rm -rf "$obj"; mkdir -p "$obj"
  for c in ioapi.c unzip.c zip.c; do clang -O2 -arch "$arch" -isysroot "$SDK" -mmacosx-version-min="$TARGET" -I"$WORK/zlib-1.3.2" -c "$WORK/zlib-1.3.2/contrib/minizip/$c" -o "$obj/${c%.c}.o"; done
  libtool -static -o "$WORK/minizip-$arch.a" "$obj"/*.o
 done
lipo -create "$WORK/minizip-arm64.a" "$WORK/minizip-x86_64.a" -output "$WORK/minizip-fat/lib/libminizip.a"

# libnatpmp: tiny C library, built per architecture then merged.
rm -rf "$WORK/libnatpmp-fat"; mkdir -p "$WORK/libnatpmp-fat/include/natpmp" "$WORK/libnatpmp-fat/lib"
cp "$WORK/libnatpmp-git/"{natpmp.h,natpmp_declspec.h,getgateway.h} "$WORK/libnatpmp-fat/include/natpmp/"
for arch in arm64 x86_64; do
  obj="$WORK/natpmp-$arch-obj"; rm -rf "$obj"; mkdir -p "$obj"
  clang -O2 -arch "$arch" -isysroot "$SDK" -mmacosx-version-min="$TARGET" -c "$WORK/libnatpmp-git/natpmp.c" -o "$obj/natpmp.o"
  clang -O2 -arch "$arch" -isysroot "$SDK" -mmacosx-version-min="$TARGET" -c "$WORK/libnatpmp-git/getgateway.c" -o "$obj/getgateway.o"
  libtool -static -o "$WORK/natpmp-$arch.a" "$obj"/*.o
 done
lipo -create "$WORK/natpmp-arm64.a" "$WORK/natpmp-x86_64.a" -output "$WORK/libnatpmp-fat/lib/libnatpmp.a"

# FFmpeg: deliberately tiny static media core for Bullfrog Smacker + Enhanced H.264/AAC MP4.
rm -rf "$WORK/ffmpeg-fat"; mkdir -p "$WORK/ffmpeg-fat/lib"
for arch in arm64 x86_64; do
  b="$WORK/ffmpeg-$arch"; p="$WORK/ffmpeg-prefix-$arch"; rm -rf "$b" "$p"; mkdir -p "$b"
  extra=(); [[ "$arch" == x86_64 ]] && extra+=(--disable-x86asm)
  (cd "$b" && "$WORK/ffmpeg-9.0.1/configure" --prefix="$p" --target-os=darwin --arch="$arch" --cc=clang --cxx=clang++ \
    "${extra[@]}" --disable-programs --disable-doc --disable-debug --disable-shared --enable-static --disable-autodetect --disable-everything \
    --enable-avformat --enable-avcodec --enable-avutil --enable-swscale --enable-swresample --enable-protocol=file \
    --enable-demuxer=smacker,mov --enable-decoder=smacker,smackaud,h264,aac --enable-parser=h264,aac \
    --enable-bsf=aac_adtstoasc,h264_mp4toannexb \
    --extra-cflags="-arch $arch -isysroot $SDK -mmacosx-version-min=$TARGET" \
    --extra-ldflags="-arch $arch -isysroot $SDK -mmacosx-version-min=$TARGET")
  grep -q '^#define CONFIG_SMACKER_DECODER 1$' "$b/config_components.h" || { echo "Smacker video decoder missing" >&2; exit 1; }
  make -C "$b" -j "$JOBS"
  make -C "$b" install
 done
cp -R "$WORK/ffmpeg-prefix-arm64/include" "$WORK/ffmpeg-fat/"
for lib in avformat avcodec swresample swscale avutil; do
  lipo -create "$WORK/ffmpeg-prefix-arm64/lib/lib$lib.a" "$WORK/ffmpeg-prefix-x86_64/lib/lib$lib.a" -output "$WORK/ffmpeg-fat/lib/lib$lib.a"
done

"$ROOT/macos/build-network-deps.sh"

echo "Universal macOS dependencies ready: $WORK"
for f in "$WORK"/{sdl-prefix/lib/libSDL3.a,sdlimg-prefix/lib/libSDL3_image.a,sdlmix-prefix/lib/libSDL3_mixer.a,openal-prefix/lib/libopenal.a,lua-fat/lib/liblua.a,spng-prefix/lib/libspng_static.a,minizip-fat/lib/libminizip.a,miniupnpc-prefix/lib/libminiupnpc.a,libnatpmp-fat/lib/libnatpmp.a,ffmpeg-fat/lib/libavcodec.a}; do lipo -info "$f"; done
