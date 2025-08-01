#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include "compiler.hpp"
#include "memtypes.hpp"
#include "utils.hpp"

namespace scee {

/**
 * IMPORTANT: objects should have unique object representations. Otherwise, the
 * checksum might be inconsistent.
 * You can use `static_assert(std::has_unique_object_representations_v<T>);` to
 * check if a type has unique object representations.
 *
 * Base class for (immutable) objects with (varying) size determined at runtime.
 * Layout:
 * | data... | checksum |
 * Fixed size objects are similarly stored, without the size field.
 *
 * The safety of the immutable part of this object is guaranteed by SCEE logs
 * and the checksum.
 *
 * To define a customized object without unique object representation, you need
 * to override the write_at() and destroy() methods.
 * write_at(shadow, real): write current object (in non-versioned memory) to
 * shadow memory as writing to real memory.
 * destroy(): destroy all objects and ptr_t instances created by this object.
 */
struct obj_header {
    size_t size() const {
        fprintf(stderr, "object size not implemented!\n");
        std::abort();
    }
    void write_at(void *shadow, void *real, size_t size) const {};
    void destroy() const {};
};

// size is the actual size of the object, memory manager will add obj_prefix
inline void *alloc_immutable(size_t size) {
    void *ptr = malloc(size + sizeof(checksum_t) + 4);
    return ptr;
}
// ptr correspond to the start of the object
inline void free_immutable(void *ptr) {
    // fprintf(stderr, "real free: %p\n", ptr);
    free(ptr);
}

// size is the actual size allocated, usually a small memory piece
inline void *alloc_mutable(size_t size) {
    // OPTIONAL: optimize this for fixed size allocation (like ptr_t)
    // IMPORTANT: when using concurrently, ensure ptr_t assignment is atomic
    return malloc(size);
}
// ptr correspond to the start of the whole memory piece
inline void free_mutable(void *ptr) {
    // OPTIONAL: optimize this for fixed size allocation (like ptr_t)
    free(ptr);
}

template <typename T>
inline size_t get_size(const T *obj) {
    if constexpr (std::is_base_of_v<obj_header, T>) {
        return obj->size();
    } else {
        return sizeof(T);
    }
}

constexpr size_t BULK_BUFFER_SIZE = 65536;
extern thread_local void *bulk_buffer;
extern thread_local size_t bulk_cursor;
inline void reset_bulk_buffer() {
    bulk_buffer = nullptr;
    bulk_cursor = BULK_BUFFER_SIZE;
}

}  // namespace scee
