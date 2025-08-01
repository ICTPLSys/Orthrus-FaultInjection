#pragma once

#include <random>
#include <cstdint>

#include "consts.hpp"
#include "context.hpp"

namespace NAMESPACE::lsmtree::rt {
using namespace NAMESPACE;

inline constexpr bool is_validator() { return NAMESPACE::is_validator(); }

double rand();

template<typename T>
inline T cache_read(T ptr) {
    return external_return(ptr);
}

inline size_t fread(void *ptr, size_t size, size_t count, FILE *stream) {
    auto ret = 0;
    struct Data {
        void *buffer;
        size_t size;
    } log;
    if (!is_validator()) {
        // record the data
        log.buffer = ::malloc(size * count);
        log.size = size * count;
        ret = ::fread(ptr, size, count, stream);
        ::memcpy(log.buffer, ptr, size * count);
        auto _ = external_return(log);
    } else {
        // Replay the fread
        auto _ = external_return(log);
        MYASSERT(log.size == size * count);
        ::memcpy(ptr, log.buffer, log.size);
    }
    return external_return(ret);
}

inline size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream) {
    auto ret = 0;
    if (!is_validator()) {
        // record the data
        ret = ::fwrite(ptr, size, count, stream);
    } else {
        // do nothing
    }
    return external_return(ret);
}

inline long ftell(FILE *stream) {
    auto ret = 0;
    if (!is_validator()) {
        ret = ::ftell(stream);
    }
    return external_return(ret);
}

inline int fseek(FILE *stream, long offset, int whence) {
    auto ret = 0;
    if (!is_validator()) {
        ret = ::fseek(stream, offset, whence);
    }
    return external_return(ret);
}

inline int fflush(FILE *stream) {
    auto ret = 0;
    if (is_validator()) {
        ret = ::fflush(stream);
    }
    return external_return(ret);
}

}  // namespace NAMESPACE::lsmtree::rt
