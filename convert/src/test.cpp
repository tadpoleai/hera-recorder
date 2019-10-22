#include "converter_manager.hpp"
#include "iostream"

using namespace wayz::tron;

int main(int args, char** argv)
{
    auto manager = new ConverterManager("test3.bag", "");
    delete manager;
}
