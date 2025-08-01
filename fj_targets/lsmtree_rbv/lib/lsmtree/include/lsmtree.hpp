#pragma once

#include "memtable.hpp"
#include "runtime.hpp"
#include "sstable.hpp"

namespace NAMESPACE::lsmtree {

namespace fs = std::filesystem;
class LSMTree {
public:
    LSMTree() = delete;
    LSMTree(const LSMTree&) = delete;
    LSMTree& operator=(const LSMTree&) = delete;
    LSMTree(LSMTree&&) = delete;
    LSMTree& operator=(LSMTree&&) = delete;

public:
    explicit LSMTree(const fs::path& sst_dir);

    ValueT Get(KeyT key);

    Retcode Set(KeyT key, ValueT value);

    Retcode Del(KeyT key);

    // void BulkLoad(std::span<KeyT> keys, std::span<ValueT> values) {
    //     MYASSERT(keys.size() == values.size());
    //     // FIXME(kuriko): use some real bulkload rather than insert one by
    //     one. for (int i = 0; i < keys.size(); i++) {
    //         Set(keys[i], values[i]);
    //     }
    // }

    // void BulkLoad(std::span<std::tuple<KeyT, ValueT>> data) {
    //     // FIXME(kuriko): use some real bulkload rather than insert one by
    //     one. for (auto [key, value] : data) {
    //         Set(key, value);
    //     }
    // }

private:
    void ImmuMemTblFlush();

    void MemTblSwap();

private:
    const fs::path sst_dir_;

    scee::ptr_t<MemTable>* mem_table_;
    scee::ptr_t<MemTable>* immu_mem_table_;
    // MemTable* mem_table_;
    // MemTable* immu_mem_table_;

    TblCache tbl_cache_;
};

}  // namespace NAMESPACE::lsmtree
