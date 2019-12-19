///
/// @file ipc_queue.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief global variable of IPC queue interface
/// @date 2019-12-19
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "ipc_queue.hpp"

namespace wayz {
namespace hera {
namespace ipc {

std::set<key_t> g_opened_keys;   ///< process-global set for opened keys
std::mutex g_mutex_opened_keys;  ///< make mutex for operation on opened_keys_

}  // namespace ipc
}  // namespace hera
}  // namespace wayz