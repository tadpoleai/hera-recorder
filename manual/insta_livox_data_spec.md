# Insta360 X4 + Livox Mid360 采集数据规格

面向后续算法验证（时间同步、SLAM、拼接等）的数据参考。目的是让不熟悉本仓库历史背景的人（含 AI agent）能快速定位：一次采集会话产出了哪些文件、每个字段的含义/单位/坐标系，以及已知的坑。

`.hera` 底层二进制容器格式（StorageHeader/StorageDataArray, V3/V4）见 [recordfile.md](recordfile.md)；本文只讲两类设备各自的 payload（DeviceData）格式和实际采集配置。

## 1. 硬件与当前采集配置

### 1.1 Insta360 X4（`device/plugin/camera/insta`）

- 工作模式：`WorkMode = RecordDownload`（默认）。Stream 模式机内拼接在 X4 固件上实测不可用（`EnableInCameraStitching` 不响应，录制格式始终为双鱼眼），因此不作为采集方案。
- 录制分辨率：`RecordResolution`，两档：
  - `R57K`（默认）→ SDK `RES_2880_2880P30`，双鱼眼 2880×2880 每镜头，数据量小，推荐
  - `R8K` → SDK `RES_3840_3840P30`，双鱼眼 3840×3840 每镜头，画质更好但文件大得多
- 码率：`VideoBitrate`，默认 10485760（10Mbps）
- 编码：机内始终为双鱼眼（不开启机内拼接），录制文件是相机自己写入 SD 卡的 `.insv`（H.264/H.265，具体由相机固件决定）
- SD 卡文件命名：相机自己按内部时钟命名 `VID_YYYYMMDD_HHMMSS_00_NNN.insv`；daemon 下载后会重命名为 `<hera 会话 basename>.insv`（多段追加 `_00`/`_01`），见 §4
- 陀螺仪：RecordDownload 模式下由一路**并发的 gyro-only `StartLiveStreaming`**提供（`RES_1440_720P30`，只开陀螺仪，不影响 SD 卡录制），实测速率约 478–500Hz（有抖动，非严格恒定）
- 拼接：SD 卡下载的原始 `.insv` 双鱼眼视频，通过 `hera-storage-ingest-insta-video`（MediaSDK VideoStitcher）离线拼接成等距柱状投影（equirectangular）JPEG 帧序列，写回一个新的 `.hera`

### 1.2 Livox Mid360（`device/plugin/lidar/livox`）

- 配置文件：`/etc/mid360_config.json`
- `SyncType = Full`，`EnableImu = true`
- IMU 速率：约 200Hz（实测 8899 samples / 44.49s ≈ 200Hz）
- 点云：按 Livox 原生格式持续输出，本文档不追求给出精确点/秒指标（跟扫描模式、有效距离内点数相关，会话间会有差异）

## 2. `.hera` 内的设备数据包（DeviceData）格式

所有 DeviceData 有相同的 24 字节通用头：`uint32 length` + `uint32 device_id` + `uint16 vendor_type` + `uint16 msg_type` + `uint32 sequence` + `uint64 timestamp_receive_ns`（daemon 收到该条数据时的本机 host UTC 时间，纳秒）。以下都是这个头**之后**的 payload 布局。

### 2.1 Insta360（`device/plugin/camera/insta/plugin_data.hpp`）

| msg_type | 名称 | 说明 |
|---|---|---|
| `0x0421` | `InstaVideoPacket` | 原始 H264/H265 视频包（仅 Stream 模式产出；RecordDownload 模式下视频不走这个包，而是相机 SD 卡的 `.insv`） |
| `0x0422` | `InstaGyroPacket` | 陀螺仪批量数据包 |
| `0x0423` | `InstaJpegFramePacket` | 离线拼接产出的等距柱状 JPEG 帧（由 `hera-storage-ingest-insta-video` 写入，不是采集时实时产生的） |

```cpp
// 0x0421 InstaVideoPacket
int64_t  timestamp_device_ns;  // SDK 回调自带时间戳
uint64_t timestamp_host_ns;    // host 收到时间（绝对，可信）
uint8_t  stream_type;          // SDK stream type
int32_t  stream_index;         // 镜头/流序号
uint32_t payload_size;
uint8_t  payload[0];           // 原始 H264/H265 bytes

// 0x0422 InstaGyroPacket
uint64_t timestamp_host_ns;    // 整个批次的 host 收到时间（绝对，可信）——注意这是"批"的时间戳，不是每个样本的
uint32_t sample_count;         // payload 里样本数
uint32_t payload_size;
uint8_t  payload[0];           // sample_count 个 GyroSample 连续排列

// GyroSample（对应 Insta360 SDK ins_camera::GyroData，stream_types.h）
int64_t timestamp;   // ⚠️ 见下方"陀螺仪时间戳坑"，不是 host 绝对纳秒
double  ax, ay, az;  // 加速度计
double  gx, gy, gz;  // 陀螺仪

// 0x0423 InstaJpegFramePacket（离线拼接产物，非实时采集）
uint64_t timestamp_host_ns;    // = record_start_host_ns + 该帧在 mp4 里的 PTS，绝对可信
uint32_t frame_index;
uint32_t width, height;
uint32_t payload_size;
uint8_t  payload[0];           // JPEG bytes
```

