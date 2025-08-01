#include <cstdio>

#include "log.hpp"

int main() {
    scee::new_log();
    scee::append_log_typed<int>(1);
    scee::log_cursor_t cursor = scee::get_log_cursor();
    scee::append_log_typed<int>(2);
    scee::unroll_log(cursor);
    scee::append_log_typed<int>(3);
    scee::append_log_typed<int>(4);
    scee::commit_log();

    // to reclaim the log in the main thread
    scee::app_thread_gc_instance = &scee::thread_gc_instance;
    auto *log =
        static_cast<scee::LogHead *>(scee::log_dequeue(&scee::log_queue));
    assert(log != nullptr);
    scee::LogReader reader;
    reader.open(log);
    reader.cmp_log_typed<int>(1);
    reader.cmp_log_typed<int>(3);
    reader.cmp_log_typed<int>(4);
    reader.close();

    printf("OK\n");

    return 0;
}