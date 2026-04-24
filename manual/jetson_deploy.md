# Jetson Nano 完整部署指南（hera-daemon + 手机 Web 访问）

本文说明如何在 Jetson Nano（aarch64, Ubuntu 18.04）上部署采集系统的完整服务端，
最终通过手机浏览器进行无线控制。

## 系统架构

```
[手机浏览器] ─── WiFi/以太网 ──→ [Jetson Nano]
                                    │
                                  nginx:80
                                    │ proxy /hera → localhost:10093
                                    ▼
                               hera-daemon:10093  (Thrift-over-HTTP)
                               ├── device plugin: Insta360 X4 (USB)
                               └── device plugin: Livox Mid360 (Ethernet)
```

---

## 一、在 Jetson 上安装依赖

以下所有命令在 Jetson Nano 上执行。

### 1.1 基础编译工具 & libconfig++

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake git \
    libboost-all-dev libevent-dev \
    automake libtool flex bison pkg-config libssl-dev \
    libconfig++-dev libconfig-dev \
    libusb-1.0-0-dev libudev-dev \
    ninja-build
```

### 1.2 安装 libjpeg-turbo（hera 必须依赖）

hera 的 cmake 在 aarch64 主机上构建时会在 `/opt/libjpeg-turbo/lib64/` 中寻找库文件。
使用官方 arm64 deb 安装，然后补充 `lib64` 符号链接。

```bash
cd /tmp

VERSION="2.0.4"
wget "https://iweb.dl.sourceforge.net/project/libjpeg-turbo/${VERSION}/libjpeg-turbo-official_${VERSION}_arm64.deb"
sudo dpkg -i libjpeg-turbo-official_${VERSION}_arm64.deb

# 查看实际的库目录
ls /opt/libjpeg-turbo/
# 如果只有 lib（无 lib64），则创建 lib64 → lib 的符号链接
sudo ln -sfn /opt/libjpeg-turbo/lib /opt/libjpeg-turbo/lib64

# 确认
ls /opt/libjpeg-turbo/lib64/libturbojpeg.so

# 将路径加入 ldconfig（hera-daemon 运行时必须能找到 libturbojpeg.so.0）
echo "/opt/libjpeg-turbo/lib64" | sudo tee /etc/ld.so.conf.d/libjpeg-turbo.conf
sudo ldconfig
```

### 1.3 安装 Thrift 0.12.0（从源码编译，约 5–10 分钟）

```bash
cd /tmp

wget "https://mirrors.tuna.tsinghua.edu.cn/apache/thrift/0.12.0/thrift-0.12.0.tar.gz"
tar -xzf thrift-0.12.0.tar.gz
cd thrift-0.12.0

./bootstrap.sh

# 仅编译 C++ 库，跳过其他语言绑定
./configure \
    --with-cpp \
    --without-python \
    --without-java \
    --without-go \
    --without-nodejs \
    --without-lua \
    --without-ruby \
    --without-perl \
    --without-php \
    --without-qt4 \
    --without-qt5 \
    --disable-tests \
    --disable-tutorial

make -j4        # Jetson Nano 内存有限，避免 -j$(nproc)
sudo make install
sudo ldconfig   # 刷新动态链接库缓存
```

> **说明**：如果下载较慢，可换成 Apache CDN：
> `wget https://archive.apache.org/dist/thrift/0.12.0/thrift-0.12.0.tar.gz`

### 1.4 安装 Node.js & npm（构建 Web 客户端）

```bash
# 使用 NodeSource 安装 Node.js 20 LTS
curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash -
sudo apt-get install -y nodejs
node --version   # 应为 v20.x
npm --version
```

### 1.5 Livox SDK2（参照 PHASE1_GUIDE.md）

如已按 PHASE1_GUIDE 安装 Livox SDK2，跳过此步。

### 1.6 Insta360 CameraSDK aarch64（参照 PHASE1_GUIDE.md）

```bash
# 将 SDK 头文件/库安装到标准路径（如尚未完成）
INSTA_SDK_ROOT="<repo>/docs/insta_sdk/CameraSDK-2.0.2-jetson/x86_64_aarch64_linux-gnu"
sudo cp ${INSTA_SDK_ROOT}/include/*.h /usr/local/include/
sudo cp ${INSTA_SDK_ROOT}/lib/*.so    /usr/local/lib/
sudo ldconfig
```

---

## 二、编译 hera-recorder（含 daemon）

