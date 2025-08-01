constexpr size_t VAL_LEN = 16;

enum NodeType : uint64_t { LEAF, INTERIOR, ROOT };

struct Value {
    char ch[VAL_LEN];
};

static constexpr int8_t fanout = 29, fanout_highbit = 16;
// static constexpr int8_t fanout = 7, fanout_highbit = 4;
static_assert(fanout_highbit * 2 > fanout && fanout_highbit <= fanout);
static_assert(fanout % 2 == 1);

/**
 * Each node of the B+ tree controls range [l, r] of keys.
 * Normal inserts that do not cause splits do not change this range.
 * Splits cause the range to be splitted.
 * If reads going down without ignoring a range, the result is correct.
 * There is only one version for each node, but many versions for node data.
 */
struct Node {
    struct Data {
        void *links[fanout];
        uint64_t keys[fanout];
        int8_t sorted[fanout], num_keys;
        char padding[7 - fanout % 8];
        int8_t locate(uint64_t key) const;
        uint64_t getkey(int8_t idx) const;
        const Node *child(int8_t idx) const;
        scee::ptr_t<Value> *value(int8_t idx) const;
    };
    scee::NodeType node_type;
    OCCControl *get_occ() const;
    scee::ptr_t<Node> *get_parent() const;
    scee::ptr_t<Data> *get_data() const;
};

inline uint64_t Node::Data::getkey(int8_t idx) const {
    return keys[sorted[idx]];
}

inline const Node *Node::Data::child(int8_t idx) const {
    return static_cast<const Node *>(links[sorted[idx]]);
}

inline ptr_t<Value> *Node::Data::value(int8_t idx) const {
    return static_cast<ptr_t<Value> *>(links[sorted[idx]]);
}

struct LeafNode : Node {
    ptr_t<LeafNode> *get_prev() const;
    ptr_t<LeafNode> *get_next() const;
};

scee::ptr_t<Node> *build_tree();
uint8_t insert(scee::ptr_t<Node> *root, uint64_t key, Value val);
const Value *update(scee::ptr_t<Node> *root, uint64_t key, Value val);
const Value *read(scee::ptr_t<Node> *root, uint64_t key);
uint64_t scan_and_sum(scee::ptr_t<Node> *root, uint64_t key, uint64_t num_keys);
scee::ptr_t<Node> *build_tree_from_keys(std::vector<uint64_t> &keys,
                                        std::vector<Value> &vals);