**⚠️ 陀螺仪时间戳坑**：`GyroSample.timestamp` 字段名和 CSV 导出列名都叫 `timestamp_ns`，但实测**不是**纳秒，也**不是** host 绝对时间——是 Insta360 SDK 内部相对时钟，经验单位接近毫秒，从陀螺仪流开始时刻起算（用一次真实采集验证：21736 个样本，`timestamp` 首尾差 44495，若按毫秒算约 44.5s，跟同一个 `.hera` 文件用容器头算出的真实时长 45.512s 相差仅 ~2%，量级吻合；若按纳秒算则意味着采样率超过 500MHz，明显不合理）。要拿到**绝对**时间，只能用同一个包里的 `timestamp_host_ns`（每 ~45ms 一个包，不是每个样本一个）——但目前 `hera-storage-extract-insta --gyro` 导出的 CSV **没有导出这个字段**，只导出了样本级别的相对 `timestamp`。如果要做逐样本级别的绝对时间对齐（不只是找一个全局 offset），需要改这个导出工具，把每个包的 `timestamp_host_ns` 也带出来，再结合包内样本序号做线性插值。

### 2.2 Livox（`device/plugin/lidar/livox/plugin_data.hpp`）

| msg_type | 名称 | 说明 |
|---|---|---|
| `0x0521` | `LivoxPacket` (= `LivoxPacketFullSynced`) | 点云包，完整字段 |
| `0x0522` | `LivoxPacketUnSynced` | 类型占位，payload 布局跟 0x0521 一致（提取工具对三者一视同仁） |
| `0x0523` | `LivoxPacketLocalSynced` | 同上 |
| `0x0524` | `LivoxImuPacket` | IMU 数据包 |

```cpp
// 0x0521/0x0522/0x0523 点云包头
uint64_t timestamp_device_ns;  // Livox 设备自身时钟
uint64_t timestamp_host_ns;    // host 收到时间（绝对，可信）
uint32_t handle;
uint8_t  livox_dev_type;
uint8_t  livox_data_type;      // 0x01=高精度(mm) 0x02=低精度(cm)，决定下面点的格式
uint8_t  livox_time_type;
uint8_t  frame_cnt;
uint16_t dot_num;               // 本包点数
uint16_t packet_length;
uint32_t payload_size;
uint8_t  payload[0];            // dot_num 个 LivoxCartesianHighPoint 或 LivoxCartesianLowPoint

// 高精度点 (data_type=0x01)
int32_t x_mm, y_mm, z_mm; uint8_t reflectivity; uint8_t tag;
// 低精度点 (data_type=0x02)
int16_t x_cm, y_cm, z_cm; uint8_t reflectivity; uint8_t tag;

// 0x0524 LivoxImuPacket
uint64_t timestamp_device_ns;
uint64_t timestamp_host_ns;    // 绝对，可信
uint32_t handle;
uint8_t  livox_dev_type;
uint8_t  livox_time_type;
float    gyro_x, gyro_y, gyro_z;  // 单位据 Livox SDK 惯例为 rad/s（本仓库未显式注明，建议用前先用已知转速核对一次）
float    acc_x, acc_y, acc_z;     // 单位据 Livox SDK 惯例为 g（重力加速度倍数，不是 m/s²，融合算法里要自己乘 9.80665）
uint32_t payload_size;
uint8_t  payload[0];
```

Livox 每个数据点/IMU 样本都自带 `timestamp_host_ns`（绝对可信），这一点比 Insta360 陀螺仪的样本级时间戳完整得多。

## 3. CSV 导出工具字段说明

工具都在 `storage/tool/`，amd64 本地构建产物在 `build_amd64/storage/`（Jetson 部署用 `build_arm64`/`build_jetson`）。

### `hera-storage-extract-insta <in.hera> [--video out.h264] [--gyro out.csv]`

- `--gyro out.csv` 列：`timestamp_ns,ax,ay,az,gx,gy,gz` — `timestamp_ns` 见 §2.1 的坑，实际是相对毫秒量级的 SDK 内部时钟
- `--video out.h264`：只认 `InstaVideoPacket(0x0421)`。RecordDownload 模式的 `.hera` 里基本没有这个包（视频走 `.insv`），这个开关目前主要对 Stream 模式的老数据有用

### `hera-storage-ingest-insta-video --session <session.json> --output <out.hera> [--fps N] [--width W] [--height H] [--no-stitch]`

