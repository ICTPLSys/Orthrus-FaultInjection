#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cassert>
#include <memory>
#include <regex>
#include <thread>
#include <vector>

#include "hashmap.hpp"
#include "profile.hpp"
#include "rbv.hpp"

#ifdef WITH_MONITOR
#include "monitor.hpp"
#endif

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
hashmap_t *hm_safe = nullptr;

#ifdef WITH_MONITOR
monitor::process_memory *mem_monitor = nullptr;
monitor::process_cpu *cpu_monitor = nullptr;
static thread_local uint64_t n_done = 0;
static constexpr uint64_t record_interval = 500000;
#endif

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
                if (errno == 0) {
                    fprintf(stderr, "connection closed by the other side\n");
                    exit(1);
                }
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

thread_local int replica_fd;
long long time_diff;

struct fd_worker {
    char *wt_buffer;
    size_t len;
    fd_reader reader;

    fd_worker(int _fd) : reader(_fd) {
        wt_buffer = (char *)malloc(kBufferSize);
        len = 0;
    }
    uint64_t parse_head_id(char *&packet, size_t &len) {
        uint64_t head_id = 0;
        while (*packet != '#') {
            head_id = head_id * 10 + (*packet & 15);
            ++packet;
            --len;
        }
        ++packet;
        --len;
        return head_id;
    }
    void run() {
        while ((len = reader.read_packet('\x01'))) {
#ifdef WITH_MONITOR
            if (n_done++ % record_interval == 0) {
                mem_monitor->record();
                cpu_monitor->record();
            }
#endif
            char *packet = reader.packet;
            long long start_us = profile::get_us_abs() + time_diff;

            if (packet[0] == 's') {  // set
                Key key;
                Val val;
                memcpy(key.ch, packet + 4, KEY_LEN);
                memcpy(val.ch, packet + 4 + KEY_LEN + 1, VAL_LEN);
                RetType ret = hashmap_set(hm_safe, key, val);
                rbv::hasher.combine((uint64_t)ret);
                memcpy(wt_buffer, kRetVals[ret], strlen(kRetVals[ret]) + 1);
            } else if (packet[0] == 'g') {  // get
                Key key;
                memcpy(key.ch, packet + 4, KEY_LEN);
                const Val *val = hashmap_get(hm_safe, key);
                if (val != nullptr) {
                    std::string ans = kRetVals[kValue];
                    ans += std::string(val->ch, VAL_LEN);
                    ans += kCrlf;
                    rbv::hasher.combine(ans);
                    memcpy(wt_buffer, ans.data(), ans.size());
                    wt_buffer[ans.size()] = '\0';
                } else {
                    memcpy(wt_buffer, kRetVals[kNotFound],
                           strlen(kRetVals[kNotFound]) + 1);
                }
            } else if (packet[0] == 'd') {  // del
                Key key;
                memcpy(key.ch, packet + 4, KEY_LEN);
                RetType ret = hashmap_del(hm_safe, key);
                rbv::hasher.combine((uint64_t)ret);
                memcpy(wt_buffer, kRetVals[ret], strlen(kRetVals[ret]) + 1);
            } else {
                memcpy(wt_buffer, kRetVals[kError],
                       strlen(kRetVals[kError]) + 1);
            }

            std::string p;
            p += rbv::toString20(start_us) + rbv::hasher.finalize() + "\x01";
            p += std::string(packet, len) + "\x01";
            write_all(replica_fd, p);

            write_all(reader.fd, wt_buffer, strlen(wt_buffer));
        }
    }
    ~fd_worker() { free(wt_buffer); }
};

const int kReplicaOffset = 11211;

int connect_server(int port) {
    struct sockaddr_in server_addr;
    int fd;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf(" create socket error!\n ");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_aton("localhost", &server_addr.sin_addr);
    server_addr.sin_port = htons(port + kReplicaOffset);

    if (connect(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        assert(false);
    }

    return fd;
}

void Start(int port, int replica_port) {
    const int MAX_EVENTS = 128;  // max total active connections

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listen_fd >= 0);

    replica_fd = connect_server(replica_port);
    {
        fd_reader clock_reader(replica_fd);
        long long client_us = profile::get_us_abs();
        write_all(replica_fd, std::to_string(client_us) + "\x01");
        size_t len = clock_reader.read_packet('\x01');
        assert(len > 0);
        long long server_us = atoll(std::string(clock_reader.packet, len - 1).c_str());
        long long client_us_2 = profile::get_us_abs();
        time_diff = server_us - client_us_2;
    }

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

    while (true) {
        int nfds = epoll_wait(efd, events, MAX_EVENTS, timeout);
        if (nfds == -1) {
            if (errno == EINTR) {
                // avoid the interrupt caused by strace
                continue;
            }
            // fprintf(stderr, ("epoll wait receive error code = " +
            // std::to_string(errno)).c_str());
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
                // new client connection
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
                // client connection closed
                close(fd);
                workers.erase(fd);
            } else {
                // receive message on this client socket
                workers[fd]->run();
                timeout = 300000;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Usage: %s <port> [ngroups] [replica_port] [log_file]\n", argv[0]);
        fprintf(stderr, "Default values: ngroups=3, replica_port=6789, log_file=server.log\n");
        return 1;
    }
    uint32_t port = atoi(argv[1]);
    int ngroups = 3;
    if (argc >= 3) ngroups = atoi(argv[2]);
    int replica_port = 6789;
    if (argc >= 4) replica_port = atoi(argv[3]);
    std::string log_file = argc >= 5 ? argv[4] : "server.log";
#ifdef WITH_MONITOR
    FILE *logger = fopen(log_file.c_str(), "a");
    fprintf(logger, "vanilla server setting ngroups=%d\n", ngroups);
    mem_monitor = new monitor::process_memory(logger, ngroups * 2);
#endif
    hm_safe = hashmap_t::make(1 << 23);
    std::vector<std::thread> threads;
    for (int i = 0; i < ngroups; ++i) {
        threads.emplace_back(Start, port + i, replica_port + i);
    }
    for (auto &thread : threads) {
        thread.join();
    }
#ifdef WITH_MONITOR
    delete mem_monitor;
    fclose(logger);
#endif
    return 0;
}