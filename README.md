# HERA 采集平台

## 简述

本软件为采集，保存和转换硬件设备数据的平台。

## 代码说明

在本仓库中，有以下子文件夹包含源代码
- **common**  
  公用依赖代码库   
  必须，构建输出为静态库文件libhera-common  

- **devices**  
  设备驱动及数据储存和转换  
  必须，构建输出为静态库文件libhera-devices  
  其中，某些传感器的实际物理连接需要第三方依赖库，该部分为可选编译。若不编译，则该传感器仅支持离线数据转换。

- **storage**  
  用于储存，重播传感器数据的代码库  
  必须，构建输出为静态库文件libhera-storage  

- daemon  
  采集服务端，在线采集用守护进程  
  可选，构建输出为可执行文件hera-daemon  

- convert  
  将储存数据转换为ROSBag的工具  
  可选，构建输出为可执行文件hera-convert  

- replay  
  将储存数据重新在线播放(通过IPC)的工具，可供slam调试  
  可选，构建输出为可执行文件hera-replay  

- slam  
  在线建图，接收IPC传输的传感器数据来生成地图  
  可选，构建输出为可执行文件hera-slam-caller，hera-slam-brige以及hera-slam-result，具体请参考slam文件夹下说明  

- client  
  采集客户端，由Vue + ts编写  
  可选，构建输出为网页目录client/dist

查看源代码文件结构和注释，可安装doxygen并运行以下命令

```shell
doxygen doxygen.config
```

然后在网页浏览器中打开doxygen生成的文件[doxygen/html/index.html](doxygen/html/index.html)

需要加入新的**数据类型**，**设备驱动**支持时，请参看[新建传感器指南](devices/NEWDEVICE.md)

## 采集适用平台

适用于运行Linux，带有网络接口的一系列硬件采集设备，目前公司内安装了该软件的硬件平台有以下：

- Tron

### 安装需求

- 工控机
  - 性能需满足相应硬件传感器数量的需求
  
  - 运行Ubuntu16.04 LTS(其他版本的Ubuntu需要对转换代码的ROS作相应修改)
  
  - 足够的储存空间和储存写入速度，以便采集软件写入文件

## 构建和部署指南

详见[安装指南](INSTALLATION.md)

## 使用说明

### 采集使用

详见[采集使用](client/USAGE.md)

### 转换格式

详见[转换格式](convert/USAGE.md)

## TODO List

可查看doxygen内的TODO List
