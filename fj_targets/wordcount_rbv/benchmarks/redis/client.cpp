#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <random>
#include <thread>
#include <vector>

#include "common.hpp"
#include "monitor.hpp"
#include "utils.hpp"

// client reside on any server that is not shared with others
constexpr int kNumClientCores = 16;
// const int kLowestCore = 4;

static std::string ip, output_file;
static uint32_t port, nsets, ngets, nclients, rps;

static inline void write_all(int fd, const char *buf, size_t len) {
    size_t written = 0;
    while (written < len) {
        ssize_t ret = write(fd, buf + written, len - written);
        assert(ret > 0);
        written += ret;
    }
}

class MemcpyMonad {
public:
    MemcpyMonad(void *dst) : dst_(reinterpret_cast<char *>(dst)), offset_(0) {}
    MemcpyMonad &Copy(const void *src, size_t len) {
        memcpy(dst_ + offset_, src, len);
        offset_ += len;
        return *this;
    }
    size_t Offset() { return offset_; }

private:
    char *dst_;
    size_t offset_;
};

static inline size_t prepare_setcmd(char *dst, const char *key,
                                    const char *val) {
    MemcpyMonad m(dst);
    m.Copy("set ", strlen("set "))
        .Copy(key, KEY_LEN)
        .Copy(" ", 1)
        .Copy(val, VAL_LEN)
        .Copy(kCrlf, strlen(kCrlf));
    return m.Offset();
}

static inline size_t prepare_getcmd(char *dst, const char *key) {
    MemcpyMonad m(dst);
    m.Copy("get ", strlen("get "))
        .Copy(key, KEY_LEN)
        .Copy(kCrlf, strlen(kCrlf));
    return m.Offset();
}

static inline size_t prepare_delcmd(char *dst, const char *key) {
    MemcpyMonad m(dst);
    m.Copy("del ", strlen("del "))
        .Copy(key, KEY_LEN)
        .Copy(kCrlf, strlen(kCrlf));
    return m.Offset();
}

static inline int parse_getret(char *rx_buf, size_t rx_len, char *value,
                               size_t value_buf_len, size_t *value_len) {
    size_t prefix_len = strlen(kRetVals[kValue]);
    if (strncmp(rx_buf, kRetVals[kValue], prefix_len) != 0) {
        return -1;
    }
    if (rx_buf[rx_len - 2] != '\r' || rx_buf[rx_len - 1] != '\n') {
        return -1;
    }
    const char *p = rx_buf + prefix_len;
    size_t vlen = rx_len - prefix_len - 2;
    assert(vlen <= value_buf_len);
    memcpy(value, p, vlen);
    *value_len = vlen;
    return 0;
}

inline std::chrono::steady_clock::time_point microtime(void) {
    auto start = std::chrono::steady_clock::now();
    return start;
}

inline uint64_t microtime_diff(
    const std::chrono::steady_clock::time_point &start,
    const std::chrono::steady_clock::time_point &end) {
    std::chrono::microseconds diff =
        std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return diff.count();
}

inline void random_string(char *data, uint32_t len, std::mt19937 *rng) {
    std::uniform_int_distribution<int> distribution('a', 'z' + 1);
    for (uint32_t i = 0; i < len; i++) {
        data[i] = char(distribution(*rng));
    }
}

const double kZipfParamS = 0.99;
const uint32_t kNumPrints = 32;
const uint32_t kMaxNumThreads = 32;
const int kBufferSize = 1024;

FILE *logger;
uint32_t kNumThreads, kNumOpsPerThread;

struct Key {
    char data[KEY_LEN];
};
struct Value {
    char data[VAL_LEN];
};

std::unique_ptr<std::mt19937> rngs[kMaxNumThreads];
std::vector<Key> all_keys;
std::vector<Value> all_vals;
std::vector<uint32_t> zipf_key_indices;
uint32_t kNumKVPairs;

void init_array() {
    kNumThreads = nclients;
    kNumOpsPerThread = ngets;
    kNumKVPairs = nsets;
    all_keys.resize(kNumKVPairs);
    all_vals.resize(kNumKVPairs);
    zipf_key_indices.resize(kNumOpsPerThread * kNumThreads);
}

void init_rng() {
    for (uint32_t i = 0; i < kMaxNumThreads; i++) {
        rngs[i] = std::make_unique<std::mt19937>((i + 1) * port);
        for (uint32_t t = 0; t < 10000; ++t) (*rngs[i])();
    }
}

