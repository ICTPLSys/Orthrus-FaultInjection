#pragma once

#include "memtypes.hpp"
#include "namespace.hpp"

// this file lists functions that requires context-specific implementation
namespace NAMESPACE {

/* allocate an object of size `size`.
 * raw & run: call alloc_immutable() to keep consistency
 * validate: simply return the pointer allocated
 */
void *alloc_obj(size_t size);

/* allocate `n` objects of type `T`, initialize them with `default_v`.
 */
template <typename T>
void alloc_obj_n(T **ptrs, size_t n, const T *default_v);

// free an object by calling free_immutable() or by pushing to free_log.
void free_obj(void *ptr);

template <typename T>
void destroy_obj(T *obj);

/* allocate a pointer.
 * raw & run: call alloc_mutable() to keep consistency
 * validate: simply return the pointer allocated
 */
void *alloc_ptr();

// free a pointer by calling free_mutable()
void free_ptr(void *ptr);

/* create a shadow memory address at validation for object initialization.
 * raw: simply return the pointer
 * run: simply return the pointer
 * validate: allocate a shadow memory address, and operate on it
 */
template <typename T>
T *shadow_init(T *ptr);

template <typename T>
T *shadow_init(T *ptr, size_t n);

/* commit the shadow memory address to the real memory address.
 * raw: update ref count and checksum
 * run: update ref count and checksum
 * validate: compare the shadow address and the real address, remove the shadow
 * address
 */
template <typename T>
void shadow_commit(const T *shadow, T *real);

template <typename T>
void shadow_commit(const T *shadow, T *real, size_t n);

// destroy the shadow memory address created by shadow_init().
template <typename T>
void shadow_destroy(const T *shadow);

template <typename T>
void shadow_destroy(const T *shadow, size_t n);

/* store an immutable object AS A WHOLE, this object is in a new ptr_t instance.
 * raw: equivalent to memcpy
 * run: dst is immutable, record dst and size, initialize obj_prefix of dst
 * validate: compare dst and size, then compute the checksum of src and compare
 * TODO: replace as a shadow memory copy
 */
void store_obj(void *dst, const void *src, size_t size);

/* load a pointer from a ptr_t instance.
 * raw: read the value directly
 * run: record the address to load and the loaded value, OPTIONALLY validate the
 * checksum validate: compare the address to load and return the recorded value
 */
const void *load_ptr(const void *ptr);

/* store a pointer to a ptr_t instance.
 * raw: write the value directly
 * run: record the address to store and the stored value
 * validate: compare the address to store and the stored value
 */
void store_ptr(const void *ptr, const void *val);

// save and reuse a return value outside the protected space.
template <typename T>
T external_return(T val);

// return true if the current context is a validator
constexpr bool is_validator();

// fetch the cursor of the current log
void *fetch_cursor();

// rollback the log to the cursor
void rollback_cursor(void *cursor);

}  // namespace NAMESPACE
