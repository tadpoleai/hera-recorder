# Hera 新传感器接入分析（Insta + Livox）

## 1. 目标与结论

本文基于当前仓库源码，完成了以下事项：

1. 梳理 Hera（重点是 recorder）的代码结构与设计思路。
2. 深读 `device/plugin` 插件机制与典型实现（camera/flir、camera/s32vsal、lidar/velodyne、external/android）。
3. 阅读 `docs/insta_sdk` 与 `docs/Livox-SDK2`，分析接入 Insta 相机和 Livox 激光雷达所需工作。
4. 在插件目录下新增了两处接入入口目录：
   - `device/plugin/camera/insta`
   - `device/plugin/lidar/livox`

---

## 2. Hera 的设计思路（结合源码）

### 2.1 分层与职责

以 `hera/recorder` 为核心，架构按职责分层：

- `common`：日志、时间、IPC、基础工具。
- `device`：传感器抽象与插件系统（本次重点）。
- `storage`：`.hera` 文件写入与读取。
- `daemon`：服务端进程，负责配置、生命周期控制、RPC、状态输出。
- `client`：Web 客户端。
- `convert`：离线把 `.hera` 转成 ROS 消息/Bag。
- `replay`：回放 `.hera` 数据到 IPC。

整体策略是：

- 采集期尽量保存“原始设备数据（DeviceData）”，降低在线处理负担。
- 通过统一转换层（DeviceData -> SensorData）屏蔽厂商差异。
- 插件动态加载，业务层只按 `type=name` 使用，不硬编码厂商驱动。

### 2.2 设备抽象：DeviceData 与 SensorData 解耦

- `DeviceData`：面向“设备原始数据”存储，带 `device_vendor_type` 和 `message_type`。
- `SensorData`：面向“统一语义数据”（例如 CompressedImage、Points）。

这意味着新增设备时优先复用现有 `SensorDataType`（例如相机图像、点云），
只有在语义无法覆盖时才新增 `SensorDataType` 并连带更新 display/convert。

### 2.3 插件化机制：自动发现 + 宏导出 + 双库产物

`device/CMakeLists.txt` 会递归扫描 `plugin/**/plugin_entry.cpp`，并对每个插件：

1. 校验 `HERA_PLUGIN_DEFINE_START("category/vendor", 0xXXXX, history_depth)` 的名称和类型唯一性。
2. 校验 `plugin_data.hpp` 里 `HERA_PLUGIN_DATA_DEFINE_START(DataType, 0xYYYY)` 的数据类型唯一性。
3. 通过 `plugin_param.src` 自动生成参数定义头（`plugin_param.hpp`）。
4. 输出两套动态库：
   - base 版：不带真实驱动，保留转换能力。
   - driver 版：带真实驱动（`WITH_DRIVER=1`）。

`Factory::load_plugins()` 运行时从插件目录 `.../plugin/base` 或 `.../plugin/driver` 动态加载，读取每个库的 `exports()` 完成注册。

### 2.4 设备生命周期与并发模型

`device::Device` 基类将采集逻辑统一为：

1. `start()`：校验参数 -> `connect()` -> 启 fetch 线程（可选 forward 线程）。
2. `fetch_thread_function()` 循环调用子类 `fetch()`：
   - 非空数据写 storage。
   - 若 forward 开启，进队列再转发 IPC。
3. `forward_thread_function()`：`convert(storage_data)` 后写 IPC。
4. `stop()`：终止线程并 `disconnect()`。

设计要点：

- 子类 `fetch()` 必须“可超时返回”，否则 stop 会阻塞。
- 对异步 SDK，通常需要“回调线程 + 本地队列 + fetch 阻塞取队列（超时）”适配。

### 2.5 参数系统设计

参数由 `plugin_param.src` 声明（immutable/mutable/convert、范围、依赖、枚举等），
自动生成 `LocalParameters`，并在 daemon 的 `meta` 接口中返回给前端。

这使得新插件参数既是运行配置，也是 UI/接口契约。

---

## 3. 新传感器接入 Hera 的标准步骤

以下步骤是对现有实现方式的归纳，适用于大多数新设备：

