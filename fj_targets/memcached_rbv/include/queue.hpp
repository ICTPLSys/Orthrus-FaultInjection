#pragma once

#include <boost/lockfree/spsc_queue.hpp>
#include <cstddef>

#include "compiler.hpp"

namespace scee {

constexpr size_t LOG_QUEUE_CAPACITY = 2048;

using LogQueue =
    boost::lockfree::spsc_queue<void *,
                                boost::lockfree::capacity<LOG_QUEUE_CAPACITY>>;

extern thread_local LogQueue log_queue;

inline void log_enqueue(void *log) {
    while (!log_queue.push(log)) {
        cpu_relax();
    }
}

inline void *log_dequeue(LogQueue *q) {
    void *log;
    if (!q->pop(log)) {
        return nullptr;
    }
    return log;
}

}  // namespace scee
