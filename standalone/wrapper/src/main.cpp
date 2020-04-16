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
#include <signal.h>
#include <unistd.h>

#include "bridge.hpp"

using namespace wayz;
wayz::Bridge bridge;
///
/// @brief Handler Ctrl+C(SIGINT)
///
/// @param s signal
void sig_int_handler_func(int s)
{
    bridge.stop();
    log::info << "Wrapper: Sigint Received, Stopping" << log::endl;
}


int main(int argc, char** argv)
{
    hera::log::onlyprint();
    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = sig_int_handler_func;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);


    bridge.spin();
}
