#pragma once

#include <thread>
#include <tuple>
#include <utility>

/*
    Do we actually need user level thread?
    Currently, this is a wrapper of std::thread.
*/

namespace scee {

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
    explicit Thread(F &&f, Args &&...args);

    void join();
    void detach();

protected:
    std::thread thread;
};

class AppThread : public Thread {
    template <typename F, typename... Args>
    friend auto main_thread(F &&f, Args &&...args);

public:
    AppThread() noexcept = default;
    AppThread(const AppThread &) = delete;
    AppThread &operator=(const AppThread &) = delete;
    AppThread(AppThread &&other) noexcept = default;
    AppThread &operator=(AppThread &&other) noexcept = default;

    template <typename F, typename... Args>
    explicit AppThread(F &&f, Args &&...args);

private:
    static void register_queue();
    static void unregister_queue();
};

template <typename F, typename... Args>
auto main_thread(F &&f, Args &&...args);

/* Internal Implementations */

inline Thread::Thread() noexcept : thread() {}

inline Thread::Thread(Thread &&other) noexcept
    : thread(std::move(other.thread)) {}

inline Thread &Thread::operator=(Thread &&other) noexcept {
    thread = std::move(other.thread);
    return *this;
}

template <typename F, typename... Args>
inline Thread::Thread(F &&f, Args &&...args)
    : thread(std::forward<F>(f), std::forward<Args>(args)...) {}

inline void Thread::join() { thread.join(); }

inline void Thread::detach() { thread.detach(); }

template <typename F, typename... Args>
inline AppThread::AppThread(F &&f, Args &&...args) {
    thread = std::thread([f = std::forward<F>(f), &args...] {
        register_queue();
        f(std::forward<Args>(args)...);
        unregister_queue();
    });
}

template <typename F, typename... Args>
auto main_thread(F &&f, Args &&...args) {
    AppThread::register_queue();
    auto ret = f(std::forward<Args>(args)...);
    AppThread::unregister_queue();
    return ret;
}

}  // namespace scee
