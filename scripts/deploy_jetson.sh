#!/bin/bash
# End-to-end deploy script: rsync source → build on Jetson → package .deb → install → verify.
#
# Usage:
#   deploy_jetson.sh [version]
#
# version defaults to the latest git tag (e.g. v1.2.3 → 1.2.3).
# If no tag exists, falls back to "0.0.0-<short-sha>".
set -euo pipefail

JETSON="yuancaimaiyi@192.168.110.211"
REMOTE_DIR="~/hera-build"

# ── Version ───────────────────────────────────────────────────────────
if [ -n "${1:-}" ]; then
    VERSION="$1"
else
    RAW=$(git describe --tags --abbrev=0 2>/dev/null || true)
    if [ -n "${RAW}" ]; then
        VERSION="${RAW#v}"
    else
        VERSION="0.0.0-$(git rev-parse --short HEAD)"
    fi
fi
echo "[deploy] version=${VERSION}  target=${JETSON}:${REMOTE_DIR}"

# ── Step 1: Sync source ───────────────────────────────────────────────
echo ""
echo "=== 1/4  Syncing source to Jetson ==="
rsync -az --info=progress2 \
    --exclude='.git' \
    --exclude='/build_amd64' \
    --exclude='/build_arm64' \
    --exclude='/build_arm' \
    --exclude='/artifacts' \
    --exclude='/artifacts_arm64' \
    --exclude='/dist' \
    --exclude='/cloud' \
    . "${JETSON}:${REMOTE_DIR}/"

# ── Step 2: Build on Jetson ───────────────────────────────────────────
echo ""
echo "=== 2/4  Building on Jetson (arm64 native) ==="
ssh "${JETSON}" bash -s "${VERSION}" << 'REMOTE_BUILD'
set -euo pipefail
VERSION="$1"
cd "$HOME/hera-build"
bash scripts/build_arm64.sh "${VERSION}"
REMOTE_BUILD

# ── Step 3: Collect artifacts and build .deb on Jetson ───────────────
echo ""
echo "=== 3/4  Packaging .deb on Jetson ==="
ssh "${JETSON}" bash -s "${VERSION}" << 'REMOTE_PACKAGE'
set -euo pipefail
VERSION="$1"
cd "$HOME/hera-build"

ARCH="arm64"
BUILD_DIR="build_arm64"
ARTS="artifacts_arm64"

mkdir -p "${ARTS}/bin/${ARCH}"
mkdir -p "${ARTS}/lib/${ARCH}"
mkdir -p "${ARTS}/plugin/${ARCH}/base"
mkdir -p "${ARTS}/plugin/${ARCH}/driver"
mkdir -p "${ARTS}/plugin/${ARCH}/upload"
mkdir -p "${ARTS}/script/daemon"

# Binaries
cp ${BUILD_DIR}/daemon/hera-daemon          "${ARTS}/bin/${ARCH}/"
cp ${BUILD_DIR}/daemon/hera-daemon-finder   "${ARTS}/bin/${ARCH}/"
cp ${BUILD_DIR}/storage/hera-storage-tool   "${ARTS}/bin/${ARCH}/" 2>/dev/null || true
cp ${BUILD_DIR}/storage/hera-storage-extract-* "${ARTS}/bin/${ARCH}/" 2>/dev/null || true
cp ${BUILD_DIR}/replay/hera-replay          "${ARTS}/bin/${ARCH}/" 2>/dev/null || true

# Libraries
cp ${BUILD_DIR}/common/libhera-common.so    "${ARTS}/lib/${ARCH}/"
cp ${BUILD_DIR}/device/libhera-device.so    "${ARTS}/lib/${ARCH}/"
cp ${BUILD_DIR}/storage/libhera-storage.so  "${ARTS}/lib/${ARCH}/"

# Plugins
find "${BUILD_DIR}/device/base"   -name '*-base.so'   -exec cp {} "${ARTS}/plugin/${ARCH}/base/"   \; 2>/dev/null || true
find "${BUILD_DIR}/device/driver" -name '*-driver.so' -exec cp {} "${ARTS}/plugin/${ARCH}/driver/" \; 2>/dev/null || true
find "${BUILD_DIR}/storage/upload" -name '*.so'       -exec cp {} "${ARTS}/plugin/${ARCH}/upload/" \; 2>/dev/null || true

# Systemd + config
cp daemon/script/hera-daemon.service "${ARTS}/script/daemon/"
cp daemon/script/udiskie.service     "${ARTS}/script/daemon/" 2>/dev/null || true
cp daemon/config/daemon.conf         "${ARTS}/script/daemon/"

bash scripts/make_deb.sh "${VERSION}" "${ARTS}" dist "${ARCH}"
ls -lh dist/*.deb
REMOTE_PACKAGE

# ── Step 4: Install and verify ────────────────────────────────────────
echo ""
echo "=== 4/4  Installing .deb and verifying service ==="
if ssh "${JETSON}" bash -s "${VERSION}" << 'REMOTE_INSTALL'
set -euo pipefail
VERSION="$1"
DEB="$HOME/hera-build/dist/hera-daemon_${VERSION}_arm64.deb"

echo "Installing: ${DEB}"
# sudo -n: non-interactive; requires NOPASSWD in sudoers.
# One-time setup on Jetson:
#   sudo visudo -f /etc/sudoers.d/hera-deploy
#   yuancaimaiyi ALL=(ALL) NOPASSWD: /usr/bin/dpkg
if ! sudo -n dpkg -i "${DEB}" 2>/dev/null; then
    echo ""
    echo "[deploy] sudo requires a password. Run this manually on the Jetson:"
    echo "  sudo dpkg -i ${DEB}"
    exit 1
fi

echo ""
echo "--- systemctl status hera-daemon ---"
systemctl status hera-daemon --no-pager -l || true

echo ""
echo "--- binary version ---"
strings /usr/local/bin/hera-daemon | grep -i "git\|built" | head -3 || true
REMOTE_INSTALL
then
    echo ""
    echo "[deploy] Done. hera-daemon ${VERSION} deployed to ${JETSON}"
else
    echo ""
    echo "[deploy] Build+package succeeded. Manual install needed:"
    echo "  ssh ${JETSON} 'sudo dpkg -i ~/hera-build/dist/hera-daemon_${VERSION}_arm64.deb'"
fi