1. 选择插件命名和 ID
   - 例如 `camera/insta`、`lidar/livox`。
   - 规划唯一 `DeviceVendorType`（插件级别）和 `DeviceDataType`（原始数据级别）。

2. 创建插件目录与基础文件
   - `plugin_entry.cpp`
   - `plugin_data.hpp`
   - `plugin_param.src`
   - `plugin.cmake`（可选，外部 SDK 检测）

3. 定义参数与连接策略
   - 在 `plugin_param.src` 定义必选参数（IP/端口/序列号/模式/同步等）。
   - 区分 `immutable`（启动前设置）与 `mutable`（运行中可调）。

4. 实现驱动态逻辑（WITH_DRIVER）
   - `connect()`：初始化 SDK、建立连接、启动数据。
   - `fetch()`：同步返回 `DeviceData`（可超时）。
   - `disconnect()`：释放资源。
   - `adjust_parameter()`：仅处理运行中可调参数。

5. 实现转换逻辑（do_convert）
   - `DeviceData` -> 既有 `SensorData`（优先复用 `CompressedImage/Image/Points/...`）。
   - 如果必须新增 `SensorDataType`，需同步改 display 与 convert(ROS)。

6. CMake 与依赖收敛
   - 在 `plugin.cmake` 检测 SDK 头库是否可用，设置 `PLUGIN_DRIVER_FOUND`。
   - 指定 `PLUGIN_DRIVER_DEP_INCLUDE/PLUGIN_DRIVER_DEP_LIBS`。

7. 接入 daemon 配置与联调
   - 前端/配置可见新 `type`。
   - 启动后能创建、参数生效、采集可持续、停止可快速返回。

8. 离线链路验证
   - `.hera` 可写可读。
   - `convert` 可得到预期 ROS topic/消息。

9. 稳定性与边界测试
   - 断连重连、无数据超时、时间戳异常、参数非法值、长时间录制。

---

## 4. 已新增目录（本次）

已在插件目录增加以下两处入口（当前是 scaffold）：

- `device/plugin/camera/insta`
- `device/plugin/lidar/livox`

---

## 5. Insta 接入分析（camera/insta）

### 5.1 从 SDK 文档可确认的信息

1. 连接方式：USB，且相机需切 Android 模式。
2. 支持平台：重点是 Ubuntu 22.04 x86_64、Windows x64。
3. 预览流接口：`StreamDelegate::OnVideoData` 回调推送码流。
4. 码流格式：H264/H265（并非直接 JPEG/RGB），分辨率高时可能双路 fisheye 流。
5. 还有 Gyro/Exposure 回调可用。

### 5.2 与 Hera 当前模型的匹配

可行，但需要一个“异步回调到同步 fetch”的适配层：

- SDK 回调线程收到数据后推入无锁队列/环形队列。
- `fetch()` 以超时方式从队列取一帧并封装 `DeviceData`。

### 5.3 数据落地策略（关键决策）

当前 Hera 相机通道最兼容的是：

- `SensorDataType::CompressedImage`（JPEG/PNG）
- `SensorDataType::Image`（原始图像）

而 Insta 预览原生是 H264/H265。可选方案：

方案 A（推荐）：在线解码并转 JPEG，再写入 `CompressedImage`
- 优点：复用现有链路（display/convert）。
- 缺点：CPU 开销增加，需要引入解码依赖（FFmpeg/GStreamer/硬解）。

方案 B：新增 H264/H265 原始码流型 SensorDataType
- 优点：保留原始码流，CPU 压力低。
- 缺点：要扩展 display/convert/回放兼容，改动面更大。

### 5.4 camera/insta 插件建议参数

启动参数（immutable）：
- 设备选择（SerialNumber）
- 分辨率/帧率
- 是否启用音频
- 是否双流
- 时间同步策略

运行参数（mutable）：
- 码率
- 曝光/ISO/白平衡（若 SDK 允许动态）

转换参数（convert）：
- 输出格式（compressed/image）
- 解码线程数

### 5.5 Insta 仍需完成的工作

