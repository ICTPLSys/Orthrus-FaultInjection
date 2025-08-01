#include <atomic>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "compiler.hpp"
#include "context.hpp"
#include "custom_stl.hpp"
#include "namespace.hpp"
#include "occ.hpp"
#include "ptr.hpp"

namespace scee {

using namespace ::NAMESPACE;

static uint64_t ooc_control_stable_version(OCCControl *occ) {
    // probably, always use load_safe and rollback the cursor when needed
    uint64_t v = 0;
    if (!is_validator()) {
        do {
            v = occ->ver.load(std::memory_order_relaxed);
        } while (v & (OCCControl::INSERT | OCCControl::SPLIT));
    }
    return external_return(v);
}

FORCE_INLINE uint64_t OCCControl::stable_version() {
    return ooc_control_stable_version(this);
}

static uint64_t ooc_control_load(OCCControl *occ) {
    uint64_t v = 0;
    if (!is_validator()) {
        v = occ->ver.load(std::memory_order_relaxed);
    }
    return external_return(v);
}

FORCE_INLINE uint64_t OCCControl::load() { return ooc_control_load(this); }

static void ooc_control_lock(OCCControl *occ) {
    while (!is_validator()) {
        uint64_t oldv;
        uint64_t newv;
        oldv = occ->ver.load(std::memory_order_relaxed);
        if (oldv & OCCControl::LOCK) {
            continue;
        }
        newv = oldv | OCCControl::LOCK;
        if (!occ->ver.compare_exchange_strong(oldv, newv,
                                              std::memory_order_relaxed)) {
            continue;
        }
        break;
    }
}

FORCE_INLINE void OCCControl::lock() { ooc_control_lock(this); }

}  // namespace scee

