#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "primitives.hpp"

struct word {
    scee::imm_array<char> data;
    size_t size;

    word() = default;

    word(scee::imm_array<char> data, size_t size) : data(data), size(size) {}
};

struct kv_pair {
    word key;
    uint64_t value;

    kv_pair() = default;

    kv_pair(word key, uint64_t value) : key(key), value(value) {}
};

template <typename T, typename U>
struct trivial_pair {
    T first;
    U second;
};

struct map_reduce_config {
    size_t n_mappers;
    size_t n_reducers;
    size_t n_buckets;
    size_t n_threads;
};

using result_t = trivial_pair<scee::imm_array<kv_pair>, size_t>;