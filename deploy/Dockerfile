#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

# TAG: v0.2
# Everytime this file is modified, please add TAG by 0.1

FROM registry.newayz.com/autonomy/leta-localization_core:latest

WORKDIR /workspace/setup

# preparation
COPY ./sources.list /etc/apt/sources.list
RUN apt-get update && \
    apt-get install -y sudo wget && \
    rm -rf /var/lib/apt/lists/*

# install thrift
COPY ./install-thrift.sh ./
RUN ./install-thrift.sh && \
    rm -rf /var/lib/apt/lists/*

# install flycapture
COPY ./install-flycapture.sh ./
RUN ./install-flycapture.sh && \
    rm -rf flycapture2-2.12.3.31-amd64 && \
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

# source dynamic lib
RUN ldconfig

WORKDIR /workspace/hera-recorder
CMD /bin/bash
