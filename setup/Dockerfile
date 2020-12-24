#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

# TAG: v0.9
# Everytime this file is modified, please add TAG by 0.1

FROM ubuntu:bionic as builder

WORKDIR /workspace/setup

# preparation
COPY ./sources.list /etc/apt/sources.list
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y build-essential g++ && \
    apt-get install -y sudo wget curl dialog apt-utils unzip && \
    apt-get install gnupg -y cmake && \
    rm -rf /var/lib/apt/lists/*

# install thrift
COPY ./install-thrift.sh ./
RUN ./install-thrift.sh && \
    rm -rf /var/lib/apt/lists/* && \
    rm -rf thrift*

# install flycapture
COPY ./flycapture2-2.13.3.31-Ubuntu18.04_amd64* ./
COPY ./install-flycapture.sh ./
RUN ./install-flycapture.sh && \
    rm -rf ./flycapture2-2.13.3.31-Ubuntu18.04_amd64* && \
    rm -rf /var/lib/apt/lists/*

# install jpeg
COPY ./install-jpeg-turbo.sh ./
RUN ./install-jpeg-turbo.sh && \
    rm -rf /var/lib/apt/lists/*

# install pcl
RUN apt-get update && \
    apt-get install -y libpcl-*1.8 && \
    rm -rf /var/lib/apt/lists/*

# install npm
COPY ./install-npm.sh ./
RUN ./install-npm.sh && \
    rm -rf /var/lib/apt/lists/*

# install git
RUN apt-get update && \
    apt-get install -y sudo git && \
    rm -rf /var/lib/apt/lists/*

# install opencv
COPY ./opencv_3.4.8_source_with_contrib_with_ippicv.tar.gz ./
COPY ./install-opencv.sh ./
RUN ./install-opencv.sh && \
    rm -rf /var/lib/apt/lists/*

#instal libnfs
COPY ./libnfs-master.zip ./
COPY ./install-libnfs.sh ./
RUN ./install-libnfs.sh && \
    rm -rf ./libnfs-master*

COPY ./install_ros.sh ./
RUN ./install_ros.sh && \
    rm -rf /var/lib/apt/lists/*

# source dynamic lib
RUN ldconfig

WORKDIR /workspace/hera-recorder
CMD /bin/bash