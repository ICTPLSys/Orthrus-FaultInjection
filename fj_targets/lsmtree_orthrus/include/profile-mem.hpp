#pragma once

#include <cstdint>

namespace profile::mem {
void init_mem(const char* filename);
void start();
void stop();
}  // namespace profile
