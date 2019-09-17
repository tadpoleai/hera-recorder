//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __data_message_data_dummy_hpp__
#define __data_message_data_dummy_hpp__
#include <cstdint>

namespace wayz {

struct DataDummy {
    int32_t DummyInt;
    float DummyFloat;
    char DummyCharArray[4];
};

}  // namespace wayz
#endif