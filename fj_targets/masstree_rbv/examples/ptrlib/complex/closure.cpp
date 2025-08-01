#include <algorithm>
#include <cstdint>
#include <cstring>

#include "context.hpp"
#include "namespace.hpp"
#include "ptr.hpp"

namespace NAMESPACE {
#include "closure.hpp"

void block_update(scee::ptr_t<block_t> *block, uint32_t l, uint32_t r,
                  uint32_t v) {
    const block_t *old_block = block->load();
    block_t new_block;
    memcpy(new_block.value, old_block->value, block_size * sizeof(uint32_t));
    for (uint32_t i = l; i <= r; i++) new_block.value[i] += v;
    memcpy(new_block.sorted, new_block.value, block_size * sizeof(uint32_t));
    std::sort(new_block.sorted, new_block.sorted + block_size);
    block->store(new_block);
}

uint32_t block_query(scee::ptr_t<block_t> *block, uint32_t l, uint32_t r,
                     uint32_t s, uint32_t t, uint32_t v) {
    const block_t *_block = block->load();
    uint32_t count = 0;
    for (uint32_t i = l; i <= r; i++) {
        if ((_block->sorted[i] + v) >= s && (_block->sorted[i] + v) <= t) {
            count++;
        }
    }
    return count;
}

uint32_t block_query(scee::ptr_t<block_t> *block, uint32_t s, uint32_t t) {
    const block_t *_block = block->load();
    uint32_t count = 0;
    count += std::upper_bound(_block->sorted, _block->sorted + block_size, t) -
             _block->sorted;
    if (s > t) count += block_size;
    count -= std::lower_bound(_block->sorted, _block->sorted + block_size, s) -
             _block->sorted;
    return count;
}

void range_update(scee::ptr_t<array_t> *a, uint32_t l, uint32_t r, uint32_t v) {
    // [l, r] += v
    // O(block_count + block_size * log(block_size))
    const array_t *array = a->load();
    tag_t new_tag = *array->tags->load();
    uint32_t block_l = l / block_size;
    uint32_t block_r = r / block_size;
    for (uint32_t i = block_l; i <= block_r; i++) {
        uint32_t ll = std::max(l, i * block_size);
        uint32_t rr = std::min(r, (i + 1) * block_size - 1);
        ll -= i * block_size, rr -= i * block_size;
        if (ll > 0 || rr < block_size - 1) {
            block_update(array->blocks[i], ll, rr, v);
        } else {
            new_tag.v[i] += v;
        }
    }
    array->tags->store(new_tag);
}

uint32_t range_query(scee::ptr_t<array_t> *a, uint32_t l, uint32_t r,
                     uint32_t s, uint32_t t) {
    // query [l, r], count number of values in [s, t]
    // O(block_count * log(block_size) + block_size)
    const array_t *array = a->load();
    const tag_t *tag = array->tags->load();
    uint32_t block_l = l / block_size;
    uint32_t block_r = r / block_size;
    uint32_t count = 0;
    for (uint32_t i = block_l; i <= block_r; i++) {
        uint32_t ll = std::max(l, i * block_size);
        uint32_t rr = std::min(r, (i + 1) * block_size - 1);
        ll -= i * block_size, rr -= i * block_size;
        if (ll > 0 || rr < block_size - 1) {
            count += block_query(array->blocks[i], ll, rr, s, t, tag->v[i]);
        } else {
            count +=
                block_query(array->blocks[i], s - tag->v[i], t - tag->v[i]);
        }
    }
    return count;
}

}  // namespace NAMESPACE
