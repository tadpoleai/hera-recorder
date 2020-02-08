///
/// @file ipc_queue.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief IPC queue interface
/// @date 2019-12-19
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cstring>
#include <memory>
#include <mutex>
#include <set>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#include "common/logger/logger.hpp"

namespace wayz {
namespace hera {
namespace ipc {

///
/// @brief Open mode of IPC Queue
///
enum class OpenMode : bool { Read = false, Write = true };

static constexpr size_t DefaultNumElement = 4096UL;    ///< Default quantity of memory slot
static constexpr size_t DefaultElementSize = 16384UL;  ///< Default size of every memory slot

extern std::set<key_t> g_opened_keys;   ///< process-global set for opened keys
extern std::mutex g_mutex_opened_keys;  ///< make mutex for operation on opened_keys_

///
/// @brief Inter process lock-free circular queue
///
/// @tparam DataType data type of element
/// @tparam NumElement quantity of memory slot (element)
/// @tparam ElementSize size of every memory slot (element)
///
/// This is for inter process message communication.
///
/// Usage:
///
/// create an ipc_queue instance by IPCQueue<DataType>::create() in each process,
/// and then call open() with the same key.
///
/// Note, In one process, only one instance of a specific key can be opened, with either write or read mode.
/// the producer should open with write mode and consumer should open with read mode.
///
/// After both side opened, data can be inserted from producer side with write(),
/// and data can be read from consumer side with read().
///
template<class DataType, size_t NumElement = DefaultNumElement, size_t ElementSize = DefaultElementSize>
class IPCQueue final {
public:
    using IPCQueuePtr = std::unique_ptr<IPCQueue>;
    using DataPtr = std::shared_ptr<DataType>;

    ///
    /// @brief Check if class DataType has member function, size_t serialize(void* dest, size_t max_size)
    ///
    /// where, serialize() is a class member function of DataType that
    /// @@brief Serialize to memory
    ///
    /// @@param dest destination pointer
    /// @@param max_size max size of destination
    /// @@return size_t 0 if failed, otherwise size after serialization
    ///
    static_assert(std::is_same<size_t,
                               decltype(std::declval<DataType&>().serialize(std::declval<void*>(),
                                                                            std::declval<size_t>()))>::value,
                  "DataType does not have member function, size_t serialize(void* dest, size_t max_size)");

    ///
    /// @brief Check if DataType has static member function, DataPtr deserialize(void* src, size_t max_size)
    ///
    /// where, deserialize() is a static class member function that
    /// @@brief Deserialize from memory
    ///
    /// @@param src source pointer
    /// @@param max_size max size of source
    /// @@return DataPtr a shared pointer to deserialized DataType object, if succeed, otherwise nullptr
    ///
    static_assert(std::is_same<DataPtr,
                               decltype(DataType::deserialize(std::declval<void*>(), std::declval<size_t>()))>::value,
                  "DataType does not have static member function, DataPtr deserialize(void* src, size_t max_size)");

    static_assert(ElementSize != 0, "ElementSize must must not be 0");
    static_assert(ElementSize % 256 == 0, "ElementSize must be multiple of 256");
    static_assert(NumElement >= 2, "NumElement must be larger than 2");
    static_assert(NumElement * ElementSize <= 256 * (1 << 20),
                  "Size of shared memory data segment should be smaller than 256MiB");

private:
#pragma pack(push, 1)
    static constexpr uint32_t MagicBase = 0x4441'7A9F;  ///< Magic number flag
    static constexpr uint32_t MagicKey = 0x0001'BF52;   /// Magic number of key

    ///
    /// @brief Header information of shared memory
    ///
    struct SharedMemoryHeader {
        volatile uint32_t read_magic;   ///< reader's magic
        volatile uint32_t write_magic;  ///< write's magic
        volatile int32_t head;          ///< head of circular queue
        volatile int32_t tail;          ///< tail of circular queue
    };

    ///
    /// @brief Data structure of shared memory
    ///
    struct SharedMemory {
        volatile SharedMemoryHeader header;     ///< header information
        uint8_t data[NumElement][ElementSize];  ///< circular queue data
    };

#define NullSharedPtr (reinterpret_cast<SharedMemory*>(-1))
    ///< shmat() returns a nullptr valued -1, make it happy

#pragma pack(pop)

public:
    ///
    /// @brief Create an instance of IPCQueue
    ///
    /// @return IPCQueuePtr unique pointer to created instance
    ///
    static IPCQueuePtr create()
    {
        return IPCQueuePtr(new IPCQueue());
    }

