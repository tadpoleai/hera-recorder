# HERA 采集平台

## 简述

本软件为采集，保存和转换硬件设备数据的平台。

### 适用平台

适用于运行Linux，带有网络接口的一系列硬件采集设备，目前公司内安装了该软件的硬件平台有以下：

- Tron

### 安装需求

- 工控机
  - 性能需满足相应硬件传感器数量的需求
  
  - 运行Ubuntu16.04 LTS(其他版本的Ubuntu需要对转换代码的ROS作相应修改)
  
  - 足够的储存空间和储存写入速度，以便采集软件写入文件

## 安装指南

详见[安装指南](INSTALLATION.md)

## 使用说明

### 源代码

查看源代码文件结构和注释，可安装doxygen并运行以下命令

```shell
doxygen doxygen.config
```

然后在网页浏览器中打开doxygen生成的文件[doxygen/html/index.html](doxygen/html/index.html)

需要加入新的**数据类型**，**设备驱动**支持时，请参看[新建传感器指南](devices/NEWDEVICE.md)

### 采集使用

详见[采集使用](client/USAGE.md)

### 转换格式

详见[转换格式](convert/USAGE.md)

## TODO List

可查看doxygen内的TODO List
