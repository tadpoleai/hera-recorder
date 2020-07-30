#!/bin/bash

#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

set -e
set -v

S32VSAL_INSTALL_PATH="/opt/s32v/"
LOCAL_TAR_GZ="s32vsal.tar.xz"
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
sudo tar -xvf ${LOCAL_TAR_GZ} -C ${S32VSAL_INSTALL_PATH}
sudo cp ${S32VSAL_INSTALL_PATH}/s32vsal/lib/3rd/import/lib/libf* \
    ${S32VSAL_INSTALL_PATH}/sysroots/aarch64-fsl-linux/usr/lib