1. SDK 头文件和动态库在目标机的安装规范（路径、版本、授权）
2. 插件 `plugin.cmake`（检测 SDK）
3. `plugin_entry.cpp`：连接、预览启动、回调入队、fetch 超时
4. `plugin_data.hpp`：定义 Insta 原始帧结构
5. `do_convert`：映射到 `CompressedImage` 或 `Image`
6. 时间戳统一（SDK timestamp 与系统 UTC 映射）
7. 断开重连、异常恢复、长稳测试

---

## 6. Livox 接入分析（lidar/livox）

### 6.1 从 SDK 文档可确认的信息

1. SDK2 提供 C 风格 API，可注册点云和 IMU 回调。
2. 支持 HAP/MID360，依赖配置 JSON（host_ip、端口、lidar_ip 等）。
3. 点云数据通过 `LivoxLidarEthernetPacket` 上报，点类型有高精度笛卡尔/低精度/球坐标。
4. 可设置工作模式、点类型、扫描模式等。

### 6.2 与 Hera 当前模型的匹配

非常匹配点云插件范式（类似 velodyne/hesai）：

- 回调拿包 -> 入队
- `fetch()` 出队封装 `DeviceData`
- `do_convert()` 转成 `data::Points`

可优先复用 `SensorDataType::Points`，不一定需要新增 SensorDataType。

### 6.3 重点实现点

1. 多设备 handle 与 Hera 单设备实例的绑定策略。
2. 时间戳处理：
   - 解析 `time_type/timestamp`，按同步模式（full/local/disabled）统一到 Hera 时间语义。
3. 点格式统一：
   - Livox 原始点 -> `Points::PointXYZCIDPAT`。
4. 可选输出：
   - XYZI / XYZIRT。
5. IMU 数据：
   - 作为同一插件额外 `DeviceDataType` 输出，或拆分到独立 `imu/livox`（需决策）。

### 6.4 lidar/livox 插件建议参数

启动参数（immutable）：
- 配置文件路径或 host/lidar ip 与端口
- 设备 SN/过滤规则
- 点数据类型（high/low/spherical）
- 同步模式

运行参数（mutable）：
- 工作模式/扫描模式（若支持在线切换）

转换参数（convert）：
- 点格式（XYZI/XYZIRT）
- 是否累计成帧、分圈策略

### 6.5 Livox 仍需完成的工作

1. `plugin.cmake` 与 SDK 探测（头文件 + lib）
2. `plugin_entry.cpp`：初始化、回调、出队、断连处理
3. `plugin_data.hpp`：定义 Livox 原始包结构（必要字段）
4. `do_convert`：映射到 `data::Points`（可选 IMU）
5. 时间同步与异常包策略
6. 多雷达并发与性能压测

---

## 7. 已确认决策（2026-04-14）

1. 开发顺序：
   - Phase 1：先打通采集链路（稳定采集落盘）。
   - Phase 2：做 convert 输出（内部工具链 + ROS），并实现关键时间对齐。
   - Phase 3：若 Phase 2 需要 Insta 高清原始影像，在工具中提供可选项。
2. Insta 侧数据形态：保存原始 H264/H265 码流。
3. 时间对齐目标精度：5ms。
4. Mid360 侧数据策略：保存原始点包；其他传感器同样优先保存原始信息，最小解析原则为“先只解析时间戳，其他字段尽量原样保存”。
5. 阶段验收优先级：稳定采集落盘优先。
6. Insta 不走 external 路径，按实时采集插件路径实现。

---

## 8. 仍需关注但不阻塞开工的问题

以下问题可在实施中并行确认，不阻塞 Phase 1 开始：

1. Insta 与 Livox 的最终 DeviceVendorType/DeviceDataType 编号分配规范（避免历史冲突）。
2. Insta SDK 和 Livox SDK2 在目标环境中的安装路径、版本固化方式、发布打包策略。
3. 如果后续需要在线可视化而非仅落盘，是否在 convert 或工具链阶段引入解码依赖。
4. IMU 对齐算法采用在线互相关还是离线批处理为主（建议先在线粗对齐 + 离线精对齐）。

---

## 9. Phase 2 第一阶段（已落地）

已在 `hera-storage-tool` 增加 Livox 原始数据统计能力（按 `.hera` 文件离线扫描）：

- 统计点包（`0x0521/0x0522/0x0523`）和 IMU 包（`0x0524`）的包数与字节数。
- 统计首尾接收时间戳（ns）、时长（s）和平均频率（Hz）。
- 提供全局统计与按 `device_id` 统计。

