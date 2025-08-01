#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <cstdio>

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

#include "profile-mem.hpp"

#ifndef FORCE_SYNC
#define FORCE_SYNC 0
#endif

DEFINE_int32(port, 114514, "main port");
DEFINE_int32(port_replica, 114515, "replica port");
DEFINE_int32(total_ops, 50'000'000, "total sent msg");
DEFINE_int32(MAX_EVENTS, 1024, "max events");
DEFINE_int32(server_workers, 2, "max events");

std::vector<int> some_buffer;
std::atomic_bool isRunning;

std::vector<uint64_t> latency_val;

std::mutex lsm_lock_1;
scee::lsmtree::LSMTree lsm1("/dev/shm/lsmtree-net-rbv-1");

std::mutex lsm_lock_2;
scee::lsmtree::LSMTree lsm2("/dev/shm/lsmtree-net-rbv-2");

const char* msg_ret = "Ok\n";

template<bool isMain>
struct ServerFdWorker {
    ssize_t kBufferSize = 10240;

    uint8_t* rd_buf;
    uint8_t* rx_buf;
    uint8_t* tmp_rx_buf;

    ssize_t rx_bytes;
    ssize_t cur_pos;

    int fd = -1;
    int fd_replica = -1;

    std::mutex* lsm_lock = nullptr;
    scee::lsmtree::LSMTree* lsm = nullptr;
    std::queue<int> new_fds_queue;
    std::mutex queue_mutex;

    ServerFdWorker(int fd) : fd(fd) {
        if constexpr (isMain) {
            lsm = &lsm1;
            lsm_lock = &lsm_lock_1;
        } else {
            lsm = &lsm2;
            lsm_lock = &lsm_lock_2;
        }
        rx_buf = new uint8_t[kBufferSize << 1];
        rd_buf = new uint8_t[kBufferSize];
        tmp_rx_buf = new uint8_t[kBufferSize];
        rx_bytes = 0;

        if constexpr (isMain) {
            fd_replica = connect_to("127.0.0.1", FLAGS_port_replica);
        }
    }

    ~ServerFdWorker() {
        delete rx_buf;
        delete rd_buf;
        delete tmp_rx_buf;
        close(fd_replica);
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
                    fprintf(stderr, "Cmd::Stop, exiting\n");
                    if constexpr (isMain) {
                        write_all(fd_replica, (const char*)&msg, sizeof(msg));
                    } else {
                        isRunning.store(false);
                    }
                    return;
                }
                case Cmd::Set: {
                    if constexpr (isMain) { // run in server mode
                        // Stage 1
                        msg.timestamp = rdtsc();
                        {
                            std::lock_guard<std::mutex> lock(*lsm_lock);
                            lsm->Set(msg.key, msg.value);
                        }

                        // Stage 2
                        // Sim RBV Internal Stats data
                        // avg. access chain length: accCnt = 44
                        //		Measured by `lsmtree_throughput_stats`
                        // avg. access internal states: accCnt * sizeof(void*)
                        //      ignore the Data update or malloc cost
                        size_t cnt = 44;
                        void* ptr = malloc(sizeof(void*) * cnt);
                        for (int i = 0; i < cnt; i++) {
                            some_buffer[i] = i;
                        }
                        memcpy(ptr, some_buffer.data(), cnt * sizeof(void*));
                        free(ptr);
                        // end of sim rbv

                        write_all(fd_replica, (const char*)&msg, sizeof(msg));

                        // Async, without waiting reply
                        #if (FORCE_SYNC)
                            read_all(fd_replica, &msg, sizeof(msg));
                        #endif

                        // Async, without waiting reply
                        write_all(fd, msg_ret, 3);
                    } else {  // run in replica
                        {
                            std::lock_guard<std::mutex> lock(*lsm_lock);
                            lsm->Set(msg.key, msg.value);
                        }
                        #if (FORCE_SYNC)
                            write_all(fd, (const char*)&msg, sizeof(msg));
                        #endif
                        #if (ENABLE_PROFILE_VAL_CDF)
                            latency_val[msg.idx] = rdtsc() - msg.timestamp;
                        #endif
                    }

                }
                default:
                    ;
            }
        }
        // DEBUG("worker exits");
    }
};

template<bool isMain>
class Worker {
public:
    int epoll_fd;
    std::thread worker_thread;
    std::map<int, std::unique_ptr<ServerFdWorker<isMain>>> fd_workers;

    std::queue<int> new_fds_queue;
    std::mutex queue_mutex;

    Worker() {
        epoll_fd = epoll_create1(0);
    }

    void Start() {
        worker_thread = std::thread(&Worker::Run, this);
    }

