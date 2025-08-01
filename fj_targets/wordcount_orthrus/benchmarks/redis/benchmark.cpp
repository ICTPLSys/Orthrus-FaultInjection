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

std::mt19937 rng(1234567);

std::map<std::string, Val> dict;

std::pair<Key, Val> mkentry(const int alphaSize, bool inside = false) {
    Key key;
    Val val;
    while (true) {
        for (size_t i = 0; i < KEY_LEN; ++i)
            key.ch[i] = rng() % alphaSize + 'A';
        if (inside) {
            assert(dict.size() > 0);
            auto it = dict.lower_bound(key.to_string());
            if (it == dict.end()) it = dict.begin();
            for (size_t i = 0; i < KEY_LEN; ++i) key.ch[i] = it->first[i];
            break;
        } else if (!dict.count(key.to_string()))
            break;
    }
    for (size_t i = 0; i < VAL_LEN; ++i) val.ch[i] = rng() % alphaSize + 'a';
    return std::make_pair(key, val);
}

constexpr int MaxCap = 1 << 23, NSets = 1 << 23, NUpdates = 1 << 23,
              NGets = 1 << 25, Alphabet = 26;
constexpr int NPrints = 4;

enum RunType {
    Baseline,
    SCEE,
    SCEEProfile,
};

template <RunType RT>
void benchmark_fn() {
    rng.seed(1);
    hashmap_t hmap_unsafe = hashmap_t::make(MaxCap);
    ptr_t<hashmap_t> *hm_safe = ptr_t<hashmap_t>::create(hmap_unsafe);

    uint64_t sum_rdtsc;
    sum_rdtsc = 0;

    for (int i = 0; i < NSets; ++i) {
        std::pair<Key, Val> entry = mkentry(Alphabet, 0);
        dict[entry.first.to_string()] = entry.second;
        if constexpr (RT == RunType::Baseline) {
            uint64_t start = _rdtsc();
            RetType ret = hashmap_set(hm_safe, entry.first, entry.second);
            sum_rdtsc += _rdtsc() - start;
            assert(ret == kCreated);
        } else {
            using HashmapSetType =
                RetType (*)(scee::ptr_t<hashmap_t> *, Key, Val);
            HashmapSetType app_fn =
                reinterpret_cast<HashmapSetType>(app::hashmap_set);
            HashmapSetType val_fn =
                reinterpret_cast<HashmapSetType>(validator::hashmap_set);
            RetType ret;
            if constexpr (RT == RunType::SCEE) {
                uint64_t start = _rdtsc();
                ret = scee::run2(app_fn, val_fn, hm_safe, entry.first,
                                 entry.second);
                sum_rdtsc += _rdtsc() - start;
            } else {
                uint64_t cycles;
                ret = scee::run2_profile(cycles, app_fn, val_fn, hm_safe,
                                         entry.first, entry.second);
                sum_rdtsc += cycles;
            }
            assert(ret == kCreated);
        }
        if ((i + 1) % (NSets / NPrints) == 0) {
            fprintf(stderr, "Set %d keys: time = %lu\n", NSets / NPrints,
                    sum_rdtsc / (NSets / NPrints));
            sum_rdtsc = 0;
        }
    }

    for (int i = 0; i < NUpdates; ++i) {
        std::pair<Key, Val> entry = mkentry(Alphabet, 1);
        dict[entry.first.to_string()] = entry.second;
        if constexpr (RT == RunType::Baseline) {
            uint64_t start = _rdtsc();
            RetType ret = hashmap_set(hm_safe, entry.first, entry.second);
            sum_rdtsc += _rdtsc() - start;
            assert(ret == kStored);
        } else {
            using HashmapSetType =
                RetType (*)(scee::ptr_t<hashmap_t> *, Key, Val);
            HashmapSetType app_fn =
                reinterpret_cast<HashmapSetType>(app::hashmap_set);
            HashmapSetType val_fn =
                reinterpret_cast<HashmapSetType>(validator::hashmap_set);
            RetType ret;
            if constexpr (RT == RunType::SCEE) {
                uint64_t start = _rdtsc();
                ret = scee::run2(app_fn, val_fn, hm_safe, entry.first,
                                 entry.second);
                sum_rdtsc += _rdtsc() - start;
            } else {
                uint64_t cycles;
                ret = scee::run2_profile(cycles, app_fn, val_fn, hm_safe,
                                         entry.first, entry.second);
                sum_rdtsc += cycles;
            }
            assert(ret == kStored);
        }
        if ((i + 1) % (NUpdates / NPrints) == 0) {
            fprintf(stderr, "Update %d keys: time = %lu\n", NUpdates / NPrints,
                    sum_rdtsc / (NUpdates / NPrints));
            sum_rdtsc = 0;
        }
    }

    for (int i = 0; i < NGets; ++i) {
        std::pair<Key, Val> entry = mkentry(Alphabet, 1);
        if constexpr (RT == RunType::Baseline) {
            uint64_t start = _rdtsc();
            Val ret = *hashmap_get(hm_safe, entry.first);
            sum_rdtsc += _rdtsc() - start;
            assert(ret == dict[entry.first.to_string()]);
        } else {
            using HashmapGetType =
                const Val *(*)(scee::ptr_t<hashmap_t> *, Key);
            HashmapGetType app_fn =
                reinterpret_cast<HashmapGetType>(app::hashmap_get);
            HashmapGetType val_fn =
                reinterpret_cast<HashmapGetType>(validator::hashmap_get);
            Val ret;
            if constexpr (RT == RunType::SCEE) {
                uint64_t start = _rdtsc();
                ret = *scee::run2(app_fn, val_fn, hm_safe, entry.first);
                sum_rdtsc += _rdtsc() - start;
            } else {
                uint64_t cycles;
                ret = *scee::run2_profile(cycles, app_fn, val_fn, hm_safe,
                                          entry.first);
                sum_rdtsc += cycles;
            }
            assert(ret == dict[entry.first.to_string()]);
        }
        if ((i + 1) % (NGets / NPrints) == 0) {
            fprintf(stderr, "Get %d keys: time = %lu\n", NGets / NPrints,
                    sum_rdtsc / (NGets / NPrints));
            sum_rdtsc = 0;
        }
    }

    destroy_obj(const_cast<hashmap_t *>(hm_safe->load()));
    hm_safe->destroy();
    dict.clear();
    fprintf(stderr, "Test passed!!!\n");
}

int main_fn(RunType rt) {
    switch (rt) {
    case RunType::Baseline:
        benchmark_fn<RunType::Baseline>();
        break;
    case RunType::SCEE:
        benchmark_fn<RunType::SCEE>();
        break;
    case RunType::SCEEProfile:
        benchmark_fn<RunType::SCEEProfile>();
        break;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [baseline|scee|scee-profile]\n", argv[0]);
        return 1;
    }
    if (strcmp(argv[1], "baseline") == 0) {
        scee::main_thread(main_fn, RunType::Baseline);
    } else if (strcmp(argv[1], "scee") == 0) {
        scee::main_thread(main_fn, RunType::SCEE);
    } else if (strcmp(argv[1], "scee-profile") == 0) {
        scee::main_thread(main_fn, RunType::SCEEProfile);
    } else {
        fprintf(stderr, "Usage: %s [baseline|scee|scee-profile]\n", argv[0]);
        return 1;
    }
    return 0;
}

/* Usage:
rm -r build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
taskset -c 0-3 ./build/benchmarks/redis/redis_benchmark scee
*/
