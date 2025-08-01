#pragma once

#include <cstring>

#include "assertion.hpp"
#include "checksum.hpp"
#include "log.hpp"
#include "memmgr.hpp"

namespace validator {

using namespace ::scee;

inline void *alloc_obj(size_t size) {
    log_reader.cmp_log_typed(size);
    void *ptr;
    log_reader.fetch_log_typed(&ptr);
    return ptr;
}

inline void free_obj(void *ptr) {}

template <typename T>
inline void destroy_obj(T *obj) {
    free_obj(obj);
}

inline void *alloc_ptr() {
    void *ptr;
    log_reader.fetch_log_typed(&ptr);
    return ptr;
}

inline void free_ptr(void *ptr) {}

constexpr size_t SHADOW_BUFFER_SIZE = 4096;

template <typename T>
inline T *shadow_init(T *ptr) {
    size_t size = get_size(ptr);
    if (unlikely(size >= SHADOW_BUFFER_SIZE)) {
        return (T *)malloc(size);
    }
    static thread_local void *shadow_buffer = malloc(SHADOW_BUFFER_SIZE);
    return (T *)shadow_buffer;
}

template <typename T>
inline void shadow_commit(const T *shadow, T *real) {
    size_t size = get_size(shadow);
    validator_assert(size == get_size(real));
    checksum_t computed = compute_checksum(shadow, size);
    checksum_t stored = *(checksum_t *)add_byte_offset(real, size);
    validator_assert(computed == stored);
}

template <typename T>
inline void shadow_destroy(const T *shadow) {
    size_t size = get_size(shadow);
    if (unlikely(size >= SHADOW_BUFFER_SIZE)) {
        free((void *)shadow);
    }
}

template <typename T>
inline void store_obj(T *dst, const T *src) {
    size_t size = get_size(src);
    validator_assert(size == get_size(dst));
    checksum_t computed = compute_checksum(src, size);
    checksum_t stored = *(checksum_t *)add_byte_offset(dst, size);
    validator_assert(computed == stored);
}

inline const void *load_ptr(const void *ptr) {
    log_reader.cmp_log_typed(ptr);
    const void *stored;
    log_reader.fetch_log_typed(&stored);
    return stored;
}

inline void store_ptr(const void *ptr, const void *val) {
    log_reader.cmp_log_typed(ptr);
    log_reader.cmp_log_typed(val);
}

inline bool is_validator() { return true; }

}  // namespace validator
