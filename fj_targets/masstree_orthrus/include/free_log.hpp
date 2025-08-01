#pragma once

#include <x86intrin.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "compiler.hpp"
#include "memmgr.hpp"
#include "spin_lock.hpp"
#include "utils.hpp"

namespace scee {

struct FreeLogEntry {
    void *ptr;
    uint64_t timestamp;
};

struct FreeLog;
void thread_gc(FreeLog *log);

constexpr uint64_t GC_TICK_CYCLES = 0x100000;

inline uint64_t current_gc_tsc() { return _rdtsc() / GC_TICK_CYCLES; }

struct alignas(CACHELINE_SIZE) FreeLog {
    // how many object should be pending for validation and free in one thread
    static constexpr size_t MAX_SIZE = 1024;

    FreeLogEntry entries[MAX_SIZE];
    size_t front;
    size_t back;

    FreeLog() {
        front = 0;
        back = 0;
    }

    void push(void *ptr) {
    retry:
        entries[back] = {ptr, current_gc_tsc()};
        back = (back + 1) % MAX_SIZE;
        if (unlikely(full())) {
            thread_gc(this);
            goto retry;
        }
    }

    void pop() { front = (front + 1) % MAX_SIZE; }

    const FreeLogEntry *peek() const { return entries + front; }

    bool empty() const { return front == back; }

    bool full() const { return (back + 1) % MAX_SIZE == front; }

    size_t size() const { return (back - front + MAX_SIZE) % MAX_SIZE; }
};

struct ClosureStartLog {
    static constexpr size_t MAX_SIZE = 256;
    static constexpr size_t MAX_TSC_INTERVAL = GC_TICK_CYCLES * MAX_SIZE;

    std::atomic_size_t closure_count[MAX_SIZE];
    SpinLock spin_lock;
    uint64_t earliest_tsc;

    ClosureStartLog() {
        for (auto &i : closure_count) {
            i = 0;
        }
        earliest_tsc = 0;
    }

    // return the tsc of the closure start
    uint64_t new_closure() {
        uint64_t tsc = current_gc_tsc();
        if (unlikely(tsc >= earliest_tsc + MAX_TSC_INTERVAL)) {
            poll_earliest_tsc();
            if (unlikely(tsc >= earliest_tsc + MAX_TSC_INTERVAL)) {
                // still can not allocate a new slot
                fprintf(stderr, "Error: closure start log is full\n");
                std::abort();
            }
        }
        closure_count[tsc % MAX_SIZE].fetch_add(1, std::memory_order_relaxed);
        return tsc;
    }

    void validated_closure(uint64_t tsc, FreeLog *log);

    uint64_t poll_earliest_tsc_impl() {
        // uint64_t etsc;
        if (unlikely(earliest_tsc == 0)) {
            return current_gc_tsc();
        }
        uint64_t current_tsc = current_gc_tsc();
        for (uint64_t tsc = earliest_tsc; tsc < current_tsc; tsc++) {
            auto &count = closure_count[tsc % MAX_SIZE];
            if (count.load(std::memory_order_relaxed) != 0) {
                return tsc;
            }
        }
        return earliest_tsc;
    }

    uint64_t poll_earliest_tsc() {
        if (!spin_lock.TryLock()) {
            return earliest_tsc;
        }
        uint64_t tsc = poll_earliest_tsc_impl();
        earliest_tsc = tsc;
        spin_lock.Unlock();
        return tsc;
    }
};  // namespace scee

struct ThreadGC {
    FreeLog free_log;
    SpinLock spin_lock;
};

extern thread_local ThreadGC thread_gc_instance;
extern thread_local ThreadGC *app_thread_gc_instance;
extern ClosureStartLog closure_start_log;

inline void ClosureStartLog::validated_closure(uint64_t tsc, FreeLog *log) {
    closure_count[tsc % MAX_SIZE].fetch_sub(1, std::memory_order_relaxed);
    if (tsc == earliest_tsc || app_thread_gc_instance->free_log.size() > 64) {
        thread_gc(log);
    }
}

inline void thread_gc(FreeLog *log) {
    auto *gc_instance = app_thread_gc_instance;
    if (gc_instance == nullptr) {
        gc_instance = &thread_gc_instance;
    }
    if (!gc_instance->spin_lock.TryLock()) return;
    uint64_t earliest_tsc = closure_start_log.poll_earliest_tsc();
    while (!log->empty()) {
        const auto *entry = log->peek();
        if (entry->timestamp < earliest_tsc) {
            free_immutable(entry->ptr);
            log->pop();
        } else {
            break;
        }
    }
    gc_instance->spin_lock.Unlock();
}

}  // namespace scee