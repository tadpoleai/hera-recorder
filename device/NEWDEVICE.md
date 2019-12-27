# 新建传感器指南

## 代码说明

### 枚举定义

本段内容与[devices/src/device_types.hpp](src/device_types.hpp)对应

- 设备种类(Device Category)

  传感器设备的种类，目前包括惯性测量单元(IMU)，激光雷达(Lidar)，相机(Camera), GNSS(GNSS)

  同一个种类下的不同传感器，即使有不同的驱动(Vendor Driver)和原始储存数据(Storage Data)，但是它们具有相似或者相同的传感器数据(Sensor Data)类型

- 设备制造商(Device Vendor)

  在相应的传感器设备种类下，再分的多个不同的厂商制造的传感器

  它们具有独享的原始储存数据(Storage Data)，独享的驱动(Vendor Driver)代码

- 原始储存数据(Storage Data)

  传感器设备输出的不经转换加工或者经过少量必要的转换(通常为加时间戳或者压缩数据)的数据，直接储存到储存设备里

  原始储存数据的头部将包含设备的类型其他必要信息，以便数据能够转换为传感器数据(Sensor Data)

- 传感器数据(Sensor Data)

  原始储存数据(Storage Data)经过转换得到的传感器数据，数据内容应该设备制造商(Device Vendor)无关，而只与该设备种类(Device Category)有关

  此外，数据内容应当与ROS现有规定的格式相同(如CompressedImage)，或者易于转换(如PointsXYZI)为ROS格式

- 传感器设备参数(Device Parameter)

  连接，采集和配置传感器设备中所需要的参数类型在这里定义

  考虑到不同种类的传感器有可能会共享同一种参数类型(如IpAddress)，这里的参数类型不分种类，单层平铺定义

### 数据实现

本段内容与[devices/src/device_data.hpp](src/device_data.hpp)对应

- 原始储存数据(Storage Data)

  包含以下字段
  
  - `uint32_t length` 数据的总长度，字节数
  - `DeviceVendorType device_vendor_type` 传感器设备的种类和制造商类型
  - `uint32_t device_id` 传感器的ID
  - `DeviceDataType message_type` 储存数据的类型
  - `uint32_t sequence` 数据的序号
  - `uint64_t timestamp_receive_ns` 接收到该数据的时间戳，UTC，纳秒

  该类型是所有储存数据的基类，基类实现了共享指针构造函数create()

- 传感器数据(Sensor Data)

  包含以下字段
  
  - `uint32_t length` 数据的总长度，字节数
  - `SensorDataType sensor_data_type` 传感器数据的类型
  - `uint32_t device_id` 传感器的ID
  - `uint32_t sequence` 数据的序号
  - `uint64_t timestamp_intrinsic_ns` 数据的真实时间戳，UTC，纳秒

  该类型是所有传感器数据的基类

### 传感器驱动

本段内容与[devices/src/device.hpp](src/device.hpp)对应

Device基类封装了接收打开与关闭设备调用，接收录制与暂停调用，储存到文件等方法，具体实现需参考源码和doxygen文档

其提供给子类的抽象接口有以下

- 成员`parameters_`，参数对，供连接和配置子类时使用
- 抽象方法`connect()`，实现连接和打开设备
- 抽象方法`disconnect`，实现关闭设备
- 抽象方法`fetch()`，实现从设备读取单个原始储存数据
- 抽象方法`convert()`，实现从原始储存数据转换到传感器数据
- 抽象方法`adjust_parameter()`，实现在传感器在线调整参数

## 在已有的种类中(Device Category)增加单个设备(Vendor)的方法

1. 在[devices/src/device_types.hpp](src/device_types.hpp)的`DeviceVendorType`中增加新的类型

1. 在`DeviceDataType`中增加对应的该设备对应的储存类型

1. 在对应的种类文件夹下`devices/src/<category>/`下，建立新的文件夹(如`devices/src/lidar/velodyne`)，然后参照现有的驱动，实现新的驱动，以及新的储存类型

1. 在设备的工厂函数实现(位于[devices/src/device_factory.hpp](src/device_factory.hpp)中)的`create()`和`check_type()`添加新的类型

1. 在客户端函数的定义文件([client/src/core/deviceDefine.ts](../client/src/core/deviceDefine.ts))中
   1. 在`deviceTypes`中添加新的类型
   1. 在`parameterTypes`中添加新的记录(该设备所需的参数类型)
   1. 如有增加参数类型，则在`parameterRules`内实现它的检查规则

1. 如有必要，在`SensorDataType`及`DeviceParameterType`中增加新的类型，并在相应的Category文件夹下实现(如`lidar_data.hpp`)

1. 如有新的`SensorDataType`，需要写对应的`ROSMessageType`，以在`Convert`里面实现对应的`Processer`函数，具体参考`convert/src/ros_message.hpp`和`convert/src/processer_impls`

## 增加新的传感器种类的方法

除了以上步骤，还需要建立`devices/src/<category>/`并定义该种类对应的SensorData的头文件`<category>_data.hpp`