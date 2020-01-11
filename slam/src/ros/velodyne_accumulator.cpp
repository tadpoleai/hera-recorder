#include "velodyne_accumulator.hpp"

VelodyneAccumulator::VelodyneAccumulator(ros::NodeHandle& node,
                                                     ros::NodeHandle& privateNode)
{

    num_packet_accumulate_ = private_node_.param<int>("num_accumulated_packet", 181);

    // buffer size 10 times scan's number(can't use resize)
    packet_buffer_.set_capacity(10 * num_packet_accumulate_);

    cloud_correct_thread_ =
            std::thread(&VelodyneAccumulator::accumulated_points_thread, this);

    //TODO:Hard code advertise topic name
    corrected_points_pub =
            node_.advertise<sensor_msgs::PointCloud2>("/corrected_points", 10, false);
}

VelodyneAccumulator::~VelodyneAccumulator()
{
    cloud_correct_thread_.join();
}

void VelodyneAccumulator::original_points_callback(
        const sensor_msgs::PointCloud2::ConstPtr& points_msg)

{
    std::unique_lock<std::mutex> lock(cloud_correct_mutex_);
    // cloud_correct_mutex_.lock();
    packet_buffer_.push_back(points_msg);

    if(packet_buffer_.size() >= num_packet_accumulate_)
    {
        fill_buffer_cv_.notify_all();
    }
    // cloud_correct_mutex_.unlock();
}

void VelodyneAccumulator::accumulated_points_thread()
{
    cloud_correct_mutex_.lock();

    std::unique_lock<std::mutex> lock_2(cloud_correct_mutex_);
    fill_buffer_cv_.wait(lock_2, [&]()->bool { return packet_buffer_.size() <  num_packet_accumulate_;});

        if (packet_buffer_.size() >= num_packet_accumulate_) {
            std::vector<sensor_msgs::PointCloud2ConstPtr> point_scan(num_packet_accumulate_);
            std::vector<ros::Time> stamps(num_packet_accumulate_);

            ROS_INFO("buffer size%ld", packet_buffer_.size());

            // circular buffer copy to a new container & unlock
            auto it = packet_buffer_.begin();
            std::advance(it, num_packet_accumulate_);
            std::copy(packet_buffer_.begin(), it, point_scan.begin());
            packet_buffer_.erase(packet_buffer_.begin(), it);

            cloud_correct_mutex_.unlock();

            int idx = 0;
            std::for_each(
                    point_scan.begin(),
                    point_scan.end(),
                    [&](const sensor_msgs::PointCloud2ConstPtr& packet) {
                        ros::Time ros_time =
                                packet->header
                                        .stamp;  // pcl_conversions::fromPCL(packet->header.stamp);
                        stamps[idx++] = std::move(ros_time);
                    });

            // ROS_INFO("STEP 03");

            std::vector<nav_msgs::Odometry> odoms;
            odoms.resize(num_packet_accumulate_);
#if 0
            startup::srv_localization_array_pose srv;
            ros::service::waitForService("localization_server", 100);
            srv.request.timestamps = stamps;

            // ROS_INFO("STAMP SIZE:%d", num_accumulated_range_data);
            if (pose_service_client.call(srv)) {
                odoms = srv.response.require_poses;
            }
#endif
            // ROS_INFO("STEP 04, odoms:%ld", odoms.size());

            // TODO:stand out a function later
            pcl::PointCloud<PointT>::Ptr cloud_corrected_accumulated;
            cloud_corrected_accumulated.reset(new pcl::PointCloud<PointT>());
            pcl::PointCloud<PointT>::Ptr cloud_corrected(new pcl::PointCloud<PointT>());
            idx = 0;
            std::for_each(odoms.begin(), odoms.end(), [&](const nav_msgs::Odometry& odom_pose) {
                const auto& packet_pose = odom_pose.pose.pose;
                const auto& cloud = point_scan[idx++];

                // transform
                Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
                transform(0, 3) = packet_pose.position.x;
                transform(1, 3) = packet_pose.position.y;
                transform(2, 3) = packet_pose.position.z;

                auto q1 = Eigen::Quaternionf(packet_pose.orientation.w,
                                             packet_pose.orientation.x,
                                             packet_pose.orientation.y,
                                             packet_pose.orientation.z);
                transform.block<3, 3>(0, 0) = q1.toRotationMatrix();

                pcl::PointCloud<PointT>::Ptr cloud_lidar(new pcl::PointCloud<PointT>());
                pcl::fromROSMsg(*cloud, *cloud_lidar);
                pcl::transformPointCloud(*cloud_lidar, *cloud_corrected, transform);
                cloud_corrected_accumulated->insert(cloud_corrected_accumulated->end(),
                                                    cloud_corrected->begin(),
                                                    cloud_corrected->end());

            });

            // ROS_INFO("STEP 06");
            pcl::PointCloud<PointT>::Ptr cloud_corrected_accumulated_pub(
                    new pcl::PointCloud<PointT>());
            Eigen::Matrix4f transform0 = Eigen::Matrix4f::Identity();

            geometry_msgs::Pose scan_pose;
            scan_pose = odoms.back().pose.pose;
            transform0(0, 3) = scan_pose.position.x;
            transform0(1, 3) = scan_pose.position.y;
            transform0(2, 3) = scan_pose.position.z;
            auto q0 = Eigen::Quaternionf(scan_pose.orientation.w,
                                         scan_pose.orientation.x,
                                         scan_pose.orientation.y,
                                         scan_pose.orientation.z);
            transform0.block<3, 3>(0, 0) = q0.toRotationMatrix();
            pcl::transformPointCloud(*cloud_corrected_accumulated,
                                     *cloud_corrected_accumulated_pub,
                                     transform0.inverse());

            pcl_conversions::toPCL(stamps.back(), cloud_corrected_accumulated_pub->header.stamp);

            //TODO:hard-code
            cloud_corrected_accumulated_pub->header.frame_id = "velodyne";
            corrected_points_pub.publish(cloud_corrected_accumulated_pub);

            cloud_corrected_accumulated->clear();
        // } else {
        //     cloud_correct_mutex_.unlock();
        }
}