命令示例：

```bash
./build_livox_phase1/storage/hera-storage-tool -i <record.hera> -p
```

说明：

- `-p` 模式与 `-b`（重建头）或 `-m`（裁剪）互斥。
- 该阶段先完成“内部工具链统计闭环”，为下一步 ROS 导出和离线时间对齐提供输入质量评估依据。

---

## 9. 可执行步骤（按阶段）

### 9.1 Phase 1：打通采集链路并稳定落盘

1. 定义原始数据结构与参数
   - camera/insta：定义视频码流包与 IMU 包的 DeviceData 结构，增加 plugin_param。
   - lidar/livox：定义点云原始包与 IMU 包的 DeviceData 结构，增加 plugin_param。
2. 实现 SDK 采集适配
   - 两插件都采用“SDK 回调入队 + fetch 超时出队”模型。
   - 确保 stop 可快速返回，避免线程卡死。
3. 时间戳最小解析
   - 每包至少记录 device timestamp 与 host receive timestamp。
   - 原始负载尽量保持不变写入 .hera。
4. daemon 侧联调
   - 在 profile 中能创建 camera/insta 与 lidar/livox。
   - start/stop/record 行为稳定。
5. Phase 1 验收
   - 连续录制稳定，无明显丢线程/死锁。
   - .hera 文件中可回读到 Insta 码流+IMU、Livox 点包+IMU。

### 9.2 Phase 2：convert 输出与关键时间对齐

1. convert 增加两类输出
   - 内部工具链输出：保留原始码流/点包导出能力。
   - ROS 输出：
     - Livox -> PointCloud2 + IMU。
     - Insta -> 先支持码流导出与时间轴对齐；图像解码能力按需求扩展。
2. 实现 IMU 对齐模块
   - 以 IMU 为公共时间桥，估计 Insta 与 Livox 时间偏移。
   - 目标误差 <= 5ms（给出统计报告：均值/方差/最大误差）。
3. Phase 2 验收
   - 两源数据在统一时间轴下可对齐。
   - ROS/内部工具链都能得到可用输出。

当前状态（2026-04-14）：

1. 已实现 Livox 点包到 `SensorDataType::Points` 的最小转换（支持高精度/低精度笛卡尔点型），并保留 Livox IMU 到 `ImuMagneticField`。
2. 已在 `hera-convert` 增加插件预加载（支持通过 `HERA_DEVICE_PLUGIN_PATH` 使用本地编译的 livox base 插件）。
3. 当前机器缺少 ROS（kinetic/melodic/noetic 之一），因此 `WITH_CONVERT=ON` 配置会被 ROS 依赖阻断；需要在 ROS 环境机完成最终 `hera-convert` 编译与 bag 导出验证。

Insta Phase 1 当前状态（2026-04-15）：

1. 已新增 `camera/insta` 插件骨架，采集链路采用 `StreamDelegate` 回调入队 + `fetch` 超时出队模式。
2. 已定义并落盘两类原始 `DeviceData`：`InstaVideoPacket(0x0421)` 与 `InstaGyroPacket(0x0422)`。
3. `do_convert` 暂保持最小实现（返回 `broken_data`），以满足“Phase 1 稳定落盘优先”。
4. 已在 `hera-storage-tool` 增加 Insta 统计模式 `-a`，可输出 video/gyro 的包数、字节数、时间范围与频率。
5. 驱动插件编译依赖 Insta SDK（`camera/camera.h` + SDK 库），通过 `INSTA_SDK_ROOT` 指定；缺 SDK 时仅构建 base 插件。

### 9.3 Phase 3：可选高清原始影像支持

1. 在工具链中提供“是否拉取/处理 Insta 高清原始影像”的选项。
2. 兼容两种模式
   - 仅使用实时码流。
   - 引入高清原始影像进行增强处理。
3. Phase 3 验收
   - 开关可控，不影响默认稳定采集链路。

---

## 10. Mid360 开工前建议（针对问题 1 和问题 2）

### 10.1 问题 1：类型编号分配建议

建议采用“按类别分段 + 按厂商预留”的固定规则，避免后续冲突。

