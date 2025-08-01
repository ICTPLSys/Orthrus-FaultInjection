#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

using ref_count_t = std::atomic<uint32_t>;
using checksum_t = uint32_t;
