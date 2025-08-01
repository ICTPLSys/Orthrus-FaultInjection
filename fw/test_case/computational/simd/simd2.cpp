#include <iostream>
#include <iterator>
#include <random>
#include <immintrin.h>
// simd avx512 version of float.cpp
// A * B + C
__attribute__((noinline)) void calc(double *A, double *B, double *C, double* D, int n) {
    for (int i = 0; i < n; i += 8) {
        __m512d a = _mm512_loadu_pd(&A[i]);
        __m512d b = _mm512_loadu_pd(&B[i]);
        __m512d c = _mm512_loadu_pd(&C[i]);
        __m512d d = _mm512_add_pd(a, b);
        __m512d e = _mm512_mul_pd(d, c);
        _mm512_storeu_pd(&D[i], e);
    }
}

__attribute__((noinline)) void calc_ref(double *A, double *B, double *C, double* D, int n) {
    for (int i = 0; i < n; i += 8) {
        __m512d a = _mm512_loadu_pd(&A[i]);
        __m512d b = _mm512_loadu_pd(&B[i]);
        __m512d c = _mm512_loadu_pd(&C[i]);
        __m512d d = _mm512_add_pd(a, b);
        __m512d e = _mm512_mul_pd(d, c);
        _mm512_storeu_pd(&D[i], e);
    }
}

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 10.0);

    double A[8], B[8], C[8];

    for (int i = 0; i < 8; ++i) {
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

    double D[8];

    calc(A, B, C, D, 8);
    std::cout << std::endl << "result: ";
    for (double d : D) {
        std::cout << d << " ";
    }
    std::cout << std::endl;

    calc_ref(A, B, C, D, 8);
    std::cout << "result_ref: ";
    for (double d : D) {
        std::cout << d << " ";
    }
    std::cout << std::endl;

    return 0;
}
