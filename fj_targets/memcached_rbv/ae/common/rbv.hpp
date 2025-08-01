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

constexpr uint64_t FNV_prime = 1099511628211ULL;
constexpr uint64_t FNV_offset_basis = 14695981039346656037ULL;

inline uint64_t fnv1a_hash_bytes(const void* data, size_t len) {
    uint64_t hash = FNV_offset_basis;
    const unsigned char* bytes = static_cast<const unsigned char*>(data);

    for (size_t i = 0; i < len; ++i) {
        hash ^= bytes[i];
        hash *= FNV_prime;
    }
    return hash;
}

constexpr int GRANULARITY = 16;

struct info_t {
    uint64_t hashv;
    uint64_t timestamp;
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
            ss << i.hashv << " " << i.timestamp << " ";
        }
        return ss.str();
    }
    void deserialize(const std::string &s) {
        std::stringstream ss(s);
        size_t info_size;
        ss >> info_size >> reference;
        info.resize(info_size);
        for (size_t i = 0; i < info_size; i++) {
            ss >> info[i].hashv >> info[i].timestamp;
        }
        latest = cursor = 0;
    }
    void combine_(uint64_t hashv) {
        latest ^= hashv + 0x9e3779b9 + (latest << 6) + (latest >> 2);
    }
    void combine(uint64_t x) {
        combine_(fnv1a_hash_bytes(&x, sizeof(x)));
    }
    void combine(const std::string& s) {
        combine_(fnv1a_hash_bytes(s.data(), s.length()));
    }
    void combine(std::string_view sv) {
        combine_(fnv1a_hash_bytes(sv.data(), sv.length()));
    }
    void checkorder(std::atomic_uint64_t &order);
    void flush();
    std::string finalize();
    void reset(){ info.clear(), reference = latest = 0, cursor = 0; }
};

extern thread_local hasher_t hasher;

static inline uint64_t relative_addr(const void *ptr, const void *ref) {
    return (uint64_t)ptr - (uint64_t)ref;
}

static inline std::string toString20(long long x) {
    std::stringstream ss;
    ss << std::setw(20) << std::setfill('0') << x;
    return ss.str();
}

}  // namespace rbv