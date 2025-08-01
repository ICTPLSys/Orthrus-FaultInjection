#include <sys/mman.h>

#include <boost/sort/parallel_stable_sort/parallel_stable_sort.hpp>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <vector>

#include "context.hpp"
#include "control.hpp"
#include "ctltypes.hpp"
#include "custom_stl.hpp"
#include "log.hpp"
#include "monitor.hpp"
#include "namespace.hpp"
#include "ptr.hpp"
#include "scee.hpp"
#include "thread.hpp"
#include "utils.hpp"

#ifdef PROFILE_IT
#include "profile.hpp"
#endif

namespace raw {
#include "closure.hpp"
}  // namespace raw
namespace app {
#include "closure.hpp"
}  // namespace app
namespace validator {
#include "closure.hpp"
}  // namespace validator

using namespace raw;

Value uint64_to_value(uint64_t val) {
    constexpr uint64_t primes[] = {
        2,   3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,  41,  43,
        47,  53,  59,  61,  67,  71,  73,  79,  83,  89,  97,  101, 103, 107,
        109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181,
        191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263,
        269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349,
        353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433,
        439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499};
    assert(VAL_LEN <= sizeof(primes) / sizeof(primes[0]));
    Value v;
    for (size_t i = 0; i < VAL_LEN; ++i) {
        v.ch[i] = 'a' + (val % primes[i]) % 26;
    }
    return v;
}

enum RunType { Baseline, SCEE };

void inline log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    time_t t = time(nullptr);
    char tmp[64] = {0};
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&t));
    printf("%s ", tmp);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

inline void multi_thread_memcpy(void *dst, const void *src, size_t size,
                                uint64_t n_threads) {
    std::vector<std::thread> threads;
    size_t stride = (size + n_threads - 1) / n_threads;
    for (uint64_t t = 0; t < n_threads; ++t) {
        size_t offset = t * stride;
        void *tdst = (char *)dst + offset;
        void *tsrc = (char *)src + offset;
        size_t tsize = std::min(stride, size - offset);
        threads.emplace_back(
            [tdst, tsrc, stride]() { memcpy(tdst, tsrc, stride); });
    }
    for (auto &t : threads) {
        t.join();
    }
}

template <typename F>
inline void parallel_for(size_t n_threads, size_t n, F &&f) {
    std::vector<std::thread> threads;
    for (uint64_t t = 0; t < n_threads; ++t) {
        threads.emplace_back([t, n_threads, n, &f]() {
            size_t stride = (n + n_threads - 1) / n_threads;
            size_t start = t * stride;
            size_t end = std::min(start + stride, n);
            for (size_t i = start; i < end; ++i) {
                f(i);
            }
        });
    }
    for (auto &t : threads) t.join();
}

constexpr uint64_t kLoaderThreads = 24;

template <typename T>
void load_from_binary(T *data, size_t size, const std::string &filename) {
    FILE *fp = fopen(filename.c_str(), "r");
    assert(fp != nullptr);
    fseek(fp, 0L, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0L, 0);
    assert(file_size == sizeof(T) * size);

    void *mmap_ptr =
        mmap(nullptr, file_size, PROT_READ, MAP_SHARED, fp->_fileno, 0);
    assert(mmap_ptr != MAP_FAILED);
    multi_thread_memcpy(data, mmap_ptr, sizeof(T) * size, kLoaderThreads);
    fclose(fp);
}

struct KeyGenerator {
    KeyGenerator(std::vector<uint64_t> &keys_to_insert)
        : keys(keys_to_insert), index(0) {}
    uint64_t operator()() {
        int idx = index.fetch_add(1, std::memory_order_relaxed);
        assert((size_t)idx < keys.size() && "Too many inserts");
        return keys[idx];
    }
    std::vector<uint64_t> &keys;
    std::atomic_int index;
};

