#include "phoenix.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <optional>
#include <string_view>

#include "context.hpp"
#include "log.hpp"
#include "scee.hpp"
#include "utils.hpp"

namespace raw {
#include "closure.hpp"
}  // namespace raw
namespace app {
#include "closure.hpp"
}  // namespace app
namespace validator {
#include "closure.hpp"
}  // namespace validator

namespace NAMESPACE {

struct raw_kv_pair {
    std::string_view key;
    uint64_t value;
};

struct hash_table_entry {
    raw_kv_pair kv;
    hash_table_entry *next;
};

struct hash_table {
    hash_table_entry *entries;
    size_t entry_count;
    size_t size;

    explicit hash_table(size_t entry_count);
    ~hash_table();
    void inc(std::string_view key, size_t hash);
    void collect(kv_pair *kv_pairs) const;
};

using scee::imm_array;
using scee::mut_array;

static word create_word(std::string_view str) {
    const auto data = imm_array<char>::create(str.size(), [str](char *data) {
        std::memcpy(data, str.data(), str.size());
    });
    return word(data, str.size());
}

void destroy_word(word w) { w.data.destroy(); }

inline hash_table::hash_table(size_t entry_count)
    : entries(new hash_table_entry[entry_count]),
      entry_count(entry_count),
      size(0) {
    memset(entries, 0, entry_count * sizeof(hash_table_entry));
}

inline hash_table::~hash_table() {
#ifdef PROFILE_MEM
    for (size_t i = 0; i < entry_count; ++i) {
        auto *item = entries[i].next;
        while (item != nullptr) {
            auto *next = item->next;
            delete item;
            item = next;
        }
    }
#endif
    delete[] entries;
}

inline size_t key_hash(std::string_view key) {
    return std::hash<std::string_view>{}(key);
}

inline bool key_equal(word w, std::string_view k) {
    return w.size == k.size() &&
           std::memcmp(w.data.deref(), k.data(), w.size) == 0;
}

inline bool key_equal(word a, word b) {
    return a.size == b.size &&
           std::memcmp(a.data.deref(), b.data.deref(), a.size) == 0;
}

inline bool key_less(word a, word b) {
    if (a.size != b.size) {
        return a.size < b.size;
    }
    return std::memcmp(a.data.deref(), b.data.deref(), a.size) < 0;
}

void hash_table::inc(std::string_view key, size_t hash) {
    size_t bucket_idx = hash % entry_count;
    auto *item = entries + bucket_idx;
    if (item->kv.key.empty()) {
        item->kv = raw_kv_pair{key, 1};
        size += 1;
        return;
    }
    while (true) {
        if (item->kv.key == key) {
            item->kv.value += 1;
            return;
        }
        if (item->next == nullptr) {
            item->next = new hash_table_entry{
                .kv = raw_kv_pair{key, 1},
                .next = nullptr,
            };
            size += 1;
            return;
        }
        item = item->next;
    }
}

void hash_table::collect(kv_pair *kv_pairs) const {
    size_t kv_offset = 0;
    for (size_t i = 0; i < entry_count; ++i) {
        auto *item = entries + i;
        if (item->kv.key.empty()) {
            continue;
        }
        while (item != nullptr) {
            kv_pairs[kv_offset++] =
                kv_pair(create_word(item->kv.key), item->kv.value);
            item = item->next;
        }
    }
    assert(kv_offset == size);
}

inline const char *skip_spaces(const char *cursor, const char *end) {
    while (cursor < end && std::isspace(*cursor)) {
        ++cursor;
    }
    return cursor;
}

inline const char *skip_word(const char *cursor, const char *end) {
    while (cursor < end && !std::isspace(*cursor)) {
        ++cursor;
    }
    return cursor;
}

scee::mut_array<result_t> create_result_array(size_t n) {
    return scee::mut_array<result_t>::create(n);
}

void destroy_result_array(scee::mut_array<result_t> results) {
    results.destroy();
}

template <typename Fn>
static size_t scan_words(const char *start, size_t length, Fn add_kv) {
    const char *end = start + length;
    const char *cursor = skip_spaces(start, end);
    while (cursor < end) {
        const char *word_start = cursor;
        cursor = skip_word(cursor, end);
        add_kv(std::string_view(word_start, cursor - word_start));
        cursor = skip_spaces(cursor, end);
    }
    return cursor - start;
}

void word_count_map_worker(std::string_view input,
                           scee::mut_array<result_t> results, size_t mapper_idx,
                           map_reduce_config config) {
    std::vector<hash_table> hash_tables;
    hash_tables.reserve(config.n_reducers);
    for (size_t i = 0; i < config.n_reducers; ++i) {
        hash_tables.emplace_back(config.n_buckets);
    }
    scan_words(
        input.data(), input.size(),
        [&hash_tables, n_reducers = config.n_reducers](std::string_view key) {
            size_t hash = key_hash(key);
            hash_table &ht = hash_tables[hash % n_reducers];
            ht.inc(key, hash);
        });
    for (size_t i = 0; i < config.n_reducers; ++i) {
        auto &ht = hash_tables[i];
        size_t idx = mapper_idx * config.n_reducers + i;
        auto kv_pairs = scee::imm_array<kv_pair>::create(
            ht.size, [&ht](kv_pair *kv_pairs) { ht.collect(kv_pairs); });
        results.store(idx, {kv_pairs, ht.size});
    }
}

static void shuffle_kv_pairs(kv_pair *kv_pairs, size_t size) {
    std::sort(kv_pairs, kv_pairs + size,
              [](const kv_pair &a, const kv_pair &b) {
                  return key_less(a.key, b.key);
              });
}

static result_t reduce(const kv_pair *kv_pairs, size_t size) {
    if (size == 0) {
        return {imm_array<kv_pair>::null(), 0};
    }
    size_t reduced_size = 0;
    imm_array<kv_pair> reduced = imm_array<kv_pair>::create(
        size, [kv_pairs, size, &reduced_size](kv_pair *reduced) {
            word current_key = kv_pairs[0].key;
            size_t current_count = 0;
            for (size_t i = 0; i < size; ++i) {
                if (key_equal(kv_pairs[i].key, current_key)) {
                    current_count += kv_pairs[i].value;
                } else {
                    reduced[reduced_size].key = current_key;
                    reduced[reduced_size].value = current_count;
                    reduced_size++;
                    current_key = kv_pairs[i].key;
                    current_count = kv_pairs[i].value;
                }
            }
            reduced[reduced_size].key = current_key;
            reduced[reduced_size].value = current_count;
            reduced_size++;
            for (size_t i = reduced_size; i < size; ++i) {
                reduced[i].key = word(imm_array<char>::null(), 0);
                reduced[i].value = 0;
            }
        });
    return {reduced, reduced_size};
}

void word_count_reduce_worker(scee::mut_array<result_t> map_results,
                              scee::mut_array<result_t> reduce_results,
                              size_t reducer_idx, map_reduce_config config) {
    auto get_ith_mapper_result = [map_results, reducer_idx, config](size_t i) {
        return map_results.deref(i * config.n_reducers + reducer_idx);
    };
    // collect all kv pairs from all mappers
    size_t total_kvs = 0;
    for (size_t i = 0; i < config.n_mappers; ++i) {
        total_kvs += get_ith_mapper_result(i)->second;
    }
    auto *kv_pairs = new kv_pair[total_kvs];
    size_t kv_offset = 0;
    for (size_t i = 0; i < config.n_mappers; ++i) {
        const auto *mapper_result = get_ith_mapper_result(i);
        const auto *src_kv_pairs = mapper_result->first.deref();
        size_t n_kvs = mapper_result->second;
        std::memcpy(kv_pairs + kv_offset, src_kv_pairs,
                    n_kvs * sizeof(kv_pair));
        kv_offset += n_kvs;
    }
    // shuffle kv pairs
    shuffle_kv_pairs(kv_pairs, total_kvs);
    // reduce kv pairs
    auto result = reduce(kv_pairs, total_kvs);
    delete[] kv_pairs;
    reduce_results.store(reducer_idx, result);
}

result_t sort_results(scee::mut_array<result_t> results,
                      map_reduce_config config) {
    size_t total_size = 0;
    size_t n_reducers = config.n_reducers;
    for (size_t i = 0; i < n_reducers; ++i) {
        total_size += results.deref(i)->second;
    }
    imm_array<kv_pair> final_results = imm_array<kv_pair>::create(
        total_size, [results, total_size, n_reducers](kv_pair *final_results) {
            size_t offset = 0;
            for (size_t i = 0; i < n_reducers; ++i) {
                const auto *result = results.deref(i);
                std::memcpy(final_results + offset,
                            result->first.deref_nullable(),
                            result->second * sizeof(kv_pair));
                offset += result->second;
            }
            std::sort(final_results, final_results + total_size,
                      [](const kv_pair &a, const kv_pair &b) {
                          return a.value > b.value;
                      });
        });
    return {final_results, total_size};
}

}  // namespace NAMESPACE
