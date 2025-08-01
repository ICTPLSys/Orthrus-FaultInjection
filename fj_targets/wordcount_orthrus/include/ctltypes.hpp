#pragma once

#include <thread>

#include "context.hpp"
#include "namespace.hpp"

namespace scee {
using namespace NAMESPACE;

struct mutex_t {
    pthread_mutex_t mtx;
    mutex_t() {
        if (!is_validator()) pthread_mutex_init(&mtx, nullptr);
    }
    ~mutex_t() {
        if (!is_validator()) pthread_mutex_destroy(&mtx);
    }
    void lock() {
        if (!is_validator()) pthread_mutex_lock(&mtx);
    }
    void unlock() {
        if (!is_validator()) pthread_mutex_unlock(&mtx);
    }
};
static_assert(sizeof(mutex_t) == sizeof(pthread_mutex_t));

class lock_guard_t {
private:
    mutex_t *mtx;

public:
    explicit lock_guard_t(mutex_t *mtx) : mtx(mtx) { mtx->lock(); }
    ~lock_guard_t() { mtx->unlock(); }
};

template <typename T>
struct mutable_list_t {
    T *v;
    size_t length;
    mutable_list_t() : v(nullptr), length(0) {}
    mutable_list_t(T *v, size_t length) : v(v), length(length) {}
    static mutable_list_t<T> create(size_t size) {
        T *v = (T *)alloc_mutable(size * sizeof(T));
        for (size_t i = 0; i < size; i++) new (&v[i]) T();
        return mutable_list_t<T>(v, size);
    }
    void destroy() const {
        for (size_t i = 0; i < length; i++) v[i].~T();
        free_mutable((void *)v);
    }
};

}  // namespace scee
