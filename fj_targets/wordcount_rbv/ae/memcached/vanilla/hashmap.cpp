#include "hashmap.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>

hashmap_t::entry_t::entry_t(Key key, Val *val, entry_t *next)
    : key(key), val_ptr(val), next(next) {}

void hashmap_t::entry_t::destroy() {
    if (val_ptr != nullptr) {
        free(val_ptr);
    }
}

void hashmap_t::entry_t::setv(Val val) {
    Val *new_val = (Val *)malloc(sizeof(Val));
    memcpy(new_val, &val, sizeof(Val));
    free(val_ptr);
    val_ptr = new_val;
}

const Val *hashmap_t::entry_t::getv() { return val_ptr; }

hashmap_t *hashmap_t::make(size_t capacity) {
    hashmap_t *hm = (hashmap_t *)malloc(sizeof(hashmap_t));
    hm->capacity = capacity;
    if (capacity > 0) {
        hm->buckets = (entry_t **)malloc(capacity * sizeof(entry_t *));
        memset(hm->buckets, 0, capacity * sizeof(entry_t *));
        hm->locks =
            (pthread_mutex_t *)malloc(capacity * sizeof(pthread_mutex_t));
        for (size_t i = 0; i < capacity; ++i) {
            pthread_mutex_init(&hm->locks[i], nullptr);
        }
    }
    return hm;
}

void hashmap_t::destroy() {
    if (capacity > 0) {
        for (size_t i = 0; i < capacity; ++i) {
            entry_t *entry = buckets[i];
            while (entry != nullptr) {  // next will not be nullptr
                entry_t *enext = entry->next;
                free(entry);
                entry = enext;
            }
        }
        free(buckets);
        for (size_t i = 0; i < capacity; ++i) {
            pthread_mutex_destroy(&locks[i]);
        }
        free(locks);
    }
    free(this);
}

const Val *hashmap_t::get(const Key &key) {
    uint32_t hv = key.hash() % capacity;
    lock_guard_t guard(&locks[hv]);
    entry_t *bucket = buckets[hv];
    while (bucket != nullptr) {
        if (bucket->key == key) {
            return bucket->getv();
        }
        bucket = bucket->next;
    }
    return nullptr;
}

RetType hashmap_t::set(const Key &key, const Val &val) {
    uint32_t hv = key.hash() % capacity;
    lock_guard_t guard(&locks[hv]);
    entry_t *bucket = buckets[hv];
    while (bucket != nullptr) {
        if (bucket->key == key) {
            bucket->setv(val);
            return kStored;
        }
        bucket = bucket->next;
    }
    entry_t *new_entry = (entry_t *)malloc(sizeof(entry_t));
    new_entry->key = key;
    Val *val_ptr = (Val *)malloc(sizeof(Val));
    memcpy(val_ptr, &val, sizeof(Val));
    new_entry->val_ptr = val_ptr;
    new_entry->next = buckets[hv];
    buckets[hv] = new_entry;
    return kCreated;
}

RetType hashmap_t::del(const Key &key) {
    uint32_t hv = key.hash() % capacity;
    lock_guard_t guard(&locks[hv]);
    entry_t **bucket = &buckets[hv];
    while (*bucket != nullptr) {
        if ((*bucket)->key == key) {
            entry_t *old = *bucket;
            *bucket = (*bucket)->next;
            free(old);
            return kDeleted;
        }
        bucket = &(*bucket)->next;
    }
    return kNotFound;
}

const Val *hashmap_get(hashmap_t *hmap, Key key) { return hmap->get(key); }

RetType hashmap_set(hashmap_t *hmap, Key key, Val val) {
    return hmap->set(key, val);
}

RetType hashmap_del(hashmap_t *hmap, Key key) { return hmap->del(key); }
