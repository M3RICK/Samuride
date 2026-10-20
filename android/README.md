# Android

Everything only the Android build needs. The game itself is the same code as on desktop (`src/`):
the few differences live in `src/common/platform.cpp` (where saves go) and the touch controls.

- `docker.sh`: the way to build: `android/docker.sh [version]` runs `package.sh` in a Docker image
  that has the build tools (`Dockerfile`, made the first time). After that, builds need no internet.
- `package.sh`: builds `dist/samuride-<version>-android.apk`, for arm64 phones. Downloads the Android SDK and NDK
  into `build/android` the first time, builds SFML for Android, then the game, and signs the APK
  with a key kept in `build/android/debug.keystore` (keep it: an update only installs over a build
  signed with the same key).
- `CMakeLists.txt`: the game as `libsamuride.so`, the library Android's NativeActivity loads.
- `AndroidManifest.xml`: no Java code, landscape, fullscreen, network access.

Releases: pushing a `v*` tag builds the APK on GitHub too (`.github/workflows/release.yml`), signed
with the same key, given to GitHub once as a secret. On GitHub: Settings > Secrets and variables >
Actions > New repository secret, named `ANDROID_KEYSTORE`, with the output of
`base64 -w0 build/android/debug.keystore` as its value. Back the keystore up somewhere: without it,
the next APK won't install over the one on the phones.

Installing: copy the APK to the phone and open it; Android asks once to allow installing apps from
that source.
