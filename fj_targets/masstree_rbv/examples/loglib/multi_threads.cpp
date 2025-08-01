#include <x86intrin.h>

#include "scee.hpp"
#include "thread.hpp"

int add(int a, int b) {
    for (size_t i = 0; i < 1000; i++) {
        asm volatile("nop");
    }
    return a + b;
}

void benchmark(size_t n, bool print) {
    uint64_t run_start = __rdtsc();
    for (size_t i = 0; i < n; ++i) {
        volatile int x = scee::run(add, 1, (int)i);
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

void multi_thread_benchmark(size_t n, size_t nr_threads) {
    std::vector<scee::AppThread> threads;
    for (size_t i = 0; i < nr_threads; ++i) {
        threads.emplace_back([n, print = (i == 0)] { benchmark(n, print); });
    }
    for (auto &t : threads) {
        t.join();
    }
}

int main_fn() {
    baseline(1000000, false);
    baseline(1000000, true);
    benchmark(1000000, true);
    for (size_t nr_threads = 1; nr_threads <= 12; ++nr_threads) {
        printf("%lu threads\t", nr_threads);
        multi_thread_benchmark(1000000, nr_threads);
    }
    return 0;
}

int main() { return scee::main_thread(main_fn); }