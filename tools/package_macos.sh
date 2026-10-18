#!/usr/bin/env bash
# Builds a self-contained macOS release on a Mac: a Samuride.app holding the binaries, the assets
# and the SFML 3 dylibs they load (SFML builds its own dependencies into them), zipped as
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

# A Samuride.app: double-clicking it opens the game window, not a Terminal.
# The binaries and SFML go in Contents/MacOS, the game data in Contents/Resources.
APP="$STAGE/Samuride.app/Contents"
rm -rf "$STAGE"
mkdir -p "$APP/MacOS/lib" "$APP/Resources"
cp "$ROOT/jetpack_client" "$ROOT/jetpack_server" "$APP/MacOS/"
cp -a "$SFML_DIR"/lib/libsfml-*.dylib "$APP/MacOS/lib/"
cp -r "$ROOT/maps" "$ROOT/assets" "$APP/Resources/"
# The sprite sheet sources are only needed to regenerate the characters
rm -rf "$APP/Resources/assets/sources"
cp "$ROOT/README.md" "$ROOT/LICENSE" "$STAGE/"

cat > "$APP/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>                 <string>Samuride</string>
    <key>CFBundleDisplayName</key>          <string>Samuride</string>
    <key>CFBundleIdentifier</key>           <string>io.github.m3rick.samuride</string>
    <key>CFBundleExecutable</key>           <string>jetpack_client</string>
    <key>CFBundlePackageType</key>          <string>APPL</string>
    <key>CFBundleShortVersionString</key>   <string>${VERSION#v}</string>
    <key>LSMinimumSystemVersion</key>       <string>13.3</string>
</dict>
</plist>
PLIST

# Everything must load from the package or the system, never from this build machine.
# otool prints each file's own path as a header: only its indented dependency lines count.
if otool -L "$APP/MacOS/jetpack_client" "$APP"/MacOS/lib/*.dylib | grep -E '^[[:space:]]' | grep -E "$ROOT|/opt/homebrew|/usr/local"; then
    echo "A binary depends on a library outside the package" >&2
    exit 1
fi

# Apple Silicon only runs signed code: ad-hoc signatures, no Apple account involved.
# Inner code first, then the app as a whole.
codesign --force --sign - "$APP"/MacOS/lib/*.dylib "$APP/MacOS/jetpack_server"
codesign --force --sign - "$STAGE/Samuride.app"

(cd "$ROOT/dist" && rm -f "$PACKAGE.zip" && zip -qry "$PACKAGE.zip" "$PACKAGE")
make -C "$ROOT" fclean
echo "Packed dist/$PACKAGE.zip"
