#include "rbv.hpp"

namespace rbv {

thread_local hasher_t hasher;

void hasher_t::checkorder(std::atomic_uint64_t &order) {
    uint64_t timestamp = order++;
    info.emplace_back(latest, timestamp);
    latest = 0;
}

std::string hasher_t::finalize() {
    std::string s = serialize();
    info.clear(), reference = latest = 0, cursor = 0;
    return s;
}

void hasher_t::flush() {
    info.emplace_back(latest, 0);
    latest = 0;
}

}  // namespace rbv
