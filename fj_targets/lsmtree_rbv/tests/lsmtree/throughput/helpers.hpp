#pragma once

#include <mutex>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <cstdio>
// #include <format>
#include <unistd.h>

#include <netinet/tcp.h>

#include "utils.hpp"

#include <functional>

#include <cerrno>

#include <hdr/hdr_histogram.h>

#ifndef ENABLE_PROFILE_MEM
#define ENABLE_PROFILE_MEM false
#endif

#ifndef ENABLE_PROFILE_VAL_CDF
#define ENABLE_PROFILE_VAL_CDF false
#endif

#ifndef ASSERT
#define ASSERT(x, fmt, ...)                                          \
    do {                                                             \
        if (!(x)) {                                                  \
            fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, \
                    ##__VA_ARGS__);                                  \
            abort();                                                 \
        }                                                            \
    } while (0)
#endif

#ifndef KDEBUG
#define KDEBUG(fmt, ...)                                                   \
    fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);
#endif

enum class Cmd {
    Set,
    Get,
    Del,
    Stop,
};

struct Msg {
    Cmd cmd;
    size_t idx;
    int64_t key;
    int64_t value;
    uint64_t timestamp;
};

class ScopeGuard {
public:
    template<typename Fn>
    ScopeGuard(Fn&& f): fn(std::move(f)) { }
    ~ScopeGuard () { fn(); }
private:
    std::function<void()> fn;
};

static inline int connect_to(const char *ip, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT(fd >= 0, "create socket error: %s", strerror(errno));

    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    inet_aton(ip, &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    int flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, &flag, sizeof(flag));

    ASSERT(
        connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) >= 0,
        "connect to server error: %s", strerror(errno));

    return fd;
}

static ssize_t read_all(int fd, void *buf, ssize_t len) {
    ssize_t bytes_read = 0;
    while (bytes_read < len) {
        ssize_t ret = read(fd, (void *)((char *)buf + bytes_read), len - bytes_read);
        if (ret < 0) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                continue;
            }
            ASSERT(0, "Read error: %s", strerror(errno));
        }
        bytes_read += ret;
    }
    return bytes_read;
}

static inline void write_all(int fd, const char *buf, size_t len) {
    size_t written = 0;
    while (written < len) {
        // std::cout << "writing: " << written << "/" << len << std::endl;
        ssize_t ret = write(fd, buf + written, len - written);
        if (ret < 0) {
            if (ret == EAGAIN || ret == EWOULDBLOCK) {
                continue;
            }
            ASSERT(0, "Write error: %s", strerror(errno));
        }
        written += ret;
    }
}

void print_status(const std::vector<uint64_t>& data) {
    std::vector<uint64_t> latency;

    int margin = data.size() * 0.1;
    for (int i = margin; i < (int)data.size() - margin; i++) {
        if (data[i] == 0) continue;
        latency.emplace_back(data[i]);
    }

    if (auto sz = latency.size(); sz > 0) {
        std::sort(latency.begin(), latency.end());
        auto avg =
            std::accumulate(latency.cbegin(), latency.cend(), 0.0) / sz / kCpuMhzNorm;
        auto p90 = latency[sz * 90 / 100] / kCpuMhzNorm;
        auto p95 = latency[sz * 95 / 100] / kCpuMhzNorm;
        auto p99 = latency[sz * 99 / 100] / kCpuMhzNorm;
        // std::cout << std::format("avg: {} us, p90: {} us, p95: {} us, p99: {} us\n", avg, p90, p95, p99);
        printf("avg: %lf us, p90: %lu us, p95: %lu us, p99: %lu us\n", avg, p90, p95, p99);
    }
}

void print_stats(
    hdr_histogram* validation_latency_histogram,
    std::string filename
) {
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
    FILE* f = fopen(filename.c_str(), "w");
    hdr_percentiles_print(validation_latency_histogram, f, 10, kCpuMhzNorm,
                          CLASSIC);
    fclose(f);
    std::cout << "CDF file: " << filename << std::endl;
    std::cout << "----------------------------------------" << std::endl;
}

void hdr_print_status(const char* filename, std::vector<uint64_t> latency){
    hdr_histogram* validation_latency_histogram = nullptr;
    int err = hdr_init(1, 10'000'000, 3, &validation_latency_histogram);
    for (auto it : latency) {
        if (it <= 1) continue;
        hdr_record_value_atomic(validation_latency_histogram, it);
    }
    print_stats(validation_latency_histogram, filename);
    return;
}
