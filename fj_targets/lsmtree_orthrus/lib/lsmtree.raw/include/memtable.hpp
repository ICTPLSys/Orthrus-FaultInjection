#pragma once

#include <cstddef>
// #include <ranges>
// #include <span>
#include <tuple>
#include <iostream>
#include <random>
#include "stack"

#include "compiler.hpp"
#include "consts.hpp"
#include "custom_stl.hpp"
#include "hash.hpp"
#include "log.hpp"
#include "namespace.hpp"
#include "ptr.hpp"
#include "runtime.hpp"
#include "scee.hpp"

#include <vector>

// namespace mt {
// #include "mersenne-twister.h"
// }


namespace NAMESPACE::lsmtree {

struct SkipListNode {
    KeyT key;
    Data* data;
    const uint64_t level;
    SkipListNode* next[SKIPLIST_MAX_LEVEL + 1];

    SkipListNode(KeyT key, ValueT value, int lvl, SkipListNode* nxt = nullptr);
    SkipListNode(KeyT key, ValueT value, int lvl, SkipListNode* nxts[]);
};

struct InternalStates {
    std::vector<std::tuple<KeyT, int>> accessed_key_and_lvl;
    void record_access(SkipListNode* p, int level) {
        accessed_key_and_lvl.push_back({p->key, level});
    }

    friend bool operator == (const InternalStates& a, const InternalStates& b) {
        const auto& aData = a.accessed_key_and_lvl;
        const auto& bData = b.accessed_key_and_lvl;

        if (aData.size() != bData.size())
            return false;

        for (int i = 0; i < aData.size(); i++) {
            if (aData[i] != bData[i]) return false;
        }
        return true;
    }
};

class SkipList {
public:
    SkipList(const SkipList&) = delete;
    SkipList& operator=(const SkipList&) = delete;
    SkipList(SkipList&&) = delete;
    SkipList& operator=(SkipList&&) = delete;

public:
    std::mt19937 gen;

    int get_random_level() {
        int lvl = 0;
        while (lvl < SKIPLIST_MAX_LEVEL) {
            auto rand_v = (long double)(1.0) * gen() / std::numeric_limits<uint32_t>::max();
            // fprintf(stderr, "rand: %lf\n", rand_v);
            if (rand_v >= SKIPLIST_P) break;
            ++lvl;
        }
        return lvl;
    }

    SkipList();

    // ~SkipList() {
    //     SkipListNode* p = head_;
    //     while (p != tail_) {
    //         auto* nxt = p->next[0];
    //         delete p;
    //         p = nxt;
    //     }
    // }

    ValueT Get(KeyT key);

    Retcode Set(KeyT key, ValueT value);

    Retcode Del(KeyT key);

    SstMeta* dump_disk(const fs::path& filepath, FILE* file);

    const InternalStates& GetInternalState() const {
        return this->internal_state;
    }

private:
    SkipListNode* head_;
    SkipListNode* tail_;

    int level_;

    InternalStates internal_state;
};

// Redefine the SkipList as MemTable.
using MemTable = SkipList;

}  // namespace NAMESPACE::lsmtree
