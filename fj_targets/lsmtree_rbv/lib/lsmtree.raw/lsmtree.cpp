#include "lsmtree.hpp"

#include "cache.hpp"
#include "consts.hpp"
#include "memtable.hpp"
#include "sstable.hpp"


namespace NAMESPACE {
using namespace lsmtree;

__attribute__((noinline)) void sim_mutex(void* lsm) {
    volatile int i = 0;
    i += 1;
    return;
}

int lsmtree_set(void *lsm, int64_t k, int64_t v) {
    sim_mutex(lsm);
    auto *tmp = reinterpret_cast<lsmtree::LSMTree *>(lsm);
    auto ret = (int)tmp->Set(k, v);
    return ret;
}

int64_t lsmtree_get(void *lsm, int64_t k) {
    sim_mutex(lsm);
    auto *tmp = reinterpret_cast<lsmtree::LSMTree *>(lsm);
    return tmp->Get(k);
}

int lsmtree_del(void *lsm, int64_t k) {
    sim_mutex(lsm);
    auto *tmp = reinterpret_cast<lsmtree::LSMTree *>(lsm);
    return (int)tmp->Del(k);
}

const InternalStates& lsmtree_get_internal_states(void* lsm) {
    auto *tmp = reinterpret_cast<lsmtree::LSMTree *>(lsm);
    return tmp->GetInternalStates();
}

__attribute__((noinline)) double get_time_info(int64_t v) {
    double ret = 0;
    ret += (double)(v / 24.0 / 60.0 / 60.0);
    ret += (double)(v / 60.0 / 60.0);
    ret += (double)(v / 60.0);
    ret += (double)(v % 60);
    return ret;
}
double lsmtree_get_time_info(void* lsm, int64_t k) {
    auto *tmp = reinterpret_cast<lsmtree::LSMTree *>(lsm);
    auto v = tmp->Get(k);
    return get_time_info(v);
}

uint64_t lsmtree_get_crc(void* lsm, int64_t k) {
    auto *tmp = reinterpret_cast<lsmtree::LSMTree *>(lsm);
    auto v = tmp->Get(k);
    return lsmtree::Data::calc_crc(k, v, 0);
}


}


namespace NAMESPACE::lsmtree {

__attribute__((noinline)) void sim_mutex2() {
    volatile int i = 0;
    i += 1;
    return;
}

LSMTree::LSMTree(const fs::path& sst_dir)
    : sst_dir_(sst_dir), tbl_cache_(sst_dir) {
    create_directories(sst_dir);

    mem_table_ = new MemTable();
    immu_mem_table_ = nullptr;
}

ValueT LSMTree::Get(KeyT key) {
    MYASSERT(mem_table_ != nullptr);

    sim_mutex2();
    auto *p_tbl = mem_table_;
    if (auto ret = p_tbl->Get(key); ret != -1) {
        return ret;
    }

    // const auto *p_immu_tbl = immu_mem_table_;
    // if (p_immu_tbl == nullptr) {
    //     return -1;
    // }

    // if (auto ret = p_immu_tbl->Get(key); ret != -1) {
    //     return ret;
    // }

    return -1;
}

Retcode LSMTree::Set(KeyT key, ValueT value) {
    sim_mutex2();
    auto *p_tbl = mem_table_;
    auto ret = p_tbl->Set(key, value);
    return ret;
}


Retcode LSMTree::Del(KeyT key) {
    sim_mutex2();
    auto *p_tbl = mem_table_;
    return p_tbl->Del(key);
}

void LSMTree::ImmuMemTblFlush() {
    auto* p_immu_tbl = immu_mem_table_;
    if (p_immu_tbl == nullptr) return;
}

void LSMTree::MemTblSwap() {
    auto* p_tbl = mem_table_;
    auto* p_immu_tbl = immu_mem_table_;
    if (p_immu_tbl != nullptr) {
        ImmuMemTblFlush();
    }
    immu_mem_table_ = p_tbl;
    mem_table_ = new MemTable();
}

}  // namespace NAMESPACE::lsmtree
