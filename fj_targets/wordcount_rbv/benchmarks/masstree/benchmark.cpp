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

std::string spaces;
std::vector<uint64_t> key_list;

void dump_tree(const Node *node, int depth) {
    // assert(node != nullptr &&
    //        !(node->occ->stable_version() & OCCControl::DELETED));
    const Node::Data *data = node->data->load();
    if (depth > 0) fprintf(stderr, "%s", spaces.substr(0, depth * 2).c_str());
    if (node->node_type == NodeType::LEAF) {
        const LeafNode *leaf = static_cast<const LeafNode *>(node);
        fprintf(stderr, "leaf:");
        for (int i = 0; i < data->num_keys; ++i) {
            fprintf(stderr, " %zu", data->getkey(i));
            key_list.push_back(data->getkey(i));
        }
        fprintf(stderr, "\n");
    } else {
        fprintf(stderr, "interior:");
        for (int i = 0; i < data->num_keys; ++i)
            fprintf(stderr, " %zu", data->getkey(i));
        fprintf(stderr, "\n");
        for (int i = 0; i < data->num_keys; ++i)
            dump_tree(data->child(i), depth + 1);
    }
}

uint64_t vectorsum(const std::vector<uint64_t> &vec) {
    uint64_t sum = 0;
    for (uint64_t v : vec) sum += v;
    return sum;
}

constexpr int kNTests = 1, kThreads = 16, kStart = 1E7 / kThreads,
              kOpsPerThread = 9E7 / kThreads;
// constexpr int kNTests = 1, kThreads = 16, kStart = 10000, kOpsPerThread =
// 10000; constexpr int kNTests = 1, kThreads = 2, kStart = 10000, kOpsPerThread
// = 10000;
constexpr int p_all = 90, p_insert = 5, p_update = 20;

enum RunType {
    Baseline,
    SCEE,
    SCEEProfile,
};

