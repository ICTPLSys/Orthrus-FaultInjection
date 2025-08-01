#include "lsmtree.hpp"

#include "cache.hpp"
#include "consts.hpp"
#include "memtable.hpp"
#include "sstable.hpp"


namespace NAMESPACE {
using namespace lsmtree;
int lsmtree_set(void *lsm, int64_t k, int64_t v) {
    auto *tmp = reinterpret_cast<lsmtree::LSMTree *>(lsm);
    auto ret = (int)tmp->Set(k, v);
    return ret;
}

int64_t lsmtree_get(void *lsm, int64_t k) {
    auto *tmp = reinterpret_cast<lsmtree::LSMTree *>(lsm);
    return tmp->Get(k);
}

int lsmtree_del(void *lsm, int64_t k) {
    auto *tmp = reinterpret_cast<lsmtree::LSMTree *>(lsm);
    return (int)tmp->Del(k);
}
}


namespace NAMESPACE::lsmtree {

LSMTree::LSMTree(const fs::path& sst_dir)
    : sst_dir_(sst_dir), tbl_cache_(sst_dir) {
    create_directories(sst_dir);

    mem_table_ = scee::ptr_t<MemTable>::create(MemTable());
    immu_mem_table_ = scee::ptr_t<MemTable>::create();
    // mem_table_ = new MemTable();
    // immu_mem_table_ = nullptr;
}

ValueT LSMTree::Get(KeyT key) {
    MYASSERT(mem_table_ != nullptr);

    const auto *p_tbl = mem_table_->load();
    // const auto *p_tbl = mem_table_;
    if (auto ret = p_tbl->Get(key); ret != -1) {
        // KDEBUG("mem_table found");
        return ret;
    }

    const auto *p_immu_tbl = immu_mem_table_->load();
    // auto *p_immu_tbl = immu_mem_table_;
    if (p_immu_tbl == nullptr) {
        return -1;
    }

    if (auto ret = p_immu_tbl->Get(key); ret != -1) {
        // KDEBUG("immu_mem_table found");
        return ret;
    }

    // TODO(kuriko): disable filesystem access
    // if (auto ret = tbl_cache_.Get(key); ret != -1) {
    //     // KDEBUG("cache found");
    //     return ret;
    // }

    // KDEBUG("Not Found Key: %ld", key);
    return -1;
}

Retcode LSMTree::Set(KeyT key, ValueT value) {
    const auto *p_tbl = mem_table_->load();
    // auto *p_tbl = mem_table_;
    // if (unlikely(p_tbl->mem_size() > MEMTABLE_MAX_SIZE)) {
    //     MemTblSwap();
    //     p_tbl = mem_table_->load();
    // }
    auto ret = p_tbl->Set(key, value);
    return ret;
}


Retcode LSMTree::Del(KeyT key) {
    const auto *p_tbl = mem_table_->load();
    // auto *p_tbl = mem_table_;
    return p_tbl->Del(key);
}

void LSMTree::ImmuMemTblFlush() {
    // auto* p_immu_tbl = const_cast<MemTable*>(immu_mem_table_->load());
    auto* p_immu_tbl = immu_mem_table_;
    if (p_immu_tbl == nullptr) return;
    // FIXME(kuriko): disable memdump
    // tbl_cache_.Add(p_immu_tbl);
}

void LSMTree::MemTblSwap() {
    const auto* p_tbl = mem_table_->load();
    const auto* p_immu_tbl = immu_mem_table_->load();
    // auto* p_tbl = mem_table_;
    // auto* p_immu_tbl = immu_mem_table_;
    if (p_immu_tbl != nullptr) {
        // KDEBUG("flush immu memtable");
        ImmuMemTblFlush();
    }
    // KDEBUG("switching memtable");
    immu_mem_table_->reref(p_tbl);
    // immu_mem_table_ = p_tbl;

    // KDEBUG("create new memtable");
    mem_table_->store(MemTable());
    // mem_table_= new MemTable();
}

}  // namespace NAMESPACE::lsmtree
