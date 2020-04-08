mkdir -p build
cd build

if [ -z "$1" ]; then
    cmake .. -Dwith-all=1 -Dwith-driver-all=0
else
    echo "git version is $1"
    cmake .. -Dwith-all=1 -Dwith-driver-all=1 -Dforce-git-info=$1
fi

if [ $? -ne 0 ]; then
    echo "cmake error"
fi

make -j
if [ $? -ne 0 ]; then
    echo "build error"
fi

make client
if [ $? -ne 0 ]; then
    echo "build client error"
fi

# Package artifacts
echo "Packaging artifacts"
rm -rf artifacts
mkdir -p artifacts/client
mkdir -p artifacts/lib
mkdir -p artifacts/bin
mkdir -p artifacts/include/common
mkdir -p artifacts/include/storage
mkdir -p artifacts/include/device
mkdir -p artifacts/script
mkdir -p artifacts/script/daemon
mkdir -p artifacts/shared
mkdir -p artifacts/manual
mkdir -p artifacts/manual/deploy
mkdir -p artifacts/manual/slam
mkdir -p artifacts/manual/convert

# Client
cp -r ../client/dist/* artifacts/client

# Headers
cp -r ../common/include/* artifacts/include/common
cp -r ../device/include/* artifacts/include/device
cp -r ../storage/include/* artifacts/include/storage

# Libraries
cp -r common/libhera-common.so artifacts/lib
cp -r device/libhera-device*.so artifacts/lib
cp -r storage/libhera-storage.so artifacts/lib

# Binaries
cp -r convert/hera-convert artifacts/bin
cp -r daemon/hera-daemon artifacts/bin
cp -r replay/hera-replay artifacts/bin
cp -r slam/bridge/hera-slam-bridge \
    slam/caller/hera-slam-caller-start \
    slam/caller/hera-slam-caller-stop \
    slam/result/hera-slam-result-test \
    artifacts/bin

# Script
cp -r ../daemon/script/* artifacts/script/daemon

# Shared
cp -r ../convert/config artifacts/shared
cp -r ../slam/carto artifacts/shared
cp -r ../cmake/FindHera.cmake artifacts/shared

# Manual
cp -r \
    ../README.md \
    ../INSTALLATION.md \
    artifacts/manual
cp -r \
    ../deploy/dev_etc.md \
    ../deploy/nginx.md \
    ../deploy/wlan.md \
    artifacts/manual/deploy
cp -r \
    ../slam/USAGE.md \
    ../convert/USAGE.md \
    artifacts/manual/slam
cp -r \
    ../convert/USAGE.md \
    artifacts/manual/convert

# Install Script
cp -r ../install_artifacts.sh artifacts
chmod +x artifacts/install_artifacts.sh

tar -czvf artifacts.tar.gz artifacts
