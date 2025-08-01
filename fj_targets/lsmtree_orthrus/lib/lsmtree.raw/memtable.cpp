#include <iostream>
#include <cstddef>
#include <tuple>
#include <inttypes.h>
// #include <ranges>
// #include <span>

#include "compiler.hpp"
#include "hash.hpp"
#include "log.hpp"
#include "namespace.hpp"
#include "ptr.hpp"
#include "runtime.hpp"

#include "runtime.hpp"
#include "memtable.hpp"
#include "consts.hpp"

namespace NAMESPACE::lsmtree {

SkipListNode::SkipListNode(KeyT key, ValueT value, int lvl, SkipListNode* nxt)
    : key(key), level(lvl) {
    data = new Data { key, value, false, };
    memset(next, 0, sizeof(next));
    for (int i = 0; i <= lvl; i++) { next[i] = nxt; }
}

SkipListNode::SkipListNode(KeyT key, ValueT value, int lvl, SkipListNode* nxts[])
    : key(key), level(lvl) {
    data = new Data { key, value, false, };
    memset(next, 0, sizeof(next));
    for (int i = 0; i <= lvl; i++) { next[i] = nxts[i]; }
}

//=====================================================================
SkipList::SkipList() : gen(114514) {
    srand(0);
    level_ = 0;
    tail_ = new SkipListNode {SKIPLIST_KEY_INVALID, 0, 0, static_cast<SkipListNode*>(nullptr)};
    head_ = new SkipListNode {SKIPLIST_KEY_INVALID, 0, SKIPLIST_MAX_LEVEL, tail_};
}


ValueT SkipList::Get(KeyT key) {
    auto level = level_;
    auto* p = head_;
    auto* p_data = p->data;

    for (int i = level; i >= 0; i--) {
        while (true) {
            auto* p_next = p->next[i];
            if (p_next->key >= key) { break; }
            p = p_next;
        }
    }
    p = p->next[0];
    p_data = p->data;
    if (p_data->key == key && !p_data->is_delete) {
        if (p_data->check_crc(key, p_data->value, false))
            return p_data->value;
        else
            return -1;
    }
    return -1;
}


struct Stats {
    // Internal states size
    std::vector<int> g_acc_cnts;

    // Timing
    uint64_t time_tot = 0;
    uint64_t time_for = 0;
    uint64_t time_assign = 0;
    uint64_t time_random = 0;
    uint64_t time_update = 0;
    uint64_t time_malloc = 0;
    uint64_t time_relink = 0;

    uint64_t time_tmp = 0;

    void prt_stat() {
        fprintf(stderr, "rations: \n");
        fprintf(stderr, "for   : %" PRIu64 ", %lf\n", time_for, time_for    * 100.0 / time_tot);
        fprintf(stderr, "assign: %" PRIu64 ", %lf\n", time_assign, time_assign * 100.0 / time_tot);
        fprintf(stderr, "random: %" PRIu64 ", %lf\n", time_random, time_random * 100.0 / time_tot);
        fprintf(stderr, "update: %" PRIu64 ", %lf\n", time_update, time_update * 100.0 / time_tot);
        fprintf(stderr, "malloc: %" PRIu64 ", %lf\n", time_malloc, time_malloc * 100.0 / time_tot);
        fprintf(stderr, "relink: %" PRIu64 ", %lf\n", time_relink, time_relink * 100.0 / time_tot);
        fprintf(stderr, "tot:    %" PRIu64 "\n", time_tot);
    }

