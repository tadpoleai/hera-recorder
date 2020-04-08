echo "Building for amd64"
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

# make client
# if [ $? -ne 0 ]; then
#     echo "build client error"
# fi

# Package artifacts
echo "Packaging artifacts/amd64"
mkdir -p ../artifacts/amd64/client
mkdir -p ../artifacts/amd64/lib
mkdir -p ../artifacts/amd64/bin
mkdir -p ../artifacts/amd64/include/common
mkdir -p ../artifacts/amd64/include/storage
mkdir -p ../artifacts/amd64/include/device
mkdir -p ../artifacts/amd64/script
mkdir -p ../artifacts/amd64/script/daemon
mkdir -p ../artifacts/amd64/shared
mkdir -p ../artifacts/amd64/manual
mkdir -p ../artifacts/amd64/manual/deploy
mkdir -p ../artifacts/amd64/manual/slam
mkdir -p ../artifacts/amd64/manual/convert

# Client
cp -r ../client/dist/* ../artifacts/amd64/client

# Headers
cp -r ../common/include/* ../artifacts/amd64/include/common
cp -r ../device/include/* ../artifacts/amd64/include/device
cp -r ../storage/include/* ../artifacts/amd64/include/storage

# Libraries
cp -r common/libhera-common.so ../artifacts/amd64/lib
cp -r device/libhera-device*.so ../artifacts/amd64/lib
cp -r storage/libhera-storage.so ../artifacts/amd64/lib

# Binaries
cp -r convert/hera-convert ../artifacts/amd64/bin
cp -r daemon/hera-daemon ../artifacts/amd64/bin
cp -r replay/hera-replay ../artifacts/amd64/bin
cp -r slam/bridge/hera-slam-bridge \
    slam/caller/hera-slam-caller-start \
    slam/caller/hera-slam-caller-stop \
    slam/result/hera-slam-result-test \
    ../artifacts/amd64/bin

# Script
cp -r ../daemon/script/* ../artifacts/amd64/script/daemon

# Shared
cp -r ../convert/config ../artifacts/amd64/shared
cp -r ../slam/carto ../artifacts/amd64/shared
cp -r ../cmake/FindHera.cmake ../artifacts/amd64/shared

# Manual
cp -r \
    ../README.md \
    ../INSTALLATION.md \
    ../artifacts/amd64/manual
cp -r \
    ../deploy/dev_etc.md \
    ../deploy/nginx.md \
    ../deploy/wlan.md \
    ../artifacts/amd64/manual/deploy
cp -r \
    ../slam/USAGE.md \
    ../artifacts/amd64/manual/slam
cp -r \
    ../convert/USAGE.md \
    ../artifacts/amd64/manual/convert

# Install Script
cp -r ../install_artifacts.sh ../artifacts/amd64
chmod +x ../artifacts/amd64/install_artifacts.sh
