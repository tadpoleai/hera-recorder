#!/bin/bash
# Native build+install of Livox-SDK2 (no cross toolchain — run natively on arm64).
set -euo pipefail

if [ -f /usr/local/lib/liblivox_lidar_sdk_shared.so ]; then
    echo "[build_livox_sdk2] already installed, skipping"
    exit 0
fi

git clone --depth 1 https://github.com/tadpoleai/Livox-SDK2.git /tmp/Livox-SDK2
cmake -S /tmp/Livox-SDK2 -B /tmp/Livox-SDK2/build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build /tmp/Livox-SDK2/build -j"$(nproc)"
cmake --install /tmp/Livox-SDK2/build