```bash
cd <repo>/hera/recorder     # 切换到 recorder 目录

# 创建构建目录
mkdir -p build_jetson
cd build_jetson

cmake .. \
    -DWITH_DAEMON=1 \
    -DINSTA_SDK_ROOT="<绝对路径>/docs/insta_sdk/CameraSDK-2.0.2-jetson/x86_64_aarch64_linux-gnu"

# 内存有限，不并行或少并行
make -j2

# 可选：如发现内存不足，改用 make（单线程）
```

> **WITH_DAEMON=1** 会同时构建：`hera-daemon`、`hera-daemon-finder`、以及所有 device plugins。  
> 若还需要 `hera-record`，添加 `-DWITH_RECORD=1`。

---

## 三、编译 hera-client（Web 前端）

```bash
cd <repo>/hera/recorder

mkdir -p build_client

cd client
npm install

# 修复 ESLint 配置文件名（源码中文件名缺少前置点）
cp eslintrc.js .eslintrc.js 2>/dev/null || true

# 修复 lodash.template 安装包损坏问题（缺少 assignWith/arrayEach）
echo "'use strict'; module.exports = require('lodash/template');" > node_modules/lodash.template/index.js

npm run thrift         # 从 IDL 生成 Thrift JS 代码
npm run build -- --dest ../build_client
```

> **提示**：如果 Jetson Nano 内存不足导致 `npm run build` OOM，可在 x86 开发机上
> 执行此步骤，然后 `scp -r build_client/ jetson:/tmp/` 传输到 Jetson。

---

## 四、安装 nginx & 配置

### 4.1 安装 nginx

```bash
cd <repo>/hera/recorder/setup
bash install-nginx.sh
```

如果 `install-nginx.sh` 报错，手动安装：
```bash
sudo apt-get install -y nginx
```

### 4.2 配置 nginx 虚拟主机

```bash
# 清理默认站点，复制 hera-client 配置
sudo rm -f /etc/nginx/sites-enabled/default
sudo cp <repo>/hera/recorder/setup/nginx/hera-client /etc/nginx/sites-enabled/hera-client

# 创建 Web 根目录
sudo mkdir -p /var/www/hera-client

# 验证 nginx.conf 包含 sites-enabled
grep "sites-enabled" /etc/nginx/nginx.conf
# 如无该行，在 http {} 块中添加：include /etc/nginx/sites-enabled/*;

sudo nginx -t    # 语法检查
```

### 4.3 部署前端文件

```bash
sudo cp -r <repo>/hera/recorder/build_client/* /var/www/hera-client/
sudo systemctl restart nginx
```

---

## 五、安装 hera-daemon

```bash
cd <repo>/hera/recorder/build_jetson
sudo make install
# 会安装：
#   /usr/local/bin/hera-daemon
#   /usr/local/bin/hera-daemon-finder
#   /usr/local/lib/hera/plugin/*.so   ← device plugins
#   /lib/systemd/system/hera-daemon.service
#   /var/hera/data, /var/hera/logs    （由 install-extra.sh 创建）
```

---

## 六、配置 /etc/hera.conf

将仓库中的模板复制并编辑：

```bash
sudo cp <repo>/hera/recorder/daemon/config/daemon.conf /etc/hera.conf
sudo nano /etc/hera.conf    # 或使用 vim
```

**必须修改的字段：**

```libconfig
# 1. 设备名称（任意）
name = "JetsonNano"

# 2. 监听端口：与 nginx（setup/nginx/hera-client）中 proxy_pass 端口一致即可
#    nginx 默认配置为 proxy_pass http://127.0.0.1:10093/，通常无需修改
listen = {
  address = "0.0.0.0"
  port = 10093
}

# 3. 数据存储目录（确保磁盘足够）
data_directory = "/var/hera/data"

# 4. Plugin 目录（安装后默认值即正确）
plugin_directory = {
  device = "/usr/local/lib/hera/plugin"
  upload = "/usr/local/lib/hera/plugin"
}

# 5. 不需要上传服务器时，清空 servers 列表
upload = {
  dynamic = false
  servers = ()
  localdisk_mountpoint = "/media/root"
}
```

> **端口说明**：`setup/nginx/hera-client` 中 `proxy_pass http://127.0.0.1:10093/`，
> 与 daemon 默认配置 `listen/port = 10093` 一致，通常无需修改此字段。

---

## 七、裁剪 Device Plugins（可选）

`/usr/local/lib/hera/plugin/` 中会有多种传感器驱动。本项目只用 Insta360 X4 和 Livox Mid360，
可以删除其他插件减少加载时间：

```bash
ls /usr/local/lib/hera/plugin/
# 保留 *insta*, *livox*, *mid360* 相关的 .so，删除其余驱动
```

---

## 八、启动服务

