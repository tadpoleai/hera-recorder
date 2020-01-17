# 2D实时建图可视化模块

## 功能

    1. 将水平32线激光数据包转换为2D水平扫描轮廓线的ROS topic,发布IMU topic;
    2. 启动cartographer 2d建图功能,发布轨迹/约束/map等topic,以此作为可视化界面数据源;

## 输入数据

    1. 水平32线激光数据包,151 packages / frame;
    2. IMU数据包,频率≥100Hz;

## 主要输出数据(ROS Topic)

### frame_id: map

    1. 2D轮廓图: /map
    2. 轨迹: /trajectory_node_list

### frame_id: velodyne

    1. 32线原始数据: /velodyne_points
    2. 激光2D轮廓线: /laserscan

### frame_id: imu_link

    Imu数据: /imu/raw_data

## 使用

### 启动实时建图

    ./launch_realtime_slam_2d.sh

### 启动rviz可视化界面

    rviz配置参数文件: carto/share/configuration_files/demo_2d.rviz
