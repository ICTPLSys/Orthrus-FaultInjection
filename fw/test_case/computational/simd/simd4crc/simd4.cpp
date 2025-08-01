// use instructions like _mm_crc32_u64 to calc CRC, not need to use avx.

#include <immintrin.h>
#include <iostream>
#include <vector>

#include <cstdint>

static __attribute__((target("sse4.2"), noinline)) uint32_t
calculate_crc32(const void* data, std::size_t length) {
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

static __attribute__((target("sse4.2"), noinline)) uint32_t
calculate_crc32_ref(const void* data, std::size_t length) {
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

int main() {
    std::vector<uint8_t> data(1024);
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = i;
    }
    std::cout << "CRC32: " << calculate_crc32(data.data(), data.size()) << std::endl;
    std::cout << "CRC32_REF: " << calculate_crc32_ref(data.data(), data.size()) << std::endl;
}