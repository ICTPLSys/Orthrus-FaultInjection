#include "hashmap.hpp"
#include "rbv.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <immintrin.h>

__attribute__((noinline)) void sim_mutex(){
    volatile int x = 0;
    x++;
}

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
    sim_mutex();
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
    sim_mutex();
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
    sim_mutex();
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


appfj_hashmap_t::entry_t::entry_t(Key key, Val *val, entry_t *next)
    : key(key), val_ptr(val), next(next) {}

void appfj_hashmap_t::entry_t::destroy() {
    if (val_ptr != nullptr) {
        free(val_ptr);
    }
}

void appfj_hashmap_t::entry_t::setv(Val val) {
    sim_mutex();
    Val *new_val = (Val *)malloc(sizeof(Val));
    memcpy(new_val, &val, sizeof(Val));
    free(val_ptr);
    val_ptr = new_val;
}

const Val *appfj_hashmap_t::entry_t::getv() { 
    sim_mutex();
    return val_ptr; }

appfj_hashmap_t *appfj_hashmap_t::make(size_t capacity) {
    appfj_hashmap_t *hm = (appfj_hashmap_t *)malloc(sizeof(appfj_hashmap_t));
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

void appfj_hashmap_t::destroy() {
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

const Val *appfj_hashmap_t::get(const Key &key) {
    uint32_t hv = key.hash() % capacity;
    sim_mutex();
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

RetType appfj_hashmap_t::set(const Key &key, const Val &val) {
    uint32_t hv = key.hash() % capacity;
    sim_mutex();
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

RetType appfj_hashmap_t::del(const Key &key) {
    sim_mutex();
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

const Val *appfj_hashmap_get(appfj_hashmap_t *hmap, Key key) { 
    sim_mutex();
    return hmap->get(key); }

RetType appfj_hashmap_set(appfj_hashmap_t *hmap, Key key, Val val) {
    sim_mutex();
    return hmap->set(key, val);
}

__attribute__((noinline)) __attribute__((target("sse4.2"))) uint32_t appfj_calculate_crc32(
    const void *data, std::size_t length) {
    std::uint32_t crc = ~0U;
    const auto *buffer = static_cast<const unsigned char *>(data);

    while (length > 0 && reinterpret_cast<std::uintptr_t>(buffer) % 8 != 0) {
        crc = _mm_crc32_u8(crc, *buffer);
        buffer++;
        length--;
    }

    const auto *buffer64 = reinterpret_cast<const std::uint64_t *>(buffer);
    while (length >= 8) {
        crc = _mm_crc32_u64(crc, *buffer64);
        buffer64++;
        length -= 8;
    }

    buffer = reinterpret_cast<const unsigned char *>(buffer64);

    if (length >= 4) {
        auto value32 = *reinterpret_cast<const std::uint32_t *>(buffer);
        crc = _mm_crc32_u32(crc, value32);
        buffer += 4;
        length -= 4;
    }

    if (length >= 2) {
        auto value16 = *reinterpret_cast<const std::uint16_t *>(buffer);
        crc = _mm_crc32_u16(crc, value16);
        buffer += 2;
        length -= 2;
    }

    if (length > 0) {
        crc = _mm_crc32_u8(crc, *buffer);
    }

    return ~crc;
}

__attribute__((noinline)) __attribute__((target("sse4.2"))) uint32_t calculate_crc32_normal(
    const void *data, std::size_t length) {
    std::uint32_t crc = ~0U;
    const auto *buffer = static_cast<const unsigned char *>(data);

    while (length > 0 && reinterpret_cast<std::uintptr_t>(buffer) % 8 != 0) {
        crc = _mm_crc32_u8(crc, *buffer);
        buffer++;
        length--;
    }

    const auto *buffer64 = reinterpret_cast<const std::uint64_t *>(buffer);
    while (length >= 8) {
        crc = _mm_crc32_u64(crc, *buffer64);
        buffer64++;
        length -= 8;
    }

    buffer = reinterpret_cast<const unsigned char *>(buffer64);

    if (length >= 4) {
        auto value32 = *reinterpret_cast<const std::uint32_t *>(buffer);
        crc = _mm_crc32_u32(crc, value32);
        buffer += 4;
        length -= 4;
    }

    if (length >= 2) {
        auto value16 = *reinterpret_cast<const std::uint16_t *>(buffer);
        crc = _mm_crc32_u16(crc, value16);
        buffer += 2;
        length -= 2;
    }

    if (length > 0) {
        crc = _mm_crc32_u8(crc, *buffer);
    }

    return ~crc;
}

uint32_t appfj_hashmap_get_crc(const Val *val) { 
    return appfj_calculate_crc32(val, VAL_LEN);
 }


 uint32_t hashmap_get_crc(const Val *val) { 
    return calculate_crc32_normal(val, VAL_LEN);
 }

RetType appfj_hashmap_del(appfj_hashmap_t *hmap, Key key) { return hmap->del(key); }
// appfj namespace implementations
