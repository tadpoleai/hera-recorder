#!/bin/bash
# Build a .deb package for hera-daemon from the artifacts directory.
#
# Usage:
#   make_deb.sh <version> [artifacts_dir] [output_dir] [arch]
#
# arch defaults to the host dpkg architecture (amd64 or arm64).
#
# Example:
#   make_deb.sh 1.2.3
#   make_deb.sh 1.2.3 ./artifacts ./dist arm64
set -euo pipefail

VERSION="${1:?Usage: make_deb.sh <version> [artifacts_dir] [output_dir] [arch]}"
ARTIFACTS_DIR="${2:-$(dirname "$0")/../artifacts}"
OUTPUT_DIR="${3:-$(dirname "$0")/../dist}"
ARCH="${4:-$(dpkg --print-architecture)}"
PKG_NAME="hera-daemon"

ARTIFACTS_DIR="$(realpath "${ARTIFACTS_DIR}")"
OUTPUT_DIR="$(realpath "${OUTPUT_DIR}")"

echo "[make_deb] version=${VERSION}  artifacts=${ARTIFACTS_DIR}  output=${OUTPUT_DIR}"

# ── Staging area ──────────────────────────────────────────────────────
STAGING=$(mktemp -d)
trap 'rm -rf "${STAGING}"' EXIT

# ── Directory layout ──────────────────────────────────────────────────
mkdir -p "${STAGING}/DEBIAN"
mkdir -p "${STAGING}/usr/local/bin"
mkdir -p "${STAGING}/usr/local/lib/hera/plugin/base"
mkdir -p "${STAGING}/usr/local/lib/hera/plugin/driver"
mkdir -p "${STAGING}/usr/local/lib/hera/plugin/upload"
mkdir -p "${STAGING}/lib/systemd/system"
mkdir -p "${STAGING}/etc"
mkdir -p "${STAGING}/var/hera/data"
mkdir -p "${STAGING}/var/hera/logs"

# ── Binaries ──────────────────────────────────────────────────────────
install -m 755 "${ARTIFACTS_DIR}/bin/${ARCH}/"* \
    "${STAGING}/usr/local/bin/"

# ── Shared libraries ──────────────────────────────────────────────────
install -m 755 "${ARTIFACTS_DIR}/lib/${ARCH}/"*.so \
    "${STAGING}/usr/local/lib/"

# ── Plugins ───────────────────────────────────────────────────────────
install -m 755 "${ARTIFACTS_DIR}/plugin/${ARCH}/base/"*.so \
    "${STAGING}/usr/local/lib/hera/plugin/base/"
install -m 755 "${ARTIFACTS_DIR}/plugin/${ARCH}/driver/"*.so \
    "${STAGING}/usr/local/lib/hera/plugin/driver/"
install -m 755 "${ARTIFACTS_DIR}/plugin/${ARCH}/upload/"*.so \
    "${STAGING}/usr/local/lib/hera/plugin/upload/"

# ── Systemd units ─────────────────────────────────────────────────────
install -m 644 "${ARTIFACTS_DIR}/script/daemon/hera-daemon.service" \
    "${STAGING}/lib/systemd/system/"
install -m 644 "${ARTIFACTS_DIR}/script/daemon/udiskie.service" \
    "${STAGING}/lib/systemd/system/"

# ── Default config ────────────────────────────────────────────────────
# Listed in conffiles so dpkg prompts before overwriting on upgrade.
install -m 644 "${ARTIFACTS_DIR}/script/daemon/daemon.conf" \
    "${STAGING}/etc/hera.conf"

# Livox Mid360 SDK config (template — edit host_ip to match Jetson's interface IP)
if [ -f "${ARTIFACTS_DIR}/script/daemon/mid360_config.json" ]; then
    install -m 644 "${ARTIFACTS_DIR}/script/daemon/mid360_config.json" \
        "${STAGING}/etc/mid360_config.json"
fi

# ── DEBIAN/conffiles ──────────────────────────────────────────────────
cat > "${STAGING}/DEBIAN/conffiles" <<'EOF'
/etc/hera.conf
/etc/mid360_config.json
EOF

# ── DEBIAN/control ────────────────────────────────────────────────────
INSTALLED_SIZE=$(du -sk "${STAGING}" | awk '{print $1}')

cat > "${STAGING}/DEBIAN/control" <<EOF
Package: ${PKG_NAME}
Version: ${VERSION}
Architecture: ${ARCH}
Maintainer: Wayz.ai <dev@wayz.ai>
Installed-Size: ${INSTALLED_SIZE}
Depends: libc6 (>= 2.17), libstdc++6 (>= 6), libusb-1.0-0
Description: Hera sensor data collection daemon
 Multi-sensor data collection daemon supporting LiDAR, camera,
 and IMU devices. Managed by systemd.
Section: misc
Priority: optional
EOF

# ── DEBIAN/postinst ───────────────────────────────────────────────────
cat > "${STAGING}/DEBIAN/postinst" <<'EOF'
#!/bin/bash
set -e
case "$1" in
    configure)
        mkdir -p /var/hera/data /var/hera/logs
        ldconfig
        systemctl daemon-reload
        systemctl enable hera-daemon.service
        systemctl enable udiskie.service || true
        systemctl restart hera-daemon.service || true
        ;;
esac
EOF
chmod 755 "${STAGING}/DEBIAN/postinst"

# ── DEBIAN/prerm ──────────────────────────────────────────────────────
cat > "${STAGING}/DEBIAN/prerm" <<'EOF'
#!/bin/bash
set -e
case "$1" in
    remove|upgrade|deconfigure)
        if systemctl is-active --quiet hera-daemon.service 2>/dev/null; then
            systemctl stop hera-daemon.service || true
        fi
        systemctl disable hera-daemon.service || true
        ;;
esac
EOF
chmod 755 "${STAGING}/DEBIAN/prerm"

# ── Build .deb ────────────────────────────────────────────────────────
mkdir -p "${OUTPUT_DIR}"
DEB_FILE="${OUTPUT_DIR}/${PKG_NAME}_${VERSION}_${ARCH}.deb"
dpkg-deb --build --root-owner-group "${STAGING}" "${DEB_FILE}"

echo "[make_deb] created: ${DEB_FILE}"
