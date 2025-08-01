#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <atomic>
#include <unistd.h>

#include <vector>
#include <map>

#include "utils.hpp"

#include <gflags/gflags.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include "thread.hpp"

#include "helpers.hpp"
#include "zipf.hpp"


DEFINE_int32(port, 114514, "main port");
DEFINE_int32(total_ops, 50'000'000, "total sent msg");
DEFINE_int32(MAX_EVENTS, 1024, "max events");

DEFINE_int32(num_clients, 8, "parallel clients");


std::vector<int64_t> some_buffer;
std::vector<uint64_t> latency_req;

const char* msg_ret = "Ok\n";


void client() {
    std::vector<std::thread> threads;

    auto start = microtime();
    for (int i = 0; i < FLAGS_num_clients; i++) {
        my_nsleep(8);
        threads.emplace_back([i] {
            char *rx_buf[1024];
            int fd = connect_to("127.0.0.1", FLAGS_port);
            auto now = microtime();
            for (int j = 0; j < FLAGS_total_ops / FLAGS_num_clients; j++) {
                size_t idx = i * FLAGS_num_clients + j;
                auto key = all_keys[zipf_key_indices[idx]];
                auto value = all_vals[zipf_key_indices[idx]];

                // fprintf(stderr, "Sending: %ld\n", key);

                now = microtime();
                Msg msg {
                    .cmd = Cmd::Set,
                    .idx = idx,
                    .key = key,
                    .value = value,
                    .timestamp = rdtsc(),
                };
                write_all(fd, (const char*)&msg, sizeof(msg));
                auto ret = read_all(fd, rx_buf, 3);
                ASSERT(
                    0 == strcmp((char*)rx_buf, msg_ret),
                    "Invalid Ret Msg: %s, size = %ld",
                    (char*)rx_buf, ret
                );
                latency_req[idx] = rdtsc() - msg.timestamp;
            }
        });
    }

    for (auto&& t : threads) { t.join(); }

    auto end = microtime();
    auto duration = microtime_diff(start, end);
    std::cerr << "execution time: " << duration
              << ", throughput: " << 1.0 * FLAGS_total_ops / (duration / 1E6)
              << "\n";

    int fd = connect_to("127.0.0.1", FLAGS_port);
    Msg msg{
        .cmd = Cmd::Stop,
        .idx = (size_t)FLAGS_total_ops,
        .key = 1,
        .value = 1,
        .timestamp = rdtsc(),
    };
    char* rx_buf[1024];
    write_all(fd, (const char*)&msg, sizeof(msg));
    // auto _ = read_all(fd, rx_buf, 3);
    DEBUG("clients exit");
}

int main(int argc, char* argv[]) {
    gflags::SetUsageMessage("Usage: ./socket [--total-ops=<int>]");
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    auto drop = ScopeGuard([&]() {
        gflags::ShutDownCommandLineFlags();
    });

    latency_req.resize(FLAGS_total_ops + 1);

    total_ops = FLAGS_total_ops;
    init_zipf();

    client();

    std::cout << "latency_req" << std::endl;
    print_status(latency_req);
    return 0;
}
