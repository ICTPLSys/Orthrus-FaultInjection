#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>

#include "log.hpp"
#include "namespace.hpp"
#include "ptr.hpp"
#include "scee.hpp"

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
ptr_t<block_t> *global_block;

uint32_t rand_uint4() { return (uint32_t)rand() & 0xF; }
uint32_t rand_uint16() { return (uint32_t)rand() & 0xFFFF; }
uint32_t rand_uint32() { return (uint32_t)rand() << 16 | rand(); }

void init() {
    srand(1234567);
    block_t block;
    for (int j = 0; j < block_size; j++) block.value[j] = rand_uint4();
    memcpy(block.sorted, block.value, sizeof(block.value));
    std::sort(block.sorted, block.sorted + block_size);
    global_block = ptr_t<block_t>::create(block);
    DEBUG("INIT FINISHED block = %p", global_block);
}

constexpr uint32_t query_count = 1E6;
constexpr uint32_t query_step = 1E4;

void run() {
    for (int i = 0; i < query_count; i++) {
        if (i % query_step == 0) fprintf(stderr, "query #%d\n", i);
        uint32_t l = rand_uint32() % block_size;
        uint32_t r = rand_uint32() % block_size;
        if (l > r) std::swap(l, r);
        if (rand_uint16() % 2) {
            // block_update
            uint32_t v = rand_uint4();
            if (rand_uint16() % 2) v = -v;
            using BlockUpdateType =
                void (*)(scee::ptr_t<block_t> *, uint32_t, uint32_t, uint32_t);
            BlockUpdateType app_fn =
                reinterpret_cast<BlockUpdateType>(app::block_update);
            BlockUpdateType val_fn =
                reinterpret_cast<BlockUpdateType>(validator::block_update);
            scee::run2(app_fn, val_fn, global_block, l, r, v);
        } else {
            // block_query
            uint32_t s = rand_uint4(), t = rand_uint4();
            if (s > t) std::swap(s, t);
            using BlockQueryType = uint32_t (*)(
                scee::ptr_t<block_t> *, uint32_t, uint32_t, uint32_t, uint32_t);
            BlockQueryType app_fn =
                reinterpret_cast<BlockQueryType>(app::block_query);
            BlockQueryType val_fn =
                reinterpret_cast<BlockQueryType>(validator::block_query);
            scee::run2(app_fn, val_fn, global_block, l, r, s, t);
        }
        auto *log =
            static_cast<scee::LogHead *>(scee::log_dequeue(&scee::log_queue));
        assert(log != nullptr);
        scee::validate_one(log);
    }
}

int main() {
    init();
    run();
    return 0;
}
