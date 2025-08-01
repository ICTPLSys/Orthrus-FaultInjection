#include <memory.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>

#include <atomic>
#include <cassert>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "../monitor.hpp"
#include "masstree.hpp"

Value uint64_to_value(uint64_t val) {
    constexpr uint64_t primes[] = {
        2,   3,   5,   7,   11,  13,  17,  19,  23,  29,  31,  37,  41,  43,
        47,  53,  59,  61,  67,  71,  73,  79,  83,  89,  97,  101, 103, 107,
        109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181,
        191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263,
        269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349,
        353, 359, 367, 373, 379, 383, 389, 397, 401, 409};
    assert(VAL_LEN <= sizeof(primes) / sizeof(primes[0]));
    Value v;
    for (size_t i = 0; i < VAL_LEN; ++i) {
        v.ch[i] = 'a' + (val % primes[i]) % 26;
    }
    return v;
}

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
    memcpy(data, mmap_ptr, sizeof(T) * size);
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

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 5) {
        fprintf(stderr,
                "Usage: %s <data_file> [n_threads]"
                "[record_count] [operation_count]\n",
                argv[0]);
        fprintf(stderr, "data_file: path to the data file\n");
        fprintf(stderr, "n_threads: number of threads to run, default: 4\n");
        fprintf(stderr,
                "record_count: number of records to load, default: 190M\n");
        fprintf(stderr,
                "operation_count: number of operations to run, default: 50M\n");
        return 1;
    }

    Node **root = build_tree();
    uint64_t n_threads = argc > 2 ? atoi(argv[2]) : 4;
    uint64_t record_count = argc > 3 ? atoi(argv[3]) : 190000000;
    uint64_t operation_count = argc > 4 ? atoi(argv[4]) : 50000000;

    fprintf(stderr, "hyperparameter parsed, start loading data\n");

    std::vector<uint64_t> keys(record_count);
    load_from_binary<uint64_t>(keys.data(), keys.size(), argv[1]);

    std::mt19937 rng(233333);
    keys.emplace_back(0);
    std::sort(keys.begin(), keys.end());
    std::vector<Value> vals(keys.size());
    for (size_t i = 0; i < keys.size(); ++i) vals[i] = uint64_to_value(rng());

    uint64_t sizes = keys.size();
    fprintf(stderr, "load completed, %zu K-V pairs\n", sizes);
    std::vector<std::thread> threads;
    {
        monitor::process_memory mem_monitor(stderr, 16 * 2);

        mem_monitor.record();

        root = build_tree_from_keys(keys, vals);

        mem_monitor.record();
    }
    /*
        fprintf(stderr, "inserting %zu K-V pairs\n", sizes);

        uint64_t insert_start = rdtsc();

        {
            constexpr uint64_t InsertPrints = 20;
            monitor::evaluation eva(stderr, sizes, n_threads,
       "MassTree-Insert"); std::atomic_uint64_t finished = 0; for (uint64_t t =
       0; t < n_threads; ++t) { threads.emplace_back( [t, root, sizes,
       n_threads, &keys, &vals, &eva, &finished]() { for (uint64_t i = t; i <
       sizes; i += n_threads) { uint64_t p = rdtsc(); auto ret = insert(root,
       keys[i], vals[i]); eva.cnts[t].c++; eva.latency[i] = nanosecond(p,
       rdtsc());
                        }
                        finished += 1;
                    });
            }
            threads.emplace_back([n_threads, &finished, &eva]() {
                while (finished.load(std::memory_order_relaxed) < n_threads) {
                    if (!finished.load(std::memory_order_relaxed)) {
                        eva.report();
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                }
            });
            for (auto &t : threads) t.join();
            threads.clear();
        }

        uint64_t insert_end = rdtsc();
        uint64_t insert_time = microsecond(insert_start, insert_end);
        fprintf(stderr, "insert finished in %zu us, throughput: %.2f Mops/s\n",
                insert_time, sizes * 1.0 / insert_time);*/

    constexpr int p_all = 100, p_insert = 0, p_read = 100;
    zipf_table_distribution<> zipf(keys.size(), 0.99);
    std::vector<uint64_t> key_in(operation_count), key_out(operation_count);
    std::vector<Value> val_out(operation_count);
    std::vector<int> ops(operation_count);
    for (uint64_t i = 0; i < operation_count; ++i) {
        key_in[i] = keys[zipf(rng)];
        key_out[i] = (rng() << 32) ^ rng();
        val_out[i] = uint64_to_value(rng());
        ops[i] = rng() % p_all;
    }

    fprintf(stderr, "workload starts\n");

    uint64_t workload_start = rdtsc();

    {
        constexpr uint64_t WorkloadPrints = 20;
        monitor::evaluation eva(stderr, operation_count, n_threads,
                                "MassTree-Workload");
        std::atomic_uint64_t finished = 0;
        for (uint64_t t = 0; t < n_threads; ++t) {
            threads.emplace_back([t, root, operation_count, n_threads, &key_in,
                                  &key_out, &val_out, &ops, &eva, &finished]() {
                for (uint64_t i = t; i < operation_count; i += n_threads) {
                    uint64_t p = rdtsc();
                    if (ops[i] < p_insert) {
                        insert(root, key_out[i], val_out[i]);
                    } else if (ops[i] < p_insert + p_read) {
                        const Value *ret = read(root, key_in[i]);
                        assert(ret != nullptr);
                    }
                    eva.cnts[t].c++;
                    eva.latency[i] = nanosecond(p, rdtsc());
                }
                finished += 1;
            });
        }
        threads.emplace_back([n_threads, &finished, &eva]() {
            while (finished.load(std::memory_order_relaxed) < n_threads) {
                if (!finished.load(std::memory_order_relaxed)) {
                    eva.report();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
        });
        for (auto &t : threads) t.join();
        threads.clear();
    }

    uint64_t workload_end = rdtsc();
    uint64_t workload_time = microsecond(workload_start, workload_end);
    fprintf(stderr, "workload finished in %zu us, throughput: %.2f Mops/s\n",
            workload_time, operation_count * 1.0 / workload_time);

    return 0;
}
