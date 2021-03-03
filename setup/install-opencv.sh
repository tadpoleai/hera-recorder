#!/bin/bash

#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

set -e
set -v

VERSION="3.4.8"
TARFILE=opencv_${VERSION}_source_with_contrib_with_ippicv.tar.gz

if [ ! -f "./$TARFILE" ]; then
    if [ -f "/mnt/nfs/hdmap/software/$TARFILE" ]; then
        cp /mnt/nfs/hdmap/software/$TARFILE .
    else
        echo 'Cannot find opencv target file:'${TARFILE}
    fi
fi

tar -xvf ${TARFILE}

cd opencv
mkdir -p build
cd build

cmake -D CMAKE_BUILD_TYPE=Release \
    -D OPENCV_EXTRA_MODULES_PATH=$(pwd)/../opencv_contrib/modules/ \
    -D ENABLE_FAST_MATH=ON \
    -D WITH_IPP=ON \
    -D WITH_GTK=OFF \
    -D WITH_VTK=OFF \
    -D WITH_CUDA=OFF \
    -D BUILD_opencv_python2=ON \
    -D BUILD_opencv_python3=ON \
    -D BUILD_EXAMPLES=OFF \
    -D INSTALL_C_EXAMPLES=OFF \
    -D OPENCV_ENABLE_NONFREE=ON \
    ..
#
make -j 5

sudo make install

cd ../..

rm -rf opencv*