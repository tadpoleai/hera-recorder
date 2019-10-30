VERSION="2.0.3"
ARCH="amd64"
NAME="libjpeg-turbo-official_${VERSION}_${ARCH}.deb"
URL="https://iweb.dl.sourceforge.net/project/libjpeg-turbo/${VERSION}/${NAME}"

if [ ! -e "${NAME}" ]; then
    wget "${URL}"
else
    echo "libjpeg-turbo-${VERSION} already downloaded."
fi

sudo dpkg -i ${NAME}
