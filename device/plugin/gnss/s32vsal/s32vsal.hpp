///
/// @file s32vsal.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-04-23
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#pragma once

#include "data/gnss_data.hpp"
#include "device.hpp"

#ifdef WITH_DRIVER
#include "S32vSAL/gnss/GNSSInterface.h"
#endif

namespace wayz {
namespace hera {
namespace device {
namespace gnss {
namespace s32vsal {

#pragma pack(push, 1)

///
/// @brief Device data for S32VSal's Data
///
class S32VSalData final : public data::DeviceData {
public:
    S32VSalData() = delete;

public:
    enum class QualityType : uint8_t {
        Invalid = 0u,
        SinglePoint = 1u,
        Pseudorangedifferential = 2u,
        Sensitive = 3u,
        RTKFixed = 4u,
        RTKFloating = 5u,
        Estimated = 6u,
        Manual = 7u,
        Simulation = 8u,
    };

public:
    ///
    /// @brief Data structure of GNSSS32VSalData
    ///
    struct S32VSalDataUnion {
        uint64_t timestamp_intrinsic_ns;  ///< UTC Timestamp in ns
        double latitude;                  ///< Latitude [degree].
        double longitude;                 ///< Longitude [degree].
        double altitude;                  ///< Altitude over sea level, represented following WGS84 [m].
        double speed;                     ///< Horizontal speed [m/s].
        QualityType quality;              ///< Quality Indicators
        double pdop;                      ///< Position Dilution of Precision
        double hdop;                      ///< Horizontal Dilution of Precision
        double vdop;                      ///< Vertical Dilution of Precision
        uint8_t latitude_valid;
        uint8_t longitude_valid;
        uint8_t altitude_valid;
        uint8_t speed_valid;
        uint8_t pdop_valid;
        uint8_t hdop_valid;
        uint8_t vdop_valid;
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        S32VSalDataUnion data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(S32VSalDataUnion)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

///
/// @brief A GNSS-Device, provided by library S32VSal
///
class S32VSal final : public Device {
public:
    ///
    /// @brief Construct a new S32VSal object
    ///
    /// @note pass DataPort as EssentialParameterTypes
    /// @see Device::Device()
    S32VSal(const uint32_t id,
            const std::string& vendor_type,
            const std::string& name,
            const bool forward,
            ipc::IPCQueue<data::SensorData>* const ipc_queue,
            storage::StorageManager* const storage) :
        Device(id, vendor_type, name, forward, ipc_queue, storage, HistoryDepth_, EssentialParameterTypes)
    {}
    S32VSal(const S32VSal&) = delete;
    S32VSal& operator=(const S32VSal&) = delete;

    ///
    /// @brief Destroy the S32VSal object
    ///
    /// calls Device::stop()
    virtual ~S32VSal()
    {
        stop();
    }

#ifdef WITH_DRIVER
    virtual HeraErrno connect() override;

    virtual void disconnect() override;

    virtual data::DeviceDataPtr fetch() override;

    // virtual HeraErrno adjust_parameter(const std::string& type, const std::string& value) override;
#endif

    virtual data::SensorDataPtr convert(data::DeviceDataPtr& storage_data) override
    {
        return do_convert(storage_data);
    }

    ///
    /// @brief Static convert function for read / convert / replay
    ///
    static data::SensorDataPtr do_convert(data::DeviceDataPtr& storage_data);

public:
    static const std::vector<std::string> EssentialParameterTypes;  ///< Essential Parameters for device

    static const std::vector<std::string> OptionalParameterTypes;  ///< Optional Parameters for device

private:
    static constexpr size_t HistoryDepth_ = 1;  ///< History Depth, 1

#ifdef WITH_DRIVER
    bool init_called_;
    EGNSSSensorType gnss_type_;
#endif
};

}  // namespace s32vsal
}  // namespace gnss
}  // namespace device
}  // namespace hera
}  // namespace wayz
