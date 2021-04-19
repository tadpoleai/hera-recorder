# Hera 部署指南

本文为将 Hera 部署到采集设备上的指南

对于使用 Hera 库文件进行开发, 或使用 Hera 的工具程序时候的安装方法, 请参考 [Hera 分发包使用指南](redist.md)

## 装配硬件

根据实际硬件方案操作

## 安装系统

- 一般地, 在 x86_64 平台上, 安装 Ubuntu 16.04 LTS  
  以便启用实时 2D 建图功能 (建议内存大于 8G)  
  或者, 在 ARM 上安装或者在其他版本的 Ubuntu 下安装

  - 建议手动给 swap 分配足够大的空间 (建议 8G 以上), 以免执行实时建图时内存耗尽
  - 建议机器名为<ProductName><SN-Number>, 其中<ProductName>为产品代号 (如创 4-Tron4, 为 TR04), 序列号为 4 位数字
  - 建议设置用户名为 wayz

- 进行系统安全更新

  - 执行 `sudo apt-get update`
  - 执行 `sudo apt-get upgrade`
  - 建议使用 Aliyun 或科大的镜像源

## 安装软件

- 安装依赖库  
  在目标机上运行相应的程序, 需要安装依赖库  
  以下为 Hera 本身所需要的依赖库  
  | 程序\依赖项 | TurboJpeg | Thrift | ROS | Carto-deps |
  | :----------: | :-------: | :----: | :-: | :--------: |
  | hera-record | 〇 | - | - | - |
  | hera-daemon | 〇 | 〇 | \*1 | \*1, \*2 |
  | hera-replay | 〇 | - | - | - |
  | hera-convert | 〇 | - | 〇 | - |  
  \*1: hera-daemon 若需要调用实时建图, 需要安装该依赖  
  \*2: 包括 `liblua5.3-dev libceres-dev libgflags-dev libgoogle-glog-dev liblas-c-dev`

  此外. 还需安装传感器设备驱动的依赖库, 如 FlyCapture 等  
  安装依赖库时, 可参考 [Hera 构建指南](manual/build.md) 中相应章节

- 安装时间同步守护进程
  对于某些采集设备 (如创系列), 传感器的时间同步使用单独的嵌入式板卡进行, 该板卡需要一个上位机运行的时间同步守护进程与之通信

  - 创-Tron 系列  
    访问 [time_sync_pc](https://git.aimap.io/wzautonomy/hera/time_sync_pc), 下载并编译安装

- 安装 nginx  
  对于需要使用 `hera-daemon` 的采集设备, 提供 `hera-client` 的网页端服务

  - 运行 `setup/install-nginx.sh`
  - 修改 `/etc/nginx/nginx.conf`, 确认 `http` 块内包含以下语句

  ```plain-text
  include /etc/nginx/sites-enabled/*;
  ```

  - 删除 `/etc/nginx/sites-enabled/*`
  - 将本仓库内的 [`setup/nginx/hera-client`](../setup/nginx/hera-client) 复制到 `/etc/nginx/sites-enabled/`
  - 新建文件夹 `/var/www/hera-client/`

- 创建 Root 用户的 SSH 秘钥  
  启用 `hera-daemon` 的数据上传功能, 需要创建 SSH 秘钥用于和数据服务器通信

  - `sudo su -`
  - `ssh-keygen -t ecdsa`

## 部署安装

可从 Jenkins 自动构建产物部署, 或本地构建后部署安装

### 从 Jenkins 构建产物部署

下载 Jenkins 构建产物, 并解压运行里面的 `install_artifacts_<arch>.sh`  
其中 `<arch>` 为系统架构

- amd64: 为 x86_64 平台部署
- arm_host: 在 x86_64 平台上安装 ARM 平台的交叉编译环境
- arm_target: 在 ARM 平台上部署

该脚本执行过程中会出现提示, 按提示操作选择需要安装的项目

### 从本地构建部署

```shell
# 安装二进制文件
cd build
sudo make install

# 安装 client
cd ..
cd build-client
sudo mkdir -p /var/www/hera-client
sudo cp * /var/www/hera-client
```

## 调整参数

打开编辑 `/etc/hera.conf`

### 设置 Hera 网络中的主机名

编辑 `"name"` 段, 将 `"Tron"` 改为主机名字

### 设置上传服务器

编辑 `"upload/servers"` 段, 按实际增加服务器记录

若必要，增加后用 Root 账户测试连接

### 裁剪传感器设备驱动

进入 `/usr/local/lib/hera/plugin` 文件夹, 将不需要的传感器驱动文件移动到别处或者删除

### 硬件配置

### 配置硬件规则

若需要为传感器分配网卡和网关等, 可参考 [硬件配置指南](deploy_udev.md)

### 配置无线 AP

若需要使用手机连接采集设备, 可参考 [无线 AP 配置指南](deploy_wlan.md)
