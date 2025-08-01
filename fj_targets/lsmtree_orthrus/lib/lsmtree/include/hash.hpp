#pragma once

#include "consts.hpp"

namespace NAMESPACE::lsmtree {

inline uint32_t FilterHash1(uint64_t k) {
    k = ~k + (k << 15);
    k = k ^ (k >> 12);
    k = k + (k << 2);
    k = k ^ (k >> 4);
    k = k * 2057;
    k = k ^ (k >> 16);
    return k % FILTER_BIT_SIZE;
}

inline uint32_t FilterHash2(uint64_t k) {
    k = (k + 0x7ed55d16) + (k << 12);
    k = (k ^ 0xc761c23c) ^ (k >> 19);
    k = (k + 0x165667b1) + (k << 5);
    k = (k + 0xd3a2646c) ^ (k << 9);
    k = (k + 0xfd7046c5) + (k << 3);
    k = (k ^ 0xb55a4f09) ^ (k >> 16);
    return k % FILTER_BIT_SIZE;
}

inline uint32_t FilterHash3(uint64_t k) {
    k = (k ^ 61) ^ (k >> 16);
    k = k + (k << 3);
    k = k ^ (k >> 4);
    k = k * 0x27d4eb2d;
    k = k ^ (k >> 15);
    return k % FILTER_BIT_SIZE;
}

inline void FilterSet(uint32_t *filter, uint32_t index) {
    const uint32_t word_index = index / (sizeof(uint32_t) * 8);
    const uint32_t bit_index = index % (sizeof(uint32_t) * 8);
    filter[word_index] |= 1U << bit_index;
}

inline bool IsFilterSet(const uint32_t *filter, uint32_t index) {
    const uint32_t word_index = index / (sizeof(uint32_t) * 8);
    const uint32_t bit_index = index % (sizeof(uint32_t) * 8);
    return (filter[word_index] & (1U << bit_index)) != 0;
}

inline void FilterSetKey(uint32_t *filter, uint64_t key) {
    FilterSet(filter, FilterHash1(key));
    FilterSet(filter, FilterHash2(key));
    FilterSet(filter, FilterHash3(key));
}

inline bool IsFilterSetKey(const uint32_t *filter, uint64_t key) {
    return (IsFilterSet(filter, FilterHash1(key)) &&
            IsFilterSet(filter, FilterHash2(key)) &&
            IsFilterSet(filter, FilterHash3(key)));
}

}  // namespace NAMESPACE::lsmtree