1. 维持现有含义：`0x05xx` 作为 lidar 相关编号段。
2. 为 livox 预留段：`0x052x`。
3. 插件级 DeviceVendorType 建议：
   - `lidar/livox` -> `0x0521`
4. DeviceDataType 建议：
   - `LivoxPacketFullSynced` -> `0x0521`
   - `LivoxPacketUnSynced` -> `0x0522`
   - `LivoxPacketLocalSynced` -> `0x0523`
   - `LivoxImuPacket` -> `0x0524`
5. 规则约束：
   - 不重用旧值，不改已发布值。
   - 新值只增不改，按文档登记。
   - 若未来需要 Mid360s/HAP 区分，可继续使用 `0x0525+`。

### 10.2 问题 2：SDK 安装路径与 CMake 搜索建议

你将在本机安装 SDK，建议使用“可配置根路径 + 标准路径兜底”策略。

1. CMake 变量优先级（从高到低）：
   - `-DLIVOX_SDK2_ROOT=/your/path`
   - 环境变量 `LIVOX_SDK2_ROOT`
   - 系统默认 `/usr/local`
2. 插件 `plugin.cmake` 检测逻辑建议：
   - 检查 `${LIVOX_SDK2_ROOT}/include/livox_lidar_api.h`
   - 检查 `${LIVOX_SDK2_ROOT}/lib/liblivox_lidar_sdk_shared.so`（或同名静态库）
   - 两者存在再置 `PLUGIN_DRIVER_FOUND=1`
3. 发布建议：
   - Phase 1 先固定一个可用版本（例如你本机安装版本），记录到文档。
   - 后续再做多版本兼容，不在 Phase 1 扩散复杂度。

---

## 11. Mid360 的 Phase 1 具体实现步骤（仅 Mid360）

### 11.1 目标

实现 `lidar/livox` 插件在 Hera 中稳定实时采集并落盘，保存原始点包与 IMU 包，最小解析仅处理时间戳和必要头字段。

### 11.2 实施步骤

1. 新建并完善插件文件
   - `device/plugin/lidar/livox/plugin_entry.cpp`
   - `device/plugin/lidar/livox/plugin_data.hpp`
   - `device/plugin/lidar/livox/plugin_param.src`
   - `device/plugin/lidar/livox/plugin.cmake`
2. `plugin_param.src` 先最小化参数
   - `ConfigPath`（Livox JSON 配置路径）
   - `SyncType`（Full/Local/Disabled）
   - `DeviceSn`（可选过滤）
   - `EnableImu`（true/false）
3. `plugin.cmake` 完成 SDK 检测
   - 按 10.2 的路径优先级查找头库
   - 设置 `PLUGIN_DRIVER_DEP_INCLUDE` 与 `PLUGIN_DRIVER_DEP_LIBS`
4. `plugin_entry.cpp` 实现采集生命周期
   - `connect()`：初始化 SDK、注册点云/IMU回调、启动数据流
   - 回调：将原始包写入线程安全队列
   - `fetch()`：带超时从队列取包并创建对应 DeviceData
   - `disconnect()`：停止 SDK、清队列、释放资源
5. `plugin_data.hpp` 定义原始数据结构
   - 点包：保存 Livox 原始包头 + 原始负载
   - IMU 包：保存 Livox 原始 IMU 负载
   - 两者都保留 device_ts 与 host_rx_ts
6. `do_convert()` 先做最小可用
   - Phase 1 可先把非关键转换保持最小，核心是稳定落盘
   - 对错误包返回 `broken_data`
7. daemon 配置联调
   - 配置一个仅含 `lidar/livox` 的 profile
   - 验证 start/stop/record 循环稳定
8. 验收测试
   - 连续录制（建议 >= 30min）
   - 统计掉包、线程退出耗时、文件可读性
   - 核对 `.hera` 中点包与 IMU 包计数增长正确

### 11.3 Phase 1 完成判定

满足以下条件视为完成：

1. `lidar/livox` 插件可被 Factory 正常注册。
2. Mid360 点包与 IMU 包可稳定写入 `.hera`。
3. 停止采集无阻塞，重复启动稳定。
4. 原始负载保真，时间戳字段可用于后续离线对齐。
