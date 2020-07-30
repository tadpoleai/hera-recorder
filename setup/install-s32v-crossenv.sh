#!/bin/bash

#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

set -e
set -v

S32V_INSTALL_PATH="/opt/s32v/"
LOCAL_TAR_GZ="S32v_env.tar.gz"
NFS_TAR_GZ="/mnt/nfs/hdmap/software/s32v234/${LOCAL_TAR_GZ}"

if [ ! -f "./${LOCAL_TAR_GZ}" ]; then
    if [ -f "${NFS_TAR_GZ}" ]; then
        cp ${NFS_TAR_GZ} .
        echo "Copying from nfs"
    else
        exit -1
    fi
fi

tar -xvf ${LOCAL_TAR_GZ}

sudo S32v_env/s32v_toolchain.sh -y -d ${S32V_INSTALL_PATH} -D
