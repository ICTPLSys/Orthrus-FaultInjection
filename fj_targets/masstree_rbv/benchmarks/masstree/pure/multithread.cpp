#include <stdio.h>

#include <atomic>
#include <cassert>
#include <map>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "compiler.hpp"
#include "masstree.hpp"
#include "utils.hpp"

std::string spaces;
std::vector<uint64_t> key_list;

void dump_tree(Node *node, int depth) {
    assert(node != nullptr &&
           !(node->occ.stable_version() & OCCControl::DELETED));
    Node::Data *data = &node->data;
    if (depth > 0) fprintf(stderr, "%s", spaces.substr(0, depth * 2).c_str());
    if (node->node_type == NodeType::LEAF) {
        LeafNode *leaf = static_cast<LeafNode *>(node);
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

// constexpr int kNTests = 1, kThreads = 3, kStart = 1E6;
constexpr int kNTests = 1, kThreads = 16, kStart = 1E7 / kThreads,
              kOpsPerThread = 9E7 / kThreads;

int main(int argc, char *argv[]) {
    for (int i = 0; i < 100; ++i) spaces += " ";
    for (int _ = 0; _ < kNTests; ++_) {
        fprintf(stderr, "Evaluating test #%d\n", _);

        Node **root = build_tree();
        fprintf(stderr, "root pointed by %p\n", root);
        int seed = _ * kThreads * 2;

        std::vector<uint64_t> basic_keys(kThreads * kStart);
        std::vector<Value> values(kThreads * kStart);
        std::vector<std::thread> threads;

        for (int t = 1; t <= kThreads; ++t) {
            threads.emplace_back([root, t, seed, &basic_keys, &values]() {
                std::mt19937 rng(t + seed);
                for (int i = 1; i <= kStart; ++i) {
                    int idx = (t - 1) * kStart + (i - 1);
                    basic_keys[idx] = (rng() << 32) | rng();
                    for (size_t j = 0; j < VAL_LEN; ++j)
                        values[idx].ch[j] = rng() % 26 + 'a';
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
                    RetType ret = insert(root, basic_keys[idx], values[idx]);
                    assert(ret == RetType::kInserted);
                }
            });
        }
        for (auto &t : threads) t.join();
        threads.clear();

        uint64_t ms_count = microsecond(clk_init, rdtsc());
        fprintf(stderr, "INSERT %.3fM keys, throughput = %.3f Mops/s\n",
                kThreads * kStart * 1e-6, (kThreads * kStart * 1.0) / ms_count);

        std::vector<uint64_t> readcnt(kThreads), insertcnt(kThreads),
            updatecnt(kThreads);
        std::vector<uint64_t> readtime(kThreads), inserttime(kThreads),
            updatetime(kThreads);
        for (int t = 1; t <= kThreads; ++t) {
            threads.emplace_back([root, t, seed, &basic_keys, &readcnt,
                                  &insertcnt, &updatecnt, &readtime,
                                  &inserttime, &updatetime]() {
                std::mt19937 rng(t + kThreads + seed);
                for (int i = 1; i <= kOpsPerThread; ++i) {
                    int p = rng() % 90;
                    if (p < 5) {
                        uint64_t key = (rng() << 32) | rng();
                        Value val;
                        for (size_t j = 0; j < VAL_LEN; ++j)
                            val.ch[j] = rng() % 26 + 'a';
                        uint64_t start = rdtsc();
                        insertcnt[t]++;
                        RetType ret = insert(root, key, val);
                        inserttime[t] += rdtsc() - start;
                        assert(ret == RetType::kInserted);
                    } else if (p < 25) {
                        int idx = rng() % (kThreads * kStart);
                        uint64_t key = basic_keys[idx];
                        Value val;
                        for (size_t j = 0; j < VAL_LEN; ++j)
                            val.ch[j] = rng() % 26 + 'a';
                        uint64_t start = rdtsc();
                        updatecnt[t]++;
                        RetType ret = update(root, key, val);
                        updatetime[t] += rdtsc() - start;
                        assert(ret == RetType::kUpdated);
                    } else {
                        int idx = rng() % (kThreads * kStart);
                        uint64_t key = basic_keys[idx];
                        uint64_t start = rdtsc();
                        readcnt[t]++;
                        const Value *ret = read(root, key);
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
