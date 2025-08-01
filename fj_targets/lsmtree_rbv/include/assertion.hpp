// TODO(unknown): allow fault injection for these functions

#pragma once

#include <cstdio>
#include <cstdlib>

static inline void validation_failed() {
    fprintf(stderr, "Validation failed\n");
    std::abort();
}

#define validator_assert(condition)                                    \
    if (!(condition)) {                                                \
        fprintf(stderr, "Validation failed at %s:%d (%s)\n", __FILE__, \
                __LINE__, #condition);                                 \
        validation_failed();                                           \
    }
