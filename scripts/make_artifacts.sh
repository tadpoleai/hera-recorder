echo "Making Artifacts"

cd $(dirname "$0")/..

mkdir -p artifacts/client
mkdir -p artifacts/manual
mkdir -p artifacts/header/hera/common
mkdir -p artifacts/header/hera/storage
mkdir -p artifacts/header/hera/device
mkdir -p artifacts/header/hera/convert
mkdir -p artifacts/script
mkdir -p artifacts/script/daemon
mkdir -p artifacts/share

# Client
cp -r build_client/* artifacts/client

# Header
cp -r common/include/* artifacts/header/hera/common
cp -r device/include/* artifacts/header/hera/device
cp -r storage/include/* artifacts/header/hera/storage
cp -r convert/include/* artifacts/header/hera/convert

# archs=("amd64" "arm")
archs=("amd64")
arch_index=0
while ((arch_index < 1)); do
    arch=${archs[arch_index]}

    mkdir -p artifacts/bin/$arch/
    mkdir -p artifacts/lib/$arch/
    mkdir -p artifacts/plugin/$arch/base
    mkdir -p artifacts/plugin/$arch/driver

    # Binaries
    if (($arch_index == 0)); then
        cp -r build_$arch/convert/hera-* artifacts/bin/$arch
        cp -r build_$arch/convert/libhera-convert-ros-message.so artifacts/lib/$arch
        cp -r \
            build_$arch/slam/bridge/hera-slam-* \
            build_$arch/slam/caller/hera-slam-* \
            build_$arch/slam/result/hera-slam-* \
            artifacts/bin/$arch
    fi

    cp -r \
        build_$arch/daemon/hera-* \
        build_$arch/device/hera-* \
        build_$arch/replay/hera-* \
        build_$arch/storage/hera-* \
        artifacts/bin/$arch

    # Libraries
    cp -r \
        build_$arch/common/libhera-common.so \
        build_$arch/device/libhera-device.so \
        build_$arch/storage/libhera-storage.so \
        artifacts/lib/$arch

    # Plugins
    cp -r \
        build_$arch/device/base/libhera-device-plugin-*-base.so \
        artifacts/plugin/$arch/base
    cp -r \
        build_$arch/device/driver/libhera-device-plugin-*-driver.so \
        artifacts/plugin/$arch/driver

    ((arch_index++))
done

# Script
cp -r daemon/script/* artifacts/script/daemon

# Shared
cp -r convert/config artifacts/share
cp -r daemon/config artifacts/share
cp -r slam/carto artifacts/share
cp -r cmake/export/* artifacts/share

# Manual
cp -r README.md artifacts
cp -r manual/* artifacts/manual

# Install Script
cp -r scripts/install_artifacts.sh artifacts/
chmod +x artifacts/install_artifacts.sh
