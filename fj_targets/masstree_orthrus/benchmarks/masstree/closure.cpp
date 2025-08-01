#include <atomic>
#include <cstring>
#include <cstdlib>

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
    auto ret = external_return(v);
    if (is_validator()) {
        validator_assert(v & (OCCControl::INSERT | OCCControl::SPLIT) == 0);
    }
    return ret;
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

Node::Node(uint64_t occ_v, const Data *data, NodeType node_type)
    : occ(OCCControl::create(occ_v)),
      parent(ptr_t<Node>::create()),
      data(ptr_t<Data>::create(data)),
      node_type(node_type) {}

void Node::destroy(const Data *dataptr) {
    occ->destroy();
    parent->destroy();
    data->destroy();
    destroy_obj(const_cast<Data *>(dataptr));
    destroy_obj(const_cast<Node *>(this));
}

void LeafNode::destroy(const Data *dataptr) {
    occ->destroy();
    parent->destroy();
    data->destroy();
    destroy_obj(const_cast<Data *>(dataptr));
    prev->destroy();
    next->destroy();
    destroy_obj(const_cast<LeafNode *>(this));
}

void data_destroy(Node::Data *ptr) {
    if (!is_validator()) {
        return;
        constexpr uint32_t kBufferSize = 23333;
        static thread_local Node::Data *buffer[kBufferSize] = {nullptr};
        static thread_local uint32_t cnt = 0;
        int idx = cnt++ % kBufferSize;
        if (buffer[idx] != nullptr) destroy_obj(buffer[idx]);
        buffer[idx] = ptr;
    }
}

LeafNode::LeafNode(uint64_t occ_v, const Data *data, const LeafNode *next)
    : Node(occ_v, data, NodeType::LEAF),
      prev(ptr_t<LeafNode>::create()),
      next(ptr_t<LeafNode>::create(next)) {}

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

ptr_t<Node> *build_tree() {
    Node::Data leaf_data;
    memset(&leaf_data, 0, sizeof(Node::Data));
    leaf_data.num_keys = 1;
    const Node *leaf = ptr_t<LeafNode>::make_obj(
        LeafNode(0, ptr_t<Node::Data>::make_obj(leaf_data), nullptr));

    Node::Data root_data;
    memset(&root_data, 0, sizeof(Node::Data));
    root_data.num_keys = 1;
    root_data.links[0] = (void *)leaf;
    const Node *root = ptr_t<Node>::make_obj(
        Node(0, ptr_t<Node::Data>::make_obj(root_data), NodeType::ROOT));

    leaf->parent->reref(root);
    ptr_t<Node> *root_ptr = ptr_t<Node>::create(root);
    root->parent->reref(reinterpret_cast<const Node *>(root_ptr));

    return root_ptr;
}

