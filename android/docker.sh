#!/usr/bin/env bash
# Builds the APK in Docker, from any Linux: dist/samuride-<version>-android.apk
# The image with the build tools is made the first time; after that, a build needs no internet.
#
# Usage: android/docker.sh [version]
#   SHOW_FPS=1 android/docker.sh: with a frame counter in a corner, for performance checks
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE="samuride-android"

docker image inspect "$IMAGE" > /dev/null 2>&1 || docker build -q -t "$IMAGE" "$ROOT/android" > /dev/null
# As you, so build/ and dist/ stay yours
docker run --rm -u "$(id -u):$(id -g)" -e HOME=/tmp -e SHOW_FPS="${SHOW_FPS:-0}" -v "$ROOT":/src -w /src "$IMAGE" android/package.sh "$@"