namespace NAMESPACE {
#include "closure.hpp"

using namespace ::scee;

const Node::Data *extend(const Node::Data *old, int8_t idx, uint64_t key,
                         void *link, void *relink) {
    assert(old->num_keys < fanout && old->num_keys > 0);
    ++idx;
    size_t size = sizeof(Node::Data);
    auto *addr = (Node::Data *)alloc_obj(size);
    auto *shadow = shadow_init(addr);
    memset(shadow, 0, size);

    int8_t n = old->num_keys;
    shadow->num_keys = n + 1;
    memcpy(shadow->links, old->links, n * sizeof(void *));
    if (relink != nullptr) shadow->links[old->sorted[idx - 1]] = relink;
    memcpy(shadow->keys, old->keys, n * sizeof(uint64_t));
    shadow->keys[n] = key, shadow->links[n] = link;
    memcpy(shadow->sorted, old->sorted, idx * sizeof(int8_t));
    shadow->sorted[idx] = n;
    if (idx < n) {
        memcpy(shadow->sorted + idx + 1, old->sorted + idx,
               (n - idx) * sizeof(int8_t));
    }

    shadow_commit(shadow, addr);
    shadow_destroy(shadow);
    return addr;
}

std::pair<const Node::Data *, const Node::Data *> divide(
    const Node::Data *old, int8_t idx, uint64_t key, void *link, void *relink) {
    assert(old->num_keys == fanout);

    size_t size = sizeof(Node::Data);
    constexpr int8_t m = (fanout + 1) / 2;

    Node::Data *left = (Node::Data *)alloc_obj(size);
    {
        auto *shadow = shadow_init(left);
        memset(shadow, 0, size);

        shadow->num_keys = m;
        for (int8_t i = 0; i < m; ++i) {
            if (i != idx + 1) {
                shadow->keys[i] = old->keys[old->sorted[i - (i > idx)]];
                shadow->links[i] = (i != idx || relink == nullptr)
                                       ? old->links[old->sorted[i - (i > idx)]]
                                       : relink;
            } else {
                shadow->keys[i] = key;
                shadow->links[i] = link;
            }
            shadow->sorted[i] = i;
        }

        shadow_commit(shadow, left);
        shadow_destroy(shadow);
    }

    Node::Data *right = (Node::Data *)alloc_obj(size);
    {
        auto *shadow = shadow_init(right);
        memset(shadow, 0, size);

        shadow->num_keys = m;
        for (int8_t i = m; i < m * 2; ++i) {
            if (i != idx + 1) {
                shadow->keys[i - m] = old->keys[old->sorted[i - (i > idx)]];
                shadow->links[i - m] =
                    (i != idx || relink == nullptr)
                        ? old->links[old->sorted[i - (i > idx)]]
                        : relink;
            } else {
                shadow->keys[i - m] = key;
                shadow->links[i - m] = link;
            }
            shadow->sorted[i - m] = i - m;
        }

        shadow_commit(shadow, right);
        shadow_destroy(shadow);
    }

    return std::make_pair(left, right);
}

inline int8_t Node::Data::locate(uint64_t key) const {
    if (unlikely(key < getkey(0))) {
        fprintf(stderr, "key outside node range: %lu < %lu\n", key, getkey(0));
        abort();
    }
    int8_t r = 0;
    for (int8_t i = fanout_highbit; i; i >>= 1) {
        if (r + i < num_keys && getkey(r + i) <= key) {
            r += i;
        }
    }
    return r;
}

struct LeafExtraHeader {
    ptr_t<LeafNode> prev;
    ptr_t<LeafNode> next;
};
constexpr size_t LeafExtraHeaderSize = sizeof(LeafExtraHeader);

struct NodeHeader {
    OCCControl occ;
    ptr_t<Node> parent;
    ptr_t<Node::Data> data;
};
constexpr size_t NodeHeaderSize = sizeof(NodeHeader);

inline OCCControl *Node::get_occ() const {
    return &((NodeHeader *)sub_byte_offset(this, NodeHeaderSize))->occ;
}

inline ptr_t<Node> *Node::get_parent() const {
    return &((NodeHeader *)sub_byte_offset(this, NodeHeaderSize))->parent;
}

inline ptr_t<LeafNode> *LeafNode::get_prev() const {
    return &((LeafExtraHeader *)sub_byte_offset(
                 this, NodeHeaderSize + LeafExtraHeaderSize))
                ->prev;
}

inline ptr_t<LeafNode> *LeafNode::get_next() const {
    return &((LeafExtraHeader *)sub_byte_offset(
                 this, NodeHeaderSize + LeafExtraHeaderSize))
                ->next;
}

inline ptr_t<Node::Data> *Node::get_data() const {
    return &((NodeHeader *)sub_byte_offset(this, NodeHeaderSize))->data;
}

const LeafNode *create_leaf_node(uint64_t occ_v, const Node::Data *data,
                                 const LeafNode *next) {
    size_t size = sizeof(LeafNode);

    LeafExtraHeader *extra = (LeafExtraHeader *)alloc_obj(
        size + NodeHeaderSize + LeafExtraHeaderSize);
    NodeHeader *header =
        (NodeHeader *)add_byte_offset(extra, LeafExtraHeaderSize);
    LeafNode *addr = (LeafNode *)add_byte_offset(header, NodeHeaderSize);
    auto *shadow = shadow_init(addr);

    if (!is_validator()) {
        new (&header->occ) OCCControl(occ_v);
    }
    store_ptr(&header->parent, nullptr);
    store_ptr(&header->data, data);
    shadow->node_type = NodeType::LEAF;
    store_ptr(&extra->prev, nullptr);
    store_ptr(&extra->next, next);

    shadow_commit(shadow, addr);
    shadow_destroy(shadow);

    return addr;
}

const LeafNode *create_leaf_node_data_nearby(uint64_t occ_v,
                                             const Node::Data &data,
                                             const LeafNode *next) {
    size_t data_size = sizeof(Node::Data);
    size_t size = sizeof(LeafNode);

    Node::Data *data_addr = (Node::Data *)alloc_obj(
        data_size + 8 + size + NodeHeaderSize + LeafExtraHeaderSize);
    auto data_shadow = shadow_init(data_addr);
    memcpy(data_shadow, &data, data_size);
    shadow_commit(data_shadow, data_addr);
    shadow_destroy(data_shadow);

    LeafExtraHeader *extra =
        (LeafExtraHeader *)add_byte_offset(data_addr, data_size + 8);
    NodeHeader *header =
        (NodeHeader *)add_byte_offset(extra, LeafExtraHeaderSize);
    LeafNode *addr = (LeafNode *)add_byte_offset(header, NodeHeaderSize);
    auto *shadow = shadow_init(addr);

    if (!is_validator()) {
        new (&header->occ) OCCControl(occ_v);
    }
    store_ptr(&header->parent, nullptr);
    store_ptr(&header->data, data_addr);
    shadow->node_type = NodeType::LEAF;
    store_ptr(&extra->prev, nullptr);
    store_ptr(&extra->next, next);

    shadow_commit(shadow, addr);
    shadow_destroy(shadow);

    return addr;
}

const Node *create_interior_node(uint64_t occ_v, const Node::Data *data,
                                 NodeType node_type) {
    size_t size = sizeof(Node);

    NodeHeader *header = (NodeHeader *)alloc_obj(size + NodeHeaderSize);
    Node *addr = (Node *)add_byte_offset(header, NodeHeaderSize);
    auto *shadow = shadow_init(addr);

    if (!is_validator()) {
        new (&header->occ) OCCControl(occ_v);
    }
    store_ptr(&header->parent, nullptr);
    store_ptr(&header->data, data);
    shadow->node_type = node_type;

    shadow_commit(shadow, addr);
    shadow_destroy(shadow);
    return addr;
}

const Node *create_interior_node_data_nearby(uint64_t occ_v,
                                             const Node::Data &data,
                                             NodeType node_type) {
    size_t data_size = sizeof(Node::Data);
    size_t size = sizeof(Node);

    Node::Data *data_addr =
        (Node::Data *)alloc_obj(data_size + 8 + size + NodeHeaderSize);
    auto data_shadow = shadow_init(data_addr);
    memcpy(data_shadow, &data, data_size);
    shadow_commit(data_shadow, data_addr);
    shadow_destroy(data_shadow);

    NodeHeader *header =
        (NodeHeader *)add_byte_offset(data_addr, data_size + 8);
    Node *addr = (Node *)add_byte_offset(header, NodeHeaderSize);
    auto *shadow = shadow_init(addr);

    if (!is_validator()) {
        new (&header->occ) OCCControl(occ_v);
    }
    store_ptr(&header->parent, nullptr);
    store_ptr(&header->data, data_addr);
    shadow->node_type = node_type;

    shadow_commit(shadow, addr);
    shadow_destroy(shadow);
    return addr;
}

ptr_t<Node> *build_tree() {
    Node::Data leaf_data;
    memset(&leaf_data, 0, sizeof(Node::Data));
    leaf_data.num_keys = 1;
    const Node *leaf =
        create_leaf_node(0, ptr_t<Node::Data>::make_obj(leaf_data), nullptr);

    Node::Data root_data;
    memset(&root_data, 0, sizeof(Node::Data));
    root_data.num_keys = 1;
    root_data.links[0] = (void *)leaf;
    const Node *root = create_interior_node(
        0, ptr_t<Node::Data>::make_obj(root_data), NodeType::ROOT);

    leaf->get_parent()->reref(root);
    ptr_t<Node> *root_ptr = ptr_t<Node>::create(root);
    root->get_parent()->reref(reinterpret_cast<const Node *>(root_ptr));

    return root_ptr;
}

const Node *recursive_build_tree(std::vector<uint64_t> &keys,
                                 std::vector<Value> &vals, uint64_t l,
                                 uint64_t r, LeafNode *&last) {
    if (r - l <= fanout) {
        Node::Data leaf_data;
        memset(&leaf_data, 0, sizeof(Node::Data));
        leaf_data.num_keys = r - l;
        for (uint64_t i = 0; i < (r - l); ++i) {
            leaf_data.keys[i] = keys[l + i];
            leaf_data.sorted[i] = i;
            leaf_data.links[i] = (void *)ptr_t<Value>::create(vals[l + i]);
        }
        const LeafNode *leaf =
            create_leaf_node_data_nearby(0, leaf_data, nullptr);

        if (last != nullptr) last->get_next()->reref(leaf);
        last = const_cast<LeafNode *>(leaf);

        return static_cast<const Node *>(leaf);
    }

    Node::Data interior_data;
    memset(&interior_data, 0, sizeof(Node::Data));
    uint64_t m = (fanout + 1) / 2;
    interior_data.num_keys = m;
    for (uint64_t i = 0; i < m; ++i) {
        const Node *child = recursive_build_tree(
            keys, vals, l + i * (r - l) / m, l + (i + 1) * (r - l) / m, last);
        interior_data.keys[i] = keys[l + i * (r - l) / m];
        interior_data.sorted[i] = i;
        interior_data.links[i] = (void *)child;
    }
    const Node *interior =
        create_interior_node_data_nearby(0, interior_data, NodeType::INTERIOR);
    const Node::Data *interior_ptr = interior->get_data()->load();
    for (uint64_t i = 0; i < m; ++i) {
        ((const Node *)interior_ptr->links[i])->get_parent()->reref(interior);
    }
    return interior;
}

scee::ptr_t<Node> *build_tree_from_keys(std::vector<uint64_t> &keys,
                                        std::vector<Value> &vals) {
    uint64_t n = keys.size(), d = (fanout + 1) / 2, m = 1;
    while (m * d < n) m *= d;
    m = n / m;
    Node::Data root_data;
    memset(&root_data, 0, sizeof(Node::Data));
    root_data.num_keys = m;
    LeafNode *last = nullptr;
    for (uint64_t i = 0; i < m; ++i) {
        const Node *child =
            recursive_build_tree(keys, vals, i * n / m, (i + 1) * n / m, last);
        root_data.links[i] = (void *)child;
        root_data.keys[i] = keys[i * n / m];
        root_data.sorted[i] = i;
    }
    const Node::Data *root_data_ptr = ptr_t<Node::Data>::make_obj(root_data);
    const Node *root = create_interior_node(0, root_data_ptr, NodeType::ROOT);
    for (uint64_t i = 0; i < m; ++i) {
        ((const Node *)root_data_ptr->links[i])->get_parent()->reref(root);
    }

    ptr_t<Node> *root_ptr = ptr_t<Node>::create(root);
    root->get_parent()->reref(reinterpret_cast<const Node *>(root_ptr));
    return root_ptr;
}

std::pair<const LeafNode *, uint64_t> reach_leaf(ptr_t<Node> *root,
                                                 uint64_t key) {
    void *cursor = fetch_cursor();

    while (true) {
        const Node *node = root->load();
        uint64_t v_stable = node->get_occ()->stable_version();

        while (true) {
            if (node->node_type == NodeType::LEAF) {
                return std::make_pair(static_cast<const LeafNode *>(node),
                                      v_stable);
            }

            const Node::Data *data = node->get_data()->load();
            uint32_t idx = data->locate(key);
            const Node *child = data->child(idx);
            uint64_t v_next = child->get_occ()->stable_version();

            uint64_t v_unstable = node->get_occ()->load();
            if (likely((v_stable ^ v_unstable) <= OCCControl::LOCK)) {
                node = child;
                v_stable = v_next;
                continue;
            }
            if (unlikely(v_unstable & OCCControl::WRITING)) {
                v_unstable = node->get_occ()->stable_version();
            }
            if (unlikely(v_unstable & OCCControl::DELETED)) {
                rollback_cursor(cursor);
                break;
            }

            v_stable = v_unstable;
        }
    }
}

const Value *read(ptr_t<Node> *root, uint64_t key) {
    // avoid cost introduced by atomic operations
    auto [leaf, v_stable] = reach_leaf(root, key);
    // leaf->get_occ()->lock();
    auto *data = leaf->get_data()->load();
    int8_t idx = data->locate(key);
    auto *value = data->getkey(idx) == key ? data->value(idx)->load() : nullptr;
    // leaf->get_occ()->unlock();
    return value;
}

uint64_t scan_and_sum(ptr_t<Node> *root, uint64_t key, uint64_t num_keys) {
    auto [leaf, v_stable] = reach_leaf(root, key);

    uint64_t ssum = 0, scnt = 0;
    auto *data = leaf->get_data()->load();
    int8_t idx = data->locate(key);

    if (data->keys[idx] < key) ++idx;
    while (scnt < num_keys) {
        if (idx == data->num_keys) {
            leaf = leaf->get_next()->load();
            if (leaf == nullptr) break;
            data = leaf->get_data()->load();
            idx = 0;
        }
        constexpr uint64_t primes[] = {2,  3,  5,  7,  11, 13, 17, 19,
                                       23, 29, 31, 37, 41, 43, 47, 53};
        uint64_t kk = data->keys[data->sorted[idx]], cnt = 0;
        for (size_t i = VAL_LEN / 2; i < VAL_LEN; ++i) {
            for (size_t j = 0; j < VAL_LEN / 2; ++j) {
                cnt += kk % primes[i] <= kk % primes[j];
            }
        }
        if (cnt % 3 == 2) {
            const Value *str = data->value(idx)->load();
            if (str != nullptr) {
                for (size_t i = 0; i < VAL_LEN; ++i) {
                    ssum += str->ch[i];
                }
            }
        }

        ++idx, ++scnt;
    }

    return ssum;
}

const LeafNode *locate_locked(ptr_t<Node> *root, uint64_t key) {
    void *cursor = fetch_cursor();
    while (true) {
        auto [leaf, v_stable] = reach_leaf(root, key);
        if (likely(!(v_stable & OCCControl::DELETED))) {
            leaf->get_occ()->lock();
            uint64_t v_stable_next = leaf->get_occ()->stable_version();
            if ((v_stable_next ^ v_stable) <= OCCControl::LOCK) {
                return leaf;
            }
            leaf->get_occ()->unlock();
        }
        rollback_cursor(cursor);
    }
}

const Value *update(ptr_t<Node> *root, uint64_t key, Value val) {
    auto *leaf = locate_locked(root, key);
    auto *data = leaf->get_data()->load();

    int8_t idx = data->locate(key);
    const Value *old_val = nullptr;
    if (data->getkey(idx) == key) {
        ptr_t<Value> *val_ptr = const_cast<ptr_t<Value> *>(data->value(idx));
        old_val = val_ptr->load();
        const Value *new_val = ptr_t<Value>::make_obj(val);
        val_ptr->reref(new_val);
    }

    leaf->get_occ()->unlock();
    return old_val;
}

const LeafNode *lock_prev(const LeafNode *leaf) {
    void *cursor = fetch_cursor();
    while (true) {
        auto *prev = leaf->get_prev()->load();
        if (likely(prev != nullptr)) prev->get_occ()->lock();
        if (prev == leaf->get_prev()->load()) {
            return prev;
        }
        if (likely(prev != nullptr)) prev->get_occ()->unlock();
        rollback_cursor(cursor);
    }
}

const Node *lock_parent(const Node *node) {
    void *cursor = fetch_cursor();
    while (true) {
        auto *parent = node->get_parent()->load();
        if (likely(parent != nullptr)) {
            parent->get_occ()->lock();
            if (parent == node->get_parent()->load()) {
                return parent;
            }
            parent->get_occ()->unlock();
        }
        rollback_cursor(cursor);
    }
}

// replace idx-th with replace, and insert child after it
void insert_interior(const Node *node, const Node::Data *data, int8_t idx,
                     uint64_t key, const Node *replace, const Node *child) {
    if (likely(data->num_keys < fanout)) {
        const Node::Data *new_data =
            extend(data, idx, key, (void *)child, (void *)replace);
        node->get_occ()->do_insert();
        node->get_data()->reref(new_data);
        node->get_occ()->done_insert();
        node->get_occ()->unlock();
        return;
    }

    auto [l_data, r_data] =
        divide(data, idx, key, (void *)child, (void *)replace);

    node->get_occ()->do_split();

    const Node *l_node = create_interior_node(
        OCCControl::LOCK | OCCControl::SPLIT, l_data, NodeType::INTERIOR);
    const Node *r_node = create_interior_node(
        OCCControl::LOCK | OCCControl::SPLIT, r_data, NodeType::INTERIOR);

    for (int8_t i = 0; i < l_data->num_keys; ++i) {
        l_data->child(i)->get_parent()->reref(l_node);
    }
    for (int8_t i = 0; i < r_data->num_keys; ++i) {
        r_data->child(i)->get_parent()->reref(r_node);
    }

    if (unlikely(node->node_type == NodeType::ROOT)) {
        Node::Data root_data;
        memset(&root_data, 0, sizeof(Node::Data));
        root_data.num_keys = 2;
        root_data.keys[0] = l_data->getkey(0);
        root_data.keys[1] = r_data->getkey(0);
        root_data.links[0] = (void *)l_node;
        root_data.links[1] = (void *)r_node;
        root_data.sorted[0] = 0;
        root_data.sorted[1] = 1;

        const Node::Data *r_data = ptr_t<Node::Data>::make_obj(root_data);
        const Node *new_root = create_interior_node(
            OCCControl::LOCK | OCCControl::SPLIT, r_data, NodeType::ROOT);

        l_node->get_parent()->reref(new_root);
        r_node->get_parent()->reref(new_root);
        auto *root_ptr =
            reinterpret_cast<const ptr_t<Node> *>(node->get_parent()->load());
        new_root->get_parent()->reref(reinterpret_cast<const Node *>(root_ptr));
        const_cast<ptr_t<Node> *>(root_ptr)->reref(
            new_root);  // should be atomic

        new_root->get_occ()->done_create();
        new_root->get_occ()->unlock();
        node->get_occ()->done_split_and_delete();
        node->get_occ()->unlock();
        l_node->get_occ()->done_create();
        l_node->get_occ()->unlock();
        r_node->get_occ()->done_create();
        r_node->get_occ()->unlock();

        // const_cast<Node *>(node)->destroy(data);

        return;
    }

    const Node *parent = lock_parent(node);
    l_node->get_parent()->reref(parent);
    r_node->get_parent()->reref(parent);

    const Node::Data *pdata = parent->get_data()->load();
    int8_t pidx = pdata->locate(r_data->getkey(0));

    node->get_occ()->done_split_and_delete();
    node->get_occ()->unlock();
    l_node->get_occ()->done_create();
    l_node->get_occ()->unlock();
    r_node->get_occ()->done_create();
    r_node->get_occ()->unlock();

    // const_cast<Node *>(node)->destroy(data);

    insert_interior(parent, pdata, pidx, r_data->getkey(0),
                    static_cast<const Node *>(l_node),
                    static_cast<const Node *>(r_node));
}

uint8_t insert(ptr_t<Node> *root, uint64_t key, Value val) {
    const LeafNode *leaf = locate_locked(root, key);
    const Node::Data *data = leaf->get_data()->load();
    ptr_t<Value> *val_ptr = ptr_t<Value>::create(val);

    int8_t idx = data->locate(key);
    if (unlikely(data->getkey(idx) == key)) {
        leaf->get_occ()->unlock();
        return 0;
    }

    if (likely(data->num_keys < fanout)) {
        const Node::Data *new_data = extend(data, idx, key, val_ptr, nullptr);
        leaf->get_occ()->do_insert();
        leaf->get_data()->reref(new_data);
        leaf->get_occ()->done_insert();
        leaf->get_occ()->unlock();
        return 1;
    }

    auto [l_data, r_data] = divide(data, idx, key, val_ptr, nullptr);

    const LeafNode *next = leaf->get_next()->load();

    const LeafNode *r_leaf =
        create_leaf_node(OCCControl::LOCK | OCCControl::SPLIT, r_data, next);
    const LeafNode *l_leaf =
        create_leaf_node(OCCControl::LOCK | OCCControl::SPLIT, l_data, r_leaf);
    r_leaf->get_prev()->reref(l_leaf);

    leaf->get_occ()->do_split();
    if (next != nullptr) next->get_prev()->reref(r_leaf);

    const LeafNode *prev = lock_prev(leaf);
    l_leaf->get_prev()->reref(prev);
    if (prev != nullptr) {
        prev->get_next()->reref(l_leaf);
        prev->get_occ()->unlock();
    }

    const Node *parent = lock_parent(leaf);
    l_leaf->get_parent()->reref(parent);
    r_leaf->get_parent()->reref(parent);

    const Node::Data *pdata = parent->get_data()->load();
    int8_t pidx = pdata->locate(l_data->getkey(0));

    leaf->get_occ()->done_split_and_delete();
    leaf->get_occ()->unlock();
    l_leaf->get_occ()->done_create();
    l_leaf->get_occ()->unlock();
    r_leaf->get_occ()->done_create();
    r_leaf->get_occ()->unlock();

    // const_cast<LeafNode *>(leaf)->destroy(data);

    insert_interior(parent, pdata, pidx, r_data->getkey(0),
                    static_cast<const Node *>(l_leaf),
                    static_cast<const Node *>(r_leaf));

    return 1;
}

}  // namespace NAMESPACE
