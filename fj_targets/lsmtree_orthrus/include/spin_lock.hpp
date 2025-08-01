#pragma once

#include <atomic>

class SpinLock {
public:
    SpinLock() : lock_(false) {}
    void Lock() {
        while (lock_.test_and_set(std::memory_order_acquire)) {
        }
    }
    bool TryLock() { return !lock_.test_and_set(std::memory_order_acquire); }
    void Unlock() { lock_.clear(std::memory_order_release); }

private:
    std::atomic_flag lock_;
};
