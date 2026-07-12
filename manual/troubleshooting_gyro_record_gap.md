# "采集开关"断连 + 陀螺仪/激光雷达录制窗口滞后 排查记录

记录 2026-07-11～07-12 两次真机排查：第一天是"采集完点击'采集开关'前端报连接断开"+"`.hera` 窗口比 SD 卡视频晚开始 25.6 秒"；第二天是"WiFi 热点模式下文件名时间戳不对"+"不点'录制数据'也会下载视频"，以及排查过程中暴露的一次部署事故（插件 vtable 不一致导致 daemon 反复崩溃）。

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

### 修复状态：**已修复（2026-07-12）**

把 `/etc/nginx/sites-enabled/hera-client` 里 `/hera` location 的 `proxy_connect_timeout`/`proxy_read_timeout`/`proxy_send_timeout` 从 30s 调到 300s，`nginx -t && systemctl reload nginx` 生效。

排查时踩了个坑：这台机器上同时存在 `/etc/nginx/hera-client`（未被引用的旧文件）和 `/etc/nginx/sites-enabled/hera-client`（真正生效的那份，`nginx.conf` 里 `include sites-enabled/*` 引用的是它），一开始改错了前者。另外备份文件如果直接放在 `sites-enabled/` 目录里，会被同一个通配符 include 当成第二个 server 块加载，报 "conflicting server name" 警告——备份应该放到 `sites-enabled/` 之外（比如 `/etc/nginx/backups/`）。

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

### 修复尝试：自动关联两个开关（2026-07-12 已回退）

daemon 侧一度新增过一个 `Device` 虚方法 `wants_auto_record()`，让 Insta360 插件在 `WorkMode==RecordDownload && AutoStartRecording==true` 时声明"我在 connect() 时就已经自己开始采集了，请在 start() 成功后自动帮我把全局录制打开"，`Service::start()` 据此自动 `recording_ = true`。

**这个方案后来被回退了**：它跟另一个明确的产品需求直接冲突——"只点'采集开关'、不点'录制数据'时，关闭'采集开关'不应该下载视频"（见 [insta_livox_data_spec.md](insta_livox_data_spec.md) 的相关记录）。自动打开"录制数据"意味着 RecordDownload 模式下这个开关**从来没有真正处于"关"的状态过**，导致"没录制就不下载"这条判断永远不成立。两个需求没法同时用这一种机制满足，权衡后优先保留"录制数据"作为操作员可信的手动开关，代价是这个陀螺仪窗口滞后的问题**重新变回操作层面的注意事项**（见下）。

### 当前状态：操作规范，不是代码修复

"采集开关"和"录制数据"两个开关要**几乎同时**打开（先确认两个开关都亮着，再开始做标定动作），不要先点采集开关就开始晃动相机——这是当前唯一的应对方式。如果之后又需要解决这个滞后问题，思路上要避开"自动强制打开录制"这条路（会跟上面的下载判断冲突），可以考虑做成"提醒但不强制"（比如'采集开关'打开时前端自动帮操作员把'录制数据'开关的 UI 状态勾上，但仍允许操作员在真正开始录制前手动取消，取消了才是真正的"不录制"）。

## 问题三：不点"录制数据"，关闭"采集开关"也会下载视频（2026-07-12，已修复）

### 现象

RecordDownload 模式下，只打开"采集开关"、全程不碰"录制数据"，关闭"采集开关"时 `.insv` 依然被下载了下来。

### 根因

`disconnect()` 里判断要不要下载，用的是 `runtime_auto_download_`（一个静态配置开关，默认 `true`），跟"录制数据"/`is_record_` 完全没关系——SD 卡录制由 `AutoStartRecording` 独立驱动，只要 `AutoDownload=true`（默认），`disconnect()` 就会无条件下载，不管操作员有没有碰过"录制数据"。

### 修复

在 `disconnect()` 判断下载与否时改成看"这次会话里录制数据是否真的被打开过"。但 `Device::stop()`（基类）在调用 `disconnect()` 之前就已经把 `is_record_` 强制清成了 `false`，所以 `disconnect()` 里直接读 `get_record()` 拿到的永远是 `false`，没法用。改法是让一直在跑的 `fetch()`（`is_record_` 被清掉之前它还在跑）顺手锁存一个新的 `recording_ever_enabled_` 标志位，`disconnect()` 改成看这个标志位：

```cpp
// fetch() 里
if (get_record()) {
    recording_ever_enabled_.store(true);
}

// disconnect() 里
const bool should_download = runtime_auto_download_.load() && recording_ever_enabled_.load();
```

`connect()` 开头重置 `recording_ever_enabled_.store(false)`，保证每次会话独立判断。真机验证：`Insta: 录制数据 was never enabled this session, skipping download` 日志确认生效。

**注意**：这个修复跟"问题二"里当时的自动关联方案（`wants_auto_record()`）直接冲突——自动关联会让"录制数据"从不处于"关"状态，这里的判断就永远不成立。两者只能二选一，已经按上文"问题二"的记录回退了自动关联，保留这个修复。

## 问题四：WiFi 热点模式下文件名时间戳不对（2026-07-12，已修复）

