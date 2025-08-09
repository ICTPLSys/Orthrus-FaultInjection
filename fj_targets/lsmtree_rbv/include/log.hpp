#pragma once

#include <atomic>
#include <boost/lockfree/spsc_queue.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <stack>
#include <type_traits>

#include "assertion.hpp"
#include "compiler.hpp"
#include "free_log.hpp"
#include "memmgr.hpp"
#include "profile.hpp"
#include "queue.hpp"
#include "spin_lock.hpp"
#include "utils.hpp"

/*
    Memory layout of a log buffer:
    |--------------------------------|
    | uint64_t nr_logs               |
    | padding to 64 bytes            |
    | uint64_t in_use                |
    | uint64_t nr_reclaimed          |
    | padding to 64 bytes            |
    |--------------------------------|
    | log 1 | uint32_t length        |
    |       | uint32_t reclaimed     |
    |       | uint64_t gc_tsc        |
    |       |                        |
    |       | (aligned with 8 bytes) |
    |       | DATA ...               |
    |       |                        |
    |       | uint32_t length        |
    |       | uint32_t 0x0000DEAD    |
    | padding to 64 bytes            |
    |--------------------------------|
    | log 2 | ...                    |
    |--------------------------------|
    | ...   | ...                    |
    |--------------------------------|
    | log n | ...                    |
    |--------------------------------|
    | padding | uint32_t length      |
    |         | uint32_t 0x1         |
    |         | ...                  |
    |--------------------------------|

*/

namespace scee {

constexpr bool CHECK_OVERFLOW_ON_APPEND = false;
constexpr bool CHECK_OVERFLOW_ON_COMMIT = false;

#define SET_LSMTREE_LOG_SIZE
#ifdef SET_LSMTREE_LOG_SIZE
constexpr size_t MIN_LOG_BUFFER_SIZE = 4096 * 1;
constexpr size_t MAX_LOG_BUFFER_SIZE = 4096 * 16 * 1;
#else
constexpr size_t MIN_LOG_BUFFER_SIZE = 4096;
constexpr size_t MAX_LOG_BUFFER_SIZE = 4096 * 16;
#endif

struct LogBufferHead {
    uint64_t nr_logs;
    std::byte padding1[CACHELINE_SIZE - 8];
    uint64_t in_use;
    std::atomic<uint64_t> nr_reclaimed;
    std::byte padding2[CACHELINE_SIZE - 16];
};

struct LogHead {
    uint32_t length;
    uint32_t reclaimed;
    uint64_t gc_tsc;
    uint64_t start_tsc;
};

struct LogTail {
    uint32_t length;
    uint32_t magic;

