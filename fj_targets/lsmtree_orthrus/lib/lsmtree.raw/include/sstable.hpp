#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <map>
#include <vector>

#include "cache.hpp"
#include "consts.hpp"
#include "hash.hpp"

namespace NAMESPACE::lsmtree {

namespace fs = std::filesystem;

class TblCache {
public:
    TblCache() = delete;
    TblCache(const TblCache&) = delete;
    TblCache& operator=(const TblCache&) = delete;
    TblCache(TblCache&&) = delete;
    TblCache& operator=(TblCache&&) = delete;

public:
    explicit TblCache(const fs::path& sst_dir)
        : sst_dir_(std::move(sst_dir)), blk_cache_(BLK_CACHE_MAX_SIZE) {}

    Retcode Get(KeyT key, ValueT* value) {
        for (int sst_id = static_cast<int>(sst_metas_.size()) - 1; sst_id >= 0;
             sst_id--) {
            auto* p_sst_meta = sst_metas_[sst_id];

            if (key < p_sst_meta->key_min || key > p_sst_meta->key_max) {
                continue;
            }

            std::vector<BlkInfo>& blk_infos = p_sst_meta->blk_infos;

            // Binary Search the blk
            int l = 0;
            int r = static_cast<int>(blk_infos.size()) - 1;
            int blk_id = -1;
            while (l <= r) {
                const int mid = (l + r) / 2;
                if (key < blk_infos[mid].blk_key_min) {
                    r = mid - 1;
                } else if (blk_infos[mid].blk_key_max < key) {
                    l = mid + 1;
                } else {
                    blk_id = mid;
                    break;
                }
            }

            if (blk_id == -1) {
                continue;
            }

            // Use bloom hash to quick check whether the key inside the sst blk
            BlkInfo& blk_info = blk_infos[blk_id];
            if (!IsFilterSetKey(blk_info.filter, key)) {
                continue;
            }

            // Load the blk into cache
            if (!blk_info.in_cache) {
                // TODO(kuriko): implement this
                CacheKey const cache_key = hash_key(sst_id, blk_id);
                LoadBlkToCache(cache_key, p_sst_meta, &blk_info);
            }

            // Load data from cache
            BlkData* blk_data = nullptr;
            auto cache_key = hash_key(sst_id, blk_id);
            blk_cache_.Get(cache_key, &blk_data);

            #ifdef false
                auto view = std::span(blk_data->data, blk_data->count);
                auto it = std::ranges::lower_bound(
                    view, key, {}, [](const Data& data) { return data.key; });
                if (it == view.end() || it->key != key) {
                    // KDEBUG("Not Found in Current Block");
                    continue;
                }
            #else
                auto it = std::lower_bound(
                        blk_data->data,
                        blk_data->data + blk_data->count,
                        key,
                        [](const Data& data, const int& key) {
                            return data.key < key;
                        }
                    );

                if (it >= blk_data->data + key || it->key != key) {
                    // KDEBUG("Not Found in Current Block");
                    continue;
                }
            #endif

            auto is_delete = it->is_delete;

            if (is_delete) {
                *value = -1;
                return Retcode::NotFound;
            }

            auto val = it->value;
            *value = val;

            return Retcode::Found;
        }

        return Retcode::NotFound;
    }

    void Add(MemTable* tbl) {
        char sst_file_name[255];
        sprintf(sst_file_name, "sst_%03lu.bin", sst_num);
        // KDEBUG("Flush %p => %s", tbl, sst_file_name);

        fs::path filepath = sst_dir_ / sst_file_name;
        FILE* file = fopen(filepath.c_str(), "wb+");

        SstMeta* sst_meta = tbl->dump_disk(filepath, file);

        // TODO(kuriko): reclaim memory.
        // delete tbl;

        sst_metas_.emplace_back(sst_meta);
        sst_files_[sst_meta] = file;
    }

private:
    static CacheKey hash_key(int sst_id, int blk_id) {
        // MYASSERT(blk_id < 0xFFFFFFFFFFFF);
        // 64 = 16 + 48
        // aka sst_id < 2**16, blk_id < 2**48
        return (static_cast<CacheKey>(sst_id) << 48) | blk_id;
    }

    void LoadBlkToCache(CacheKey cache_key, SstMeta* sst_meta,
                        BlkInfo* blk_info) {
        FILE* file = sst_files_[sst_meta];
        MYASSERT(file != nullptr);

        fseek(file, blk_info->offset_in_sst, SEEK_SET);

        Data* data = new Data[blk_info->count];
        fread(data, sizeof(Data), blk_info->count, file);

        auto* blk_data = new BlkData{
            .blk_info = blk_info,
            .data = data,
            .count = blk_info->count,
        };

        blk_cache_.Set(cache_key, blk_data);

        blk_info->in_cache = true;
    }

private:
    const fs::path sst_dir_;

    size_t sst_num;
    std::vector<SstMeta*> sst_metas_;

    std::map<SstMeta*, FILE*> sst_files_;

    // TODO(kuriko): use scee compatible ds
    HashMap<CacheKey, BlkData*> blk_cache_;
};

}  // namespace NAMESPACE::lsmtree
