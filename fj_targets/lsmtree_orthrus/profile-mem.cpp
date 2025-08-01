#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>

#include "utils.hpp"

#include "profile-mem.hpp"

namespace profile::mem {

#ifndef ENABLE_PROFILE
#define ENABLE_PROFILE true
#endif

bool profile_mem_enabled = false;

const char* MEM_PROFILE_FILENAME = "memory_status.log";
void init_mem(const char* filename = "memory_status.log") {
    MEM_PROFILE_FILENAME = filename;
}

void start() {
    fprintf(stderr, "Profile-mem.cpp: memory usage profile: %d\n", ENABLE_PROFILE);
    #if (ENABLE_PROFILE)
        profile_mem_enabled = true;
    #endif
}

void stop() {}

struct MemoryProfile {
    bool is_running = false;
    std::thread t_monitor;

    void run() {
        while (is_running && !profile_mem_enabled) { my_usleep(500); }
        if (!is_running) return;

        FILE* fout = fopen(MEM_PROFILE_FILENAME, "w");
        fprintf(stderr, "MemoryProfile Starts\n");

        while (is_running) {
            FILE* f = fopen("/proc/self/status", "r");
            if (!f) { fprintf(stderr, "fopen failed\n"); }
            char line[255];
            while (f && fgets(line, sizeof(line), f)) {
                if (!strncmp(line, "VmRSS", 5)) {
                    fprintf(fout, "%s", line);
                }
            }
            fclose(f);
            my_usleep(1000);
        }
        fflush(fout);
        fclose(fout);
    }

    MemoryProfile() {
        #if (ENABLE_PROFILE)
            is_running = true;
            t_monitor = std::thread([&]() { run(); });
        #else
            profile_mem_enabled = false;
        #endif
    }

    ~MemoryProfile() {
        #if (ENABLE_PROFILE)
            is_running = false;
            t_monitor.join();
            fprintf(stderr, "MemoryProfile Stopped\n");
        #endif
    }
} g_mem_profile;

}  // namespace profile
