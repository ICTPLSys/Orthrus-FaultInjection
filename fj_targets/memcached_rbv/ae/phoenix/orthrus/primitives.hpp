#include <concepts>
#include <cstddef>
#include <utility>

#include "context.hpp"
#include "memmgr.hpp"
#include "namespace.hpp"
#include "ptr.hpp"
#include "utils.hpp"

namespace NAMESPACE::detail {

template <typename T>
class mut_array;

template <typename T>
class mut_pointer {
private:
    ptr_t<T> *ptr;

private:
    friend class mut_array<T>;

    explicit mut_pointer(ptr_t<T> *ptr) : ptr(ptr) {}

public:
    mut_pointer() = default;

    template <typename... Args>
    static mut_pointer<T> create(Args &&...args) {
        return mut_pointer<T>(ptr_t<T>::create(std::forward<Args>(args)...));
    }

    static mut_pointer<T> null() { return mut_pointer<T>(nullptr); }

    void destroy() {
        const T *old = ptr->load();
        if (old != nullptr) destroy_obj(const_cast<T *>(old));
        ptr->destroy();
    }

    bool is_null() const { return ptr == nullptr; }

    T load() const { return *(ptr->load()); }

    const T *deref() const { return ptr->load(); }

    const T *deref_logless() const { return ptr->load_logless(); }

    const T *deref_nullable() const { return ptr ? ptr->load() : nullptr; }

    void store(T value) { ptr->store(value); }
};

inline void *alloc_immutable_bulk(size_t size) {
    using scee::bulk_buffer;
    using scee::BULK_BUFFER_SIZE;
    using scee::bulk_cursor;
    size_t size_ = size + sizeof(checksum_t);
    if (unlikely(size_ >= BULK_BUFFER_SIZE)) {
        return alloc_obj(size);
    }
    if (bulk_cursor + size_ > BULK_BUFFER_SIZE) {
        bulk_buffer = alloc_obj(BULK_BUFFER_SIZE);
        bulk_cursor = 0;
    }
    assert(bulk_buffer != nullptr);
    void *ptr = add_byte_offset(bulk_buffer, bulk_cursor);
    bulk_cursor += size_;
    return ptr;
}

template <typename T>
class imm_array {
private:
    const T *ptr;  // point to first element of the array

private:
    explicit imm_array(const T *ptr) : ptr(ptr) {}

public:
    imm_array() : ptr(nullptr) {}

    template <std::invocable<T *> Fn>
    static imm_array<T> create(size_t size, Fn init) {
        T *arr;
        if constexpr (std::is_same_v<T, char>) {
            arr = (T *)alloc_immutable_bulk(size * sizeof(T));
        } else {
            arr = (T *)alloc_obj(size * sizeof(T));
        }
        T *shadow = shadow_init(arr, size);
        init(shadow);
        shadow_commit(shadow, arr, size);
        shadow_destroy(shadow);
        return imm_array<T>(arr);
    }

    static imm_array<T> null() { return imm_array<T>(); }

    void destroy() const {
        assert(false);
        // TODO: fix this
        // if (ptr == nullptr) return;
        // const T *arr = ptr->load();
        // destroy_obj(const_cast<T *>(arr));
        // ptr->destroy();
    }

    bool is_null() const { return ptr == nullptr; }

    const T *deref() const { return ptr; }

    const T *deref_nullable() const { return ptr; }
};

template <typename T>
class mut_array {  // reside in versioned memory!!!
private:
    ptr_t<T> *ptr;
    size_t size;

private:
    explicit mut_array(ptr_t<T> *ptr, size_t size) : ptr(ptr), size(size) {}

public:
    static mut_array<T> create(size_t size) {
        auto *arr = (ptr_t<T> *)alloc_obj(size * sizeof(ptr_t<T>));
        ptr_t<T> *shadow = shadow_init(arr, size);
        memset(shadow, 0, size * sizeof(ptr_t<T>));
        shadow_destroy(shadow);
        return mut_array<T>(arr, size);
    }

    static mut_array<T> create(size_t size, T default_v) {
        static_assert(sizeof(ptr_t<T>) == sizeof(void *));
        auto *arr = (T **)alloc_obj(size * sizeof(void *));
        alloc_obj_n(arr, size, &default_v);
        return mut_array<T>(reinterpret_cast<ptr_t<T> *>(arr), size);
    }

    void destroy() const {
        for (size_t i = 0; i < size; ++i) {
            const T *old = ptr[i].load();
            if (old != nullptr) destroy_obj(const_cast<T *>(old));
        }
        free_obj(ptr);
    }

    mut_pointer<T> get(size_t idx) const { return mut_pointer<T>(&ptr[idx]); }

    const T *deref(size_t idx) const { return ptr[idx].load(); }

    const T *deref_nullable(size_t idx) const {
        return ptr[idx].is_null() ? nullptr : ptr[idx].load();
    }

    void store(size_t idx, T value) const { ptr[idx].store(value); }
};

}  // namespace NAMESPACE::detail

namespace scee {

template <typename T>
class mut_pointer : public NAMESPACE::detail::mut_pointer<T> {
    using Base = NAMESPACE::detail::mut_pointer<T>;

    mut_pointer(Base base) : Base(base) {}

public:
    mut_pointer() = default;

    template <typename... Args>
    FORCE_INLINE static mut_pointer<T> create(Args &&...args) {
        return Base::create(std::forward<Args>(args)...);
    }

    FORCE_INLINE static mut_pointer<T> null() { return Base::null(); }
};

template <typename T>
class imm_array : public NAMESPACE::detail::imm_array<T> {
    using Base = NAMESPACE::detail::imm_array<T>;

    imm_array(Base base) : Base(base) {}

public:
    imm_array() = default;

    template <std::invocable<T *> Fn>
    FORCE_INLINE static imm_array<T> create(size_t size, Fn init) {
        return Base::create(size, init);
    }

    FORCE_INLINE static imm_array<T> null() { return Base::null(); }
};

template <typename T>
class mut_array : public NAMESPACE::detail::mut_array<T> {
    using Base = NAMESPACE::detail::mut_array<T>;

    mut_array(Base base) : Base(base) {}

public:
    mut_array() = default;

    FORCE_INLINE static mut_array<T> create(size_t size) {
        return Base::create(size);
    }

    FORCE_INLINE static mut_array<T> create(size_t size, T default_v) {
        return Base::create(size, default_v);
    }
};

}  // namespace scee
