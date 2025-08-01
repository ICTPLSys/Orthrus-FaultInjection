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

static constexpr size_t kBufferSize = 1 << 14, kMaxCmdLen = 1 << 10;

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

    my_usleep(1000);
    return fd;
}
