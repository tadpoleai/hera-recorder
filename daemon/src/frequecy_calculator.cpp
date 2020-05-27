///
/// @file frequecy_calculator.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Calculate realtime frequency of device
/// @date 2020-05-27
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "frequecy_calculator.hpp"

#include "common/include/logger/logger.hpp"
#include "common/include/utils/time.hpp"

namespace wayz {
namespace hera {
namespace daemon {

FrequecyCalculator::FrequecyCalculator(std::vector<device::DevicePtr>* devices) :
    result_(devices->size(), 0.0),
    devices_ptr_(devices),
    running_(true),
    thread_(new std::thread(&FrequecyCalculator::thread_function, this))
{}

FrequecyCalculator::~FrequecyCalculator()
{
    running_ = false;
    thread_->join();
}

void FrequecyCalculator::thread_function()
{
    static constexpr auto SampleRate = 50u;
    static constexpr auto SleepTimeUs = 1000000ul / SampleRate;

    static constexpr auto NumZeros = 2u;
    static constexpr auto NumPoles = 2u;

    static const auto NumDevices = result_.size();
    auto seq = new int32_t[NumDevices][2];
    auto xv = new float[NumDevices][NumZeros + 1];
    auto yv = new float[NumDevices][NumPoles + 1];
    auto t0 = time::Timestamp::now();

    bzero(seq, NumDevices * 2 * sizeof(int32_t));
    bzero(xv, NumDevices * (NumZeros + 1) * sizeof(float));
    bzero(yv, NumDevices * (NumPoles + 1) * sizeof(float));

    log::debug << "FrequecyCalculator: Start running" << log::endl;

    while (running_) {
        auto t1 = time::Timestamp::now();
        for (size_t i = 0; i < NumDevices; ++i) {
            seq[i][0] = seq[i][1];
            seq[i][1] = (*devices_ptr_)[i]->get_sequence();
            float input_value = seq[i][1] - seq[i][0];
            float duration = float(t1 - t0) / float(time::OneSecond);
            input_value /= duration;

            /// Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
            /// Command line: /www/usr/fisher/helpers/mkfilter -Bu -Lp -o 2 -a 5.0000000000e-03 0.0000000000e+00
            /// -l
            ///
            /// filtertype	=	Butterworth
            /// passtype	=	Lowpass
            /// ripple	=
            /// order	=	2
            /// samplerate	=	50
            /// corner1	=	0.25
            ///
            static constexpr auto Gain = 4.143204922e+03f;
            xv[i][0] = xv[i][1];
            xv[i][1] = xv[i][2];
            xv[i][2] = input_value / Gain;
            yv[i][0] = yv[i][1];
            yv[i][1] = yv[i][2];
            yv[i][2] = (xv[i][0] + xv[i][2]) + 2 * xv[i][1] + (-0.9565436765 * yv[i][0]) + (1.9555782403 * yv[i][1]);
            result_[i] = yv[i][2];
            /// End of Filter
        }
        t0 = t1;
        usleep(SleepTimeUs);
    }

    log::debug << "FrequecyCalculator: Stop running" << log::endl;
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz
