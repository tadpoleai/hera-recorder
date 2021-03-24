echo "Building for amd64"

cd $(dirname "$0")/..

mkdir -p build_amd64
cd build_amd64

if [ -z "$1" ]; then
    cmake .. -DWITH_ALL=ON
else
    echo "git version is $1"
    cmake .. -DWITH_ALL=ON -Dforce-git-info=$1
fi

if [ $? -ne 0 ]; then
    echo "cmake error"
fi

make -j4
if [ $? -ne 0 ]; then
    echo "build error"
fi