template <RunType RT, typename ThreadType>
int main_fn(int argc, char *argv[]) {
    if (argc < 3 || argc > 7) {
        fprintf(stderr,
                "Usage: %s <baseline|scee> <data_file> [n_threads] "
                "[rps] [record_count] [operation_count]\n",
                argv[0]);
        fprintf(stderr, "data_file: path to the data file\n");
        fprintf(stderr, "n_threads: number of threads to run, default: 4\n");
        fprintf(
            stderr,
            "rps: maximum number of records per second to load, default: 1M\n");
        fprintf(stderr,
                "record_count: number of records to load, default: 190M\n");
        fprintf(stderr,
                "operation_count: number of operations to run, default: 20M\n");
        return 1;
    }

    scee::ptr_t<Node> *root = build_tree();
    uint64_t n_threads = argc > 3 ? atoi(argv[3]) : 4;
    uint64_t rps = argc > 4 ? atoi(argv[4]) : 20000000;
    uint64_t record_count = argc > 5 ? atoi(argv[5]) : 190000000;
    uint64_t operation_count = argc > 6 ? atoi(argv[6]) : 20000000;

    fprintf(stderr, "hyperparameter parsed, start loading data\n");

    std::vector<uint64_t> keys(record_count);
    load_from_binary<uint64_t>(keys.data(), keys.size(), argv[2]);
    fprintf(stderr, "keys loaded\n");

    std::mt19937 rng(233333);
    keys.push_back(0);
    boost::sort::parallel_stable_sort(keys.begin(), keys.end(), kLoaderThreads);
    fprintf(stderr, "keys sorted\n");
    std::vector<Value> vals(keys.size());
    parallel_for(kLoaderThreads, keys.size(),
                 [&](size_t i) { vals[i] = uint64_to_value(rng()); });
    fprintf(stderr, "values generated\n");

    uint64_t sizes = keys.size();
    fprintf(stderr, "load completed, %zu K-V pairs\n", sizes - 1);

    root = build_tree_from_keys(keys, vals);

    fprintf(stderr, "tree built recursively\n");
    std::vector<ThreadType> threads;

    constexpr int p_all = 100, p_insert = 0, p_update = 50, p_read = 0,
                  p_scan = 50;
    // constexpr int p_all = 100, p_insert = 0, p_update = 100, p_read = 0,
    // p_scan = 0;
    constexpr int scan_max = 450, scan_min = 50;
    zipf_table_distribution<> zipf(keys.size(), 0.99);
    uint64_t kmin = ~0ULL, kmax = 0;
    for (uint64_t i = 0; i < keys.size(); ++i) {
        kmin = std::min(kmin, keys[i]);
        kmax = std::max(kmax, keys[i]);
    }
    std::vector<uint64_t> key_in(operation_count), key_out(operation_count),
        cnts(operation_count);
    std::vector<Value> val_out(operation_count);
    std::vector<int> ops(operation_count);
    for (uint64_t i = 0; i < operation_count; ++i) {
        key_in[i] = keys[zipf(rng)];
        key_out[i] = ((rng() << 32) ^ rng()) % (kmax - kmin + 1) + kmin;
        val_out[i] = uint64_to_value(rng());
        ops[i] = rng() % p_all;
        cnts[i] = rng() % (scan_max - scan_min + 1) + scan_min;
    }

    fprintf(stderr, "workload starts\n");

#ifdef PROFILE_IT
    profile::start();
#endif

    uint64_t workload_start = rdtsc();
    FILE *logger = fopen(
        ("build/masstree-orthrus-p95-" + std::to_string(rps / 1000) + ".txt")
            .c_str(),
        "w");

    {
        constexpr uint64_t WorkloadPrints = 20;
        monitor::evaluation eva(logger, operation_count, n_threads,
                                "MassTree-Workload");
        monitor::process_memory mem(logger, n_threads * 2);
        std::atomic_uint64_t finished = 0;
        for (uint64_t t = 0; t < n_threads; ++t) {
            threads.emplace_back([t, root, rps, operation_count, n_threads,
                                  &key_in, &key_out, &val_out, &ops, &eva,
                                  &finished, &cnts]() {
                std::exponential_distribution<double> sampler(rps / n_threads /
                                                              1e9);
                std::mt19937 rng(1235467 + t);
                rng();
                rng();
                rng();
                rng();
                rng();
                const uint64_t BNS = 1e6;
                uint64_t t_start = rdtsc();
                double t_dur = 0;
                for (uint64_t i = t; i < operation_count; i += n_threads) {
                    t_dur += sampler(rng);
                    uint64_t p = rdtsc(), t_offset = 0;
                    uint64_t t_now = nanosecond(t_start, p);
                    if (t_now + BNS < t_dur) {
                        my_nsleep(t_dur - t_now - (BNS / 2));
                    } else if (t_dur + BNS < t_now) {
                        t_offset = t_now - t_dur - (BNS / 2);
                    }
                    int op = ops[i];
                    op = 1LL * i * p_all / operation_count;
                    if (op < p_insert) {
                        uint8_t ret;
                        if constexpr (RT == Baseline) {
                            ret = insert(root, key_out[i], val_out[i]);
                        } else {
                            using InsertType = uint8_t (*)(scee::ptr_t<Node> *,
                                                           uint64_t, Value);
                            auto app_fn =
                                reinterpret_cast<InsertType>(app::insert);
                            auto val_fn =
                                reinterpret_cast<InsertType>(validator::insert);
                            ret = scee::run2(app_fn, val_fn, root, key_out[i],
                                             val_out[i]);
                        }
                        assert(ret == 1);
                    } else if (op < p_insert + p_read) {
                        const Value *ret;
                        if constexpr (RT == RunType::Baseline) {
                            ret = read(root, key_in[i]);
                        } else {
                            using ReadType =
                                const Value *(*)(scee::ptr_t<Node> *, uint64_t);
                            auto app_fn = reinterpret_cast<ReadType>(app::read);
                            auto val_fn =
                                reinterpret_cast<ReadType>(validator::read);
                            ret = scee::run2(app_fn, val_fn, root, key_in[i]);
                        }
                        assert(ret != nullptr);
                    } else if (op < p_insert + p_read + p_update) {
                        const Value *ret;
                        if constexpr (RT == Baseline) {
                            ret = update(root, key_in[i], val_out[i]);
                        } else {
                            using UpdateType =
                                const Value *(*)(scee::ptr_t<Node> *, uint64_t,
                                                 Value);
                            auto app_fn =
                                reinterpret_cast<UpdateType>(app::update);
                            auto val_fn =
                                reinterpret_cast<UpdateType>(validator::update);
                            ret = scee::run2(app_fn, val_fn, root, key_in[i],
                                             val_out[i]);
                        }
                        assert(ret != nullptr);
                        // ret should be reclaimed here
                    } else if (op < p_insert + p_read + p_update + p_scan) {
                        uint64_t ret;
                        if constexpr (RT == Baseline) {
                            ret = scan_and_sum(root, key_in[i], cnts[i]);
                        } else {
                            using ScanType = uint64_t (*)(scee::ptr_t<Node> *,
                                                          uint64_t, uint64_t);
                            auto app_fn =
                                reinterpret_cast<ScanType>(app::scan_and_sum);
                            auto val_fn = reinterpret_cast<ScanType>(
                                validator::scan_and_sum);
                            ret = scee::run2(app_fn, val_fn, root, key_in[i],
                                             cnts[i]);
                        }
                        if (ret == 0x23146789) {
                            fprintf(stderr, "amazing\n");
                        }
                    }
                    eva.cnts[t].c++;
                    eva.latency[i] = nanosecond(p, rdtsc()) + t_offset;
                }
                finished += 1;
            });
        }
        threads.emplace_back([n_threads, &finished, &eva, &mem]() {
            while (finished.load(std::memory_order_relaxed) < n_threads) {
                if (!finished.load(std::memory_order_relaxed)) {
                    eva.report();
                    mem.record();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        });
        for (auto &t : threads) t.join();
        threads.clear();
    }

    fclose(logger);

#ifdef PROFILE_IT
    profile::stop();
#endif

    uint64_t workload_end = rdtsc();
    uint64_t workload_time = microsecond(workload_start, workload_end);
    fprintf(stderr, "workload finished in %zu us, throughput: %.2f Mops/s\n",
            workload_time, operation_count * 1.0 / workload_time);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "scee") == 0) {
        scee::main_thread(main_fn<RunType::SCEE, scee::AppThread>, argc, argv);
    } else {
        scee::main_thread(main_fn<RunType::Baseline, std::thread>, argc, argv);
    }
    return 0;
}