    constexpr static uint32_t MAGIC = 0x0000DEAD;
};

struct Log {
    void *cursor;
    LogHead *head;
};

static_assert(sizeof(LogBufferHead) == CACHELINE_SIZE * 2);

inline LogBufferHead *get_log_buffer_head(void *log) {
    static_assert(is_power_of_2(MAX_LOG_BUFFER_SIZE));
    uintptr_t addr =
        reinterpret_cast<uintptr_t>(log) & ~(MAX_LOG_BUFFER_SIZE - 1);
    return reinterpret_cast<LogBufferHead *>(addr);
}

inline bool is_buffer_exhausted(LogHead *log) {
    auto *buffer = get_log_buffer_head(log);
    void *cursor = add_byte_offset(log, align_size_to_cacheline(log->length));
    return ptr_distance(buffer, cursor) >
           MAX_LOG_BUFFER_SIZE - MIN_LOG_BUFFER_SIZE;
}

struct GlobalLogBufferAllocator {
    static SpinLock spin_lock;
    static std::queue<void *> free_buffers;
};

// allocate a new, free log buffer
// each buffer has a size of MAX_LOG_BUFFER_SIZE
inline void *allocate_log_buffer() {
    void *buffer;
    GlobalLogBufferAllocator::spin_lock.Lock();
    if (GlobalLogBufferAllocator::free_buffers.empty()) {
        // fprintf(stderr, "new buffer\n");
        buffer = std::aligned_alloc(MAX_LOG_BUFFER_SIZE, MAX_LOG_BUFFER_SIZE);
    } else {
        buffer = GlobalLogBufferAllocator::free_buffers.front();
        GlobalLogBufferAllocator::free_buffers.pop();
    }
    GlobalLogBufferAllocator::spin_lock.Unlock();
    return buffer;
}

// reclaim a log
inline void reclaim_log(LogHead *log) {
    closure_start_log.validated_closure(log->gc_tsc,
                                        &app_thread_gc_instance->free_log);
    LogBufferHead *buffer = get_log_buffer_head(log);
    buffer->nr_reclaimed.fetch_add(1, std::memory_order_relaxed);
    // check `in_use` first to avoid false sharing
    // if `in_use` is not 0
    // cacheline of `nr_logs` is MODIFIED in mutator thread
    if (unlikely(buffer->in_use == 0)) {
        if (buffer->nr_reclaimed == buffer->nr_logs) {
            GlobalLogBufferAllocator::spin_lock.Lock();
            GlobalLogBufferAllocator::free_buffers.push(buffer);
            GlobalLogBufferAllocator::spin_lock.Unlock();
        }
    }
}

class ThreadLogAllocator {
public:
    LogHead *allocate() {
        if (unlikely(buffers.empty())) {
            auto *buffer = static_cast<LogBufferHead *>(allocate_log_buffer());
            buffer->nr_logs = 0;
            buffer->in_use = 1;
            buffer->nr_reclaimed.store(0, std::memory_order_relaxed);
            void *next_log_addr =
                add_byte_offset(buffer, sizeof(LogBufferHead));
            static_assert(MAX_LOG_BUFFER_SIZE >=
                          sizeof(LogHead) + MIN_LOG_BUFFER_SIZE);
            return static_cast<LogHead *>(next_log_addr);
        }

        auto *log = buffers.top();
        buffers.pop();
        return static_cast<LogHead *>(log);
    }

    void commit(LogHead *log) {
        if constexpr (CHECK_OVERFLOW_ON_COMMIT) {
            if (unlikely(log->length > MIN_LOG_BUFFER_SIZE)) {
                fprintf(stderr, "Error: log length %u exceeded the limit %lu\n",
                        log->length, MIN_LOG_BUFFER_SIZE);
                std::abort();
            }
        }

        auto *buffer = get_log_buffer_head(log);
        buffer->nr_logs++;
        void *next_log_addr =
            add_byte_offset(log, align_size_to_cacheline(log->length));

        // check if there are enough space to reuse this buffer
        if (likely(ptr_distance(buffer, next_log_addr) <=
                   MAX_LOG_BUFFER_SIZE - MIN_LOG_BUFFER_SIZE)) {
            // there are enough space to reuse this buffer
            buffers.push(next_log_addr);
        } else {
            // no enough space to reuse this buffer
            // this is relaxed memory order
            // because we will enqueue this log latter
            buffer->in_use = 0;
        }
    }

public:
    // buffers:
    // pointers to the first unused memory in the thread-local log buffers
    // each buffer has a size no less than MIN_LOG_BUFFER_SIZE
    std::stack<void *> buffers;
};

struct ThreadLogManager {
    Log current_log;
    std::stack<Log> caller_logs;
    ThreadLogAllocator allocator;
};

// TODO(quanxi): is this safe in uthread?
extern thread_local ThreadLogManager thread_log_manager;

inline ThreadLogManager *get_thread_log_manager() {
    return &thread_log_manager;
}

inline Log *get_current_log() { return &get_thread_log_manager()->current_log; }

// allocate a new log for current thread
// if there is already an old log in use (for the caller closure),
// stash the old log and allocate a new one
inline void new_log() {
    auto *manager = get_thread_log_manager();
    // fprintf(stderr, "caller_logs.size: %zu\n", manager->caller_logs.size());
    // fprintf(stderr, "buffers.size: %zu\n", manager->allocator.buffers.size());
    // if there is already a log in use, stash it
    // if (manager->current_log.head) {
    //     manager->caller_logs.push(manager->current_log);
    // }
    // allocate a new log
    LogHead *log = manager->allocator.allocate();
    log->reclaimed = 0;
    log->gc_tsc = closure_start_log.new_closure();
    log->start_tsc = rdtsc();
    manager->current_log.head = log;
    manager->current_log.cursor = add_byte_offset(log, sizeof(LogHead));
}

template <size_t Size>
inline const void *append_log(const void *data) {
    constexpr size_t AlignedSize = (Size + 7) & ~7;
    auto *log = get_current_log();
    void *dst = log->cursor;
    memcpy(dst, data, Size);
    log->cursor = add_byte_offset(dst, AlignedSize);
    if constexpr (CHECK_OVERFLOW_ON_APPEND) {
        auto length = ptr_distance(log->head, log->cursor);
        if (unlikely(length >= MIN_LOG_BUFFER_SIZE)) {
            fprintf(stderr, "Error: log length %u exceeded the limit %lu\n",
                    length, MIN_LOG_BUFFER_SIZE);
            std::abort();
        }
    }
    return dst;
}

template <typename T>
inline auto append_log_typed(T &&data) {
    using U = std::remove_cvref_t<T>;
    constexpr size_t AlignedSize = (sizeof(U) + 7) & ~7;
    auto *log = get_current_log();
    void *dst = log->cursor;
    new (dst) U(std::forward<U>(data));
    log->cursor = add_byte_offset(dst, AlignedSize);
    if constexpr (CHECK_OVERFLOW_ON_APPEND) {
        auto length = ptr_distance(log->head, log->cursor);
        if (unlikely(length >= MIN_LOG_BUFFER_SIZE)) {
            fprintf(stderr, "Error: log length %u exceeded the limit %lu\n",
                    length, MIN_LOG_BUFFER_SIZE);
            std::abort();
        }
    }
    return static_cast<const U *>(dst);
}

inline void commit_log() {
    auto *manager = get_thread_log_manager();
    auto log = manager->current_log;
    auto *log_tail = static_cast<LogTail *>(log.cursor);
    log.cursor = add_byte_offset(log.cursor, sizeof(LogTail));
    uint32_t log_length = ptr_distance(log.head, log.cursor);
    log.head->length = log_length;
    *log_tail = {.length = log_length, .magic = LogTail::MAGIC};
    manager->allocator.commit(log.head);
    log_enqueue(log.head);
}

class LogReader {
public:
    LogReader() = default;

