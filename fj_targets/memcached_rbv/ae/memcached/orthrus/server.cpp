#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <memory>
#include <regex>
#include <vector>

#include "context.hpp"
#include "ctltypes.hpp"
#include "custom_stl.hpp"
#include "log.hpp"
#include "namespace.hpp"
#include "ptr.hpp"
#include "scee.hpp"
#include "thread.hpp"
#include "../comm.hpp"

#ifdef PROFILE
#include "profile.hpp"
#endif

#ifdef PROFILE_MEM
#include "profile-mem.hpp"
#endif

namespace raw {
#include "closure.hpp"
}  // namespace raw
namespace app {
#include "closure.hpp"
}  // namespace app
namespace validator {
#include "closure.hpp"
}  // namespace validator

using namespace raw;

ptr_t<hashmap_t> *hm_safe = nullptr;

struct fd_worker {
    char *wt_buffer;
    size_t len;
    fd_reader reader;
    fd_worker(int _fd) : reader(_fd) {
        wt_buffer = (char *)malloc(kBufferSize);
        len = 0;
    }
    int64_t parse_head_id(char *&packet, int &len) {
        int64_t head_id = 0;
        while (*packet != '#') {
            head_id = head_id * 10 + (*packet & 15);
            ++packet;
            --len;
        }
        ++packet;
        --len;
        return head_id;
    }
    bool run() {
        while ((len = reader.read_packet())) {
            if (!memcmp(reader.packet, "quit", 4)) return true;
            if (reader.packet[0] == 's') {  // set
                Key key;
                Val val;
                memcpy(key.ch, reader.packet + 4, KEY_LEN);
                memcpy(val.ch, reader.packet + 4 + KEY_LEN + 1, VAL_LEN);
                RetType ret;
                using HashmapSetType = RetType (*)(scee::ptr_t<hashmap_t> *, Key, Val);
                auto app_fn = reinterpret_cast<HashmapSetType>(app::hashmap_set);
                auto val_fn = reinterpret_cast<HashmapSetType>(validator::hashmap_set);
                ret = scee::run2(app_fn, val_fn, hm_safe, key, val);
                memcpy(wt_buffer, kRetVals[ret], strlen(kRetVals[ret]) + 1);
            } else if (reader.packet[0] == 'g') {  // get
                Key key;
                memcpy(key.ch, reader.packet + 4, KEY_LEN);
                const Val *val;
                using HashmapGetType = const Val *(*)(scee::ptr_t<hashmap_t> *, Key);
                auto app_fn = reinterpret_cast<HashmapGetType>(app::hashmap_get);
                auto val_fn = reinterpret_cast<HashmapGetType>(validator::hashmap_get);
                val = scee::run2(app_fn, val_fn, hm_safe, key);
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
            } else if (reader.packet[0] == 'd') {  // del
                Key key;
                memcpy(key.ch, reader.packet + 4, KEY_LEN);
                RetType ret;
                using HashmapDelType = RetType (*)(scee::ptr_t<hashmap_t> *, Key);
                auto app_fn = reinterpret_cast<HashmapDelType>(app::hashmap_del);
                auto val_fn = reinterpret_cast<HashmapDelType>(validator::hashmap_del);
                ret = scee::run2(app_fn, val_fn, hm_safe, key);
                memcpy(wt_buffer, kRetVals[ret], strlen(kRetVals[ret]) + 1);
            } else {
                memcpy(wt_buffer, kRetVals[kError], strlen(kRetVals[kError]) + 1);
            }
            write_all(reader.fd, wt_buffer, strlen(wt_buffer));
        }
        return false;
    }
    ~fd_worker() { free(wt_buffer); }
};

void Start(int port) {
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

    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];
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
            DEBUG("server stopped due to inactivity.\n");
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

int main_fn(int port, int num_servers) {
#ifdef PROFILE
    profile::start();
#endif
#ifdef PROFILE_MEM
    profile::mem::start();
#endif
    hm_safe = ptr_t<hashmap_t>::create(hashmap_t::make(1 << 24));
    std::vector<scee::AppThread> app_threads;
    for (int i = 0; i < num_servers; ++i) {
        app_threads.emplace_back([port, i]() { Start(port + i); });
    }
    for (auto &thread : app_threads) {
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

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <port> [num_servers]\n", argv[0]);
        return 1;
    }
    uint32_t port = atoi(argv[1]);
    uint32_t num_servers = 3;
    if (argc >= 3) num_servers = atoi(argv[2]);
    scee::main_thread(main_fn, port, num_servers);
    return 0;
}