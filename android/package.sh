#!/usr/bin/env bash
# Builds an Android APK (arm64 phones): SFML and the game with the NDK, packed with the SDK's build
# tools, signed with a local debug key. No Gradle, no Java code: Android's NativeActivity runs the game.
# Writes dist/samuride-<version>-android-arm64.apk
#
# Usage: android/package.sh [version]
# Needs a JDK, cmake, git, make, unzip, zip and curl (Ubuntu: openjdk-17-jdk-headless cmake git make
# unzip zip curl). The Android SDK and NDK are downloaded into build/android the first time.
# From any Linux with Docker:
#   docker run --rm -v "$PWD":/src -w /src ubuntu:24.04 bash -c \
#     'apt-get update -qq && apt-get install -y -qq openjdk-17-jdk-headless cmake git make unzip zip curl \
#      && android/package.sh && chown -R '"$(id -u):$(id -g)"' build dist'
set -euo pipefail

VERSION="${1:-dev}"
SFML_VERSION="3.1.0"
ABI="arm64-v8a"
MIN_SDK=26
TARGET_SDK=34
NDK_VERSION="27.2.12479018"
BUILD_TOOLS="34.0.0"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build/android"
SDK="$BUILD/sdk"
NDK="$SDK/ndk/$NDK_VERSION"
TOOLS="$SDK/build-tools/$BUILD_TOOLS"
PACKAGE="samuride-$VERSION-android-arm64"

# Android SDK and NDK, downloaded once
if [ ! -x "$TOOLS/aapt2" ] || [ ! -d "$NDK" ]; then
    mkdir -p "$SDK"
    curl -sSfLo "$BUILD/cmdline-tools.zip" https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip
    rm -rf "$SDK/cmdline-tools"
    unzip -q "$BUILD/cmdline-tools.zip" -d "$SDK/cmdline-tools"
    mv "$SDK/cmdline-tools/cmdline-tools" "$SDK/cmdline-tools/latest"
    # yes dies of SIGPIPE once the licenses are accepted: not a failure
    (yes || true) | "$SDK/cmdline-tools/latest/bin/sdkmanager" --sdk_root="$SDK" --licenses > /dev/null
    "$SDK/cmdline-tools/latest/bin/sdkmanager" --sdk_root="$SDK" \
        "platforms;android-$TARGET_SDK" "build-tools;$BUILD_TOOLS" "ndk;$NDK_VERSION" > /dev/null
fi

ANDROID_CMAKE=(-DCMAKE_SYSTEM_NAME=Android -DCMAKE_ANDROID_NDK="$NDK" -DCMAKE_ANDROID_ARCH_ABI="$ABI"
               -DCMAKE_ANDROID_API="$MIN_SDK" -DCMAKE_ANDROID_STL_TYPE=c++_static -DCMAKE_BUILD_TYPE=Release)

# SFML 3 for Android, static, built once; it installs itself into the NDK
SFML_CMAKE_DIR="$NDK/sources/third_party/sfml/lib/$ABI/cmake/SFML"
if [ ! -d "$SFML_CMAKE_DIR" ]; then
    rm -rf "$BUILD/sfml-src" "$BUILD/sfml-build"
    git clone -q --depth 1 --branch "$SFML_VERSION" https://github.com/SFML/SFML.git "$BUILD/sfml-src"
    cmake -S "$BUILD/sfml-src" -B "$BUILD/sfml-build" "${ANDROID_CMAKE[@]}" \
          -DBUILD_SHARED_LIBS=OFF -DSFML_BUILD_NETWORK=OFF
    cmake --build "$BUILD/sfml-build" --parallel
    cmake --install "$BUILD/sfml-build"
fi

# The game: libsamuride.so
cmake -S "$ROOT/android" -B "$BUILD/game" "${ANDROID_CMAKE[@]}" -DSFML_DIR="$SFML_CMAKE_DIR"
cmake --build "$BUILD/game" --parallel

# What the APK holds: the manifest, the assets (read where the game expects them: assets/..., maps/...)
# and the library
STAGE="$BUILD/stage"
rm -rf "$STAGE"
mkdir -p "$STAGE/assets" "$STAGE/lib/$ABI"
cp -r "$ROOT/assets" "$ROOT/maps" "$STAGE/assets/"
# The sprite sheet sources are only needed to regenerate the characters
rm -rf "$STAGE/assets/assets/sources"
cp "$BUILD/game/libsamuride.so" "$STAGE/lib/$ABI/"

"$TOOLS/aapt2" link -o "$BUILD/unaligned.apk" --manifest "$ROOT/android/AndroidManifest.xml" \
    -I "$SDK/platforms/android-$TARGET_SDK/android.jar" --min-sdk-version "$MIN_SDK" \
    --target-sdk-version "$TARGET_SDK" --version-name "$VERSION" -A "$STAGE/assets"
(cd "$STAGE" && zip -q -0 -r "$BUILD/unaligned.apk" lib)

# Signed with a key kept in build/android: reinstalls over a previous build need the same key
KEYSTORE="$BUILD/debug.keystore"
if [ ! -f "$KEYSTORE" ]; then
    keytool -genkeypair -keystore "$KEYSTORE" -storepass android -keypass android -alias samuride \
            -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Samuride" > /dev/null 2>&1
fi
mkdir -p "$ROOT/dist"
"$TOOLS/zipalign" -f -P 16 4 "$BUILD/unaligned.apk" "$BUILD/aligned.apk"
"$TOOLS/apksigner" sign --ks "$KEYSTORE" --ks-pass pass:android --out "$ROOT/dist/$PACKAGE.apk" "$BUILD/aligned.apk"
echo "Packed dist/$PACKAGE.apk"
