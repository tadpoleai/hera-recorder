# 代码说明

## 术语和类型定义

头文件 [types.hpp](../device/include/types.hpp) 以及实现文件 [device_types_impl.hpp](../device/base/device_types_impl.hpp)

### 设备类别(Device Category)

传感器设备的类别，目前包括惯性测量单元(IMU)，惯性导航系统(INS), 激光雷达(Lidar)，相机(Camera), GNSS(GNSS), 测距计(Odometry)

同一个类别下的不同传感器，使用相同或者相似的传感器数据(Sensor Data)类型, 即使他们使用不同的厂商驱动(Device Vendor)和不同的原始储存数据(Device Data)

### 设备厂商和型号(Device Vendor Type)

在每个传感器设备种类下，根据设备厂商和具体型号细分的传感器设备型号

对于每一个型号, 在 [plugin](../device/plugin/) 具有一个独立的从基类 `device::Device` 继承来的子类的实现(具体实现内容在后续描述)  
 每一个型号具有独享的原始储存数据(Storage Data), 并且在 `device_types_impl.hpp` 中 `DeviceVendorType` 中有一个**唯一且不可修改**的枚举值

### 原始储存数据(Device Data)

从传感器设备获取的,不经转换加工或者仅经过少量必要的转换(通常为加时间戳或者压缩数据)的原始数据，该数据将被直接写入到记录文件 `.hera` 中

原始储存数据包含足够的信息,使得其能够被单独地转换为, 不依赖具体设备型号的传感器数据(Sensor Data)

原始储存数据的基类定义在 [device_data.hpp](../device/include/device_data.hpp) 中. 如下

> |               类型 | 字段名                 |                                       含义 |
> | -----------------: | :--------------------- | -----------------------------------------: |
> |         `uint32_t` | `length`               |                       数据的总长度，字节数 |
> | `DeviceVendorType` | `device_vendor_type`   |                    枚举类型,设备厂商和型号 |
> |         `uint32_t` | `device_id`            |    传感器 ID, 由采集程序赋值,从 0 开始递增 |
> |   `DeviceDataType` | `message_type`         |                枚举类型,原始储存数据的类型 |
> |         `uint32_t` | `sequence`             | 数据序号, 从 0 开始递增,每个传感器独立计数 |
> |         `uint64_t` | `timestamp_receive_ns` |          接收到该数据时的时间戳，UTC，纳秒 |

其中, 原始储存数据类型(DeviceDataType)指示该原始数据的具体子类类型,  
 参考`device_types_impl.hpp` 中的 `DeviceDataType`
每一个 DeviceDataType 都是**唯一且不可修改**的

特别地, 记录文件(`.hera`)的格式请参考 [Hera 二进制数据记录文件](recordfile.md)

### 传感器数据(Sensor Data)

原始储存数据(Device Data)经过转换得到的传感器数据，数据内容应该设备厂商和型号(Device Vendor Type)无关，而只与该设备种类(Device Category)有关

此外，数据内容应当尽量与 ROS 的 `sensor_msgs` 现有规定的格式相同(如 `CompressedImage`).  
当 ROS 中缺少相应定义时, 可使用自定义格式, 但需易于转换(如 `PointsXYZI`)为 ROS 格式

在 [sensor_data_types.hpp](../device/include/sensor_data_types.hpp) 中包含了传感器数据的类型定义
在 [sensor_data.hpp](../device/include/sensor_data.hpp) 为该类基类的具体实现. 如下

> |             类型 | 字段名                   |                                       含义 |
> | ---------------: | :----------------------- | -----------------------------------------: |
> |       `uint32_t` | `length`                 |                       数据的总长度，字节数 |
> | `SensorDataType` | `sensor_data_type`       |                    枚举类型,传感器数据类型 |
> |       `uint32_t` | `device_id`              |    传感器 ID, 由采集程序赋值,从 0 开始递增 |
> |       `uint32_t` | `sequence`               | 数据序号, 从 0 开始递增,每个传感器独立计数 |
> |       `uint64_t` | `timestamp_intrinsic_ns` |                数据的真实时间戳，UTC，纳秒 |

子类的传感器数据定义在对应的设备类别下, [data](../device/include/data/) 文件夹.  
每个设备类别拥有一个头文件,其中定义了该类别下所使用的传感器数据的具体字段

### 显示数据(Display Data)

在需要将数据显示在屏幕上时, 需要将数据转换为显示数据.  
定义和实现分别在[display_data.hpp](../device/include/display_data.hpp) 以及文件夹 [display](../device/display/)

考虑到单个传感器可能输出包含多个类型的的数据,  
`DisplayData` 中包含了一个从 `SensorDataType` 到 `SingleDisplayData` 的 `map`, 以便客户端能同时显示多种类型的数据.

`SingleDisplayData` 是单个 `SensorDataType` 转换而来的数据, 可包括图片数据和文字数据.  
字段中 `std::string` 格式的 `text_data` 和 `jpeg_data`, 分别是文字数据和图片数据

## 传感器采集实现

本段描述传感器设备的实现

### 共同定义

#### HeraErrno

Hera 的错误码, 实现于 [hera_errno.hpp](../common/include/hera_errno.h)

### 传感器设备基类 `class device::Device` 的实现

参考头文件 [device.hpp](../device/include/device.hpp) 与 实现 [device.cpp](../device/base/device.cpp)

基类封装了打开与关闭设备，接收录制与暂停，配置参数, 储存到文件等方法

