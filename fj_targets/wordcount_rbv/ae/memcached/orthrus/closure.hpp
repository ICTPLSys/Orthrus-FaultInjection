/*
Following are the required headers:
#include <cstring>

#include "context.hpp"
#include "ctltypes.hpp"
#include "custom_stl.hpp"
#include "namespace.hpp"
#include "ptr.hpp"
*/
#include "common.hpp"

struct Key {
    char ch[KEY_LEN];
    uint32_t hash() const {
        uint32_t hash = 5381;
        for (size_t i = 0; i < KEY_LEN; i++) {
            hash = ((hash << 5) + hash) + ((uint32_t)ch[i]);
        }
        return hash;
    }
    bool operator==(const Key &other) const {
        return memcmp(ch, other.ch, KEY_LEN) == 0;
    }
    std::string to_string() const { return std::string(ch, KEY_LEN); }
};

struct Val {
    char ch[VAL_LEN];
    static const Val empty() {
        Val val;
        memset(val.ch, 0, VAL_LEN);
        return val;
    }
    bool operator==(const Val &other) const {
        return memcmp(ch, other.ch, VAL_LEN) == 0;
    }
    std::string to_string() const { return std::string(ch, VAL_LEN); }
};

struct hashmap_t : public scee::imm_nonunique_t {
    size_t size() const { return sizeof(*this); }
    struct entry_t : public scee::imm_nonunique_t {
        size_t size() const { return sizeof(*this); }
        Key key;
        scee::ptr_t<Val> *val_ptr;
        scee::fixed_ptr_t<entry_t> next;
        entry_t(Key key, Val val, scee::fixed_ptr_t<entry_t> next);
        void destroy() const;
        void setv(Val val) const;
        const Val *getv() const;
    };
    static_assert(std::has_unique_object_representations_v<entry_t>);
    size_t capacity;
    scee::fixed_ptr_t<scee::mut_array_t<entry_t>> buckets;
    scee::mutable_list_t<scee::mutex_t> locks;
    hashmap_t() { capacity = 0; }
    // make: create a hashmap instance in non-versioned memory
    static hashmap_t make(size_t cap);
    void destroy() const;
    const Val *get(const Key &key) const;
    RetType set(const Key &key, const Val &val) const;
    RetType del(const Key &key) const;
};

const Val *hashmap_get(scee::ptr_t<hashmap_t> *hmap, Key key);
RetType hashmap_set(scee::ptr_t<hashmap_t> *hmap, Key key, Val val);
RetType hashmap_del(scee::ptr_t<hashmap_t> *hmap, Key key);
