# Hera 构建指南

## 本地构建

- 目标系统: ARM 平台  
  不推荐在 ARM 环境中构建, 可选择使用 `docker` 环境构建,或者安装交叉编译器进行构建
- 目标系统: x86-64 平台  
  请继续阅读该章节

### 安装构建依赖

以下安装步骤默认在 Ubuntu 系统上执行,若使用其他系统需要对安装步骤和脚本做相应的修改

- 进入 `setup` 目录,以下脚本均在该目录下执行

- 挂载公司 NFS 到 `/mnt/nfs` 目录 (仅 ARM 编译环境需要,若已挂载或不需要编译 ARM 可忽略)

- 安装 TurboJpeg (驱动库依赖,必须)  
  根据目标系统,执行

  - x86-64: `install-jpeg-turbo.sh`
  - ARM: `install-jpeg-turbo-arm.sh`

- 安装 ROS (非必须,仅构建`hera-convert`时需要)  
  参考[官方安装教程](http://wiki.ros.org/kinetic/Installation/Ubuntu)，源可以选用国内的源  
  或者参考 gaia 仓库内的 ros 安装脚本

- 安装 PCL (非必须,仅构建 `hera-convert` 时需要)  
  点云数据转换依赖,执行 `install-pcl.sh`

- 安装 Thrift (非必须,仅构建 `hera-daemon` 时需要)  
  根据目标系统,执行

  - x86-64: `install-thrift.sh`
  - ARM: `install-thrift-arm.sh`

- 安装 LibConfig (非必须,仅构建 `hera-daemon` 时需要)
  执行 `sudo apt-get install libconfig++-dev libconfig-dev`

- 安装 Npm (非必须,仅构建 `hera-daemon` 对应的客户端 `client` 时需要)  
  执行 `install-npm.sh`

- 安装 S32V 交叉编译器 (仅在 ARM-S32V 上交叉编译时候需要)  
  执行 `install-s32v-crossenv.sh`

- 安装设备驱动 (按需安装)
  - FlyCapture(传感器设备 `camera/flir` 依赖)  
    仅在 Ubuntu 14, Ubuntu 16, x86_64 下可用  
    执行 `install-flycapture.sh`
  - S32VSAL 驱动(传感器设备 `s32vsal`, `s32vgeely` 依赖)  
    仅在 ARM-S32V 环境中可用  
    执行 `install-s32vsal.sh`, `install-s32v-libfast.sh`

### 构建步骤

#### 构建 C++ 代码

运行以下命令

```shell
# 创建build文件夹
mkdir build
cd build

# 若需要交叉编译 ARM-S32V
# 执行以下两行
# unset LD_LIBRARY_PATH
# . /opt/s32v/environment-setup-aarch64-fsl-linux

# 若需要构建所有模块 使用DWITH_ALL=1
cmake .. -DWITH_ALL=1

# 若不需构建所有模块
# 可使用以下编译开关 WITH_DAEMON, WITH_CONVERT, WITH_REPLAY, WITH_SLAM
# 分别可以开关对应模块的编译
# 如 cmake .. -DWITH_REPLAY=1 -DWITH_CONVERT=1, 可构建hera-replay, hera-convert
# common, device, storage 为依赖库, 总是会被构建

# 在一般的机器上构建
make -j

# 在内存很少的平台上上构建时, 并行构建可能会导致内存耗尽, 可禁止并行构建
# make
```

#### 构建客户端

```shell
# 构建结果目录
mkdir -p build_client

cd client
npm install
npm run thrift
npm run build -- --dest ../build_client
```

#### 生成文档

运行 `doxygen doxygen/doxygen.config`, 生成文档在 `doxygen/html` 目录下