基类构造函数被 `device::Factory` 调用, 详见后续描述

基类对外抽象接口有以下:

- `HeraErrno start()`, 连接传感器并开始获取数据
- `void stop()`, 断开传感器并停止获取数据
- `HeraErrno record(bool value)`, 切换传感器获取到的数据是否写入储存
- `HeraErrno parameter(const std::string& type, const std::string& value)`, 设置参数
- 各类私有成员的 getter

需要子类去实现的抽象方法有如下:

- `virtual HeraErrno connect()`，子类传感器设备建立连接
- `virtual void disconnect()`，子类传感器设备断开链接
- `virtual data::DeviceDataPtr fetch()`，子类传感器数据的获取函数(需实现为带超时的阻塞函数)
- `virtual HeraErrno adjust_parameter(const std::string& type, const std::string& value)`, 实时调整子类传感器设备参数的函数
- `virtual data::SensorDataPtr convert(data::DeviceDataPtr& storage_data)`, 将子类传感器设备的原始数据转换为传感器数据

子类可使用的保护对象有如下:

> |                                 类型 | 字段名         |                                                 含义 |
> | -----------------------------------: | :------------- | ---------------------------------------------------: |
> |                     `const uint32_t` | `id_`          |                      传感器设备 ID 号, 从 0 开始递增 |
> |                  `const std::string` | `vendor_type_` |         字符串形式的设备厂商和型号, 如 `camera/flir` |
> |                  `const std::string` | `name_`        | 传感器设备名称, 通常为描述安装位置的词语, 如 `front` |
> | `std::map<std::string, std::string>` | `parameters_`  |                                           传感器参数 |
> |                           `uint32_t` | `sequence_`    |                数据的序号, 需由子类 `fetch` 函数管理 |

### 传感器插件的实现

#### 编译方式

在文件夹 [plugin](../device/plugin/)下根据设备类别(Device Category)分文件夹, 在每个类别下, 每个设备厂商和型号单独拥有一个文件夹

在该文件夹内, 有且仅有一个 cpp 文件, 该文件将被 CMake 规则编译成一个插件, 插件分为两个全功能的版本和基础版,两个版本由宏 `WITH_DRIVER` 控制

若插件需要单独的 CMake 文件, 可在文件夹下使用和 C++文件夹同名的 `.cmake` 文件, 并在里面通过设置 `PLUGIN_DRIVER_FOUND` 来通知顶层 CMake 该插件的驱动是否可用

具体可参考 [foobar.hpp](../device/plugin/dummy/foobar.hpp) 及 [foobar.cpp](../device/plugin/dummy/foobar.cpp)

#### 源码规则

子类传感器需要在对应的 namespace 中先定义子类传感器使用的原始储存数据, 然后定义一个子类继承基类 `device::Device`, 并实现上一节提到的抽象方法

在 cpp 文件中, 需要使用宏 `HERA_PLUGIN_EXPORT(type_enum_param, type_name_param)`来导出符号  
`type_enum_param` 为 `DeviceVendorType` 中枚举的名称, `type_name_param` 为字符串形式的名称(如`camera/flir`)
如 `HERA_PLUGIN_EXPORT(DummyFoobar, "dummy/foobar");`

#### 工厂函数

详见源码 [factory.cpp](../device/base/factory.cpp)

## 添加新的传感器

若需要添加新的传感器, 参考以下步骤

1. 注册传感器设备类型 ID

   - 查阅 [device_types_impl.hpp](../device/base/device_types_impl.hpp)
   - 往 `enum class DeviceVendorType` 中新增一个传感器名
     取名为`<Category><Vendor>`, 并显式规定一个 `uint16_t` 的数字(如 `CameraFlir = 0x0401` )
   - 根据需要, 往 `enum class DeviceDataType` 中增加相应的原始储存数据类型
     取名为`<Category><Vendor><DataType>`, 并为每个数据类型显式规定一个 `uint16_t` 位的数字(如 `CameraFlirCompressedImage = 0x0401` )

1. 注册传感器数据 ID (可选)
   若该传感器使用了一种全新的传感器数据类型, 则需要新建传感器数据类型

   - 编辑 [sensor_data_types.hpp](../device/include/sensor_data_types.hpp), 增加一种新的传感器数据类型
   - 在 [display](../device/display/) 适当的位置, 编写将该传感器数据转换为显示数据的函数

   ```cpp
      template<> SingleDisplayData SingleDisplayData::parse<SensorDataType::NEW_TYPE>(std::vector<SensorDataPtr>&& sensor_datas, const bool is_detail)
   ```

   - 在 [ros_message_impl](../convert/src/ros_message_impl) 文件夹中适当的位置, 编写将该传感器数据转换为 ROS 数据的函数

   ```cpp
   template<>
   std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::NEW_TYPE>(
           device::data::SensorDataPtr& sensor_data,
           const std::string& topic_prefix,
           const std::string& frame_id,
           const common::Remapper* remapper)
   ```

1. 编写源码

   - 在文件夹 [plugin](../device/plugin/) 对应的 Category 的文件夹下新建一个文件夹, 取名为`<vendor>`(如`device/plugin/camera/flir`)
   - 将源代码(`<vendor>.cpp` 以及 `<vendor.hpp>`) 放入上述文件夹
   - 源代码的编写方式可参考 [foobar.hpp](../device/plugin/dummy/foobar.hpp) 及 [foobar.cpp](../device/plugin/dummy/foobar.cpp)
