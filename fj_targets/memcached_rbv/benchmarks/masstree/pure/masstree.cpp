#include "masstree.hpp"

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <utility>

#include "compiler.hpp"
// #include "mimalloc-new-delete.h"

void extend(Node::Data *old, int8_t idx, uint64_t key, void *link,
            void *relink) {
    assert(old->num_keys < fanout && old->num_keys > 0);
    if (relink != nullptr) {
        old->links[old->sorted[idx]] = relink;
    }
    ++idx;
    int8_t n = old->num_keys;
    ++old->num_keys;
    old->keys[n] = key;
    old->links[n] = link;
    for (int8_t i = n; i > idx; --i) old->sorted[i] = old->sorted[i - 1];
    old->sorted[idx] = n;
}

std::pair<Node::Data, Node::Data> divide(Node::Data *old, int8_t idx,
                                         uint64_t key, void *link,
                                         void *relink) {
    assert(old->num_keys == fanout);

    size_t size = sizeof(Node::Data);
    constexpr int8_t m = (fanout + 1) / 2;

    Node::Data left, right;

    memset(&left, 0, size);
    left.num_keys = m;
    for (int8_t i = 0; i < m; ++i) {
        if (i != idx + 1) {
            left.keys[i] = old->keys[old->sorted[i - (i > idx)]];
            left.links[i] = (i != idx || relink == nullptr)
                                ? old->links[old->sorted[i - (i > idx)]]
                                : relink;
        } else {
            left.keys[i] = key;
            left.links[i] = link;
        }
        left.sorted[i] = i;
    }

    memset(&right, 0, size);
    right.num_keys = m;
    for (int8_t i = m; i < m * 2; ++i) {
        if (i != idx + 1) {
            right.keys[i - m] = old->keys[old->sorted[i - (i > idx)]];
            right.links[i - m] = (i != idx || relink == nullptr)
                                     ? old->links[old->sorted[i - (i > idx)]]
                                     : relink;
        } else {
            right.keys[i - m] = key;
            right.links[i - m] = link;
        }
        right.sorted[i - m] = i - m;
    }

    return std::make_pair(left, right);
}

Node **build_tree() {
    Node::Data leaf_data;
    memset(&leaf_data, 0, sizeof(Node::Data));
    leaf_data.num_keys = 1;
    LeafNode *leaf = new LeafNode(0, leaf_data, nullptr);

    Node::Data root_data;
    memset(&root_data, 0, sizeof(Node::Data));
    root_data.num_keys = 1;
    root_data.links[0] = (void *)leaf;
    Node *root = new Node(0, root_data, NodeType::ROOT);

    leaf->parent = root;
    Node **root_ptr = new Node *;
    *root_ptr = root;
    root->parent = reinterpret_cast<Node *>(root_ptr);

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
            leaf_data.links[i] = (Value *)malloc(sizeof(Value));
            memcpy(leaf_data.links[i], &vals[l + i], sizeof(Value));
        }
        LeafNode *leaf = new LeafNode(0, leaf_data, nullptr);

        if (last != nullptr) last->next = leaf;
        last = leaf;
        leaf->prev = last;

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
    const Node *interior = new Node(0, interior_data, NodeType::INTERIOR);
    for (uint64_t i = 0; i < m; ++i) {
        ((Node *)interior->data.links[i])->parent =
            const_cast<Node *>(interior);
    }
    return interior;
}

Node **build_tree_from_keys(std::vector<uint64_t> &keys,
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
    Node *root = new Node(0, root_data, NodeType::ROOT);
    for (uint64_t i = 0; i < m; ++i) {
        ((Node *)root->data.links[i])->parent = const_cast<Node *>(root);
    }

    Node **root_ptr = new Node *;
    *root_ptr = root;
    root->parent = reinterpret_cast<Node *>(root_ptr);
    return root_ptr;
}

std::pair<LeafNode *, uint64_t> reach_leaf(Node **root, uint64_t key) {
    while (true) {
        Node *node = *root;
        uint64_t v_stable = node->occ.stable_version();

        while (true) {
            if (node->node_type == NodeType::LEAF) {
                return std::make_pair(static_cast<LeafNode *>(node), v_stable);
            }

            uint32_t idx = node->data.locate(key);
            Node *child = node->data.child(idx);
            uint64_t v_next = child->occ.stable_version();

            uint64_t v_unstable = node->occ.load();
            if (likely((v_stable ^ v_unstable) <= OCCControl::LOCK)) {
                node = child;
                v_stable = v_next;
                continue;
            }
            if (unlikely(v_unstable & OCCControl::WRITING)) {
                v_unstable = node->occ.stable_version();
            }
            if (unlikely(v_unstable & OCCControl::DELETED)) {
                break;
            }

            v_stable = v_unstable;
        }
    }
}

const Value *read(Node **root, uint64_t key) {
    auto [leaf, v_stable] = reach_leaf(root, key);
    leaf->occ.lock();
    auto *data = &leaf->data;
    int8_t idx = data->locate(key);
    auto *value = data->getkey(idx) == key ? data->value(idx) : nullptr;
    leaf->occ.unlock();
    return value;
}

LeafNode *locate_locked(Node **root, uint64_t key) {
    while (true) {
        auto [leaf, v_stable] = reach_leaf(root, key);
        if (likely(!(v_stable & OCCControl::DELETED))) {
            leaf->occ.lock();
            uint64_t v_stable_next = leaf->occ.stable_version();
            if ((v_stable_next ^ v_stable) <= OCCControl::LOCK) {
                return leaf;
            }
            leaf->occ.unlock();
        }
    }
}

