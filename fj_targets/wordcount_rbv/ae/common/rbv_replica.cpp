#include "rbv.hpp"

#define myassert(x) if (!(x)) { \
    std::cerr << "assert failed: " << #x << std::endl; exit(1); }

namespace rbv {

std::map<std::string, std::atomic_uint64_t> order;
std::shared_mutex mtx;
thread_local hasher_t hasher;

bool hasher_t::checkorder(std::string orderId) {
    ensure_order(orderId);
    myassert(cursor < info.size());
    myassert(orderId == info[cursor].orderId);
    std::shared_lock<std::shared_mutex> lock(mtx);
    uint64_t timestamp = order[orderId];
    myassert(timestamp <= info[cursor].timestamp);
    if (timestamp != info[cursor].timestamp) return false;
    order[orderId]++;
    latest = 0;
    return true;
}

std::string hasher_t::finalize() {
    myassert(latest == reference);
    info.clear(), reference = latest = 0, cursor = 0;
    return "";
}

}  // namespace rbv
