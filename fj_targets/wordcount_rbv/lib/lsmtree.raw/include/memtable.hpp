#pragma once

#include <cstddef>
// #include <ranges>
// #include <span>
#include <tuple>

#include "compiler.hpp"
#include "consts.hpp"
#include "custom_stl.hpp"
#include "hash.hpp"
#include "log.hpp"
#include "namespace.hpp"
#include "ptr.hpp"
#include "runtime.hpp"
#include "scee.hpp"

namespace NAMESPACE::lsmtree {

struct SkipListNode {
    KeyT key;
    Data* data;
    const uint64_t level;
    SkipListNode* next[SKIPLIST_MAX_LEVEL + 1];

    SkipListNode(KeyT key, ValueT value, int lvl, SkipListNode* nxt = nullptr);
    SkipListNode(KeyT key, ValueT value, int lvl, SkipListNode* nxts[]);

    static int get_random_level() {
        int lvl = 0;
        while (lvl < SKIPLIST_MAX_LEVEL) {
            auto rand_v = rt::rand();
            if (rand_v >= SKIPLIST_P) break;
            ++lvl;
        }
        return lvl;
    }
};

class SkipList {
public:
    SkipList(const SkipList&) = delete;
    SkipList& operator=(const SkipList&) = delete;
    SkipList(SkipList&&) = delete;
    SkipList& operator=(SkipList&&) = delete;

public:
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

private:
    SkipListNode* head_;
    SkipListNode* tail_;

    int level_;
};

// Redefine the SkipList as MemTable.
using MemTable = SkipList;

}  // namespace NAMESPACE::lsmtree
