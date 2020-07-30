echo "Building for amd64"

cd $(dirname "$0")/..

mkdir -p build_amd64
cd build_amd64

if [ -z "$1" ]; then
    cmake .. -Dwith-all=1
else
    echo "git version is $1"
    cmake .. -Dwith-all=1 -Dforce-git-info=$1
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
