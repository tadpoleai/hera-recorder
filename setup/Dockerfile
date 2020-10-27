#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

# TAG: v0.8
# Everytime this file is modified, please add TAG by 0.1

FROM registry.newayz.com/autonomy/leta-localization_core:latest

WORKDIR /workspace/setup

# preparation
COPY ./sources.list /etc/apt/sources.list
RUN rm -rf /etc/apt/sources.list.d/* && \
    apt-get update && \
    apt-get install -y sudo wget apt-utils && \
    rm -rf /var/lib/apt/lists/*

# install thrift
COPY ./install-thrift.sh ./
RUN ./install-thrift.sh && \
    rm -rf /var/lib/apt/lists/* && \
    rm -rf thrift*

# install flycapture
COPY ./flycapture2-2.12.3.31-amd64* ./
COPY ./install-flycapture.sh ./
RUN ./install-flycapture.sh && \
    rm -rf ./flycapture2-2.12.3.31-amd64* && \
    rm -rf /var/lib/apt/lists/*

# install jpeg
COPY ./install-jpeg-turbo.sh ./
RUN ./install-jpeg-turbo.sh && \
    rm -rf /var/lib/apt/lists/*

# install pcl
COPY ./install-pcl.sh ./
RUN ./install-pcl.sh && \
    rm -rf /var/lib/apt/lists/*

# install npm
COPY ./install-npm.sh ./
RUN ./install-npm.sh && \
    rm -rf /var/lib/apt/lists/*

# install s32v-crossenv
COPY ./S32v_env.tar.gz ./
COPY ./install-s32v-crossenv.sh ./
RUN ./install-s32v-crossenv.sh && \
    rm -rf /var/lib/apt/lists/* && \
    rm -rf ./S32v_env.tar.gz ./S32v_env

# compile s32v-arm-turbojpeg
COPY ./install-jpeg-turbo-arm.sh ./
RUN ./install-jpeg-turbo-arm.sh && \
    rm -rf /var/lib/apt/lists/* && \
    rm -rf ./libjpeg-turbo*

# compile s32v-arm-thrift
COPY ./thrift-0.12.0-aarch64-lib.tar.gz ./
COPY ./install-thrift-arm.sh ./
RUN ./install-thrift-arm.sh && \
    rm -rf ./thrift-0.12.0-aarch64-lib.tar.gz

# install s32v-extra-depends
COPY ./s32vsal.tar.xz ./
COPY ./install-s32vsal.sh ./
RUN ./install-s32vsal.sh && \
    rm -rf ./s32vsal.tar.xz

# install s32v-libfast
COPY ./libfast.tar.xz ./
COPY ./install-s32v-libfast.sh ./
RUN ./install-s32v-libfast.sh && \
    rm -rf ./libfast.tar.xz

# install git
RUN apt-get update && \
    apt-get install -y sudo git && \
    rm -rf /var/lib/apt/lists/*

# install opencv
COPY ./opencv-3.4.12.tar.xz ./
COPY ./install-opencv.sh ./
RUN ./install-opencv.sh && \
    rm -rf ./opencv-3.4.12.tar.xz /var/lib/apt/lists/*

# source dynamic lib
RUN ldconfig

WORKDIR /workspace/hera-recorder
CMD /bin/bash
