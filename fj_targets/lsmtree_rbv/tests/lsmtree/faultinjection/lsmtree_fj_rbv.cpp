// #include <format>
#include <iostream>
#include <map>
#include <random>

#define NAMESPACE app
#include "lsmtree.hpp"
#include "lsmtree-closure.hpp"


constexpr int KEY_MAX = 100000;

std::array<uint64_t, KEY_MAX> keys;
std::array<uint64_t, KEY_MAX> values;
std::map<uint64_t, uint64_t> data;

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
        auto key = dist(gen);
        auto value = dist(gen);
        keys[i] = key;
        values[i] = value;
        data[key] = value;
    }
}

static __attribute__((target("sse4.2"))) uint32_t kompute_crc32_special(
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

static uint64_t calc_crc_special(uint64_t key, uint64_t value, uint64_t is_delete) {
    uint8_t buf[sizeof(key) + sizeof(value) + sizeof(is_delete)];
    *(decltype(value)*)(buf) = value;
    *(decltype(key)*)(buf+sizeof(value)) = key;
    *(decltype(is_delete)*)(buf+sizeof(value)+sizeof(key)) = is_delete;
    auto crc = kompute_crc32_special(buf, sizeof(buf));
    return crc;
}

double get_time_info_nofj(uint64_t value) {
    double ret = 0;
    ret += (double)(value / 24.0 / 60.0 / 60.0);
    ret += (double)(value / 60.0 / 60.0);
    ret += (double)(value / 60.0);
    ret += (double)(value % 60);
    return ret;
}

}  // namespace NO_FAULT_INJECTION


using namespace NO_FAULT_INJECTION;

namespace scee {
int main_fn() {
    init();

    void* lsm_main = new app::lsmtree::LSMTree("/dev/shm/lsmtree-main");
    void* lsm_replica = new app::lsmtree::LSMTree("/dev/shm/lsmtree-replica");

    for (int i = 0; i < KEY_MAX; i++) {
        uint64_t key = keys[i];
        uint64_t value = values[i];
        // fprintf(stderr, "key = %lu, value = %lu\n", key, value);

        // === begin simulate rbv ===
        // fprintf(stderr, "main set\n");
        auto ret1 = (uint64_t)app::lsmtree_set(lsm_main, key, value);
        // fprintf(stderr, "replica set\n");
        auto ret2 = (uint64_t)validator::lsmtree_set(lsm_replica, key, value);
        ASSERT_EQ(ret1, ret2);
        const auto& lsm_main_states = app::lsmtree_get_internal_states(lsm_main);
        const auto& lsm_replica_states = validator::lsmtree_get_internal_states(lsm_replica);
        bool is_same = lsm_main_states == lsm_replica_states;
        if (!is_same) {
            auto f = [](auto state) {
                for (auto it : state.accessed_key_and_lvl) {
                    auto& [key, lvl] = it;
                    fprintf(stderr, "key: %lu lvl: %lu\n", key, lvl);
                }
            };

            fprintf(stderr, "==== main ====\n");
            f(lsm_main_states);
            fprintf(stderr, "==== replica ====\n");
            f(lsm_replica_states);
        }
        ASSERT_EQ(is_same, true);
        double time_info = app::lsmtree_get_time_info(lsm_main, key);
        double time_info_2 = get_time_info_nofj(value);
        ASSERT_EQ(time_info, time_info_2);
        // === end simulate rbv ===

        // === begin check rbv ===
        uint64_t ret3 = ((validator::lsmtree::LSMTree*)lsm_main)->Get(key);
        ASSERT_EQ(ret3, value);
        uint64_t crc32 = calc_crc_special(key, value, 0);
        uint64_t crc32_2 = app::lsmtree_get_crc(lsm_main, key);
        // fprintf(stderr, "crc32 = %lu, crc32_2 = %lu\n", crc32, crc32_2);
        ASSERT_EQ(crc32, crc32_2);
        // fprintf(stderr, "ret3 = %lu\n", ret3);
        // === end check rbv ===
    }

    for (int i = 0; i < keys.size(); i++) {
        uint64_t key = keys[i];
        uint64_t v2 = data[key];

        // === begin simulate rbv ===
        auto ret1 = (uint64_t)validator::lsmtree_get(lsm_main, key);
        // fprintf(stderr, "replica set\n");
        auto ret2 = (uint64_t)validator::lsmtree_get(lsm_replica, key);
        ASSERT_EQ(ret1, ret2);
        const auto& lsm_main_states = app::lsmtree_get_internal_states(lsm_main);
        const auto& lsm_replica_states = validator::lsmtree_get_internal_states(lsm_replica);
        bool is_same = lsm_main_states == lsm_replica_states;
        ASSERT_EQ_FINAL(is_same, true);
        // double time_info = app::lsmtree_get_time_info(lsm_main, key);
        // double time_info_2 = validator::lsmtree_get_time_info(lsm_replica, key);
        // ASSERT_EQ_FINAL(time_info, time_info_2);
        // === end simulate rbv ===

        // === begin check rbv ===
        // ASSERT_EQ_FINAL(ret1, v2);
        // === end check rbv ===
    }

    return 0;
}
}  // namespace scee

int main() {
    scee::main_fn();
    return 0;
}
