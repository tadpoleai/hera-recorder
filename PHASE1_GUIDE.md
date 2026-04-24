# Phase-1 多传感器采集指南

> 设备组合：**Insta360 X4 全景相机** + **Livox Mid360 激光雷达**  
> 支持平台：Linux aarch64（Jetson 等）/ x86_64  
> 验证状态：✅ 已通过数据正确性验证（2026-04-22）

---

## 目录

1. [前置依赖](#1-前置依赖)
2. [编译](#2-编译)
3. [采集](#3-采集)
4. [解析与验证](#4-解析与验证)
5. [输出文件说明](#5-输出文件说明)
6. [常见问题](#6-常见问题)

---

## 1. 前置依赖

### 1.1 系统包

```bash
sudo apt update
sudo apt install -y \
    cmake build-essential \
    libusb-1.0-0-dev libudev-dev \
    usb-utils          # 提供 usbreset 工具（相机 USB 重置必需）
```

### 1.2 Insta360 Camera SDK

从 Insta360 开发者平台获取对应平台的 SDK，解压后令 `INSTA_SDK_ROOT` 指向包含 `include/` 和 `lib/` 的根目录：

| 平台 | SDK 目标架构 |
|------|-------------|
| Jetson / aarch64 | `x86_64_aarch64_linux-gnu`（交叉编译版，直接在 aarch64 运行） |
| x86_64 PC | `CameraSDK-*-Linux`（x86_64 原生版） |

```bash
# aarch64 示例（Jetson）
export INSTA_SDK_ROOT=/path/to/CameraSDK-2.0.2-jetson/x86_64_aarch64_linux-gnu

# x86_64 示例
export INSTA_SDK_ROOT=/path/to/CameraSDK-2.1.1-Linux
```

### 1.3 Livox SDK2

按照 [Livox-SDK2 README](../../../docs/Livox-SDK2/README.md) 编译并安装（默认安装到 `/usr/local`）：

```bash
cd docs/Livox-SDK2
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
sudo cmake --install build
```

安装后 `liblivox_lidar_sdk_shared.so` 位于 `/usr/local/lib/`。

### 1.4 Livox Mid360 网络配置文件

Livox SDK2 需要一个描述雷达 IP 的 JSON 配置文件。可从 SDK 示例中复制：

```bash
cp docs/Livox-SDK2/samples/logger/mid360_config.json \
   hera/recorder/device/src/mid360_config.json
# 按实际修改 lidar_configs[0].ip 为你的 Mid360 IP（默认 192.168.1.1xx）
```

`phase1_record.sh` 会按以下顺序自动查找该文件：
1. `device/src/mid360_config.json`（推荐放置位置）
2. 环境变量 `LIVOX_CONFIG_PATH`
3. 其他常见路径

---

## 2. 编译

```bash
cd hera/recorder

# 配置（指定 Insta360 SDK 路径）
cmake -B build_livox_phase1 \
    -DCMAKE_BUILD_TYPE=Release \
    -DINSTA_SDK_ROOT="${INSTA_SDK_ROOT}"

# 构建所需目标
cmake --build build_livox_phase1 -j4 \
    --target hera-device-record \
    --target hera-storage-tool \
    --target hera-storage-extract-insta \
    --target hera-storage-extract-mid360
```

构建产物：

| 可执行文件 | 路径 |
|-----------|------|
| 采集程序 | `build_livox_phase1/device/hera-device-record` |
| 存储统计 | `build_livox_phase1/storage/hera-storage-tool` |
| Insta 提取 | `build_livox_phase1/storage/hera-storage-extract-insta` |
| Mid360 提取 | `build_livox_phase1/storage/hera-storage-extract-mid360` |

---

## 3. 采集

### 3.1 硬件连接

1. Insta360 X4 通过 USB 连接（Android USB 模式，USB 2.0 Full-Speed）
2. Livox Mid360 通过以太网连接，IP 与 `mid360_config.json` 中一致

### 3.2 启动采集

```bash
cd hera/recorder

# 数据默认保存到 ./data/<timestamp>_phase1/
bash phase1_record.sh

# 或指定输出目录
bash phase1_record.sh /data/phase1/session_001
```

脚本会自动：
- 请求 sudo 权限（Insta360 USB 访问必需）
- 禁用 USB autosuspend 并重置相机 USB 会话状态
- 生成运行时配置文件（替换 Livox 配置路径）
- 启动采集，Insta360 + Mid360 写入**同一个 `.hera` 文件**
- 捕获 Ctrl+C，优雅退出（最多等待 15 秒，超时强制终止）

按 **Ctrl+C** 停止采集。输出目录内将有：

```
<output_dir>/
├── <timestamp>_phase1.hera         # 所有传感器原始数据（合并）
├── record.log                      # 采集日志
└── .config_phase1_runtime.json     # 运行时配置（自动生成，可忽略）
```

### 3.3 采集参数

核心参数已预设在 `device/src/config_phase1_combined.json`：

| 设备 | 参数 | 值 | 说明 |
|------|------|-----|------|
| Insta360 | `VideoResolution` | `R3840x1920P30` | 4K 全景 30fps |
| Insta360 | `EnableGyro` | `true` | 启用陀螺仪 |
| Insta360 | `EnableAudio` | `false` | 不采集音频 |
| Mid360 | `SyncType` | `Full` | 时间戳同步到主机 |
| Mid360 | `EnableImu` | `true` | 启用 IMU |

---

## 4. 解析与验证

### 4.1 一键解析+验证（推荐）

```bash
bash phase1_validate.sh \
    /path/to/<timestamp>_phase1.hera \
    /path/to/extracted_output_dir
```

脚本依次执行：
1. **存储统计**：包数、码率
2. **Insta 提取**：`insta_video.h264` + `insta_gyro.csv`
3. **Mid360 提取**：`mid360_points.csv` + `mid360_imu.csv`
4. **完整性检查**：文件大小、行数、时间戳单调性、加速度合向量（期望 ≈ 1g）

### 4.2 单独查看存储统计

```bash
./build_livox_phase1/storage/hera-storage-tool -i <file.hera> -a
```

### 4.3 单独提取 Insta360

```bash
./build_livox_phase1/storage/hera-storage-extract-insta \
    <phase1.hera> \
    --video out.h264 \
    --gyro  out_gyro.csv
```

Gyro CSV 格式（`ins_camera::GyroData`，约 125Hz）：

```csv
timestamp_ns,ax,ay,az,gx,gy,gz
346157,-0.997070,0.000000,-0.009766,0.000000,0.000000,0.003196
```

### 4.4 单独提取 Mid360

```bash
./build_livox_phase1/storage/hera-storage-extract-mid360 \
    <phase1.hera> \
    --points out_points.csv \
    --imu    out_imu.csv
```

点云 CSV（x/y/z 单位：米）：

```csv
timestamp_device_ns,timestamp_host_ns,x_m,y_m,z_m,reflectivity,tag,data_type
```

IMU CSV（约 200Hz）：

```csv
timestamp_device_ns,timestamp_host_ns,gyro_x,gyro_y,gyro_z,acc_x,acc_y,acc_z
```

---

## 5. 输出文件说明

| 文件 | 说明 |
|------|------|
| `<ts>_phase1.hera` | 原始二进制存储，Insta360 + Mid360 合并 |
| `insta_video.h264` | H.264 裸码流（`ffplay -f h264 insta_video.h264`） |
| `insta_gyro.csv` | 加速度计 + 陀螺仪（约 125Hz） |
| `mid360_points.csv` | 点云 XYZ + 反射率（以米为单位） |
| `mid360_imu.csv` | 角速度 + 加速度（约 200Hz） |

---

## 6. 常见问题

### `timeout to wait for synchronize`

相机上一次采集因强制退出导致 USB 会话状态未正确关闭。  
**修复**：断开并重新插拔 USB 线，或运行：  
```bash
sudo usbreset $(readlink -f /sys/bus/usb/devices/$(ls /sys/bus/usb/devices/ | xargs -I{} sh -c 'cat /sys/bus/usb/devices/{}/idVendor 2>/dev/null | grep -q 2e1a && echo {}' | head -1)/../../)  2>/dev/null || sudo usbreset /dev/bus/usb/001/$(cat /sys/bus/usb/devices/1-1/devnum 2>/dev/null | xargs printf '%03d')
```
`phase1_record.sh` 在每次启动时会自动执行 USB 重置，多数情况下无需手动操作。

### `no USB camera discovered`

- 确认相机已开机
- 确认 USB 模式为 **Android**（在相机设置中切换）
- 确认以 root（或 sudo）运行

### Mid360 点云提取结果为 0 行

- 确认传入的是 **合并 `.hera` 文件**（由 `phase1_record.sh` 生成）
- 检查 `record.log` 中 Mid360 是否有 `rx(point)=0`
- 确认 `mid360_config.json` 中的 IP 与实际雷达 IP 一致

### 视频文件大小为 0 或过小

- Insta360 启动流时需要数秒准备时间，采集时长需 > 5 秒
- 检查 `record.log` 中是否有 `StartLiveStreaming failed`

---

## 附：源码位置

| 文件 | 说明 |
|------|------|
| `storage/tool/extract_insta.cpp` | Insta360 数据提取工具 |
| `storage/tool/extract_mid360.cpp` | Mid360 数据提取工具 |
| `device/src/config_phase1_combined.json` | 采集配置（两传感器合并） |
| `device/plugin/camera/insta/plugin_entry.cpp` | Insta360 设备插件 |
| `device/plugin/lidar/livox/plugin_entry.cpp` | Mid360/Livox 设备插件 |


---

## 1. 设备与数据格式

### Insta360 X4

| 类型 | 详情 |
|------|------|
| 设备配置 | `device/src/config_insta_phase1.json` |
| 存储文件 | `<timestamp>_insta_phase1.hera` |
| 视频格式 | H.264，3840×1920@30fps，Bitrate 512 Kbps |
| Gyro 格式 | `ins_camera::GyroData`：`int64 timestamp + 6×double (ax,ay,az,gx,gy,gz)` |
| 包类型代码 | 视频 `0x0421`，Gyro `0x0422` |

### Livox Mid360

| 类型 | 详情 |
|------|------|
| 设备配置 | `device/src/config_mid360_livox.json` |
| 存储文件 | `<timestamp>_mid360_phase1.hera` |
| 点云格式 | Cartesian High（1mm精度）或 Low（1cm精度），含反射率和标签 |
| IMU 格式 | `float32 × 6 (gyro_xyz, acc_xyz)`，设备时间戳 + 主机时间戳 |
| 包类型代码 | 点云 `0x0521`，IMU `0x0524` |

---

## 2. 编译

所有工具通过一次 CMake 构建完成，使用 `build_livox_phase1` 目录：

```bash
cd /home/yuancaimaiyi/Code/insta-data-collection/hera/recorder

# 首次配置（已有构建目录可跳过）
cmake -B build_livox_phase1 \
    -DCMAKE_BUILD_TYPE=Release \
    -DWITH_LIVOX=ON

# 构建所有目标（含新增工具）
cmake --build build_livox_phase1 -j4

# 仅构建数据提取工具
cmake --build build_livox_phase1 -j4 \
    --target hera-storage-extract-insta \
    --target hera-storage-extract-mid360 \
    --target hera-storage-tool \
    --target hera-device-record
```

构建产物位置：

| 可执行文件 | 路径 |
|-----------|------|
| 采集程序 | `build_livox_phase1/device/hera-device-record` |
| 存储统计 | `build_livox_phase1/storage/hera-storage-tool` |
| Insta 提取 | `build_livox_phase1/storage/hera-storage-extract-insta` |
| Mid360 提取 | `build_livox_phase1/storage/hera-storage-extract-mid360` |

设备插件（`.so`）位于：
- 基础版：`build_livox_phase1/device/base/`
- 驱动版：`build_livox_phase1/device/driver/`

---

## 3. 采集

### 快速采集（推荐）

使用 `phase1_record.sh` 同时启动两个设备的采集：

```bash
cd /home/yuancaimaiyi/Code/insta-data-collection/hera/recorder

# 数据默认保存到 ./data/<timestamp>_phase1/
bash phase1_record.sh

# 或指定输出目录
bash phase1_record.sh /data/phase1/session_001
```

按 **Ctrl+C** 停止采集。两个进程会同时退出，输出目录内将有：

```
<output_dir>/
├── <timestamp>_insta_phase1.hera   # Insta360 原始数据
├── <timestamp>_mid360_phase1.hera  # Mid360 原始数据
├── insta_record.log                # Insta 采集日志
└── mid360_record.log               # Mid360 采集日志
```

### 手动采集

如需分别控制两个进程：

```bash
cd <output_dir>
export HERA_DEVICE_PLUGIN_PATH="<repo>/build_livox_phase1/device"
export HERA_DEVICE_LOAD_DRIVER=1

# 终端 1：Insta360 X4
./build_livox_phase1/device/hera-device-record \
    ./device/src/config_insta_phase1.json

# 终端 2：Livox Mid360
./build_livox_phase1/device/hera-device-record \
    ./device/src/config_mid360_livox.json
```

### 采集参数说明

`config_insta_phase1.json` 中的关键参数：

| 参数 | 值 | 说明 |
|------|-----|------|
| `VideoResolution` | `R3840x1920P30` | 4K 全景 30fps |
| `EnableGyro` | `true` | 启用陀螺仪数据流 |
| `EnableAudio` | `false` | 不采集音频 |
| `SyncLocalTime` | `false` | 不同步到相机时间 |

`config_mid360_livox.json` 中的关键参数：

| 参数 | 值 | 说明 |
|------|-----|------|
| `SyncType` | `Full` | 全同步模式（时间戳同步到主机） |
| `EnableImu` | `true` | 启用 IMU 数据 |

---

## 4. 解析与验证

### 一键解析+验证（推荐）

```bash
bash phase1_validate.sh \
    /path/to/<timestamp>_insta_phase1.hera \
    /path/to/<timestamp>_mid360_phase1.hera \
    /path/to/output_dir
```

脚本会依次执行：
1. **存储统计**：包数、码率、时长
2. **Insta 提取**：`insta_video.h264` + `insta_gyro.csv`
3. **Mid360 提取**：`mid360_points.csv` + `mid360_imu.csv`
4. **完整性检查**：大小、行数、时间戳单调性、加速度合向量（≈1g）

### 单独查看存储统计

```bash
./build_livox_phase1/storage/hera-storage-tool -i file.hera -a
```

示例输出（Insta360）：
```
Video(all): packets=10395, bytes=364133622, rate=30.002Hz
Gyro(all):  packets=3465, bytes=9420712,   rate=9.9992Hz
```

### 单独提取 Insta360 数据

```bash
# 提取视频 + gyro
./build_livox_phase1/storage/hera-storage-extract-insta \
    <input.hera> \
    --video out.h264 \
    --gyro  out_gyro.csv
```

Gyro CSV 格式（匹配 `ins_camera::GyroData`）：

```csv
timestamp_ns,ax,ay,az,gx,gy,gz
346157,-0.997070,0.000000,-0.009766,0.000000,0.000000,0.003196
346159,-0.998047,0.000977,-0.009766,0.001065,0.002131,0.001065
...
```

### 单独提取 Mid360 数据

```bash
# 提取点云 + IMU
./build_livox_phase1/storage/hera-storage-extract-mid360 \
    <input.hera> \
    --points out_points.csv \
    --imu    out_imu.csv
```

点云 CSV 格式（已转换为米）：

```csv
timestamp_device_ns,timestamp_host_ns,x_m,y_m,z_m,reflectivity,tag,data_type
1234567890,1234567891,1.2345,-0.4321,0.5678,200,0,1
...
```

IMU CSV 格式：

```csv
timestamp_device_ns,timestamp_host_ns,gyro_x,gyro_y,gyro_z,acc_x,acc_y,acc_z
1234567890,1234567891,0.001234,-0.000456,0.002345,0.123456,-0.234567,9.812345
...
```

---

## 5. 输出文件说明

| 文件 | 来源 | 说明 |
|------|------|------|
| `*_insta_phase1.hera` | Insta360 | 原始二进制存储，包含视频+gyro包 |
| `*_mid360_phase1.hera` | Mid360 | 原始二进制存储，包含点云+IMU包 |
| `insta_video.h264` | 提取 | H.264 裸码流，可用 ffplay/ffmpeg 播放 |
| `insta_gyro.csv` | 提取 | 加速度 + 陀螺仪，约 125Hz |
| `mid360_points.csv` | 提取 | 点云 XYZ + 反射率，以米为单位 |
| `mid360_imu.csv` | 提取 | 角速度 + 加速度，约 200Hz |

---

## 6. 常见问题

### 采集程序找不到设备插件

```
[错误] 采集程序未找到
```
→ 检查 `HERA_DEVICE_PLUGIN_PATH` 是否指向包含 `.so` 的 `device/` 目录

### Insta360 连接超时

配置中 `CameraTimeoutMs` 默认 10000ms。确保相机已开机并通过 USB 连接，可在 `config_insta_phase1.json` 中增大超时值。

### hera-storage-extract-mid360 提取点数为 0

1. 确认使用的是 mid360 的 `.hera` 文件（而非 insta 文件）
2. 检查采集日志 `mid360_record.log` 中是否有 `rx(point/imu)=0/0`
3. 确认 Livox SDK 配置文件路径正确：`ConfigPath` 参数

### 视频文件无法播放

```bash
ffplay -f h264 insta_video.h264
# 或
ffmpeg -i insta_video.h264 out.mp4
```
