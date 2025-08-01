#include <concepts>
#include <cstddef>
#include <utility>

namespace scee {

// 前向声明mut_array模板类
template <typename T>
class mut_array;

template <typename T>
class mut_pointer {
private:
    T *ptr;

private:
    friend class mut_array<T>;

    explicit mut_pointer(T *ptr) : ptr(ptr) {}

public:
    mut_pointer() = default;

    template <typename... Args>
    static mut_pointer<T> create(Args &&... args) {
        return mut_pointer<T>(new T(std::forward<Args>(args)...));
    }

    static mut_pointer<T> null() { return mut_pointer<T>(nullptr); }

    void destroy() { delete ptr; }

    bool is_null() const { return ptr == nullptr; }

    T load() const { return *ptr; }

    const T *deref() const { return ptr; }

    const T *deref_logless() const { return ptr; }

    const T *deref_nullable() const { return ptr; }

    void store(T value) { *ptr = value; }
};

template <typename T>
class imm_array {
private:
    const T *ptr;

private:
    explicit imm_array(const T *ptr) : ptr(ptr) {}

public:
    imm_array() : ptr(nullptr) {}

    template <std::invocable<T *> Fn>
    static imm_array<T> create(size_t size, Fn init) {
        T *arr = new T[size];
        init(arr);
        return imm_array<T>(arr);
    }

    static imm_array<T> null() { return imm_array<T>(nullptr); }

    void destroy() const { delete[] ptr; }

    bool is_null() const { return ptr == nullptr; }

    const T *deref() const { return ptr; }

    const T *deref_nullable() const { return ptr; }
};

template <typename T>
class mut_array {
private:
    T *ptr;

private:
    explicit mut_array(T *ptr) : ptr(ptr) {}

public:
    static mut_array<T> create(size_t size) {
        T *arr = new T[size];
        return mut_array<T>(arr);
    }

    static mut_array<T> create(size_t size, T default_v) {
        T *arr = new T[size];
        for (size_t i = 0; i < size; ++i) {
            arr[i] = default_v;
        }
        return mut_array<T>(arr);
    }

    void destroy() const { delete[] ptr; }

    mut_pointer<T> get(size_t idx) const { return mut_pointer<T>(ptr + idx); }

    const T *deref(size_t idx) const { return ptr + idx; }

    const T *deref_nullable(size_t idx) const { return ptr + idx; }

    void store(size_t idx, T value) const { ptr[idx] = value; }
};

}  // namespace scee
