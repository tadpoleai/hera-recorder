#include "common/utils/folder_content.hpp"
#include "slam/result/include/result.hpp"

using namespace wayz::hera;

int main()
{
    log::onlyprint();

    auto result_handler = slam::Result::handler();
    log::info << "Slam Result: Waiting for data" << log::endl;

    while (1) {
        auto result = result_handler->read();
        if (result) {
            log::info << "Slam Result: Get result, w = " << result->width << ", h = " << result->height
                      << ", size = " << file::FileSize(result->data.size()) << ", saved as map.jpg" << log::endl;
            std::ofstream out_file("map.jpg", std::ios::out | std::ios::binary);
            out_file.write((char*)result->data.data(), result->data.size());
            out_file.close();
        } else {
            usleep(100000);
        }
    }
}