### 根因

Jetson 的 RTC（`nvvrs-pseq-rtc`）开机时不提供正确时间（观测到开机时系统时钟是 "Jan 01"），依赖 `systemd-timesyncd` 联网 NTP 校准。Jetson 作为 WiFi 热点给手机/电脑控制采集时，它自己没有上行外网，NTP 永远同步不上，系统时钟会一直停在"上次成功同步时缓存下来的时间戳"上（可能是几小时甚至几天前），`.hera`/`.insv` 文件名（来自 `Service::start()` 里的 `time::Timestamp::now()`）因此不准。

### 修复：客户端浏览器兜底校时

`start()` RPC 新增一个可选参数 `clientEpochMs`（毫秒，`double` 类型——特意不用 `i64`，避免生成的 TS 客户端要引入 `Int64` 包装类型），前端在调用 `start()` 时顺手把浏览器自己的 `Date.now()` 带过去：

```ts
// client/src/store/modules/acquisitionControl.ts
const data = await client.start(Date.now());
```

daemon 侧 `Service::start()` 一收到就跟系统时钟比较，drift 超过 5 秒（`kClockCorrectionThresholdMs`）才通过 `clock_settime(CLOCK_REALTIME, ...)` 纠正（避免正常 NTP 抖动/RPC 延迟触发不必要的跳变），顺手 `hwclock --systohc` 尽量把这个较准的时间也写回硬件 RTC。手机/电脑本身的时钟基本可信，这是目前这套系统里最省成本的可信时间来源。

## 部署事故：只重新部署了 insta 插件，导致 daemon 反复崩溃（2026-07-12）

### 现象

改完"问题三"的修复后部署到 Jetson，daemon 反复 `*** stack smashing detected ***` → `SIGSEGV` → 被 `Restart=on-failure` 拉起 → 再崩，循环往复。

### 根因

修 wants_auto_record() 相关代码时改过 `device/include/device.hpp` 里 `Device` 基类的虚函数声明顺序，这会改变虚函数表（vtable）布局。部署时图省事，只重新编译+拷贝了改动到的那部分（daemon 二进制 + insta 插件 `.so`），livox 插件的 `.so` 还是按旧 `device.hpp` 布局编译的旧版本。新 daemon（新 vtable 布局）+ 旧 livox 插件（旧 vtable 布局）混用，一旦 daemon 通过基类指针调用 livox 设备对象的虚函数，实际跳转的槽位就和 livox 插件里真正实现的对不上，直接内存越界——日志里两次崩溃都能看到 "Device lidar/livox/01 errored"，是这个不一致的直接体现。

### 修复

把这一次 build 产出的**全部**插件 `.so`（`device/driver/*.so` 和 `device/base/*.so`，不只是改动过的那个）都重新拷贝部署，daemon 立刻恢复稳定。

### 关键教训（追加到部署 checklist）

**只要改了 `device/include/device.hpp`（尤其是新增/删除/挪动虚函数声明），必须把所有插件 `.so` 一起重新部署，不能只挑改动相关的那几个。** 这跟改了 daemon 或某一个插件自己的 `.cpp` 完全不同——那种改动只影响自己的编译单元，可以只重新部署对应的产物；但基类头文件的虚函数表布局是所有继承它的插件共享的 ABI 约定，任何一个还在用旧布局的 `.so` 混进来，都会在运行时进程内产生内存越界，且报错信息（这里是 "lidar/livox errored" + stack smashing）通常跟真正改动的模块（insta 插件）看起来毫不相关，容易误导排查方向。

## 关键教训

1. 长耗时同步 RPC（这里是 `stop()` 里的 USB 文件下载）如果前面有反向代理，代理自己的超时要按最坏情况（最长录制时长对应的下载时间）设置，不能假设跟后端服务的超时一致——两边独立配置，容易一边改了一边忘。
2. 排查"数据缺失/窗口不对"类问题时，不要只看单个文件内部是否自洽，要跟同一次会话里**其他产出物的绝对时间戳**交叉验证（这里是 `.hera` 容器头的 `timestamp_start_ns/end_ns` vs session json 里 SD 卡视频的 `record_start_host_ns`/`stop()` 调用时刻）——只看 `.hera` 自己内部的数据是完全自洽的（陀螺仪和激光雷达两路在同一个文件里，起止时间必然一致），单独看意识不到"整个文件都启动晚了"。
3. 两个独立开关分别控制"设备连接/相机自身录制"和"daemon 持久化"，在架构上是合理的（允许连接后预览而不落盘）。但两个开关各自服务的产品需求可能互相冲突（这里是"自动对齐时间窗口" vs "不录制就不下载"），遇到这种情况不要急着用一种机制强行合并两个开关的行为，先跟需求方确认清楚哪个更优先。
4. 改动一个被多个独立编译单元（多个插件 `.so`）共同继承/包含的基类头文件时，虚函数声明的增删改都会牵扯到 ABI（vtable 布局），部署 checklist 里要把"头文件改动 → 全量重新部署"作为硬性规则，不能靠"感觉只影响这一个模块"来决定部署范围。
