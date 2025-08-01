#pragma once

#include <cstdlib>
#include <cstdio>
#include <chrono>
#include <vector>
#include <thread>

#include "utils.hpp"

int64_t total_ops;

inline std::chrono::steady_clock::time_point microtime(void) {
    auto start = std::chrono::steady_clock::now();
    return start;
}

inline uint64_t microtime_diff(
    const std::chrono::steady_clock::time_point& start,
    const std::chrono::steady_clock::time_point& end) {
    std::chrono::microseconds diff =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return diff.count();
}

const double kZipfParamS = 0.9;
const uint32_t kNumPrints = 32;
const uint32_t kMaxNumThreads = 32;
const uint32_t kMaxPrintIntervalUs = 1000000;
const uint32_t kMaxNumOps = 1 << 25;

uint32_t kNumThreads, kNumOpsPerThread;

std::unique_ptr<std::mt19937> rngs[kMaxNumThreads];
std::vector<int64_t> all_keys;
std::vector<int64_t> all_vals;
std::vector<uint32_t> zipf_key_indices;
uint32_t kNumKVPairs;

void prepare_zipf_index() {
    fprintf(stderr, "Generate zipfian indices...\n");
    zipf_table_distribution<> zipf(kNumKVPairs, kZipfParamS);
    std::vector<std::thread> threads;
    auto start = microtime();
    for (uint32_t i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([i, &zipf]() {
            for (uint32_t k = 0; k < kNumOpsPerThread; ++k) {
                zipf_key_indices[k * kNumThreads + i] = zipf(*rngs[i]);
            }
        });
    }
    for (uint32_t i = 0; i < kNumThreads; ++i) {
        threads[i].join();
    }
    threads.clear();
    auto end = microtime();
    fprintf(
        stderr,
        "Generate %ld zipf key indices, time: %ld us, avg throughput: %f/s\n",
        (uint64_t)kNumOpsPerThread * kNumThreads, microtime_diff(start, end),
        (uint64_t)kNumOpsPerThread * kNumThreads * 1000000.0 /
            microtime_diff(start, end));
}

void prepare_key() {
    fprintf(stderr, "Prepare keys...\n");
    std::vector<std::thread> threads;
    auto start = microtime();
    for (uint32_t i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([i]() {
            uint32_t start_idx = i * kNumKVPairs / kNumThreads;
            uint32_t end_idx =
                std::min((i + 1) * kNumKVPairs / kNumThreads, kNumKVPairs);

            std::uniform_int_distribution<int> distribution(0, total_ops);
            for (uint32_t k = start_idx; k < end_idx; ++k) {
                all_keys[k] = distribution(*rngs[i].get());
                all_vals[k] = distribution(*rngs[i].get());
            }
        });
        // bind_core(threads[i].native_handle(), i + kLowestCore);
    }
    for (uint32_t i = 0; i < kNumThreads; ++i) {
        threads[i].join();
    }
    threads.clear();
    auto end = microtime();
    fprintf(stderr, "Prepare %d kv pairs, time: %ld us, avg throughput: %f/s\n",
            kNumKVPairs, microtime_diff(start, end),
            kNumKVPairs * 1000000.0 / microtime_diff(start, end));
}

void init_array() {
    kNumThreads = 1;
    kNumOpsPerThread = total_ops;  // ngets
    kNumKVPairs = total_ops;       // nsets
    all_keys.resize(kNumKVPairs);
    all_vals.resize(kNumKVPairs);
    zipf_key_indices.resize(kNumOpsPerThread * kNumThreads);
}

void init_rng() {
    srand(0);
    for (uint32_t i = 0; i < kMaxNumThreads; i++) {
        rngs[i] = std::make_unique<std::mt19937>(i);
    }
}

void init_zipf() {
    init_array();
    init_rng();
    prepare_key();
    prepare_zipf_index();
}