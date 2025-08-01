// this file implements the a test memcpy function by avx2 intrinsics

#include <immintrin.h>
#include <iostream>


__attribute__((noinline)) void simd_kernel(__m256i* d, const __m256i* s) {
    _mm256_storeu_si256(d, _mm256_loadu_si256(s));
}

__attribute__((noinline)) void memcpy_avx2(void* dest, const void* src, size_t n) {
    __m256i* d = (__m256i*)dest;
    const __m256i* s = (const __m256i*)src;

    size_t i;
    for (i = 0; i <= n - 32; i += 32) {
        simd_kernel(d++, s++);
    }
    for (; i < n; i++) {
        ((char*)d)[i] = ((char*)s)[i];
    }
}

int main() {
    const size_t N = 1024;
    char* src = (char*)malloc(N);
    char* dest = (char*)malloc(N);
    // generate random data
    // src = "I got a smoke";
    for (size_t i = 0; i < N; i++) {
        src[i] = rand() % 256;
    }
    memcpy_avx2(dest, src, N);
    // check if the data is correct
    for (size_t i = 0; i < N; i++) {
        if (dest[i] != src[i]) {
            std::cout << "error at " << i << std::endl;
            // break;
        }
    }

    // std::cout << "src: " << src << std::endl;
    // std::cout << "dest: " << dest << std::endl;
    free(src);
    free(dest);
}