#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/times.h>

#include <atomic>
#include <chrono>
#include <string>
#include <vector>

#include "utils.hpp"

namespace monitor {

struct _lockguard {
    pthread_mutex_t mtx;
    _lockguard() { pthread_mutex_init(&mtx, nullptr); }
    ~_lockguard() { pthread_mutex_destroy(&mtx); }
    void lock() { pthread_mutex_lock(&mtx); }
    void unlock() { pthread_mutex_unlock(&mtx); }
};

// monitor the memory used by this process
// log: the file descriptor to output summary
// step: output on stderr every #step updates
// record: returns the memory offset compared to initial time
struct process_memory {
    process_memory(FILE *log_file, int step);
    ~process_memory();
    size_t record();
    size_t read_mem_usage();
    FILE *log;
    int _step;
    std::vector<size_t> records;
    size_t offset;
};

// monitor the cpu used by this process
// log: the file descriptor to output summary
// step: output on stderr every #step updates
// record: return the average cpu usage during last update
struct process_cpu {
    process_cpu(FILE *log_file, int step);
    ~process_cpu();
    double record();
    struct timing_data {
        clock_t tim, sys, usr;
    };
    timing_data now();
    FILE *log;
    int _step;
    std::vector<timing_data> records;
};

// monitor the throughput and latency of events
// log: the file descriptor to output summary
// num_ops: total number of operations for all threads
// n_threads: number of threads executing the ops
// task: task name of the evaluation
// cnts: # of operations executed on each thread
// latency: the latency for each operation by counting with rdtsc()
// report: report on stderr for most recent throughput, with last_scnt and
// last_rdtsc value
struct evaluation {
    static constexpr int max_n_threads = 256;
    evaluation(FILE *log, uint64_t num_ops, int n_threads, std::string task);
    ~evaluation();
    FILE *log;
    uint64_t num_ops;
    int n_threads;
    std::string task;
    struct alignas(64) Cnt {
        uint64_t c;
    };
    std::vector<uint64_t> latency;
    Cnt cnts[max_n_threads];
    std::vector<std::pair<std::chrono::steady_clock::time_point, uint64_t>>
        records;
    std::vector<uint64_t> scnts;
    void report();
};

process_memory::process_memory(FILE *log_file, int step) {
    offset = read_mem_usage();
    log = log_file;
    _step = step;
}

process_memory::~process_memory() {
    // divide into 32 phases
    int n_phases = std::min((int)records.size(), 32);
    auto dump = [this](std::string prefix, int l, int r) {
        fprintf(this->log, "%s: peak %zu KB, low %zu KB, avg %zu KB\n",
                prefix.c_str(),
                *std::max_element(this->records.begin() + l,
                                  this->records.begin() + r),
                *std::min_element(this->records.begin() + l,
                                  this->records.begin() + r),
                std::accumulate(this->records.begin() + l,
                                this->records.begin() + r, size_t(0)) /
                    (r - l));
    };
    dump("Overall", 0, (int)records.size());
    for (int i = 0; i < n_phases; ++i) {
        int l = (int)records.size() * i / n_phases;
        int r = (int)records.size() * (i + 1) / n_phases;
        dump("Block " + std::to_string(i), l, r);
    }
}

size_t process_memory::record() {
    static _lockguard lock;
    lock.lock();
    size_t state = read_mem_usage() - offset / 1000;
    records.emplace_back(state);
    if (records.size() % _step == 0) {
        fprintf(stderr, "live: %zu KB after %zu updates\n", state,
                records.size());
    }
    lock.unlock();
    return state;
}

size_t process_memory::read_mem_usage() {
    static _lockguard lock;
    lock.lock();
    FILE *file = fopen("/proc/self/status", "r");
    size_t result = 0;
    char buffer[128];
    while (fgets(buffer, 128, file) != nullptr) {
        if (strncmp(buffer, "VmSize:", 7) == 0) {
            // in the form of kB
            sscanf(buffer, "VmSize: %zu", &result);
            break;
        }
    }
    fclose(file);
    lock.unlock();
    return result;
}

process_cpu::timing_data process_cpu::now() {
    struct tms timeSample;
    timing_data res;
    res.tim = times(&timeSample);
    res.sys = timeSample.tms_stime;
    res.usr = timeSample.tms_utime;
    return res;
}

process_cpu::process_cpu(FILE *log_file, int step) {
    records.emplace_back(now());
    log = log_file;
    _step = step;
}

process_cpu::~process_cpu() {
    // divide into 32 phases
    int n_phases = std::min((int)records.size() - 1, 32);
    auto dump = [this](std::string prefix, int l, int r) {
        auto rr = this->records[r];
        auto ll = this->records[l];
        fprintf(
            this->log, "%s: %.2f seconds, %.3f cpu cores (%.2f%% kernel)\n",
            prefix.c_str(),
            (double)(rr.tim - ll.tim) * 0.01,  // 100 clock ticks per second
            (double)(rr.usr - ll.usr + rr.sys - ll.sys) / (rr.tim - ll.tim),
            100.f * (rr.sys - ll.sys) / (rr.usr - ll.usr + rr.sys - ll.sys));
    };
    dump("Overall", 0, (int)records.size() - 1);
    for (int i = 0; i < n_phases; ++i) {
        int l = ((int)records.size() - 1) * i / n_phases;
        int r = ((int)records.size() - 1) * (i + 1) / n_phases;
        dump("Block " + std::to_string(i), l, r);
    }
}

double process_cpu::record() {
    static _lockguard lock;
    auto tnow = now();
    lock.lock();
    double tnum =
        (tnow.sys - records.back().sys) + (tnow.usr - records.back().usr);
    double tden = (tnow.tim - records.back().tim);
    double tans = tden > 0 ? tnum / tden : -1;
    records.emplace_back(tnow);
    if (records.size() % _step == 0) {
        fprintf(stderr, "live: cpu usage = %.3f at %zu updates\n", tans,
                records.size());
    }
    lock.unlock();
    return tans;
}

evaluation::evaluation(FILE *log, uint64_t num_ops, int n_threads,
                       std::string task)
    : log(log), num_ops(num_ops), n_threads(n_threads), task(task) {
    latency.resize(num_ops);
    records.emplace_back(std::chrono::steady_clock::now(), 0);
    for (int i = 0; i < max_n_threads; ++i) cnts[i].c = 0;
}

evaluation::~evaluation() {
    uint64_t n_phases = std::min(num_ops, 8LU);
    uint64_t l = num_ops / n_phases, r = num_ops * (n_phases - 1) / n_phases;
    std::sort(latency.begin() + l, latency.begin() + r);
    uint64_t p90 = nanosecond(0, latency[l + uint64_t((r - l) * .9)]);
    uint64_t p95 = nanosecond(0, latency[l + uint64_t((r - l) * .95)]);
    uint64_t p99 = nanosecond(0, latency[l + uint64_t((r - l) * .99)]);
    uint64_t avg = nanosecond(0, std::accumulate(latency.begin() + l,
                                                 latency.begin() + r, 0ULL)) /
                   (r - l);
    auto period = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - records[0].first);
    fprintf(stderr, "Finished task %s. Time: %ld us; Throughput: %f/s.\n",
            task.c_str(), period.count(), num_ops * 1e6 / period.count());
    l = ((uint64_t)records.size() - 1) / n_phases,
    r = ((uint64_t)records.size() - 1) * (n_phases - 1) / n_phases;
    period = std::chrono::duration_cast<std::chrono::microseconds>(
        records[r + 1].first - records[l].first);
    uint64_t put = (records[r + 1].second - records[l].second) * 1000000LL /
                   period.count();
    fprintf(stderr, "Estimated (operation) throughput: %lu/s\n", put);
    fprintf(log, "%s put %lu avg %lu p90 %lu p95 %lu p99 %lu\n", task.c_str(),
            put, avg, p90, p95, p99);
}

void evaluation::report() {
    static _lockguard lock;
    lock.lock();
    uint64_t cnt = 0;
    for (int i = 0; i < n_threads; ++i) cnt += cnts[i].c;
    if (cnt > records.back().second + 16384) {  // minimum print interval
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                            now - records.back().first)
                            .count();
        fprintf(stderr, "Instant throughput: %f/s\n",
                (cnt - records.back().second) * 1e6 / duration);
    }
    records.emplace_back(std::chrono::steady_clock::now(), cnt);
    lock.unlock();
}

}  // namespace monitor
