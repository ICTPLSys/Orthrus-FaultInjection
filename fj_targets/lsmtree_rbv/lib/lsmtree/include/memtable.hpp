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
    scee::ptr_t<Data>* data;
    // Data* data;
    const uint64_t level;
    const SkipListNode* next[SKIPLIST_MAX_LEVEL + 1];

    SkipListNode(KeyT key, ValueT value, int lvl, const SkipListNode* nxt = nullptr);
    SkipListNode(KeyT key, ValueT value, int lvl, const SkipListNode* nxts[]);

    static int get_random_level() {
        int lvl = 0;
        while (lvl < SKIPLIST_MAX_LEVEL) {
            auto rand_v = (long double)(1.0) * rt::rand() / std::numeric_limits<uint32_t>::max();
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
    // size_t size() const { return sizeof(this); }
    // void write_at(void *shadow, void *real, size_t size) const {};
    // void destroy() const {};

    SkipList();

    // ~SkipList() {
    //     SkipListNode* p = head_;
    //     while (p != tail_) {
    //         auto* nxt = p->next[0];
    //         delete p;
    //         p = nxt;
    //     }
    // }

    ValueT Get(KeyT key) const;

    Retcode Set(KeyT key, ValueT value) const;

    Retcode Del(KeyT key) const;

    const SkipListNode* findCache(
        const SkipListNode* p, const Data* p_data, int level, KeyT key) const;

    void updateCache(int level, const SkipListNode* node) const;

    // size_t mem_size() const {
    //     auto ret = *mem_size_->load();
    //     return ret;
    // }
    // size_t count() const { return *count_->load(); }

    SstMeta* dump_disk(const fs::path& filepath, FILE* file);

private:
    SkipListNode* head_;
    SkipListNode* tail_;

    scee::ptr_t<int>* level_;
    // int* level_;

#if (LSMTREE_ENABLE_SKIPLIST_CACHE_ACCESS)
    scee::ptr_t<SkipListNode>* cache_access_[SKIPLIST_MAX_LEVEL + 1];
#endif

    // The actual count of elements in the SkipList
    // scee::ptr_t<size_t> *count_;
    // scee::ptr_t<KeyT>   *avg_;  // actually this should be middle

    // The actual memory usage
    // scee::ptr_t<size_t>* mem_size_;
};
static_assert(std::has_unique_object_representations_v<SkipList>);

// R;edefine the SkipList as MemTable.
using MemTable = SkipList;

}  // namespace NAMESPACE::lsmtree
