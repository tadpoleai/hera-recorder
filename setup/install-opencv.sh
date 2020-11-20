#!/bin/bash

#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

set -e
set -v

VERSION="3.4.12"

tar -xf opencv-${VERSION}.tar.xz

cd opencv-${VERSION}
mkdir -p build
cd build

cmake -D CMAKE_BUILD_TYPE=Release \
    -D ENABLE_FAST_MATH=ON \
    -D WITH_IPP=OFF \
    -D WITH_GTK=OFF \
    -D WITH_VTK=OFF \
    -D WITH_CUDA=OFF \
    -D BUILD_opencv_python2=OFF \
    -D BUILD_opencv_python3=OFF \
    -D BUILD_EXAMPLES=OFF \
    -D INSTALL_C_EXAMPLES=OFF \
    -D OPENCV_EXTRA_MODULE_PATH=$(pwd)/../opencv_contrib/modules/ \
    ..

make -j 3

sudo make install

cd ../..

rm -rf opencv-*
