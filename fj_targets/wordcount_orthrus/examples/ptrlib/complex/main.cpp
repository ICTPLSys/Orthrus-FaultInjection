#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>

#include "log.hpp"
#include "namespace.hpp"
#include "ptr.hpp"
#include "scee.hpp"
#include "thread.hpp"

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

// these are all versions of raw
ptr_t<array_t> *global_arr;

uint32_t rand_uint16() { return (uint32_t)rand() & 0xFFFF; }
uint32_t rand_uint32() { return (uint32_t)rand() << 16 | rand(); }

void init() {
    srand(1234567);
    array_t arr;
    tag_t tag;
    memset(tag.v, 0, sizeof(tag.v));
    arr.tags = ptr_t<tag_t>::create(tag);
    for (int i = 0; i < block_count; i++) {
        block_t block;
        for (int j = 0; j < block_size; j++) block.value[j] = rand_uint32();
        memcpy(block.sorted, block.value, sizeof(block.value));
        std::sort(block.sorted, block.sorted + block_size);
        arr.blocks[i] = ptr_t<block_t>::create(block);
    }
    global_arr = ptr_t<array_t>::create(arr);
    DEBUG("INIT FINISHED arr = %p", global_arr);
}

constexpr uint32_t query_count = 5E5;
constexpr uint32_t query_step = 1E5;

void baseline() {
    srand(1234567);
    uint64_t sum_rdtsc = 0;
    for (int i = 1; i <= query_count; i++) {
        uint32_t l = rand_uint32() % (block_count * block_size);
        uint32_t r = rand_uint32() % (block_count * block_size);
        if (l > r) std::swap(l, r);
        uint16_t op = rand_uint16() % 2;
        uint32_t s = rand_uint32(), t = rand_uint32();
        uint64_t start = __rdtsc();
        if (op == 0)
            range_update(global_arr, l, r, s);
        else
            range_query(global_arr, l, r, std::min(s, t), std::max(s, t));
        sum_rdtsc += _rdtsc() - start;
        if (i % query_step == 0) {
            fprintf(stderr, "Baseline %d queries: time = %lu\n", i,
                    sum_rdtsc / i);
        }
    }
}

void run() {
    srand(1234567);
    uint64_t sum_rdtsc = 0;
    for (int i = 1; i <= query_count; i++) {
        uint32_t l = rand_uint32() % (block_count * block_size);
        uint32_t r = rand_uint32() % (block_count * block_size);
        if (l > r) std::swap(l, r);
        uint16_t op = rand_uint16() % 2;
        uint32_t s = rand_uint32(), t = rand_uint32();
        uint64_t start = __rdtsc();
        if (op == 0) {
            // range_update
            using RangeUpdateType =
                void (*)(scee::ptr_t<array_t> *, uint32_t, uint32_t, uint32_t);
            RangeUpdateType app_fn =
                reinterpret_cast<RangeUpdateType>(app::range_update);
            RangeUpdateType val_fn =
                reinterpret_cast<RangeUpdateType>(validator::range_update);
            scee::run2(app_fn, val_fn, global_arr, l, r, s);
        } else {
            // range_query
            using RangeQueryType = uint32_t (*)(
                scee::ptr_t<array_t> *, uint32_t, uint32_t, uint32_t, uint32_t);
            RangeQueryType app_fn =
                reinterpret_cast<RangeQueryType>(app::range_query);
            RangeQueryType val_fn =
                reinterpret_cast<RangeQueryType>(validator::range_query);
            scee::run2(app_fn, val_fn, global_arr, l, r, s, t);
        }
        sum_rdtsc += _rdtsc() - start;
        if (i % query_step == 0) {
            fprintf(stderr, "Run-Val %d queries: time = %lu\n", i,
                    sum_rdtsc / i);
        }
    }
}

int main_fn() {
    init();
    run();
    baseline();
    return 0;
}

int main() { return scee::main_thread(main_fn); }
