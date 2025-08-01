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
                     uint32_t s, uint32_t t) {
    const block_t *_block = block->load();
    uint32_t count = 0;
    for (uint32_t i = l; i <= r; i++) {
        if (_block->sorted[i] >= s && _block->sorted[i] <= t) {
            count++;
        }
    }
    return count;
}

}  // namespace NAMESPACE
