/*
Following are the required headers:
#include <cstring>
#include "ptr.hpp"
#include "context.hpp"
#include "ctltypes.hpp"
#include "namespace.hpp"
#include "custom_stl.hpp"
*/
enum RetType {
    kError,
    kDeleted,
    kNotFound,
    kStored,
    kCreated,
    kEnd,
    kValue,
    kNumRetVals,
};

static std::string kRetVals[] = {"ERROR\r\n",  "DELETED\r\n", "NOT_FOUND\r\n",
                                 "STORED\r\n", "CREATED\r\n", "END\r\n",
                                 "VALUE "};
static std::string kCrlf = "\r\n";

constexpr size_t KEY_LEN = 12;
constexpr size_t VAL_LEN = 4;

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
    std::string to_string() const { return std::string(ch, VAL_LEN); }
};

struct hashmap_t : public scee::imm_nonunique_t {
    struct entry_t {
        Key key;
        Val val;
        scee::ptr_t<entry_t> *next;
        entry_t(Key key, Val val, scee::ptr_t<entry_t> *next)
            : key(key), val(val), next(next) {}
    };
    size_t capacity;
    scee::fixed_ptr_t<scee::mut_array_t<entry_t>> buckets;
    scee::mutable_list_t<scee::mutex_t> locks;
    hashmap_t() { capacity = 0; }
    // make: create a hashmap instance in non-versioned memory
    static hashmap_t make(size_t cap);
    void destroy() const;
    std::string get(const Key &key) const;
    std::string set(const Key &key, const Val &val) const;
    std::string del(const Key &key) const;
};

scee::ptr_t<scee::imm_string_t> *hashmap_get(scee::ptr_t<hashmap_t> *hmap,
                                             Key key);
scee::ptr_t<scee::imm_string_t> *hashmap_set(scee::ptr_t<hashmap_t> *hmap,
                                             Key key, Val val);
scee::ptr_t<scee::imm_string_t> *hashmap_del(scee::ptr_t<hashmap_t> *hmap,
                                             Key key);
