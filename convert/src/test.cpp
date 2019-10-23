#include <common/logger/logger.hpp>

#include "converter_manager.hpp"

using namespace wayz::tron;

int main(int args, char** argv)
{
    Logger::create("logs");
    Logger::info() << "Converter: Conversion Started" << Logger::endl;
    auto manager = new ConverterManager("test3.bag", "");
    delete manager;
}
