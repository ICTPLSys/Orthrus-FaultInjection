// SHOULD NOT include any header files
// SHOULD NOT #pragma once

constexpr int block_size = 1024;
struct block_t {
    uint32_t value[block_size], sorted[block_size];
};
static_assert(std::has_unique_object_representations_v<block_t>);

constexpr int block_count = 128;
struct tag_t {
    uint32_t v[block_count];
};
static_assert(std::has_unique_object_representations_v<tag_t>);
struct array_t {
    scee::ptr_t<block_t> *blocks[block_count];
    scee::ptr_t<tag_t> *tags;
};
static_assert(std::has_unique_object_representations_v<array_t>);

// a[l, r] += v
void range_update(scee::ptr_t<array_t> *a, uint32_t l, uint32_t r, uint32_t v);

// query a[l, r], count number of values in [s, t]
uint32_t range_query(scee::ptr_t<array_t> *a, uint32_t l, uint32_t r,
                     uint32_t s, uint32_t t);
