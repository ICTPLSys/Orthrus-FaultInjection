#pragma once

#include <immintrin.h>
#include <x86intrin.h>

#include "memtypes.hpp"

namespace scee {

#ifndef NO_CRC
static __attribute__((target("sse4.2"))) uint32_t calculate_crc32(
    const void* data, std::size_t length) {
    std::uint32_t crc = ~0U;
    const auto* buffer = static_cast<const unsigned char*>(data);

    while (length > 0 && reinterpret_cast<std::uintptr_t>(buffer) % 8 != 0) {
        crc = _mm_crc32_u8(crc, *buffer);
        buffer++;
        length--;
    }

    const auto* buffer64 = reinterpret_cast<const std::uint64_t*>(buffer);
    while (length >= 8) {
        crc = _mm_crc32_u64(crc, *buffer64);
        buffer64++;
        length -= 8;
    }

    buffer = reinterpret_cast<const unsigned char*>(buffer64);

    if (length >= 4) {
        auto value32 = *reinterpret_cast<const std::uint32_t*>(buffer);
        crc = _mm_crc32_u32(crc, value32);
        buffer += 4;
        length -= 4;
    }

    if (length >= 2) {
        auto value16 = *reinterpret_cast<const std::uint16_t*>(buffer);
        crc = _mm_crc32_u16(crc, value16);
        buffer += 2;
        length -= 2;
    }

    if (length > 0) {
        crc = _mm_crc32_u8(crc, *buffer);
    }

    return ~crc;
}

inline checksum_t compute_checksum(const void* ptr, size_t size) {
    return calculate_crc32(ptr, size);
}

#else

inline checksum_t compute_checksum(const void* ptr, size_t size) { return 0; }

#endif

}  // namespace scee
