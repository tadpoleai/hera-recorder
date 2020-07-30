#!/bin/bash

#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

set -e
set -v

INSTALL_PREFIX="/opt/s32v/"

mkdir -p ${INSTALL_PREFIX}

LOCAL_TAR_GZ="thrift-0.12.0-aarch64-lib.tar.gz"
NFS_TAR_GZ="/mnt/nfs/hdmap/software/s32v234/${LOCAL_TAR_GZ}"

if [ ! -f "./${LOCAL_TAR_GZ}" ]; then
    if [ -f "${NFS_TAR_GZ}" ]; then
        cp ${NFS_TAR_GZ} .
        echo "Copying from nfs"
    else
        exit -1
    fi
fi

sudo tar -xvf ${LOCAL_TAR_GZ} -C ${INSTALL_PREFIX}
