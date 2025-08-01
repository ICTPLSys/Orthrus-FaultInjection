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

template <typename T>
inline void shadow_commit(const T *shadow, T *real) {
    size_t size = get_size(real);
    *(checksum_t *)add_byte_offset(real, size) = compute_checksum(real, size);
}

template <typename T>
inline void shadow_destroy(const T *shadow) {}

template <typename T>
inline void store_obj(T *dst, const T *src) {
    size_t size = get_size(src);
    memcpy(dst, src, size);
    *(checksum_t *)add_byte_offset(dst, size) = compute_checksum(src, size);
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

inline bool is_validator() { return false; }

}  // namespace app
