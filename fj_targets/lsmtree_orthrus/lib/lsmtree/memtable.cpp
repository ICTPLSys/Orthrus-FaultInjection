#include <iostream>
#include <cstddef>
#include <tuple>
#include <cinttypes>
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

SkipListNode::SkipListNode(KeyT key, ValueT value, int lvl, const SkipListNode* nxt)
    : key(key), level(lvl) {
    data = ptr_t<Data>::create({ key, value, false, });
    memset(next, 0, sizeof(next));
    for (int i = 0; i <= lvl; i++) { next[i] = nxt; }
}

SkipListNode::SkipListNode(KeyT key, ValueT value, int lvl, const SkipListNode* nxts[])
    : key(key), level(lvl) {
    data = ptr_t<Data>::create({ key, value, false, });
    memset(next, 0, sizeof(next));
    for (int i = 0; i <= lvl; i++) { next[i] = nxts[i]; }
}

//=====================================================================
SkipList::SkipList() {
    level_ = ptr_t<int>::create(int(0));
    // level_ = new int(0);
    tail_ = new SkipListNode {SKIPLIST_KEY_INVALID, 0, 0, static_cast<SkipListNode*>(nullptr)};
    head_ = new SkipListNode {SKIPLIST_KEY_INVALID, 0, SKIPLIST_MAX_LEVEL, tail_};
    #if (LSMTREE_ENABLE_SKIPLIST_CACHE_ACCESS)
        for (int i = 0; i <= SKIPLIST_MAX_LEVEL; i++) {
            cache_access_[i] = scee::ptr_t<SkipListNode>::create(p_head);
        }
    #endif
}

const SkipListNode* SkipList::findCache(
    const SkipListNode* p, const Data* p_data, int level, KeyT key) const {
    #if (LSMTREE_ENABLE_SKIPLIST_CACHE_ACCESS)
        const auto* last_visit_p = cache_access_[level]->load();
        const auto* last_visit_p_data = last_visit_p->data->load();
        if (p_data->key <= last_visit_p_data->key && last_visit_p_data->key < key) {
            p = last_visit_p;
        }
    #endif
    return p;
}

void SkipList::updateCache(int level, const SkipListNode* node) const {
    #if (LSMTREE_ENABLE_SKIPLIST_CACHE_ACCESS)
        cache_access_[level]->reref(node);
    #endif
}

ValueT SkipList::Get(KeyT key) const {
    auto level = *level_->load();
    // auto level = *level_;

    const auto* p = head_;

    for (int i = level; i >= 0; i--) {
        while (true) {
            const auto* p_next = rt::cache_read(p->next[i]);
            if (p_next->key >= key) { break; }
            p = p_next;
        }
    }
    p = rt::cache_read(p->next[0]);
    auto p_data = p->data->load();
    // auto p_data = p->data;
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
            fprintf(stderr, "acc size: %zu\n", g_acc_cnts.size());
            // uint64_t sum = 0;
            // for (auto it : g_acc_cnts) { sum += it; }
            uint64_t sum = std::accumulate(g_acc_cnts.cbegin(), g_acc_cnts.cend(), 0);
            fprintf(stderr, "avg acc length: %lf\n", sum * 1.0 / g_acc_cnts.size());
        #endif
    }
} g_stat;

