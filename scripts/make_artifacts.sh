#!/bin/bash
# Collect build artifacts for packaging.
#
# Usage:
#   make_artifacts.sh [arch]      # arch: amd64 (default) | arm64
#
# For amd64: collects from build_amd64/ (full build including convert + slam)
# For arm64: collects from build_arm64/ (daemon + replay + storage only)
set -euo pipefail

ARCH="${1:-amd64}"
BUILD_DIR="build_${ARCH}"

echo "Making Artifacts (arch=${ARCH}, build_dir=${BUILD_DIR})"

cd "$(dirname "$0")/.."

mkdir -p artifacts/script/daemon

mkdir -p "artifacts/bin/${ARCH}/"
mkdir -p "artifacts/lib/${ARCH}/"
mkdir -p "artifacts/plugin/${ARCH}/base"
mkdir -p "artifacts/plugin/${ARCH}/driver"
mkdir -p "artifacts/plugin/${ARCH}/upload"

# ── Binaries ──────────────────────────────────────────────────────────────────
if [ "${ARCH}" = "amd64" ]; then
    # amd64 includes convert + slam (optional — skip if not built)
    for d in convert/hera-*; do
        [ -f "${BUILD_DIR}/${d}" ] && cp "${BUILD_DIR}/${d}" "artifacts/bin/${ARCH}/" || true
    done
    [ -f "${BUILD_DIR}/convert/libhera-convert-ros-message.so" ] && \
        cp "${BUILD_DIR}/convert/libhera-convert-ros-message.so" "artifacts/lib/${ARCH}/" || true

    for slam_bin in \
        "${BUILD_DIR}/slam/bridge/hera-slam-"* \
        "${BUILD_DIR}/slam/caller/hera-slam-"* \
        "${BUILD_DIR}/slam/result/hera-slam-"*; do
        [ -f "${slam_bin}" ] && cp "${slam_bin}" "artifacts/bin/${ARCH}/" || true
    done
fi

# Daemon, device tools, replay, storage tools (both arches)
for sub in daemon device replay storage; do
    if compgen -G "${BUILD_DIR}/${sub}/hera-*" > /dev/null 2>&1; then
        cp -r "${BUILD_DIR}/${sub}/hera-"* "artifacts/bin/${ARCH}/"
    fi
done

# ── Libraries ─────────────────────────────────────────────────────────────────
for lib in common/libhera-common.so device/libhera-device.so storage/libhera-storage.so; do
    [ -f "${BUILD_DIR}/${lib}" ] && \
        cp "${BUILD_DIR}/${lib}" "artifacts/lib/${ARCH}/" || true
done

# ── Plugins ───────────────────────────────────────────────────────────────────
if compgen -G "${BUILD_DIR}/device/base/libhera-device-plugin-*-base.so" > /dev/null 2>&1; then
    cp "${BUILD_DIR}/device/base/libhera-device-plugin-"*"-base.so" \
       "artifacts/plugin/${ARCH}/base/"
fi

# Camera / sensor driver plugins (may be absent if SDK not found)
if compgen -G "${BUILD_DIR}/device/driver/libhera-device-plugin-*-driver.so" > /dev/null 2>&1; then
    cp "${BUILD_DIR}/device/driver/libhera-device-plugin-"*"-driver.so" \
       "artifacts/plugin/${ARCH}/driver/"
fi

if compgen -G "${BUILD_DIR}/storage/upload/libhera-storage-upload-*.so" > /dev/null 2>&1; then
    cp "${BUILD_DIR}/storage/upload/libhera-storage-upload-"*.so \
       "artifacts/plugin/${ARCH}/upload/"
fi

# ── Vendor SDK runtime libraries ──────────────────────────────────────────────
# libCameraSDK.so must ship in the .deb; it's a closed-source binary not in apt.
if [ -n "${INSTA_SDK_ROOT:-}" ] && [ -f "${INSTA_SDK_ROOT}/lib/libCameraSDK.so" ]; then
    cp "${INSTA_SDK_ROOT}/lib/libCameraSDK.so" "artifacts/lib/${ARCH}/"
    echo "  + libCameraSDK.so (Insta360 runtime, from ${INSTA_SDK_ROOT}/lib)"
fi

# ── Systemd + config ──────────────────────────────────────────────────────────
cp daemon/script/hera-daemon.service artifacts/script/daemon/
cp daemon/script/udiskie.service     artifacts/script/daemon/ 2>/dev/null || true
cp daemon/config/daemon.conf         artifacts/script/daemon/

echo "Artifacts ready in artifacts/ (arch=${ARCH})"
echo "  bins:    $(ls artifacts/bin/${ARCH}/ | wc -l) files"
echo "  libs:    $(ls artifacts/lib/${ARCH}/ | wc -l) files"
echo "  plugins: $(find artifacts/plugin/${ARCH}/ -name '*.so' | wc -l) .so files"
