echo "Building for arm"

cd $(dirname "$0")/..

mkdir -p build_arm
cd build_arm

unset LD_LIBRARY_PATH
. /opt/s32v/environment-setup-aarch64-fsl-linux

if [ -z "$1" ]; then
    cmake .. -DWITH_DAEMON=ON -DWITH_CONVERT=0 -DWITH_REPLAY=1 \
        -DCMAKE_INSTALL_PREFIX=/opt/s32v/hera
else
    echo "git version is $1"
    cmake .. -DWITH_DAEMON=ON -DWITH_CONVERT=0 -DWITH_REPLAY=1 \
        -DCMAKE_INSTALL_PREFIX=/opt/s32v/hera -Dforce-git-info=$1
fi

if [ $? -ne 0 ]; then
    echo "cmake error"
fi

make -j4
if [ $? -ne 0 ]; then
    echo "build error"
fi
