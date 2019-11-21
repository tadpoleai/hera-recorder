# HERA 安装和部署指南

## 安装依赖库

以下步骤假设在Ubuntu 16.04 LTS上进行

### 安装ROS

参考[官方安装教程](http://wiki.ros.org/kinetic/Installation/Ubuntu)，源可以选用国内的源

或者参考gaia仓库内的ros安装脚本

### 安装服务器部署依赖

- `deploy/install-nginx.sh`

### 安装服务端构建依赖

- `deploy/install-thrift.sh`

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
make
```

### 构建客户端

```shell
cd client
npm i
npm build
```

## 部署

### 配置Nginx

参考`deploy/nginx.md`，配置Nginx

### 配置网口(若有需要)

参考[硬件设置](https://confluence.newayz.com/pages/viewpage.action?pageId=20644872)中的网口配置，设置网口MTU，内核缓存以及udev文件

### 安装服务端为自启动

```shell
cd build
sudo make install
```

### 部署客户端到Nginx指定页面

```shell
cd client
npm run deploy-linux
```
