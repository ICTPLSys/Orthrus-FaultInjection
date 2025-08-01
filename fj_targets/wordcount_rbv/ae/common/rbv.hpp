#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <shared_mutex>
#include <functional>
#include <iostream>
#include <iomanip>
#include <utility>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <memory>
#include <vector>
#include <atomic>
#include <string>
#include <thread>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <regex>
#include <map>

namespace rbv {

extern std::map<std::string, std::atomic_uint64_t> order;
extern std::shared_mutex mtx;

static inline void ensure_order(std::string orderId) {
    {
        std::shared_lock<std::shared_mutex> lock(mtx);
        if (order.find(orderId) != order.end()) return;
    }
    {
        std::unique_lock<std::shared_mutex> lock(mtx);
        if (order.find(orderId) == order.end()) order[orderId] = 0;
    }
}

struct info_t {
    uint64_t hashv;
    uint64_t timestamp;
    std::string orderId;
} ;

struct hasher_t {
    std::vector<info_t> info;
    uint64_t latest, reference;
    size_t cursor;
    hasher_t(){ latest = reference = 0, cursor = 0; }
    std::string serialize() {
        std::stringstream ss;
        ss << info.size() << " " << latest << " ";
        for (auto &i : info) {
            ss << i.hashv << " " << i.orderId << " " << i.timestamp << " ";
        }
        return ss.str();
    }
    void deserialize(const std::string &s) {
        std::stringstream ss(s);
        size_t info_size;
        ss >> info_size >> reference;
        info.resize(info_size);
        for (size_t i = 0; i < info_size; i++) {
            ss >> info[i].hashv >> info[i].orderId >> info[i].timestamp;
        }
        latest = cursor = 0;
    }
    void combine_(uint64_t hashv) {
        latest ^= hashv + 0x9e3779b9 + (latest << 6) + (latest >> 2);
    }
    template <typename T>
    void combine(T value) {
        combine_(std::hash<T>()(value));
    }
    bool checkorder(std::string orderId);
    std::string finalize();
};

extern thread_local hasher_t hasher;

static inline uint64_t relative_addr(const void *ptr, const void *ref) {
    return (uint64_t)ptr - (uint64_t)ref;
}

static std::string toString20(long long x) {
    std::stringstream ss;
    ss << std::setw(20) << std::setfill('0') << x;
    return ss.str();
}

}  // namespace rbv