#pragma once

#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <custom_stl.hpp>
#include <filesystem>
#include <vector>

#include "checksum.hpp"

namespace NAMESPACE::lsmtree {

#ifndef LSMTREE_ENABLE_SKIPLIST_CACHE_ACCESS
#define LSMTREE_ENABLE_SKIPLIST_CACHE_ACCESS false
#endif

#ifndef LSMTREE_PROFILE_SKIPLIST_RDTSC
#define LSMTREE_PROFILE_SKIPLIST_RDTSC true
#endif

#ifndef LSMTREE_PROFILE_MEMORY_USAGE
#define LSMTREE_PROFILE_MEMORY_USAGE false
#endif

#ifndef LSMTREE_PROFILE_RUNTIME_RDTSC
#define LSMTREE_PROFILE_RUNTIME_RDTSC false
#endif

namespace fs = std::filesystem;

//! TEMPORARY: this is a temp solution
#ifndef MYASSERT
#define MYASSERT(x)                                                           \
    do {                                                                      \
        if (!(x)) {                                                           \
            fprintf(stderr, "[%s:%d] Assertion failed:", __FILE__, __LINE__); \
            fputs(#x "\n", stderr);                                           \
            abort();                                                          \
        }                                                                     \
    } while (0)
#endif

#ifndef KDEBUG
#define KDEBUG(fmt, ...) \
    fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define KTRAP(x)            \
    do {                    \
        if (!(x)) {         \
            raise(SIGTRAP); \
        }                   \
    } while (0)

using KeyT = int64_t;
using ValueT = int64_t;
constexpr KeyT InvalidKey = std::numeric_limits<KeyT>::max();

enum class Retcode {
    Success = 0,

    Found,
    NotFound,

    Update,
    Insert,

    Empty,

    Fail = 255,
};

struct Data {
    KeyT key;
    ValueT value;
    uint32_t crc;
    uint32_t is_delete;

    Data() = default;

    Data(KeyT key, ValueT value, uint32_t is_delete)
        : key(key), value(value), is_delete(is_delete)
    {
        set_crc();
    }

    static uint64_t calc_crc(KeyT key, ValueT value, uint32_t is_delete) {
        uint8_t buf[sizeof(key) + sizeof(value) + sizeof(is_delete)];
        *(decltype(value)*)(buf) = value;
        *(decltype(key)*)(buf+sizeof(value)) = key;
        *(decltype(is_delete)*)(buf+sizeof(value)+sizeof(key)) = is_delete;
        auto crc = calculate_crc32(buf, sizeof(buf));
        return crc;
    }

    void set_crc() {
        this->crc = calc_crc(this->key, this->value, this->is_delete);
    }

    bool check_crc(KeyT key, ValueT value, uint32_t is_delete) const {
        auto crc = calc_crc(key, value, is_delete);
        return crc == this->crc;
    }
};
// static_assert(sizeof(Data) == 32);
static_assert(std::has_unique_object_representations_v<Data>);

// SkipList consts
constexpr KeyT SKIPLIST_KEY_INVALID = InvalidKey;
constexpr int SKIPLIST_MAX_LEVEL = 12;
constexpr int SKIPLIST_P_MASK = 0xFFFF;
constexpr double SKIPLIST_P = 1.0/4.0;

// BlkCache consts
constexpr int BLK_CACHE_MAX_COUNT = 1024;
constexpr int BLK_CACHE_MAX_SIZE =
    BLK_CACHE_MAX_COUNT * sizeof(Data);  // 4KB aligned
static_assert(BLK_CACHE_MAX_SIZE % 4 * 1024 == 0);
constexpr uint64_t MEMTABLE_MAX_SIZE = 512UL << 20;  // 512MiB
constexpr int FILTER_BIT_SIZE = BLK_CACHE_MAX_COUNT;

struct BlkInfo {
    // key range
    KeyT blk_key_min = SKIPLIST_KEY_INVALID;
    KeyT blk_key_max = 0;  // int64_t but only positive part.

    // bloom filter
    uint32_t filter[FILTER_BIT_SIZE / sizeof(uint32_t)];

    // blk meta data
    int offset_in_sst = 0;
    int count = 0;

    // Blk status
    bool in_cache = false;

    BlkInfo() { memset(filter, 0, sizeof(filter)); }
};

struct BlkData {
    BlkInfo* blk_info = nullptr;

    Data* data;
    int count;

    ~BlkData() {
        // FIXME(kuriko): I dont think this relationship is good.
        delete data;
        blk_info->in_cache = false;
    }
};

struct SstMeta {
    fs::path filepath;

    KeyT key_min;
    KeyT key_max;

    std::vector<BlkInfo> blk_infos;
};

using CacheKey = uint64_t;
}  // namespace NAMESPACE::lsmtree
