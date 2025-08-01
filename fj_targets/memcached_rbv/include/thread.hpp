#pragma once

#include <atomic>
#include <thread>
#include <tuple>
#include <utility>
#include "utils.hpp"

/*
    Do we actually need user level thread?
    Currently, this is a wrapper of std::thread.
*/

namespace scee {

// constexpr size_t NCPU = 48;

/* APIs */

class Thread {
public:
    Thread() noexcept;
    Thread(const Thread &) = delete;
    Thread &operator=(const Thread &) = delete;
    Thread(Thread &&other) noexcept;
    Thread &operator=(Thread &&other) noexcept;
    ~Thread() = default;

    template <typename F, typename... Args>
    explicit Thread(F &&f, Args &&... args);

    void join();
    void detach();

protected:
    std::thread thread;
};

class AppThread : public Thread {
    template <typename F, typename... Args>
    friend auto main_thread(F &&f, Args &&... args);

public:
    AppThread() noexcept = default;
    AppThread(const AppThread &) = delete;
    AppThread &operator=(const AppThread &) = delete;
    AppThread(AppThread &&other) noexcept = default;
    AppThread &operator=(AppThread &&other) noexcept = default;

    template <typename F, typename... Args>
    explicit AppThread(F &&f, Args &&... args);

private:
    static void register_queue();
    static void unregister_queue();
};

template <typename F, typename... Args>
auto main_thread(F &&f, Args &&... args);

/* Internal Implementations */

inline Thread::Thread() noexcept : thread() {}

inline Thread::Thread(Thread &&other) noexcept
    : thread(std::move(other.thread)) {}

inline Thread &Thread::operator=(Thread &&other) noexcept {
    thread = std::move(other.thread);
    return *this;
}

extern int core_id, sampling_rate, sampling_method;

template <typename F, typename... Args>
inline Thread::Thread(F &&f, Args &&... args)
    : thread(std::forward<F>(f), std::forward<Args>(args)...) {
   // bind_core(thread.native_handle(), (core_id + NCPU / 2) % NCPU);
}

inline void Thread::join() { thread.join(); }

inline void Thread::detach() { thread.detach(); }

template <typename F, typename... Args>
inline AppThread::AppThread(F &&f, Args &&... args) {
    thread = std::thread([f = std::forward<F>(f), &args...] {
        register_queue();
        f(std::forward<Args>(args)...);
        unregister_queue();
    });
    // bind_core(thread.native_handle(), core_id % NCPU);
    // core_id++;
}

template <typename F, typename... Args>
auto main_thread(F &&f, Args &&... args) {
#ifdef SAMPLING
    FILE *fp = fopen("sampling.config", "r");
    char method[10];
    int percentage;
    fscanf(fp, "%s %d", method, &percentage);
    fclose(fp);
    sampling_method = strcmp(method, "random") == 0 ? 1 : 0;
    sampling_rate = percentage;
    fprintf(stderr, "sampling method: %s, sampling rate: %d\n", method, percentage);
#else
    sampling_method = 1;
    sampling_rate = 100;
#endif
    AppThread::register_queue();
    auto ret = f(std::forward<Args>(args)...);
    AppThread::unregister_queue();
    return ret;
}

}  // namespace scee
