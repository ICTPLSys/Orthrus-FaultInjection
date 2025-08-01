#include <algorithm>
#include <cstdint>
#include <cstring>

#include "compiler.hpp"
#include "context.hpp"
#include "ctltypes.hpp"
#include "custom_stl.hpp"
#include "namespace.hpp"
#include "ptr.hpp"

namespace NAMESPACE {
#include "closure.hpp"

using namespace ::scee;

hashmap_t::entry_t::entry_t(Key key, Val val, fixed_ptr_t<entry_t> next)
    : key(key), val_ptr(ptr_t<Val>::create(val)), next(next) {}

void hashmap_t::entry_t::destroy() const {
    if (val_ptr != nullptr) {
        destroy_obj(const_cast<Val *>(val_ptr->load()));
        val_ptr->destroy();
    }
}

void hashmap_t::entry_t::setv(Val val) const { val_ptr->store(val); }

const Val *hashmap_t::entry_t::getv() const { return val_ptr->load(); }

hashmap_t hashmap_t::make(size_t capacity) {
    hashmap_t hm;
    hm.capacity = capacity;
    if (capacity > 0) {
        mut_array_t<entry_t> zeros(nullptr, capacity);
        hm.buckets = ptr_t<mut_array_t<entry_t>>::create_fixed(zeros);
        hm.locks = mutable_list_t<mutex_t>::create(capacity);
    }
    return hm;
}

void hashmap_t::destroy() const {
    if (capacity > 0) {
        for (size_t i = 0; i < capacity; ++i) {
            const entry_t *entry = buckets.get()->v[i]->load();
            if (entry != nullptr) {
                entry = entry->next.get();
                while (entry != nullptr) {  // next will not be nullptr
                    const entry_t *enext = entry->next.get();
                    destroy_obj(const_cast<entry_t *>(entry));
                    entry = enext;
                }
            }
        }
        buckets.destroy();
        locks.destroy();
    }
}

const Val *hashmap_t::get(const Key &key) const {
    uint32_t hv = key.hash() % capacity;
    lock_guard_t guard(&locks.v[hv]);
    const entry_t *bucket = buckets.get()->v[hv]->load();
    while (bucket != nullptr) {
        if (bucket->key == key) {
            return bucket->getv();
        }
        bucket = bucket->next.get();
    }
    return nullptr;
}

RetType hashmap_t::set(const Key &key, const Val &val) const {
    uint32_t hv = key.hash() % capacity;
    lock_guard_t guard(&locks.v[hv]);
    const mut_array_t<entry_t> *_buckets = buckets.get();
    const entry_t *head = _buckets->v[hv]->load();
    const entry_t *bucket = head;
    while (bucket != nullptr) {
        if (bucket->key == key) {
            bucket->setv(val);
            return kStored;
        }
        bucket = bucket->next.get();
    }
    // originally, we are changing entry_t*, will become ptr_t.reref
    const entry_t *new_entry =
        ptr_t<entry_t>::make_obj(entry_t(key, val, fixed_ptr_t<entry_t>(head)));
    _buckets->v[hv]->reref(new_entry);
    return kCreated;
}

RetType hashmap_t::del(const Key &key) const {
    std::abort();
    /*
    uint32_t hv = key.hash() % capacity;
    // lock_guard_t guard(&locks.v[hv]);
    ptr_t<entry_t> *head = buckets.get()->v[hv];
    const entry_t *bucket = head->load();
    while (bucket != nullptr) {
        if (bucket->key == key) {
            const entry_t *next = bucket->next->load();
            head->reref(next);
            bucket->next->destroy();
            destroy_obj(const_cast<entry_t *>(bucket));
            return kDeleted;
        }
        head = bucket->next;
        bucket = head->load();
    }
    return kNotFound;
    */
}

const Val *hashmap_get(ptr_t<hashmap_t> *hmap, Key key) {
    return hmap->load()->get(key);
}

RetType hashmap_set(ptr_t<hashmap_t> *hmap, Key key, Val val) {
    return hmap->load()->set(key, val);
}

RetType hashmap_del(ptr_t<hashmap_t> *hmap, Key key) {
    return hmap->load()->del(key);
}

}  // namespace NAMESPACE
