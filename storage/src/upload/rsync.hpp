///
/// @file rsync.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Uploader using rsync protocol
/// @date 2020-05-13
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <thread>

#include "common/include/utils/process.hpp"
#include "upload.hpp"

namespace wayz {
namespace hera {
namespace storage {
namespace upload {

///
/// @brief Rsync transmisstion
///
/// @see <a href="https://linux.die.net/man/1/rsync target="_blank"
/// rel="noopener noreferrer">rsync-manual</a>
///
class Rsync final : public Manager {
public:
    Rsync(const Config& config);
    Rsync(const Manager&) = delete;
    Rsync& operator=(const Manager&) = delete;

    virtual ~Rsync();

    virtual void terminate() override;

private:
    void parse(const std::string& line);

    std::unique_ptr<common::Process> process_;
    std::unique_ptr<std::thread> thread_out_;
    std::unique_ptr<std::thread> thread_err_;
};

}  // namespace upload
}  // namespace storage
}  // namespace hera
}  // namespace wayz
