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

inline void free_obj(void *ptr) {
    // fprintf(stderr, "val free: %p (bypass)\n", ptr);
    // ::free(ptr);
}

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

__attribute__((target("sse4.2"))) inline uint32_t calculate_crc32_val(const void* data,
    std::size_t length) {
    // 初始化CRC值为全1（这是CRC32的标准初始值）
    std::uint32_t crc = ~0U;
    const auto* buffer = static_cast<const unsigned char*>(data);

    // 处理未对齐的前缀字节，直到达到8字节对齐
    while (length > 0 && reinterpret_cast<std::uintptr_t>(buffer) % 8 != 0) {
    crc = _mm_crc32_u8(crc, *buffer);
    buffer++;
    length--;
    }

    // 以8字节为单位进行处理（最高效）
    const auto* buffer64 = reinterpret_cast<const std::uint64_t*>(buffer);
    while (length >= 8) {
    crc = _mm_crc32_u64(crc, *buffer64);
    buffer64++;
    length -= 8;
    }

    // 处理剩余的字节
    buffer = reinterpret_cast<const unsigned char*>(buffer64);

    // 尝试以4字节处理
    if (length >= 4) {
    auto value32 = *reinterpret_cast<const std::uint32_t*>(buffer);
    crc = _mm_crc32_u32(crc, value32);
    buffer += 4;
    length -= 4;
    }

    // 尝试以2字节处理
    if (length >= 2) {
    auto value16 = *reinterpret_cast<const std::uint16_t*>(buffer);
    crc = _mm_crc32_u16(crc, value16);
    buffer += 2;
    length -= 2;
    }

    // 处理最后可能剩余的1个字节
    if (length > 0) {
    crc = _mm_crc32_u8(crc, *buffer);
    }

    // 对结果取反，得到最终的CRC32值
    return ~crc;
}

inline checksum_t compute_checksum_val(const void* ptr, size_t size) {
    return calculate_crc32_val(ptr, size);
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

inline constexpr bool is_validator() { return true; }

template <typename T>
inline T external_return(T val) {
    log_reader.fetch_log_typed(&val);
    return val;
}

}  // namespace validator
