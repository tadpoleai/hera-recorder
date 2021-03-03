#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

# TAG: v0.10
# Everytime this file is modified, please add TAG by 0.1

FROM ubuntu:bionic as builder

WORKDIR /workspace/setup

# preparation
# COPY ./sources.list /etc/apt/sources.list
ARG DEBIAN_FRONTEND=noninteractive
COPY ./docker_pkgs.sh ./
COPY ./docker_pkgs_install.sh ./
RUN ./docker_pkgs.sh && \
    ./docker_pkgs_install.sh && \
    rm -rf /var/lib/apt/lists/*

# install opencv
COPY ./opencv_3.4.8_source_with_contrib_with_ippicv.tar.gz ./
COPY ./install-opencv.sh ./
RUN ./install-opencv.sh

# install thrift
COPY ./install-thrift.sh ./
RUN ./install-thrift.sh

#instal libnfs
COPY ./libnfs-master.zip ./
COPY ./install-libnfs.sh ./
RUN ./install-libnfs.sh

# install flycapture
COPY ./flycapture2-2.13.3.31-Ubuntu18.04_amd64* ./
COPY ./install-flycapture.sh ./
RUN ./install-flycapture.sh

# install jpeg
COPY ./install-jpeg-turbo.sh ./
RUN ./install-jpeg-turbo.sh

# install npm
COPY ./install-npm.sh ./
RUN ./install-npm.sh

# source dynamic lib
RUN ldconfig

RUN rm -rf /workspace/setup

WORKDIR /workspace/hera-recorder
CMD /bin/bash