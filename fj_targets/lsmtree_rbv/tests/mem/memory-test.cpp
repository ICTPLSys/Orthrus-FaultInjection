#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <thread>
#include <iostream>
#ifndef NOMIMALLOC
#include "mimalloc.h"
#endif
#include "profile-mem.hpp"

int main() {
    #ifndef NOMIMALLOC
    mi_option_set(mi_option_t::mi_option_show_stats, 1);
    mi_option_set(mi_option_t::mi_option_show_errors, 1);
    mi_option_set(mi_option_t::mi_option_verbose, 1);
    #endif

    profile::mem::init_mem("mem-test.log");
    profile::mem::start();

    sleep(5);

    int* ptrs[100];
    for (int i = 0; i < 10; i++) {
        sleep(1);
        printf("allocing: %d\n", i);
        const int size = 1024*1024/sizeof(int);
        ptrs[i] = (int*)malloc(sizeof(int)*size); // 100MiB
        for (int j = 0; j < size; j++) {
            ptrs[i][j] = rand();
        }
    }
    puts("deallocing");
    sleep(5);
    for (int i = 0; i < 10; i++) {
        sleep(1);
        printf("deallocing: %d\n", i);
        free(ptrs[i]);
    }
    sleep(5);
    profile::mem::stop();
}
