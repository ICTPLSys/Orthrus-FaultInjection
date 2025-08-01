#include <iostream>
#include <random>

#define NELEMS 1000

double A[NELEMS + 1][NELEMS + 1];
double B[NELEMS + 1][NELEMS + 1];
double B_ref[NELEMS + 1][NELEMS + 1];

__attribute__((noinline)) double stencil_kernel(double a, double b, double c, double d, double e) {
    e = 1.0 * a + 2.0 * b + 3.0 *c + 4.0 *d + 5.0 * e;
    return e;    
}

__attribute__((noinline)) double stencil_kernel_ref(double a, double b, double c, double d, double e) {
    e = 1.0 * a + 2.0 * b + 3.0 *c + 4.0 *d + 5.0 * e;
    return e;    
}


__attribute__((noinline)) void calc(void) {
    for (int i = 1; i < NELEMS; i++) {
        for (int j = 1; j < NELEMS; j++) {
            B[i][j] = stencil_kernel(A[i-1][j-1], A[i-1][j], A[i-1][j+1], A[i][j-1], A[i][j]);
        }
    }
}

__attribute__((noinline)) void calc_ref(void) {
    for (int i = 1; i < NELEMS; i++) {
        for (int j = 1; j < NELEMS; j++) {
            B_ref[i][j] = stencil_kernel_ref(A[i-1][j-1], A[i-1][j], A[i-1][j+1], A[i][j-1], A[i][j]);
        }
    }
}


int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 10.0);

    for (int i = 0; i < NELEMS + 1; i++) {
        for (int j = 0; j < NELEMS + 1; j++) {
            A[i][j] = dis(gen);
        }
    }

    calc();

    double sum = 0.0;
    for (int i = 1; i < NELEMS; i++) {
        for (int j = 1; j < NELEMS; j++) {
            sum += B[i][j];
        }
    }

    calc_ref();

    double sum_ref = 0.0;
    for (int i = 1; i < NELEMS; i++) {
        for (int j = 1; j < NELEMS; j++) {
            sum_ref += B_ref[i][j];
        }
    }

    std::cout << "result is " << sum << std::endl;
    std::cout << "result_ref is " << sum_ref << std::endl;

    return 0;
}
