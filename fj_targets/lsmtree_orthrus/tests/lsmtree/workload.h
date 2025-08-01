#pragma once
#include "dist.h"
#include <iostream>
#include <platform/assert.h>
#include <platform/lock.h>
#include <db.h>
#include <memory>

struct WorkloadConfig {
    enum {
        kTotalRatio = 100,
    };
    int read_ratio;
    int scan_ratio;
    int insert_ratio;
    int update_ratio;
    int readmodifywrite_ratio;
    int max_scan_length;
    const char* request_dist;
    const char* scan_length_dist;
};

static const WorkloadConfig kWorkloadAConfig = {
    .read_ratio = 50,
    .scan_ratio = 0,
    .insert_ratio = 50,
    .update_ratio = 0,
    .readmodifywrite_ratio = 0,
    .max_scan_length = 0,
    .request_dist = "scrambled_zipfian",
    .scan_length_dist = nullptr,
};

static const WorkloadConfig kWorkloadBConfig = {
    .read_ratio = 95,
    .scan_ratio = 0,
    .insert_ratio = 0,
    .update_ratio = 5,
    .readmodifywrite_ratio = 0,
    .max_scan_length = 0,
    .request_dist = "scrambled_zipfian",
    .scan_length_dist = nullptr,
};

static const WorkloadConfig kWorkloadCConfig = {
    .read_ratio = 100,
    .scan_ratio = 0,
    .insert_ratio = 0,
    .update_ratio = 0,
    .readmodifywrite_ratio = 0,
    .max_scan_length = 0,
    .request_dist = "scrambled_zipfian",
    .scan_length_dist = nullptr,
};

static const WorkloadConfig kWorkloadDConfig = {
    .read_ratio = 95,
    .scan_ratio = 0,
    .insert_ratio = 5,
    .update_ratio = 0,
    .readmodifywrite_ratio = 0,
    .max_scan_length = 0,
    .request_dist = "latest",
    .scan_length_dist = nullptr,
};

static const WorkloadConfig kWorkloadEConfig = {
    .read_ratio = 0,
    .scan_ratio = 95,
    .insert_ratio = 5,
    .update_ratio = 0,
    .readmodifywrite_ratio = 0,
    .max_scan_length = 100,
    .request_dist = "scrambled_zipfian",
    .scan_length_dist = "uniform",
};

static const WorkloadConfig kWorkloadFConfig = {
    .read_ratio = 50,
    .scan_ratio = 0,
    .insert_ratio = 0,
    .update_ratio = 0,
    .readmodifywrite_ratio = 50,
    .max_scan_length = 0,
    .request_dist = "scrambled_zipfian",
    .scan_length_dist = nullptr,
};

template <typename K, typename V>
class Workload {
public:
    Workload(const WorkloadConfig& config, int record_count,
        int operation_count, std::vector<K>& keys)
        : config_(config)
        , keys_(keys)
        , inserted_(record_count)
    {
        MYASSERT(config_.read_ratio + config_.scan_ratio + config_.insert_ratio + config_.update_ratio + config_.readmodifywrite_ratio == WorkloadConfig::kTotalRatio);
        MYASSERT(keys.size() == (size_t)record_count);
        op_dis_.reset(make_generator("uniform", WorkloadConfig::kTotalRatio));
        if (config.scan_ratio > 0) {
            MYASSERT(config.max_scan_length > 0);
            scan_len_dis_.reset(make_generator(config_.scan_length_dist,
                config_.max_scan_length));
        }
        int expect_insert = 0;
        if (config.insert_ratio > 0) {
            expect_insert = (int)(operation_count * config_.insert_ratio / (double)WorkloadConfig::kTotalRatio * 2.0); // 2 is fudge factor
            keys_.reserve(record_count + expect_insert);
        }

        if (std::string(config_.request_dist) == "latest") {
            req_dis_.reset(new LatestGenerator(record_count));
        } else {
            req_dis_.reset(make_generator(config_.request_dist,
                record_count + expect_insert));
        }
    }

    template <typename KeyGenerator, typename ValGenerator>
    void DoTransaction(Database<K, V>& db, KeyGenerator& key_gen,
        ValGenerator& val_gen)
    {
        int r = op_dis_->operator()();
        if (r < config_.read_ratio) {
            DoRead(db);
        } else if (r < config_.read_ratio + config_.scan_ratio) {
            DoScan(db);
        } else if (r < config_.read_ratio + config_.scan_ratio + config_.insert_ratio) {
            DoInsert(db, key_gen, val_gen);
        } else if (r < config_.read_ratio + config_.scan_ratio + config_.insert_ratio + config_.update_ratio) {
            DoUpdate(db, val_gen);
        } else {
            DoRmw(db);
        }
    }

    void DoRead(Database<K, V>& db)
    {
        int idx = req_dis_->NextValue(inserted_.load(std::memory_order_acquire));
        K key = keys_[idx];
        V val;
        int r = db.Read(key, &val);
        MYASSERT(r == 0);
    }

    void DoScan(Database<K, V>& db)
    {
        int max_idx = inserted_.load(std::memory_order_acquire);
        int idx = req_dis_->NextValue(max_idx);
        int len = std::min(max_idx - idx, scan_len_dis_->operator()() + 1);
        std::vector<K> keys;
        std::vector<V> vals;
        int r = db.Scan(keys_[idx], len, keys, vals);
        MYASSERT(r == 0);
    }

    template <typename KeyGenerator, typename ValGenerator>
    void DoInsert(Database<K, V>& db, KeyGenerator& key_gen,
        ValGenerator& val_gen)
    {
        K key = key_gen();
        V val = val_gen();
        insert_lock_.Lock();
        keys_.push_back(key);
        insert_lock_.Unlock();
        int r = db.Insert(key, val);
        MYASSERT(r == 0);
        inserted_.fetch_add(1, std::memory_order_release);
    }

    template <typename ValGenerator>
    void DoUpdate(Database<K, V>& db, ValGenerator& val_gen)
    {
        int idx = req_dis_->NextValue(inserted_.load(std::memory_order_acquire));
        K key = keys_[idx];
        V val = val_gen();
        int r = db.Update(key, val);
        MYASSERT(r == 0);
    }

    void DoRmw(Database<K, V>& db)
    {
        int idx = req_dis_->NextValue(inserted_.load(std::memory_order_acquire));
        K key = keys_[idx];
        int r = db.ReadModifyWrite(
            key, [](V val, void* arg) { return val + 1; }, nullptr);
        MYASSERT(r == 0);
    }

    // private:
    const WorkloadConfig config_;
    std::vector<K>& keys_;
    std::unique_ptr<Generator> op_dis_;
    std::unique_ptr<Generator> req_dis_;
    std::unique_ptr<Generator> scan_len_dis_;
    std::atomic_int inserted_;
    scee::utils::SpinLock insert_lock_;
};