    explicit LogReader(LogHead *log) : log(log) { cursor = log + 1; }

    void open(LogHead *log) {
        this->log = log;
        cursor = log + 1;
    }

    template <size_t Size>
    inline void fetch_log(void *data) {
        constexpr size_t AlignedSize = (Size + 7) & ~7;
        memcpy(data, cursor, Size);
        cursor = add_byte_offset(cursor, AlignedSize);
    }

    template <typename T>
    inline void fetch_log_typed(T *data) {
        static_assert(std::is_trivially_copyable_v<T>);
        fetch_log<sizeof(T)>(data);
    }

    void close() {
        LogTail tail;
        fetch_log_typed(&tail);
        if (tail.magic != LogTail::MAGIC) {
            fprintf(stderr, "Error: log tail magic number mismatch\n");
            std::abort();
        }
        if (tail.length != ptr_distance(log, cursor)) {
            fprintf(stderr, "Error: log length mismatch\n");
            std::abort();
        }
        uint64_t validation_latency = rdtsc() - log->start_tsc;
        profile::record_validation_latency(validation_latency);
        reclaim_log(log);
    }

    template <size_t Size>
    inline void skip() {
        constexpr size_t AlignedSize = (Size + 7) & ~7;
        cursor = add_byte_offset(cursor, AlignedSize);
    }

    template <typename T>
    inline const T *peek() {
        return static_cast<const T *>(cursor);
    }

    template <size_t Size>
    inline void cmp_log(const void *data) {
        constexpr size_t AlignedSize = (Size + 7) & ~7;
        bool same = !memcmp(data, cursor, Size);
        validator_assert(same);
        cursor = add_byte_offset(cursor, AlignedSize);
    }

    template <typename T>
    inline void cmp_log_typed(const T &data) {
        cmp_log<sizeof(data)>(&data);
    }

private:
    LogHead *log = nullptr;
    void *cursor = nullptr;
};

// for validator threads
// TODO(quanxi): merge this with thread_log_manager
extern thread_local LogReader log_reader;

}  // namespace scee
