#pragma once

#include <cstddef>
#include <cstdint>

#include "context.hpp"
#include "memmgr.hpp"
#include "namespace.hpp"
#include "utils.hpp"

namespace scee {
using namespace NAMESPACE;

template <typename T>
class mut_array;

// A fixed pointer to an immutable object.
// need to be destroyed after an immutable object is destroyed.
template <typename T>
class fixed_ptr_t {
    // static_assert(std::has_unique_object_representations_v<T> ||
    //              std::is_base_of_v<obj_header, T>);

public:
    fixed_ptr_t() : ptr(nullptr) {}
    explicit fixed_ptr_t(const T *ptr) : ptr(ptr) {}
    const T *get() const { return ptr; }
    void destroy() const { destroy_obj(const_cast<T *>(ptr)); }

private:
    const T *ptr;
};

/**
 * A smart pointer class for managing (immutable) objects in shared memory.
 *
 * This class provides safe access and modification of objects in shared address
 * space, with built-in reference counting and validation support.
 *
 * Available methods:
 * 1. ptr->load(): decodes and returns the pointer value
 * 2. ptr->store(const T &): creates and stores a new version of the object,
 * returns its pointer and destroys the old version
 * 3. ptr->reref(const T*): updates this pointer to reference a different
 * object, possibly nullptr
 * 4. ptr->create([const T*|const T&]): creates a new ptr_t instance, optionally
 * initialized with given value
 * 5. ptr->destroy(): destroys and reclaims this ptr_t instance ONLY.
 * 6. ptr->create_fixed(const T &): creates a fixed pointer with given value.
 *
 * The load of any mutable object, like ptr_t, cannot be protected by SCEE logs.
 * There are two ways to corrupt future reads:
 * 1. The mutable object is changed unexpectedly, or loaded incorrectly.
 * 2. The mutable object update is not properly handled.
 *
 * For ptr_t, the first way is handled by the checksum of the pointed object.
 * The second way is handled by recording 1) the updated address, and 2) the
 * value to be stored. After the store at run(), we also verify the stored
 * value.
 */
template <typename T>
class ptr_t {
    static_assert(std::has_unique_object_representations_v<T> ||
                  std::is_base_of_v<obj_header, T>);

public:
    ptr_t<T> &operator=(const ptr_t<T> &) = delete;

    // we should only load each ptr once in each closure
    FORCE_INLINE const T *load() const {
        const T *p = (const T *)load_ptr(this);
        return p;
    }

    FORCE_INLINE const T *load_logless() const { return this->ptr; }

    FORCE_INLINE const T *store(const T &obj) {
        const T *old = (const T *)load_ptr(this);
        const T *addr = make_obj(obj);
        store_ptr(this, addr);
        if (old != nullptr) destroy_obj(const_cast<T *>(old));
        return addr;
    }

    FORCE_INLINE void reref(const T *ptr) { store_ptr(this, ptr); }

    FORCE_INLINE static ptr_t<T> *create() {
        auto *t = (ptr_t<T> *)alloc_ptr();
        store_ptr(t, nullptr);
        return t;
    }

    FORCE_INLINE static ptr_t<T> *create(const T *ptr) {
        auto *t = (ptr_t<T> *)alloc_ptr();
        store_ptr(t, ptr);
        return t;
    }

    FORCE_INLINE static ptr_t<T> *create(const T &obj) {
        auto *t = (ptr_t<T> *)alloc_ptr();
        const T *addr = make_obj(obj);
        store_ptr(t, addr);
        return t;
    }

    FORCE_INLINE void destroy() const {
        // destroy_obj should be handled by the application
        free_ptr((void *)this);
    }

    FORCE_INLINE static fixed_ptr_t<T> create_fixed(const T &obj) {
        return fixed_ptr_t<T>(make_obj(obj));
    }

    FORCE_INLINE static const T *make_obj(const T &obj) {
        // rwith initial ref count = 1
        size_t size = get_size(&obj);
        T *addr = (T *)alloc_obj(size);
        if constexpr (std::is_base_of_v<obj_header, T>) {
            T *shadow = shadow_init(addr);
            obj.write_at(shadow, addr, size);
            shadow_commit(shadow, addr);
            shadow_destroy(shadow);
        } else {
            store_obj(addr, &obj);
        }
        return addr;
    }

private:
    friend class mut_array<T>;

    T *ptr;
};

}  // namespace scee
