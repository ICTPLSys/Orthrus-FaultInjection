#include <iostream>
#include <iterator>
#include <random>
#include <immintrin.h>
// simd avx2 version of float.cpp
// A * B + C
__attribute__((noinline)) void calc(double *A, double *B, double *C, double* D, int n) {
    for (int i = 0; i < n; i += 4) {
        __m256d a = _mm256_loadu_pd(&A[i]);
        __m256d b = _mm256_loadu_pd(&B[i]);
        __m256d c = _mm256_loadu_pd(&C[i]);
        __m256d d = _mm256_add_pd(a, b);
        __m256d e = _mm256_mul_pd(d, c);
        _mm256_storeu_pd(&D[i], e);
    }
}

__attribute__((noinline)) void calc_ref(double *A, double *B, double *C, double* D, int n) {
    for (int i = 0; i < n; i += 4) {
        __m256d a = _mm256_loadu_pd(&A[i]);
        __m256d b = _mm256_loadu_pd(&B[i]);
        __m256d c = _mm256_loadu_pd(&C[i]);
        __m256d d = _mm256_add_pd(a, b);
        __m256d e = _mm256_mul_pd(d, c);
        _mm256_storeu_pd(&D[i], e);
    }
}

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 10.0);

    double A[4], B[4], C[4];

    for (int i = 0; i < 4; ++i) {
        A[i] = dis(gen);
        B[i] = dis(gen);
        C[i] = dis(gen);
    }

    std::cout << "A: ";
    for (double a : A) {
        std::cout << a << " ";
    }
    std::cout << std::endl;

    std::cout << "B: ";
    for (double b : B) {
        std::cout << b << " ";
    }
    std::cout << std::endl;

    std::cout << "C: ";
    for (double c : C) {
        std::cout << c << " ";
    }

    double D[4];

    calc(A, B, C, D, 4);
    std::cout << std::endl << "result: ";
    for (double d : D) {
        std::cout << d << " ";
    }
    std::cout << std::endl;

    calc_ref(A, B, C, D, 4);
    std::cout << "result_ref: ";
    for (double d : D) {
        std::cout << d << " ";
    }
    std::cout << std::endl;

    return 0;
}