    void AddNewFd(int fd) {
        std::lock_guard<std::mutex> lock(queue_mutex);
        new_fds_queue.push(fd);
    }

    void RegisterNewConnection(int sock) {
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        epoll_event ev = {};
        ev.events = EPOLLIN;
        ev.data.fd = sock;

        if (epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, sock, &ev) < 0) {
            KDEBUG("epoll_ctl failed for sock %d: %s", sock, strerror(errno));
            close(sock);
        } else {
            fd_workers[sock] = std::make_unique<ServerFdWorker<isMain>>(sock);
            KDEBUG("Worker thread registered new connection fd: %d", sock);
        }
    }

    void Run() {
        epoll_event events[FLAGS_MAX_EVENTS];

        while (isRunning.load()) {
            int new_fd = -1;
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                if (!new_fds_queue.empty()) {
                    new_fd = new_fds_queue.front();
                    new_fds_queue.pop();
                }
            }
            if (new_fd != -1) {
                RegisterNewConnection(new_fd);
            }

            int nfds = epoll_wait(epoll_fd, events, FLAGS_MAX_EVENTS, 1);

            for (int i = 0; i < nfds; ++i) {
                int event_fd = events[i].data.fd;
                if (events[i].events & EPOLLIN) {
                    fd_workers[event_fd]->Run();
                }
            }
        }
    }
};

template<bool isMain>
struct Server {
    int fd = -1;
    int epoll_fd = -1;
    std::vector<std::unique_ptr<Worker<isMain>>> workers;
    int next_worker_idx = 0;

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
        if constexpr (isMain) {
            address.sin_port = htons(FLAGS_port);
        } else {
            address.sin_port = htons(FLAGS_port_replica);
        }

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
        KDEBUG("%s Running", isMain ? "server" : "replica");

        isRunning.store(true);

        int num_workers = FLAGS_server_workers;
        for (int i = 0; i < num_workers; i++) {
            workers.emplace_back(std::make_unique<Worker<isMain>>());
            workers.back()->Start();
        }

        epoll_fd = epoll_create1(0);
        if (epoll_fd < 0) abort();

        epoll_event ev = {};
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            abort();
        }

        epoll_event events[FLAGS_server_workers + 1];

        auto timeout = -1;

        while (isRunning.load()) {
            int nfds = epoll_wait(epoll_fd, events, 10, 1000);
            if (nfds < 0) {
                if (errno == EINTR) continue;
                break;
            }

            for (int i = 0; i < nfds; ++i) {
                if (events[i].data.fd == fd) {
                    sockaddr_in client_address;
                    socklen_t client_len = sizeof(client_address);
                    int client_fd = accept(fd, (sockaddr*)&client_address, &client_len);

                    if (client_fd < 0) {
                        KDEBUG("client fd failed");
                        abort();
                    }

                    workers[next_worker_idx]->AddNewFd(client_fd);
                    next_worker_idx = (next_worker_idx + 1) % workers.size();
                }
            }
        }

        for (auto& worker : workers) {
            worker->worker_thread.join();
        }
        close(fd);
        close(epoll_fd);
        DEBUG("server exit");
    }
};


int main(int argc, char* argv[]) {
    fprintf(stderr, "Profile Mem: %d\n", ENABLE_PROFILE_MEM);
    fprintf(stderr, "Profile Val CDF: %d\n", ENABLE_PROFILE_VAL_CDF);

    profile::mem::init_mem("lsmtree-memory_status-rbv.log");

    some_buffer.resize(1024);

    gflags::SetUsageMessage("Usage: ./socket [--total-ops=<int>]");
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    auto drop = ScopeGuard([&]() {
        gflags::ShutDownCommandLineFlags();
    });

    #if (ENABLE_PROFILE_VAL_CDF)
        latency_val.resize(FLAGS_total_ops + 1);
    #endif

    std::thread t_replica([&]() {
        Server<false> replica;
        replica.Run();
        return 0;
    });

    std::thread t_server([&]() {
        #if (ENABLE_PROFILE_MEM)
            profile::mem::start();
            Server<true> server;
            server.Run();
            profile::mem::stop();
        #else
            Server<true> server;
            server.Run();
        #endif
        return 0;
    });

    t_server.join();
    t_replica.join();

    #if (ENABLE_PROFILE_VAL_CDF)
        FILE* fout = fopen("lsmtree-validation_latency-rbv.raw", "w+");
        for (auto lat : latency_val) {
            fprintf(fout, "%lu\n", lat);
        }
        fclose(fout);

        hdr_print_status("lsmtree-validation_latency-rbv.cdf", latency_val);
    #endif
    return 0;
}
