#pragma once

#include <cstring>

#include "assertion.hpp"
#include "checksum.hpp"
#include "log.hpp"
#include "memmgr.hpp"

namespace validator {

using namespace ::scee;

// duplicated implementation for fault injection
static __attribute__((target("sse4.2"))) uint32_t calculate_crc32_val(
    const void *data, std::size_t length) {
    std::uint32_t crc = ~0U;
    const auto *buffer = static_cast<const unsigned char *>(data);

    while (length > 0 && reinterpret_cast<std::uintptr_t>(buffer) % 8 != 0) {
        crc = _mm_crc32_u8(crc, *buffer);
        buffer++;
        length--;
    }

    const auto *buffer64 = reinterpret_cast<const std::uint64_t *>(buffer);
    while (length >= 8) {
        crc = _mm_crc32_u64(crc, *buffer64);
        buffer64++;
        length -= 8;
    }

    buffer = reinterpret_cast<const unsigned char *>(buffer64);

    if (length >= 4) {
        auto value32 = *reinterpret_cast<const std::uint32_t *>(buffer);
        crc = _mm_crc32_u32(crc, value32);
        buffer += 4;
        length -= 4;
    }

    if (length >= 2) {
        auto value16 = *reinterpret_cast<const std::uint16_t *>(buffer);
        crc = _mm_crc32_u16(crc, value16);
        buffer += 2;
        length -= 2;
    }

    if (length > 0) {
        crc = _mm_crc32_u8(crc, *buffer);
    }

    return ~crc;
}

static checksum_t compute_checksum_val(const void *ptr, size_t size) {
    return calculate_crc32_val(ptr, size);
}

inline void *alloc_obj(size_t size) {
    log_reader.cmp_log_typed(size);
    void *ptr;
    log_reader.fetch_log_typed(&ptr);
    return ptr;
}

template <typename T>
inline void alloc_obj_n(T **ptrs, size_t n, const T *default_v) {
    checksum_t checksum = compute_checksum_val(default_v, sizeof(T));
    log_reader.cmp_log_typed(ptrs);
    log_reader.cmp_log_typed(n);
    log_reader.cmp_log_typed(checksum);
    T **backup;
    log_reader.fetch_log_typed(&backup);
    for (size_t i = 0; i < n; ++i) {
        T *ptr = backup[i];
        auto stored_checksum = *reinterpret_cast<checksum_t *>(ptr + 1);
        validator_assert(stored_checksum == checksum);
        validator_assert(compute_checksum(ptr, sizeof(T)) == checksum);
    }
    free(backup);
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
inline T *shadow_init(T *, size_t n) {
    size_t size = sizeof(T) * n;
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
    checksum_t computed = compute_checksum_val(shadow, size);
    checksum_t stored = *(checksum_t *)add_byte_offset(real, size);
    validator_assert(computed == stored);
}

template <typename T>
inline void shadow_commit(const T *shadow, T *real, size_t n) {
    size_t size = sizeof(T) * n;
    checksum_t computed = compute_checksum_val(shadow, size);
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
inline void shadow_destroy(const T *shadow, size_t n) {
    size_t size = sizeof(T) * n;
    if (unlikely(size >= SHADOW_BUFFER_SIZE)) {
        free((void *)shadow);
    }
}

template <typename T>
inline void store_obj(T *dst, const T *src) {
    size_t size = get_size(src);
    validator_assert(size == get_size(dst));
    checksum_t computed = compute_checksum_val(src, size);
    checksum_t stored = *(checksum_t *)add_byte_offset(dst, size);
    validator_assert(computed == stored);
}

inline const void *load_ptr(const void *ptr) {
    // log_reader.cmp_log_typed(ptr);
    const void *stored;
    log_reader.fetch_log_typed(&stored);
    return stored;
}

inline void store_ptr(const void *ptr, const void *val) {
    // log_reader.cmp_log_typed(ptr);
    log_reader.cmp_log_typed(val);
}

template <typename T>
inline T external_return(T val) {
    log_reader.fetch_log_typed(&val);
    return val;
}

inline constexpr bool is_validator() { return true; }

inline void *fetch_cursor() { return nullptr; }

inline void rollback_cursor(void *cursor) {}

}  // namespace validator
