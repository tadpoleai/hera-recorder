///
/// @file main.cpp
/// @author chunchen.wang (chunchen.wang@wayz.ai)
/// @brief
/// @version 0.1
/// @date 2020-04-07
///
/// Copyright 2018 Wayz.ai. All Rights Reserved.
///
///
#include <iostream>

#include "bridge.hpp"

using namespace wayz;

int main(int argc, char** argv)
{
    hera::log::onlyprint();

    wayz::Bridge bridge;
    bridge.spin();
}
