mkdir -p build
cd build

cmake .. -Dwith-all=1 -Dwith-driver-all=1
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
    slam/hera-slam \
    ../daemon/script \
    ../client/dist \
    ../README.md \
    ../INSTALLATION.md \
    ../client/USAGE.md \
    ../convert/USAGE.md \
    ../deploy/*.md \
    ../jenkins_deploy.sh
