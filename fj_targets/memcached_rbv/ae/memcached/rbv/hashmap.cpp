#include "hashmap.hpp"
#include "rbv.hpp"

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

struct ordered_guard_t {
    ordered_mutex_t *mtx;
    ordered_guard_t(ordered_mutex_t *mtx) : mtx(mtx) {
        pthread_mutex_lock(&mtx->mtx);
        rbv::hasher.checkorder(mtx->order);
    }
    ~ordered_guard_t() {
        rbv::hasher.checkorder(mtx->order);
        pthread_mutex_unlock(&mtx->mtx);
    }
} ;

hashmap_t *hashmap_t::make(size_t capacity) {
    hashmap_t *hm = (hashmap_t *)malloc(sizeof(hashmap_t));
    hm->capacity = capacity;
    if (capacity > 0) {
        hm->buckets = (entry_t **)malloc(capacity * sizeof(entry_t *));
        memset(hm->buckets, 0, capacity * sizeof(entry_t *));
        hm->locks =
            (ordered_mutex_t *)malloc(capacity * sizeof(ordered_mutex_t));
        for (size_t i = 0; i < capacity; ++i) {
            pthread_mutex_init(&hm->locks[i].mtx, nullptr);
            hm->locks[i].order = 0;
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
        free((void *)buckets);
        for (size_t i = 0; i < capacity; ++i) {
            pthread_mutex_destroy(&locks[i].mtx);
        }
        free(locks);
    }
    free(this);
}

const Val *hashmap_t::get(const Key &key) {
    uint32_t hv = key.hash() % capacity;
    ordered_guard_t guard(&locks[hv]);
    entry_t *bucket = buckets[hv];
    while (bucket != nullptr) {
        rbv::hasher.combine(std::string(bucket->key.ch, KEY_LEN));
        if (bucket->key == key) {
            rbv::hasher.combine(std::string(bucket->getv()->ch, VAL_LEN));
            return bucket->getv();
        }
        bucket = bucket->next;
    }
    return nullptr;
}

RetType hashmap_t::set(const Key &key, const Val &val) {
    uint32_t hv = key.hash() % capacity;
    ordered_guard_t guard(&locks[hv]);
    entry_t *bucket = buckets[hv];
    while (bucket != nullptr) {
        rbv::hasher.combine(std::string(bucket->key.ch, KEY_LEN));
        if (bucket->key == key) {
            rbv::hasher.combine(std::string(bucket->getv()->ch, VAL_LEN));
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
    rbv::hasher.combine(std::string(new_entry->key.ch, KEY_LEN));
    rbv::hasher.combine(std::string(new_entry->getv()->ch, VAL_LEN));
    buckets[hv] = new_entry;
    rbv::hasher.combine(std::string(buckets[hv]->key.ch, KEY_LEN));
    return kCreated;
}

RetType hashmap_t::del(const Key &key) {
    uint32_t hv = key.hash() % capacity;
    ordered_guard_t guard(&locks[hv]);
    entry_t **bucket = &buckets[hv];
    while (*bucket != nullptr) {
        rbv::hasher.combine(std::string((*bucket)->key.ch, KEY_LEN));
        if ((*bucket)->key == key) {
            rbv::hasher.combine(std::string((*bucket)->getv()->ch, VAL_LEN));
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
