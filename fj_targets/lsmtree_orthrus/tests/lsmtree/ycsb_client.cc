#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstring>
#include <random>
#include <sys/mman.h>

#include <gflags/gflags.h>
#include <platform/utils.h>

#include "scee.h"

#include "consts.h"
#include "dist.h"
#include "workload.h"

// #include <lib/tree/concurrent_btree/cc_db.h>
// #include <lib/tree/stx/btree_map.h>
// #include <lib/tree/stx_db.h>
// #include "lsmtree_db.h"
#include "./rocksdb/rocksdb_db.h"
// #include "./lsmtree_raw_db.h"
#include "./lsmtree_db.h"

std::random_device rd;
std::mt19937 g(rd());

DEFINE_bool(safemode, false, "Enable safe mode");
DEFINE_string(db, "rocksdb",
    "Database (rocksdb, lsmtree_raw, lsmtree)");
DEFINE_string(input, "", "Input file");
DEFINE_string(workload, "a", "YCSB workloads (a-f)");
DEFINE_int32(record_count, 190000000, "Number of records to load");
DEFINE_int32(operation_count, 10000000, "Number of operations to run");
DEFINE_string(sst_file_dir, "/dev/shm/", "Temp directory for sst files");
// DEFINE_string(tmp_dir, "/dev/shm", "Temp directory for storage-based DBs");

static WorkloadConfig kAllYCSBConfigs[] = {
    kWorkloadAConfig, kWorkloadBConfig,
    kWorkloadCConfig, kWorkloadDConfig,
    kWorkloadEConfig, kWorkloadFConfig
};

void inline log(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    time_t t = time(nullptr);
    char tmp[64] = { 0 };
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&t));
    printf("%s ", tmp);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

template <typename T>
void load_from_binary(T* data, size_t size, const std::string& filename)
{
    FILE* fp = fopen(filename.c_str(), "r");
    MYASSERT(fp != nullptr);
    fseek(fp, 0L, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0L, 0);
    // MYASSERT(file_size == sizeof(T) * size);

    void* mmap_ptr = mmap(nullptr, file_size, PROT_READ, MAP_SHARED, fp->_fileno, 0);
    MYASSERT(mmap_ptr != MAP_FAILED);
    memcpy(data, mmap_ptr, sizeof(T) * size);
    fclose(fp);
}

struct KeyGenerator {
    KeyGenerator(std::vector<uint64_t>& keys_to_insert)
        : keys(keys_to_insert)
        , index(0)
    {
    }
    uint64_t operator()()
    {
        int idx = index.fetch_add(1, std::memory_order_relaxed);
        MYASSERT((size_t)idx < keys.size() && "Too many inserts");
        return keys[idx];
    }
    std::vector<uint64_t>& keys;
    std::atomic_int index;
};

static inline WorkloadConfig parse_workload(const std::string& workload)
{
    char c = workload[0];
    if (c >= 'A' && c <= 'F') {
        c = c - 'A' + 'a';
    }
    MYASSERT(c >= 'a' && c <= 'f');
    return kAllYCSBConfigs[c - 'a'];
}

static inline Database<uint64_t, uint64_t> *make_db(
    const std::string &name, bool safemode) {

    if (name == "rocksdb") {
        return new RocksDBDatabase<uint64_t, uint64_t>("./temp/rocksdb");
    }
    // if (name == "lsmtree_raw") {
    //     using namespace scee::raw::lsmtree;
    //     return new LSMTreeRawDatabase<KeyT, ValueT>("./temp/lsmtree_raw");
    // }
    if (name == "lsmtree") {
        using namespace scee::lsmtree;
        scee::g_ctx = &scee::context::plain::g_ctx;
        return new LSMTreeDatabase<KeyT, ValueT>("./temp/lsmtree_db");
    }

    printf("Unknown database %s\n", name.c_str());
    MYASSERT(false);
    return nullptr;
}

int main(int argc, char** argv)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(7, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
        printf("Could not set CPU affinity.\n");
        return -1;
    }

    gflags::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_input.empty()) {
        printf("Please specify the input file\n");
        return -1;
    }
    log("Loading data from %s", FLAGS_input.c_str());
    auto config = parse_workload(FLAGS_workload);
    std::vector<uint64_t> keys(FLAGS_record_count);
    load_from_binary<uint64_t>(keys.data(), keys.size(), FLAGS_input);
    std::vector<uint64_t> keys_to_insert;
    int expect_insert = 0;
    if (config.insert_ratio > 0) {
        expect_insert = int(
            FLAGS_operation_count * (1.0 * config.insert_ratio / static_cast<int>(WorkloadConfig::kTotalRatio)) * 2);
        MYASSERT((size_t)expect_insert < keys.size());

        std::shuffle(keys.begin(), keys.end(), g);
        keys_to_insert.insert(keys_to_insert.end(), keys.begin(),
            keys.begin() + expect_insert);
        keys.erase(keys.begin(), keys.begin() + expect_insert);
    }
    std::sort(keys.begin(), keys.end());
    std::vector<uint64_t> vals(keys.size());
    memcpy(vals.data(), keys.data(), keys.size() * sizeof(uint64_t));

    std::unique_ptr<Database<uint64_t, uint64_t>> db(
        make_db(FLAGS_db, FLAGS_safemode));
    // std::unique_ptr<Database<uint64_t, uint64_t>> db(
    //     new LSMTreeDatabase<uint64_t, uint64_t>(const_cast<char*>(FLAGS_sst_file_dir.c_str())));
    log("Loading data into the database...");
    db->BulkLoad(keys, vals);

    auto workload = std::make_unique<Workload<uint64_t, uint64_t>>(
        config, keys.size(), FLAGS_operation_count, keys);

    KeyGenerator key_gen(keys_to_insert);
    UniformGenerator val_gen(~0U);

    log("Running workload %s ...", FLAGS_workload.c_str());
    // auto start_ts = scee::utils::rdtsc();
    auto start_ts = std::chrono::system_clock::now();
    for (int i = 0; i < FLAGS_operation_count; ++i) {
        workload->DoTransaction(*db, key_gen, val_gen);
    }
    // auto end_ts = scee::utils::rdtsc();
    // double seconds = scee::utils::nanosecond(start_ts, end_ts) / 1000000000.0;
    auto end_ts = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_ts - start_ts);
    double seconds = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
    log("Time: %.4f seconds, avg thrupt %.2f/s\n", seconds,
        FLAGS_operation_count / seconds);
}