RetType update(Node **root, uint64_t key, Value val) {
    auto *leaf = locate_locked(root, key);
    auto *data = &leaf->data;

    int8_t idx = data->locate(key);
    RetType ret = RetType::kNotFound;
    if (data->getkey(idx) == key) {
        *data->value(idx) = val;
        ret = RetType::kUpdated;
    }

    leaf->occ.unlock();
    return ret;
}

LeafNode *lock_prev(LeafNode *leaf) {
    while (true) {
        auto *prev = leaf->prev;
        if (likely(prev != nullptr)) prev->occ.lock();
        if (prev == leaf->prev) {
            return prev;
        }
        if (likely(prev != nullptr)) prev->occ.unlock();
    }
}

Node *lock_parent(Node *node) {
    while (true) {
        auto *parent = node->parent;
        if (likely(parent != nullptr)) {
            parent->occ.lock();
            if (parent == node->parent) {
                return parent;
            }
            parent->occ.unlock();
        }
    }
}

// replace idx-th with replace, and insert child after it
void insert_interior(Node *node, Node::Data *data, int8_t idx, uint64_t key,
                     Node *replace, Node *child) {
    if (likely(data->num_keys < fanout)) {
        node->occ.do_insert();
        extend(data, idx, key, (void *)child, (void *)replace);
        node->occ.done_insert();
        node->occ.unlock();
        return;
    }

    auto [l_data_, r_data_] =
        divide(data, idx, key, (void *)child, (void *)replace);

    node->occ.do_split();

    Node *l_node = new Node(OCCControl::LOCK | OCCControl::SPLIT, l_data_,
                            NodeType::INTERIOR);
    Node *r_node = new Node(OCCControl::LOCK | OCCControl::SPLIT, r_data_,
                            NodeType::INTERIOR);
    auto l_data = &l_node->data;
    auto r_data = &r_node->data;

    for (int8_t i = 0; i < l_data->num_keys; ++i) {
        l_data->child(i)->parent = l_node;
    }
    for (int8_t i = 0; i < r_data->num_keys; ++i) {
        r_data->child(i)->parent = r_node;
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

        Node *new_root = new Node(OCCControl::LOCK | OCCControl::SPLIT,
                                  root_data, NodeType::ROOT);
        const Node::Data *r_data = &new_root->data;

        l_node->parent = new_root;
        r_node->parent = new_root;
        new_root->parent = node->parent;
        *((Node **)new_root->parent) = new_root;

        new_root->occ.done_create();
        new_root->occ.unlock();
        node->occ.done_split_and_delete();
        node->occ.unlock();
        l_node->occ.done_create();
        l_node->occ.unlock();
        r_node->occ.done_create();
        r_node->occ.unlock();

        return;
    }

    Node *parent = lock_parent(node);
    l_node->parent = parent;
    r_node->parent = parent;

    Node::Data *pdata = &parent->data;
    int8_t pidx = pdata->locate(r_data->getkey(0));

    node->occ.done_split_and_delete();
    node->occ.unlock();
    l_node->occ.done_create();
    l_node->occ.unlock();
    r_node->occ.done_create();
    r_node->occ.unlock();

    insert_interior(parent, pdata, pidx, r_data->getkey(0),
                    static_cast<Node *>(l_node), static_cast<Node *>(r_node));
}

RetType insert(Node **root, uint64_t key, Value val) {
    LeafNode *leaf = locate_locked(root, key);
    Node::Data *data = &leaf->data;
    Value *val_ptr = (Value *)malloc(sizeof(Value));
    memcpy(val_ptr, &val, sizeof(Value));

    int8_t idx = data->locate(key);
    if (unlikely(data->getkey(idx) == key)) {
        leaf->occ.unlock();
        return RetType::kExists;
    }

    if (likely(data->num_keys < fanout)) {
        leaf->occ.do_insert();
        extend(data, idx, key, val_ptr, nullptr);
        leaf->occ.done_insert();
        leaf->occ.unlock();
        return RetType::kInserted;
    }

    auto [l_data_, r_data_] = divide(data, idx, key, val_ptr, nullptr);

    LeafNode *next = leaf->next;

    LeafNode *r_leaf =
        new LeafNode(OCCControl::LOCK | OCCControl::SPLIT, r_data_, next);
    LeafNode *l_leaf =
        new LeafNode(OCCControl::LOCK | OCCControl::SPLIT, l_data_, r_leaf);
    r_leaf->prev = l_leaf;

    auto *l_data = &l_leaf->data;
    auto *r_data = &r_leaf->data;

    leaf->occ.do_split();
    if (next != nullptr) next->prev = r_leaf;

    LeafNode *prev = lock_prev(leaf);
    l_leaf->prev = prev;
    if (prev != nullptr) {
        prev->next = l_leaf;
        prev->occ.unlock();
    }

    Node *parent = lock_parent(leaf);
    l_leaf->parent = parent;
    r_leaf->parent = parent;

    Node::Data *pdata = &parent->data;
    int8_t pidx = pdata->locate(l_data->getkey(0));

    leaf->occ.done_split_and_delete();
    leaf->occ.unlock();
    l_leaf->occ.done_create();
    l_leaf->occ.unlock();
    r_leaf->occ.done_create();
    r_leaf->occ.unlock();

    // const_cast<LeafNode *>(leaf)->destroy(data);

    insert_interior(parent, pdata, pidx, r_data->getkey(0),
                    static_cast<Node *>(l_leaf), static_cast<Node *>(r_leaf));

    return RetType::kInserted;
}