Retcode SkipList::Set(KeyT key, ValueT value) const {
    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        uint64_t now = rdtsc();
        uint64_t start_0 = now;
        uint64_t start = now;
    #endif

    SkipListNode const* update[SKIPLIST_MAX_LEVEL + 1];

    auto level = *level_->load();
    // auto level = *level_;

    // fprintf(stderr, "%s, level: %d\n", TO_STRING(NAMESPACE), level);
    const auto* p = head_;

    int acc_cnt = 0;
    for (int i = level; i >= 0; i--) {
        while (true) {
            acc_cnt += 1;
            // auto* p_next = p->next[i];
            auto* p_next = rt::cache_read(p->next[i]);
            // fprintf(stderr, "%s, p_next: %p\n", TO_STRING(NAMESPACE), p_next);
            if (p_next->key >= key) { break; }
            p = p_next;
        }
        update[i] = p;
    }
    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        g_stat.g_acc_cnts.emplace_back(acc_cnt + 1);
    #endif

    p = rt::cache_read(p->next[0]);
    // p = p->next[0];
    // fprintf(stderr, "%s, p: %p\n", TO_STRING(NAMESPACE), p);

    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        now = rdtsc();
        g_stat.time_for += now - start;
        start = now;
    #endif

    if (p->key == key) {
        p->data->store({ key, value, false, });

        #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
            now = rdtsc();
            g_stat.time_assign += now - start;
            g_stat.time_tot += now - start_0;
        #endif
        // fprintf(stderr, "%s, ret: %d\n", TO_STRING(NAMESPACE), Retcode::Update);
        return Retcode::Update;
    }
    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        now = rdtsc();
        g_stat.time_assign += now - start;
        start = now;
    #endif

    int lvl = SkipListNode::get_random_level();
    // fprintf(stderr, "%s, lvl: %d\n", TO_STRING(NAMESPACE), lvl);
    if (lvl > level) {
        lvl = ++level;
        level_->store(lvl);
        // *level_ = lvl;
        // fprintf(stderr, "%s, new lvl: %d\n", TO_STRING(NAMESPACE), lvl);
        update[lvl] = head_;
    }
    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        now = rdtsc();
        g_stat.time_random += now - start;
        start = now;
    #endif

    const SkipListNode* update_next[SKIPLIST_MAX_LEVEL + 1];
    for (int i = 0; i <= lvl; i++) {
        // const auto* ptr = update[i]->next[i];
        const auto* ptr = rt::cache_read(update[i]->next[i]);
        // fprintf(stderr, "%s, ptr: %p\n", TO_STRING(NAMESPACE), ptr);
        update_next[i] = ptr;
    }
    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        now = rdtsc();
        g_stat.time_update += now - start;
        start = now;
    #endif

    auto* new_node = scee::ptr_t<SkipListNode>::make_obj({
        key, value, lvl, update_next,
    });
    // SkipListNode* new_node = nullptr;
    // if (!rt::is_validator()) {
    //     new_node = new SkipListNode({
    //         key, value, lvl, update_next,
    //     });
    // }
    #if (LSMTREE_PROFILE_SKIPLIST_RDTSC)
        now = rdtsc();
        g_stat.time_malloc += now - start;
        start = now;
    #endif

    if constexpr (!rt::is_validator()) {
        // FIXME(kuriko): actually this should be a cache_write
        for (auto i = lvl; i >= 0; i--) {
            const auto* p = update[i];
            const_cast<SkipListNode*>(p)->next[i] = new_node;
        }
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
    // fprintf(stderr, "%s, ret: %d\n", TO_STRING(NAMESPACE), Retcode::Insert);
    return Retcode::Insert;
}

Retcode SkipList::Del(KeyT key) const {
    int const level = *level_->load();
    const SkipListNode* p = head_;

    for (int i = level; i >= 0; i--) {
        while (true) {
            auto* p_next = rt::cache_read(p->next[i]);
            if (p_next->key >= key) { break; }
            p = p_next;
        }
    }

    p = rt::cache_read(p->next[0]);

    const auto* p_data = p->data->load();
    if (unlikely(p_data->key != key)) {
        return Retcode::NotFound;
    }
    auto data_clone = *p_data;
    data_clone.is_delete = true;
    p->data->store(data_clone);
    return Retcode::Success;
}

SstMeta* SkipList::dump_disk(const fs::path& filepath, FILE* file) {
    return nullptr;
    // // SstMeta* sst_meta = new SstMeta{
    // //     .filepath = std::move(filepath),
    // //     .key_min = std::numeric_limits<KeyT>::max(),
    // //     .key_max = 0,
    // // };

    // // auto* sst_meta = scee::ptr_t<SstMeta>::create({
    // //     .filepath = std::move(filepath),
    // //     .key_min = std::numeric_limits<KeyT>::max(),
    // //     .key_max = 0,
    // // });

    // // FIXME(kuriko): this will cause memory leak.
    // auto* sst_meta = new SstMeta {
    //     .filepath = std::move(filepath),
    //     .key_min = std::numeric_limits<KeyT>::max(),
    //     .key_max = 0,
    // };

    // BlkInfo blk_info;
    // blk_info.blk_key_min = std::numeric_limits<KeyT>::max();
    // blk_info.blk_key_max = 0;
    // blk_info.offset_in_sst = 0;

    // // SkipListNode const* p = head_->load()->next[0]->load();
    // const SkipListNode* p = head_->load()->next.get()->v[0]->load();
    // const auto* p_tail = tail_->load();
    // while (p != p_tail) {
    //     const auto* p_data = p->data->load();
    //     auto key = p_data->key;

    //     blk_info.blk_key_min = std::min(blk_info.blk_key_min, key);
    //     blk_info.blk_key_max = std::max(blk_info.blk_key_max, key);
    //     blk_info.count++;
    //     FilterSetKey(blk_info.filter, key);

    //     rt::fwrite(static_cast<void const*>(&p->data), sizeof(Data), 1, file);

    //     // const auto* p_next = p->next[0]->load();
    //     const auto* p_next = p->next.get()->v[0]->load();
    //     if (blk_info.count >= BLK_CACHE_MAX_COUNT || p_next == p_tail) {
    //         // KDEBUG(
    //         //     "new blkinfo: id: %lu, offset_in_sst %u, blksz %u, min: "
    //         //     "%ld, max: %ld",
    //         //     sst_meta->blk_infos.size(), blk_info.offset_in_sst,
    //         //     blk_info.count, blk_info.blk_key_min,
    //         //     blk_info.blk_key_max);

    //         sst_meta->key_min =
    //             std::min(sst_meta->key_min, blk_info.blk_key_min);
    //         sst_meta->key_max =
    //             std::max(sst_meta->key_max, blk_info.blk_key_max);

    //         sst_meta->blk_infos.emplace_back(blk_info);

    //         blk_info = BlkInfo();
    //         blk_info.offset_in_sst = static_cast<int>(rt::ftell(file));
    //     }
    //     p = p_next;
    // }

    // auto* ret = rt::cache_read(sst_meta);

    // return ret;
}

}  // namespace NAMESPACE::lsmtree
