# 2D实时建图可视化模块

## 架构

- bridge
    - 将Hera实时数据转化为ROS Topic并发布
        - 输入数据
            - ImuMagneticField
            - PointsXYZI
        - 发布数据
            - topic: imu, frame_id: imu_link, type: sensor_msgs::IMU
            - topic: scan, frame_id: scan_link, type: sensor_msgs::LaserScan
    - 自ROS订阅建图结果，并转换为可视化图像
        - 订阅数据
            - topic: map 占用栅格地图
            - topic: trajectory_node_list 轨迹标记点
        - 生成数据
            - Jpeg 地图图像文件
- carto
    - bin 编译好的carto可执行文件
    - share carto的配置文件
        - config carto的节点配置文件
        - script 启动/停止建图模块的脚本文件
        - urdf Tron等设备的描述文件
- caller
  - 包含调用建图模块的C++库文件

## 使用说明

### 构建和安装

本地构建和安装，依赖hera/recorder顶层，请参照顶层的构建说明，并在顶层的cmake参数中，开启`-Dwith-slam=1`

从CI自动编译结果安装，请执行顶层目录下的`jenkins_deploy.sh`

### 调用建图

本模块暂时只支持Ubuntu16和ROS Kinetic，并仅支持在**安装后**调用

#### 开启建图

执行`hera-slam-caller-start`，将会启动roscore、carto以及hera-slam-bridge

或者，可以链接库`libhera-slam-caller`，并调用slam::Caller::start()来在程序中执行该脚本

通过hera-replay播放数据或hera-daemon开始采集数据即可向hera-slam-bridge输入数据

#### 结束建图

执行`hera-slam-caller-stop`，将会调用kill杀死ros、carto以及slam进程

或者，可以链接库`libhera-slam-caller`，并调用slam::Caller::stop()来在程序中执行该脚本

**警告，该命令会杀死所有ROS, SLAM, CARTO相关进程！不要再其他任务执行的时候调用**

#### 调试建图

若需修改编译调试hera-slam-bridge，可执行`carto/share/script/start_ros.sh`，将会启动roscore、carto，但不启动hera-slam-bridge。  
此后，可手动启动hera-slam-bridge进行调试

### 获取结果

hera-slam-bridge的Bridge::map_handler()会生成jpeg格式的二进制流，并将其通过文件保存到当前目录，文件名为map.jpg

在调试时，也使用rviz查看，可加入topic: map以及trajectory_node_list来查看

TODO：hera-slam-bridge会将jpeg数据通过ipcBuffer发布，可使用程序读取内容并通过可视化查看
