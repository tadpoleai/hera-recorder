#!/bin/bash
# Native arm64 build — intended to run directly on the Jetson (or any arm64 host).
# For cross-compilation from x86, use build_arm_host.sh instead.
#
# Insta360 CameraSDK: place the Jetson SDK at ~/insta_sdk/ on the Jetson, or set
# INSTA_SDK_ROOT=/path/to/CameraSDK-2.1.1-jetson before invoking this script.
set -euo pipefail

echo "Building for arm64 (native)"

cd "$(dirname "$0")/.."

mkdir -p build_arm64
cd build_arm64

# Pass INSTA_SDK_ROOT to CMake if set; cmake/plugin.cmake also auto-scans ~/insta_sdk.
INSTA_SDK_ARG=""
if [ -n "${INSTA_SDK_ROOT:-}" ]; then
    INSTA_SDK_ARG="-DINSTA_SDK_ROOT=${INSTA_SDK_ROOT}"
fi

GIT_VERSION="${1:-}"
if [ -n "${GIT_VERSION}" ]; then
    echo "git version override: ${GIT_VERSION}"
    cmake .. \
        -DWITH_DAEMON=ON \
        -DWITH_REPLAY=ON \
        -DWITH_CONVERT=OFF \
        -DWITH_SLAM=OFF \
        ${INSTA_SDK_ARG} \
        -Dforce-git-info="${GIT_VERSION}"
else
    cmake .. \
        -DWITH_DAEMON=ON \
        -DWITH_REPLAY=ON \
        -DWITH_CONVERT=OFF \
        -DWITH_SLAM=OFF \
        ${INSTA_SDK_ARG}
fi

make -j"$(nproc)"
