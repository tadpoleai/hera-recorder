#!/bin/bash

#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

set -e
set -v

unzip libnfs-master.zip
cd libnfs-master

./bootstrap

./configure

sed -i '1s/(WARN_CFLAGS)/(WARN_CFLAGS) -fPIC/' lib/Makefile.am

make -j 3

sudo make install