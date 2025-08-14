#include <stdio.h>

#include <atomic>
#include <map>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "context.hpp"
#include "ctltypes.hpp"
#include "custom_stl.hpp"
#include "log.hpp"
#include "namespace.hpp"
#include "occ.hpp"
#include "ptr.hpp"
#include "scee.hpp"
#include "thread.hpp"
#include "utils.hpp"

namespace raw {
#include "closure.hpp"
}  // namespace raw
namespace app {
#include "closure.hpp"
}  // namespace app
namespace validator {
#include "closure.hpp"
}  // namespace validator

using namespace raw;

ptr_t<Node> *root;

namespace do_not_inject {
#define sdc_assert(condition)                                    \
    if (!(condition)) {                                                \
        fprintf(stderr, "SDC Not Detected at %s:%d (%s)\n", __FILE__, \
                __LINE__, #condition);                                 \
        std::abort();                                           \
    }

    constexpr int kNThreads = 4, kNKeysPerThread = 20;
    uint64_t keys[kNThreads][kNKeysPerThread];
    Value values[kNThreads][kNKeysPerThread];
    ptr_t<Node> *root_backup;
    const Node *root_ptr;
    std::map<uint64_t, Value> kv_map;
    void prepare_dataset(){
        std::mt19937 gen(123456789);
        std::uniform_int_distribution<> dis(0, 1000000000);
        for (int i = 0; i < kNThreads; ++i) {
            for (int j = 0; j < kNKeysPerThread; ++j) {
                keys[i][j] = gen() % 100000 + 1; // (uint64_t(gen()) << 32) ^ gen();
                for (char &c : values[i][j].ch) {
                    c = (char)(gen() % 26 + 'a');
                }
                kv_map[keys[i][j]] = values[i][j];
            }
        }
        root_backup = root = build_tree();
    }
    std::pair<uint64_t, Value> get_kv(int thread_id, int op_id) {
        return {keys[thread_id][op_id], values[thread_id][op_id]};
    }

    std::vector<const LeafNode *> leaf_nodes;
    std::vector<uint64_t> leaf_keys;
    void traverse_tree(const Node *node, const Node *parent) {
        assert(!(node->occ->ver.load(std::memory_order_relaxed) & OCCControl::DELETED));
        assert(*(Node **)node->parent == parent);
        const Node::Data *data = *(Node::Data **)node->data;
        if (node->node_type == NodeType::LEAF) {
            const LeafNode *leaf = static_cast<const LeafNode *>(node);
            leaf_nodes.emplace_back(leaf);
            for (int i = 0; i < data->num_keys; ++i) {
                // sdc_assert(data->sorted[i] < data->num_keys && data->sorted[i] >= 0);
                if (leaf_keys.size() > 0) {
                    // sdc_assert(kv_map.find(data->keys[data->sorted[i]]) != kv_map.end());
                    const Value *val = *(Value **)(data->links[data->sorted[i]]);
                    sdc_assert(val != nullptr);
                    // fprintf(stderr, "CHK key: %lu, val: %s\n", data->keys[data->sorted[i]], std::string(val->ch, VAL_LEN).c_str());
                    sdc_assert(memcmp(val->ch, kv_map[data->keys[data->sorted[i]]].ch, VAL_LEN) == 0);
                }
                leaf_keys.emplace_back(data->keys[data->sorted[i]]);
            }
        } else {
            if (node->node_type != NodeType::INTERIOR) {
                assert(node == root_ptr);
            }
            for (int i = 0; i < data->num_keys; ++i) {
                sdc_assert(data->sorted[i] < data->num_keys && data->sorted[i] >= 0);
                const Node *child = static_cast<const Node *>(data->links[data->sorted[i]]);
                const Node::Data *child_data = *(Node::Data **)child->data;
                sdc_assert(child_data->keys[child_data->sorted[0]] == data->keys[data->sorted[i]]);
                traverse_tree(child, node);
            }
        }
    }
    void final_check() {
        sdc_assert(root == root_backup);
        root_ptr = *(Node **)root;
        traverse_tree(root_ptr, reinterpret_cast<Node *>(root));
        sdc_assert(leaf_keys.size() == kv_map.size() + 1);
        // for (size_t i = 1; i < leaf_keys.size(); ++i) {
        //     sdc_assert(leaf_keys[i] > leaf_keys[i - 1]);
        // }
        for (size_t i = 0; i < leaf_nodes.size(); ++i) {
            const LeafNode *exp_prev = i > 0 ? leaf_nodes[i - 1] : nullptr;
            const LeafNode *exp_next = i < leaf_nodes.size() - 1 ? leaf_nodes[i + 1] : nullptr;
            const LeafNode *leaf = leaf_nodes[i];
            sdc_assert(*(LeafNode **)leaf->prev == exp_prev);
            sdc_assert(*(LeafNode **)leaf->next == exp_next);
        }
    }
}

int main_fn() {
    do_not_inject::prepare_dataset();

    std::vector<scee::AppThread> threads;
    for (int t = 0; t < do_not_inject::kNThreads; ++t) {
        threads.emplace_back([t]() {
            for (int i = 0; i < do_not_inject::kNKeysPerThread; ++i) {
                auto [key, val] = do_not_inject::get_kv(t, i);
                // fprintf(stderr, "INS key: %lu, val: %s\n", key, std::string(val.ch, VAL_LEN).c_str());
                using InsertType = uint8_t (*)(scee::ptr_t<Node> *, uint64_t, Value);
                auto app_fn = reinterpret_cast<InsertType>(app::insert);
                auto val_fn = reinterpret_cast<InsertType>(validator::insert);
                scee::run2(app_fn, val_fn, root, key, val);
            }
        });
    }
    for (auto &t : threads) t.join();
    threads.clear();

    sleep(1);

    do_not_inject::final_check();

    fprintf(stderr, "fault injection test passed\n");
    return 0;
}

int main(int argc, char *argv[]) {
    scee::main_thread(main_fn);
    return 0;
}