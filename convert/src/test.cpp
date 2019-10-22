#include <devices/src/lidar/lidar.hpp>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl_ros/point_cloud.h>
#include <sensor_msgs/PointCloud2.h>

#include "all_converters.hpp"
#include "iostream"

using namespace wayz::tron;

int main(int args, char** argv)
{
    Converter::open_bag("test2.bag");
    Converter* imu_converter =
            new ImuConverter("imu", "internal", "../20191018103800_record_test_1min/Imu/internal/");
    Converter* lidar_converter_top =
            new LidarConverter("lidar", "top", "../20191018103800_record_test_1min/Lidar/top/");
    Converter* lidar_converter_back =
            new LidarConverter("lidar", "back", "../20191018103800_record_test_1min/Lidar/back/");
    delete lidar_converter_top;
    delete lidar_converter_back;
    delete imu_converter;
    Converter::close_bag();
}
