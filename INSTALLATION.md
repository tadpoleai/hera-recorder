# HERA 安装和部署指南

以下步骤需在Ubuntu 16.04 LTS上进行

## 构建

### 本地构建

#### 安装构建依赖

- ROS

    参考[官方安装教程](http://wiki.ros.org/kinetic/Installation/Ubuntu)，源可以选用国内的源

    或者参考gaia仓库内的ros安装脚本

- `ROS`以外的依赖，会在构建过程中自动按需安装。

#### 构建全部代码

```shell
mkdir build
cd build
cmake .. Dwith-all=1 Dwith-driver-all=1
make -j
make client
```


#### 仅构建客户端

```shell
sudo deploy/install-thrift.sh
cd client
npm i
npm run thrift
npm run build
```

#### 按需构建代码

```shell
mkdir build
cd build
cmake .. [Dwith-<module>=1] [...]
make -j
make client
```

其中`module`，可从子文件夹(`daemon`, `convert`, `replay`, `slam`, `client`)中选择。

此外，`Dwith-driver-<driver_name>` 或者`Dwith-driver-all`，可以指定某些物理设备驱动是否编译。

*例如，需要本地构建，replay和slam，以便播放离线数据来调试实时建图，可以输入`cmake .. Dwith-slam=1 Dwith-replay=1`*

## 部署

### 预设置

#### 配置UDEV

以太网接口和USB接口的传感器，需要配置设备名，网卡名，内核参数，网络地址等等，可以参考[`deploy/dev_etc.md`](deploy/dev_etc.md)

#### 配置无线AP

参考[`deploy/wlan.md`](deploy/wlan.md)，配置无线AP

#### 配置Nginx

参考[`deploy/nginx.md`](deploy/nginx.md)，配置Nginx

### 部署文件

#### 从Jenkins构建产物部署

下载Jenkins构建产物，并解压运行里面的install_artifacts.sh

#### 从本地构建部署

```shell
cd build
sudo make install
cd ..
cd client
npm run deploy-linux
```
