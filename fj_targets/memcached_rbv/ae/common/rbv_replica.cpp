#include <cassert>
#include <cstring>
#include <iostream>
#include "rbv.hpp"

namespace rbv {

thread_local hasher_t hasher;

void hasher_t::checkorder(std::atomic_uint64_t &order) {
    assert(cursor < info.size());
    uint64_t timenow = order.load();
    while (timenow != info[cursor].timestamp) {
        assert(timenow <= info[cursor].timestamp);
        order.wait(timenow);
        timenow = order.load();
    }
    assert(info[cursor].hashv == latest);
    latest = 0, order++, cursor++;
    order.notify_all();
}

void hasher_t::flush() {
    assert(cursor < info.size());
    assert(info[cursor].hashv == latest);
    assert(info[cursor].timestamp == 0);
    cursor++;
}

std::string hasher_t::finalize() {
    if (latest != reference) {
        fprintf(stderr, "%s %lu\n", serialize().c_str(), reference);
    }
    assert(latest == reference);
    info.clear(), reference = latest = 0, cursor = 0;
    return "";
}

}  // namespace rbv
