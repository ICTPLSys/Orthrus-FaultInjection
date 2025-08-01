#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "compiler.hpp"

/**
 * OCCControl is the optimistic concurrency control structure.
 * LOCK: this node is blocked to writes;
 * INSERT: we are inserting to this node, maximum fanout-1 times;
 * SPLIT: we are splitting this node, maximum 1 time;
 * DELETED: this node have been removed from main tree;
 * INSERTCNT: count the number of times we have inserted.
 */
struct OCCControl {
    enum {
        LOCK = 1ULL,
        INSERT = LOCK << 1,
        SPLIT = INSERT << 1,
        DELETED = SPLIT << 1,
        INSERTCNT = DELETED << 1,
    };
    constexpr static uint64_t WRITING = INSERT | SPLIT;
    std::atomic<uint64_t> ver;
    explicit OCCControl(uint64_t version) : ver(version) {}
    uint64_t stable_version() {
        uint64_t v = 0;
        do {
            v = ver.load(std::memory_order_relaxed);
        } while (v & (INSERT | SPLIT));
        return v;
    }
    uint64_t load() { return ver.load(std::memory_order_relaxed); }
    void lock() {
        while (true) {
            uint64_t oldv;
            uint64_t newv;
            oldv = ver.load(std::memory_order_relaxed);
            if (oldv & LOCK) {
                continue;
            }
            newv = oldv | LOCK;
            if (!ver.compare_exchange_strong(oldv, newv,
                                             std::memory_order_relaxed)) {
                continue;
            }
            break;
        }
    }
    void unlock() { ver ^= LOCK; }
    void do_insert() { ver |= INSERT; }
    void done_insert() {
        ver += INSERTCNT;
        ver ^= INSERT;
    }
    void do_split() { ver |= SPLIT; }
    void done_create() {
        ver += INSERTCNT;
        ver ^= SPLIT;
    }
    void done_split_and_delete() {
        ver += INSERTCNT;
        ver |= DELETED;
        ver ^= SPLIT;
    }
};

constexpr size_t VAL_LEN = 16;

enum RetType { kError, kUpdated, kNotFound, kExists, kInserted };
enum NodeType : uint64_t { LEAF, INTERIOR, ROOT };

struct Value {
    char ch[VAL_LEN];
};

static constexpr int8_t fanout = 29, fanout_highbit = 16;
// static constexpr int8_t fanout = 5, fanout_highbit = 4;
static_assert(fanout_highbit * 2 > fanout && fanout_highbit <= fanout);
static_assert(fanout % 2 == 1);

struct Node {
    struct Data {
        void *links[fanout];
        uint64_t keys[fanout];
        int8_t sorted[fanout], num_keys;
        int8_t locate(uint64_t key) {
            if (unlikely(key < getkey(0))) {
                fprintf(stderr, "key outside node range: %lu < %lu\n", key,
                        getkey(0));
                std::abort();
            }
            int8_t r = 0;
            for (int8_t i = fanout_highbit; i; i >>= 1) {
                if (r + i < num_keys && getkey(r + i) <= key) {
                    r += i;
                }
            }
            return r;
        }
        uint64_t getkey(int8_t idx) { return keys[sorted[idx]]; }
        Node *child(int8_t idx) {
            return static_cast<Node *>(links[sorted[idx]]);
        }
        Value *value(int8_t idx) {
            return static_cast<Value *>(links[sorted[idx]]);
        }
    };

    OCCControl occ;
    Node *parent;
    Data data;
    NodeType node_type;

    Node(uint64_t occ_v, Data data, NodeType node_type)
        : occ(occ_v), parent(nullptr), data(data), node_type(node_type) {}
};

struct LeafNode : Node {
    LeafNode *prev, *next;
    LeafNode(uint64_t occ_v, Data data, LeafNode *next)
        : Node(occ_v, data, NodeType::LEAF), prev(nullptr), next(next) {}
};

Node **build_tree();
const Value *read(Node **root, uint64_t key);
RetType insert(Node **root, uint64_t key, Value val);
RetType update(Node **root, uint64_t key, Value val);
Node **build_tree_from_keys(std::vector<uint64_t> &keys,
                            std::vector<Value> &vals);
