// #include <format>
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

constexpr int KEY_MAX = 100000;

using namespace raw::lsmtree;
std::array<KeyT, KEY_MAX> keys;
std::array<ValueT, KEY_MAX> values;
std::map<KeyT, ValueT> data;

namespace NO_FAULT_INJECTION {

template <typename T>
void ASSERT_EQ(const T& val1, const T& val2) {
    if ((val1) != (val2)) {
        std::cerr << "Validation failed: ASSERT_EQ(" << val1 << ", " << val2 << ")\n";
        abort();
    }
}

void init() {
    constexpr int seed = 114514;
    std::mt19937 gen(seed);
    std::uniform_int_distribution dist(1, KEY_MAX);

    for (int i = 0; i < KEY_MAX; i++) {
        KeyT key = dist(gen);
        ValueT value = dist(gen);
        keys[i] = key;
        values[i] = value;
        data[key] = value;
    }
}

}  // namespace NO_FAULT_INJECTION

using namespace NO_FAULT_INJECTION;

namespace scee {
int main_fn() {
    init();

    void* lsmtree = new LSMTree("/dev/shm/lsmtree");

    for (int i = 0; i < KEY_MAX; i++) {
        auto key = keys[i];
        auto value = values[i];

        scee::run2<int>(
            app::lsmtree_set,
            validator::lsmtree_set,
            lsmtree, key, value);
        auto ret = scee::run2<int64_t>(
            app::lsmtree_get,
            validator::lsmtree_get,
            lsmtree, key);
        ASSERT_EQ(ret, value);
    }

    for (int i = 0; i < keys.size(); i++) {
        auto key = keys[i];
        auto ret = scee::run2<int64_t>(
            app::lsmtree_get,
            validator::lsmtree_get,
            lsmtree, key);
        auto v2 = data[key];
        ASSERT_EQ(ret, v2);
    }

    return 0;
}
}  // namespace scee

int main() {
    scee::main_thread(scee::main_fn);
    return 0;
}
