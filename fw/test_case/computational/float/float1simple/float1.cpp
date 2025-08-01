#include <iostream>
#include <random>

__attribute__((noinline)) double calc(double a, double b, double c) {
    return a * b + c;
}

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 10.0);

    double a = dis(gen);
    double b = dis(gen);
    double c = dis(gen);

    double result = calc(a, b, c);
    double result_ref = a * b + c;

    std::cout << "result is " << result << std::endl;
    std::cout << "result_ref is " << result_ref << std::endl;

    return 0;
}
