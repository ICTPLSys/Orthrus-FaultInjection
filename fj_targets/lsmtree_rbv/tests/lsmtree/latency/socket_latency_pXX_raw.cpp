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

#include "context.hpp"
#include "ctltypes.hpp"
#include "custom_stl.hpp"
#include "log.hpp"
#include "lsmtree-closure.hpp"
#include "lsmtree.hpp"
#include "namespace.hpp"
#include "ptr.hpp"
#include "scee.hpp"
#include "thread.hpp"

#include "helpers.hpp"
#include "zipf.hpp"


DEFINE_int32(port, 114514, "main port");
DEFINE_int32(total_ops, 50'000'000, "total sent msg");
DEFINE_int32(MAX_EVENTS, 1024, "max events");

std::vector<int64_t> some_buffer;

std::atomic_bool isRunning;

std::vector<uint64_t> latency_net;

std::mutex lsm_lock;
scee::lsmtree::LSMTree lsm("/dev/shm/lsmtree-net");

const char* msg_ret = "Ok\n";

struct ServerFdWorker {
    ssize_t kBufferSize = 10240;

    uint8_t* rd_buf;
    uint8_t* rx_buf;
    uint8_t* tmp_rx_buf;

    ssize_t rx_bytes;
    ssize_t cur_pos;

    int fd;

    ServerFdWorker(int fd) : fd(fd) {
        rx_buf = new uint8_t[kBufferSize << 1];
        rd_buf = new uint8_t[kBufferSize];
        tmp_rx_buf = new uint8_t[kBufferSize];
        rx_bytes = 0;
    }

    ~ServerFdWorker() {
        delete rx_buf;
        delete rd_buf;
        delete tmp_rx_buf;
    }

    int read_from_socket() {
        ssize_t ret = read(fd, rx_buf + rx_bytes, kBufferSize - rx_bytes);
        if (ret <= 0) {
            return -1;
        }
        rx_bytes += ret;
        return 0;
    }

    int read_packet(ssize_t len) {
        if (cur_pos >= kBufferSize || rx_bytes == 0) {
            cur_pos = 0;
            if (read_from_socket() < 0) {
                ASSERT((errno == EAGAIN) || (errno == EWOULDBLOCK), "Invalid errno");
                return 0;
            }
        }

        if (rx_bytes < len) {
            memcpy(rx_buf, rx_buf + cur_pos, rx_bytes);
            cur_pos = 0;
            if (read_from_socket() < 0) {
                ASSERT((errno == EAGAIN) || (errno == EWOULDBLOCK), "Invalid errno");
                return 0;
            }
        }

        if (rx_bytes < len) {
            return 0;
        }

        memcpy(rd_buf, rx_buf + cur_pos, len);
        cur_pos += len;
        rx_bytes -= len;
        return len;
    }

    void Run() {
        while(auto len = read_packet(sizeof(Msg))) {
            Msg msg = *(Msg*)this->rd_buf;
            auto recv_timestamp = rdtsc();
            auto diff = recv_timestamp - msg.timestamp;
            // printf("%d, latency: %d\n", msg.idx, diff);
            switch (msg.cmd) {
                case Cmd::Stop: {
                    isRunning.store(false);
                    return;
                }
                case Cmd::Set: {
                    latency_net[msg.idx] = diff;
                    // printf("Key: %ld\n", msg.key);
                    lsm.Set(msg.key, msg.value);
                    write_all(fd, msg_ret, 3);
                }
                default:
                    ;
            }
        }
        // DEBUG("worker exits");
    }
};

struct Server {
    int fd;
    int epoll_fd;
    std::map<int, std::unique_ptr<ServerFdWorker>> fd_workers;

    void init_listen_fd() {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            KDEBUG("socket() failed: %s", strerror(errno));
            abort();
        }
        int opt = 1; // avoid port occupied
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        int flag = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
        setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, &flag, sizeof(flag));


        sockaddr_in address = {};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(FLAGS_port);

        if (bind(fd, (sockaddr *)&address, sizeof(address)) < 0) {
            KDEBUG("bind() failed: %s", strerror(errno));
            close(fd); abort();
        }

        if (listen(fd, SOMAXCONN) < 0) {
            KDEBUG("listen() failed: %s", strerror(errno));
            close(fd); abort();
        }
    }

    void Run() {
        init_listen_fd();
        KDEBUG("%s Running", "server");

        epoll_fd = epoll_create1(0);
        if (epoll_fd < 0) {
            KDEBUG("epoll_create1() failed: %s", strerror(errno));
            close(fd); abort();
        }

        epoll_event ev = {};
        epoll_event events[FLAGS_MAX_EVENTS];

        auto epoll_init = [&](int sock) {
            int flags = fcntl(sock, F_GETFL, 0);
            fcntl(sock, F_SETFL, flags | O_NONBLOCK);
            // ev.events = EPOLLIN | EPOLLET;  // Monitor for read events
            ev.events = EPOLLIN; // Monitor for read events
            ev.data.fd = sock;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev) < 0) {
                KDEBUG("epoll_ctl failed: %s", strerror(errno));
                close(fd); close(epoll_fd); abort();
            }
        };

        epoll_init(fd);

        isRunning.store(true);

        auto timeout = -1;

        while (isRunning.load()) {
            int nfds = epoll_wait(epoll_fd, events, FLAGS_MAX_EVENTS, timeout);
            if (nfds < 0) {
                KDEBUG("epoll_wait failed: %s", strerror(errno)); break;
            } else if (nfds == 0) {
                continue;
            }

            for (int i = 0; i < nfds; ++i) {
                auto event_fd = events[i].data.fd;
                auto event = events[i].events;

                if (event_fd == fd) {
                    KDEBUG("incomming connection");

                    sockaddr_in client_address;
                    socklen_t client_len = sizeof(client_address);
                    int client_fd = accept(fd, (sockaddr *)&client_address, &client_len);
                    if (client_fd < 0) {
                        KDEBUG("accept failed: %s", strerror(errno));
                        continue;
                    }
                    epoll_init(client_fd);
                    fd_workers[client_fd] = std::make_unique<ServerFdWorker>(client_fd);
                } else if ((event & (EPOLLERR | EPOLLHUP)) && !(event & EPOLLIN)) {
                    // client connection closed
                    close(event_fd);
                    fd_workers.erase(event_fd);
                } else {
                    fd_workers[event_fd]->Run();
                    timeout = 1;
                }
            }
        }

        fd_workers.clear();
        close(fd);
        close(epoll_fd);
        DEBUG("server exit");
    }
};


int main(int argc, char* argv[]) {
    some_buffer.resize(1024);

    gflags::SetUsageMessage("Usage: ./socket [--total-ops=<int>]");
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    auto drop = ScopeGuard([&]() {
        gflags::ShutDownCommandLineFlags();
    });

    latency_net.resize(FLAGS_total_ops + 1);

    std::thread t_server([&]() {
        Server server;
        server.Run();
        return 0;
    });

    t_server.join();

    std::cout << "latency_net" << std::endl;
    print_status(latency_net);

    return 0;
}