用 MediaSDK 把 `.insv` 双鱼眼离线拼接成等距柱状投影，写成新的 `.hera`（只含 `InstaJpegFramePacket`）。`--session` 要传 `insta_session.json`（不是 `.insv` 本身），里面记录了 `record_start_host_ns` 和 `mp4_files` 路径。**在非采集用的机器上处理时，`mp4_files`/`hera_session_path` 里是 Jetson 上的绝对路径，需要先改成本机路径的副本再喂给这个工具**（见 §5）。本机若无 GPU/CUDA/Vulkan，MediaSDK 走软件拼接路径也能工作，只是慢。

拼接产物用 `python3 -c "from hera import HeraFile; from hera.export.video import save_jpegs; ..."`（`hera-sdk-python` 包）导出成单帧 JPEG 便于人工核对图像质量。**注意**：`python3 -m hera.cli extract-jpegs ...` 目前跑了没反应也不报错——`hera/cli.py` 的 `main()` 缺一个 `if __name__ == "__main__": main()` 入口调用，是个死命令，需要绕开直接调 API。

### `hera-storage-extract-mid360 <in.hera> [--points out.csv] [--imu out.csv]`

- `--imu out.csv` 列：`timestamp_device_ns,timestamp_host_ns,gyro_x,gyro_y,gyro_z,acc_x,acc_y,acc_z`
- `--points out.csv` 列：`timestamp_device_ns,timestamp_host_ns,x_m,y_m,z_m,reflectivity,tag,data_type`（已从 mm/cm 转换成米）

### `multi_source_synchronizer <insta.hera> <mid360.hera> [--OutputOffset path]`

（独立仓库 `/home/fred/Code/multi_source_synchronizer`，直接读 `.hera` 原始包，不经过上面的 CSV）对两路陀螺仪的运动特征做互相关，求出 `offset_sec`，定义为 `t_mid = t_insta + offset_sec`。若两个 `.hera` 是同一个文件（insta 和 mid360 数据合并采集在一起，本仓库当前默认配置就是这样），两个参数传同一个文件即可，工具会自动识别里面各自的设备流。

需要至少几十秒、包含明显运动特征的重叠窗口，否则会报 "Outbound error is too low"（不是 bug，是数据不够，需要重新做一段更剧烈/更长的标定动作）。

## 4. 一次采集会话的文件产出与对应关系

一次 RecordDownload 会话结束后，`DownloadDir`（默认 `/var/hera/data/insta_insv`）下会有三个**同 basename** 的文件（`<时间戳>_<operator>_<place>`，跟 `.hera` 存储文件同名）：

```
20260711193312_fred_office.hera          # 陀螺仪 + Livox 点云/IMU（daemon 直接持久化）
20260711193312_fred_office.insv          # SD 卡下载的双鱼眼原始视频（daemon 下载后改名）
20260711193312_fred_office.session.json  # 关联元数据：record_start_host_ns / hera_session_path / mp4_files
```

`.hera` 落在 `DataDirectory_`（daemon 配置的存储根目录），`.insv`/`.session.json` 落在 `DownloadDir`，两者物理目录可能不同，但 basename 一致，靠文件名就能对上。

## 5. 跨机器拷贝注意事项

`insta_session.json` 里的 `hera_session_path`/`mp4_files` 记录的是 Jetson 上的绝对路径。拷贝三件套到本机分析时，`hera-storage-ingest-insta-video --session` 要能找到 `.insv`，需要先做一份路径改写过的 session json 副本，例如：

```python
import json
with open('20260711193312_fred_office.session.json') as f:
    d = json.load(f)
d['hera_session_path'] = '/local/path/20260711193312_fred_office.hera'
d['mp4_files'] = ['/local/path/20260711193312_fred_office.insv']
with open('insta_session_local.json', 'w') as f:
    json.dump(d, f, indent=2)
```

再用 `insta_session_local.json` 喂给 ingest 工具。

## 6. 时间同步与已知问题

- 各数据流的**绝对**锚点时间都是 host 端 `timestamp_host_ns`（daemon 收到数据时的本机 UTC 时间），除了 Insta360 陀螺仪 CSV 当前导出的样本级 `timestamp`（见 §2.1 的坑，只能定位到"批"级别的绝对时间）
- **已知问题（2026-07-11 已修复）**：RecordDownload 模式下，SD 卡录制在前端"采集开关"打开时（`AutoStartRecording`）立即开始，但 `.hera`（陀螺仪 + Livox）的持久化过去依赖前端另一个独立的"录制数据"开关（daemon 的全局 `setRecord`），如果操作者没有几乎同时按下两个开关，`.hera` 窗口会比 SD 卡视频晚开始——某次实测晚了 25.6 秒。已改为 `AutoStartRecording` 触发时daemon自动 `setRecord(true)`，不再依赖操作员手动对齐两个开关的时机。细节见 [troubleshooting_gyro_record_gap.md](troubleshooting_gyro_record_gap.md)。
- `multi_source_synchronizer` 要求重叠窗口内有一段明显的晃动/加速度变化模式，静止或运动特征太弱会导致互相关置信度不够（"Outbound error too low"），需要提前规划采集动作，不要只是站着不动
