#include "all_converters.hpp"
#include "iostream"

using namespace wayz::tron;

int main(int args, char** argv)
{
    Converter::open_bag("test.bag");
    Converter* imu_converter =
            new ImuConverter("imu", "internal", "../20191018103800_record_test_1min/Imu/internal/");
    delete imu_converter;
    Converter::close_bag();
}
