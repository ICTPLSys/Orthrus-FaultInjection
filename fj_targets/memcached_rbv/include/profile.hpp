#pragma once

#include <cstdint>

namespace profile {
uint64_t thread_us();
long long get_us_abs(); // absolute time, in microseconds
void start(const char* cdf_filename="validation-latency-scee.cdf");
void stop();
void record_validation_latency(uint64_t latency_us);
void record_validation_cpu_time(uint64_t cpu_time_cycles,
                                uint64_t validation_count);
void print_stats();
}  // namespace profile