    Stats() { }
    ~Stats() {
        #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
            fprintf(stderr, "\n========== stats(%s) =========\n", TO_STRING(NAMESPACE));
            fprintf(stderr, "  -------- timing --------------\n");
            prt_stat();
            fprintf(stderr, "  --------- acc --------------\n");
            std::sort(g_acc_cnts.begin(), g_acc_cnts.end());
            uint64_t sum = 0;
            fprintf(stderr, "acc size: %zu\n", g_acc_cnts.size());
            for (auto it : g_acc_cnts) { sum += it; }
            fprintf(stderr, "avg acc length: %lf\n", sum * 1.0 / g_acc_cnts.size());
        #endif
    }
} g_stat;

Retcode SkipList::Set(KeyT key, ValueT value) {
    // reset internal state cache
    this->internal_state = InternalStates();

    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        uint64_t now = rdtsc();
        uint64_t start_0 = now;
        uint64_t start = now;
    #endif

    SkipListNode* update[SKIPLIST_MAX_LEVEL + 1];

    auto level = level_;
    auto* p = head_;
    // if (::rand() % 2 == 1) {
    //     fprintf(stderr, "triggered\n");
    //     level = 0;
    // }

    this->internal_state.record_access(p, level);

    int acc_cnt = 0;
    for (int i = level; i >= 0; i--) {
        while (true) {
            acc_cnt += 1;
            auto* p_next = p->next[i];
            if (p_next->key >= key) { break; }
            p = p_next;
            this->internal_state.record_access(p, i);
        }
        update[i] = p;
    }
    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        g_stat.g_acc_cnts.emplace_back(acc_cnt + 1);
    #endif

    p = p->next[0];
    this->internal_state.record_access(p, 0);

    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        now = rdtsc();
        g_stat.time_for += now - start;
        start = now;
    #endif

    if (p->key == key) {
        *p->data = { key, value, false, };

        #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
            now = rdtsc();
            g_stat.time_assign += now - start;
            g_stat.time_tot += now - start_0;
        #endif
        return Retcode::Update;
    }
    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        now = rdtsc();
        g_stat.time_assign += now - start;
        start = now;
    #endif

    int lvl = this->get_random_level();
    if (lvl > level) {
        lvl = ++level;
        level_ = lvl;
        update[lvl] = head_;
    }
    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        now = rdtsc();
        g_stat.time_random += now - start;
        start = now;
    #endif

    SkipListNode* update_next[SKIPLIST_MAX_LEVEL + 1];
    for (int i = 0; i <= lvl; i++) {
        auto* ptr = update[i]->next[i];
        update_next[i] = ptr;
    }
    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        now = rdtsc();
        g_stat.time_update += now - start;
        start = now;
    #endif

    auto* new_node = new SkipListNode {
        key, value, lvl, update_next,
    };
    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        now = rdtsc();
        g_stat.time_malloc += now - start;
        start = now;
    #endif

    // FIXME(kuriko): actually this should be a cache_write
    for (auto i = lvl; i >= 0; i--) {
        auto* p = update[i];
        p->next[i] = new_node;
    }
    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        now = rdtsc();
        g_stat.time_relink += now - start;
        start = now;
    #endif

    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        now = rdtsc();
        g_stat.time_tot += now - start_0;
    #endif
    return Retcode::Insert;
}

Retcode SkipList::Del(KeyT key) {
    // int const level = *level_->load();
    // const SkipListNode* p = head_->load();
    // const auto* p_data = p->data->load();

    // for (int i = level; i >= 0; i--) {
    //     while (true) {
    //         // const auto* p_next = p->next[i]->load();
    //         const auto* p_next = p->next.get()->v[i]->load();
    //         const auto* p_next_data = p_next->data->load();
    //         if (p_next_data->key >= key) {
    //             break;
    //         }
    //         p = p_next;
    //     }
    // }

    // // p = p->next[0]->load();
    // p = p->next.get()->v[0]->load();
    // p_data = p->data->load();

    // if (unlikely(p_data->key != key)) {
    //     return Retcode::NotFound;
    // }
    // auto data_clone = *p_data;
    // data_clone.is_delete = true;
    // p->data->store(data_clone);
    // count_->store(*count_->load() - 1);
    return Retcode::Success;
}

SstMeta* SkipList::dump_disk(const fs::path& filepath, FILE* file) {
    return nullptr;
}

}  // namespace NAMESPACE::lsmtree
