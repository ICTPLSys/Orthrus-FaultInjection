#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <random>
#include <thread>
#include <vector>

#include "monitor.hpp"
#include "profile.hpp"
#include "utils.hpp"

std::string ip = "localhost";
uint32_t port = 23333;

int connect_server() {
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

int main() {
    FILE *fp = fopen("trace.log", "r");
    vector<uint32_t> vvs;
    uint32_t l, cnt;
    while (fscanf(fp, "%u %u", &l, &cnt) != EOF) {
        while (cnt--) vvs.push_back(l);
    }
    std::mt19937 rng(2345678);
    std::shuffle(vvs.begin(), vvs.end(), rng);
    int fd = connect_server();
    assert(fd >= 0);

    constexpr uint32_t kBufferSize = 1024;
    constexpr uint32_t kNumOpsPerThread = 1000000;
    std::vector<char> tx_buf(kBufferSize);
    std::vector<char> rx_buf(kBufferSize);
    monitor::evaluation monitor(kNumOpsPerThread, kNumThreads);
    for (uint32_t k = 0; k < kNumOpsPerThread; ++k) {
        auto &key = all_keys[zipf_key_indices[k * kNumThreads + i]];
        auto &val = all_vals[zipf_key_indices[k * kNumThreads + i]];
        char buf[VAL_LEN];
        size_t val_len;
        size_t len = prepare_getcmd(tx_buf.data(), key.data);
        uint64_t timestamp = rdtsc();
        write_all(fd, tx_buf.data(), len);
        size_t rx_len = read(fd, rx_buf.data(), kBufferSize);
        monitor.latency[k * kNumThreads + i] = rdtsc() - timestamp;
        assert(rx_len > 0);
        int r = parse_getret(rx_buf.data(), rx_len, buf, VAL_LEN, &val_len);
        if (r != 0) {
            printf("Get error: key %s\n",
                   std::string(key.data, KEY_LEN).c_str());
        } else {
            assert(val_len == VAL_LEN);
            if (memcmp(val.data, buf, VAL_LEN)) {
                printf("Get error: key %s, val %s %s\n",
                       std::string(key.data, KEY_LEN).c_str(),
                       std::string(val.data, VAL_LEN).c_str(),
                       std::string(buf, VAL_LEN).c_str());
                assert(false);
            }
        }
        monitor.cnts[i].c++;

        uint64_t completed = (uint64_t)(k + 1) * kNumPrints;
        if (completed % kNumOpsPerThread < kNumPrints) {
            completed /= kNumOpsPerThread;
            if (completed % kNumThreads == i) {
                monitor.report();
            }
        }
    }
}
