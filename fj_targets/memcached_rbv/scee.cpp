#include "scee.hpp"

#include "compiler.hpp"
#include "free_log.hpp"
#include "log.hpp"
#include "profile.hpp"
#include "queue.hpp"
#include "thread.hpp"

// #define DISABLE_VALIDATION

namespace scee {

// queue.hpp
thread_local LogQueue log_queue;

// free_log.hpp
thread_local ThreadGC thread_gc_instance;
thread_local ThreadGC *app_thread_gc_instance = nullptr;
ClosureStartLog closure_start_log;

// log.hpp
SpinLock GlobalLogBufferAllocator::spin_lock;
std::queue<void *> GlobalLogBufferAllocator::free_buffers;
thread_local ThreadLogManager thread_log_manager;
thread_local LogReader log_reader;

// thread.hpp
int sampling_rate = 100, sampling_method = 1, core_id = 0;

std::atomic_size_t n_validation_core = 0;
size_t max_validation_core = 0;

// scee.hpp
void validate_one(LogHead *log) {
    bool do_validation = true;
    if (sampling_rate < 100) {
        if (sampling_method == 1) {
            do_validation = rand() % 100 < sampling_rate;
        } else {
            // NOT IMPLEMENTED
            assert(false);
        }
    }
#ifdef DISABLE_VALIDATION
    do_validation = false;
#endif
    auto validate = [&] {
        log_reader.open(log);
        reset_bulk_buffer();
        const auto *validable = log_reader.peek<Validable>();
        validable->validate(&log_reader);
        log_reader.close();
    };

    if (do_validation) {
        if (max_validation_core != 0) {
            if (n_validation_core.fetch_add(1, std::memory_order_relaxed) <
                max_validation_core) {
                validate();
            } else {
                reclaim_log(log);
            }
            n_validation_core.fetch_sub(1, std::memory_order_relaxed);
        } else {
            validate();
        }
    } else {
        reclaim_log(log);
    }
}

// thread.hpp
thread_local Thread validator_thread;
thread_local std::atomic<bool> stop_validation;

// memmgr.hpp
thread_local void *bulk_buffer = nullptr;
thread_local size_t bulk_cursor = BULK_BUFFER_SIZE;

void validate(LogQueue *queue, std::atomic<bool> &stop, ThreadGC *thread_gc) {
    app_thread_gc_instance = thread_gc;
    while (!stop) {
        while (queue->empty() && !stop) {
            cpu_relax();
        }
        const uint64_t start = rdtsc();
        size_t validation_count = 0;
        while (true) {
            auto *log = static_cast<LogHead *>(log_dequeue(queue));
            if (log == nullptr) {
                break;
            }
            validate_one(log);
            validation_count++;
        }
        const uint64_t end = rdtsc();
        if (validation_count > 0) {
            profile::record_validation_cpu_time(end - start, validation_count);
        }
    }
}

void AppThread::register_queue() {
    stop_validation = false;
    LogQueue *queue = &log_queue;
#ifndef DISABLE_SCEE
    validator_thread =
        Thread(validate, queue, std::ref(stop_validation), &thread_gc_instance);
#endif
}

void AppThread::unregister_queue() {
    stop_validation = true;
#ifndef DISABLE_SCEE
    validator_thread.join();
#endif
}

// scheduler.hpp

}  // namespace scee
