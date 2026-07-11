# "采集开关"断连 + 陀螺仪/激光雷达录制窗口滞后 排查记录

记录 2026-07-11 一次真机排查：采集完点击"采集开关"前端报连接断开，以及紧接着发现的一个更隐蔽的问题——`.hera`（陀螺仪+激光雷达）实际录制窗口比 SD 卡视频晚开始了 25.6 秒。两个问题看似独立，根因都在 RecordDownload 模式的控制流上。

## 问题一：点击"采集开关"关闭后，daemon 报错、前端提示"连接断开"

### 现象

采集完一段 RecordDownload 会话后，点击前端"采集开关"（关闭），前端提示连接断开；同时 Jetson 上 `hera-daemon` 日志里能看到看似异常的报错。

### 排查

`journalctl -u hera-daemon` 拉出完整时间线：

```
19:34:24.027  Daemon::stop called
19:34:25.037  相机进入 NOT_CAPTURE（StopRecording 完成）
19:34:25 → 19:35:01（约36秒，无日志）  SDK 通过 USB 从相机下载 .insv 文件
19:35:01.279  下载完成
19:35:01.565  重命名为 <basename>.insv，写 session json
19:35:02.522  Daemon::Succeed —— daemon 尝试把响应写回客户端
19:35:02.522  Thrift: TSocket::write_partial() send(): Broken pipe   ← 这里报错
19:35:19.085  17秒后收到重复的 stop 调用
19:35:19.085  ERROR: Daemon is already stopped（无害）
```

整个 `stop()` 从被调用到 daemon 真正处理完，耗时约 **38.5 秒**（主要是 USB 下载 `.insv` 的 36 秒）。

### 根因

Jetson 上 nginx 把 `/hera` 反代到 `127.0.0.1:10093`（`daemon` 本身监听在这个端口，nginx 在前面做一层反代）：

```nginx
location /hera {
    proxy_pass http://127.0.0.1:10093/;
    proxy_connect_timeout 30;
    proxy_read_timeout 30;
    proxy_send_timeout 30;
    ...
}
```

三个超时都是 30 秒，比 RecordDownload 模式 `stop()` 实际耗时（38.5s，随录制时长增长）短。nginx 在 30 秒等不到 upstream 响应就会主动断开连接；daemon 完全不知道连接已经被 nginx 掐断，等真正处理完（38.5s 后）才发现写不回去，日志里打出 "Broken pipe"。浏览器侧则在约 30 秒时先一步收到 nginx 断连的信号，表现为前端的"连接断开"提示。

**数据本身没有任何问题**——录制停止、下载、重命名、session json 全部正常完成，17 秒后的"重复调用"只是前端/操作者又点了一次，correctly 报了"已经停止"，无害。

### 修复状态：**未做，待用户确认**

建议把 `/hera` location 的三个超时调大（比如 300 秒），一行 nginx 配置改动，无需碰 daemon/前端代码。这一步截至记录时**尚未执行**——录制时长越长（比如后续 60s+ 标定动作），下载耗时越长，越容易复现，需要尽快处理。

### 验证方法

```bash
ssh <jetson> 'echo <sudo密码> | sudo -S nginx -T 2>&1 | grep -A10 "location /hera"'
```

改完后 `sudo nginx -t && sudo systemctl reload nginx`，再做一次长录制验证 `stop()` 不再触发 Broken pipe。

## 问题二：`.hera`（陀螺仪+激光雷达）录制窗口比 SD 卡视频晚开始

### 发现过程

排查问题一之后，用户要求验证一次新采集的标定数据（`/home/fred/Data/0711/02`）。跑 `multi_source_synchronizer` 之前，先用 `HeraFile(...).info()` 看这个 `.hera` 文件本身的起止时间，跟 session json 里记录的 SD 卡视频起止时间对比：

```
.insv 视频（SD卡录制）  ：19:33:12.877 → ~19:34:23.35   （70.47s）
.hera（陀螺仪+激光雷达）：19:33:38.519 → 19:34:24.031   （45.51s）
```

`.hera` 开始时间比视频晚了 **25.6 秒**；结束时间倒是跟 `stop()` 调用时刻（19:34:24.027）几乎完全吻合（差 4ms），说明结束是对的，只有开始滞后。

### 根因

前端 `AcquisitionControl.vue` 有两个独立开关：

