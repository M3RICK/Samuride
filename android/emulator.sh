#!/usr/bin/env bash
# Plays the APK in Android's emulator on this PC (x86_64 Linux with KVM): a landscape phone running
# Android 14, the latest dist/samuride-*-android.apk installed and started.
# The mouse is a finger; the keyboard is passed through, so Space, F and E play as on desktop.
#
# Usage: android/emulator.sh [--headless]
#   --headless: no window, for scripted checks (adb exec-out screencap -p, adb shell input tap X Y)
# Run android/package.sh first. The emulator and the system image are downloaded into build/android
# the first time, with Docker (the SDK manager needs Java); the virtual phone lives there too.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build/android"
SDK="$BUILD/sdk"
IMAGE_DIR="system-images/android-34/google_apis/x86_64"
export ANDROID_SDK_ROOT="$SDK" ANDROID_HOME="$SDK" ANDROID_AVD_HOME="$BUILD/avd"
ADB="$SDK/platform-tools/adb"
APK="$(ls -t "$ROOT"/dist/samuride-*-android.apk 2>/dev/null | head -1)"

[ -n "$APK" ] || { echo "No APK in dist/: run android/package.sh first" >&2; exit 1; }
[ -w /dev/kvm ] || { echo "The emulator needs KVM: /dev/kvm is not writable" >&2; exit 1; }

if [ ! -x "$SDK/emulator/emulator" ] || [ ! -d "$SDK/$IMAGE_DIR" ] || [ ! -x "$ADB" ]; then
    docker run --rm -v "$SDK":/sdk ubuntu:24.04 bash -c \
        'export DEBIAN_FRONTEND=noninteractive; apt-get update -qq && apt-get install -y -qq openjdk-17-jre-headless > /dev/null 2>&1 \
         && /sdk/cmdline-tools/latest/bin/sdkmanager --sdk_root=/sdk emulator platform-tools "system-images;android-34;google_apis;x86_64" > /dev/null \
         && chown -R '"$(id -u):$(id -g)"' /sdk'
fi

# The virtual phone: a 20:9 screen, the host keyboard passed through
AVD="$ANDROID_AVD_HOME/samuride.avd"
if [ ! -f "$AVD/config.ini" ]; then
    mkdir -p "$AVD"
    printf 'avd.ini.encoding=UTF-8\npath=%s\ntarget=android-34\n' "$AVD" > "$ANDROID_AVD_HOME/samuride.ini"
    cat > "$AVD/config.ini" <<CONFIG
avd.ini.encoding=UTF-8
abi.type=x86_64
hw.cpu.arch=x86_64
hw.cpu.ncore=4
hw.ramSize=4096
disk.dataPartition.size=4G
image.sysdir.1=$IMAGE_DIR/
tag.id=google_apis
hw.lcd.width=1080
hw.lcd.height=2400
hw.lcd.density=420
hw.keyboard=yes
hw.gpu.enabled=yes
hw.gpu.mode=auto
showDeviceFrame=no
CONFIG
fi

WINDOW=()
[ "${1:-}" = "--headless" ] && WINDOW=(-no-window -no-audio -gpu swiftshader_indirect)
"$SDK/emulator/emulator" -avd samuride -no-snapshot -no-boot-anim "${WINDOW[@]}" > "$BUILD/emulator.log" 2>&1 &

"$ADB" wait-for-device
until [ "$("$ADB" shell getprop sys.boot_completed 2>/dev/null | tr -d '\r')" = "1" ]; do sleep 2; done
"$ADB" install -r "$APK" > /dev/null
"$ADB" shell am start -n com.m3rick.samuride/android.app.NativeActivity > /dev/null
echo "Samuride is running ($APK). Crash logs: $ADB logcat -b crash"
