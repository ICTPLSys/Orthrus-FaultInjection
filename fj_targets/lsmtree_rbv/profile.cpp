#include "profile.hpp"

#include <hdr/hdr_histogram.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>

#include "utils.hpp"

namespace profile {

#ifndef ENABLE_PROFILE
#define ENABLE_PROFILE true
#endif

bool profile_enabled = false;

hdr_histogram* validation_latency_histogram;
std::atomic<uint64_t> validation_cpu_time_cycles = 0;
std::atomic<uint64_t> validation_count = 0;

void start() {
    fprintf(stderr, "Profile.cpp: ENABLE_PROFILE: %d\n", ENABLE_PROFILE);
    #if (ENABLE_PROFILE)
        profile_enabled = true;
    #endif
}

void stop() { profile_enabled = false; }

struct HistogramInitializer {
    HistogramInitializer() {
        #if (ENABLE_PROFILE)
            int err;
            err = hdr_init(1, 10'000'000, 3, &validation_latency_histogram);
            if (err != 0) {
                std::cerr
                    << "Error: failed to initialize validation latency histogram"
                    << std::endl;
                std::abort();
            }
        #else
            profile_enabled = false;
        #endif
    }
    ~HistogramInitializer() {
        #if (ENABLE_PROFILE)
            print_stats();
            hdr_close(validation_latency_histogram);
        #endif
    }
};

HistogramInitializer initializer;

uint64_t thread_us() {
    struct timespec ts;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

void record_validation_latency(uint64_t latency_cycles) {
    if (!profile_enabled) return;
    hdr_record_value_atomic(validation_latency_histogram, latency_cycles);
}

void record_validation_cpu_time(uint64_t cpu_time_cycles, uint64_t count) {
    if (!profile_enabled) return;
    validation_cpu_time_cycles += cpu_time_cycles;
    validation_count += count;
}

static std::string get_current_time() {
    time_t now = time(nullptr);
    struct tm* localTime = localtime(&now);
    char* timeStr = asctime(localTime);

    std::string result(timeStr);
    if (!result.empty() && result[result.length() - 1] == '\n') {
        result.erase(result.length() - 1);
    }
    for (char& c : result) {
        if (c == ' ') {
            c = '_';
        }
    }
    return result;
}

void print_stats() {
    if (validation_count == 0) return;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Validation CPU Time (us): "
              << validation_cpu_time_cycles / kCpuMhzNorm << std::endl;
    std::cout << "Validation Count: " << validation_count << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Validation Latency (us)" << std::endl;
    std::cout << "mean:   "
              << hdr_mean(validation_latency_histogram) / kCpuMhzNorm
              << std::endl;
    std::cout << "p50:    "
              << hdr_value_at_percentile(validation_latency_histogram, 50) /
                     kCpuMhzNorm
              << std::endl;
    std::cout << "p90:    "
              << hdr_value_at_percentile(validation_latency_histogram, 90) /
                     kCpuMhzNorm
              << std::endl;
    std::cout << "p95:    "
              << hdr_value_at_percentile(validation_latency_histogram, 95) /
                     kCpuMhzNorm
              << std::endl;
    std::cout << "p99:    "
              << hdr_value_at_percentile(validation_latency_histogram, 99) /
                     kCpuMhzNorm
              << std::endl;
    // std::string filename = "validation-latency-" + get_current_time() + ".cdf";
    std::string filename = "validation-latency-scee.cdf";
    FILE* f = fopen(filename.c_str(), "w");
    hdr_percentiles_print(validation_latency_histogram, f, 10, kCpuMhzNorm,
                          CLASSIC);
    fclose(f);
    std::cout << "CDF file: " << filename << std::endl;
    std::cout << "----------------------------------------" << std::endl;
}

}  // namespace profile