```pug
van-cell(title='采集开关')   → onClickSwitchStart → daemon.start()/stop()
van-cell(title='录制数据')   → onClickSwitchRecord → daemon.setRecord(bool)
```

（`client/src/views/Home/AcquisitionControl.vue:4-20`）

`setRecord` 是 daemon **全局**的开关（`daemon/src/service/service_control.cpp:179-198`），对所有已连接设备一视同仁地调用 `device->record(on)`，写不写盘由这个全局 `recording_`/`is_record_` 决定（`fetch_thread_function()` 里 `storage_->add_data(id, data, is_record_)`）。

但 Insta360 RecordDownload 模式的 SD 卡录制是相机自己的行为：只要 `WorkMode=RecordDownload` 且 `AutoStartRecording=true`，"采集开关"一打开、`connect()` 一完成就立刻触发相机的 `StartRecording()`，跟这个全局 `recording_`/`setRecord` 完全无关。

结果就是：两个开关操作的时间差 = `.hera` 窗口比 SD 卡视频晚开始的时间差。这次实测差了 25.6 秒，恰好这次运气好，剩下 45.5 秒的窗口里仍包含了足够的标定动作，`multi_source_synchronizer` 校准通过；但如果标定动作是从"采集开关"打开就立刻开始做的，前 25.6 秒的动作会完整地在 `.insv` 原始视频里，却完全不在 `.hera`（陀螺仪+激光雷达）里——而后者才是时间同步算法真正用得上的数据。

### 修复

daemon 侧新增一个 `Device` 虚方法，让设备插件可以声明"我在 connect() 时就已经自己开始采集了，请在 start() 成功后自动帮我把全局录制打开"：

- `device/include/device.hpp`：新增 `virtual bool wants_auto_record() const noexcept { return false; }`（默认关闭，其他插件不受影响）
- `device/plugin/camera/insta/plugin_entry.cpp`：`DevicePlugin::wants_auto_record()` 在 `WorkMode==RecordDownload && AutoStartRecording==true` 时返回 `true`
- `daemon/src/service/service_control.cpp` 的 `Service::start()`：成功启动所有设备后，检查是否有设备 `wants_auto_record()`，若有则跟 `setRecord(true)` 走一样的逻辑（`recording_ = true` + 对所有设备调用 `record(true)`），日志打 `"Daemon:: auto-starting record (device requested)"`

这样"采集开关"打开的那一刻，`.hera` 持久化和 SD 卡录制就是同一时刻触发的，不再依赖操作员手动对齐两个开关。Stream 模式（不满足 `wants_auto_record()` 条件）和其他设备插件（默认返回 `false`）行为不变，仍然需要手动 `setRecord`。

### 验证方法

真机部署（`build_arm64`/`build_jetson`，注意插件要放到 `/usr/local/lib/hera/plugin/driver/`，不是 flat 路径）后做一次 RecordDownload 会话，只点"采集开关"、不碰"录制数据"，用 `HeraFile(...).info()` 确认 `.hera` 的 `timestamp_start_ns` 跟 session json 的 `record_start_host_ns` 差在 1 秒以内（正常的 connect/StartRecording 时序抖动，而不是几十秒的操作员延迟）。

## 关键教训

1. 长耗时同步 RPC（这里是 `stop()` 里的 USB 文件下载）如果前面有反向代理，代理自己的超时要按最坏情况（最长录制时长对应的下载时间）设置，不能假设跟后端服务的超时一致——两边独立配置，容易一边改了一边忘。
2. 排查"数据缺失/窗口不对"类问题时，不要只看单个文件内部是否自洽，要跟同一次会话里**其他产出物的绝对时间戳**交叉验证（这里是 `.hera` 容器头的 `timestamp_start_ns/end_ns` vs session json 里 SD 卡视频的 `record_start_host_ns`/`stop()` 调用时刻）——只看 `.hera` 自己内部的数据是完全自洽的（陀螺仪和激光雷达两路在同一个文件里，起止时间必然一致），单独看意识不到"整个文件都启动晚了"。
3. 两个独立开关分别控制"设备连接/相机自身录制"和"daemon 持久化"，在架构上是合理的（允许连接后预览而不落盘），但只要有一个设备的采集行为不受 daemon 全局录制开关约束（这里是相机 SD 卡录制由自己的固件状态机决定），就会出现两者"各自为政"的时间差——这种情况需要设备插件主动声明意图（`wants_auto_record()`），而不是指望前端操作员精确同步点两个按钮。
