#!/usr/bin/env bash
# Builds a self-contained Linux release: binaries, assets and the SFML 3 libraries they load,
# packed as dist/samuride-<version>-linux-x86_64.tar.gz
#
# Usage: tools/package_release.sh [version]
# Needs a C++20 compiler, cmake, git and the SFML build dependencies (see the release workflow).
set -euo pipefail

VERSION="${1:-dev}"
SFML_VERSION="3.1.0"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build/release"
SFML_DIR="$BUILD/sfml"
PACKAGE="samuride-$VERSION-linux-x86_64"
STAGE="$ROOT/dist/$PACKAGE"

# SFML 3, built once and reused
if [ ! -f "$SFML_DIR/lib/libsfml-graphics.so" ]; then
    git clone --depth 1 --branch "$SFML_VERSION" https://github.com/SFML/SFML.git "$BUILD/sfml-src"
    cmake -S "$BUILD/sfml-src" -B "$BUILD/sfml-build" -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_SHARED_LIBS=ON -DSFML_BUILD_NETWORK=OFF -DCMAKE_INSTALL_PREFIX="$SFML_DIR" \
          -DCMAKE_INSTALL_LIBDIR=lib
    cmake --build "$BUILD/sfml-build" --parallel
    cmake --install "$BUILD/sfml-build"
fi

# The binaries look for SFML in lib/ next to themselves
make -C "$ROOT" fclean
make -C "$ROOT" -j"$(nproc)" SFML_DIR="$SFML_DIR" RPATH='$$ORIGIN/lib'

rm -rf "$STAGE"
mkdir -p "$STAGE/lib"
cp "$ROOT/Samuride" "$ROOT/Samuride_server" "$ROOT/README.md" "$ROOT/LICENSE" "$STAGE/"
cp -a "$SFML_DIR"/lib/libsfml-*.so* "$STAGE/lib/"

# SFML's own dependencies (FreeType, HarfBuzz, FLAC, Vorbis...) change versions between distros:
# bundle them too, and rely only on what every Linux desktop has
ldd "$STAGE"/lib/libsfml-*.so.* | awk '/=> \//{print $3}' | sort -u \
    | grep -vE '/(libc|libm|libdl|libpthread|librt|libstdc\+\+|libgcc_s|ld-linux[^/]*|libX[^/]*|libxcb[^/]*|libGL[^/]*|libOpenGL|libudev|libsfml[^/]*)\.so' \
    | xargs -r cp -L -t "$STAGE/lib/"
cp -r "$ROOT/maps" "$STAGE/"
cp -r "$ROOT/assets" "$STAGE/"
# The sprite sheet sources are only needed to regenerate the characters
rm -rf "$STAGE/assets/sources"

tar -C "$ROOT/dist" -czf "$ROOT/dist/$PACKAGE.tar.gz" "$PACKAGE"
make -C "$ROOT" fclean
echo "Packed dist/$PACKAGE.tar.gz"