int connect_server() {
    struct sockaddr_in server_addr;
    int fd;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf(" create socket error!\n ");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_aton(ip.c_str(), &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    if (connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        assert(false);
    }

    return fd;
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
            for (uint32_t k = start_idx; k < end_idx; ++k) {
                random_string(all_keys[k].data, KEY_LEN, rngs[i].get());
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
        // bind_core(threads[i].native_handle(), i + kLowestCore);
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

template <RetType ret_type>
void run_set() {
    fprintf(stderr, "SET (nthreads=%d) start running...\n", kNumThreads);
    std::vector<std::thread> threads;
    static bool is_init = true;
    monitor::evaluation monitor(logger, kNumKVPairs, kNumThreads,
                                is_init ? "SET" : "UPDATE");
    is_init = false;
    for (uint32_t i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([i, &monitor]() {
            int fd = connect_server();
            assert(fd >= 0);
            std::vector<char> tx_buf(kBufferSize);
            std::vector<char> rx_buf(kBufferSize);
            for (uint32_t k = i; k < kNumKVPairs; k += kNumThreads) {
                auto &key = all_keys[k];
                auto &val = all_vals[k];
                random_string(val.data, VAL_LEN, rngs[i].get());
                size_t len = prepare_setcmd(tx_buf.data(), key.data, val.data);

                uint64_t timestamp = rdtsc();
                write_all(fd, tx_buf.data(), len);
                size_t rx_len = read(fd, rx_buf.data(), kBufferSize);
                monitor.latency[k] = rdtsc() - timestamp;

                assert(rx_len > 0);
                if (strncmp(rx_buf.data(), kRetVals[ret_type],
                            sizeof(kRetVals[ret_type]) - 1) != 0) {
                    printf("Set error: key %s\n",
                           std::string(key.data, KEY_LEN).c_str());
                }
                monitor.cnts[i].c++;

                uint64_t completed = (uint64_t)(k + kNumThreads) * kNumPrints;
                if (completed % kNumKVPairs < kNumThreads * kNumPrints) {
                    completed /= kNumKVPairs;
                    if (completed % kNumThreads == i) {
                        monitor.report();
                    }
                }
            }
        });
        // bind_core(threads[i].native_handle(), i + kLowestCore);
    }
    for (uint32_t i = 0; i < kNumThreads; ++i) {
        threads[i].join();
    }
    threads.clear();
}

void run_get() {
    prepare_zipf_index();
    fprintf(stderr, "GET (nthreads=%d) start running...\n", kNumThreads);
    std::vector<std::thread> threads;
    monitor::evaluation monitor(logger, kNumOpsPerThread * kNumThreads,
                                kNumThreads, "GET");
    for (uint32_t i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([i, &monitor]() {
            int fd = connect_server();
            assert(fd >= 0);
            Value val;
            std::vector<char> tx_buf(kBufferSize);
            std::vector<char> rx_buf(kBufferSize);
            for (uint32_t k = 0; k < kNumOpsPerThread; ++k) {
                auto &key = all_keys[zipf_key_indices[k * kNumThreads + i]];
                auto &val = all_vals[zipf_key_indices[k * kNumThreads + i]];
                char buf[VAL_LEN];
                size_t val_len;
                size_t len = prepare_getcmd(tx_buf.data(), key.data);
                uint64_t timestamp = rdtsc();
                write_all(fd, tx_buf.data(), len);
                size_t rx_len = read(fd, rx_buf.data(), kBufferSize);
                monitor.latency[k * kNumThreads + i] = rdtsc() - timestamp;
                assert(rx_len > 0);
                int r =
                    parse_getret(rx_buf.data(), rx_len, buf, VAL_LEN, &val_len);
                if (r != 0) {
                    printf("Get error: key %s\n",
                           std::string(key.data, KEY_LEN).c_str());
                } else {
                    assert(val_len == VAL_LEN);
                    if (memcmp(val.data, buf, VAL_LEN)) {
                        printf("Get error: key %s, val %s %s\n",
                               std::string(key.data, KEY_LEN).c_str(),
                               std::string(val.data, VAL_LEN).c_str(),
                               std::string(buf, VAL_LEN).c_str());
                        assert(false);
                    }
                }
                monitor.cnts[i].c++;

                uint64_t completed = (uint64_t)(k + 1) * kNumPrints;
                if (completed % kNumOpsPerThread < kNumPrints) {
                    completed /= kNumOpsPerThread;
                    if (completed % kNumThreads == i) {
                        monitor.report();
                    }
                }
            }
        });
        // bind_core(threads[i].native_handle(), i + kLowestCore);
    }
    for (uint32_t i = 0; i < kNumThreads; ++i) {
        threads[i].join();
    }
    threads.clear();
}

int main(int argc, char **argv) {
    if (argc >= 9 || argc <= 1) {
        fprintf(stderr,
                "Usage: %s <ip> <port> <log_file> <nclients> <nsets> <ngets> "
                "<rps>\n",
                argv[0]);
        fprintf(stderr,
                "Default values: ip=127.0.0.1, port=6379, "
                "log_file=build/client.log, nclients=16, nsets=1<<22, "
                "ngets=1<<20, rps=0\n");
        return 1;
    }
    ip = argc >= 2 ? argv[1] : "127.0.0.1";
    port = argc >= 3 ? atoi(argv[2]) : 6379;
    output_file = argc >= 4 ? argv[3] : "build/client.log";
    nclients = argc >= 5 ? atoi(argv[4]) : 16;
    nsets = argc >= 6 ? 1 << atoi(argv[5]) : 1 << 22;
    ngets = argc >= 7 ? 1 << atoi(argv[6]) : 1 << 20;
    rps = argc >= 8 ? atoi(argv[7]) : 0;
    logger = fopen(output_file.c_str(), "w");
    init_array();
    init_rng();
    prepare_key();
    run_set<kCreated>();  // set
    run_set<kStored>();   // update
    run_get();            // get
    fclose(logger);
    return 0;
}
