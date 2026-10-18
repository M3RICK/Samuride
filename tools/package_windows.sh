#!/usr/bin/env bash
# Cross-compiles a Windows release from Linux with MinGW: the .exe files, the SFML DLLs and the
# MinGW runtime they need, and the assets, zipped as dist/samuride-<version>-windows-x86_64.zip
#
# Usage: tools/package_windows.sh [version]
# Needs the MinGW-w64 POSIX-threads compiler, cmake, git, make and zip
# (Ubuntu: g++-mingw-w64-x86-64-posix cmake git make zip).
set -euo pipefail

VERSION="${1:-dev}"
SFML_VERSION="3.1.0"
CXX="x86_64-w64-mingw32-g++-posix"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build/windows"
SFML_DIR="$BUILD/sfml"
PACKAGE="samuride-$VERSION-windows-x86_64"
STAGE="$ROOT/dist/$PACKAGE"

# SFML 3 for Windows, built once and reused; it fetches its own dependencies
if [ ! -f "$SFML_DIR/lib/libsfml-graphics.dll.a" ]; then
    git clone --depth 1 --branch "$SFML_VERSION" https://github.com/SFML/SFML.git "$BUILD/sfml-src"
    cmake -S "$BUILD/sfml-src" -B "$BUILD/sfml-build" -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_CXX_COMPILER="$CXX" \
          -DCMAKE_C_COMPILER="x86_64-w64-mingw32-gcc-posix" -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres \
          -DBUILD_SHARED_LIBS=ON -DSFML_BUILD_NETWORK=OFF -DCMAKE_INSTALL_PREFIX="$SFML_DIR"
    cmake --build "$BUILD/sfml-build" --parallel
    cmake --install "$BUILD/sfml-build"
fi

make -C "$ROOT" fclean
make -C "$ROOT" -j"$(nproc)" TARGET=windows CC="$CXX" SFML_DIR="$SFML_DIR"

rm -rf "$STAGE"
mkdir -p "$STAGE"
cp "$ROOT/Samuride.exe" "$ROOT/Samuride_server.exe" "$ROOT/README.md" "$ROOT/LICENSE" "$STAGE/"
cp "$SFML_DIR"/bin/*.dll "$STAGE/"

# SFML's DLLs need the MinGW runtime the game links statically
for dll in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
    cp "$($CXX -print-file-name=$dll)" "$STAGE/"
done

cp -r "$ROOT/maps" "$ROOT/assets" "$STAGE/"
# The sprite sheet sources are only needed to regenerate the characters
rm -rf "$STAGE/assets/sources"

(cd "$ROOT/dist" && rm -f "$PACKAGE.zip" && zip -qr "$PACKAGE.zip" "$PACKAGE")
make -C "$ROOT" fclean
echo "Packed dist/$PACKAGE.zip"
