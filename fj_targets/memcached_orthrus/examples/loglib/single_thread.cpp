#include <x86intrin.h>

#include <cassert>
#include <cstdio>

#include "scee.hpp"

int add(int a, int b) { return a + b; }

void benchmark(size_t n, bool print) {
    uint64_t run_start = __rdtsc();
    for (size_t i = 0; i < n; ++i) {
        // mutator run
        auto x = scee::run(add, 1, (int)i);
        // validation
        scee::LogHead *log =
            static_cast<scee::LogHead *>(scee::log_dequeue(&scee::log_queue));
        assert(log != nullptr);
        scee::validate_one(log);
    }
    uint64_t run_end = __rdtsc();
    if (print) {
        printf("Run time: %lu\n", (run_end - run_start) / n);
    }
}

void baseline(size_t n, bool print) {
    uint64_t run_start = __rdtsc();
    for (size_t i = 0; i < n; ++i) {
        volatile int x = add(1, (int)i);
    }
    uint64_t run_end = __rdtsc();
    if (print) {
        printf("Baseline time: %lu\n", (run_end - run_start) / n);
    }
}

int main() {
    benchmark(1000000, true);
    baseline(1000000, true);
    return 0;
}