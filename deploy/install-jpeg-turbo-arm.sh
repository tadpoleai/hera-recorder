VERSION="2.0.4"
NAME="libjpeg-turbo-${VERSION}.tar.gz"
URL="https://iweb.dl.sourceforge.net/project/libjpeg-turbo/${VERSION}/${NAME}"

if [ ! -e "${NAME}" ]; then
    wget "${URL}"
else
    echo "libjpeg-turbo-${VERSION} already downloaded."
fi

tar -xvf ${NAME}

cd libjpeg-turbo-2.0.4
mkdir build
cd build

unset LD_LIBRARY_PATH
. /opt/s32v/environment-setup-aarch64-fsl-linux
cmake .. -DCMAKE_INSTALL_PREFIX=/opt/s32v/extra-depends/3rd/libjpeg-turbo
make -j
sudo mkdir -p /opt/s32v/extra-depends/3rd/libjpeg-turbo
sudo make install

cd ../..
rm $NAME
