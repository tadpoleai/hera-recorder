# =============================================================================
# Stage 1: Base environment with ROS Melodic and all system dependencies
# =============================================================================
FROM ubuntu:bionic AS base

ARG DEBIAN_FRONTEND=noninteractive

# Use Aliyun mirrors for faster downloads inside China
RUN sed -i 's|http://archive.ubuntu.com|http://mirrors.aliyun.com|g' /etc/apt/sources.list && \
    sed -i 's|http://security.ubuntu.com|http://mirrors.aliyun.com|g' /etc/apt/sources.list

RUN apt-get update && apt-get install -y \
    build-essential g++ cmake git wget curl unzip \
    lsb-release gnupg \
    libboost-dev libboost-filesystem-dev libboost-thread-dev \
    libboost-program-options-dev libboost-test-dev \
    libevent-dev libconfig++-dev \
    libpcl-dev libpcl-common1.8 \
    automake libtool flex bison pkg-config libssl-dev \
    python3 python3-pip python3-dev \
    && rm -rf /var/lib/apt/lists/*

# Install ROS Melodic
RUN sh -c 'echo "deb http://mirrors.aliyun.com/ros/ubuntu bionic main" > /etc/apt/sources.list.d/ros-latest.list' && \
    apt-key adv --keyserver keyserver.ubuntu.com --recv-keys F42ED6FBAB17C654 && \
    apt-get update && apt-get install -y \
        ros-melodic-ros-base \
        ros-melodic-rosbag \
        ros-melodic-pcl-ros \
        ros-melodic-eigen-conversions \
        ros-melodic-tf2 \
    && rm -rf /var/lib/apt/lists/*

# =============================================================================
# Stage 2: Build Thrift and TurboJPEG
# =============================================================================
FROM base AS deps-builder

WORKDIR /build

# Build Thrift 0.12.0
RUN wget -q "http://mirrors.tuna.tsinghua.edu.cn/apache/thrift/0.12.0/thrift-0.12.0.tar.gz" && \
    tar -zxf thrift-0.12.0.tar.gz && \
    cd thrift-0.12.0 && \
    ./bootstrap.sh && \
    ./configure \
        --with-cpp \
        --without-qt4 --without-qt5 --without-c_glib --without-csharp \
        --without-java --without-erlang --without-nodejs --without-lua \
        --without-python --without-py3 --without-perl --without-php \
        --without-ruby --without-haskell --without-go --without-rs \
        --without-swift --disable-tests --disable-tutorial \
        CFLAGS="-g -O2" CXXFLAGS="-g -O2" && \
    make -j$(nproc) && make install && \
    cp /usr/local/lib/libthrift-0.12.0.so /usr/lib/libthrift-0.12.0.so && \
    ldconfig && \
    cd /build && rm -rf thrift*

# Install TurboJPEG 2.0.4
RUN wget -q "https://iweb.dl.sourceforge.net/project/libjpeg-turbo/2.0.4/libjpeg-turbo-official_2.0.4_amd64.deb" && \
    dpkg -i libjpeg-turbo-official_2.0.4_amd64.deb && \
    cp /opt/libjpeg-turbo/lib64/libturbojpeg.so /usr/lib/x86_64-linux-gnu/libturbojpeg.so && \
    ln -sf /opt/libjpeg-turbo/lib64/libturbojpeg.so /usr/lib/x86_64-linux-gnu/libturbojpeg.so.0 && \
    ldconfig && \
    rm -f libjpeg-turbo-official_2.0.4_amd64.deb

# Build OpenCV 3.4.8 (without contrib, without ippicv for faster build)
RUN wget -q "https://github.com/opencv/opencv/archive/3.4.8.tar.gz" -O opencv.tar.gz && \
    tar -xf opencv.tar.gz && \
    mkdir -p opencv-3.4.8/build && \
    cd opencv-3.4.8/build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DWITH_IPP=OFF \
          -DBUILD_SHARED_LIBS=ON \
          -DBUILD_opencv_apps=OFF \
          -DBUILD_TESTS=OFF \
          -DBUILD_PERF_TESTS=OFF \
          -DBUILD_DOCS=OFF \
          -DBUILD_EXAMPLES=OFF \
          -DWITH_CUDA=OFF \
          -DWITH_OPENCL=OFF \
          .. && \
    make -j$(nproc) && make install && ldconfig && \
    cd /build && rm -rf opencv.tar.gz opencv-3.4.8

# =============================================================================
# Stage 3: Build hera-convert
# =============================================================================
FROM deps-builder AS hera-builder

WORKDIR /workspace

# Copy source code
COPY . /workspace/hera-recorder

# Build with only the convert component
RUN . /opt/ros/melodic/setup.sh && \
    mkdir -p /workspace/hera-recorder/build_cloud && \
    cd /workspace/hera-recorder/build_cloud && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DWITH_CONVERT=ON \
        -DWITH_GIT_INFO=OFF \
        -DCMAKE_INSTALL_PREFIX=/usr/local && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# =============================================================================
# Stage 4: Runtime image (FC custom container)
# =============================================================================
FROM base AS runtime

# Install ROS runtime libraries (already installed in base)
# Copy Thrift shared library
COPY --from=deps-builder /usr/local/lib/libthrift*.so* /usr/local/lib/
COPY --from=deps-builder /usr/lib/libthrift*.so* /usr/lib/
COPY --from=deps-builder /usr/lib/x86_64-linux-gnu/libturbojpeg.so* /usr/lib/x86_64-linux-gnu/
COPY --from=deps-builder /opt/libjpeg-turbo /opt/libjpeg-turbo
COPY --from=deps-builder /usr/local/lib/libopencv_*.so* /usr/local/lib/
COPY --from=deps-builder /usr/local/lib/libopencv_*.so.3.4 /usr/local/lib/

# Copy hera libraries and binary
COPY --from=hera-builder /usr/local/lib/libhera*.so* /usr/local/lib/
COPY --from=hera-builder /usr/local/lib/hera/ /usr/local/lib/hera/
COPY --from=hera-builder /usr/local/bin/hera-convert /usr/local/bin/hera-convert

RUN ldconfig

# Install Python deps for FC handler (oss2 for OSS access)
RUN pip3 install --no-cache-dir oss2

# Copy FC handler
COPY cloud/docker/fc_handler.py /opt/fc_handler.py

# FC custom container listens on port 9000
EXPOSE 9000

ENV HERA_DEVICE_PLUGIN_PATH=/usr/local/lib/hera/plugin
ENV HERA_DEVICE_LOAD_DRIVER=0

CMD ["python3", "/opt/fc_handler.py"]
