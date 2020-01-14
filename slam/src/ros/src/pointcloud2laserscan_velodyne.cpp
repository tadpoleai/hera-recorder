#include "pointcloud2laserscan_velodyne.hpp"
#include <sensor_msgs/LaserScan.h>
#include <tf2_sensor_msgs/tf2_sensor_msgs.h>

Pointcloud2LaserScanVelodyne::Pointcloud2LaserScanVelodyne(ros::NodeHandle& node, ros::NodeHandle& privateNode)
{
    nh_ = node;
    private_nh_ = privateNode;

    //parameter 05:target_frame
    private_nh_.param<std::string>("target_frame", target_frame_, "velodyne");

    //parameter 06:min_height
    private_nh_.param<double>("min_height", min_height_, -1.0);

    //parameter 07:max_height
    private_nh_.param<double>("max_height", max_height_, 1.0);

    //parameter 08:angle_min
    private_nh_.param<double>("horizontal_angle_min", horizontal_angle_min_, -M_PI);

    //parameter 09:angle_max
    private_nh_.param<double>("horizontal_angle_max", horizontal_angle_max_, M_PI);

    //parameter 10:angle_increment
    private_nh_.param<double>("horizontal_angle_increment", horizontal_angle_increment_, 0.2 / 360.0);

    //parameter 11:vertical_angle_min
    private_nh_.param<double>("vertical_angle_min", vertical_angle_min_, -0.1 / 57.3);

    //parameter 12:vertical_angle_max
    private_nh_.param<double>("vertical_angle_max", vertical_angle_max_, 0.1 / 57.3);

    //parameter 13:range_min_
    private_nh_.param<double>("range_min", range_min_, 0.5);

    //parameter 14:range_max_
    private_nh_.param<double>("range_max", range_max_, 100.0);

    //parameter 15:output_topic
    private_nh_.param<std::string>("output_topic", laserscan_output_topic_, "laserscan");
    laserscan_pub_ =
            nh_.advertise<sensor_msgs::LaserScan>(laserscan_output_topic_, 10, false);
}

Pointcloud2LaserScanVelodyne::~Pointcloud2LaserScanVelodyne(){}

void Pointcloud2LaserScanVelodyne::pointcloud_callback(const sensor_msgs::PointCloud2ConstPtr &cloud_msg)
{
    //build laserscan output_laserscan
    sensor_msgs::LaserScan output_laserscan;
    output_laserscan.header = cloud_msg->header;
    if (!target_frame_.empty())
    {
      output_laserscan.header.frame_id = target_frame_;
    }

    output_laserscan.angle_min = horizontal_angle_min_;
    output_laserscan.angle_max = horizontal_angle_max_;
    output_laserscan.angle_increment = horizontal_angle_increment_;
    output_laserscan.time_increment = 0.0;
    // output_laserscan.scan_time = scan_time_;
    output_laserscan.range_min = range_min_;
    output_laserscan.range_max = range_max_;

    //determine amount of rays to create
    uint32_t ranges_size = std::ceil((output_laserscan.angle_max - output_laserscan.angle_min) / output_laserscan.angle_increment);

    output_laserscan.ranges.assign(ranges_size, 0.f);

    sensor_msgs::PointCloud2ConstPtr transformed_cloud;

    // Transform cloud if necessary
    if (!(output_laserscan.header.frame_id == cloud_msg->header.frame_id))
    {
      try
      {
          sensor_msgs::PointCloud2Ptr cloud;
          // constexpr double tolerance = 0.01;
          cloud.reset(new sensor_msgs::PointCloud2);

          //TODO: receive tf transform if needed; such as laser correct to truely horizontal
          // tf2_->transform(*cloud_msg, *cloud, target_frame_, ros::Duration(tolerance));
          transformed_cloud = cloud;
      }
      catch (tf2::TransformException ex)
      {
          ROS_ERROR_STREAM("Transform failure: " << ex.what());
          return;
      }
    }
    else
    {
        transformed_cloud = cloud_msg;
    }

    // Iterate through pointcloud
    for (sensor_msgs::PointCloud2ConstIterator<float>
              iter_x(*transformed_cloud, "x"), iter_y(*transformed_cloud, "y"), iter_z(*transformed_cloud, "z");
              iter_x != iter_x.end();
              ++iter_x, ++iter_y, ++iter_z)
    {

      if (std::isnan(*iter_x) || std::isnan(*iter_y) || std::isnan(*iter_z))
      {
          ROS_DEBUG("rejected for nan in point(%f, %f, %f)\n", *iter_x, *iter_y, *iter_z);
          continue;
      }

      if (*iter_z > max_height_ || *iter_z < min_height_)
      {
          ROS_DEBUG("rejected for height %f not in range (%f, %f)\n", *iter_z, min_height_, max_height_);
          continue;
      }

      double range = hypot(*iter_x, *iter_y);
      if (range < range_min_)
      {
          ROS_DEBUG("rejected for range %f below minimum value %f. Point: (%f, %f, %f)", range, range_min_, *iter_x, *iter_y,
                  *iter_z);
          continue;
      }

      //vertical angle
      double v_angle = atan2(*iter_z, range);
      if (v_angle < vertical_angle_min_ || v_angle > vertical_angle_max_)
      {
          ROS_DEBUG("rejected for angle %f not in range (%f, %f)\n", v_angle, vertical_angle_min_, vertical_angle_max_);
          continue;
      }

      // horizontal angle
      double h_angle = atan2(*iter_y, *iter_x);
      if (h_angle < output_laserscan.angle_min || h_angle > output_laserscan.angle_max)
      {
          ROS_DEBUG("rejected for angle %f not in range (%f, %f)\n", h_angle, output_laserscan.angle_min, output_laserscan.angle_max);
          continue;
      }

      //overwrite range at laserscan ray if new range is smaller
      int index = (h_angle - output_laserscan.angle_min) / output_laserscan.angle_increment;
      if (range > output_laserscan.ranges[index])
      {
          output_laserscan.ranges[index] = range;
      }


    }

    laserscan_pub_.publish(output_laserscan);
}