template <RunType RT, typename ThreadType>
int main_fn() {
    for (int i = 0; i < 100; ++i) spaces += " ";
    for (int _ = 0; _ < kNTests; ++_) {
        fprintf(stderr, "Evaluating test #%d\n", _);

        ptr_t<Node> *root = build_tree();
        fprintf(stderr, "root pointed by %p\n", root);
        int seed = _ * kThreads * 2;

        std::vector<uint64_t> basic_keys(kThreads * kStart);
        std::vector<Value> values(kThreads * kStart);
        std::vector<ThreadType> threads;

        for (int t = 1; t <= kThreads; ++t) {
            threads.emplace_back([t, seed, &basic_keys, &values]() {
                std::mt19937 rng(t + seed);
                for (int i = 1; i <= kStart; ++i) {
                    int idx = (t - 1) * kStart + (i - 1);
                    basic_keys[idx] = (rng() << 32) | rng();
                    for (char &c : values[idx].ch) {
                        c = (char)(rng() % 26 + 'a');
                    }
                }
            });
        }
        for (auto &t : threads) t.join();
        threads.clear();
        fprintf(stderr, "kv pairs prepared!\n");

        uint64_t clk_init = rdtsc();
        for (int t = 1; t <= kThreads; ++t) {
            threads.emplace_back([root, t, seed, &basic_keys, &values]() {
                std::mt19937 rng(t + seed);
                for (int i = 1; i <= kStart; ++i) {
                    int idx = (t - 1) * kStart + (i - 1);
                    uint8_t ret;
                    if constexpr (RT == RunType::Baseline) {
                        ret = insert(root, basic_keys[idx], values[idx]);
                    } else {
                        using InsertType =
                            uint8_t (*)(scee::ptr_t<Node> *, uint64_t, Value);
                        auto app_fn = reinterpret_cast<InsertType>(app::insert);
                        auto val_fn =
                            reinterpret_cast<InsertType>(validator::insert);
                        ret = scee::run2(app_fn, val_fn, root, basic_keys[idx],
                                         values[idx]);
                    }
                    assert(ret == 1);
                }
            });
        }
        for (auto &t : threads) t.join();
        threads.clear();

        // dump_tree(root->load(), 0);
        // for (size_t i = 1; i < key_list.size(); ++i) {
        //     assert(key_list[i] > key_list[i - 1]);
        // }

        uint64_t ms_count = microsecond(clk_init, rdtsc());
        fprintf(stderr, "INSERT %.3fM keys, throughput = %.3f Mops/s\n",
                kThreads * kStart * 1e-6, (kThreads * kStart * 1.0) / ms_count);

        fprintf(stderr, "pass inserting phase\n");

        std::vector<uint64_t> readcnt(kThreads + 1);
        std::vector<uint64_t> insertcnt(kThreads + 1);
        std::vector<uint64_t> updatecnt(kThreads + 1);
        std::vector<uint64_t> readtime(kThreads + 1);
        std::vector<uint64_t> inserttime(kThreads + 1);
        std::vector<uint64_t> updatetime(kThreads + 1);
        for (int t = 1; t <= kThreads; ++t) {
            threads.emplace_back([root, t, seed, &basic_keys, &readcnt,
                                  &insertcnt, &updatecnt, &readtime,
                                  &inserttime, &updatetime]() {
                std::mt19937 rng(t + kThreads + seed);
                for (int i = 1; i <= kOpsPerThread; ++i) {
                    int p = rng() % p_all;
                    if (p < p_insert) {
                        uint64_t key = (rng() << 32) | rng();
                        Value val;
                        for (size_t j = 0; j < VAL_LEN; ++j)
                            val.ch[j] = rng() % 26 + 'a';
                        uint64_t start = rdtsc();
                        insertcnt[t]++;
                        uint8_t ret;
                        if constexpr (RT == RunType::Baseline) {
                            ret = insert(root, key, val);
                        } else {
                            using InsertType = uint8_t (*)(scee::ptr_t<Node> *,
                                                           uint64_t, Value);
                            auto app_fn =
                                reinterpret_cast<InsertType>(app::insert);
                            auto val_fn =
                                reinterpret_cast<InsertType>(validator::insert);
                            ret = scee::run2(app_fn, val_fn, root, key, val);
                        }
                        inserttime[t] += rdtsc() - start;
                        assert(ret == 1);
                    } else if (p < p_insert + p_update) {
                        int idx = rng() % (kThreads * kStart);
                        uint64_t key = basic_keys[idx];
                        Value val;
                        for (size_t j = 0; j < VAL_LEN; ++j)
                            val.ch[j] = rng() % 26 + 'a';
                        uint64_t start = rdtsc();
                        updatecnt[t]++;
                        const Value *ret;
                        if constexpr (RT == RunType::Baseline) {
                            ret = update(root, key, val);
                        } else {
                            using UpdateType =
                                const Value *(*)(scee::ptr_t<Node> *, uint64_t,
                                                 Value);
                            auto app_fn =
                                reinterpret_cast<UpdateType>(app::update);
                            auto val_fn =
                                reinterpret_cast<UpdateType>(validator::update);
                            ret = scee::run2(app_fn, val_fn, root, key, val);
                        }
                        updatetime[t] += rdtsc() - start;
                        assert(ret != nullptr);
                        // TODO: reclaim value resource here
                    } else {
                        int idx = rng() % (kThreads * kStart);
                        uint64_t key = basic_keys[idx];
                        uint64_t start = rdtsc();
                        readcnt[t]++;
                        const Value *ret;
                        if constexpr (RT == RunType::Baseline) {
                            ret = read(root, key);
                        } else {
                            using ReadType =
                                const Value *(*)(scee::ptr_t<Node> *, uint64_t);
                            auto app_fn = reinterpret_cast<ReadType>(app::read);
                            auto val_fn =
                                reinterpret_cast<ReadType>(validator::read);
                            ret = scee::run2(app_fn, val_fn, root, key);
                        }
                        readtime[t] += rdtsc() - start;
                        assert(ret != nullptr);
                    }
                }
            });
        }
        for (auto &t : threads) t.join();
        threads.clear();

        fprintf(stderr,
                "%.3fM READ, %.3fM UPDATE, %.3fM INSERT workload finished!\n",
                vectorsum(readcnt) * 1e-6, vectorsum(updatecnt) * 1e-6,
                vectorsum(insertcnt) * 1e-6);
        double read_mops =
            vectorsum(readcnt) * 1.0 / microsecond(0, vectorsum(readtime));
        double update_mops =
            vectorsum(updatecnt) * 1.0 / microsecond(0, vectorsum(updatetime));
        double insert_mops =
            vectorsum(insertcnt) * 1.0 / microsecond(0, vectorsum(inserttime));
        fprintf(stderr,
                "READ: %.3f Mops/s, UPDATE: %.3f Mops/s, INSERT: %.3f Mops/s\n",
                read_mops * kThreads, update_mops * kThreads,
                insert_mops * kThreads);

        // dump_tree(root->load(), 0);
        // fprintf(stderr, "key_list size: %zu\n", key_list.size());
        // for (size_t i = 0; i + 1 < key_list.size(); ++i) {
        //     assert(key_list[i] < key_list[i + 1]);
        // }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [baseline|scee]\n", argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "baseline") == 0) {
        scee::main_thread(main_fn<RunType::Baseline, std::thread>);
    } else if (strcmp(argv[1], "scee") == 0) {
        scee::main_thread(main_fn<RunType::SCEE, scee::AppThread>);
    } else {
        fprintf(stderr, "Usage: %s [baseline|scee]\n", argv[0]);
        return 1;
    }
    return 0;
}