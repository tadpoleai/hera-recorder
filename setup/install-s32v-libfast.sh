#!/bin/bash

#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

set -e
set -v

S32VSAL_INSTALL_PATH="/opt/s32v/"
LOCAL_LIB="libfast"
LOCAL_TAR_GZ="${LOCAL_LIB}.tar.xz"
NFS_TAR_GZ="/mnt/nfs/hdmap/software/s32v234/${LOCAL_TAR_GZ}"

if [ ! -f "./${LOCAL_TAR_GZ}" ]; then
    if [ -f "${NFS_TAR_GZ}" ]; then
        cp ${NFS_TAR_GZ} .
        echo "Copying from nfs"
    else
        exit -1
    fi
fi

sudo mkdir -p ${S32VSAL_INSTALL_PATH}
sudo tar -xvf ${LOCAL_TAR_GZ} 
sudo cp ${LOCAL_LIB}/lib* \
    ${S32VSAL_INSTALL_PATH}/sysroots/aarch64-fsl-linux/usr/lib
