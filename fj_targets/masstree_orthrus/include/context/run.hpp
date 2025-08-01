#pragma once

#include <cstring>

#include "checksum.hpp"
#include "free_log.hpp"
#include "log.hpp"
#include "memmgr.hpp"

namespace app {

using namespace ::scee;

inline void *alloc_obj(size_t size) {
    append_log_typed(size);
    void *ptr = alloc_immutable(size);
    append_log_typed(ptr);
    return ptr;
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

__attribute__((target("sse4.2"))) inline uint32_t calculate_crc32_app(const void* data,
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


inline checksum_t compute_checksum_app(const void* ptr, size_t size) {
    return calculate_crc32_app(ptr, size);
}

template <typename T>
inline void shadow_commit(const T *shadow, T *real) {
    size_t size = get_size(real);
    *(checksum_t *)add_byte_offset(real, size) = compute_checksum_app(real, size);
}

template <typename T>
inline void shadow_destroy(const T *shadow) {}

template <typename T>
inline void store_obj(T *dst, const T *src) {
    size_t size = get_size(src);
    memcpy(dst, src, size);
    *(checksum_t *)add_byte_offset(dst, size) = compute_checksum_app(src, size);
}

inline const void *load_ptr(const void *ptr) {
    append_log_typed(ptr);
    const void *stored = *((const void **)ptr);
    append_log_typed(stored);
    return stored;
}

inline void store_ptr(const void *ptr, const void *val) {
    append_log_typed(ptr);
    append_log_typed(val);
    *((const void **)ptr) = val;
}

template <typename T>
inline T external_return(T val) {
    append_log_typed(val);
    return val;
}

inline bool is_validator() { return false; }

inline void *fetch_cursor() { return get_log_cursor(); }

inline void rollback_cursor(void *cursor) { unroll_log(cursor); }

}  // namespace app
