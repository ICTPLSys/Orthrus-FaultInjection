#include <iostream>
#include <random>

__attribute__((noinline)) double calc(double *a, double *b, double *c, size_t n) {
    double result = 0;
    for (size_t i = 0; i < n; i++) {
        result += a[i] * b[i] + c[i];
    }
    return result;
}

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 10.0);

    double vec_a[4000], vec_b[4000], vec_c[4000];
    for (int i = 0; i < 4000; i++) {
        vec_a[i] = dis(gen);
        vec_b[i] = dis(gen);
        vec_c[i] = dis(gen);
    }

    double result = 0;
    double result_ref = 0;
    for (int i = 0; i < 4000; i++) {
        result += calc(vec_a, vec_b, vec_c, 4000);
        result_ref += vec_a[i] * vec_b[i] + vec_c[i];
    }
    std::cout << "result is " << result << std::endl;
    std::cout << "result_ref is " << result_ref << std::endl;

    return 0;
}