    IPCQueue(const IPCQueue&) = delete;
    IPCQueue& operator=(const IPCQueue&) = delete;

    ///
    /// @brief Open IPC with shared memory
    ///
    /// @param key key of shared memory
    /// @param mode write mode or read mode
    /// @return true succeed
    /// @return false failed
    ///
    bool open(const uint32_t key, const OpenMode mode)
    {
        // Check if opened by this instance
        if (is_open() || is_closed_) {
            log::error << "IPCQueue: Can not open key = '" << key << "', mode = '"
                       << (mode == OpenMode::Write ? "w" : "r") << "', already opened" << log::endl;
            return false;
        }

        // Check if opened by other instance in this process
        bool already_opened_by_other = false;
        g_mutex_opened_keys.lock();
        if (g_opened_keys.count(key) != 0) {
            already_opened_by_other = true;
        } else {
            already_opened_by_other = false;
            g_opened_keys.insert(key);
        }
        g_mutex_opened_keys.unlock();
        if (already_opened_by_other) {
            log::error << "IPCQueue: Can not open key = '" << key << "', mode = '"
                       << (mode == OpenMode::Write ? "w" : "r") << "', already opened by other" << log::endl;
            return false;
        }

        // Create or open shared memory
        mode_ = mode;
        key_ = key;
        auto ipc_mode = 0777 | IPC_CREAT;
        shm_id_ = shmget((key_t)(key + MagicKey), sizeof(SharedMemory), ipc_mode);
        if (shm_id_ == -1) {
            log::error << "IPCQueue: Can not open key = '" << key << "', mode = '"
                       << (mode == OpenMode::Write ? "w" : "r") << "', can not create" << log::endl;
            close();
            return false;
        }

        // Attach shared memory's address
        shm_ptr_ = reinterpret_cast<SharedMemory*>(shmat(shm_id_, nullptr, 0));
        if (shm_ptr_ == NullSharedPtr) {
            log::error << "IPCQueue: Can not open key = '" << key << "', mode = '"
                       << (mode == OpenMode::Write ? "w" : "r") << "', can not attach" << log::endl;
            close();
            return false;
        }

        // Write magic and initialize head / tail
        magic_ = MagicBase ^ key;
        if (mode_ == OpenMode::Write) {
            if (shm_ptr_->header.read_magic == magic_) {
                shm_ptr_->header.head = shm_ptr_->header.tail;
            } else {
                shm_ptr_->header.head = 0;
            }
            shm_ptr_->header.write_magic = magic_;
        } else {
            if (shm_ptr_->header.write_magic == magic_) {
                shm_ptr_->header.tail = shm_ptr_->header.head;
            } else {
                shm_ptr_->header.tail = 0;
            }
            shm_ptr_->header.read_magic = magic_;
        }

        log::info << "IPCQueue: Opened key = '" << key << "', mode = '" << (mode == OpenMode::Write ? "w" : "r") << "'"
                  << log::endl;
        return true;
    }

    ///
    /// @brief Close opened shared memory
    ///
    void close()
    {
        if (is_closed_) {
            return;
        }

        is_closed_ = true;
        bool to_remove = false;
        if (shm_ptr_ != NullSharedPtr) {
            if (mode_ == OpenMode::Write) {
                shm_ptr_->header.write_magic = 0;
                if (shm_ptr_->header.read_magic == 0) {
                    to_remove = true;
                }
            } else {
                shm_ptr_->header.read_magic = 0;
                if (shm_ptr_->header.write_magic == 0) {
                    to_remove = true;
                }
            }
            shmdt(shm_ptr_);
        }

        if (shm_id_ != -1) {
            if (to_remove) {
                shmctl(shm_id_, IPC_RMID, 0);
            }
        }

        g_mutex_opened_keys.lock();
        if (g_opened_keys.count(key_) != 0) {
            g_opened_keys.erase(key_);
        }
        g_mutex_opened_keys.unlock();
    }

