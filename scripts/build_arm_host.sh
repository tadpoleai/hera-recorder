echo "Building for arm"
mkdir -p build-arm
cd build-arm

unset LD_LIBRARY_PATH
. /opt/s32v/environment-setup-aarch64-fsl-linux

if [ -z "$1" ]; then
    cmake .. -Dwith-driver-arm=1 -Dwith-daemon=1 -Dwith-client=0 -Dwith-convert=0 -Dwith-replay=1 \
        -Dtarget-arm=1 -DCMAKE_INSTALL_PREFIX=/opt/s32v/hera
else
    echo "git version is $1"
    cmake .. -Dwith-driver-arm=1 -Dwith-daemon=1 -Dwith-client=0 -Dwith-convert=0 -Dwith-replay=1 \
        -Dtarget-arm=1 -DCMAKE_INSTALL_PREFIX=/opt/s32v/hera -Dforce-git-info=$1
fi

if [ $? -ne 0 ]; then
    echo "cmake error"
fi

make -j
if [ $? -ne 0 ]; then
    echo "build error"
fi

# Package artifacts/arm
echo "Packaging artifacts/arm/"
mkdir -p ../artifacts/arm/client
mkdir -p ../artifacts/arm/lib
mkdir -p ../artifacts/arm/bin
mkdir -p ../artifacts/arm/include/common
mkdir -p ../artifacts/arm/include/storage
mkdir -p ../artifacts/arm/include/device
mkdir -p ../artifacts/arm/manual
mkdir -p ../artifacts/arm/script
mkdir -p ../artifacts/arm/script/daemon
mkdir -p ../artifacts/arm/shared

# Client
cp -r ../client/dist/* ../artifacts/arm/client

# Headers
cp -r ../common/include/* ../artifacts/arm/include/common
cp -r ../device/include/* ../artifacts/arm/include/device
cp -r ../storage/include/* ../artifacts/arm/include/storage

# Libraries
cp -r common/libhera-common.so ../artifacts/arm/lib
cp -r device/libhera-device*.so ../artifacts/arm/lib
cp -r storage/libhera-storage.so ../artifacts/arm/lib

# Copy Dependency
cp -rL /opt/s32v/thrift/lib/libthrift.so ../artifacts/arm/lib

# Binaries
cp -r daemon/hera-daemon ../artifacts/arm/bin
cp -r device/hera-device*test ../artifacts/arm/bin
cp -r replay/hera-replay ../artifacts/arm/bin

# Shared
cp -r ../cmake/FindHera.cmake ../artifacts/arm/shared

# Script
cp -r ../daemon/script/* ../artifacts/arm/script/daemon

# Manual
cp -r \
    ../README.md \
    ../INSTALLATION.md \
    ../artifacts/arm/manual

# Install Script
cp -r ../scripts/install_artifacts_arm_host.sh ../artifacts/arm
cp -r ../scripts/install_artifacts_arm_target.sh ../artifacts/arm
chmod +x ../artifacts/arm/install_artifacts_arm_host.sh
chmod +x ../artifacts/arm/install_artifacts_arm_target.sh
