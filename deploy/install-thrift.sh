#!/bin/bash

#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

set -e
set -v


VERSION="0.12.0"

sudo apt-get update
sudo apt-get install -y \
    libboost-dev \
    libboost-test-dev \
    libboost-program-options-dev \
    libboost-filesystem-dev \
    libboost-thread-dev \
    libevent-dev \
    automake \
    libtool \
    flex \
    bison \
    pkg-config \
    g++ \
    libssl-dev

if [ ! -e "thrift-${VERSION}.tar.gz" ]; then
    wget "http://mirrors.tuna.tsinghua.edu.cn/apache/thrift/${VERSION}/thrift-${VERSION}.tar.gz"
else
    echo "thrift-${VERSION} already downloaded."
fi

tar -zxf thrift-${VERSION}.tar.gz

cd thrift-${VERSION}

echo $(pwd)

./bootstrap.sh

./configure \
    --with-cpp \
    --without-qt4 --without-qt5 --without-c_glib --without-csharp \
    --without-java --without-erlang --without-nodejs --without-nodets --without-lua \
    --without-python --without-py3 --without-perl --without-php --without-php_extension --without-ruby \
    --without-haskell --without-go --without-rs --without-cl --without-haxe --without-d \
    --disable-tests --disable-tutorial \
    CFLAGS="-g0 -O3" CXXFLAGS="-g0 -O3"

make -j
sudo make install