    ///
    /// @brief Get if shared memory is already for write / read
    ///
    /// @return true shared memory is already
    /// @return false otherwise
    ///
    inline bool is_open() const noexcept
    {
        return shm_id_ != -1 && shm_ptr_ != NullSharedPtr;
    }

    ///
    /// @brief Get if shared memory is writable
    ///
    /// @return true shared memory is writable
    /// @return false otherwise
    ///
    bool writable() const noexcept
    {
        if (!is_open() || (mode_ == OpenMode::Read) || is_closed_) {
            return false;
        }

        // No reader
        if (shm_ptr_->header.read_magic != magic_) {
            return false;
        }

        auto tail = shm_ptr_->header.tail;
        auto head = shm_ptr_->header.head;

        // Queue full
        if ((tail - head) % NumElement == 1) {
            return false;
        }

        return true;
    }

    ///
    /// @brief Insert a data to circular queue
    ///
    /// @param data a shared pointer of a serializable data
    /// @return true succeed
    /// @return false failed
    ///
    bool write(DataPtr& data)
    {
        if (!is_open() || (mode_ == OpenMode::Read) || is_closed_) {
            return false;
        }

        // No reader
        if (shm_ptr_->header.read_magic != magic_) {
            return false;
        }

        auto tail = shm_ptr_->header.tail;
        auto head = shm_ptr_->header.head;

        // Queue full
        if ((tail - head) % NumElement == 1) {
            return false;
        }

        // Write data
        void* dest = &shm_ptr_->data[head];
        if (data->serialize(dest, ElementSize) != 0) {
            auto next_head = head + 1;
            next_head %= NumElement;
            shm_ptr_->header.head = next_head;
            return true;
        } else {
            return false;
        }
    }

    ///
    /// @brief Insert a data to circular queue
    ///
    /// @param data a serializable data
    /// @return true succeed
    /// @return false failed
    ///
    bool write(const DataType& data)
    {
        if (!is_open() || (mode_ == OpenMode::Read) || is_closed_) {
            return false;
        }

        // No reader
        if (shm_ptr_->header.read_magic != magic_) {
            return false;
        }

        auto tail = shm_ptr_->header.tail;
        auto head = shm_ptr_->header.head;

        // Queue full
        if ((tail - head) % NumElement == 1) {
            return false;
        }

        // Write data
        void* dest = &shm_ptr_->data[head];
        if (data.serialize(dest, ElementSize) != 0) {
            auto next_head = head + 1;
            next_head %= NumElement;
            shm_ptr_->header.head = next_head;
            return true;
        } else {
            return false;
        }
    }

    ///
    /// @brief Pop a data from circular queue
    ///
    /// @return a shared pointer of a serializable data, if succeed, otherwise, nullptr
    ///
    DataPtr read()
    {
        if (!is_open() || (mode_ == OpenMode::Write) || is_closed_) {
            return nullptr;
        }

        // No writer
        if (shm_ptr_->header.write_magic != magic_) {
            return nullptr;
        }

        auto tail = shm_ptr_->header.tail;
        auto head = shm_ptr_->header.head;

        // Queue empty
        if (tail == head) {
            return nullptr;
        }

        // Read data
        void* src = &shm_ptr_->data[tail];
        auto data = DataType::deserialize(src, ElementSize);
        auto next_tail = tail + 1;
        next_tail %= NumElement;
        shm_ptr_->header.tail = next_tail;

        return data;
    }

private:
    ///
    /// @brief Construct a new IPCQueue object
    ///
    IPCQueue() : mode_(OpenMode::Read), is_closed_(false), shm_id_(-1), shm_ptr_(NullSharedPtr) {}

public:
    ///
    /// @brief Destroy the IPCQueue object
    ///
    ~IPCQueue()
    {
        close();
    };

private:
    uint32_t key_;    // key of shared memory
    OpenMode mode_;   ///< open mode of IPC Queue
    bool is_closed_;  ///< is this queue already closed

    uint64_t magic_;         ///< magic number of this queue / key
    int shm_id_;             ///< shared memory id, -1 indicates invalid
    SharedMemory* shm_ptr_;  ///< pointer to attached shared memory, -1 indicates nullptr
};

}  // namespace ipc
}  // namespace hera
}  // namespace wayz
