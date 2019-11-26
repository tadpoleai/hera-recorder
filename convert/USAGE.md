# HERA 数据转换工具

## 简述

本包主要功能为将HERA采集的原始数据转换为OS的Bag格式

## 安装需求

- 运行Ubuntu16.04 LTS(其他版本的Ubuntu需要对ROS版本作相应修改)

## 使用说明

命令行输入

```shell
hera-convert -i <input> [-o <output> -r <remap> -vl]
```

其中

- `input` 为待转换的原始数据(文件夹)
- `output` 为转换后的bag文件名(可选)，默认为`<input>.bag`
- `remap` 为FrameId和TopicName使用的重映射文件，默认为空(不进行重映射)，关于重映射详见下一小节
- `-v` 开启debug模式，输出更多信息
- `-l` 开启log，将控制台输出写入log文件

转换过程将会提示进度和预计剩余时间

转换完成后可能需要等待一段时间，使Bag的写入缓存同步到文件系统(特别，当输出路径为网络驱动器时)

## 重映射

### 默认的FrameId和TopicName约定

数据采集时每个Device对应一个固定的FrameId，而ROSMessage的TopicName随ROSMessage的类型改变

- FrameId名为`<deviceType>_<deviceName>_link`

  如`lidar/velodyne/horizontal`的FrameId为`lidar_horizontal_link`

- TopicName名为`<deviceType>/<deviceName>/${snack_case(<ROSMessageType>)`

  如传感器`imu/embedded`会输出`sensor_msgs/Imu`和`sensor_msgs/MagneticField`两种Message

  其TopicName分别为`/imu/embedded/imu`和`/imu/embedded/magnetic_field`

### 重映射文件格式

由于有转换为原有ROSBag格式的需求，本工具接受可选参数`-r remap`

该`remap`是一个json文件，将TopicName和FrameID重映射为指定字段

其记录格式如下：`"<NameFrom>": "<NameTo>"`，如

```json
{
    "/imu/embedded/imu": "/imu/raw_data",
    "/imu/embedded/magnetic_field": "/imu/mag",
    "imu_embedded_link": "imu_link",
    "/lidar/horizontal/point_cloud2": "/velodyne_points",
    "lidar_horizontal_link": "frame_velodyne_points",
    "/lidar/inclined/point_cloud2": "/l2",
    "lidar_inclined_link": "frame_l2",
    "/camera/rear/compressed_image": "/camera/left/image_raw",
    "camera_rear_link": "camera_left"
}
```
