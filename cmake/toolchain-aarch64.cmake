# Cross-compilation toolchain for aarch64 (arm64) on Ubuntu 22.04 host.
#
# Compatible with Ubuntu multiarch where arm64 libraries live in
# /usr/lib/aarch64-linux-gnu/ and headers are shared in /usr/include/.
#
# Usage:
#   cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-aarch64.cmake \
#            -DCMAKE_LIBRARY_PATH=/usr/lib/aarch64-linux-gnu
#
# Install prerequisites:
#   dpkg --add-architecture arm64
#   apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
#     thrift-compiler \
#     libthrift-dev:arm64 libboost-dev:arm64 libturbojpeg0-dev:arm64 ...

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)

# BOTH: cmake searches root paths first, then native system paths.
# Combined with -DCMAKE_LIBRARY_PATH=/usr/lib/aarch64-linux-gnu passed at
# cmake invocation time, find_library() prefers arm64 packages over amd64.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
