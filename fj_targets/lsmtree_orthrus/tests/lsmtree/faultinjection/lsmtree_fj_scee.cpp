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

template <typename T>
void ASSERT_EQ_FINAL(const T& val1, const T& val2) {
    if ((val1) != (val2)) {
        std::cerr << "SDC Not Detected Test failed: ASSERT_EQ(" << val1 << ", " << val2 << ")\n";
        abort();
    }
}

void init() {
    constexpr int seed = 114514;
    std::mt19937 gen(seed);
    std::uniform_int_distribution dist(1, KEY_MAX);

    for (int i = 0; i < KEY_MAX; i++) {
        // KeyT key = dist(gen);
        ValueT value = dist(gen);
        keys[i] = (uint64_t)i;
        values[i] = (uint64_t)(value);
        data[i] = value;
    }
}

    static __attribute__((target("sse4.2"))) uint32_t kompute_crc32_no_fault(
        const void* data, std::size_t length) {
        std::uint32_t crc = ~0U;
        const auto* buffer = static_cast<const unsigned char*>(data);
    
        while (length > 0 && reinterpret_cast<std::uintptr_t>(buffer) % 8 != 0) {
            crc = _mm_crc32_u8(crc, *buffer);
            buffer++;
            length--;
        }
    
        const auto* buffer64 = reinterpret_cast<const std::uint64_t*>(buffer);
        while (length >= 8) {
            crc = _mm_crc32_u64(crc, *buffer64);
            buffer64++;
            length -= 8;
        }
    
        buffer = reinterpret_cast<const unsigned char*>(buffer64);
    
        if (length >= 4) {
            auto value32 = *reinterpret_cast<const std::uint32_t*>(buffer);
            crc = _mm_crc32_u32(crc, value32);
            buffer += 4;
            length -= 4;
        }
    
        if (length >= 2) {
            auto value16 = *reinterpret_cast<const std::uint16_t*>(buffer);
            crc = _mm_crc32_u16(crc, value16);
            buffer += 2;
            length -= 2;
        }
    
        if (length > 0) {
            crc = _mm_crc32_u8(crc, *buffer);
        }
    
        return ~crc;
    }


double get_as_time_no_fault(int64_t v) {
    double ret = 0;
    ret += (double)(v / 24.0 / 60.0 / 60.0);
    ret += (double)(v / 60.0 / 60.0);
    ret += (double)(v / 60.0);
    ret += (double)(v % 60);
    return ret;
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
        auto ret = scee::run2<int>(
            app::lsmtree_set,
            validator::lsmtree_set,
            lsmtree, key, value);
    }

    for (int i = 0; i < KEY_MAX; i++) {
        auto key = keys[i];
        auto value = values[i];

        auto ret = scee::run2<int64_t>(
            app::lsmtree_get,
            validator::lsmtree_get,
            lsmtree, key);
        ASSERT_EQ(ret, value);


        auto ret2 = scee::run2<uint32_t>(
            app::lsmtree_get_as_crc32,
            validator::lsmtree_get_as_crc32,
            lsmtree, key);
        // std::cerr << "ret2 = " << ret2 << std::endl;
        ASSERT_EQ(ret2, kompute_crc32_no_fault(&value, sizeof(value)));

        auto ret3 = scee::run2<double>(
            app::lsmtree_get_as_time,
            validator::lsmtree_get_as_time,
            lsmtree, key);
        ASSERT_EQ(ret3, get_as_time_no_fault(value));
    }

    sleep(2);

    std::vector<int64_t> ret_values;
    std::vector<uint32_t> ret_crc32s;
    std::vector<double> ret_times;

    for (int i = 0; i < keys.size(); i++) {
        auto key = keys[i];
        auto ret = scee::run2<int64_t>(
            app::lsmtree_get,
            validator::lsmtree_get,
            lsmtree, key);
        auto v2 = data[key];
        ret_values.push_back(ret);

        auto ret2 = scee::run2<uint32_t>(
            app::lsmtree_get_as_crc32,
            validator::lsmtree_get_as_crc32,
            lsmtree, key);
        ret_crc32s.push_back(ret2);

        auto ret3 = scee::run2<double>(
            app::lsmtree_get_as_time,
            validator::lsmtree_get_as_time,
            lsmtree, key);
        ret_times.push_back(ret3);
    }

    sleep(2);

    for (int i = 0; i < ret_values.size(); i++) {
        // ASSERT_EQ_FINAL(ret_values[i], data[keys[i]]);
        ASSERT_EQ_FINAL(ret_crc32s[i], kompute_crc32_no_fault(&ret_values[i], sizeof(ret_values[i])));
        ASSERT_EQ_FINAL(ret_times[i], get_as_time_no_fault(ret_values[i]));
    }



    return 0;
}
}  // namespace scee

int main() {
    scee::main_thread(scee::main_fn);
    return 0;
}
