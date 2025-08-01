#include <immintrin.h>
#include <x86intrin.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>

#include "checksum.hpp"

__attribute__((target("sse4.2"))) uint32_t calculate_crc32(const void* data,
                                                           std::size_t length) {
    // 初始化CRC值为全1（这是CRC32的标准初始值）
    std::uint32_t crc = ~0U;
    const auto* buffer = static_cast<const unsigned char*>(data);

    // 处理未对齐的前缀字节，直到达到8字节对齐
    while (length > 0 && reinterpret_cast<std::uintptr_t>(buffer) % 8 != 0) {
        crc = _mm_crc32_u8(crc, *buffer);
        buffer++;
        length--;
    }

    // 以8字节为单位进行处理（最高效）
    const auto* buffer64 = reinterpret_cast<const std::uint64_t*>(buffer);
    while (length >= 8) {
        crc = _mm_crc32_u64(crc, *buffer64);
        buffer64++;
        length -= 8;
    }

    // 处理剩余的字节
    buffer = reinterpret_cast<const unsigned char*>(buffer64);

    // 尝试以4字节处理
    if (length >= 4) {
        auto value32 = *reinterpret_cast<const std::uint32_t*>(buffer);
        crc = _mm_crc32_u32(crc, value32);
        buffer += 4;
        length -= 4;
    }

    // 尝试以2字节处理
    if (length >= 2) {
        auto value16 = *reinterpret_cast<const std::uint16_t*>(buffer);
        crc = _mm_crc32_u16(crc, value16);
        buffer += 2;
        length -= 2;
    }

    // 处理最后可能剩余的1个字节
    if (length > 0) {
        crc = _mm_crc32_u8(crc, *buffer);
    }

    // 对结果取反，得到最终的CRC32值
    return ~crc;
}

void bench_sse_crc(size_t size, size_t repeat, bool warmup = false) {
    auto* data = (uint8_t*)aligned_alloc(64, size);
    std::default_random_engine engine(0);
    std::uniform_int_distribution<uint8_t> distribution(0, 255);
    for (size_t i = 0; i < size; ++i) {
        data[i] = distribution(engine);
    }

    uint64_t start = _rdtsc();
    volatile uint32_t crc;
    for (size_t i = 0; i < repeat; ++i) {
        crc = calculate_crc32(data, size);
    }
    uint64_t end = _rdtsc();
    if (!warmup) {
        printf("size: %zu, cycles: %lu\n", size, (end - start) / repeat);
    }

    free(data);
}

void bench_checksum(size_t size, size_t repeat, bool warmup = false) {
    auto* data = (uint8_t*)aligned_alloc(64, size);
    std::default_random_engine engine(0);
    std::uniform_int_distribution<uint8_t> distribution(0, 255);
    for (size_t i = 0; i < size; ++i) {
        data[i] = distribution(engine);
    }

    uint64_t start = _rdtsc();
    volatile uint32_t crc;
    for (size_t i = 0; i < repeat; ++i) {
        crc = scee::compute_checksum(data, size);
    }
    uint64_t end = _rdtsc();
    if (!warmup) {
        printf("size: %zu, cycles: %lu\n", size, (end - start) / repeat);
    }

    free(data);
}

void bench_memcpy(size_t size, size_t repeat, bool warmup = false) {
    auto* data = (uint8_t*)aligned_alloc(64, size);
    auto* dst = (uint8_t*)aligned_alloc(64, size);
    std::default_random_engine engine(0);
    std::uniform_int_distribution<uint8_t> distribution(0, 255);
    for (size_t i = 0; i < size; ++i) {
        data[i] = distribution(engine);
    }

    uint64_t start = _rdtsc();
    for (size_t i = 0; i < repeat; ++i) {
        memcpy(dst, data, size);
    }
    uint64_t end = _rdtsc();
    if (!warmup) {
        printf("size: %zu, cycles: %lu\n", size, (end - start) / repeat);
    }

    free(data);
    free(dst);
}

int main() {
    size_t repeat = 1000000;
    // warm up
    bench_sse_crc(8, repeat, true);
    bench_checksum(8, repeat, true);
    bench_memcpy(8, repeat, true);
    printf("--------------------------------\n");
    printf("sse crc32\n");
    for (size_t size = 8; size <= 4096; size *= 2) {
        bench_sse_crc(size, repeat);
    }
    printf("--------------------------------\n");
    printf("isal crc32 gzip refl\n");
    for (size_t size = 8; size <= 4096; size *= 2) {
        bench_checksum(size, repeat);
    }
    printf("--------------------------------\n");
    printf("memcpy\n");
    for (size_t size = 8; size <= 4096; size *= 2) {
        bench_memcpy(size, repeat);
    }
    return 0;
}
