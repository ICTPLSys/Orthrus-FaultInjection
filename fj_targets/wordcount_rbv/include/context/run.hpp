#pragma once

#include <cstring>

#include "checksum.hpp"
#include "free_log.hpp"
#include "log.hpp"
#include "memmgr.hpp"

namespace app {

using namespace ::scee;

// duplicated implementation for fault injection
static __attribute__((target("sse4.2"))) uint32_t calculate_crc32_app(
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

static checksum_t compute_checksum_app(const void *ptr, size_t size) {
    return calculate_crc32_app(ptr, size);
}

inline void *alloc_obj(size_t size) {
    append_log_typed(size);
    void *ptr = alloc_immutable(size);
    append_log_typed(ptr);
    return ptr;
}

template <typename T>
inline void alloc_obj_n(T **ptrs, size_t n, const T *default_v) {
    checksum_t checksum = compute_checksum_app(default_v, sizeof(T));
    append_log_typed(ptrs);
    append_log_typed(n);
    append_log_typed(checksum);
    T **backup = (T **)malloc(sizeof(T *) * n);
    append_log_typed(backup);
    for (size_t i = 0; i < n; ++i) {
        T *ptr = (T *)alloc_immutable(sizeof(T));
        memcpy(ptr, default_v, sizeof(T));
        *(checksum_t *)add_byte_offset(ptr, sizeof(T)) = checksum;
        backup[i] = ptr;
    }
    memcpy(ptrs, backup, sizeof(T *) * n);
}

inline void free_obj(void *ptr) { thread_gc_instance.free_log.push(ptr); }

inline void *alloc_ptr() {
    void *ptr = alloc_mutable(sizeof(void *));
    append_log_typed(ptr);
    return ptr;
}

inline void free_ptr(void *ptr) { free_mutable(ptr); }

template <typename T>
inline void destroy_obj(T *obj) {
    if constexpr (std::is_base_of_v<obj_header, T>) {
        obj->destroy();
    }
    free_obj(obj);
}

template <typename T>
inline T *shadow_init(T *ptr) {
    return ptr;
}

template <typename T>
inline T *shadow_init(T *ptr, size_t /* n */) {
    return ptr;
}

template <typename T>
inline void shadow_commit(const T * /* shadow */, T *real) {
    size_t size = get_size(real);
    *(checksum_t *)add_byte_offset(real, size) =
        compute_checksum_app(real, size);
}

template <typename T>
inline void shadow_commit(const T * /* shadow */, T *real, size_t n) {
    size_t size = sizeof(T) * n;
    *(checksum_t *)add_byte_offset(real, size) =
        compute_checksum_app(real, size);
}

template <typename T>
inline void shadow_destroy(const T *shadow) {}

template <typename T>
inline void shadow_destroy(const T * /* shadow */, size_t /* n */) {}

template <typename T>
inline void store_obj(T *dst, const T *src) {
    size_t size = get_size(src);
    memcpy(dst, src, size);
    *(checksum_t *)add_byte_offset(dst, size) = compute_checksum_app(src, size);
}

inline const void *load_ptr(const void *ptr) {
    // append_log_typed(ptr);
    const void *stored = *((const void **)ptr);
    append_log_typed(stored);
    return stored;
}

inline void store_ptr(const void *ptr, const void *val) {
    // append_log_typed(ptr);
    append_log_typed(val);
    *((const void **)ptr) = val;
}

template <typename T>
inline T external_return(T val) {
    append_log_typed(val);
    return val;
}

inline constexpr bool is_validator() { return false; }

inline void *fetch_cursor() { return get_log_cursor(); }

inline void rollback_cursor(void *cursor) { unroll_log(cursor); }

}  // namespace app
