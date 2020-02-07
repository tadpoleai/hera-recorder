mkdir -p build
cd build

if [ -z "$1" ]; then
    cmake .. -Dwith-all=1 -Dwith-driver-all=1
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

echo "Packaging artifacts"
tar -czvf \
    install.tar.gz \
    daemon/hera-daemon \
    convert/hera-convert \
    replay/hera-replay \
    slam/bridge/hera-slam-bridge \
    slam/caller/hera-slam-caller-start \
    slam/caller/hera-slam-caller-stop \
    ../slam/carto \
    ../daemon/script \
    ../client/dist \
    ../convert/config \
    ../README.md \
    ../INSTALLATION.md \
    ../client/USAGE.md \
    ../convert/USAGE.md \
    ../slam/USAGE.md \
    ../deploy/*.md \
    ../jenkins_deploy.sh
