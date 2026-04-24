# Changelog

## [Unreleased] — 2026-04-14 ~ 2026-04-24

本阶段主要工作：接入 Insta360 X4 相机与 Livox Mid360 激光雷达，完成 Phase 1 数据采集流程，
并修复将系统通过 hera-daemon + nginx 对外暴露为 Web 服务时发现的一系列问题。

---

### 新增功能

#### Livox Mid360 激光雷达支持（`device/plugin/lidar/livox/`）
- 新增 Livox SDK2 设备插件，支持点云数据与 IMU 数据的实时采集与写入 `.hera` 文件
- 参数说明：
  - `ConfigPath`（默认 `/etc/mid360_config.json`）：Livox SDK2 JSON 配置文件路径，开箱即用无需手动填写
  - `DeviceSn`：序列号过滤，留空绑定首个发现的设备
  - `SyncType`：时间同步方式（`Full` / `Local` / `Disabled`）
  - `EnableImu`：是否采集 IMU 数据
- 新增 Mid360 SDK 配置文件模板 `config/mid360_config.json`（host_ip=192.168.1.5，lidar 侧端口按 SDK 默认值）
- 新增数据提取工具 `storage/tool/extract_mid360.cpp`，可将 `.hera` 文件中的 Mid360 原始帧转储为 CSV/二进制
- 新增 `device/src/config_mid360_livox.json`：daemon 设备配置模板

#### Insta360 X4 相机支持（`device/plugin/camera/insta/`）
- 新增 Insta360 CameraSDK 2.0.2 (aarch64) 设备插件
- 支持两种工作模式：
  - `Stream`（默认）：实时预览流，通过 USB 回传 H.264 双目流 + 陀螺仪数据
  - `RecordDownload`：触发相机内录制，停止后自动下载到本地
- 参数包括分辨率、码率、帧率、时间同步、陀螺仪开关、InCamera 拼接等
- 新增数据提取工具 `storage/tool/extract_insta.cpp`
- 新增 `device/src/config_insta_phase1.json` 和 `config_phase1_combined.json`（Insta360+Mid360 联合采集）
- 新增 `device/plugin/camera/instawebcam/`：基于 V4L2/OpenCV 的 Webcam 模式插件（相机切换为 UVC 模式时使用）

#### Phase 1 采集脚本与验证工具
- 新增 `phase1_record.sh`：一键启动 Insta360+Mid360 联合采集的 shell 脚本
  - 采集前自动 USB 重置 Insta360（防止 `Synchronize` 超时）
  - 信号处理：收到 SIGINT/SIGTERM 后优雅停止，`camera->Close()` 超时 15s 后强制 SIGKILL 并补充 USB 重置
- 新增 `phase1_validate.sh`：采集后数据完整性验证脚本，检查 `.hera` 文件中各设备的包数与时间戳连续性
- 新增 `PHASE1_GUIDE.md`：Phase 1 全流程操作手册（环境准备、采集、验证、数据提取）

#### Web 访问支持（hera-client + nginx）
- `client/src/api/index.ts`：Thrift 连接路径改为 `/hera`（通过 nginx 反向代理），不再直接连接 daemon 端口
- `client/src/store/modules/main.ts`：生产环境默认端口改为 `80`，自动读取 `window.location.hostname`
- `setup/nginx/hera-client`：`proxy_pass` 端口由 `9090` 更正为 `10093`
- 新增 `manual/jetson_deploy.md`：完整部署文档（x86 开发机 / Jetson Nano），含 nginx、systemd、依赖安装、FAQ

---

### Bug 修复

#### `daemon/broadcast/broadcaster.cpp` — SIGSEGV（启动即崩溃）
- **问题**：遍历网络接口时，VPN/tun/docker 等虚拟接口的 `ifa_addr` 或 `ifa_dstaddr` 可能为 `nullptr`，直接解引用导致段错误，daemon 启动即崩溃。
- **修复**：在访问 `sa_family`、`ifu_broadaddr` 前加入 `nullptr` 检查，遇到无 IPv4 地址的接口直接跳过。

#### `daemon/network/network.cpp` — SIGSEGV（点击「网络设置」崩溃）
- **问题**：`Network::retrieve()` 中同样缺少 `ifa_addr` 空指针保护，在有 VPN/docker 网卡时点击 Web UI「网络设置」必崩。
- **修复**：同上，加入 `if (ifa->ifa_addr == nullptr || ...)` 保护。

#### `device/plugin/camera/insta/plugin_entry.cpp` — `parse packet error` / 首次连接失败
- **问题**：采集异常结束（SIGKILL、daemon crash）后，相机 USB Bulk-IN 端点的硬件 FIFO 中残留上次协议交换的字节尾。首次 `Open()` 读到脏数据，`io_device.cpp` 解析包头失败（日志：`parse packet error`），返回 `false`。手动再运行一次即正常。
- **根因**：USB Bulk Transfer 是流式协议，异常断开不会 flush 端点缓冲区；首次 `Open()` 的失败读操作恰好消耗了脏字节，第二次自然成功。
- **修复**：`connect()` 中将 `camera_->Open()` 包装为最多重试 2 次（间隔 300ms）的循环，对调用方透明。

#### `daemon/script/hera-daemon.service` — 相机重启后 `no device found`
- **问题**：daemon crash 后，相机 USB 控制端点会话状态机停留在 "session open"，`GetAvailableDevices()` 枚举不到设备，需要手动断电重启相机。
- **修复**：在 systemd 服务中加入 `ExecStartPre`，每次 daemon 启动前自动扫描 USB 设备，对 VID=`2e1a`（Insta360）的设备执行 `usbreset`，等待 4s 重新枚举。`Restart=on-failure` + `RestartSec=5s` 组合下，crash 后约 9s 自动完全恢复，无需人工介入。

#### `device/plugin/lidar/livox/plugin_param.hpp` — `ConfigPath` 默认值
- **问题**：`ConfigPath` 默认值为空字符串，每次在 Web UI 配置设备时必须手动填写，否则报 `202 ConfigPath is empty`。
- **修复**：默认值改为 `/etc/mid360_config.json`，与 `config/mid360_config.json` 配套，开箱即用。

---

### 部署相关

#### 必须手动执行的一次性操作（新机器部署时）

```bash
# 1. libjpeg-turbo 库路径注册（hera-daemon 运行时依赖）
echo "/opt/libjpeg-turbo/lib64" | sudo tee /etc/ld.so.conf.d/libjpeg-turbo.conf
sudo ldconfig

# 2. Mid360 SDK 配置文件（Livox 插件读取）
sudo cp hera/recorder/config/mid360_config.json /etc/mid360_config.json
# 按实际网络修改 host_ip 和 lidar ip（默认 host=192.168.1.5）

# 3. nginx 配置（proxy_pass 到 daemon 10093 端口）
sudo cp hera/recorder/setup/nginx/hera-client /etc/nginx/sites-enabled/hera-client
sudo nginx -t && sudo systemctl reload nginx
```

详见 `manual/jetson_deploy.md`。

---

### 已知限制

- Insta360 CameraSDK v2.0.2 aarch64 版在 `StopLiveStreaming()` 调用时存在概率性崩溃（SDK 内部 bug），当前插件在 `disconnect()` 中跳过该调用，直接执行 `camera->Close()`。
- `phase1_validate.sh` 在仅有 Insta360 的采集文件上退出码为 1（验证器内部对 Livox 帧数有最低要求）；同时采集两个设备时验证正常通过。
