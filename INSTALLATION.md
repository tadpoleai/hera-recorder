# HERA 安装和部署指南

## 安装依赖库

以下步骤假设在Ubuntu 16.04 LTS上进行

### 安装ROS

参考[官方安装教程](http://wiki.ros.org/kinetic/Installation/Ubuntu)，源可以选用国内的源

或者参考gaia仓库内的ros安装脚本

### 安装服务器部署依赖

- `deploy/install-nginx.sh`

### 安装服务端构建依赖

- `deploy/install-thrift.sh` (该步骤可能会需要比较长的时间)

### 安装设备驱动依赖

- `deploy/install-flycapture.sh`
- `deploy/install-jpeg-turbo.sh`
- `deploy/install-pcl.sh`

### 安装客户端构建依赖

- `deploy/install-npm.sh`

## 构建

### 构建服务端

```shell
mkdir build
cd build
cmake ..
make -j
```

### 构建客户端

```shell
cd client
npm i
npm run thrift
npm run build
```

## 部署

### 配置硬件

以太网接口和USB接口的传感器，需要配置设备名，网卡名，内核参数，网络地址等等，可以参考[`deploy/dev_etc.md`](deploy/dev_etc.md)

### 配置无线AP

参考[`deploy/wlan.md`](deploy/wlan.md)，配置无线AP

### 配置Nginx

参考[`deploy/nginx.md`](deploy/nginx.md)，配置Nginx

### 安装软件

```shell
cd build
sudo make install
```

### 部署服务器(仅对下位机执行)

```shell
cd daemon/script/
sh install-extra.sh
```

### 部署客户端到Nginx

```shell
cd client
npm run deploy-linux
```

