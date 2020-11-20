///
/// @file rsync.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Uploader using rsync protocol
/// @date 2020-05-13
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <thread>

#include "upload.hpp"

#ifdef LIBNFS_FOUND
#include "libnfs.h"
//
#include "libnfs-raw.h"
//
#include "libnfs-raw-mount.h"
#endif

namespace wayz {
namespace hera {
namespace storage {
namespace upload {

///
/// @brief NFS transmisstion
///
class Nfs final : public Manager {
public:
    Nfs(const Config& config);
    Nfs(const Manager&) = delete;
    Nfs& operator=(const Manager&) = delete;

    virtual ~Nfs();

    virtual void terminate() override;

private:
#ifdef LIBNFS_FOUND
    std::unique_ptr<std::thread> thread_run_;

    int source_file_handler_{-1};

    struct nfs_context* nfs_context_{nullptr};
    struct nfsfh* nfs_file_handler_{nullptr};

#endif
};

}  // namespace upload
}  // namespace storage
}  // namespace hera
}  // namespace wayz
