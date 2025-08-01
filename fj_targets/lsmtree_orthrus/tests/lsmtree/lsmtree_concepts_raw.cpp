#include <iostream>
#include <map>
#include <random>

#include "context.hpp"
#include "ctltypes.hpp"
#include "custom_stl.hpp"
#include "log.hpp"
#include "lsmtree-closure.hpp"
#include "lsmtree.hpp"
#include "namespace.hpp"
#include "ptr.hpp"
#include "scee.hpp"
#include "thread.hpp"

constexpr int KEY_MAX = 1000000;

using namespace raw::lsmtree;
std::array<KeyT, KEY_MAX> keys;
std::array<ValueT, KEY_MAX> values;
std::map<KeyT, ValueT> data;

int main_fn() {
    constexpr int seed = 114514;
    std::mt19937 gen(seed);
    std::uniform_int_distribution dist(1, KEY_MAX);

    std::cout << "Hello, World!\n";

    void* lsmtree = new LSMTree("/dev/shm/lsmtree");

    auto start = rdtsc();

    for (int i = 0; i < KEY_MAX; i++) {
        KeyT key = dist(gen);
        ValueT value = dist(gen);
        // KeyT key = i;
        // ValueT value = i;
        keys[i] = key;
        values[i] = value;
        data[key] = value;
    }

    for (int i = 0; i < KEY_MAX; i++) {
        int64_t key = keys[i];
        int64_t value = values[i];
        // printf("set %d/%d, key: %ld\n", i, KEY_MAX, key);
        ((LSMTree*)lsmtree)->Set(key, value);
        // sleep(1);
    }

    for (int i = 0; i < keys.size(); i++) {
        // printf("get %d/%d\n", i, KEY_MAX);
        int64_t key = keys[i];
        auto ret = ((LSMTree*)lsmtree)->Get(key);
        int64_t v2 = data[key];
        MYASSERT(ret == v2);
    }

    auto end = rdtsc();
    auto diff = end - start;
    printf("throughput: %lf ops\n", KEY_MAX * 2.0 * kCpuMhzNorm * 1e6 / diff);

    return 0;
}

int main() {
    scee::main_thread(main_fn);
    return 0;
}