///
/// @file bridge.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of Node to bridge between ROS and Hera
/// @version 0.1
/// @date 2020-02-04
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "bridge.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <turbojpeg.h>

#include "common/include/logger/logger.hpp"
#include "common/include/utils/time.hpp"
#include "device/include/include.hpp"

namespace wayz {
namespace hera {
namespace slam {


Bridge::Bridge(ros::NodeHandle& nh, ros::NodeHandle& private_nh) :
    nh_(nh),
    private_nh_(private_nh),
    rate_(SleepRate),
    ipc_queue_(ipc::IPCQueue<device::data::SensorData>::create()),
    ipc_result_(slam::Result::handler(ipc::OpenMode::Write)),
    scan_msg_inited_(false),
    last_azimuth_(-0.0f)
{
    log::debug << "Init OK" << log::endl;
    // Get paramaters
    private_nh_.param<int>("lidar_sensor_id", lidar_sensor_id_, -1);
    private_nh_.param<int>("imu_sensor_id", imu_sensor_id_, -1);

    private_nh_.param<std::string>("scan_topic", scan_topic_, "scan");
    private_nh_.param<std::string>("imu_topic", imu_topic_, "imu");
    private_nh_.param<std::string>("scan_frame", scan_frame_, "scan_link");
    private_nh_.param<std::string>("imu_frame", imu_frame_, "imu_link");
    private_nh_.param<std::string>("map_topic", map_topic_, "map");
    private_nh_.param<std::string>("trajectory_topic", trajectory_topic_, "trajectory_node_list");

    private_nh_.param<double>("min_height", min_height_, -1.0);
    private_nh_.param<double>("max_height", max_height_, +1.0);
    private_nh_.param<double>("min_pitch", min_pitch_, -1.5 / 180.0 * M_PI);
    private_nh_.param<double>("max_pitch", max_pitch_, +1.5 / 180.0 * M_PI);
    private_nh_.param<double>("min_range", min_range_, +0.4);
    private_nh_.param<double>("max_range", max_range_, +50.0);

    private_nh_.param<int>("azimuth_grids", azimuth_grids_, 1800);

    scan_msg_.ranges.assign(azimuth_grids_, 0.0f);
    scan_msg_.angle_increment = 2 * M_PI / azimuth_grids_;
    scan_msg_.angle_min = -M_PI;
    scan_msg_.angle_max = +M_PI;
    scan_msg_.header.frame_id = scan_frame_;

    // Advertise publishers
    scan_pub_ = nh_.advertise<sensor_msgs::LaserScan>(scan_topic_, 10, false);
    imu_pub_ = nh_.advertise<sensor_msgs::Imu>(imu_topic_, 10, false);

    // Register subscriber
    ipc_queue_->open(0, ipc::OpenMode::Read);
    map_sub_ = nh_.subscribe(map_topic_, 1, &Bridge::map_handler, this);
    trajectory_sub_ = nh_.subscribe(trajectory_topic_, 1, &Bridge::trajectory_handler, this);
}

Bridge::~Bridge()
{
    ipc_queue_->close();
    ipc_result_->close();
}

void Bridge::spin()
{
    while (true) {
        auto data = ipc_queue_->read();

        if (data == nullptr) {
            ros::spinOnce();
            rate_.sleep();
            continue;
        }

        switch (data->sensor_data_type) {
        case device::SensorDataType::PointsXYZI:
            if (int32_t(data->sensor_id) == lidar_sensor_id_ || lidar_sensor_id_ < 0) {
                auto data_impl = reinterpret_cast<device::data::PointsXYZI*>(data.get());
                lidar_handler(data_impl);
            }
            break;
        case device::SensorDataType::ImuMagneticField:
            if (int32_t(data->sensor_id) == imu_sensor_id_ || imu_sensor_id_ < 0) {
                auto data_impl = reinterpret_cast<device::data::ImuMagneticField*>(data.get());
                imu_handler(data_impl);
            }
            break;
        default:
            break;
        }
    }
}

void Bridge::lidar_handler(const device::data::PointsXYZI* const data)
{
    const bool ccw = (data->meta.rotation_direction > 0);
    for (size_t i = 0; i < data->point_number; ++i) {
        const auto& pt = data->points[i];

        if (pt.pitch > max_pitch_)
            continue;
        if (pt.pitch < min_pitch_)
            continue;

        if ((ccw && last_azimuth_ > 0 && pt.azimuth < 0) || (!ccw && last_azimuth_ < 0 && pt.azimuth > 0)) {
            // Full circle
            if (scan_msg_inited_) {
                scan_msg_.scan_time = (data->timestamp_intrinsic_ns - last_circle_time_) / time::OneSecond;
                if (ccw) {
                    scan_msg_.header.stamp = to_ros_time(last_circle_time_);
                    scan_msg_.time_increment = +scan_msg_.scan_time / azimuth_grids_;
                } else {
                    scan_msg_.header.stamp = to_ros_time(data->timestamp_intrinsic_ns);
                    scan_msg_.time_increment = -scan_msg_.scan_time / azimuth_grids_;
                }
                sensor_msgs::LaserScan pub_msg = scan_msg_;
                scan_pub_.publish(pub_msg);
                scan_msg_.ranges.assign(azimuth_grids_, 0.0f);
                last_circle_time_ = data->timestamp_intrinsic_ns;
            }

            last_circle_time_ = data->timestamp_intrinsic_ns;
            scan_msg_inited_ = true;

            max_pitch_ = std::min(max_pitch_, 0.25f * data->meta.nominal_pitch_increment);
            min_pitch_ = std::max(min_pitch_, -0.75f * data->meta.nominal_pitch_increment);
            max_range_ = std::min(max_range_, data->meta.nominal_max_range);
            min_range_ = std::max(min_range_, data->meta.nominal_min_range);

            scan_msg_.range_min = min_range_;
            scan_msg_.range_max = max_range_;
        }

        last_azimuth_ = pt.azimuth;

        if (pt.z > max_height_)
            continue;
        if (pt.z < min_height_)
            continue;
        if (pt.horizontal_distance > max_range_)
            continue;
        if (pt.horizontal_distance < min_range_)
            continue;

        float index_f = (pt.azimuth + M_PI) / (2 * M_PI) * azimuth_grids_;
        int32_t index = std::round(index_f);

        if (index >= 0 && index < azimuth_grids_) {
            scan_msg_.ranges[index] = pt.horizontal_distance;
        }
    }
}

void Bridge::imu_handler(const device::data::ImuMagneticField* const data)
{
    sensor_msgs::Imu ros_msg;
    ros_msg.header.frame_id = imu_topic_;
    ros_msg.header.stamp = to_ros_time(data->timestamp_intrinsic_ns);
    ros_msg.orientation_covariance[0] = -1;
    ros_msg.linear_acceleration.x = data->linear_acceleration[0];
    ros_msg.linear_acceleration.y = data->linear_acceleration[1];
    ros_msg.linear_acceleration.z = data->linear_acceleration[2];
    ros_msg.angular_velocity.x = data->angular_velocity[0];
    ros_msg.angular_velocity.y = data->angular_velocity[1];
    ros_msg.angular_velocity.z = data->angular_velocity[2];
    imu_pub_.publish(ros_msg);
}

void Bridge::map_handler(const nav_msgs::OccupancyGrid::ConstPtr& msg)
{
    if (!ipc_result_->writable()) {
        return;
    }

    const size_t canvas_size = msg->data.size() * 3;
    auto canvas = std::unique_ptr<uint8_t[]>(new uint8_t[canvas_size]);

    // Render Base
    uint8_t* canvas_ptr = canvas.get();
    for (auto&& pt : msg->data) {
        if (pt < 0) {
            *canvas_ptr++ = 0;
            *canvas_ptr++ = 0;
            *canvas_ptr++ = 0;
        } else {
            *canvas_ptr++ = 255 - pt * 2;
            *canvas_ptr++ = 255 - pt * 2;
            *canvas_ptr++ = 255 - pt * 2;
        }
    }

    // Get Pose -> Cell map
    canvas_ptr = canvas.get();
    const int32_t ceil_org_map_x = msg->info.origin.position.x;
    const int32_t ceil_org_map_y = msg->info.origin.position.y;
    const int32_t ceil_width = msg->info.width;
    const int32_t ceil_height = msg->info.height;
    const auto render = [=](const Vector2d& pose, int32_t radius, uint8_t b, uint8_t g, uint8_t r) {
        float y_shift = pose.y - ceil_org_map_y;
        float x_shift = pose.x - ceil_org_map_x;
        int32_t ceil_y = y_shift / msg->info.resolution;
        int32_t ceil_x = x_shift / msg->info.resolution;
        int32_t y_min = std::max(0, ceil_y - radius + 1);
        int32_t y_max = std::min(ceil_height, ceil_y + radius);
        int32_t x_min = std::max(0, ceil_x - radius + 1);
        int32_t x_max = std::min(ceil_width, ceil_x + radius);

        for (int32_t y = y_min; y < y_max; ++y) {
            for (int32_t x = x_min; x < x_max; ++x) {
                uint8_t* ptr = canvas_ptr + 3 * (y * ceil_width + x);
                *ptr++ = b;
                *ptr++ = g;
                *ptr = r;
            }
        }
    };

    static constexpr auto StartPointSize = 14;
    static constexpr auto EndPointSize = 14;
    static constexpr auto LineSize = 2;
    static constexpr auto Grid10Size = 4;
    static constexpr auto Grid50Size = 5;
    static constexpr auto Grid100Size = 8;

    render({0, 0}, StartPointSize, 255, 0, 0);

    for (size_t i = 0; i < trajectory_.size(); ++i) {
        int32_t ratio = 2 * 255 * ((float)(i) / trajectory_.size());
        uint8_t r = ratio > 255 ? ratio - 255 : 0;
        uint8_t b = ratio < 255 ? 255 - ratio : 0;
        uint8_t g = ratio > 255 ? 2 * 255 - ratio : ratio;
        int32_t pt_size =
                (i % 10 == 0) ? (i % 50 == 0) ? (i % 100 == 0) ? Grid100Size : Grid50Size : Grid10Size : LineSize;
        render(trajectory_[i], pt_size, b, g, r);
    }

    if (!trajectory_.empty()) {
        render(trajectory_.back(), EndPointSize, 0, 0, 255);
    }

    auto tj_instance = tjInitCompress();
    if (!tj_instance) {
        return;
    }
    uint8_t* dst_image = nullptr;
    size_t dst_size = 0;

    if (tjCompress2(tj_instance,
                    canvas.get(),
                    ceil_width,
                    0,
                    ceil_height,
                    TJPF_BGR,    // Source Image Format
                    &dst_image,  // Output Image Pointer
                    &dst_size,   // Output Image Size
                    TJSAMP_411,  // YUV Binning
                    30,          // Quality
                    TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE | TJXOP_VFLIP) != 0) {
        log::warn << "Display: Can not compress jpeg" << log::endl;
        tjDestroy(tj_instance);
        if (dst_image) {
            tjFree(dst_image);
        }
        return;
    }

    Result result;
    result.height = ceil_height;
    result.width = ceil_width;
    result.data.resize(dst_size);
    memcpy(result.data.data(), dst_image, dst_size);
    ipc_result_->write(result);

    tjDestroy(tj_instance);
    tjFree(dst_image);
}

void Bridge::trajectory_handler(const visualization_msgs::MarkerArray::ConstPtr& msg)
{
    size_t num_points = 0;
    for (const auto& marker : msg->markers) {
        num_points += marker.points.size();
    }
    trajectory_.reserve(num_points / TrajectorySizeStep + TrajectorySizeStep);
    trajectory_.resize(0);
    for (const auto& marker : msg->markers) {
        for (const auto& point : marker.points) {
            Vector2d pt = {float(point.x), float(point.y)};
            trajectory_.emplace_back(pt);
        }
    }
}

}  // namespace slam
}  // namespace hera
}  // namespace wayz