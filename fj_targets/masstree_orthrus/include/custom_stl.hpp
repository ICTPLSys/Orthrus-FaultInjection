#pragma once

#include <vector>

#include "memmgr.hpp"
#include "ptr.hpp"

namespace scee {
using namespace NAMESPACE;

// immutable without unique object representation
class imm_nonunique_t : public obj_header {
public:
    void write_at(void *shadow, void *real, size_t size) const {
        memcpy(shadow, this, size);
    }
    void destroy() const {}
};

/**
 * 1-D array that is immutable at runtime.
 * Layout:
 * | ref count | checksum | size | pointer to data | data |
 *
 * To initialize an imm_array_t<T> in versioned memory, call
 * ptr_t<imm_array_t<T>>::create() with an imm_array_t<T> instance in the
 * non-versioned memory.
 *
 * In non-versioned memory, data do not have to be stored immediately after this
 * instance.
 *
 * Optional: remove data field in versioned memory
 * NOTE: when T=ptr_t<SomeType>*, you will have to destroy these pointers
 * manually.
 */
template <typename T>
class imm_array_t : public imm_nonunique_t {
public:
    T *v;
    size_t length;
    imm_array_t(void *data, size_t length) : v((T *)data), length(length) {}
    size_t size() const { return sizeof(*this) + length * sizeof(T); }
    void write_at(void *shadow, void *real, size_t size) const {
        void *real_data_ptr = add_byte_offset(real, sizeof(imm_array_t<T>));
        void *shadow_data_ptr = add_byte_offset(shadow, sizeof(imm_array_t<T>));
        *(imm_array_t<T> *)shadow = imm_array_t<T>(real_data_ptr, this->length);
        memcpy(shadow_data_ptr, this->v, this->length * sizeof(T));
    }
};
using imm_string_t = imm_array_t<char>;

/**
 * 1-D array that can be mutable at runtime.
 * Very similar to imm_array_t<ptr_t<T>*>.
 * The key difference is that these ptr_t pointers are initialized at creation
 * time. Consequently, these pointers are going to be destroyed when the
 * mut_array_t is destroyed.
 *
 * A store(index, value) is provided, equivalent to data[index]->store(value).
 */
template <typename T>
class mut_array_t : public imm_array_t<ptr_t<T> *> {
public:
    using local_ptr_t = ptr_t<T> *;
    mut_array_t(void *data, size_t length)
        : imm_array_t<local_ptr_t>(data, length) {}
    size_t size() const { return imm_array_t<local_ptr_t>::size(); }
    void write_at(void *shadow, void *real, size_t size) const {
        void *real_data_ptr = add_byte_offset(real, sizeof(*this));
        void *shadow_data_ptr = add_byte_offset(shadow, sizeof(*this));
        *(mut_array_t<T> *)shadow = mut_array_t<T>(real_data_ptr, this->length);
        local_ptr_t *vec = (local_ptr_t *)shadow_data_ptr;
        T *v_data = (T *)this->v;
        if (v_data != nullptr) {
            for (size_t i = 0; i < this->length; i++) {
                vec[i] = ptr_t<T>::create(v_data[i]);
            }
        } else {
            for (size_t i = 0; i < this->length; i++) {
                vec[i] = ptr_t<T>::create();
            }
        }
    }
    // called before actually destroying an object
    void destroy() const {
        for (size_t i = 0; i < this->length; i++) {
            const T *old = this->v[i]->load();
            if (old != nullptr) destroy_obj(const_cast<T *>(old));
            this->v[i]->destroy();
        }
    }
    // store only works for mut_array_t reside in versioned memory
    void store(int index, const T &value) const {
        assert(index >= 0 && index < this->length);
        this->v[index]->store(value);
    }
};

// Optional: create a continuous piece of memory for a mut_array_t
// Pro: faster edit of the array
// Con: not being able to detect the error of writing address flipped

}  // namespace scee
