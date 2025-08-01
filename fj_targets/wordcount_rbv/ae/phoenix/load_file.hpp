#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <shared_mutex>
#include <functional>
#include <iostream>
#include <iomanip>
#include <utility>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <memory>
#include <vector>
#include <atomic>
#include <string>
#include <thread>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <regex>
#include <map>

#include "utils.hpp"

static inline void write_all(int fd, const char *buf, size_t len) {
    size_t written = 0;
    while (written < len) {
        ssize_t ret = write(fd, buf + written, len - written);
        assert(ret > 0);
        written += ret;
    }
}

static inline void write_all(int fd, std::string s) {
    size_t written = 0, len = s.size();
    while (written < len) {
        ssize_t ret = write(fd, s.data() + written, len - written);
        assert(ret > 0);
        written += ret;
    }
}

static constexpr size_t kBufferSize = 1 << 16, kMaxCmdLen = 50000;

struct fd_reader {
    int fd;
    char *rd_buffer, *packet;
    size_t rx_bytes, cur_pos;
    fd_reader(int _fd) {
        fd = _fd;
        rd_buffer = (char *)malloc(kBufferSize);
        packet = nullptr;
        rx_bytes = cur_pos = 0;
    }
    ~fd_reader() { free(rd_buffer); }
    int read_from_socket() {
        ssize_t ret = read(fd, rd_buffer + rx_bytes, kBufferSize - rx_bytes);
        if (ret <= 0) {
            return -1;
        }
        rx_bytes += ret;
        return 0;
    }
    int read_packet(char delim = '\n') {
        if (cur_pos >= kBufferSize || rx_bytes == 0) {
            cur_pos = 0;
            if (read_from_socket() < 0) {
                assert((errno == EAGAIN) || (errno == EWOULDBLOCK));
                return 0;
            }
        }
        void *end = memchr(rd_buffer + cur_pos, delim, rx_bytes);
        if (!end) {
            assert(rx_bytes <= kMaxCmdLen);
            memcpy(rd_buffer, rd_buffer + cur_pos, rx_bytes);
            cur_pos = 0;
            if (read_from_socket() < 0) {
                assert((errno == EAGAIN) || (errno == EWOULDBLOCK));
                return 0;
            }
            end = memchr(rd_buffer + cur_pos, delim, rx_bytes);
        }
        if (!end) {
            return 0;
        }
        packet = rd_buffer + cur_pos;
        size_t len = (char *)end - (rd_buffer + cur_pos) + 1;
        cur_pos += len;
        rx_bytes -= len;
        return len;
    }
};

static inline int connect_server(std::string ip, int port) {
    struct sockaddr_in server_addr;
    int fd;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf(" create socket error!\n ");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_aton(ip.c_str(), &server_addr.sin_addr);
    server_addr.sin_port = htons(port);

    if (connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        assert(false);
    }

    return fd;
}

#ifdef RBV_PRIMARY
thread_local int replica_fd;
#endif

struct fd_worker {
    char *wt_buffer;
    size_t len;
    fd_reader reader;
    char *buffer;
    size_t buffer_len, cursor;
    fd_worker(int _fd) : reader(_fd) {
        wt_buffer = (char *)malloc(kBufferSize);
        len = 0;
        buffer = nullptr;
        buffer_len = cursor = 0;
    }
    void run() {
        while ((len = reader.read_packet('\x01'))) {
#ifdef RBV_PRIMARY
            write_all(replica_fd, reader.packet, len);
#endif
            if (buffer == nullptr) {
                buffer_len = 0;
                for (size_t i = 0; i < len; ++i) if (reader.packet[i] != '\x01') {
                    buffer_len = buffer_len * 10 + (reader.packet[i] & 15);
                }
                buffer = (char *)malloc(buffer_len);
            } else {
                cursor = atol(std::string(reader.packet, 15).c_str());
                memcpy(buffer + cursor, reader.packet + 15, len - 16);
            }
        }
    }
    ~fd_worker() { free(wt_buffer); }
};

std::pair<char *, size_t> load_file(int port) {
    const int MAX_EVENTS = 128;  // max total active connections

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listen_fd >= 0);

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = {.s_addr = INADDR_ANY},
    };
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
        0) {
        close(listen_fd);
        assert("bind error" && false);
    }
    if (listen(listen_fd, 1) < 0) {
        close(listen_fd);
        assert("listen error" && false);
    }
    printf("server listening on port %d\n", port);

    int efd = epoll_create1(0);
    if (efd == -1) {
        assert("epoll create error" && false);
    }

    struct epoll_event ev, events[MAX_EVENTS];
    auto epoll_init = [&](int fd) {
        fcntl(fd, F_SETFL, O_NONBLOCK);
        ev.data.fd = fd;
        ev.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev) == -1) {
            assert("epoll ctl error" && false);
        }
    };
    epoll_init(listen_fd);

    std::map<int, std::unique_ptr<fd_worker>> workers;

    int timeout = -1;

    uint64_t start_load_time;

    while (true) {
        int nfds = epoll_wait(efd, events, MAX_EVENTS, timeout);
        if (nfds == -1) {
            if (errno == EINTR) continue;
            assert("epoll wait error" && false);
        }
        if (nfds == 0) {
            fprintf(stderr, "server stopped due to inactivity.\n");
            break;
        }
        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            uint32_t state = events[i].events;
            if (fd == listen_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                while (true) {
                    int conn_fd =
                        accept(listen_fd, (struct sockaddr *)&client_addr,
                               &client_addr_len);
                    if (conn_fd == -1) {
                        assert((errno == EAGAIN) || (errno == EWOULDBLOCK));
                        break;
                    }
                    epoll_init(conn_fd);
                    workers[conn_fd] = std::make_unique<fd_worker>(conn_fd);
                }
            } else if ((state & (EPOLLERR | EPOLLHUP)) && !(state & EPOLLIN)) {
                close(fd);
            } else {
                if (timeout == -1) {
                    start_load_time = rdtsc();
                    timeout = 200; // allow 200ms transmission overhead
                }
                workers[fd]->run();
            }
        }
    }

    int fd = -1;
    for (auto &[fd_, worker] : workers) if (fd_ != listen_fd) {
        fd = fd_;
        break;
    }
    assert(fd != -1);

    auto ret = std::make_pair(workers[fd]->buffer, workers[fd]->buffer_len);
    uint64_t end_load_time = rdtsc();
    printf("Load time (single-thread): %zu ms for %zu bytes\n",
           microsecond(start_load_time, end_load_time) / 1000, ret.second);

    workers.clear();
    return ret;
}

std::pair<char *, size_t> load_file(const std::string &file_path) {
    FILE *file = fopen(file_path.c_str(), "r");
    if (!file) {
        std::cerr << "Error: failed to open input file" << std::endl;
        exit(1);
    }

    const int N_COPIES = 3;

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    std::cout << "Loading file, size: " << file_size * N_COPIES << " bytes" << std::endl;

    char *buffer = (char *)malloc(file_size * N_COPIES);
    fread(buffer, 1, file_size, file);
    for (int i = 1; i < N_COPIES; ++i) {
        memcpy(buffer + file_size * i, buffer, file_size);
    }
    fclose(file);
    std::cout << "File loaded" << std::endl;

    return {buffer, file_size * N_COPIES};
}
