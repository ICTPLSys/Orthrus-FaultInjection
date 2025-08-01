#pragma once

#include "scee.hpp"

namespace origin {

inline int *new_ptr(int value) { return new int(value); }

inline void delete_ptr(int *ptr) { delete ptr; }

}  // namespace origin

namespace app {

inline int *new_ptr(int value) {
    int *ptr = new int(value);
    scee::append_log_typed(ptr);
    return ptr;
}

inline void delete_ptr(int *ptr) {
    scee::append_log_typed(ptr);
    // its the validator's job to delete the ptr
    // TODO(unknown): what if the validation task is skipped?
}

}  // namespace app

namespace validator {

inline int *new_ptr(int value) {
    int *ptr;
    scee::log_reader.fetch_log_typed(&ptr);
    return ptr;
}

inline void delete_ptr(int *ptr) {
    scee::log_reader.cmp_log_typed(ptr);
    delete ptr;
}

}  // namespace validator

#ifndef app_namespace
#define app_namespace origin
#endif