std::pair<const LeafNode *, uint64_t> reach_leaf(ptr_t<Node> *root,
                                                 uint64_t key) {
    void *cursor = fetch_cursor();

    while (true) {
        const Node *node = root->load();
        uint64_t v_stable = node->occ->stable_version();

        while (true) {
            if (node->node_type == NodeType::LEAF) {
                return std::make_pair(static_cast<const LeafNode *>(node),
                                      v_stable);
            }

            const Node::Data *data = node->data->load();
            uint32_t idx = data->locate(key);
            const Node *child = data->child(idx);
            uint64_t v_next = child->occ->stable_version();

            uint64_t v_unstable = node->occ->load();
            if (likely((v_stable ^ v_unstable) <= OCCControl::LOCK)) {
                node = child;
                v_stable = v_next;
                continue;
            }
            if (unlikely(v_unstable & OCCControl::WRITING)) {
                v_unstable = node->occ->stable_version();
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
    leaf->occ->lock();
    auto *data = leaf->data->load();
    int8_t idx = data->locate(key);
    auto *value = data->getkey(idx) == key ? data->value(idx)->load() : nullptr;
    leaf->occ->unlock();
    return value;
}

const LeafNode *locate_locked(ptr_t<Node> *root, uint64_t key) {
    void *cursor = fetch_cursor();
    while (true) {
        auto [leaf, v_stable] = reach_leaf(root, key);
        if (likely(!(v_stable & OCCControl::DELETED))) {
            leaf->occ->lock();
            uint64_t v_stable_next = leaf->occ->stable_version();
            if ((v_stable_next ^ v_stable) <= OCCControl::LOCK) {
                return leaf;
            }
            leaf->occ->unlock();
        }
        rollback_cursor(cursor);
    }
}

const Value *update(ptr_t<Node> *root, uint64_t key, Value val) {
    auto *leaf = locate_locked(root, key);
    auto *data = leaf->data->load();

    int8_t idx = data->locate(key);
    const Value *old_val = nullptr;
    if (data->getkey(idx) == key) {
        ptr_t<Value> *val_ptr = const_cast<ptr_t<Value> *>(data->value(idx));
        old_val = val_ptr->load();
        const Value *new_val = ptr_t<Value>::make_obj(val);
        val_ptr->reref(new_val);
    }

    leaf->occ->unlock();
    return old_val;
}

const LeafNode *lock_prev(const LeafNode *leaf) {
    void *cursor = fetch_cursor();
    while (true) {
        auto *prev = leaf->prev->load();
        if (likely(prev != nullptr)) prev->occ->lock();
        if (prev == leaf->prev->load()) {
            return prev;
        }
        if (likely(prev != nullptr)) prev->occ->unlock();
        rollback_cursor(cursor);
    }
}

const Node *lock_parent(const Node *node) {
    void *cursor = fetch_cursor();
    while (true) {
        auto *parent = node->parent->load();
        if (likely(parent != nullptr)) {
            parent->occ->lock();
            if (parent == node->parent->load()) {
                return parent;
            }
            parent->occ->unlock();
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
        node->occ->do_insert();
        node->data->reref(new_data);
        node->occ->done_insert();
        node->occ->unlock();
        data_destroy(const_cast<Node::Data *>(data));
        return;
    }

    auto [l_data, r_data] =
        divide(data, idx, key, (void *)child, (void *)replace);

    node->occ->do_split();

    const Node *l_node = ptr_t<Node>::make_obj(
        Node(OCCControl::LOCK | OCCControl::SPLIT, l_data, NodeType::INTERIOR));
    const Node *r_node = ptr_t<Node>::make_obj(
        Node(OCCControl::LOCK | OCCControl::SPLIT, r_data, NodeType::INTERIOR));

    for (int8_t i = 0; i < l_data->num_keys; ++i) {
        l_data->child(i)->parent->reref(l_node);
    }
    for (int8_t i = 0; i < r_data->num_keys; ++i) {
        r_data->child(i)->parent->reref(r_node);
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
        const Node *new_root = ptr_t<Node>::make_obj(
            Node(OCCControl::LOCK | OCCControl::SPLIT, r_data, NodeType::ROOT));

        l_node->parent->reref(new_root);
        r_node->parent->reref(new_root);
        auto *root_ptr =
            reinterpret_cast<const ptr_t<Node> *>(node->parent->load());
        new_root->parent->reref(reinterpret_cast<const Node *>(root_ptr));
        const_cast<ptr_t<Node> *>(root_ptr)->reref(
            new_root);  // should be atomic

        new_root->occ->done_create();
        new_root->occ->unlock();
        node->occ->done_split_and_delete();
        node->occ->unlock();
        l_node->occ->done_create();
        l_node->occ->unlock();
        r_node->occ->done_create();
        r_node->occ->unlock();

        // const_cast<Node *>(node)->destroy(data);

        return;
    }

    const Node *parent = lock_parent(node);
    l_node->parent->reref(parent);
    r_node->parent->reref(parent);

    const Node::Data *pdata = parent->data->load();
    int8_t pidx = pdata->locate(r_data->getkey(0));

    node->occ->done_split_and_delete();
    node->occ->unlock();
    l_node->occ->done_create();
    l_node->occ->unlock();
    r_node->occ->done_create();
    r_node->occ->unlock();

    // const_cast<Node *>(node)->destroy(data);

    insert_interior(parent, pdata, pidx, r_data->getkey(0),
                    static_cast<const Node *>(l_node),
                    static_cast<const Node *>(r_node));
}

uint8_t insert(ptr_t<Node> *root, uint64_t key, Value val) {
    const LeafNode *leaf = locate_locked(root, key);
    const Node::Data *data = leaf->data->load();
    ptr_t<Value> *val_ptr = ptr_t<Value>::create(val);

    int8_t idx = data->locate(key);
    if (unlikely(data->getkey(idx) == key)) {
        leaf->occ->unlock();
        return 0;
    }

    if (likely(data->num_keys < fanout)) {
        const Node::Data *new_data = extend(data, idx, key, val_ptr, nullptr);
        leaf->occ->do_insert();
        leaf->data->reref(new_data);
        leaf->occ->done_insert();
        leaf->occ->unlock();
        data_destroy(const_cast<Node::Data *>(data));
        return 1;
    }

    auto [l_data, r_data] = divide(data, idx, key, val_ptr, nullptr);

    const LeafNode *next = leaf->next->load();

    const LeafNode *r_leaf = ptr_t<LeafNode>::make_obj(
        LeafNode(OCCControl::LOCK | OCCControl::SPLIT, r_data, next));
    const LeafNode *l_leaf = ptr_t<LeafNode>::make_obj(
        LeafNode(OCCControl::LOCK | OCCControl::SPLIT, l_data, r_leaf));
    r_leaf->prev->reref(l_leaf);

    leaf->occ->do_split();
    if (next != nullptr) next->prev->reref(r_leaf);

    const LeafNode *prev = lock_prev(leaf);
    l_leaf->prev->reref(prev);
    if (prev != nullptr) {
        prev->next->reref(l_leaf);
        prev->occ->unlock();
    }

    const Node *parent = lock_parent(leaf);
    l_leaf->parent->reref(parent);
    r_leaf->parent->reref(parent);

    const Node::Data *pdata = parent->data->load();
    int8_t pidx = pdata->locate(l_data->getkey(0));

    leaf->occ->done_split_and_delete();
    leaf->occ->unlock();
    l_leaf->occ->done_create();
    l_leaf->occ->unlock();
    r_leaf->occ->done_create();
    r_leaf->occ->unlock();

    // const_cast<LeafNode *>(leaf)->destroy(data);

    insert_interior(parent, pdata, pidx, r_data->getkey(0),
                    static_cast<const Node *>(l_leaf),
                    static_cast<const Node *>(r_leaf));

    return 1;
}

}  // namespace NAMESPACE
