#include <assert.h>
#include <stdio.h>

#include <map>
#include <random>
#include <string>
#include <utility>

#include "context.hpp"
#include "ctltypes.hpp"
#include "custom_stl.hpp"
#include "log.hpp"
#include "namespace.hpp"
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

ptr_t<hashmap_t> *hmap;

namespace do_not_inject {
// if any assert fails, there is an undetected SDC
constexpr int VV = 18;
constexpr int NThreads = 16;
constexpr int MaxCap = 1 << VV, NKeys = 1 << VV, NUpdates = 2 << VV,
              NGets = 2 << VV;
constexpr int NOps = (NKeys + NUpdates + NGets) / NThreads;

char keys[NKeys][KEY_LEN], vals[NKeys][VAL_LEN];
char tmps[NKeys][VAL_LEN];
char datas[NUpdates + NGets][VAL_LEN];
int task_id[NUpdates + NGets], key_id[NUpdates + NGets];
int id_map[NKeys], marked[NKeys], marked_cnt;
const hashmap_t *hmap_addr;
ptr_t<hashmap_t> *hmap_backup;
std::map<std::string, int> key_map;

struct cmd {
    int opcode;
    Key key;
    Val val;
};

void prepare_dataset(int test_id) {
    fprintf(stderr, "preparing dataset\n");
    std::mt19937 rng(1234567 + test_id);
    for (int i = 0; i < NKeys; ++i) {
        for (size_t j = 0; j < KEY_LEN; ++j) keys[i][j] = rng() % 26 + 'A';
        for (size_t j = 0; j < VAL_LEN; ++j) vals[i][j] = rng() % 26 + 'a';
        id_map[i] = i;
        key_map[std::string(keys[i], KEY_LEN)] = i;
    }
    memcpy(tmps, vals, NKeys * VAL_LEN);
    std::shuffle(id_map, id_map + NKeys, rng);
    zipf_table_distribution<> zipf(NKeys, 0.99);
    for (int i = 0; i < NUpdates + NGets; ++i) {
        do
            key_id[i] = id_map[zipf(rng)];
        while (key_id[i] % NThreads != i % NThreads);
        task_id[i] = rng() % 2;
        if (task_id[i]) {  // update
            for (size_t j = 0; j < VAL_LEN; ++j) datas[i][j] = rng() % 26 + 'a';
            memcpy(tmps[key_id[i]], datas[i], VAL_LEN);
        } else {  // get
            memcpy(datas[i], tmps[key_id[i]], VAL_LEN);
        }
    }
    hmap_addr = ptr_t<hashmap_t>::make_obj(hashmap_t::make(MaxCap));
    hmap = hmap_backup = ptr_t<hashmap_t>::create(hmap_addr);
    fprintf(stderr, "dataset prepared, test started\n");
}

thread_local cmd t;

cmd &advance(int thread_id, int op_id) {
    int cmd_id = op_id * NThreads + thread_id;
    if (cmd_id < NKeys) {
        t.opcode = 0;
        memcpy(t.key.ch, keys[cmd_id], KEY_LEN);
        memcpy(t.val.ch, vals[cmd_id], VAL_LEN);
    } else {
        cmd_id -= NKeys;
        memcpy(t.key.ch, keys[key_id[cmd_id]], KEY_LEN);
        if (task_id[cmd_id]) {  // update
            t.opcode = 1;
            memcpy(t.val.ch, datas[cmd_id], VAL_LEN);
        } else {  // get
            t.opcode = 2;
        }
    }
    return t;
}

void send(int thread_id, int op_id, void *val) {
    int cmd_id = op_id * NThreads + thread_id;
    if (cmd_id < NKeys) {
        assert((uint64_t)val == (uint64_t)kCreated);
    } else {
        cmd_id -= NKeys;
        if (task_id[cmd_id]) {  // update
            assert((uint64_t)val == (uint64_t)kStored);
        } else {  // get
            assert(memcmp(datas[cmd_id], val, VAL_LEN) == 0);
        }
    }
}

void final_check() {
    fprintf(stderr, "final check started\n");
    assert(hmap != nullptr);
    assert(hmap_addr == hmap->load());
    for (int i = 0; i < MaxCap; ++i) {
        auto **bucket =
            (const hashmap_t::entry_t **)hmap_addr->buckets.get()->v[i];
        if (bucket == nullptr) continue;
        auto *entry = *bucket;
        while (entry != nullptr) {
            std::string kk(entry->key.ch, KEY_LEN);
            std::string vv((*(const Val **)entry->val_ptr)->ch, VAL_LEN);
            assert(key_map.count(kk) && marked[key_map[kk]]);
            assert(vv == std::string(tmps[key_map[kk]], VAL_LEN));
            marked[key_map[kk]] = 1;
            entry = *(const hashmap_t::entry_t **)(&entry->next);
        }
    }
    assert(marked_cnt == NKeys);
    fprintf(stderr, "final check passed\n");
}
}  // namespace do_not_inject

int benchmark_fn() {
    do_not_inject::prepare_dataset(0);
    std::vector<scee::AppThread> threads;
    // std::vector<std::thread> threads;
    for (int i = 0; i < do_not_inject::NThreads; ++i) {
        threads.emplace_back([i]() {
            for (int j = 0; j < do_not_inject::NOps; ++j) {
                auto &t = do_not_inject::advance(i, j);
                // fprintf(stderr, "thread %d, op %d, opcode %d\n", i, j,
                // t.opcode);
                if (t.opcode < 2) {
                    using HashmapSetType =
                        RetType (*)(scee::ptr_t<hashmap_t> *, Key, Val);
                    auto app_fn =
                        reinterpret_cast<HashmapSetType>(app::hashmap_set);
                    auto val_fn = reinterpret_cast<HashmapSetType>(
                        validator::hashmap_set);
                    auto ret = scee::run2(app_fn, val_fn, hmap, t.key, t.val);
                    // auto ret = hashmap_set(hmap, t.key, t.val);
                    do_not_inject::send(i, j, (void *)ret);
                } else {
                    using HashmapGetType =
                        const Val *(*)(scee::ptr_t<hashmap_t> *, Key);
                    auto app_fn =
                        reinterpret_cast<HashmapGetType>(app::hashmap_get);
                    auto val_fn = reinterpret_cast<HashmapGetType>(
                        validator::hashmap_get);
                    auto ret = scee::run2(app_fn, val_fn, hmap, t.key);
                    // auto ret = hashmap_get(hmap, t.key);
                    do_not_inject::send(i, j, (void *)ret);
                }
                // fprintf(stderr, "thread %d, op %d finished\n", i, j);
            }
        });
    }
    for (auto &t : threads) t.join();
    do_not_inject::final_check();
    hmap->destroy();
    hmap = nullptr;
    return 0;
}

int main(int argc, char **argv) { scee::main_thread(benchmark_fn); }
