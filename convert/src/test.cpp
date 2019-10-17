#include <ros/ros.h>
#include <rosbag/bag.h>
#include <std_msgs/String.h>

#include "iostream"

int main(int args, char** argv)
{
    std_msgs::String a;
    a.data = "a";

    rosbag::Bag bag;
    bag.open("a.bag", rosbag::bagmode::Write);
    bag.write("test/string", ros::Time(1), a);
    bag.close();

    std::cout << a << std::endl;
    return 0;
}
