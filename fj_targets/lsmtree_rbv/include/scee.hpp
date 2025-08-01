#pragma once

#include <x86intrin.h>

#include <type_traits>
#include <utility>

#include "log.hpp"

namespace scee {

struct Validable {
    virtual void validate(LogReader *) const = 0;
};

template <typename Ret, typename... Args>
struct Closure : public Validable {
    using Fn = Ret (*)(Args...);

    Fn fn;
    std::tuple<Args...> args;

    explicit Closure(Fn fn, Args &&...args)
        : fn(std::move(fn)), args(std::move(args)...) {}

    auto run() const { return std::apply(fn, args); }

    auto run_with_fn(Fn fn) const { return std::apply(fn, args); }

    void validate(LogReader *reader) const override {
        reader->template skip<sizeof(*this)>();
        if constexpr (std::is_void_v<Ret>) {
            run();
        } else {
            auto ret = run();
            reader->cmp_log_typed(ret);
        }
    }
};

template <typename Ret, typename... Args>
Ret run(Ret (*fn)(Args...), Args... args) {
    static_assert(std::is_trivial_v<Ret>);
    new_log();
    const auto *func =
        append_log_typed(Closure(fn, std::forward<Args>(args)...));
    Ret ret = func->run();
    append_log_typed(ret);
    commit_log();
    return ret;
}

template <typename Ret, typename... Args>
Ret run2(Ret (*app_fn)(Args...), Ret (*val_fn)(Args...), Args... args) {
    static_assert(std::is_void_v<Ret> || std::is_trivial_v<Ret>);
    new_log();
    // fprintf(stderr, "new: %p\n", thread_log_manager.current_log.head);
    const auto *func =
        append_log_typed(Closure(val_fn, std::forward<Args>(args)...));
    if constexpr (std::is_void_v<Ret>) {
        func->run_with_fn(app_fn);
        commit_log();
    } else {
        Ret ret = func->run_with_fn(app_fn);
        append_log_typed(ret);
        commit_log();
        return ret;
    }
    // commit_log();
    // return app_fn(std::forward<Args>(args)...);
}


template <typename Ret, typename... Args>
Ret run2_profile(uint64_t &cycles, Ret (*app_fn)(Args...),
                 Ret (*val_fn)(Args...), Args... args) {
    static_assert(std::is_void_v<Ret> || std::is_trivial_v<Ret>);
    new_log();
    const auto *func =
        append_log_typed(Closure(val_fn, std::forward<Args>(args)...));
    if constexpr (std::is_void_v<Ret>) {
        uint64_t start = _rdtsc();
        func->run_with_fn(app_fn);
        cycles = _rdtsc() - start;
        commit_log();
    } else {
        uint64_t start = _rdtsc();
        Ret ret = func->run_with_fn(app_fn);
        cycles = _rdtsc() - start;
        append_log_typed(ret);
        commit_log();
        return ret;
    }
}

void validate_one(LogHead *log);

}  // namespace scee
