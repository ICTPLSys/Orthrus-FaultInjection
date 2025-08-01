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

hashmap_t hashmap_t::make(size_t capacity) {
    hashmap_t hm;
    hm.capacity = capacity;
    if (capacity > 0) {
        mut_array_t<entry_t> zeros(nullptr, capacity);
        hm.buckets = ptr_t<mut_array_t<entry_t>>::create_fixed(zeros);
        hm.locks = mutable_list_t<mutex_t>::create(capacity);
        hm.size = sizeof(hashmap_t);
    }
    return hm;
}

void hashmap_t::destroy() const {
    if (capacity > 0) {
        for (size_t i = 0; i < capacity; ++i) {
            const entry_t *entry = buckets.get()->v[i]->load();
            if (entry != nullptr) {
                const entry_t *enext = entry->next->load();
                entry->next->destroy();
                entry = enext;
                while (entry != nullptr) {  // next will not be nullptr
                    const entry_t *enext = entry->next->load();
                    entry->next->destroy();
                    destroy_obj(const_cast<entry_t *>(entry));
                    entry = enext;
                }
            }
        }
        buckets.destroy();
        locks.destroy();
    }
}

std::string hashmap_t::get(const Key &key) const {
    uint32_t hv = key.hash() % capacity;
    lock_guard_t guard(&locks.v[hv]);
    const entry_t *bucket = buckets.get()->v[hv]->load();
    while (bucket != nullptr) {
        if (bucket->key == key) {
            return kRetVals[kValue] + bucket->val.to_string() + kCrlf;
        }
        bucket = bucket->next->load();
    }
    return kRetVals[kEnd];
}

std::string hashmap_t::set(const Key &key, const Val &val) const {
    uint32_t hv = key.hash() % capacity;
    scee::lock_guard_t guard(&locks.v[hv]);
    const mut_array_t<entry_t> *_buckets = buckets.get();
    ptr_t<entry_t> *head = _buckets->v[hv];
    const entry_t *bucket = head->load(), *head_ptr = bucket;
    while (bucket != nullptr) {
        if (bucket->key == key) {
            head->store(entry_t(key, val, bucket->next));
            return kRetVals[kStored];
        }
        head = bucket->next;
        bucket = head->load();
    }
    // originally, we are changing entry_t*, will become ptr_t.reref
    ptr_t<entry_t> *next = ptr_t<entry_t>::create(head_ptr);
    const entry_t *new_entry =
        ptr_t<entry_t>::make_obj(entry_t(key, val, next));
    buckets.get()->v[hv]->reref(new_entry);
    return kRetVals[kCreated];
}

std::string hashmap_t::del(const Key &key) const {
    uint32_t hv = key.hash() % capacity;
    scee::lock_guard_t guard(&locks.v[hv]);
    ptr_t<entry_t> *head = buckets.get()->v[hv];
    const entry_t *bucket = head->load();
    while (bucket != nullptr) {
        if (bucket->key == key) {
            const entry_t *next = bucket->next->load();
            head->reref(next);
            bucket->next->destroy();
            destroy_obj(const_cast<entry_t *>(bucket));
            return kRetVals[kDeleted];
        }
        head = bucket->next;
        bucket = head->load();
    }
    return kRetVals[kNotFound];
}

ptr_t<imm_string_t> *hashmap_get(scee::ptr_t<hashmap_t> *hmap, Key key) {
    std::string ret = hmap->load()->get(key);
    return ptr_t<imm_string_t>::create(imm_string_t(ret.data(), ret.size()));
}

scee::ptr_t<scee::imm_string_t> *hashmap_set(scee::ptr_t<hashmap_t> *hmap,
                                             Key key, Val val) {
    std::string ret = hmap->load()->set(key, val);
    return ptr_t<imm_string_t>::create(imm_string_t(ret.data(), ret.size()));
}

scee::ptr_t<scee::imm_string_t> *hashmap_del(scee::ptr_t<hashmap_t> *hmap,
                                             Key key) {
    std::string ret = hmap->load()->del(key);
    return ptr_t<imm_string_t>::create(imm_string_t(ret.data(), ret.size()));
}

}  // namespace NAMESPACE
