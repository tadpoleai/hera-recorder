#!/bin/bash
# Cross-compile hera-daemon for Jetson (aarch64) and package as .deb.
# Run on a standard Ubuntu 22.04/24.04 x86_64 host.
#
# Usage:
#   ./scripts/build_deb_arm64.sh [version]
#
# Env overrides:
#   INSTA_SDK_ROOT  path to CameraSDK-2.1.1-jetson  (auto-detected below)
#   LIVOX_SDK2_ROOT path to Livox-SDK2 repo root     (auto-detected below)
set -euo pipefail

VERSION="${1:-dev}"
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# ── SDK auto-detect ────────────────────────────────────────────────────────────
: "${INSTA_SDK_ROOT:=/home/fred/Code/sdks/insta_sdk/CameraSDK-2.1.1-jetson}"
: "${LIVOX_SDK2_ROOT:=/home/fred/Code/sdks/Livox-SDK2}"

# ── Dependency check / install ─────────────────────────────────────────────────
if ! command -v aarch64-linux-gnu-gcc &>/dev/null; then
    echo "[setup] Installing aarch64 cross-compile toolchain..."
    sudo dpkg --add-architecture arm64
    sudo sed -i 's/^deb /deb [arch=amd64] /' /etc/apt/sources.list 2>/dev/null || true
    for f in /etc/apt/sources.list.d/*.list; do
        sudo sed -i 's/^deb /deb [arch=amd64] /' "$f" 2>/dev/null || true
    done
    CODENAME="$(. /etc/os-release && echo "${VERSION_CODENAME}")"
    cat <<EOF | sudo tee /etc/apt/sources.list.d/arm64-ports.list
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports ${CODENAME} main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports ${CODENAME}-updates main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports ${CODENAME}-security main restricted universe multiverse
EOF
    sudo apt-get update -qq
    sudo apt-get install -y --no-install-recommends \
        gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
        cmake make pkg-config dpkg-dev thrift-compiler \
        libboost-dev:arm64 libboost-filesystem-dev:arm64 \
        libboost-thread-dev:arm64 libboost-program-options-dev:arm64 \
        libevent-dev:arm64 libconfig++-dev:arm64 \
        libthrift-dev:arm64 \
        libturbojpeg0-dev:arm64 \
        libusb-1.0-0-dev:arm64
fi

# libjpeg-turbo symlinks cmake's turbojpeg.cmake expects on aarch64
if [ ! -f /opt/libjpeg-turbo/lib64/libturbojpeg.so ]; then
    sudo mkdir -p /opt/libjpeg-turbo/lib64 /opt/libjpeg-turbo/include
    sudo ln -sf /usr/lib/aarch64-linux-gnu/libturbojpeg.so \
                /opt/libjpeg-turbo/lib64/libturbojpeg.so
    sudo ln -sf /usr/include/turbojpeg.h \
                /opt/libjpeg-turbo/include/turbojpeg.h
fi

# ── Livox SDK2: ensure lib/ layout ────────────────────────────────────────────
LIVOX_LIB="${LIVOX_SDK2_ROOT}/lib"
LIVOX_SO="${LIVOX_SDK2_ROOT}/build/sdk_core/liblivox_lidar_sdk_shared.so"
if [ ! -f "${LIVOX_LIB}/liblivox_lidar_sdk_shared.so" ]; then
    echo "[setup] Copying Livox arm64 .so to ${LIVOX_LIB}/"
    mkdir -p "${LIVOX_LIB}"
    cp "${LIVOX_SO}" "${LIVOX_LIB}/"
fi

# ── Validate SDKs ──────────────────────────────────────────────────────────────
if [ ! -f "${INSTA_SDK_ROOT}/include/camera/camera.h" ]; then
    echo "ERROR: Insta360 SDK not found at ${INSTA_SDK_ROOT}"
    echo "  Set INSTA_SDK_ROOT to the CameraSDK-2.1.1-jetson directory."
    exit 1
fi
if [ ! -f "${LIVOX_LIB}/liblivox_lidar_sdk_shared.so" ]; then
    echo "ERROR: Livox SDK2 .so not found. Build it first:"
    echo "  cd ${LIVOX_SDK2_ROOT} && mkdir -p build && cd build"
    echo "  cmake .. -DCMAKE_TOOLCHAIN_FILE=${REPO_ROOT}/cmake/toolchain-aarch64.cmake"
    echo "  make -j\$(nproc)"
    exit 1
fi

echo "[build] version=${VERSION}"
echo "[build] INSTA_SDK_ROOT=${INSTA_SDK_ROOT}"
echo "[build] LIVOX_SDK2_ROOT=${LIVOX_SDK2_ROOT}"

# ── CMake cross-compile ────────────────────────────────────────────────────────
cd "${REPO_ROOT}"
export ARCH=aarch64
export PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig
export PKG_CONFIG_LIBDIR=/usr/lib/aarch64-linux-gnu/pkgconfig

rm -rf build_arm64
mkdir -p build_arm64 && cd build_arm64

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-aarch64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_DAEMON=ON \
    -DWITH_REPLAY=ON \
    -DWITH_CONVERT=OFF \
    -DWITH_SLAM=OFF \
    -DWITH_GIT_INFO=OFF \
    -DCMAKE_LIBRARY_PATH=/usr/lib/aarch64-linux-gnu \
    -DJPEG_TURBO_ROOT_DIR=/opt/libjpeg-turbo \
    -DINSTA_SDK_ROOT="${INSTA_SDK_ROOT}" \
    -DLIVOX_SDK2_ROOT="${LIVOX_SDK2_ROOT}" \
    -Dforce-git-info="${VERSION}"

make -j"$(nproc)"

# ── Package ────────────────────────────────────────────────────────────────────
cd "${REPO_ROOT}"
bash scripts/make_artifacts.sh arm64
mkdir -p dist
bash scripts/make_deb.sh "${VERSION}" artifacts dist arm64

echo ""
echo "===== Done ====="
ls -lh dist/*.deb
echo ""
echo "Install on Jetson:"
echo "  scp dist/*.deb hera@<JETSON_IP>:~/"
echo "  ssh hera@<JETSON_IP> 'echo 123456 | sudo -S dpkg -i ~/hera-daemon_${VERSION}_arm64.deb'"
