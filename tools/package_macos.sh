#!/usr/bin/env bash
# Builds a self-contained macOS release on a Mac: binaries, assets and the SFML 3 dylibs they
# load (SFML builds its own dependencies into them), zipped as
# dist/samuride-<version>-macos-arm64.zip
#
# Usage: tools/package_macos.sh [version]
# Needs the Xcode command line tools and cmake. Built for Apple Silicon, macOS 13.3 or newer.
set -euo pipefail

VERSION="${1:-dev}"
SFML_VERSION="3.1.0"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build/macos"
SFML_DIR="$BUILD/sfml"
PACKAGE="samuride-$VERSION-macos-arm64"
STAGE="$ROOT/dist/$PACKAGE"

# SFML 3, built once and reused; its dylibs are named @rpath/... so they can travel
if [ ! -f "$SFML_DIR/lib/libsfml-graphics.dylib" ]; then
    git clone --depth 1 --branch "$SFML_VERSION" https://github.com/SFML/SFML.git "$BUILD/sfml-src"
    cmake -S "$BUILD/sfml-src" -B "$BUILD/sfml-build" -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_SHARED_LIBS=ON -DSFML_BUILD_NETWORK=OFF -DCMAKE_INSTALL_PREFIX="$SFML_DIR" \
          -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=13.3 -DCMAKE_INSTALL_NAME_DIR=@rpath
    cmake --build "$BUILD/sfml-build" --parallel
    cmake --install "$BUILD/sfml-build"
fi

# The binaries look for SFML in lib/ next to themselves
make -C "$ROOT" fclean
make -C "$ROOT" -j"$(sysctl -n hw.ncpu)" CC=clang++ SFML_DIR="$SFML_DIR" RPATH='@executable_path/lib'
make -C "$ROOT" CC=clang++ SFML_DIR="$SFML_DIR" tests

rm -rf "$STAGE"
mkdir -p "$STAGE/lib"
cp "$ROOT/jetpack_client" "$ROOT/jetpack_server" "$ROOT/README.md" "$ROOT/LICENSE" "$STAGE/"
cp -a "$SFML_DIR"/lib/libsfml-*.dylib "$STAGE/lib/"
cp -r "$ROOT/maps" "$ROOT/assets" "$STAGE/"
# The sprite sheet sources are only needed to regenerate the characters
rm -rf "$STAGE/assets/sources"

# Everything must load from the package or the system, never from this build machine
if otool -L "$STAGE/jetpack_client" "$STAGE"/lib/*.dylib | grep -E "$ROOT|/opt/homebrew|/usr/local"; then
    echo "A binary depends on a library outside the package" >&2
    exit 1
fi

# Apple Silicon only runs signed code: an ad-hoc signature, no Apple account involved
codesign --force --sign - "$STAGE/jetpack_client" "$STAGE/jetpack_server" "$STAGE"/lib/*.dylib

(cd "$ROOT/dist" && rm -f "$PACKAGE.zip" && zip -qry "$PACKAGE.zip" "$PACKAGE")
make -C "$ROOT" fclean
echo "Packed dist/$PACKAGE.zip"
