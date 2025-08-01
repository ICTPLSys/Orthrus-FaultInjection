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
#include "rbv.hpp"
#include "utils.hpp"
#include "../comm.hpp"

#ifdef PROFILE
#include "profile.hpp"
#endif

#ifdef PROFILE_MEM
#include "profile-mem.hpp"
#endif

hashmap_t *hm_safe = nullptr;
std::string replica_ip;

thread_local int replica_fd;

struct fd_worker {
    char *wt_buffer;
    size_t len;
    fd_reader reader;

    fd_worker(int _fd) : reader(_fd) {
        wt_buffer = (char *)malloc(kBufferSize);
        len = 0;
    }

    bool run() {
        while ((len = reader.read_packet('\n'))) {
            if (!memcmp(reader.packet, "quit", 4)) return true;
            char *packet = reader.packet;

            if (packet[0] == 's') {  // set
                Key key;
                Val val;
                memcpy(key.ch, packet + 4, KEY_LEN);
                memcpy(val.ch, packet + 4 + KEY_LEN + 1, VAL_LEN);
                RetType ret = hashmap_set(hm_safe, key, val);
                memcpy(wt_buffer, kRetVals[ret], strlen(kRetVals[ret]) + 1);
            } else if (packet[0] == 'g') {  // get
                Key key;
                memcpy(key.ch, packet + 4, KEY_LEN);
                const Val *val = hashmap_get(hm_safe, key);
                if (val != nullptr) {
                    std::string ans = kRetVals[kValue];
                    ans += std::string(val->ch, VAL_LEN);
                    ans += kCrlf;
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
                memcpy(wt_buffer, kRetVals[ret], strlen(kRetVals[ret]) + 1);
            } else {
                memcpy(wt_buffer, kRetVals[kError],
                       strlen(kRetVals[kError]) + 1);
            }

            std::string p;
            p += rbv::hasher.finalize() + "\n";
            p += std::string(packet, len) + "\n";
            write_all(replica_fd, p);

            write_all(reader.fd, wt_buffer, strlen(wt_buffer));
        }
        return false;
    }
    ~fd_worker() { free(wt_buffer); }
};

void Start(int port, int replica_port) {
    const int MAX_EVENTS = 128;  // max total active connections

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listen_fd >= 0);

    replica_fd = connect_server(replica_ip, replica_port);

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
                if (workers[fd]->run()) {
                    close(fd);
                    workers.erase(fd);
                    timeout = 10000;
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 5) {
        fprintf(stderr, "Usage: %s <port> [ngroups] [replica-port] [replica-ip]\n", argv[0]);
        fprintf(stderr, "Default values: ngroups=3, replica-port=6789, replica-ip=localhost\n");
        return 1;
    }
#ifdef PROFILE
    profile::start();
#endif
#ifdef PROFILE_MEM
    profile::mem::start();
#endif
    int port = atoi(argv[1]);
    int ngroups = 3;
    if (argc >= 3) ngroups = atoi(argv[2]);
    int replica_port = 6789;
    if (argc >= 4) replica_port = atoi(argv[3]);
    replica_ip = "localhost";
    if (argc >= 5) replica_ip = argv[4];
    hm_safe = hashmap_t::make(1 << 24);
    std::vector<std::thread> threads;
    for (int i = 0; i < ngroups; ++i) {
        threads.emplace_back(Start, port + i, replica_port + i);
    }
    for (auto &thread : threads) {
        thread.join();
    }
#ifdef PROFILE_MEM
    profile::mem::stop();
#endif
#ifdef PROFILE
    profile::stop();
#endif
    return 0;
}