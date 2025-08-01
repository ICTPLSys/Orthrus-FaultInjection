#pragma once

#include <cstring>
#include <type_traits>

#include "checksum.hpp"
#include "memmgr.hpp"

namespace raw {
// raw is the no-validation version of run

using namespace ::scee;

inline void *alloc_obj(size_t size) { return alloc_immutable(size); }

template <typename T>
inline void alloc_obj_n(T **ptrs, size_t n, const T *default_v) {
    checksum_t checksum = compute_checksum(default_v, sizeof(T));
    for (size_t i = 0; i < n; ++i) {
        T *ptr = (T *)alloc_immutable(sizeof(T));
        memcpy(ptr, default_v, sizeof(T));
        *(checksum_t *)add_byte_offset(ptr, sizeof(T)) = checksum;
        ptrs[i] = ptr;
    }
}

inline void free_obj(void *ptr) { free_immutable(ptr); }

inline void *alloc_ptr() { return alloc_mutable(sizeof(void *)); }

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
inline void shadow_commit(const T * /*shadow*/, T *real) {
    size_t size = get_size(real);
    *(checksum_t *)add_byte_offset(real, size) = compute_checksum(real, size);
}

template <typename T>
inline void shadow_commit(const T * /*shadow*/, T *real, size_t n) {
    size_t size = sizeof(T) * n;
    *(checksum_t *)add_byte_offset(real, size) = compute_checksum(real, size);
}

template <typename T>
inline void shadow_destroy(const T * /* shadow */) {}

template <typename T>
inline void shadow_destroy(const T * /* shadow */, size_t /* n */) {}

template <typename T>
inline void store_obj(T *dst, const T *src) {
    size_t size = get_size(src);
    memcpy(dst, src, size);
    *(checksum_t *)add_byte_offset(dst, size) = compute_checksum(src, size);
}

inline const void *load_ptr(const void *ptr) { return *((const void **)ptr); }

inline void store_ptr(const void *ptr, const void *val) {
    *((const void **)ptr) = val;
}

inline bool is_validator() { return false; }

}  // namespace raw