```bash
# 启动 hera-daemon（需要 root 权限访问 USB 和网络）
sudo systemctl daemon-reload
sudo systemctl enable hera-daemon
sudo systemctl start hera-daemon

# 检查状态
sudo systemctl status hera-daemon
journalctl -u hera-daemon -f   # 实时日志

# 确保 nginx 也在运行
sudo systemctl enable nginx
sudo systemctl start nginx
sudo systemctl status nginx
```

---

## 九、手机访问

1. 让手机与 Jetson Nano 处于同一网络（WiFi 或有线）
2. 查询 Jetson 的 IP 地址：`hostname -I | awk '{print $1}'`
3. 手机浏览器访问 `http://<jetson-ip>/`

> **确保局域网互通**：如果 Jetson 挂载到交换机/路由器，手机连接同一 SSID 即可。
> 若需要长期固定 IP，在路由器上给 Jetson 的 MAC 绑定静态 DHCP，或配置 netplan。

---

## 十、FAQ

| 现象 | 排查步骤 |
|------|---------|
| nginx 返回 502 Bad Gateway | hera-daemon 未启动，或 `/etc/hera.conf` 中 `listen/port` 不是 10093 |
| hera-daemon 启动即崩溃 | `journalctl -u hera-daemon -n 50` 看错误；常见原因：plugin .so 找不到依赖库（跑 `ldd /usr/local/lib/hera/plugin/*.so`）|
| libjpeg-turbo cmake 报错 | 检查 `/opt/libjpeg-turbo/lib64/libturbojpeg.so` 是否存在，不存在则补充 `lib64 → lib` 符号链接 |
| hera-daemon 启动报 `libturbojpeg.so.0 not found` | `/opt/libjpeg-turbo/lib64` 未加入 ldconfig；执行 `echo "/opt/libjpeg-turbo/lib64" \| sudo tee /etc/ld.so.conf.d/libjpeg-turbo.conf && sudo ldconfig` |
| Thrift 找不到 | `ldconfig` 后再重新 cmake；或 `find /usr/local/lib -name "libthrift*"` 确认安装 |
| 手机打开 Web 页面空白 | `ls /var/www/hera-client/` 确认 `index.html` 存在；nginx 日志 `/var/log/nginx/error.log` |
| 采集时提示 `505 Place given '' is not safe` | 前端「地点」字段为空或含特殊字符，仅允许 `[a-zA-Z0-9_]`，例如填入 `test_01` |
| 采集时提示 `202 ConfigPath is empty` | Livox 设备未填写 Config Path；在 Web UI 中补充 JSON 配置文件绝对路径，填入 `/etc/mid360_config.json`（或 `/etc/livox_lidar_config.json`，两者均有效） |
| 采集时提示 `202 LivoxLidarSdkInit failed` | ConfigPath 所指文件不存在或路径有误；确认填入的路径为 `/etc/mid360_config.json`，并确认文件已创建（`ls /etc/mid360_config.json`）|
| 点击「网络设置」daemon 立即崩溃 | `Network::retrieve()` 中 `ifa_addr` 空指针（VPN/tun 等虚拟接口无 IPv4 地址）；已在 `daemon/network/network.cpp` 中加空指针保护并重新部署 |
| 重启后报 `IPCQueue: Can not open key = '0', can not create` | 上次 daemon 崩溃残留了共享内存段；运行 `ipcrm -M $((0x0001BF52))` 清除后再重启 daemon |
| Insta360 "no device found" / "timeout to wait for synchronize" | daemon 崩溃后相机 USB 会话状态机停留在 "session open"；重启 daemon 时 `ExecStartPre` 会自动执行 USB 重置（见 `daemon/script/hera-daemon.service`），等待约 9 秒后自动恢复。若仍无法发现设备，手动执行：`sudo usbreset $(ls /dev/bus/usb/$(cat /sys/bus/usb/devices/$(ls /sys/bus/usb/devices/ \| xargs -I{} sh -c 'cat /sys/bus/usb/devices/{}/idVendor 2>/dev/null \| grep -q 2e1a && echo {}' \| head -1)/busnum 2>/dev/null \| xargs printf '%03d')/*.*)` 后重启 daemon；最后方案是手动断电重启相机 |
| Insta360 "parse packet error" / 第一次连接失败，第二次成功 | 采集异常结束（SIGKILL/daemon crash）后相机 USB Bulk-IN 端点残留上次协议交换的字节，首次 `Open()` 读到脏数据解析失败；已在 `plugin_entry.cpp` 中加入自动重试（最多 2 次，间隔 300ms），hera-daemon 内无需手动操作即可恢复 |
