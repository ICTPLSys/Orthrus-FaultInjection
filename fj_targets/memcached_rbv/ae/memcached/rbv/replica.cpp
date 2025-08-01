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

hashmap_t *hm_safe = nullptr;

struct fd_worker {
    char *wt_buffer;
    size_t len;
    fd_reader reader;
    bool validation;

    fd_worker(int _fd) : reader(_fd) {
        wt_buffer = (char *)malloc(kBufferSize);
        len = 0;
        validation = false;
    }
    bool run() {
        while ((len = reader.read_packet('\n'))) {
            if (!memcmp(reader.packet, "quit", 4)) return true;
            if (!validation) {
                std::string s = std::string(reader.packet, len - 1);
                rbv::hasher.deserialize(s);
                validation = true;
                continue;
            }
            
            len = reader.read_packet('\n');
            assert(len > 0);
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
                memcpy(wt_buffer, kRetVals[kError], strlen(kRetVals[kError]) + 1);
            }

            rbv::hasher.finalize();
            validation = false;
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
    
    int client_fd = accept(listen_fd, nullptr, nullptr);
    assert(client_fd >= 0);
    fd_worker worker(client_fd);
    while (!worker.run());
}

int main(int argc, char *argv[]) {
    if (argc < 1 || argc > 3) {
        fprintf(stderr, "Usage: %s [replica-port] [ngroups]\n", argv[0]);
        fprintf(stderr, "Default values: replica-port=6789, ngroups=3\n");
        return 1;
    }
    int replica_port = 6789;
    if (argc >= 2) replica_port = atoi(argv[1]);
    int ngroups = 3;
    if (argc >= 3) ngroups = atoi(argv[2]);
    hm_safe = hashmap_t::make(1 << 24);
    std::vector<std::thread> threads;
    for (int i = 0; i < ngroups; ++i) {
        threads.emplace_back(Start, replica_port + i);
    }
    for (auto &thread : threads) {
        thread.join();
    }
    return 0;
}