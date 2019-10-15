VERSION="0.12.0"

sudo apt install -y libboost-dev libboost-test-dev libboost-program-options-dev libboost-filesystem-dev libboost-thread-dev libevent-dev automake libtool flex bison pkg-config g++ libssl-dev

if [ ! -e "thrift-${VERSION}.tar.gz" ]; then
    wget "http://mirrors.tuna.tsinghua.edu.cn/apache/thrift/${VERSION}/thrift-${VERSION}.tar.gz"
else
    echo "thrift-${VERSION} already downloaded."
fi

#tar -zxf thrift-${VERSION}.tar.gz 
cd thrift-${VERSION}
echo `pwd`
./bootstrap.sh
./configure --without-java --without-php --without-qt4 --without-qt5
make
sudo make install
