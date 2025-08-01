// SHOULD NOT include any header files
// SHOULD NOT #pragma once

constexpr int block_size = 4;
struct block_t {
    uint32_t value[block_size], sorted[block_size];
};
static_assert(std::has_unique_object_representations_v<block_t>);

// a[l, r] += v
void block_update(scee::ptr_t<block_t> *block, uint32_t l, uint32_t r,
                  uint32_t v);

// query a[l, r], count number of values in [s, t]
uint32_t block_query(scee::ptr_t<block_t> *block, uint32_t l, uint32_t r,
                     uint32_t s, uint32_t t);
