///
/// @file plugin_base.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2021-03-05
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "upload.hpp"

#define HERA_UPLOAD_PLUGIN_DEFINE_START                        \
    class UploadPlugin final : public Transmission {           \
    public:                                                    \
        UploadPlugin(const Config& config);                    \
        UploadPlugin(const Transmission&) = delete;            \
        UploadPlugin& operator=(const Transmission&) = delete; \
                                                               \
        virtual ~UploadPlugin();                               \
        virtual void terminate() override;                     \
                                                               \
    private:

#define HERA_UPLOAD_PLUGIN_DEFINE_END \
    }                                 \
    ;

///
/// @brief Use this macro in a entry's cpp file to export an plugin
///
#define HERA_UPLOAD_PLUGIN_EXPORT(name_param)      \
    extern "C" {                                   \
    UploadPlugin* ctor(const Config& config)       \
    {                                              \
        return new UploadPlugin(config);           \
    }                                              \
                                                   \
    Transmission::PluginEntry exports()            \
    {                                              \
        return {.name = name_param, .ctor = ctor}; \
    }                                              \
    }
