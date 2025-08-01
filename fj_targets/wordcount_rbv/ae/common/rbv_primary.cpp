#include "rbv.hpp"

namespace rbv {

std::map<std::string, std::atomic_uint64_t> order;
std::shared_mutex mtx;
thread_local hasher_t hasher;

bool hasher_t::checkorder(std::string orderId) {
    ensure_order(orderId);
    std::shared_lock<std::shared_mutex> lock(mtx);
    uint64_t timestamp = order[orderId]++;
    info.emplace_back(latest, timestamp, orderId);
    latest = 0;
    return true;
}

std::string hasher_t::finalize() {
    std::string s = serialize();
    info.clear(), reference = latest = 0, cursor = 0;
    return s;
}

}  // namespace rbv
