#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <map>

#include "consts.hpp"
#include "lrucache.hpp"

namespace NAMESPACE::lsmtree {

template <typename K, typename V>
struct HashMapNode {
    K key;
    V value;
};

template <typename K, typename V>
class HashMap {
    using Node = HashMapNode<K, V>;
    using Bucket = LRUCache<K, V>;

public:
    explicit HashMap(int capacity) : capacity_(capacity) {
        buckets = new Bucket(0);
    }

    Retcode Get(K key, V* value) {
        if (!data.contains(key)) {
            return Retcode::NotFound;
        }

        auto ret = data[key];
        *value = ret;
        return Retcode::Found;
    }

    Retcode Set(K key, const V& value) {
        data.insert_or_assign(key, value);
        return Retcode::Success;
    }

private:
    Bucket* buckets;

    // TODO(kuriko): temp
    std::map<K, V> data;

    const int capacity_;
};

}  // namespace NAMESPACE::lsmtree
