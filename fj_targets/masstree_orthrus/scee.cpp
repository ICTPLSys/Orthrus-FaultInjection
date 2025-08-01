#include "scee.hpp"

#include "compiler.hpp"
#include "free_log.hpp"
#include "log.hpp"
#include "queue.hpp"
#include "thread.hpp"

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

// scee.hpp
void validate_one(LogHead *log) {
    log_reader.open(log);
    const auto *validable = log_reader.peek<Validable>();
    validable->validate(&log_reader);
    log_reader.close();
}

// thread.hpp
thread_local Thread validator_thread;
thread_local std::atomic<bool> stop_validation;

void validate(LogQueue *queue, std::atomic<bool> &stop, ThreadGC *thread_gc) {
    app_thread_gc_instance = thread_gc;
    while (!stop) {
        auto *log = static_cast<LogHead *>(log_dequeue(queue));
        if (log == nullptr) {
            cpu_relax();
            continue;
        }
        validate_one(log);
    }
}

void AppThread::register_queue() {
    LogQueue *queue = &log_queue;
    validator_thread =
        Thread(validate, queue, std::ref(stop_validation), &thread_gc_instance);
}

void AppThread::unregister_queue() {
    stop_validation = true;
    validator_thread.join();
}

// scheduler.hpp

}  // namespace scee
