#!/usr/bin/env bash
# Builds the APK in Docker, from any Linux: dist/samuride-<version>-android.apk
# The image with the build tools is made the first time; after that, a build needs no internet.
#
# Usage: android/docker.sh [version]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE="samuride-android"

docker image inspect "$IMAGE" > /dev/null 2>&1 || docker build -q -t "$IMAGE" "$ROOT/android" > /dev/null
# As you, so build/ and dist/ stay yours
docker run --rm -u "$(id -u):$(id -g)" -e HOME=/tmp -v "$ROOT":/src -w /src "$IMAGE" android/package.sh "$@"
